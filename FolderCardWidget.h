#ifndef FOLDERCARDWIDGET_H
#define FOLDERCARDWIDGET_H

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QDesktopServices>
#include <QUrl>
#include <QContextMenuEvent>
#include <QMenu>
#include <QClipboard>
#include <QApplication>
#include <QCoreApplication>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QFileInfo>
#include <QDir>
#include <QMouseEvent>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <Windows.h>
#include <shellapi.h>
#include "DatabaseManager.h"

namespace {
QString appSettingsPath()
{
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (dirPath.isEmpty()) {
        dirPath = QCoreApplication::applicationDirPath();
    }

    return QDir(dirPath).filePath("settings.ini");
}

QString commandLineEnvScriptPath(const QString& key)
{
    QSettings settings(appSettingsPath(), QSettings::IniFormat);
    return settings.value(key, "").toString().trimmed();
}

QString quotedCmdArg(const QString& value)
{
    return QString("\"%1\"").arg(QDir::toNativeSeparators(value));
}

QString highlightText(const QString& text, const QString& keyword)
{
    const QString trimmed = keyword.trimmed();
    QString escapedText = text.toHtmlEscaped();
    if (trimmed.isEmpty()) {
        return escapedText;
    }

    const QString escapedKeyword = trimmed.toHtmlEscaped();
    int index = escapedText.indexOf(escapedKeyword, 0, Qt::CaseInsensitive);
    while (index >= 0) {
        const QString matched = escapedText.mid(index, escapedKeyword.length());
        const QString replacement = QString("<span style=\"background-color:#fff3a3;color:#20242a;\">%1</span>").arg(matched);
        escapedText.replace(index, escapedKeyword.length(), replacement);
        index = escapedText.indexOf(escapedKeyword, index + replacement.length(), Qt::CaseInsensitive);
    }
    return escapedText;
}

void openCommandPrompt(const QString& path, const QString& envScriptPath = QString())
{
    const QString workingDir = QDir::toNativeSeparators(path);
    QString parameters = QStringLiteral("/K");
    if (QFileInfo(envScriptPath).isFile()) {
        const QString command = QString("%1 && cd /d %2")
                .arg(quotedCmdArg(envScriptPath), quotedCmdArg(workingDir));
        parameters = QString("/K \"%1\"").arg(command);
    }

    ShellExecuteW(nullptr,
                  L"open",
                  L"cmd.exe",
                  reinterpret_cast<LPCWSTR>(parameters.utf16()),
                  reinterpret_cast<LPCWSTR>(workingDir.utf16()),
                  SW_SHOWNORMAL);
}
}

class FolderCardWidget : public QWidget {
    Q_OBJECT
public:
    explicit FolderCardWidget(const FFolderItem& item,
                              int mode,
                              int index,
                              const QString& keyword = QString(),
                              QWidget *parent = nullptr)
        : QWidget(parent), m_item(item), m_mode(mode), m_keyword(keyword) {

        auto *mainLayout = new QHBoxLayout(this);
        mainLayout->setContentsMargins(10, 5, 10, 5); // 适当留白
        mainLayout->setSpacing(10);

        // 1. 序号
        QLabel *indexLabel = new QLabel(QString::number(index + 1) + ".", this);
        indexLabel->setFixedWidth(30);

        // 2. 时间 (仅模式0/Timeline显示)
        QLabel *timeLabel = nullptr;
        if (m_mode == 0) {
            timeLabel = new QLabel(m_item.lastTime, this);
            timeLabel->setStyleSheet("color: gray;");
        }

        // 3. 内容展示 (别名或路径)
        QLabel *contentLabel = new QLabel(this);
        contentLabel->setTextFormat(Qt::RichText);
        if (m_item.isFavorite || (m_mode == 0 && m_item.isPinned)) {
            QStringList markers;
            if (m_mode == 0 && m_item.isPinned) markers << "[🔝]";
            if (m_item.isFavorite) markers << "[🌟]";
            contentLabel->setText(markers.join(" ") + " " + highlightText(m_item.displayName(), m_keyword));
            contentLabel->setProperty("isFavorite", true);
        } else {
            // 注意：在 ListWidget 中，通常由容器控制宽度，此处设置占位
            contentLabel->setText(highlightText(m_item.path, m_keyword));
            contentLabel->setProperty("isFavorite", false);
        }
        contentLabel->setToolTip(m_item.path);

        mainLayout->addWidget(indexLabel);
        if (timeLabel) mainLayout->addWidget(timeLabel);
        mainLayout->addWidget(contentLabel, 1);

        // 保持双击逻辑
        this->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    }

protected:
    // 将双击和右键逻辑保留，QListWidget 会透传这些事件到 ItemWidget
    void mouseDoubleClickEvent(QMouseEvent *) override {
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_item.path));
    }

    void contextMenuEvent(QContextMenuEvent *event) override {
        QMenu menu(this);
        auto *openExplorerAct = menu.addAction("在资源管理器中打开");
        auto *openCmdMenu = menu.addMenu("在命令行打开");
        auto *openCmdAct = openCmdMenu->addAction("普通 CMD");
        auto *openQtCmdAct = openCmdMenu->addAction("Qt 环境");
        auto *openVsCmdAct = openCmdMenu->addAction("VS 环境");
        auto *openCustomCmdAct = openCmdMenu->addAction("自定义环境");
        openQtCmdAct->setEnabled(QFileInfo(commandLineEnvScriptPath("commandLine/qtEnvScriptPath")).isFile());
        openVsCmdAct->setEnabled(QFileInfo(commandLineEnvScriptPath("commandLine/vsEnvScriptPath")).isFile());
        openCustomCmdAct->setEnabled(QFileInfo(commandLineEnvScriptPath("commandLine/customEnvScriptPath")).isFile());
        auto *copyAct = menu.addAction("复制路径");
        auto *deleteAct = menu.addAction("删除记录");
        QAction *pinAct = nullptr;
        if (m_mode == 0) {
            pinAct = m_item.isPinned ? menu.addAction("取消置顶") : menu.addAction("置顶到时间线");
        }
        auto *favAct = (m_item.isFavorite) ? menu.addAction("取消收藏") : menu.addAction("收藏项目");

        auto *res = menu.exec(event->globalPos());
        if (res == openExplorerAct) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(m_item.path));
        } else if (res == openCmdAct) {
            openCommandPrompt(m_item.path);
        } else if (res == openQtCmdAct) {
            openCommandPrompt(m_item.path, commandLineEnvScriptPath("commandLine/qtEnvScriptPath"));
        } else if (res == openVsCmdAct) {
            openCommandPrompt(m_item.path, commandLineEnvScriptPath("commandLine/vsEnvScriptPath"));
        } else if (res == openCustomCmdAct) {
            openCommandPrompt(m_item.path, commandLineEnvScriptPath("commandLine/customEnvScriptPath"));
        } else if (res == copyAct) {
            QApplication::clipboard()->setText(m_item.path);
        } else if (res == deleteAct) {
            const auto reply = QMessageBox::question(this,
                                                     "删除记录",
                                                     QString("确定删除这条记录吗？\n%1").arg(m_item.path));
            if (reply == QMessageBox::Yes) {
                DatabaseManager::removePath(m_item.path);
                emit dataChanged();
            }
        } else if (pinAct && res == pinAct) {
            DatabaseManager::setPinned(m_item.path, !m_item.isPinned);
            emit dataChanged();
        } else if (res == favAct) {
            if (m_item.isFavorite) {
                DatabaseManager::setFavorite(m_item.path, false);
            } else {
                bool ok;
                QString alias = QInputDialog::getText(this, "别名", "设置别名:", QLineEdit::Normal, QFileInfo(m_item.path).fileName(), &ok);
                if (ok && !alias.isEmpty()) DatabaseManager::setFavorite(m_item.path, true, alias);
            }
            emit dataChanged();
        }
    }

signals:
    void dataChanged();

private:
    FFolderItem m_item;
    int m_mode;
    QString m_keyword;
};

#endif
