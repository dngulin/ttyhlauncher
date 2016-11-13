#-------------------------------------------------
#
# Project created by QtCreator 2014-08-05T23:29:23
#
#-------------------------------------------------

QT       += core gui widgets network

TARGET = ttyhlauncher
TEMPLATE = app

LIBS += -lquazip5

QMAKE_CXXFLAGS += -std=c++11

SOURCES += main.cpp \
    launcherwindow.cpp \
    skinuploaddialog.cpp \
    settingsdialog.cpp \
    updatedialog.cpp \
    feedbackdialog.cpp \
    aboutdialog.cpp \
    settings.cpp \
    logger.cpp \
    util.cpp \
    licensedialog.cpp \
    jsonparser.cpp \
    libraryinfo.cpp \
    fileinfo.cpp \
    gamerunner.cpp \
    filefetcher.cpp \
    datafetcher.cpp \
    hashchecker.cpp \
    logview.cpp \
    selfupdatedialog.cpp \
    storesettingsdialog.cpp \
    storemanagedialog.cpp \
    storeinstalldialog.cpp \
    installinfo.cpp \
    fileinstaller.cpp

HEADERS += launcherwindow.h \
    skinuploaddialog.h \
    settingsdialog.h \
    updatedialog.h \
    feedbackdialog.h \
    aboutdialog.h \
    settings.h \
    logger.h \
    util.h \
    licensedialog.h \
    jsonparser.h \
    libraryinfo.h \
    fileinfo.h \
    gamerunner.h \
    filefetcher.h \
    datafetcher.h \
    hashchecker.h \
    logview.h \
    selfupdatedialog.h \
    storesettingsdialog.h \
    storemanagedialog.h \
    storeinstalldialog.h \
    installinfo.h \
    fileinstaller.h

FORMS += launcherwindow.ui \
    skinuploaddialog.ui \
    settingsdialog.ui \
    updatedialog.ui \
    feedbackdialog.ui \
    aboutdialog.ui \
    licensedialog.ui \
    selfupdatedialog.ui \
    storesettingsdialog.ui \
    storemanagedialog.ui \
    storeinstalldialog.ui

RESOURCES += resources.qrc

TRANSLATIONS += translations/ru.ts \
    translations/koi7.ts

RC_ICONS = resources/favicon.ico

ICON = resources/favicon.icns

unix {
    target.path = $$PREFIX/bin
    INSTALLS    += target

    OBJECTS_DIR = .obj
    MOC_DIR     = .moc
    UI_DIR      = .ui

    desktopfile.files = resources/unix/ttyhlauncher.desktop
    desktopicon.files = resources/unix/ttyhlauncher.svg
    desktopfile.path  = $$PREFIX/share/applications
    desktopicon.path  = $$PREFIX/share/icons/hicolor/scalable/apps

    CONFIG(unix_desktop): INSTALLS += desktopfile desktopicon
}

macx {
    INCLUDEPATH += /usr/local/include
    LIBS += -L/usr/local/lib
    QMAKE_MAC_SDK = macosx10.9
}
