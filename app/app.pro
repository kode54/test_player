#-------------------------------------------------
#
# Project created by QtCreator 2012-12-20T15:37:24
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = test_player
TEMPLATE = app

LIBS += -L$$OUT_PWD/../File_Extractor/prj/File_Extractor/ \
        -L$$OUT_PWD/../Game_Music_Emu/prj/Game_Music_Emu/ \
        -L$$OUT_PWD/../dumb/dumb/prj/dumb/

LIBS += -lGame_Music_Emu -lFile_Extractor -ldumb -lz -lao -lm

DEPENDPATH += $$PWD/../File_Extractor/prj/File_Extractor \
              $$PWD/../Game_Music_Emu/prj/Game_Music_Emu \
              $$PWD/../dumb/dumb/prj/dumb

PRE_TARGETDEPS += $$OUT_PWD/../File_Extractor/prj/File_Extractor/libFile_Extractor.a \
                  $$OUT_PWD/../Game_Music_Emu/prj/Game_Music_Emu/libGame_Music_Emu.a \
                  $$OUT_PWD/../dumb/dumb/prj/dumb/libdumb.a

INCLUDEPATH += ../File_Extractor \
               ../Game_Music_Emu \
               ../dumb/dumb/include

SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui
