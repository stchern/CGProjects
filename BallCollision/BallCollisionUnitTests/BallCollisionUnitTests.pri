!BALL_COLLISION_UNIT_TEST_PRI{
CONFIG += BALL_COLLISION_TEST_PRI

INCLUDEPATH *= $$clean_path($$PWD/../)
include($$PWD/../BallCollision/BallCollision.pri)


}

HEADERS += \
    $$PWD/testBallUtils.h \
    $$PWD/testDrawingUtils.h \
    $$PWD/testVectorUtils.h

SOURCES += \
    $$PWD/testBallUtils.cpp \
    $$PWD/testDrawingUtils.cpp \
    $$PWD/testVectorUtils.cpp



