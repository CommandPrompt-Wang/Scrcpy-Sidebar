QT += core gui network multimedia
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

SOURCES += \
    advancedkeyboard.cpp \
    externalprocess.cpp \
    globalconfig.cpp \
    main.cpp \
    settingswindow.cpp \
    toolboxmain.cpp

HEADERS += \
    advancedkeyboard.h \
    externalprocess.h \
    globalconfig.h \
    settingswindow.h \
    toolboxmain.h

FORMS += \
    advancedkeyboard.ui \
    settingswindow.ui \
    toolboxmain.ui

QMAKE_EXTRA_COMPILERS += lrelease
lrelease.input = TRANSLATIONS
lrelease.output = $$QMAKE_TRANSLATIONS.ts.qm
lrelease.commands = $$LRELEASE ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
lrelease.CONFIG += no_link target_predeps

TRANSLATIONS += Scrcpy_Sidebar_en_US.ts
RESOURCES += translation.qrc

CONFIG(release, debug|release) {
    DEFINES += QT_NO_DEBUG
}

DEFINES += WIN32_LEAN_AND_MEAN
