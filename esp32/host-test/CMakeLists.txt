# Builds the ESP32 glue for the host, so that the render pipeline can
# be tested without an ESP toolchain.  It writes each frame to a PPM
# file.
#
#   cmake -S esp32/host-test -B output/esp32-host-test
#   cmake --build output/esp32-host-test
#   output/esp32-host-test/RunEsp32Host frame.ppm

cmake_minimum_required(VERSION 3.20)
project(XCSoarEsp32HostTest CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

get_filename_component(XCSOAR_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../.. ABSOLUTE)
include(${XCSOAR_ROOT}/esp32/Sources.cmake)

xcsoar_fetch_fmt()

find_package(Freetype REQUIRED)

set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/include)
xcsoar_generate_math_tables(${XCSOAR_ROOT} ${GENERATED_DIR})

list(TRANSFORM XCSOAR_ESP32_SOURCES PREPEND ${XCSOAR_ROOT}/)
list(TRANSFORM XCSOAR_ESP32_INCLUDE_DIRS PREPEND ${XCSOAR_ROOT}/)

# font search paths; the ESP32 build uses src/ui/canvas/esp32/Files.cpp
if(APPLE)
  set(font_files ${XCSOAR_ROOT}/src/ui/canvas/apple/Files.cpp)
else()
  set(font_files ${XCSOAR_ROOT}/src/ui/canvas/custom/Files.cpp)
endif()

add_executable(RunEsp32Host
  main.cpp
  ${font_files}
  ${XCSOAR_ESP32_SOURCES})
target_include_directories(RunEsp32Host PRIVATE
  ${XCSOAR_ESP32_INCLUDE_DIRS} ${GENERATED_DIR} ${fmt_SOURCE_DIR}/include)
target_compile_definitions(RunEsp32Host PRIVATE ${XCSOAR_ESP32_DEFINES})
target_link_libraries(RunEsp32Host PRIVATE Freetype::Freetype)
