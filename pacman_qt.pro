QT       += core gui widgets network multimedia

TARGET = pacman_qt
TEMPLATE = app

SOURCES += main.cpp \
           mainwindow.cpp \
           mainwindow_maze.cpp \
           mainwindow_gameplay.cpp \
           mainwindow_render_game.cpp \
           mainwindow_map.cpp \
           mainwindow_input.cpp \
           gameserver.cpp \
           soundmanager.cpp

HEADERS += mainwindow.h \
           gameserver.h \
           soundmanager.h

CONFIG += c++17
