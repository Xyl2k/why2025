#
# add this content into badgevms_firmware\sdk_apps\CMakeLists.txt and compile the whole firmware thingy.
#

build_app(Seizure_nickname PREINSTALL
    SOURCES
      main.c
    LIBRARIES
      sdl3
)