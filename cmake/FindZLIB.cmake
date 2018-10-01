if(NOT PREFER_BUNDLED_LIBS)
  set(CMAKE_MODULE_PATH ${ORIGINAL_CMAKE_MODULE_PATH})
  find_package(ZLIB)
  set(CMAKE_MODULE_PATH ${OWN_CMAKE_MODULE_PATH})
  if(ZLIB_FOUND)
    set(ZLIB_BUNDLED OFF)
    set(ZLIB_DEP)
  endif()
endif()

if(NOT ZLIB_FOUND)
  set(ZLIB_BUNDLED ON)
  set(ZLIB_SRC_DIR src/engine/external/zlib)
  set_glob(ZLIB_SRC GLOB ${ZLIB_SRC_DIR}
    adler32.c
    compress.c
    crc32.c
    crc32.h
    deflate.c
    deflate.h
    gzguts.h
    infback.c
    inffast.c
    inffast.h
    inffixed.h
    inflate.c
    inflate.h
    inftrees.c
    inftrees.h
    trees.c
    trees.h
    uncompr.c
    zconf.h
    zlib.h
    zutil.c
    zutil.h
  )
  add_library(zlib EXCLUDE_FROM_ALL OBJECT ${ZLIB_SRC})
  set(ZLIB_INCLUDEDIR ${ZLIB_SRC_DIR})
  target_include_directories(zlib PRIVATE ${ZLIB_INCLUDEDIR})

  set(ZLIB_DEP $<TARGET_OBJECTS:zlib>)
  set(ZLIB_INCLUDE_DIRS ${ZLIB_INCLUDEDIR})
  set(ZLIB_LIBRARIES)

  list(APPEND TARGETS_DEP zlib)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(ZLIB DEFAULT_MSG ZLIB_INCLUDEDIR)
endif()
