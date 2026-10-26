QT       += core gui widgets

TARGET = pacman_qt
TEMPLATE = app

SOURCES += main.cpp \
           mainwindow.cpp \
           mainwindow_maze.cpp \
           mainwindow_gameplay.cpp \
           mainwindow_render_game.cpp \
           mainwindow_map.cpp \
           mainwindow_input.cpp

HEADERS += mainwindow.h

CONFIG += c++17
