# The third-party libraries XCSoar needs, built from source for the
# ESP-IDF component: the same releases as build/python/build/libs.py.
# They are downloaded once into XCSOAR_ROOT/output/esp32-libs.

include(FetchContent)

# Download and unpack a source tarball; sets <name>_SOURCE_DIR.
function(xcsoar_fetch name url sha256)
  if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
  endif()
  set(download_dir ${XCSOAR_ROOT}/output/esp32-libs)
  get_filename_component(archive ${url} NAME)
  string(REGEX REPLACE "\\.tar\\.(gz|xz|bz2)$" "" base ${archive})
  FetchContent_Declare(${name}
    URL ${url}
    URL_HASH SHA256=${sha256}
    DOWNLOAD_DIR ${download_dir}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    SOURCE_DIR ${download_dir}/${base})
  FetchContent_GetProperties(${name})
  if(NOT ${name}_POPULATED)
    FetchContent_Populate(${name})
  endif()
  set(${name}_SOURCE_DIR ${${name}_SOURCE_DIR} PARENT_SCOPE)
endfunction()

# Add the libraries and link them into target.  idf_deps are the
# ESP-IDF component libraries (e.g. idf::lwip) whose headers the
# libraries need.
function(xcsoar_add_third_party target idf_deps)
  # zlib
  xcsoar_fetch(zlib
    https://github.com/madler/zlib/releases/download/v1.3.2/zlib-1.3.2.tar.xz
    d7a0654783a4da529d1bb793b7ad9c3318020af77667bcae35f95d0e42a792f3)
  set(zlib_dir ${zlib_SOURCE_DIR})
  add_library(xcsoar_zlib STATIC
    ${zlib_dir}/adler32.c ${zlib_dir}/compress.c ${zlib_dir}/crc32.c
    ${zlib_dir}/deflate.c ${zlib_dir}/gzclose.c ${zlib_dir}/gzlib.c
    ${zlib_dir}/gzread.c ${zlib_dir}/gzwrite.c ${zlib_dir}/infback.c
    ${zlib_dir}/inffast.c ${zlib_dir}/inflate.c ${zlib_dir}/inftrees.c
    ${zlib_dir}/trees.c ${zlib_dir}/uncompr.c ${zlib_dir}/zutil.c)
  target_include_directories(xcsoar_zlib PUBLIC ${zlib_dir})
  target_compile_definitions(xcsoar_zlib PUBLIC ZLIB_CONST PRIVATE Z_HAVE_UNISTD_H)
  target_link_libraries(xcsoar_zlib PRIVATE ${idf_deps})

  # libpng, with the default configuration and no SIMD code
  xcsoar_fetch(libpng
    https://downloads.sourceforge.net/project/libpng/libpng16/1.6.43/libpng-1.6.43.tar.xz
    6a5ca0652392a2d7c9db2ae5b40210843c0bbc081cbd410825ab00cc59f14a6c)
  set(png_dir ${libpng_SOURCE_DIR})
  set(png_config_dir ${CMAKE_CURRENT_BINARY_DIR}/libpng)
  configure_file(${png_dir}/scripts/pnglibconf.h.prebuilt
    ${png_config_dir}/pnglibconf.h COPYONLY)
  add_library(xcsoar_png STATIC
    ${png_dir}/png.c ${png_dir}/pngerror.c ${png_dir}/pngget.c
    ${png_dir}/pngmem.c ${png_dir}/pngpread.c ${png_dir}/pngread.c
    ${png_dir}/pngrio.c ${png_dir}/pngrtran.c ${png_dir}/pngrutil.c
    ${png_dir}/pngset.c ${png_dir}/pngtrans.c ${png_dir}/pngwio.c
    ${png_dir}/pngwrite.c ${png_dir}/pngwtran.c ${png_dir}/pngwutil.c)
  target_include_directories(xcsoar_png PUBLIC ${png_dir} ${png_config_dir})
  target_compile_definitions(xcsoar_png PRIVATE PNG_ARM_NEON_OPT=0)
  target_link_libraries(xcsoar_png PUBLIC xcsoar_zlib PRIVATE ${idf_deps})

  # Lua, as build/python/build/lua.py builds it
  xcsoar_fetch(lua
    https://www.lua.org/ftp/lua-5.4.6.tar.gz
    7d5ea1b9cb6aa0b59ca3dde1c6adcb57ef83a1ba8e5432c0ecd06bf439b3ad88)
  file(GLOB lua_sources ${lua_SOURCE_DIR}/src/*.c)
  list(REMOVE_ITEM lua_sources
    ${lua_SOURCE_DIR}/src/lua.c ${lua_SOURCE_DIR}/src/luac.c)
  add_library(xcsoar_lua STATIC ${lua_sources})
  target_include_directories(xcsoar_lua PUBLIC ${lua_SOURCE_DIR}/src)
  target_compile_definitions(xcsoar_lua PRIVATE
    "lua_getlocaledecpoint()='.'")
  target_link_libraries(xcsoar_lua PRIVATE ${idf_deps})

  # IJG libjpeg: libjpeg-turbo (build/python/build/libs.py) does not
  # allow being built inside another CMake project; XCSoar only uses
  # the API both share
  xcsoar_fetch(libjpeg
    https://www.ijg.org/files/jpegsrc.v9f.tar.gz
    04705c110cb2469caa79fb71fba3d7bf834914706e9641a4589485c1f832565b)
  set(jpeg_dir ${libjpeg_SOURCE_DIR})
  set(jpeg_config_dir ${CMAKE_CURRENT_BINARY_DIR}/libjpeg)
  configure_file(${jpeg_dir}/jconfig.txt ${jpeg_config_dir}/jconfig.h
    COPYONLY)
  set(jpeg_sources
    jaricom jcomapi jutils jerror jmemmgr jmemnobs
    jcapimin jcapistd jcarith jctrans jcparam jdatadst jcinit jcmaster
    jcmarker jcmainct jcprepct jccoefct jccolor jcsample jchuff jcdctmgr
    jfdctfst jfdctflt jfdctint
    jdapimin jdapistd jdarith jdtrans jdatasrc jdmaster jdinput jdmarker
    jdhuff jdmainct jdcoefct jdpostct jddctmgr jidctfst jidctflt jidctint
    jdsample jdcolor jquant1 jquant2 jdmerge)
  list(TRANSFORM jpeg_sources PREPEND ${jpeg_dir}/)
  list(TRANSFORM jpeg_sources APPEND .c)
  add_library(xcsoar_jpeg STATIC ${jpeg_sources})
  target_include_directories(xcsoar_jpeg PUBLIC ${jpeg_dir} ${jpeg_config_dir})
  target_link_libraries(xcsoar_jpeg PRIVATE ${idf_deps})

  # c-ares
  xcsoar_fetch(cares
    https://github.com/c-ares/c-ares/releases/download/v1.34.8/c-ares-1.34.8.tar.gz
    c222b6d681096f9444d2c4863d2c1174019e27cacca0a4a5c114d36dd7d7bf78)
  # c-ares's configure checks cannot run with the ESP-IDF toolchain,
  # so it builds with the configuration in esp32/cares
  file(GLOB_RECURSE cares_sources ${cares_SOURCE_DIR}/src/lib/*.c)
  list(FILTER cares_sources EXCLUDE REGEX "/thirdparty/")
  add_library(xcsoar_cares STATIC ${cares_sources})
  # before c-ares's own ares_build.h
  target_include_directories(xcsoar_cares BEFORE PUBLIC
    ${XCSOAR_ROOT}/esp32/cares)
  target_include_directories(xcsoar_cares PUBLIC ${cares_SOURCE_DIR}/include
    PRIVATE ${cares_SOURCE_DIR}/src/lib ${cares_SOURCE_DIR}/src/lib/include)
  target_compile_definitions(xcsoar_cares
    PUBLIC CARES_STATICLIB
    PRIVATE HAVE_CONFIG_H CARES_BUILDING_LIBRARY)
  target_link_libraries(xcsoar_cares PRIVATE ${idf_deps})

  target_link_libraries(${target} PRIVATE
    xcsoar_zlib xcsoar_png xcsoar_lua xcsoar_jpeg xcsoar_cares)
endfunction()
