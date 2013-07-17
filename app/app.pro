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
macx:QMAKE_CXXFLAGS += -mmacosx-version-min=10.7 -stdlib=libc++

LIBS += -L$$OUT_PWD/../../../../bass
LIBS += -L$$OUT_PWD/../../../../bass/c

LIBS += -lao -lbassmidi -lbass -lm

INCLUDEPATH += ../../../../bass
INCLUDEPATH += ../../../../bass/c

SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../File_Extractor/prj/File_Extractor/release/ -lFile_Extractor
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../File_Extractor/prj/File_Extractor/debug/ -lFile_Extractor
else:unix: LIBS += -L$$OUT_PWD/../File_Extractor/prj/File_Extractor/ -lFile_Extractor

INCLUDEPATH += $$PWD/../File_Extractor $$PWD/../File_Extractor/prj/File_Extractor
DEPENDPATH += $$PWD/../File_Extractor/prj/File_Extractor

win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../File_Extractor/prj/File_Extractor/release/libFile_Extractor.a
else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../File_Extractor/prj/File_Extractor/debug/libFile_Extractor.a
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../File_Extractor/prj/File_Extractor/libFile_Extractor.a

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../Game_Music_Emu/prj/Game_Music_Emu/release/ -lGame_Music_Emu
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../Game_Music_Emu/prj/Game_Music_Emu/debug/ -lGame_Music_Emu
else:unix: LIBS += -L$$OUT_PWD/../Game_Music_Emu/prj/Game_Music_Emu/ -lGame_Music_Emu

INCLUDEPATH += $$PWD/../Game_Music_Emu $$PWD/../Game_Music_Emu/prj/Game_Music_Emu
DEPENDPATH += $$PWD/../Game_Music_Emu/prj/Game_Music_Emu

win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Game_Music_Emu/prj/Game_Music_Emu/release/libGame_Music_Emu.a
else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../Game_Music_Emu/prj/Game_Music_Emu/debug/libGame_Music_Emu.a
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../Game_Music_Emu/prj/Game_Music_Emu/libGame_Music_Emu.a

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../dumb/dumb/prj/dumb/release/ -ldumb
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../dumb/dumb/prj/dumb/debug/ -ldumb
else:unix: LIBS += -L$$OUT_PWD/../dumb/dumb/prj/dumb/ -ldumb

INCLUDEPATH += $$PWD/../dumb/dumb/include $$PWD/../dumb/dumb/prj/dumb
DEPENDPATH += $$PWD/../dumb/dumb/prj/dumb

win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../dumb/dumb/prj/dumb/release/libdumb.a
else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../dumb/dumb/prj/dumb/debug/libdumb.a
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../dumb/dumb/prj/dumb/libdumb.a

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../psflib/release/ -lpsflib
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../psflib/debug/ -lpsflib
else:unix: LIBS += -L$$OUT_PWD/../psflib/ -lpsflib

INCLUDEPATH += $$PWD/../psflib
DEPENDPATH += $$PWD/../psflib

win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../psflib/release/libpsflib.a
else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../psflib/debug/libpsflib.a
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../psflib/libpsflib.a

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../PSXCore/Core/release/ -lPSXCore
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../PSXCore/Core/debug/ -lPSXCore
else:unix: LIBS += -L$$OUT_PWD/../PSXCore/Core/ -lPSXCore

INCLUDEPATH += $$PWD/../PSXCore/Core
DEPENDPATH += $$PWD/../PSXCore/Core

win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../PSXCore/Core/release/libPSXCore.a
else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../PSXCore/Core/debug/libPSXCore.a
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../PSXCore/Core/libPSXCore.a

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../SegaCore/Core/release/ -lSegaCore
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../SegaCore/Core/debug/ -lSegaCore
else:unix: LIBS += -L$$OUT_PWD/../SegaCore/Core/ -lSegaCore

INCLUDEPATH += $$PWD/../SegaCore/Core
DEPENDPATH += $$PWD/../SegaCore/Core

win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../SegaCore/Core/release/libSegaCore.a
else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../SegaCore/Core/debug/libSegaCore.a
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../SegaCore/Core/libSegaCore.a

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../QSoundCore/Core/release/ -lQSoundCore
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../QSoundCore/Core/debug/ -lQSoundCore
else:unix: LIBS += -L$$OUT_PWD/../QSoundCore/Core/ -lQSoundCore

INCLUDEPATH += $$PWD/../QSoundCore/Core
DEPENDPATH += $$PWD/../QSoundCore/Core

win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../QSoundCore/Core/release/libQSoundCore.a
else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../QSoundCore/Core/debug/libQSoundCore.a
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../QSoundCore/Core/libQSoundCore.a

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../midi_processing/release/ -lmidi_processing
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../midi_processing/debug/ -lmidi_processing
else:unix: LIBS += -L$$OUT_PWD/../midi_processing/ -lmidi_processing

INCLUDEPATH += $$PWD/../midi_processing
DEPENDPATH += $$PWD/../midi_processing

win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../midi_processing/release/libmidi_processing.a
else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../midi_processing/debug/libmidi_processing.a
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../midi_processing/libmidi_processing.a

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../viogsf/release/ -lviogsf
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../viogsf/debug/ -lviogsf
else:unix: LIBS += -L$$OUT_PWD/../viogsf/ -lviogsf

INCLUDEPATH += $$PWD/../viogsf/vbam
DEPENDPATH += $$PWD/../viogsf

win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../viogsf/release/libviogsf.a
else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../viogsf/debug/libviogsf.a
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../viogsf/libviogsf.a

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../hvl_replay/release/ -lhvl_replay
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../hvl_replay/debug/ -lhvl_replay
else:unix: LIBS += -L$$OUT_PWD/../hvl_replay/ -lhvl_replay

INCLUDEPATH += $$PWD/../hvl_replay
DEPENDPATH += $$PWD/../hvl_replay

win32:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../hvl_replay/release/hvl_replay.lib
else:win32:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../hvl_replay/debug/hvl_replay.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../hvl_replay/libhvl_replay.a

LIBS += -lz

macx:LIBS += -lc++
