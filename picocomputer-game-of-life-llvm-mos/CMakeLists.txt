cmake_minimum_required(VERSION 3.18)
add_subdirectory(tools)
set(LLVM_MOS_PLATFORM rp6502)
find_package(llvm-mos-sdk REQUIRED)
project(RP6502-GAME-OF-LIFE)
add_executable(life)
rp6502_asset(life 0x10000 assets/bin/startscreen.bin)
rp6502_asset(life 0x19600 assets/bin/bmp640x480x1bpp_header.bin)
rp6502_executable(life
    ${CMAKE_CURRENT_LIST_DIR}/src/help.rp6502
    ${CMAKE_CURRENT_LIST_DIR}/build/startscreen.bin.rp6502
    ${CMAKE_CURRENT_LIST_DIR}/build/bmp640x480x1bpp_header.bin.rp6502
)
target_sources(life PRIVATE
    src/colors.c
    src/bitmap_graphics.c
    src/mouse.c
    src/cellmap.c
    src/main.c
)

