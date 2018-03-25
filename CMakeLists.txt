cmake_minimum_required(VERSION 3.8)
project(chip8)

add_executable(chip8 src/main.cpp src/chip8.cpp)

find_package(SDL2 REQUIRED)

target_include_directories(chip8
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(chip8 ${SDL2_LIBRARIES})

target_compile_options(chip8 PRIVATE -Wall -Werror -Wextra)
target_compile_features(chip8 PRIVATE cxx_std_17)
