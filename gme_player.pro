#-------------------------------------------------
#
# Project created by QtCreator 2012-12-20T15:37:24
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = gme_player
TEMPLATE = app

CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../gme_player/File_Extractor/prj/File_Extractor-build-Desktop-Release/ \
                                        -L$$OUT_PWD/../gme_player/Game_Music_Emu/prj/Game_Music_Emu-build-Desktop-Release/
CONFIG(debug, debug|release):   LIBS += -L$$OUT_PWD/../gme_player/File_Extractor/prj/File_Extractor-build-Desktop-Debug/ \
                                        -L$$OUT_PWD/../gme_player/Game_Music_Emu/prj/Game_Music_Emu-build-Desktop-Debug/

LIBS += -lGame_Music_Emu -lFile_Extractor -lz -lao

DEPENDPATH += $$OUT_PWD/../gme_player/File_Extractor/prj \
              $$OUT_PWD/../gme_player/Game_Music_Emu/prj

CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../gme_player/File_Extractor/prj/File_Extractor-build-Desktop-Release/libFile_Extractor.a \
                                                  $$OUT_PWD/../gme_player/Game_Music_Emu/prj/Game_Music_Emu-build-Desktop-Release/libGame_Music_Emu.a
CONFIG(debug, debug|release):   PRE_TARGETDEPS += $$OUT_PWD/../gme_player/File_Extractor/prj/File_Extractor-build-Desktop-Debug/libFile_Extractor.a \
                                                  $$OUT_PWD/../gme_player/Game_Music_Emu/prj/Game_Music_Emu-build-Desktop-Debug/libGame_Music_Emu.a

INCLUDEPATH += File_Extractor \
               Game_Music_Emu

SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui
