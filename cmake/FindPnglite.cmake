if(NOT PREFER_BUNDLED_LIBS)
  if(NOT CMAKE_CROSSCOMPILING)
    find_package(PkgConfig QUIET)
    pkg_check_modules(PC_PNGLITE pnglite)
  endif()

  find_library(PNGLITE_LIBRARY
    NAMES pnglite
    HINTS ${PC_PNGLITE_LIBDIR} ${PC_PNGLITE_LIBRARY_DIRS}
    ${CROSSCOMPILING_NO_CMAKE_SYSTEM_PATH}
  )
  find_path(PNGLITE_INCLUDEDIR
    NAMES pnglite.h
    HINTS ${PC_PNGLITE_INCLUDEDIR} ${PC_PNGLITE_INCLUDE_DIRS}
    ${CROSSCOMPILING_NO_CMAKE_SYSTEM_PATH}
  )

  mark_as_advanced(PNGLITE_LIBRARY PNGLITE_INCLUDEDIR)

  if(PNGLITE_LIBRARY AND PNGLITE_INCLUDEDIR)
    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(Pnglite DEFAULT_MSG PNGLITE_LIBRARY PNGLITE_INCLUDEDIR)

    set(PNGLITE_LIBRARIES ${PNGLITE_LIBRARY})
    set(PNGLITE_INCLUDE_DIRS ${PNGLITE_INCLUDEDIR})
    set(PNGLITE_BUNDLED OFF)
  endif()
endif()

if(NOT PNGLITE_FOUND)
  set(PNGLITE_SRC_DIR src/engine/external/pnglite)
  set_glob(PNGLITE_SRC GLOB ${PNGLITE_SRC_DIR} pnglite.c pnglite.h)
  add_library(pnglite EXCLUDE_FROM_ALL OBJECT ${PNGLITE_SRC})
  list(APPEND TARGETS_DEP pnglite)

  set(PNGLITE_INCLUDEDIR ${PNGLITE_SRC_DIR})
  target_include_directories(pnglite PRIVATE ${ZLIB_INCLUDE_DIRS})

  set(PNGLITE_DEP $<TARGET_OBJECTS:pnglite>)
  set(PNGLITE_INCLUDE_DIRS ${PNGLITE_INCLUDEDIR})
  set(PNGLITE_LIBRARIES)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Pnglite DEFAULT_MSG PNGLITE_INCLUDEDIR)
  set(PNGLITE_BUNDLED ON)
endif()
