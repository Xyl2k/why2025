#
# add this content into badgevms_firmware\sdk_apps\CMakeLists.txt and compile the whole firmware thingy.
#

build_app(efuse_dump PREINSTALL
SOURCES
main.c
)