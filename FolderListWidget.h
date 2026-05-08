#ifndef FOLDERLISTWIDGET_H
#define FOLDERLISTWIDGET_H

#include <QListWidget>
#include <QListWidgetItem>
#include <QDesktopServices>
#include <QUrl>
#include "FolderCardWidget.h"

class FolderListWidget : public QListWidget {
    Q_OBJECT
public:
    enum ViewMode { TimelineMode, RankingMode, FavoriteMode };

    explicit FolderListWidget(QWidget *parent = nullptr)
        : QListWidget(parent), m_mode(TimelineMode) {

        this->setSpacing(2);
        this->setFrameShape(QFrame::NoFrame);
        this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        this->setSelectionMode(QAbstractItemView::SingleSelection);

        // 允许 QSS 样式控制
        this->setStyleSheet("QListWidget::item { border-bottom: 1px solid #f0f0f0; } "
                            "QListWidget::item:selected { background: #e5f1fb; }");
    }

    void setMode(ViewMode mode) { m_mode = mode; }

    void updateData(const QList<FFolderItem>& dataList, const QString& keyword = QString()) {
        this->setUpdatesEnabled(false);
        this->clear(); // 清空旧项
        m_dataList = dataList;

        for (int i = 0; i < dataList.size(); ++i) {
            auto *listItem = new QListWidgetItem(this);
            auto *card = new FolderCardWidget(dataList[i], (int)m_mode, i, keyword, this);

            // 关键：设置 Item 的大小提示，否则 Widget 可能显示不出来
            listItem->setSizeHint(card->sizeHint());

            // 转发信号
            connect(card, &FolderCardWidget::dataChanged, this, &FolderListWidget::favoriteChanged);

            this->addItem(listItem);
            this->setItemWidget(listItem, card);
        }

        this->setUpdatesEnabled(true);
    }

    bool openAt(int index) {
        if (index < 0 || index >= m_dataList.size()) {
            return false;
        }

        setCurrentRow(index);
        scrollToItem(item(index), QAbstractItemView::PositionAtCenter);
        return QDesktopServices::openUrl(QUrl::fromLocalFile(m_dataList[index].path));
    }

signals:
    void favoriteChanged();

private:
    ViewMode m_mode;
    QList<FFolderItem> m_dataList;
};

#endif
