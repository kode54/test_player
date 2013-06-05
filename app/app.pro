#-------------------------------------------------
#
# Project created by QtCreator 2012-12-20T15:37:24
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = test_player
TEMPLATE = app

DEFINES += HAVE_STDINT_H

QMAKE_CXXFLAGS += -std=c++0x

LIBS += -L$$OUT_PWD/../File_Extractor/prj/File_Extractor/ \
        -L$$OUT_PWD/../Game_Music_Emu/prj/Game_Music_Emu/ \
        -L$$OUT_PWD/../dumb/dumb/prj/dumb/ \
        -L$$OUT_PWD/../midi_processing/ \
        -L$$OUT_PWD/../psflib/ \
        -L$$OUT_PWD/../PSXCore/Core/ \
        -L$$OUT_PWD/../SegaCore/Core/ \
        -L$$OUT_PWD/../QSoundCore/Core/ \
        -L$$OUT_PWD/../viogsf/ \
        -L$$OUT_PWD/../../../bass/x64

LIBS += -lGame_Music_Emu -lFile_Extractor -ldumb -lmidi_processing -lpsflib -lPSXCore -lSegaCore -lQSoundCore -lviogsf -lz -lao -lbassmidi -lbass -lm

DEPENDPATH += $$PWD/../File_Extractor/prj/File_Extractor \
              $$PWD/../Game_Music_Emu/prj/Game_Music_Emu \
              $$PWD/../dumb/dumb/prj/dumb \
              $$PWD/../midi_processing \
              $$PWD/../psflib \
              $$PWD/../PSXCore/Core \
              $$PWD/../SegaCore/Core \
              $$PWD/../QSoundCore/Core \
              $$PWD/../viogsf

PRE_TARGETDEPS += $$OUT_PWD/../File_Extractor/prj/File_Extractor/libFile_Extractor.a \
                  $$OUT_PWD/../Game_Music_Emu/prj/Game_Music_Emu/libGame_Music_Emu.a \
                  $$OUT_PWD/../dumb/dumb/prj/dumb/libdumb.a \
                  $$OUT_PWD/../midi_processing/libmidi_processing.a \
                  $$OUT_PWD/../psflib/libpsflib.a \
                  $$OUT_PWD/../PSXCore/Core/libPSXCore.a \
                  $$OUT_PWD/../SegaCore/Core/libSegaCore.a \
                  $$OUT_PWD/../QSoundCore/Core/libQSoundCore.a \
                  $$OUT_PWD/../viogsf/libviogsf.a

INCLUDEPATH += ../File_Extractor \
               ../Game_Music_Emu \
               ../dumb/dumb/include \
               ../midi_processing \
               ../../../bass \
               ../psflib \
               ../PSXCore/Core \
               ../SegaCore/Core \
               ../QSoundCore/Core \
               ../viogsf/vbam

SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui
