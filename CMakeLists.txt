cmake_minimum_required(VERSION 3.16)
project(smallsh LANGUAGES C)

set(CMAKE_C_STANDARD 11)
add_executable(smallsh main.c parsers.c error_handlers.c signal_handlers.c commands.c)
add_compile_options(-O3 -Wunused-result)
