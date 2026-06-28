QT += widgets multimedia

CONFIG += c++17

SOURCES += \
    ai_controller.cpp \
    audiomanager.cpp \
    bomb.cpp \
    bullet.cpp \
    gamemanager.cpp \
    gameobject.cpp \
    gamewidget.cpp \
    helpdialog.cpp \
    main.cpp \
    mainwindow.cpp \
    player.cpp \
    platform.cpp \
    simple_mlp.cpp

HEADERS += \
    ai_controller.h \
    ai_types.h \
    audiomanager.h \
    bomb.h \
    bullet.h \
    gameconfig.h \
    gamemanager.h \
    gameobject.h \
    gamewidget.h \
    helpdialog.h \
    keyassigndialog.h \
    mainwindow.h \
    pausedialog.h \
    player.h \
    platform.h \
    simple_mlp.h

FORMS += \
    helpdialog.ui

RESOURCES += \
    resources.qrc
