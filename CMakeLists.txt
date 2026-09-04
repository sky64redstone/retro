cmake_minimum_required(VERSION 3.10)

project(retro
  VERSION 0.1.0
  LANGUAGES C
)

set(CMAKE_C_STANDARD 11)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type" FORCE)
endif()

add_executable(retro
  src/main.c
  src/vec.c
  src/game.c

  src/games/snake.c
)

target_include_directories(retro PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3-shared)
target_link_libraries(retro PRIVATE SDL3::SDL3)
target_link_libraries(retro PRIVATE m)

if (CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wformat=2
    -Wmissing-prototypes
    -Wstrict-prototypes
  )
else()
  message(FATAL_ERROR "Unknown compiler!")
endif()

