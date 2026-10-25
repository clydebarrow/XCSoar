# ESP-IDF component that runs XCSoar inside an ESP-IDF application;
# see include/XCSoarEsp32.hpp for the interface.

get_filename_component(XCSOAR_ROOT ${CMAKE_CURRENT_LIST_DIR}/../.. ABSOLUTE)
include(${XCSOAR_ROOT}/esp32/Sources.cmake)

set(srcs
  ${XCSOAR_ESP32_SOURCES}
  src/ui/canvas/esp32/Files.cpp
  esp32/xcsoar/src/RomFs.cpp)
list(TRANSFORM srcs PREPEND ${XCSOAR_ROOT}/)

set(private_include_dirs ${XCSOAR_ESP32_INCLUDE_DIRS})
list(TRANSFORM private_include_dirs PREPEND ${XCSOAR_ROOT}/)

idf_component_register(
  SRCS ${srcs}
  INCLUDE_DIRS include
  PRIV_INCLUDE_DIRS ${private_include_dirs}
  PRIV_REQUIRES vfs espressif__freetype)

if(NOT CMAKE_BUILD_EARLY_EXPANSION)
  xcsoar_fetch_libraries()

  set(generated_dir ${CMAKE_CURRENT_BINARY_DIR}/include)
  xcsoar_generate_math_tables(${XCSOAR_ROOT} ${generated_dir})

  target_include_directories(${COMPONENT_LIB} PRIVATE
    ${generated_dir} ${fmt_SOURCE_DIR}/include ${boost_SOURCE_DIR})
  target_compile_definitions(${COMPONENT_LIB} PRIVATE
    ${XCSOAR_ESP32_DEFINES})

  # XCSoar reports errors with exceptions; this needs
  # CONFIG_COMPILER_CXX_EXCEPTIONS in the application's sdkconfig.
  # Options given here come after the project-wide ones, so this
  # also overrides a global -fno-exceptions.
  target_compile_options(${COMPONENT_LIB} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++20 -fexceptions>)
endif()
