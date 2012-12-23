TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS = File_Extractor/prj/File_Extractor \
          Game_Music_Emu/prj/Game_Music_Emu \
          dumb/dumb/prj/dumb \
          app

app.depends = File_Extractor/prj/File_Extractor \
              Game_Music_Emu/prj/Game_Music_Emu \
              dumb/dumb/prj/dumb

