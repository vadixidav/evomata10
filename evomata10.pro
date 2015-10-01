TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += c++11

QMAKE_CXXFLAGS += -pthread 
LIBS += -pthread

LIBS += \
    -ldrew \
    -lgpi \
    -lphitron \
    -lSDL2 \
    -lGL \
    -lGLU \
    -lGLEW

SOURCES += main.cpp \
    cell.cpp \
    group.cpp \
    draw.cpp

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
    cell.h \
    group.h \
    draw.h

