# ESP-IDF component that runs XCSoar inside an ESP-IDF application;
# see include/XCSoarEsp32.hpp for the interface.
#
# The sources and flags come from the make build (TARGET=ESP32)
# through esp32/generate-sources.py, which is run here when its output
# is missing.  Run it again after changing the make files.

get_filename_component(XCSOAR_ROOT ${CMAKE_CURRENT_LIST_DIR}/../.. ABSOLUTE)
include(${XCSOAR_ROOT}/esp32/Sources.cmake)
include(${XCSOAR_ROOT}/esp32/ThirdParty.cmake)

set(app_sources_file ${XCSOAR_ROOT}/output/ESP32/Sources.cmake)
if(NOT EXISTS ${app_sources_file})
  execute_process(
    COMMAND python3 ${XCSOAR_ROOT}/esp32/generate-sources.py
    WORKING_DIRECTORY ${XCSOAR_ROOT}
    RESULT_VARIABLE result)
  if(result)
    message(FATAL_ERROR "esp32/generate-sources.py failed")
  endif()
endif()
include(${app_sources_file})

set(srcs
  ${XCSOAR_APP_SOURCES}
  esp32/xcsoar/src/App.cpp
  esp32/xcsoar/src/Demo.cpp
  esp32/xcsoar/src/Host.cpp
  esp32/xcsoar/src/RomFs.cpp)
list(TRANSFORM srcs PREPEND ${XCSOAR_ROOT}/)

set(private_include_dirs ${XCSOAR_APP_INCLUDE_DIRS} src/unix)
list(TRANSFORM private_include_dirs PREPEND ${XCSOAR_ROOT}/)

idf_component_register(
  SRCS ${srcs}
  INCLUDE_DIRS include
  PRIV_INCLUDE_DIRS ${private_include_dirs}
  PRIV_REQUIRES vfs lwip espressif__freetype)

if(NOT CMAKE_BUILD_EARLY_EXPANSION)
  xcsoar_fetch_libraries()
  xcsoar_add_third_party(${COMPONENT_LIB} "idf::newlib;idf::lwip;idf::vfs")

  target_include_directories(${COMPONENT_LIB} PRIVATE
    ${fmt_SOURCE_DIR}/include ${boost_SOURCE_DIR})
  # NDEBUG: a release build, without XCSoar's debug checks and its
  # "DEBUG BUILD - DO NOT FLY!" banner
  target_compile_definitions(${COMPONENT_LIB} PRIVATE
    ${XCSOAR_APP_DEFINES} FMT_HEADER_ONLY NDEBUG)

  # resources.c and output/data/*.c embed files by paths relative to
  # XCSOAR_ROOT
  target_compile_options(${COMPONENT_LIB} PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-Wa,-I${XCSOAR_ROOT}>)

  # XCSoar reports errors with exceptions; this needs
  # CONFIG_COMPILER_CXX_EXCEPTIONS in the application's sdkconfig.
  # Options given here come after the project-wide ones, so this
  # also overrides a global -fno-exceptions.
  target_compile_options(${COMPONENT_LIB} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++20 -fexceptions>)

  # Optimise for speed, not size: map drawing and the glide computer
  # are CPU bound, and the application has flash to spare
  target_compile_options(${COMPONENT_LIB} PRIVATE -O2)

  # The toolchain's limits.h defines LLONG_MAX for C only, but Lua's
  # headers need it in C++ too; libstdc++'s <climits> adds it
  target_compile_options(${COMPONENT_LIB} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-include climits>)
endif()
