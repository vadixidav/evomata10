TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += c++11

LIBS += -L/usr/local/lib \
    -lgpi \
    -lphitron

SOURCES += main.cpp \
    cell.cpp \
    group.cpp

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
    cell.h \
    group.h

