cmake_minimum_required(VERSION 3.25)
project(FoP)

set(CMAKE_CXX_STANDARD 17)

include_directories(C:/MinGW/include/SDL2)
link_directories(C:/MinGW/lib)

add_executable(FoP main.cpp
        game.h)

target_link_libraries(FoP
        mingw32
        SDL2main
        SDL2
)
