if(NOT CMAKE_CROSSCOMPILING)
  find_package(PkgConfig QUIET)
  pkg_check_modules(PC_SDL sdl)
endif()

set_extra_dirs_lib(SDL sdl)
find_library(SDL_LIBRARY
  NAMES SDL
  HINTS ${HINTS_SDL_LIBDIR} ${PC_SDL_LIBDIR} ${PC_SDL_LIBRARY_DIRS}
  PATHS ${PATHS_SDL_LIBDIR}
  ${CROSSCOMPILING_NO_CMAKE_SYSTEM_PATH}
)
set(CMAKE_FIND_FRAMEWORK FIRST)
set_extra_dirs_include(SDL sdl "${SDL_LIBRARY}")
# Looking for 'SDL.h' directly might accidentally find a SDL 2 instead of SDL
# installation. Look for a header file only present in SDL instead.
find_path(SDL_INCLUDEDIR SDL_active.h
  PATH_SUFFIXES SDL
  HINTS ${HINTS_SDL_INCLUDEDIR} ${PC_SDL_INCLUDEDIR} ${PC_SDL_INCLUDE_DIRS}
  PATHS ${PATHS_SDL_INCLUDEDIR}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SDL DEFAULT_MSG SDL_LIBRARY SDL_INCLUDEDIR)

mark_as_advanced(SDL_LIBRARY SDL_INCLUDEDIR)

if(SDL_FOUND)
  set(SDL_LIBRARIES ${SDL_LIBRARY})
  set(SDL_INCLUDE_DIRS ${SDL_INCLUDEDIR})

  is_bundled(SDL_BUNDLED "${SDL_LIBRARY}")
  if(SDL_BUNDLED AND TARGET_OS STREQUAL "windows")
    set(SDL_COPY_FILES "${EXTRA_SDL_LIBDIR}/SDL.dll")
  else()
    set(SDL_COPY_FILES)
  endif()
endif()
