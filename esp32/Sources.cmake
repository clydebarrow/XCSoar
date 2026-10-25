# XCSoar sources and flags shared by the ESP-IDF component and the
# host test build.  Paths are relative to XCSOAR_ROOT.

set(XCSOAR_ESP32_SOURCES
  src/Math/FastTrig.cpp
  src/event/CoarseTimerEvent.cxx
  src/event/DeferEvent.cxx
  src/event/FineTimerEvent.cxx
  src/event/InjectEvent.cxx
  src/event/Loop.cxx
  src/event/PollBackend.cxx
  src/event/SocketEvent.cxx
  src/event/TimerList.cxx
  src/event/TimerWheel.cxx
  src/Screen/Debug.cpp
  src/lib/fmt/RuntimeError.cxx
  src/system/EventFD.cxx
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
  src/ui/event/Globals.cpp
  src/ui/event/Idle.cpp
  src/ui/event/Notify.cpp
  src/ui/event/poll/InputQueue.cpp
  src/ui/event/poll/Loop.cpp
  src/ui/event/poll/Queue.cpp
  src/ui/event/poll/Timer.cpp
  src/ui/window/BufferWindow.cpp
  src/ui/window/ContainerWindow.cpp
  src/ui/window/DoubleBufferWindow.cpp
  src/ui/window/Init.cpp
  src/ui/window/SingleWindow.cpp
  src/ui/window/Window.cpp
  src/ui/window/custom/ContainerWindow.cpp
  src/ui/window/custom/SingleWindow.cpp
  src/ui/window/custom/TopWindow.cpp
  src/ui/window/custom/WList.cpp
  src/ui/window/custom/Window.cpp
  src/ui/window/fb/SingleWindow.cpp
  src/ui/window/fb/TopWindow.cpp
  src/ui/window/fb/Window.cpp
  src/ui/window/poll/TopWindow.cpp
  src/util/StringCompare.cxx
  src/util/UTF8.cpp
  esp32/xcsoar/src/Demo.cpp
  esp32/xcsoar/src/Host.cpp
)

set(XCSOAR_ESP32_DEFINES
  HAVE_POSIX
  USE_MEMORY_CANVAS
  USE_FREETYPE
  USE_EXTERNAL_FB
  USE_VFB
  USE_POLL_EVENT
  USE_INJECTED_INPUT
  FMT_HEADER_ONLY
)

set(XCSOAR_ESP32_INCLUDE_DIRS
  src
  src/unix
  esp32/xcsoar/include
)

# Download the header-only libraries into XCSOAR_ROOT/output/esp32-libs,
# shared by all builds, and set fmt_SOURCE_DIR and boost_SOURCE_DIR.
macro(xcsoar_fetch_libraries)
  include(FetchContent)
  if(POLICY CMP0169)
    # allow FetchContent_Populate(), which does not add_subdirectory()
    cmake_policy(SET CMP0169 OLD)
  endif()
  set(xcsoar_download_dir ${XCSOAR_ROOT}/output/esp32-libs)

  FetchContent_Declare(fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 11.2.0
    GIT_SHALLOW TRUE
    SOURCE_DIR ${xcsoar_download_dir}/fmt-11.2.0)
  FetchContent_GetProperties(fmt)
  if(NOT fmt_POPULATED)
    FetchContent_Populate(fmt)
  endif()

  # the same release as build/libboost.mk
  FetchContent_Declare(boost
    URL https://archives.boost.io/release/1.92.0/source/boost_1_92_0.tar.bz2
    URL_HASH SHA256=5c1d40cb8e19adbf740a4ec2da35b3e58f3f5804b1dce44deb53df72193cbc6c
    DOWNLOAD_DIR ${xcsoar_download_dir}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_DIR ${xcsoar_download_dir}/boost_1_92_0)
  FetchContent_GetProperties(boost)
  if(NOT boost_POPULATED)
    FetchContent_Populate(boost)
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
