cmake_minimum_required(VERSION 3.18)
add_subdirectory(tools)
set(LLVM_MOS_PLATFORM rp6502)
find_package(llvm-mos-sdk REQUIRED)
project(MY-RP6502-PROJECT)
add_executable(life)
rp6502_executable(life)
target_include_directories(life PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)
target_sources(life PRIVATE
    src/colors.c
    src/bitmap_graphics.c
    src/main.c
)
