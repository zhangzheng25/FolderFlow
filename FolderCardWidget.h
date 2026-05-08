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
#include <QInputDialog>
#include <QLineEdit>
#include <QFileInfo>
#include <QMouseEvent>
#include <QProcess>
#include <QStringList>
#include "DatabaseManager.h"

class FolderCardWidget : public QWidget {
    Q_OBJECT
public:
    explicit FolderCardWidget(const FFolderItem& item, int mode, int index, QWidget *parent = nullptr)
        : QWidget(parent), m_item(item), m_mode(mode) {

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
        if (m_item.isFavorite || (m_mode == 0 && m_item.isPinned)) {
            QStringList markers;
            if (m_mode == 0 && m_item.isPinned) markers << "[🔝]";
            if (m_item.isFavorite) markers << "[🌟]";
            contentLabel->setText(markers.join(" ") + " " + m_item.displayName());
            contentLabel->setProperty("isFavorite", true);
        } else {
            // 注意：在 ListWidget 中，通常由容器控制宽度，此处设置占位
            contentLabel->setText(m_item.path);
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
        auto *openCmdAct = menu.addAction("在命令行打开");
        auto *copyAct = menu.addAction("复制路径");
        QAction *pinAct = nullptr;
        if (m_mode == 0) {
            pinAct = m_item.isPinned ? menu.addAction("取消置顶") : menu.addAction("置顶到时间线");
        }
        auto *favAct = (m_item.isFavorite) ? menu.addAction("取消收藏") : menu.addAction("收藏项目");

        auto *res = menu.exec(event->globalPos());
        if (res == openCmdAct) {
            QProcess::startDetached("cmd.exe", QStringList() << "/K", m_item.path);
        } else if (res == copyAct) {
            QApplication::clipboard()->setText(m_item.path);
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
};

#endif
