QT += testlib

CONFIG += c++17 console
CONFIG -= app_bundle

include($$PWD/../BallCollisionUnitTests/BallCollisionUnitTests.pri)

SOURCES += \
        main.cpp
