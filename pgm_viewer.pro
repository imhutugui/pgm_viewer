QT += core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = pgm_viewer
TEMPLATE = app

CONFIG += c++17

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    imageview.cpp

HEADERS += \
    mainwindow.h \
    imageview.h

FORMS += \
    mainwindow.ui
