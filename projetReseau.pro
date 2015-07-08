TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt
QMAKE_CFLAGS += -std=c11
QMAKE_CXXFLAGS += -pthread
LIBS += -pthread
SOURCES += main.c
QMAKE_CFLAGS_RELEASE
