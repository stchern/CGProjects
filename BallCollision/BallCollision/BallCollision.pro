#QT += gui

CONFIG += c++17 console
CONFIG -= app_bundle

INCLUDEPATH *= $$clean_path($$PWD/../)
INCLUDEPATH += $$clean_path(/usr/include/)


SOURCES += \
        $$PWD/main.cpp \
        BallUtils.cpp \
        DrawingUtils.cpp \
        VectorUtils.cpp

HEADERS += \
    $$PWD/MiddleAverageFilter.h \
    Ball.h \
    BallUtils.h \
    DrawingUtils.h \
    VectorUtils.h

LIBS += -L$$PWD/../SFML-2.5.1/lib/ -lsfml-audio
LIBS += -L$$PWD/../SFML-2.5.1/lib/ -lsfml-graphics
LIBS += -L$$PWD/../SFML-2.5.1/lib/ -lsfml-network
LIBS += -L$$PWD/../SFML-2.5.1/lib/ -lsfml-system
LIBS += -L$$PWD/../SFML-2.5.1/lib/ -lsfml-window

INCLUDEPATH += $$PWD/../SFML-2.5.1/include
DEPENDPATH += $$PWD/../SFML-2.5.1/include
