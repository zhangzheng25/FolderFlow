#include "mainwindow.h"
#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QLockFile>
#include <QScreen>
#include <QStandardPaths>
#include <QStyleFactory>
#include <QTextStream>
#include <objbase.h> // 必须包含此头文件
#include <qhotkey.h>

void loadStyleSheet() {
    QFile file(":/style.qss"); // 建议使用资源文件 .qrc
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream ts(&file);
        QString styleSheet = ts.readAll();
        const QScreen *screen = qApp->primaryScreen();
        if (screen && screen->availableGeometry().height() >= 1400) {
            styleSheet.replace("font-size: 15px;", "font-size: 17px;");
            styleSheet.replace("font-size: 13px;", "font-size: 15px;");
        }
        qApp->setStyleSheet(styleSheet);
    }
}

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    // 必须在所有 COM 对象（IShellWindows）创建之前调用
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr)) {
        return -1;
    }

    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("FolderFlow");
    QCoreApplication::setApplicationName("FolderFlow");

    QString lockDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (lockDir.isEmpty()) {
        lockDir = QDir::tempPath() + "/FolderFlow";
    }
    QDir().mkpath(lockDir);
    QLockFile singleInstanceLock(QDir(lockDir).filePath("FolderFlow.lock"));
    singleInstanceLock.setStaleLockTime(0);
    if (!singleInstanceLock.tryLock(100)) {
        CoUninitialize();
        return 0;
    }

    // a.setStyle(QStyleFactory::create("fusion"));
    loadStyleSheet();
    MainWindow w;
    const bool startupMode = a.arguments().contains("--startup");
    if (!startupMode) {
        w.show();
    }

    QHotkey hotkey(QKeySequence("Ctrl+Space"), true, &a); //The hotkey will be automatically registered
    qDebug() << "Is registered:" << hotkey.isRegistered();

    QObject::connect(&hotkey, &QHotkey::activated, qApp, [&](){
        if (w.isVisible()) {
            w.hide();
        } else {
            w.showAndFocusSearch();
        }
    });

    int result = a.exec();

    // 释放 COM 资源 (当窗口关闭，a.exec 退出后执行)
    CoUninitialize();

    return result;
}
