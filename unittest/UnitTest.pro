QT += testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

CONFIG += c++17

TEMPLATE = app

SOURCES +=  ../BuildInfo.cpp tst_buildinfotest.cpp
