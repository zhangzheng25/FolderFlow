#include "DatabaseManager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QStringList>

namespace {
QStringList g_ignoredKeywords;
}

bool DatabaseManager::init(int keepDays)
{
    QSqlDatabase db = QSqlDatabase::contains()
            ? QSqlDatabase::database()
            : QSqlDatabase::addDatabase("QSQLITE");

    const QString dbPath = databasePath();
    migrateLegacyDatabase(dbPath);

    db.setDatabaseName(dbPath);
    if (!db.open()) {
        qDebug() << "Database open error:" << db.lastError().text();
        return false;
    }

    QSqlQuery query(db);
    if (!query.exec("CREATE TABLE IF NOT EXISTS folder_data ("
                    "path TEXT PRIMARY KEY, "
                    "last_time DATETIME, "
                    "access_count INTEGER DEFAULT 0, "
                    "is_favorite INTEGER DEFAULT 0, "
                    "is_pinned INTEGER DEFAULT 0, "
                    "alias_name TEXT)")) {
        qDebug() << "Create table error:" << query.lastError().text();
        return false;
    }

    if (!query.exec("ALTER TABLE folder_data ADD COLUMN is_pinned INTEGER DEFAULT 0")) {
        const QString error = query.lastError().text();
        if (!error.contains("duplicate column name", Qt::CaseInsensitive)) {
            qDebug() << "Add is_pinned column error:" << error;
            return false;
        }
    }

    cleanupExpiredAccessData(keepDays);
    return true;
}

void DatabaseManager::cleanupExpiredAccessData(int keepDays)
{
    QSqlQuery query;
    query.prepare("DELETE FROM folder_data "
                  "WHERE is_favorite = 0 "
                  "AND is_pinned = 0 "
                  "AND (last_time IS NULL OR last_time < datetime('now', 'localtime', ?))");
    query.addBindValue(QString("-%1 days").arg(keepDays));

    if (!query.exec()) {
        qDebug() << "cleanupExpiredAccessData Error:" << query.lastError().text();
    }
}

void DatabaseManager::clearHistory()
{
    QSqlQuery deleteQuery;
    deleteQuery.prepare("DELETE FROM folder_data WHERE is_favorite = 0 AND is_pinned = 0");
    if (!deleteQuery.exec()) {
        qDebug() << "clearHistory delete Error:" << deleteQuery.lastError().text();
    }

    QSqlQuery resetQuery;
    resetQuery.prepare("UPDATE folder_data SET access_count = 0 WHERE is_favorite = 1 OR is_pinned = 1");
    if (!resetQuery.exec()) {
        qDebug() << "clearHistory reset Error:" << resetQuery.lastError().text();
    }
}

void DatabaseManager::setIgnoredKeywords(const QStringList& keywords)
{
    g_ignoredKeywords.clear();
    for (const QString& keyword : keywords) {
        const QString trimmed = keyword.trimmed();
        if (!trimmed.isEmpty()) {
            g_ignoredKeywords.append(trimmed);
        }
    }
}

void DatabaseManager::logAccess(const QString& path)
{
    if (isIgnored(path)) {
        return;
    }

    QSqlQuery query;
    query.prepare("INSERT INTO folder_data (path, last_time, access_count) "
                  "VALUES (?, datetime('now', 'localtime'), 1) "
                  "ON CONFLICT(path) DO UPDATE SET "
                  "last_time = datetime('now', 'localtime'), "
                  "access_count = access_count + 1");
    query.addBindValue(path);

    if (!query.exec()) {
        qDebug() << "logAccess Error:" << query.lastError().text();
    }
}

void DatabaseManager::setFavorite(const QString& path, bool fav, const QString& alias)
{
    QSqlQuery query;
    if (fav) {
        query.prepare("INSERT INTO folder_data (path, is_favorite, alias_name) VALUES (?, 1, ?) "
                      "ON CONFLICT(path) DO UPDATE SET "
                      "is_favorite = 1, "
                      "alias_name = CASE "
                      "WHEN excluded.alias_name != '' THEN excluded.alias_name "
                      "ELSE folder_data.alias_name END");
        query.addBindValue(path);
        query.addBindValue(alias);
    } else {
        query.prepare("UPDATE folder_data SET is_favorite = 0, alias_name = NULL WHERE path = ?");
        query.addBindValue(path);
    }

    if (!query.exec()) {
        qDebug() << "setFavorite Error:" << query.lastError().text();
    }
}

void DatabaseManager::setPinned(const QString& path, bool pinned)
{
    QSqlQuery query;
    if (pinned) {
        query.prepare("INSERT INTO folder_data (path, is_pinned, last_time, access_count) "
                      "VALUES (?, 1, datetime('now', 'localtime'), 0) "
                      "ON CONFLICT(path) DO UPDATE SET is_pinned = 1");
        query.addBindValue(path);
    } else {
        query.prepare("UPDATE folder_data SET is_pinned = 0 WHERE path = ?");
        query.addBindValue(path);
    }

    if (!query.exec()) {
        qDebug() << "setPinned Error:" << query.lastError().text();
    }
}

QList<FFolderItem> DatabaseManager::getTimeline(int limit, const QString& keyword) const
{
    QSqlQuery query;
    QString sql = "SELECT * FROM folder_data ";
    const bool hasKeyword = !keyword.trimmed().isEmpty();
    if (hasKeyword) {
        sql += "WHERE (path LIKE ? OR alias_name LIKE ?) ";
    }
    sql += hasKeyword
            ? "ORDER BY is_pinned DESC, COALESCE(NULLIF(alias_name, ''), path) ASC LIMIT ?"
            : "ORDER BY is_pinned DESC, last_time DESC LIMIT ?";

    query.prepare(sql);
    bindKeyword(query, keyword);
    query.addBindValue(limit);
    return executeQuery(query);
}

QList<FFolderItem> DatabaseManager::getHotRanking(double weightA,
                                                  double weightB,
                                                  int days,
                                                  int limit,
                                                  const QString& keyword) const
{
    QSqlQuery query;
    QString sql =
            "SELECT *, "
            "((access_count * ?) + "
            "((1.0 / (julianday('now', 'localtime') - julianday(last_time) + 0.001)) * ?)) "
            "AS rank_score "
            "FROM folder_data "
            "WHERE is_favorite = 0 "
            "AND last_time > datetime('now', 'localtime', ?) ";

    if (!keyword.trimmed().isEmpty()) {
        sql += "AND (path LIKE ? OR alias_name LIKE ?) ";
    }

    sql += "ORDER BY rank_score DESC LIMIT ?";

    query.prepare(sql);
    query.addBindValue(weightA);
    query.addBindValue(weightB);
    query.addBindValue(QString("-%1 days").arg(days));
    bindKeyword(query, keyword);
    query.addBindValue(limit);
    return executeQuery(query);
}

QList<FFolderItem> DatabaseManager::getFavorites(const QString& keyword) const
{
    QSqlQuery query;
    QString sql = "SELECT * FROM folder_data WHERE is_favorite = 1 ";
    if (!keyword.trimmed().isEmpty()) {
        sql += "AND (path LIKE ? OR alias_name LIKE ?) ";
    }
    sql += "ORDER BY COALESCE(NULLIF(alias_name, ''), path) ASC";

    query.prepare(sql);
    bindKeyword(query, keyword);
    return executeQuery(query);
}

QString DatabaseManager::databasePath()
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataDir.isEmpty()) {
        dataDir = QCoreApplication::applicationDirPath();
    }

    QDir dir(dataDir);
    if (!dir.exists() && !dir.mkpath(".")) {
        qDebug() << "Create database directory failed:" << dataDir;
        return QCoreApplication::applicationDirPath() + "/folder_flow.db";
    }

    return dir.filePath("folder_flow.db");
}

void DatabaseManager::migrateLegacyDatabase(const QString& targetPath)
{
    if (QFile::exists(targetPath)) {
        return;
    }

    const QStringList legacyPaths = {
        QDir::current().filePath("folder_flow.db"),
        QCoreApplication::applicationDirPath() + "/folder_flow.db"
    };

    const QString canonicalTarget = QFileInfo(targetPath).canonicalFilePath();
    for (const QString& legacyPath : legacyPaths) {
        QFileInfo legacyInfo(legacyPath);
        if (!legacyInfo.exists() || legacyInfo.canonicalFilePath() == canonicalTarget) {
            continue;
        }

        if (!QFile::copy(legacyPath, targetPath)) {
            qDebug() << "Legacy database migration failed:" << legacyPath << "->" << targetPath;
        }
        return;
    }
}

void DatabaseManager::bindKeyword(QSqlQuery& query, const QString& keyword)
{
    const QString trimmed = keyword.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }

    const QString pattern = "%" + trimmed + "%";
    query.addBindValue(pattern);
    query.addBindValue(pattern);
}

bool DatabaseManager::isIgnored(const QString& path, const QString& alias)
{
    if (g_ignoredKeywords.isEmpty()) {
        return false;
    }

    Q_UNUSED(alias);
    const QString folderName = QFileInfo(path).fileName();
    for (const QString& keyword : g_ignoredKeywords) {
        if (folderName.compare(keyword, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }

    return false;
}

QList<FFolderItem> DatabaseManager::executeQuery(QSqlQuery& query)
{
    QList<FFolderItem> result;
    if (query.exec()) {
        while (query.next()) {
            FFolderItem item;
            item.path = query.value("path").toString();
            item.alias = query.value("alias_name").toString();
            item.isFavorite = query.value("is_favorite").toInt() == 1;
            item.isPinned = query.value("is_pinned").toInt() == 1;
            item.lastTime = query.value("last_time").toDateTime().toString("yyyy-MM-dd HH:mm:ss");
            if (!isIgnored(item.path, item.alias)) {
                result.append(item);
            }
        }
    } else {
        qDebug() << "SQL Error:" << query.lastError().text();
    }
    return result;
}
