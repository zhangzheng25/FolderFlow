#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QList>
#include <QString>
#include <QStringList>

struct FFolderItem {
    QString path;
    QString alias;
    QString lastTime;
    bool isFavorite = false;
    bool isPinned = false;

    QString displayName() const {
        return alias.isEmpty() ? path : alias;
    }
};

class QSqlQuery;

class DatabaseManager {
public:
    static bool init(int keepDays = 7);
    static void cleanupExpiredAccessData(int keepDays = 7);
    static void clearHistory();
    static void setIgnoredKeywords(const QStringList& keywords);
    static void logAccess(const QString& path);
    static void setFavorite(const QString& path, bool fav, const QString& alias = "");
    static void setPinned(const QString& path, bool pinned);

    QList<FFolderItem> getTimeline(int limit = 50, const QString& keyword = QString()) const;
    QList<FFolderItem> getHotRanking(double weightA,
                                     double weightB,
                                     int days = 7,
                                     int limit = 10,
                                     const QString& keyword = QString()) const;
    QList<FFolderItem> getFavorites(const QString& keyword = QString()) const;

private:
    static QString databasePath();
    static void migrateLegacyDatabase(const QString& targetPath);
    static void bindKeyword(QSqlQuery& query, const QString& keyword);
    static bool isIgnored(const QString& path, const QString& alias = QString());
    static QList<FFolderItem> executeQuery(QSqlQuery& query);
};

#endif
