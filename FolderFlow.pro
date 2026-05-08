QT += widgets sql
LIBS += -luser32 -lole32 -loleaut32 -lshell32

CONFIG += c++17
msvc {
    QMAKE_CFLAGS += /utf-8
    QMAKE_CXXFLAGS += /utf-8
}

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    DatabaseManager.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    DatabaseManager.h \
    FolderCardWidget.h \
    FolderListWidget.h \
    FolderTracker.h \
    mainwindow.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

CONFIG(release, debug|release) {
    DESTDIR     = $$PWD/build/bin
    OBJECTS_DIR = $$PWD/build/obj
    MOC_DIR     = $$PWD/build/moc
    UI_DIR      = $$PWD/build/ui
    RCC_DIR     = $$PWD/build/qrc
}

RESOURCES += \
    qrc/theme.qrc

RC_FILE = qrc/icon.rc

INCLUDEPATH +=  $$PWD/QHotkey\


include($$PWD/QHotkey/QHotkey.pri)
