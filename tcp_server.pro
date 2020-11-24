TEMPLATE = app
CONFIG += console static
CONFIG -= app_bundle
CONFIG -= qt

CONFIG += c++11
QMAKE_LFLAGS += -static

LIBS +=  -lpthread

SOURCES += \
        main.cpp \

HEADERS += \
