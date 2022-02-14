cmake_minimum_required(VERSION 3.10)
project(smallsh C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wunused-result")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -g -Wall")

add_executable(smallsh main.c parsers.c error_handlers.c signal_handlers.c commands.c)
