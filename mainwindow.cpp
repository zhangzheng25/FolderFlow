#include "mainwindow.h"

#include "FolderListWidget.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QScreen>
#include <QShortcut>
#include <QSizePolicy>
#include <QSettings>
#include <QStandardPaths>
#include <QTabBar>
#include <QVBoxLayout>

namespace {
constexpr const char *APP_NAME = "FolderFlow";

QString qs(const wchar_t *text)
{
    return QString::fromWCharArray(text);
}

QWidget *createScriptPathRow(QWidget *parent,
                             QLineEdit **lineEdit,
                             QPushButton **browseButton,
                             QPushButton **clearButton,
                             const QString& placeholder)
{
    auto *widget = new QWidget(parent);
    auto *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    *lineEdit = new QLineEdit(parent);
    (*lineEdit)->setPlaceholderText(placeholder);
    *browseButton = new QPushButton(qs(L"\u6d4f\u89c8"), parent);
    *clearButton = new QPushButton(qs(L"\u6e05\u7a7a"), parent);

    layout->addWidget(*lineEdit, 1);
    layout->addWidget(*browseButton);
    layout->addWidget(*clearButton);
    return widget;
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    loadSettings();
    applyIgnoredKeywords();
    initFolderFlow();
    initConnect();
    setupShortcuts();
    setupTrayIcon();
}

MainWindow::~MainWindow()
{
    saveSettings();
    delete m_tracker;
}

void MainWindow::showAndFocusSearch()
{
    showNormal();
    raise();
    activateWindow();
    if (!m_compactMode) {
        m_tabWidget->setCurrentIndex(0);
    }
    m_lineSearch->setFocus(Qt::ShortcutFocusReason);
    m_lineSearch->selectAll();
}

void MainWindow::setupUi()
{
    setWindowTitle(APP_NAME);
    setWindowIcon(QIcon(":/favicon.ico"));
    setWindowOpacity(0.99);

    const QRect availableGeometry = QApplication::primaryScreen()
            ? QApplication::primaryScreen()->availableGeometry()
            : QRect(0, 0, 800, 800);
    resize(qMin(900, availableGeometry.width() - 80),
           qMin(800, availableGeometry.height() - 80));
    setMinimumSize(760, 560);

    auto *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    auto *searchLayout = new QHBoxLayout;
    searchLayout->setSpacing(8);
    m_lineSearch = new QLineEdit(this);
    m_lineSearch->setObjectName("line_search");
    m_lineSearch->setMinimumHeight(34);
    m_lineSearch->setPlaceholderText(qs(L"\u641c\u7d22\u8def\u5f84\u6216\u522b\u540d"));
    m_lineSearch->setClearButtonEnabled(true);

    m_btnSearch = new QPushButton(qs(L"\u641c\u7d22"), this);
    m_btnSearch->setObjectName("btn_search");
    m_btnSearch->setMinimumSize(72, 34);

    m_btnClearSearch = new QPushButton(qs(L"\u6e05\u7a7a"), this);
    m_btnClearSearch->setObjectName("btn_clear_search");
    m_btnClearSearch->setMinimumSize(72, 34);

    m_btnViewMode = new QPushButton(qs(L"\u7cbe\u7b80\u6a21\u5f0f"), this);
    m_btnViewMode->setObjectName("btn_view_mode");
    m_btnViewMode->setMinimumSize(88, 34);

    searchLayout->addWidget(m_lineSearch, 1);
    searchLayout->addWidget(m_btnSearch);
    searchLayout->addWidget(m_btnClearSearch);
    searchLayout->addWidget(m_btnViewMode);
    mainLayout->addLayout(searchLayout);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setObjectName("tabWidget");

    auto *recentTab = new QWidget(m_tabWidget);
    auto *recentLayout = new QVBoxLayout(recentTab);
    recentLayout->setContentsMargins(0, 8, 0, 0);
    recentLayout->setSpacing(8);

    m_quickAccessWidget = new QWidget(recentTab);
    auto *quickAccessLayout = new QHBoxLayout(m_quickAccessWidget);
    quickAccessLayout->setContentsMargins(0, 0, 0, 0);
    quickAccessLayout->setSpacing(8);

    auto *rankingBox = new QGroupBox(qs(L"\u6700\u8fd1\u7ecf\u5e38\u6253\u5f00"), recentTab);
    auto *rankingLayout = new QVBoxLayout(rankingBox);
    rankingLayout->setContentsMargins(4, 4, 4, 4);
    rankingLayout->setSpacing(4);
    m_listRanking = new FolderListWidget(rankingBox);
    m_listRanking->setObjectName("list_ranking");
    rankingLayout->addWidget(m_listRanking);

    auto *favoriteBox = new QGroupBox(qs(L"\u6536\u85cf"), recentTab);
    auto *favoriteLayout = new QVBoxLayout(favoriteBox);
    favoriteLayout->setContentsMargins(4, 4, 4, 4);
    favoriteLayout->setSpacing(4);
    m_listFavorite = new FolderListWidget(favoriteBox);
    m_listFavorite->setObjectName("list_favorite");
    favoriteLayout->addWidget(m_listFavorite);

    quickAccessLayout->addWidget(rankingBox, 2);
    quickAccessLayout->addWidget(favoriteBox, 1);

    auto *timelineBox = new QGroupBox(qs(L"\u64cd\u4f5c\u65f6\u95f4\u7ebf"), recentTab);
    auto *timelineLayout = new QVBoxLayout(timelineBox);
    timelineLayout->setContentsMargins(4, 4, 4, 4);
    timelineLayout->setSpacing(4);
    m_listTimeline = new FolderListWidget(timelineBox);
    m_listTimeline->setObjectName("list_timeline");
    timelineLayout->addWidget(m_listTimeline);

    recentLayout->addWidget(m_quickAccessWidget, 1);
    recentLayout->addWidget(timelineBox, 1);
    m_tabWidget->addTab(recentTab, qs(L"\u6700\u8fd1"));

    auto *manageTab = new QWidget(m_tabWidget);
    auto *manageLayout = new QVBoxLayout(manageTab);
    manageLayout->setContentsMargins(0, 8, 0, 0);
    manageLayout->setSpacing(8);

    auto *settingsBox = new QGroupBox(qs(L"\u5c5e\u6027\u914d\u7f6e"), manageTab);
    settingsBox->setMinimumWidth(560);
    settingsBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    auto *settingsLayout = new QVBoxLayout(settingsBox);
    settingsLayout->setContentsMargins(12, 28, 12, 12);
    settingsLayout->setSpacing(10);

    auto *settingsForm = new QFormLayout;
    settingsForm->setContentsMargins(0, 0, 0, 0);
    settingsForm->setSpacing(10);
    settingsForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
    settingsForm->setFormAlignment(Qt::AlignTop);
    settingsForm->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    auto *weightLabelLayout = new QHBoxLayout;
    weightLabelLayout->setContentsMargins(0, 0, 0, 0);
    weightLabelLayout->setSpacing(8);
    m_labelCount = new QLabel(qs(L"\u8ba1\u6570:50%"), settingsBox);
    m_labelLatest = new QLabel(qs(L"\u65b0\u9c9c\u5ea6:50%"), settingsBox);
    weightLabelLayout->addWidget(m_labelCount);
    weightLabelLayout->addStretch();
    weightLabelLayout->addWidget(m_labelLatest);

    m_sliderWeight = new QSlider(Qt::Horizontal, settingsBox);
    m_sliderWeight->setObjectName("slider_weight");
    m_sliderWeight->setRange(1, 100);
    m_sliderWeight->setValue(50);
    m_sliderWeight->setTickPosition(QSlider::TicksAbove);

    auto *weightWidget = new QWidget(settingsBox);
    auto *weightLayout = new QVBoxLayout(weightWidget);
    weightLayout->setContentsMargins(0, 0, 0, 0);
    weightLayout->setSpacing(4);
    weightLayout->addLayout(weightLabelLayout);
    weightLayout->addWidget(m_sliderWeight);

    m_labelFormula = new QLabel(settingsBox);
    m_labelFormula->setObjectName("label_formula");
    m_labelFormula->setWordWrap(false);
    m_labelFormula->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    auto *formulaWidget = new QWidget(settingsBox);
    auto *formulaLayout = new QHBoxLayout(formulaWidget);
    formulaLayout->setContentsMargins(0, 0, 0, 0);
    formulaLayout->addWidget(m_labelFormula, 1);

    m_spinQueryCount = new QSpinBox(settingsBox);
    m_spinQueryCount->setObjectName("spin_query_count");
    m_spinQueryCount->setRange(10, 200);
    m_spinQueryCount->setValue(10);
    m_spinQueryCount->setSuffix(" item");

    m_spinQueryTime = new QSpinBox(settingsBox);
    m_spinQueryTime->setObjectName("spin_query_time");
    m_spinQueryTime->setRange(1, 30);
    m_spinQueryTime->setValue(7);
    m_spinQueryTime->setSuffix(" day");

    m_spinKeepDays = new QSpinBox(settingsBox);
    m_spinKeepDays->setObjectName("spin_keep_days");
    m_spinKeepDays->setRange(1, 365);
    m_spinKeepDays->setValue(7);
    m_spinKeepDays->setSuffix(" day");

    auto *queryWidget = new QWidget(settingsBox);
    auto *queryLayout = new QHBoxLayout(queryWidget);
    queryLayout->setContentsMargins(0, 0, 0, 0);
    queryLayout->setSpacing(8);
    auto *queryCountLabel = new QLabel(qs(L"\u6761\u6570"), settingsBox);
    auto *queryTimeLabel = new QLabel(qs(L"\u5929\u6570"), settingsBox);
    auto *keepDaysInlineLabel = new QLabel(qs(L"\u4fdd\u7559"), settingsBox);
    queryLayout->addWidget(queryCountLabel);
    queryLayout->addWidget(m_spinQueryCount);
    queryLayout->addWidget(queryTimeLabel);
    queryLayout->addWidget(m_spinQueryTime);
    queryLayout->addSpacing(12);
    queryLayout->addWidget(keepDaysInlineLabel);
    queryLayout->addWidget(m_spinKeepDays);
    queryLayout->addStretch();

    m_lineIgnoreKeywords = new QLineEdit(settingsBox);
    m_lineIgnoreKeywords->setObjectName("line_ignore_keywords");
    m_lineIgnoreKeywords->setPlaceholderText(qs(L"\u4f8b\u5982: build; debug; release"));

    auto *commandEnvWidget = new QWidget(settingsBox);
    auto *commandEnvLayout = new QVBoxLayout(commandEnvWidget);
    commandEnvLayout->setContentsMargins(0, 0, 0, 0);
    commandEnvLayout->setSpacing(6);
    auto *qtEnvWidget = createScriptPathRow(settingsBox,
                                            &m_lineQtEnvScript,
                                            &m_btnBrowseQtEnvScript,
                                            &m_btnClearQtEnvScript,
                                            qs(L"Qt: qtenv2.bat"));
    auto *vsEnvWidget = createScriptPathRow(settingsBox,
                                            &m_lineVsEnvScript,
                                            &m_btnBrowseVsEnvScript,
                                            &m_btnClearVsEnvScript,
                                            qs(L"VS: VsDevCmd.bat"));
    auto *customEnvWidget = createScriptPathRow(settingsBox,
                                                &m_lineCustomEnvScript,
                                                &m_btnBrowseCustomEnvScript,
                                                &m_btnClearCustomEnvScript,
                                                qs(L"\u81ea\u5b9a\u4e49: *.bat / *.cmd"));
    m_lineQtEnvScript->setObjectName("line_qt_env_script");
    m_lineVsEnvScript->setObjectName("line_vs_env_script");
    m_lineCustomEnvScript->setObjectName("line_custom_env_script");
    commandEnvLayout->addWidget(qtEnvWidget);
    commandEnvLayout->addWidget(vsEnvWidget);
    commandEnvLayout->addWidget(customEnvWidget);

    m_checkStartup = new QCheckBox(qs(L"\u5f00\u673a\u542f\u52a8"), settingsBox);
    m_checkStartup->setObjectName("check_startup");

    m_btnClearHistory = new QPushButton(qs(L"\u6e05\u7a7a\u5386\u53f2"), settingsBox);
    m_btnClearHistory->setObjectName("btn_clear_history");
    m_btnClearHistory->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    auto *actionsWidget = new QWidget(settingsBox);
    auto *actionsLayout = new QHBoxLayout(actionsWidget);
    actionsLayout->setContentsMargins(0, 0, 0, 0);
    actionsLayout->setSpacing(12);
    actionsLayout->addWidget(m_checkStartup);
    actionsLayout->addWidget(m_btnClearHistory);
    actionsLayout->addStretch();

    auto *shortcutBox = new QGroupBox(qs(L"\u5feb\u6377\u952e"), manageTab);
    shortcutBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    auto *shortcutLayout = new QVBoxLayout(shortcutBox);
    shortcutLayout->setContentsMargins(12, 10, 12, 12);
    shortcutLayout->setSpacing(6);
    auto *shortcutHelp = new QLabel(shortcutBox);
    shortcutHelp->setObjectName("shortcut_help");
    shortcutHelp->setText(qs(L"Ctrl + Space\t\t\u663e\u793a/\u9690\u85cf\n"
                             L"Ctrl + L\t\t\t\u805a\u7126\u641c\u7d22\n"
                             L"Alt + 1..6\t\t\t\u6253\u5f00\u65f6\u95f4\u7ebf\u524d 6 \u9879\n"
                             L"Enter\t\t\t\u6267\u884c\u641c\u7d22\n"
                             L"Esc\t\t\t\u6e05\u7a7a\u641c\u7d22/\u9690\u85cf"));
    shortcutHelp->setTextInteractionFlags(Qt::TextSelectableByMouse);
    shortcutLayout->addWidget(shortcutHelp);

    settingsForm->addRow(qs(L"\u6392\u5e8f\u6743\u91cd"), weightWidget);
    settingsForm->addRow(qs(L"\u8ba1\u7b97\u516c\u5f0f"), formulaWidget);
    settingsForm->addRow(qs(L"\u67e5\u8be2 / \u5386\u53f2"), queryWidget);
    settingsForm->addRow(qs(L"\u5ffd\u7565\u76ee\u5f55"), m_lineIgnoreKeywords);
    settingsForm->addRow(qs(L"\u547d\u4ee4\u884c\u73af\u5883"), commandEnvWidget);
    settingsForm->addRow(qs(L"\u5e38\u7528\u64cd\u4f5c"), actionsWidget);
    settingsLayout->addLayout(settingsForm);
    settingsLayout->addStretch();

    manageLayout->addWidget(settingsBox, 0);
    manageLayout->addStretch(1);
    manageLayout->addWidget(shortcutBox, 0);
    m_tabWidget->addTab(manageTab, qs(L"\u8bbe\u7f6e"));

    mainLayout->addWidget(m_tabWidget, 1);
    setCentralWidget(central);
}

void MainWindow::initFolderFlow()
{
    if (!DatabaseManager::init(m_spinKeepDays->value())) {
        qDebug() << "FolderFlow database initialization failed.";
    }

    m_tracker = new FolderTracker;
    m_listFavorite->setMode(FolderListWidget::FavoriteMode);
    m_listRanking->setMode(FolderListWidget::RankingMode);
    m_listTimeline->setMode(FolderListWidget::TimelineMode);
    updateList();
}

void MainWindow::initConnect()
{
    connect(m_tracker, &FolderTracker::updateList, this, &MainWindow::updateList);

    connect(m_listRanking, &FolderListWidget::favoriteChanged, this, &MainWindow::updateList);
    connect(m_listTimeline, &FolderListWidget::favoriteChanged, this, &MainWindow::updateList);
    connect(m_listFavorite, &FolderListWidget::favoriteChanged, this, &MainWindow::updateList);

    connect(m_sliderWeight, &QSlider::valueChanged, this, [this]() {
        saveSettings();
        updateList();
    });

    connect(m_btnSearch, &QPushButton::clicked, this, &MainWindow::updateList);
    connect(m_btnClearSearch, &QPushButton::clicked, this, [this]() {
        m_lineSearch->clear();
        updateList();
    });

    connect(m_btnViewMode, &QPushButton::clicked, this, [this]() {
        setCompactMode(!m_compactMode);
        saveSettings();
    });

    connect(m_spinQueryCount, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        saveSettings();
        updateList();
    });

    connect(m_spinQueryTime, QOverload<int>::of(&QSpinBox::valueChanged), this, [this]() {
        saveSettings();
        updateList();
    });

    connect(m_spinKeepDays, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int days) {
        saveSettings();
        DatabaseManager::cleanupExpiredAccessData(days);
        updateList();
    });

    connect(m_lineIgnoreKeywords, &QLineEdit::editingFinished, this, [this]() {
        applyIgnoredKeywords();
        saveSettings();
        updateList();
    });

    const auto connectScriptRow = [this](QLineEdit *lineEdit, QPushButton *browseButton, QPushButton *clearButton) {
        connect(lineEdit, &QLineEdit::editingFinished, this, [this]() {
            saveSettings();
        });

        connect(browseButton, &QPushButton::clicked, this, [this, lineEdit]() {
            const QString selectedPath = QFileDialog::getOpenFileName(
                    this,
                    qs(L"\u9009\u62e9\u547d\u4ee4\u884c\u73af\u5883\u811a\u672c"),
                    lineEdit->text().trimmed(),
                    qs(L"\u547d\u4ee4\u811a\u672c (*.bat *.cmd);;\u6240\u6709\u6587\u4ef6 (*.*)"));
            if (selectedPath.isEmpty()) {
                return;
            }

            lineEdit->setText(QDir::toNativeSeparators(selectedPath));
            saveSettings();
        });

        connect(clearButton, &QPushButton::clicked, this, [this, lineEdit]() {
            lineEdit->clear();
            saveSettings();
        });
    };
    connectScriptRow(m_lineQtEnvScript, m_btnBrowseQtEnvScript, m_btnClearQtEnvScript);
    connectScriptRow(m_lineVsEnvScript, m_btnBrowseVsEnvScript, m_btnClearVsEnvScript);
    connectScriptRow(m_lineCustomEnvScript, m_btnBrowseCustomEnvScript, m_btnClearCustomEnvScript);

    connect(m_checkStartup, &QCheckBox::toggled, this, [this](bool enabled) {
        applyStartupSetting(enabled);
        saveSettings();
    });

    connect(m_btnClearHistory, &QPushButton::clicked, this, [this]() {
        const auto reply = QMessageBox::question(
                this,
                qs(L"\u6e05\u7a7a\u5386\u53f2"),
                qs(L"\u786e\u5b9a\u6e05\u7a7a\u666e\u901a\u5386\u53f2\u8bb0\u5f55\u5417\uff1f\u6536\u85cf\u548c\u7f6e\u9876\u4f1a\u4fdd\u7559\u3002"));
        if (reply != QMessageBox::Yes) {
            return;
        }

        DatabaseManager::clearHistory();
        updateList();
    });
}

void MainWindow::loadSettings()
{
    QSettings settings(settingsPath(), QSettings::IniFormat);
    m_sliderWeight->setValue(settings.value("ranking/countWeight", 50).toInt());
    m_spinQueryCount->setValue(settings.value("query/count", 10).toInt());
    m_spinQueryTime->setValue(settings.value("query/days", 7).toInt());
    m_spinKeepDays->setValue(settings.value("history/keepDays", 7).toInt());
    m_lineIgnoreKeywords->setText(settings.value("history/ignoredFolderNames", "").toString());
    m_lineQtEnvScript->setText(settings.value("commandLine/qtEnvScriptPath", "").toString());
    m_lineVsEnvScript->setText(settings.value("commandLine/vsEnvScriptPath", "").toString());
    m_lineCustomEnvScript->setText(settings.value("commandLine/customEnvScriptPath",
                                                  settings.value("commandLine/envScriptPath", "")).toString());
    m_checkStartup->setChecked(settings.value("startup/enabled", isStartupEnabled()).toBool());
    setCompactMode(settings.value("window/compactMode", false).toBool());
}

void MainWindow::saveSettings() const
{
    QSettings settings(settingsPath(), QSettings::IniFormat);
    settings.setValue("ranking/countWeight", m_sliderWeight->value());
    settings.setValue("query/count", m_spinQueryCount->value());
    settings.setValue("query/days", m_spinQueryTime->value());
    settings.setValue("history/keepDays", m_spinKeepDays->value());
    settings.setValue("history/ignoredFolderNames", m_lineIgnoreKeywords->text().trimmed());
    settings.setValue("commandLine/qtEnvScriptPath", m_lineQtEnvScript->text().trimmed());
    settings.setValue("commandLine/vsEnvScriptPath", m_lineVsEnvScript->text().trimmed());
    settings.setValue("commandLine/customEnvScriptPath", m_lineCustomEnvScript->text().trimmed());
    settings.setValue("startup/enabled", m_checkStartup->isChecked());
    settings.setValue("window/compactMode", m_compactMode);
}

void MainWindow::applyStartupSetting(bool enabled) const
{
    QSettings runSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                          QSettings::NativeFormat);
    if (enabled) {
        const QString appPath = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
        runSettings.setValue(APP_NAME, QString("\"%1\" --startup").arg(appPath));
    } else {
        runSettings.remove(APP_NAME);
    }
}

bool MainWindow::isStartupEnabled() const
{
    QSettings runSettings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                          QSettings::NativeFormat);
    return runSettings.contains(APP_NAME);
}

QString MainWindow::settingsPath() const
{
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (dirPath.isEmpty()) {
        dirPath = QCoreApplication::applicationDirPath();
    }

    QDir dir(dirPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    return dir.filePath("settings.ini");
}

QStringList MainWindow::ignoredKeywords() const
{
    QString text = m_lineIgnoreKeywords->text();
    text.replace(QChar(0xff0c), ',');
    text.replace(QChar(0xff1b), ';');
    text.replace(',', ';');
    text.replace('\n', ';');
    text.replace('\t', ';');

    QStringList result;
    for (const QString& item : text.split(';', QString::SkipEmptyParts)) {
        const QString trimmed = item.trimmed();
        if (!trimmed.isEmpty()) {
            result.append(trimmed);
        }
    }
    return result;
}

void MainWindow::applyIgnoredKeywords() const
{
    DatabaseManager::setIgnoredKeywords(ignoredKeywords());
}

void MainWindow::setupShortcuts()
{
    for (int i = 0; i < 6; ++i) {
        auto *shortcut = new QShortcut(QKeySequence(QString("Alt+%1").arg(i + 1)), this);
        shortcut->setContext(Qt::WindowShortcut);
        connect(shortcut, &QShortcut::activated, this, [this, i]() {
            m_tabWidget->setCurrentIndex(0);
            m_listTimeline->openAt(i);
        });
    }

    auto *escapeShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    escapeShortcut->setContext(Qt::WindowShortcut);
    connect(escapeShortcut, &QShortcut::activated, this, [this]() {
        if (!m_lineSearch->text().isEmpty()) {
            m_lineSearch->clear();
            updateList();
        } else {
            hide();
        }
    });

    connect(m_lineSearch, &QLineEdit::returnPressed, this, &MainWindow::updateList);

    auto *focusSearchShortcut = new QShortcut(QKeySequence("Ctrl+L"), this);
    focusSearchShortcut->setContext(Qt::WindowShortcut);
    connect(focusSearchShortcut, &QShortcut::activated, this, &MainWindow::showAndFocusSearch);
}

void MainWindow::setCompactMode(bool compact)
{
    m_compactMode = compact;
    applyViewMode();
}

void MainWindow::applyViewMode()
{
    m_quickAccessWidget->setVisible(!m_compactMode);
    m_tabWidget->tabBar()->setVisible(!m_compactMode);
    m_tabWidget->setCurrentIndex(0);
    m_btnViewMode->setText(m_compactMode ? qs(L"\u5168\u91cf\u6a21\u5f0f") : qs(L"\u7cbe\u7b80\u6a21\u5f0f"));

    if (m_compactMode) {
        setMinimumSize(560, 360);
        resize(qMin(width(), 720), qMin(height(), 520));
    } else {
        setMinimumSize(760, 560);
    }
}

void MainWindow::updateList()
{
    int countWeight = m_sliderWeight->value();
    int timeWeight = 100 - countWeight;
    int queryCount = m_spinQueryCount->value();
    int queryTime = m_spinQueryTime->value();
    const QString keyword = currentSearchKeyword();
    auto list = m_dbManager.getHotRanking(countWeight, timeWeight, queryTime, queryCount, keyword);

    m_listRanking->updateData(list, keyword);
    m_listTimeline->updateData(m_dbManager.getTimeline(queryCount, keyword), keyword);
    m_listFavorite->updateData(m_dbManager.getFavorites(keyword), keyword);

    m_labelCount->setText(qs(L"\u8ba1\u6570:%1%").arg(countWeight));
    m_labelLatest->setText(qs(L"\u65b0\u9c9c\u5ea6:%1%").arg(100 - countWeight));
    m_labelFormula->setText(qs(L"\u516c\u5f0f: \u70ed\u5ea6 = \u6b21\u6570 \u00d7 %1% + \u65b0\u9c9c\u5ea6 \u00d7 %2%")
                                    .arg(countWeight)
                                    .arg(timeWeight));
}

QString MainWindow::currentSearchKeyword() const
{
    return m_lineSearch->text().trimmed();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (trayIcon && trayIcon->isVisible()) {
        hide();
        event->ignore();
    }
}

void MainWindow::setupTrayIcon()
{
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setIcon(QIcon(":/favicon.ico"));
    trayIcon->setToolTip(APP_NAME);

    trayMenu = new QMenu(this);
    QAction *restoreAction = new QAction(qs(L"\u663e\u793a\u4e3b\u754c\u9762"), this);
    QAction *quitAction = new QAction(qs(L"\u9000\u51fa\u7a0b\u5e8f"), this);

    trayMenu->addAction(restoreAction);
    trayMenu->addSeparator();
    trayMenu->addAction(quitAction);

    trayIcon->setContextMenu(trayMenu);

    connect(trayIcon, &QSystemTrayIcon::activated, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            showNormal();
            activateWindow();
        }
    });

    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    connect(restoreAction, &QAction::triggered, this, &QWidget::showNormal);

    trayIcon->show();
}
