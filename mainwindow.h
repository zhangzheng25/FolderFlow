#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <DatabaseManager.h>
#include <FolderTracker.h>

#include <QAction>
#include <QCheckBox>
#include <QCloseEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QSystemTrayIcon>
#include <QTabWidget>
#include <QStringList>

class FolderListWidget;
class QWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    void showAndFocusSearch();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setupUi();
    void setupTrayIcon();
    void initFolderFlow();
    void initConnect();
    void loadSettings();
    void saveSettings() const;
    void applyStartupSetting(bool enabled) const;
    bool isStartupEnabled() const;
    QString settingsPath() const;
    QString currentSearchKeyword() const;
    QStringList ignoredKeywords() const;
    void applyIgnoredKeywords() const;
    void setupShortcuts();
    void setCompactMode(bool compact);
    void applyViewMode();
    void updateList();

private:
    FolderTracker *m_tracker = nullptr;
    DatabaseManager m_dbManager;

    QLineEdit *m_lineSearch = nullptr;
    QPushButton *m_btnSearch = nullptr;
    QPushButton *m_btnClearSearch = nullptr;
    QPushButton *m_btnViewMode = nullptr;
    QTabWidget *m_tabWidget = nullptr;
    QWidget *m_quickAccessWidget = nullptr;
    FolderListWidget *m_listTimeline = nullptr;
    FolderListWidget *m_listRanking = nullptr;
    FolderListWidget *m_listFavorite = nullptr;
    QLabel *m_labelFormula = nullptr;
    QLabel *m_labelCount = nullptr;
    QLabel *m_labelLatest = nullptr;
    QSlider *m_sliderWeight = nullptr;
    QSpinBox *m_spinQueryCount = nullptr;
    QSpinBox *m_spinQueryTime = nullptr;
    QSpinBox *m_spinKeepDays = nullptr;
    QLineEdit *m_lineIgnoreKeywords = nullptr;
    QLineEdit *m_lineCommandEnvScript = nullptr;
    QPushButton *m_btnBrowseCommandEnvScript = nullptr;
    QPushButton *m_btnClearCommandEnvScript = nullptr;
    QCheckBox *m_checkStartup = nullptr;
    QPushButton *m_btnClearHistory = nullptr;
    bool m_compactMode = false;

    QSystemTrayIcon *trayIcon = nullptr;
    QMenu *trayMenu = nullptr;
};

#endif // MAINWINDOW_H
