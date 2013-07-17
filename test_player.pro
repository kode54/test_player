TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS = File_Extractor/prj/File_Extractor \
          Game_Music_Emu/prj/Game_Music_Emu \
          dumb/dumb/prj/dumb \
          midi_processing \
          psflib \
          PSXCore/Core \
          SegaCore/Core \
          QSoundCore/Core \
          viogsf \
          hvl_replay \
          app

app.depends = File_Extractor/prj/File_Extractor \
              Game_Music_Emu/prj/Game_Music_Emu \
              dumb/dumb/prj/dumb \
              hvl_replay \
              midi_processing \
              psflib \
              PSXCore/Core \
              SegaCore/Core \
              QSoundCore/Core \
              viogsf
