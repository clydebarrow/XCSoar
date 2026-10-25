# XCSoar sources and flags shared by the ESP-IDF component and the
# host test build.  Paths are relative to XCSOAR_ROOT.

set(XCSOAR_ESP32_SOURCES
  src/Math/FastTrig.cpp
  src/Screen/Debug.cpp
  src/lib/fmt/RuntimeError.cxx
  src/system/FileUtil.cpp
  src/system/Path.cpp
  src/ui/canvas/BufferCanvas.cpp
  src/ui/canvas/Color.cpp
  src/ui/canvas/FontSearch.cpp
  src/ui/canvas/Util.cpp
  src/ui/canvas/custom/Cache.cpp
  src/ui/canvas/custom/MoreCanvas.cpp
  src/ui/canvas/custom/Pen.cpp
  src/ui/canvas/fb/TopCanvas.cpp
  src/ui/canvas/freetype/Font.cpp
  src/ui/canvas/freetype/Init.cpp
  src/ui/canvas/memory/Canvas.cpp
  src/ui/canvas/memory/RawBitmap.cpp
  src/ui/canvas/memory/SubCanvas.cpp
  src/ui/canvas/memory/VirtualCanvas.cpp
  src/util/StringCompare.cxx
  src/util/UTF8.cpp
  esp32/xcsoar/src/Main.cpp
)

set(XCSOAR_ESP32_DEFINES
  HAVE_POSIX
  USE_MEMORY_CANVAS
  USE_FREETYPE
  USE_EXTERNAL_FB
  FMT_HEADER_ONLY
)

set(XCSOAR_ESP32_INCLUDE_DIRS
  src
  src/unix
  esp32/xcsoar/include
)

# Download libfmt (used header-only) and set fmt_SOURCE_DIR.
macro(xcsoar_fetch_fmt)
  include(FetchContent)
  if(POLICY CMP0169)
    # allow FetchContent_Populate(), which does not add_subdirectory()
    cmake_policy(SET CMP0169 OLD)
  endif()
  FetchContent_Declare(fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 11.2.0
    GIT_SHALLOW TRUE)
  FetchContent_GetProperties(fmt)
  if(NOT fmt_POPULATED)
    FetchContent_Populate(fmt)
  endif()
endmacro()

# Generate MathTables.h into OUT_DIR at configure time.  The generator
# is built with the host compiler, which is not the target compiler
# in an ESP-IDF build.
function(xcsoar_generate_math_tables XCSOAR_ROOT OUT_DIR)
  set(header ${OUT_DIR}/MathTables.h)
  set(source ${XCSOAR_ROOT}/tools/GenerateSineTables.cpp)
  if(EXISTS ${header} AND NOT ${source} IS_NEWER_THAN ${header})
    return()
  endif()

  find_program(XCSOAR_HOST_CXX NAMES c++ g++ clang++ REQUIRED)
  file(MAKE_DIRECTORY ${OUT_DIR})
  set(tool ${OUT_DIR}/GenerateSineTables)
  execute_process(
    COMMAND ${XCSOAR_HOST_CXX} -std=c++20 -O2 -I${XCSOAR_ROOT}/src
            -o ${tool} ${source}
    RESULT_VARIABLE result)
  if(result)
    message(FATAL_ERROR "Failed to build GenerateSineTables")
  endif()

  execute_process(COMMAND ${tool} OUTPUT_FILE ${header}.tmp
                  RESULT_VARIABLE result)
  if(result)
    message(FATAL_ERROR "GenerateSineTables failed")
  endif()
  file(RENAME ${header}.tmp ${header})
endfunction()
