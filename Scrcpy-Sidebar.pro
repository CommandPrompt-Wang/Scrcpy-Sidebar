QT       += core gui network multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

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

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

CONFIG(release, debug|release) {
    DEFINES += QT_NO_DEBUG_OUTPUT
    QMAKE_CXXFLAGS_RELEASE += -O2
    QMAKE_LFLAGS_RELEASE += -s
}

CONFIG(debug, debug|release) {
    DEFINES += DEBUG_MODE
}
