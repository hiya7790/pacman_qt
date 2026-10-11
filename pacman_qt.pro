QT       += core gui widgets

TARGET = pacman_qt
TEMPLATE = app

SOURCES += main.cpp \
           mainwindow.cpp

HEADERS += mainwindow.h \
           mazesolver.h

CONFIG += c++17
