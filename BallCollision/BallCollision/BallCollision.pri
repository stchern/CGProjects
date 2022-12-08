!BALL_COLLISION_PRI{
CONFIG += BALL_COLLISION_PRI

INCLUDEPATH *= $$clean_path($$PWD/../)
INCLUDEPATH += $$clean_path(/usr/include/)


SOURCES += \
        $$PWD/BallUtils.cpp \
        $$PWD/DrawingUtils.cpp

HEADERS += \
    $$PWD/MiddleAverageFilter.h \
    $$PWD/Ball.h \
    $$PWD/BallUtils.h \
    $$PWD/DrawingUtils.h \
    $$PWD/VectorUtils.h

LIBS += -L$$PWD/../SFML-2.5.1/lib/ -lsfml-audio
LIBS += -L$$PWD/../SFML-2.5.1/lib/ -lsfml-graphics
LIBS += -L$$PWD/../SFML-2.5.1/lib/ -lsfml-network
LIBS += -L$$PWD/../SFML-2.5.1/lib/ -lsfml-system
LIBS += -L$$PWD/../SFML-2.5.1/lib/ -lsfml-window

INCLUDEPATH += $$PWD/../SFML-2.5.1/include
DEPENDPATH += $$PWD/../SFML-2.5.1/include

}
