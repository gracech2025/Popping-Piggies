# Build every .c file in src/, and build test.wav into the firmware so the
# code can play it (it becomes _binary_test_wav_start in audio.c).
# idf_component_register must only be called ONCE per folder.
# New sound files must be added to EMBED_FILES here AND to board_build.embed_files
# in platformio.ini.

FILE(GLOB app_sources ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

idf_component_register(
    SRCS
        ${app_sources}
    INCLUDE_DIRS
        "."
    EMBED_FILES
        "test.wav"
)
