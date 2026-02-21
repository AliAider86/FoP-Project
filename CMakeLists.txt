cmake_minimum_required(VERSION 3.25)
project(FoP)
# به جای BUILD_SHARED_LIBS=OFF
set(BUILD_SHARED_LIBS ON)

set(CMAKE_CXX_STANDARD 17)

include_directories(C:/MinGW/include/SDL2)
link_directories(C:/MinGW/lib)

add_executable(FoP main.cpp
        blocks.h
        engine.h
        engine.cpp
        ui.h
        ui.cpp
        value.h
        operators.h
        looks.h
        logger.h
        logger.cpp)

target_link_libraries(${PROJECT_NAME}
        -lmingw32
        -lSDL2main
        -lSDL2
        -lSDL2_ttf
        -lSDL2_image
        -lSDL2_mixer
)