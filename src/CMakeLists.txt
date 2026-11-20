# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)

idf_component_register(SRCS ${app_sources})

# Embed .wav file to ESP32 flash memory
idf_component_register(
    SRCS
        "main.c"
        "wav_player.c"
    INCLUDE_DIRS
        "."
    EMBED_FILES
        "test.wav"
)
