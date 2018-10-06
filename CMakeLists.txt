cmake_minimum_required(VERSION 2.8)

file(STRINGS src/game/version.h VERSION_LINE
  LIMIT_COUNT 1
  REGEX GAME_RELEASE_VERSION
)

if(VERSION_LINE MATCHES "\"([0-9]+)\\.([0-9]+)\\.([0-9]+)\"")
  set(VERSION_MAJOR ${CMAKE_MATCH_1})
  set(VERSION_MINOR ${CMAKE_MATCH_2})
  set(VERSION_PATCH ${CMAKE_MATCH_3})
elseif(VERSION_LINE MATCHES "\"([0-9]+)\\.([0-9]+)\"")
  set(VERSION_MAJOR ${CMAKE_MATCH_1})
  set(VERSION_MINOR ${CMAKE_MATCH_2})
  set(VERSION_PATCH "0")
else()
  message(FATAL_ERROR "Couldn't parse version from src/game/version.h")
endif()

if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW)
  if(VERSION_PATCH STREQUAL "0")
    project(teeworlds VERSION ${VERSION_MAJOR}.${VERSION_MINOR})
  else()
    project(teeworlds VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
  endif()
else()
  project(teeworlds)
  set(PROJECT_VERSION_MAJOR ${VERSION_MAJOR})
  set(PROJECT_VERSION_MINOR ${VERSION_MINOR})
  set(PROJECT_VERSION_PATCH ${VERSION_PATCH})
  if(VERSION_PATCH STREQUAL "0")
    set(PROJECT_VERSION ${VERSION_MAJOR}.${VERSION_MINOR})
  else()
    set(PROJECT_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})
  endif()
endif()

set(ORIGINAL_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH})
set(ORIGINAL_CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES})
set(ORIGINAL_CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
set(OWN_CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_MODULE_PATH ${OWN_CMAKE_MODULE_PATH})

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(TARGET_BITS "64")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(TARGET_BITS "32")
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(TARGET_OS "windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(TARGET_OS "linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(TARGET_OS "mac")
endif()

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckSymbolExists)

check_symbol_exists(__i386 "" TARGET_ARCH_X86_i386)
if(TARGET_ARCH_X86_i386)
  set(TARGET_ARCH x86)
else()
  set(TARGET_ARCH)
endif()

set(AUTO_DEPENDENCIES_DEFAULT OFF)
if(TARGET_OS STREQUAL "windows")
  set(AUTO_DEPENDENCIES_DEFAULT ON)
endif()

option(CLIENT "Compile client" ON)
option(PREFER_BUNDLED_LIBS "Prefer bundled libraries over system libraries" ${AUTO_DEPENDENCIES_DEFAULT})
option(DEV "Don't generate stuff necessary for packaging" OFF)

# Set the default build type to Release
if(NOT(CMAKE_BUILD_TYPE))
  if(DEV)
    set(CMAKE_BUILD_TYPE Release)
  else()
    set(CMAKE_BUILD_TYPE Debug)
  endif()
endif()

set(DBG $<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  src/game/version.h
)

set(SERVER_EXECUTABLE teeworlds_srv CACHE STRING "Name of the built server executable")
set(CLIENT_EXECUTABLE teeworlds CACHE STRING "Name of the build client executable")

########################################################################
# Compiler flags
########################################################################

function(add_c_compiler_flag_if_supported VARIABLE FLAG)
  if(ARGC GREATER 2)
    set(CHECKED_FLAG "${ARGV2}")
  else()
    set(CHECKED_FLAG "${FLAG}")
  endif()
  string(REGEX REPLACE "[^A-Za-z0-9]" "_" CONFIG_VARIABLE "FLAG_SUPPORTED${CHECKED_FLAG}")
  check_c_compiler_flag("${CHECKED_FLAG}" ${CONFIG_VARIABLE})
  if(${CONFIG_VARIABLE})
    if(${VARIABLE})
      set("${VARIABLE}" "${${VARIABLE}};${FLAG}" PARENT_SCOPE)
    else()
      set("${VARIABLE}" "${FLAG}" PARENT_SCOPE)
    endif()
  endif()
endfunction()

if(NOT MSVC)
  # Protect the stack pointer.
  # -fstack-protector-all doesn't work on MinGW.
  add_c_compiler_flag_if_supported(OUR_FLAGS -fstack-protector-all)

  # Inaccurate floating point numbers cause problems on mingw-w64-gcc when
  # compiling for x86, might cause problems elsewhere. So don't store floats
  # in registers but keep them at higher accuracy.

  if(TARGET_ARCH STREQUAL "x86")
    add_c_compiler_flag_if_supported(OUR_FLAGS -ffloat-store)
  endif()

  if(TARGET_OS STREQUAL "mac")
    add_c_compiler_flag_if_supported(OUR_FLAGS -stdlib=libc++)
    add_c_compiler_flag_if_supported(OUR_FLAGS -mmacosx-version-min=10.7)
  endif()

  add_c_compiler_flag_if_supported(OUR_FLAGS_OWN -Wall)
  if(CMAKE_VERSION VERSION_GREATER 3.3 OR CMAKE_VERSION VERSION_EQUAL 3.3)
    add_c_compiler_flag_if_supported(OUR_FLAGS_OWN
      $<$<COMPILE_LANGUAGE:C>:-Wdeclaration-after-statement>
      -Wdeclaration-after-statement
    )
  endif()
  add_c_compiler_flag_if_supported(OUR_FLAGS_OWN -Wextra)
  add_c_compiler_flag_if_supported(OUR_FLAGS_OWN -Wno-unused-parameter)
  add_c_compiler_flag_if_supported(OUR_FLAGS_OWN -Wno-missing-field-initializers)
  add_c_compiler_flag_if_supported(OUR_FLAGS_OWN -Wformat=2) # Warn about format strings.
  add_c_compiler_flag_if_supported(OUR_FLAGS_DEP -Wno-implicit-function-declaration)
endif()

if(NOT MSVC)
  check_c_compiler_flag("-O2;-Wp,-Werror;-D_FORTIFY_SOURCE=2" DEFINE_FORTIFY_SOURCE) # Some distributions define _FORTIFY_SOURCE by themselves.
endif()

########################################################################
# COMMON FUNCTIONS
########################################################################

function(set_glob VAR GLOBBING DIRECTORY) # ...
  file(${GLOBBING} GLOB_RESULT "${DIRECTORY}/*.c" "${DIRECTORY}/*.cpp" "${DIRECTORY}/*.h")
  list(SORT GLOB_RESULT)
  set(FILES)
  foreach(file ${ARGN})
    list(APPEND FILES "${PROJECT_SOURCE_DIR}/${DIRECTORY}/${file}")
  endforeach()

  if(NOT FILES STREQUAL GLOB_RESULT)
    message(AUTHOR_WARNING "${VAR} does not contain every file from directory ${DIRECTORY}")
    set(LIST_BUT_NOT_GLOB)
    if(POLICY CMP0057)
      cmake_policy(SET CMP0057 NEW)
      foreach(file ${FILES})
        if(NOT file IN_LIST GLOB_RESULT)
          list(APPEND LIST_BUT_NOT_GLOB ${file})
        endif()
      endforeach()
      if(LIST_BUT_NOT_GLOB)
        message(AUTHOR_WARNING "Entries only present in ${VAR}: ${LIST_BUT_NOT_GLOB}")
      endif()
      set(GLOB_BUT_NOT_LIST)
      foreach(file ${GLOB_RESULT})
        if(NOT file IN_LIST FILES)
          list(APPEND GLOB_BUT_NOT_LIST ${file})
        endif()
      endforeach()
      if(GLOB_BUT_NOT_LIST)
        message(AUTHOR_WARNING "Entries only present in ${DIRECTORY}: ${GLOB_BUT_NOT_LIST}")
      endif()
    endif()
  endif()

  set(${VAR} ${FILES} PARENT_SCOPE)
endfunction()

########################################################################
# INITIALIZE TARGET LISTS
########################################################################

set(TARGETS_OWN)
set(TARGETS_DEP)

set(TARGETS_LINK) # Targets with a linking stage.

########################################################################
# DEPENDENCIES
########################################################################

function(set_extra_dirs_lib VARIABLE NAME)
  set("PATHS_${VARIABLE}_LIBDIR" PARENT_SCOPE)
  set("HINTS_${VARIABLE}_LIBDIR" PARENT_SCOPE)
  if(PREFER_BUNDLED_LIBS)
    set(TYPE HINTS)
  else()
    set(TYPE PATHS)
  endif()
  if(TARGET_BITS AND TARGET_OS)
    set(DIR "other/${NAME}/${TARGET_OS}/lib${TARGET_BITS}")
    set("${TYPE}_${VARIABLE}_LIBDIR" "${DIR}" PARENT_SCOPE)
    set("EXTRA_${VARIABLE}_LIBDIR" "${DIR}" PARENT_SCOPE)
  endif()
endfunction()

function(set_extra_dirs_include VARIABLE NAME LIBRARY)
  set("PATHS_${VARIABLE}_INCLUDEDIR" PARENT_SCOPE)
  set("HINTS_${VARIABLE}_INCLUDEDIR" PARENT_SCOPE)
  is_bundled(IS_BUNDLED "${LIBRARY}")
  if(IS_BUNDLED)
    set("HINTS_${VARIABLE}_INCLUDEDIR" "other/${NAME}/include" "other/${NAME}/include/${TARGET_OS}" PARENT_SCOPE)
  endif()
endfunction()

if(CMAKE_CROSSCOMPILING)
  set(CROSSCOMPILING_NO_CMAKE_SYSTEM_PATH NO_CMAKE_SYSTEM_PATH)
else()
  set(CROSSCOMPILING_NO_CMAKE_SYSTEM_PATH)
endif()

function(is_bundled VARIABLE PATH)
  if(PATH)
    string(FIND "${PATH}" "${PROJECT_SOURCE_DIR}" LOCAL_PATH_POS)
    if(LOCAL_PATH_POS EQUAL 0 AND TARGET_BITS AND TARGET_OS)
      set("${VARIABLE}" ON PARENT_SCOPE)
    else()
      set("${VARIABLE}" OFF PARENT_SCOPE)
    endif()
  else()
    set("${VARIABLE}" OFF PARENT_SCOPE)
  endif()
endfunction()

if(NOT CMAKE_CROSSCOMPILING)
  # Check for PkgConfig once so all the other `find_package` calls can do it
  # quietly.
  find_package(PkgConfig)
endif()
find_package(ZLIB)
find_package(Freetype)
find_package(Git)
find_package(Pnglite)
find_package(PythonInterp)
find_package(SDL)
find_package(Threads)
find_package(Wavpack)


if(TARGET_OS AND TARGET_OS STREQUAL "mac")
  find_program(DMG dmg)
  find_program(HFSPLUS hfsplus)
  find_program(NEWFS_HFS newfs_hfs)
  if(DMG AND HFSPLUS AND NEWFS_HFS)
    set(DMGTOOLS_FOUND ON)
  else()
    set(DMGTOOLS_FOUND OFF)
  endif()

  find_program(HDIUTIL hdiutil)
endif()

message(STATUS "******** Teeworlds ********")
message(STATUS "Target OS: ${TARGET_OS} ${TARGET_BITS}bit")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

message(STATUS "Dependencies:")
function(show_dependency_status OUTPUT_NAME NAME)
  if(${NAME}_FOUND)
    if(${NAME}_BUNDLED)
      message(STATUS " * ${OUTPUT_NAME} not found (using bundled version)")
    else()
      message(STATUS " * ${OUTPUT_NAME} found")
    endif()
  else()
    message(STATUS " * ${OUTPUT_NAME} not found")
  endif()
endfunction()

if(TARGET_OS AND TARGET_OS STREQUAL "mac")
  show_dependency_status("Dmg tools" DMGTOOLS)
endif()
show_dependency_status("Freetype" FREETYPE)
if(TARGET_OS AND TARGET_OS STREQUAL "mac")
  show_dependency_status("Hdiutil" HDIUTIL)
endif()
show_dependency_status("Pnglite" PNGLITE)
show_dependency_status("PythonInterp" PYTHONINTERP)
show_dependency_status("SDL" SDL)
show_dependency_status("Wavpack" WAVPACK)
show_dependency_status("Zlib" ZLIB)

if(NOT(PYTHONINTERP_FOUND))
  message(SEND_ERROR "You must install Python to compile Teeworlds")
endif()

if(CLIENT AND NOT(FREETYPE_FOUND))
  message(SEND_ERROR "You must install Freetype to compile the Teeworlds client")
endif()
if(CLIENT AND NOT(SDL_FOUND))
  message(SEND_ERROR "You must install SDL to compile the Teeworlds client")
endif()

if(TARGET_OS STREQUAL "windows")
  set(PLATFORM_CLIENT)
  set(PLATFORM_CLIENT_LIBS opengl32 glu32 winmm)
  set(PLATFORM_LIBS ws2_32) # Windows sockets
elseif(TARGET_OS STREQUAL "mac")
  find_library(CARBON Carbon)
  find_library(COCOA Cocoa)
  find_library(OPENGL OpenGL)
  set(PLATFORM_CLIENT
    src/osxlaunch/client.h
    src/osxlaunch/client.m
  )
  set(PLATFORM_CLIENT_LIBS ${COCOA} ${OPENGL})
  set(PLATFORM_LIBS ${CARBON})
else()
  set(PLATFORM_CLIENT)
  set(PLATFORM_CLIENT_LIBS GL GLU X11)
  if(TARGET_OS STREQUAL "linux")
    set(PLATFORM_LIBS rt) # clock_gettime for glibc < 2.17
  else()
    set(PLATFORM_LIBS)
  endif()
endif()

########################################################################
# DEPENDENCY COMPILATION
########################################################################

set_glob(DEP_MD5_SRC GLOB src/engine/external/md5 md5.c md5.h)
add_library(md5 EXCLUDE_FROM_ALL OBJECT ${DEP_MD5_SRC})

list(APPEND TARGETS_DEP md5)
set(DEP_MD5 $<TARGET_OBJECTS:md5>)

########################################################################
# COPY DATA AND DLLS
########################################################################

file(COPY data DESTINATION .)
set(COPY_FILES
  ${FREETYPE_COPY_FILES}
  ${SDL_COPY_FILES}
)
file(COPY ${COPY_FILES} DESTINATION .)

########################################################################
# CODE GENERATION
########################################################################

function(chash output_file)
  add_custom_command(OUTPUT ${output_file}
    COMMAND ${PYTHON_EXECUTABLE} scripts/cmd5.py ${ARGN}
      > "${PROJECT_BINARY_DIR}/${output_file}"
    DEPENDS scripts/cmd5.py ${ARGN}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )
endfunction()

function(generate_source output_file script_parameter)
  add_custom_command(OUTPUT ${output_file}
    COMMAND ${PYTHON_EXECUTABLE} datasrc/compile.py ${script_parameter}
      > "${PROJECT_BINARY_DIR}/${output_file}"
    DEPENDS
      datasrc/compile.py
      datasrc/content.py
      datasrc/datatypes.py
      datasrc/network.py
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  )
endfunction()

file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/src/game/generated/")
if(GIT_FOUND)
  execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --git-dir
    ERROR_QUIET
    OUTPUT_VARIABLE PROJECT_GIT_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE PROJECT_GIT_DIR_ERROR
  )
  if(NOT PROJECT_GIT_DIR_ERROR)
    set(GIT_REVISION_EXTRA_DEPS
      ${PROJECT_GIT_DIR}/index
      ${PROJECT_GIT_DIR}/logs/HEAD
    )
  endif()
endif()
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/src/game/generated/git_revision.cpp
  COMMAND ${PYTHON_EXECUTABLE}
    scripts/git_revision.py
    > ${PROJECT_BINARY_DIR}/src/game/generated/git_revision.cpp
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  DEPENDS
    ${GIT_REVISION_EXTRA_DEPS}
    scripts/git_revision.py
)
chash("src/game/generated/nethash.cpp"
  "src/engine/shared/protocol.h"
  "${PROJECT_BINARY_DIR}/src/game/generated/protocol.h"
  "src/game/tuning.h"
  "src/game/gamecore.cpp"
)
generate_source("src/game/generated/client_data.cpp" "client_content_source")
generate_source("src/game/generated/client_data.h" "client_content_header")
generate_source("src/game/generated/protocol.cpp" "network_source")
generate_source("src/game/generated/protocol.h" "network_header")
generate_source("src/game/generated/server_data.cpp" "server_content_source")
generate_source("src/game/generated/server_data.h" "server_content_header")


########################################################################
# SHARED
########################################################################

# Sources
set_glob(BASE GLOB_RECURSE src/base
  detect.h
  math.h
  system.c
  system.h
  tl/algorithm.h
  tl/allocator.h
  tl/array.h
  tl/base.h
  tl/range.h
  tl/sorted_array.h
  tl/string.h
  tl/threading.h
  vmath.h
)
set_glob(ENGINE_INTERFACE GLOB src/engine
  client.h
  config.h
  console.h
  demo.h
  editor.h
  engine.h
  friends.h
  graphics.h
  input.h
  kernel.h
  keys.h
  map.h
  masterserver.h
  message.h
  server.h
  serverbrowser.h
  sound.h
  storage.h
  textrender.h
)
set_glob(ENGINE_SHARED GLOB src/engine/shared
  compression.cpp
  compression.h
  config.cpp
  config.h
  config_variables.h
  console.cpp
  console.h
  datafile.cpp
  datafile.h
  demo.cpp
  demo.h
  econ.cpp
  econ.h
  engine.cpp
  filecollection.cpp
  filecollection.h
  huffman.cpp
  huffman.h
  jobs.cpp
  jobs.h
  kernel.cpp
  linereader.cpp
  linereader.h
  map.cpp
  mapchecker.cpp
  mapchecker.h
  masterserver.cpp
  memheap.cpp
  memheap.h
  message.h
  netban.cpp
  netban.h
  network.cpp
  network.h
  network_client.cpp
  network_conn.cpp
  network_console.cpp
  network_console_conn.cpp
  network_server.cpp
  network_server_hack.cpp
  packer.cpp
  packer.h
  protocol.h
  ringbuffer.cpp
  ringbuffer.h
  snapshot.cpp
  snapshot.h
  storage.cpp
)
set(ENGINE_GENERATED_SHARED src/game/generated/protocol.cpp src/game/generated/protocol.h)
set_glob(GAME_SHARED GLOB src/game
  collision.cpp
  collision.h
  gamecore.cpp
  gamecore.h
  layers.cpp
  layers.h
  localization.cpp
  localization.h
  mapitems.h
  tuning.h
  variables.h
  version.h
  voting.h
)
set(GAME_GENERATED_SHARED
  src/game/generated/git_revision.cpp
  src/game/generated/nethash.cpp
  src/game/generated/protocol.h
)

set(DEPS ${DEP_MD5} ${ZLIB_DEP})

# Libraries
set(LIBS ${CMAKE_THREAD_LIBS_INIT} ${ZLIB_LIBRARIES} ${PLATFORM_LIBS})

# Targets
add_library(engine-shared EXCLUDE_FROM_ALL OBJECT ${ENGINE_INTERFACE} ${ENGINE_SHARED} ${ENGINE_GENERATED_SHARED} ${BASE})
add_library(game-shared EXCLUDE_FROM_ALL OBJECT ${GAME_SHARED} ${GAME_GENERATED_SHARED})
list(APPEND TARGETS_OWN engine-shared game-shared)


########################################################################
# CLIENT
########################################################################

if(CLIENT)
  # Sources
  set_glob(ENGINE_CLIENT GLOB src/engine/client
    backend_sdl.cpp
    backend_sdl.h
    client.cpp
    client.h
    friends.cpp
    friends.h
    graphics.cpp
    graphics.h
    graphics_threaded.cpp
    graphics_threaded.h
    input.cpp
    input.h
    keynames.h
    serverbrowser.cpp
    serverbrowser.h
    sound.cpp
    sound.h
    text.cpp
  )
  set_glob(GAME_CLIENT GLOB_RECURSE src/game/client
    animstate.cpp
    animstate.h
    component.h
    components/binds.cpp
    components/binds.h
    components/broadcast.cpp
    components/broadcast.h
    components/camera.cpp
    components/camera.h
    components/chat.cpp
    components/chat.h
    components/console.cpp
    components/console.h
    components/controls.cpp
    components/controls.h
    components/countryflags.cpp
    components/countryflags.h
    components/damageind.cpp
    components/damageind.h
    components/debughud.cpp
    components/debughud.h
    components/effects.cpp
    components/effects.h
    components/emoticon.cpp
    components/emoticon.h
    components/flow.cpp
    components/flow.h
    components/hud.cpp
    components/hud.h
    components/items.cpp
    components/items.h
    components/killmessages.cpp
    components/killmessages.h
    components/mapimages.cpp
    components/mapimages.h
    components/maplayers.cpp
    components/maplayers.h
    components/menus.cpp
    components/menus.h
    components/menus_browser.cpp
    components/menus_demo.cpp
    components/menus_ingame.cpp
    components/menus_settings.cpp
    components/motd.cpp
    components/motd.h
    components/nameplates.cpp
    components/nameplates.h
    components/particles.cpp
    components/particles.h
    components/players.cpp
    components/players.h
    components/scoreboard.cpp
    components/scoreboard.h
    components/skins.cpp
    components/skins.h
    components/sounds.cpp
    components/sounds.h
    components/spectator.cpp
    components/spectator.h
    components/voting.cpp
    components/voting.h
    gameclient.cpp
    gameclient.h
    lineinput.cpp
    lineinput.h
    render.cpp
    render.h
    render_map.cpp
    ui.cpp
    ui.h
  )
  set_glob(GAME_EDITOR GLOB src/game/editor
    auto_map.cpp
    auto_map.h
    editor.cpp
    editor.h
    io.cpp
    layer_game.cpp
    layer_quads.cpp
    layer_tiles.cpp
    popups.cpp
  )
  set(GAME_GENERATED_CLIENT
    src/game/generated/client_data.cpp
    src/game/generated/client_data.h
  )
  set(CLIENT_SRC ${ENGINE_CLIENT} ${PLATFORM_CLIENT} ${GAME_CLIENT} ${GAME_EDITOR} ${GAME_GENERATED_CLIENT})

  set(DEPS_CLIENT ${DEPS} ${PNGLITE_DEP} ${WAVPACK_DEP})

  # Libraries
  set(LIBS_CLIENT
    ${LIBS}
    ${FREETYPE_LIBRARIES}
    ${PNGLITE_LIBRARIES}
    ${SDL_LIBRARIES}
    ${WAVPACK_LIBRARIES}
    ${PLATFORM_CLIENT_LIBS}
  )

  if(TARGET_OS STREQUAL "windows")
    set(CLIENT_ICON "other/icons/${CLIENT_EXECUTABLE}.rc")
  else()
    set(CLIENT_ICON)
  endif()

  # Target
  set(TARGET_CLIENT ${CLIENT_EXECUTABLE})
  add_executable(${TARGET_CLIENT}
    ${CLIENT_SRC}
    ${CLIENT_ICON}
    ${CLIENT_MANIFEST}
    ${DEPS_CLIENT}
    $<TARGET_OBJECTS:engine-shared>
    $<TARGET_OBJECTS:game-shared>
  )
  target_link_libraries(${TARGET_CLIENT} ${LIBS_CLIENT})

  target_include_directories(${TARGET_CLIENT} PRIVATE
    ${FREETYPE_INCLUDE_DIRS}
    ${PNGLITE_INCLUDE_DIRS}
    ${SDL_INCLUDE_DIRS}
    ${WAVPACK_INCLUDE_DIRS}
  )

  set(PARAMS "${WAVPACK_INCLUDE_DIRS};${WAVPACK_INCLUDE_DIRS}")
  if(NOT(WAVPACK_OPEN_FILE_INPUT_EX_PARAMS STREQUAL PARAMS))
    unset(WAVPACK_OPEN_FILE_INPUT_EX CACHE)
  endif()
  set(WAVPACK_OPEN_FILE_INPUT_EX_PARAMS "${PARAMS}" CACHE INTERNAL "")

  set(CMAKE_REQUIRED_INCLUDES ${ORIGINAL_CMAKE_REQUIRED_INCLUDES} ${WAVPACK_INCLUDE_DIRS})
  set(CMAKE_REQUIRED_LIBRARIES ${ORIGINAL_CMAKE_REQUIRED_LIBRARIES} ${WAVPACK_LIBRARIES})
  check_symbol_exists(WavpackOpenFileInputEx wavpack.h WAVPACK_OPEN_FILE_INPUT_EX)
  set(CMAKE_REQUIRED_INCLUDES ${ORIGINAL_CMAKE_REQUIRED_INCLUDES})
  set(CMAKE_REQUIRED_LIBRARIES ${ORIGINAL_CMAKE_REQUIRED_LIBRARIES})

  if(WAVPACK_OPEN_FILE_INPUT_EX)
    target_compile_definitions(${TARGET_CLIENT} PRIVATE CONF_WAVPACK_OPEN_FILE_INPUT_EX)
  endif()

  list(APPEND TARGETS_OWN ${TARGET_CLIENT})
  list(APPEND TARGETS_LINK ${TARGET_CLIENT})
endif()


########################################################################
# SERVER
########################################################################

# Sources
set_glob(ENGINE_SERVER GLOB src/engine/server
  register.cpp
  register.h
  server.cpp
  server.h
)
set_glob(GAME_SERVER GLOB_RECURSE src/game/server
  entities/character.cpp
  entities/character.h
  entities/flag.cpp
  entities/flag.h
  entities/laser.cpp
  entities/laser.h
  entities/pickup.cpp
  entities/pickup.h
  entities/projectile.cpp
  entities/projectile.h
  entity.cpp
  entity.h
  eventhandler.cpp
  eventhandler.h
  gamecontext.cpp
  gamecontext.h
  gamecontroller.cpp
  gamecontroller.h
  gamemodes/ctf.cpp
  gamemodes/ctf.h
  gamemodes/dm.cpp
  gamemodes/dm.h
  gamemodes/mod.cpp
  gamemodes/mod.h
  gamemodes/tdm.cpp
  gamemodes/tdm.h
  gameworld.cpp
  gameworld.h
  player.cpp
  player.h
)
set(GAME_GENERATED_SERVER
  "src/game/generated/server_data.cpp"
  "src/game/generated/server_data.h"
)
set(SERVER_SRC ${ENGINE_SERVER} ${GAME_SERVER} ${GAME_GENERATED_SERVER})
if(TARGET_OS STREQUAL "windows")
  set(SERVER_ICON "other/icons/${SERVER_EXECUTABLE}.rc")
else()
  set(SERVER_ICON)
endif()

# Libraries
set(LIBS_SERVER ${LIBS})

# Target
set(TARGET_SERVER ${SERVER_EXECUTABLE})
add_executable(${TARGET_SERVER}
  ${DEPS}
  ${SERVER_SRC}
  ${SERVER_ICON}
  $<TARGET_OBJECTS:engine-shared>
  $<TARGET_OBJECTS:game-shared>
)
target_link_libraries(${TARGET_SERVER} ${LIBS_SERVER})
list(APPEND TARGETS_OWN ${TARGET_SERVER})
list(APPEND TARGETS_LINK ${TARGET_SERVER})

if(TARGET_OS AND TARGET_OS STREQUAL "mac")
  set(SERVER_LAUNCHER_SRC src/osxlaunch/server.mm)
  set(TARGET_SERVER_LAUNCHER ${TARGET_SERVER}-Launcher)
  add_executable(${TARGET_SERVER_LAUNCHER} ${SERVER_LAUNCHER_SRC})
  target_link_libraries(${TARGET_SERVER_LAUNCHER} ${COCOA})
  list(APPEND TARGETS_OWN ${TARGET_SERVER_LAUNCHER})
  list(APPEND TARGETS_LINK ${TARGET_SERVER_LAUNCHER})
endif()

########################################################################
# VARIOUS TARGETS
########################################################################

set_glob(MASTERSRV_SRC GLOB src/mastersrv mastersrv.cpp mastersrv.h)
set_glob(VERSIONSRV_SRC GLOB src/versionsrv mapversions.h versionsrv.cpp versionsrv.h)

set(TARGET_MASTERSRV mastersrv)
set(TARGET_VERSIONSRV versionsrv)

add_executable(${TARGET_MASTERSRV} EXCLUDE_FROM_ALL ${MASTERSRV_SRC} $<TARGET_OBJECTS:engine-shared> ${DEPS})
add_executable(${TARGET_VERSIONSRV} EXCLUDE_FROM_ALL ${VERSIONSRV_SRC} $<TARGET_OBJECTS:engine-shared> ${DEPS})

target_link_libraries(${TARGET_MASTERSRV} ${LIBS})
target_link_libraries(${TARGET_VERSIONSRV} ${LIBS})

list(APPEND TARGETS_OWN ${TARGET_MASTERSRV} ${TARGET_VERSIONSRV})
list(APPEND TARGETS_LINK ${TARGET_MASTERSRV} ${TARGET_VERSIONSRV})

set(TARGETS_TOOLS)
set_glob(TOOLS GLOB src/tools
  crapnet.cpp
  dilate.cpp
  fake_server.cpp
  map_resave.cpp
  map_version.cpp
  packetgen.cpp
  tileset_borderadd.cpp
  tileset_borderfix.cpp
  tileset_borderrem.cpp
  tileset_borderset.cpp
)
foreach(ABS_T ${TOOLS})
  file(RELATIVE_PATH T "${PROJECT_SOURCE_DIR}/src/tools/" ${ABS_T})
  if(T MATCHES "\\.cpp$")
    string(REGEX REPLACE "\\.cpp$" "" TOOL "${T}")
    set(TOOL_DEPS ${DEPS})
    set(TOOL_LIBS ${LIBS})
    if(TOOL MATCHES "^(tileset_.*|dilate|map_extract|map_replace_image)$")
      list(APPEND TOOL_DEPS ${PNGLITE_DEP})
      list(APPEND TOOL_LIBS ${PNGLITE_LIBRARIES})
      list(APPEND TOOL_INCLUDE_DIRS ${PNGLITE_INCLUDE_DIRS})
    endif()
    set(EXCLUDE_FROM_ALL)
    add_executable(${TOOL} EXCLUDE_FROM_ALL
      ${TOOL_DEPS}
      src/tools/${TOOL}.cpp
      ${EXTRA_TOOL_SRC}
      $<TARGET_OBJECTS:engine-shared>
    )
    target_include_directories(${TOOL} PRIVATE ${TOOL_INCLUDE_DIRS})
    target_link_libraries(${TOOL} ${TOOL_LIBS})
    list(APPEND TARGETS_TOOLS ${TOOL})
  endif()
endforeach()

list(APPEND TARGETS_OWN ${TARGETS_TOOLS})
list(APPEND TARGETS_LINK ${TARGETS_TOOLS})

add_custom_target(tools DEPENDS ${TARGETS_TOOLS})
add_custom_target(everything DEPENDS ${TARGETS_OWN})

########################################################################
# INSTALLATION
########################################################################

function(escape_regex VAR STRING)
  string(REGEX REPLACE "([][^$.+*?|()\\\\])" "\\\\\\1" ESCAPED "${STRING}")
  set(${VAR} ${ESCAPED} PARENT_SCOPE)
endfunction()

function(escape_backslashes VAR STRING)
  string(REGEX REPLACE "\\\\" "\\\\\\\\" ESCAPED "${STRING}")
  set(${VAR} ${ESCAPED} PARENT_SCOPE)
endfunction()

function(max_length VAR)
  set(MAX_LENGTH 0)
  foreach(str ${ARGN})
    string(LENGTH ${str} LENGTH)
    if(LENGTH GREATER MAX_LENGTH)
      set(MAX_LENGTH ${LENGTH})
    endif()
  endforeach()
  set(${VAR} ${MAX_LENGTH} PARENT_SCOPE)
endfunction()

# Tries to generate a list of regex that matches everything except the given
# parameters.
function(regex_inverted VAR)
  max_length(MAX_LENGTH ${ARGN})
  math(EXPR UPPER_BOUND "${MAX_LENGTH}-1")

  set(REMAINING ${ARGN})
  set(RESULT)

  foreach(i RANGE ${UPPER_BOUND})
    set(TEMP ${REMAINING})
    set(REMAINING)
    foreach(str ${TEMP})
      string(LENGTH ${str} LENGTH)
      if(i LESS LENGTH)
        list(APPEND REMAINING ${str})
      endif()
    endforeach()

    set(ADDITIONAL)
    foreach(outer ${REMAINING})
      string(SUBSTRING ${outer} 0 ${i} OUTER_PREFIX)
      set(CHARS "")
      foreach(inner ${REMAINING})
        string(SUBSTRING ${inner} 0 ${i} INNER_PREFIX)
        if(OUTER_PREFIX STREQUAL INNER_PREFIX)
          string(SUBSTRING ${inner} ${i} 1 INNER_NEXT)
          set(CHARS "${CHARS}${INNER_NEXT}")
        endif()
      endforeach()
      escape_regex(OUTER_PREFIX_ESCAPED "${OUTER_PREFIX}")

      list(APPEND ADDITIONAL "${OUTER_PREFIX_ESCAPED}([^${CHARS}]|$)")
    endforeach()
    list(REMOVE_DUPLICATES ADDITIONAL)
    list(APPEND RESULT ${ADDITIONAL})
  endforeach()
  set(${VAR} ${RESULT} PARENT_SCOPE)
endfunction()

set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_GENERATOR TGZ TXZ)
set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
set(CPACK_STRIP_FILES TRUE)
set(CPACK_COMPONENTS_ALL portable)
set(CPACK_SOURCE_GENERATOR ZIP TGZ TBZ2 TXZ)
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
if(VERSION_PATCH STREQUAL "0")
  set(CPACK_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR})
else()
  set(CPACK_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})
endif()
set(CPACK_SYSTEM_NAME ${CMAKE_SYSTEM_NAME})

if(TARGET_OS AND TARGET_BITS)
  if(TARGET_OS STREQUAL "windows")
    set(CPACK_SYSTEM_NAME "win${TARGET_BITS}")
    set(CPACK_GENERATOR ZIP)
  elseif(TARGET_OS STREQUAL "linux")
    # Assuming Intel here.
    if(TARGET_BITS EQUAL 32)
      set(CPACK_SYSTEM_NAME "linux_x86")
    elseif(TARGET_BITS EQUAL 64)
      set(CPACK_SYSTEM_NAME "linux_x86_64")
    endif()
  elseif(TARGET_OS STREQUAL "mac")
    set(CPACK_SYSTEM_NAME "osx")
    set(CPACK_GENERATOR DMG)
  endif()
endif()

set(CPACK_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_SYSTEM_NAME})
set(CPACK_ARCHIVE_PORTABLE_FILE_NAME ${CPACK_PACKAGE_FILE_NAME})
set(CPACK_SOURCE_PACKAGE_FILE_NAME ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-src)
set(CPACK_SOURCE_FILES
  CMakeLists.txt
  bam.lua
  cmake/
  configure.lua
  data/
  datasrc/
  docs/
  license.txt
  objs/
  other/
  readme.txt
  scripts/
  src/
  storage.cfg
)
set(CPACK_SOURCE_IGNORE_FILES
  "\\\\.o$"
  "\\\\.pyc$"
  "/\\\\.git"
  "/__pycache__/"
)

regex_inverted(CPACK_SOURCE_FILES_INVERTED ${CPACK_SOURCE_FILES})
escape_regex(PROJECT_SOURCE_DIR_ESCAPED ${PROJECT_SOURCE_DIR})

foreach(str ${CPACK_SOURCE_FILES_INVERTED})
  escape_backslashes(STR_ESCAPED "${PROJECT_SOURCE_DIR_ESCAPED}/${str}")
  list(APPEND CPACK_SOURCE_IGNORE_FILES "${STR_ESCAPED}")
endforeach()

set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME ${PROJECT_NAME})

set(CPACK_TARGETS
  ${TARGET_CLIENT}
  ${TARGET_SERVER}
)
set(CPACK_DIRS data)
set(CPACK_FILES
  license.txt
  storage.cfg
  ${COPY_FILES}
)
if(TARGET_OS STREQUAL "windows")
  list(APPEND CPACK_FILES other/config_directory.bat)
endif()

if(NOT DEV)
  install(DIRECTORY data DESTINATION share/${PROJECT_NAME} COMPONENT data)
  install(TARGETS ${TARGET_CLIENT} DESTINATION bin COMPONENT client)
  install(TARGETS ${TARGET_SERVER} DESTINATION bin COMPONENT server)
endif()

if(DEV)
  # Don't generate CPack targets.
elseif(CMAKE_VERSION VERSION_LESS 3.6 OR CMAKE_VERSION VERSION_EQUAL 3.6)
  message(WARNING "Cannot create CPack targets, CMake version too old. Use CMake 3.6 or newer.")
else()
  set(EXTRA_ARGS DESTINATION ${CPACK_PACKAGE_FILE_NAME} COMPONENT portable EXCLUDE_FROM_ALL)
  install(TARGETS ${CPACK_TARGETS} ${EXTRA_ARGS})
  install(DIRECTORY ${CPACK_DIRS} ${EXTRA_ARGS})
  install(FILES ${CPACK_FILES} ${EXTRA_ARGS})
endif()

set(PACKAGE_TARGETS)

if(CLIENT AND (DMGTOOLS_FOUND OR HDIUTIL))
  file(MAKE_DIRECTORY bundle/client/)
  file(MAKE_DIRECTORY bundle/server/)
  configure_file(other/bundle/client/Info.plist.in bundle/client/Info.plist)
  configure_file(other/bundle/server/Info.plist.in bundle/server/Info.plist)

  if(HDIUTIL)
    set(DMG_PARAMS --hdiutil ${HDIUTIL})
  elseif(DMGTOOLS_FOUND)
    set(DMG_PARAMS --dmgtools ${DMG} ${HFSPLUS} ${NEWFS_HFS})
  endif()
  set(DMG_TMPDIR pack_${CPACK_PACKAGE_FILE_NAME}_dmg)
  set(DMG_MKDIRS
    ${TARGET_CLIENT}.app
    ${TARGET_CLIENT}.app/Contents
    ${TARGET_CLIENT}.app/Contents/Frameworks
    ${TARGET_CLIENT}.app/Contents/MacOS
    ${TARGET_CLIENT}.app/Contents/Resources
    ${TARGET_SERVER}.app
    ${TARGET_SERVER}.app/Contents
    ${TARGET_SERVER}.app/Contents/MacOS
    ${TARGET_SERVER}.app/Contents/Resources
    ${TARGET_SERVER}.app/Contents/Resources/data
    ${TARGET_SERVER}.app/Contents/Resources/data/mapres
  )
  set(DMG_MKDIR_COMMANDS)
  foreach(dir ${DMG_MKDIRS})
    list(APPEND DMG_MKDIR_COMMANDS COMMAND ${CMAKE_COMMAND} -E make_directory ${DMG_TMPDIR}/${dir})
  endforeach()
  add_custom_command(OUTPUT ${CPACK_PACKAGE_FILE_NAME}.dmg
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${DMG_TMPDIR}
    ${DMG_MKDIR_COMMANDS}

    # CLIENT
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/data ${DMG_TMPDIR}/${TARGET_CLIENT}.app/Contents/Resources/data
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/other/icons/${TARGET_CLIENT}.icns ${DMG_TMPDIR}/${TARGET_CLIENT}.app/Contents/Resources/
    COMMAND ${CMAKE_COMMAND} -E copy bundle/client/Info.plist ${PROJECT_SOURCE_DIR}/other/bundle/client/PkgInfo ${DMG_TMPDIR}/${TARGET_CLIENT}.app/Contents/
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${TARGET_CLIENT}> ${DMG_TMPDIR}/${TARGET_CLIENT}.app/Contents/MacOS/
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/other/sdl/mac/lib64/SDL.framework ${DMG_TMPDIR}/${TARGET_CLIENT}.app/Contents/Frameworks/SDL.framework
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/other/freetype/mac/lib64/libfreetype.6.dylib ${DMG_TMPDIR}/${TARGET_CLIENT}.app/Contents/Frameworks/
    COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/darwin_change_dylib.py change --tools ${CMAKE_INSTALL_NAME_TOOL} ${CMAKE_OTOOL} ${DMG_TMPDIR}/${TARGET_CLIENT}.app/Contents/MacOS/${TARGET_CLIENT} SDL @executable_path/../Frameworks/SDL.framework/SDL
    COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/darwin_change_dylib.py change --tools ${CMAKE_INSTALL_NAME_TOOL} ${CMAKE_OTOOL} ${DMG_TMPDIR}/${TARGET_CLIENT}.app/Contents/MacOS/${TARGET_CLIENT} libfreetype @executable_path/../Frameworks/libfreetype.6.dylib

    # SERVER
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/data/maps ${DMG_TMPDIR}/${TARGET_SERVER}.app/Contents/Resources/data/maps
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/other/icons/${TARGET_SERVER}.icns ${DMG_TMPDIR}/${TARGET_SERVER}.app/Contents/Resources/
    COMMAND ${CMAKE_COMMAND} -E copy bundle/server/Info.plist ${PROJECT_SOURCE_DIR}/other/bundle/server/PkgInfo ${DMG_TMPDIR}/${TARGET_SERVER}.app/Contents/
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${TARGET_SERVER}> $<TARGET_FILE:${TARGET_SERVER_LAUNCHER}> ${DMG_TMPDIR}/${TARGET_SERVER}.app/Contents/MacOS/

    # DMG
    COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/scripts/dmg.py create ${DMG_PARAMS} ${CPACK_PACKAGE_FILE_NAME}.dmg ${CPACK_PACKAGE_FILE_NAME} ${DMG_TMPDIR}

    DEPENDS
      ${TARGET_CLIENT}
      ${TARGET_SERVER_LAUNCHER}
      ${TARGET_SERVER}
      ${PROJECT_BINARY_DIR}/bundle/client/Info.plist
      ${PROJECT_BINARY_DIR}/bundle/server/Info.plist
      data
      other/bundle/client/PkgInfo
      other/bundle/server/PkgInfo
      other/icons/${TARGET_CLIENT}.icns
      other/icons/${TARGET_SERVER}.icns
      scripts/dmg.py
  )
  add_custom_target(package_dmg DEPENDS ${CPACK_PACKAGE_FILE_NAME}.dmg)
  list(APPEND PACKAGE_TARGETS package_dmg)
endif()

foreach(ext zip tar.gz tar.xz)
  set(TAR_MODE c)
  set(TAR_EXTRA_ARGS)
  string(REPLACE . _ EXT_SLUG ${ext})

  set(TMPDIR pack_${CPACK_PACKAGE_FILE_NAME}_${EXT_SLUG}/${CPACK_PACKAGE_FILE_NAME})

  set(COPY_FILE_COMMANDS)
  set(COPY_DIR_COMMANDS)
  set(COPY_TARGET_COMMANDS)
  set(STRIP_TARGET_COMMANDS)
  foreach(file ${CPACK_FILES})
    list(APPEND COPY_FILE_COMMANDS COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/${file} ${TMPDIR}/)
  endforeach()
  foreach(dir ${CPACK_DIRS})
    list(APPEND COPY_DIR_COMMANDS COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/${dir} ${TMPDIR}/${dir})
  endforeach()
  foreach(target ${CPACK_TARGETS})
    list(APPEND COPY_TARGET_COMMANDS COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${target}> ${TMPDIR}/)
  endforeach()

  if(ext STREQUAL zip)
    set(TAR_EXTRA_ARGS --format=zip)
  elseif(ext STREQUAL tar.gz)
    set(TAR_MODE cz)
  elseif(ext STREQUAL tar.xz)
    set(TAR_MODE cJ)
  endif()
  add_custom_command(OUTPUT ${CPACK_PACKAGE_FILE_NAME}.${ext}
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${TMPDIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${TMPDIR}
    ${COPY_FILE_COMMANDS}
    ${COPY_DIR_COMMANDS}
    ${COPY_TARGET_COMMANDS}
    ${STRIP_TARGET_COMMANDS}
    COMMAND ${CMAKE_COMMAND} -E chdir pack_${CPACK_PACKAGE_FILE_NAME}_${EXT_SLUG} ${CMAKE_COMMAND} -E tar ${TAR_MODE} ../${CPACK_PACKAGE_FILE_NAME}.${ext} ${TAR_EXTRA_ARGS} -- ${CPACK_PACKAGE_FILE_NAME}/
    DEPENDS ${CPACK_TARGETS}
  )
  add_custom_target(package_${EXT_SLUG} DEPENDS ${CPACK_PACKAGE_FILE_NAME}.${ext})
  list(APPEND PACKAGE_TARGETS package_${EXT_SLUG})
endforeach()

set(PACKAGE_DEFAULT tar_xz)
if(TARGET_OS STREQUAL "windows")
  set(PACKAGE_DEFAULT zip)
elseif(TARGET_OS STREQUAL "mac")
  set(PACKAGE_DEFAULT dmg)
endif()
add_custom_target(package_default DEPENDS package_${PACKAGE_DEFAULT})

add_custom_target(package_all DEPENDS ${PACKAGE_TARGETS})

# Unset these variables, they might do something in the future of CPack.
unset(CPACK_SOURCE_FILES)
unset(CPACK_SOURCE_FILES_INVERTED)
unset(CPACK_TARGETS)
unset(CPACK_DIRS)
unset(CPACK_FILES)

include(CPack)

########################################################################
# COMPILER-SPECIFICS
########################################################################

# In the future (CMake 3.8.0+), use source_group(TREE ...)
macro(source_group_tree dir)
  file(GLOB ents RELATIVE ${PROJECT_SOURCE_DIR}/${dir} ${PROJECT_SOURCE_DIR}/${dir}/*)
  foreach(ent ${ents})
    if(IS_DIRECTORY ${PROJECT_SOURCE_DIR}/${dir}/${ent})
      source_group_tree(${dir}/${ent})
    else()
      string(REPLACE "/" "\\" group ${dir})
      source_group(${group} FILES ${PROJECT_SOURCE_DIR}/${dir}/${ent})
    endif()
  endforeach()
endmacro()
source_group_tree(src)

set(TARGETS ${TARGETS_OWN} ${TARGETS_DEP})

foreach(target ${TARGETS})
  if(MSVC)
    target_compile_options(${target} PRIVATE $<$<NOT:${DBG}>:/MT> $<${DBG}:/MTd>) # Use static CRT
    target_compile_options(${target} PRIVATE /MP) # Use multiple cores
    target_compile_options(${target} PRIVATE /EHsc) # Only catch C++ exceptions with catch.
    target_compile_options(${target} PRIVATE /GS) # Protect the stack pointer.
    target_compile_options(${target} PRIVATE /wd4996) # Use of non-_s functions.
  endif()
  if(OUR_FLAGS)
    target_compile_options(${target} PRIVATE ${OUR_FLAGS})
  endif()
  if(DEFINE_FORTIFY_SOURCE)
    target_compile_definitions(${target} PRIVATE $<$<NOT:$<CONFIG:Debug>>:_FORTIFY_SOURCE=2>) # Detect some buffer overflows.
  endif()
endforeach()

foreach(target ${TARGETS_LINK})
  if(MSVC)
    set_property(TARGET ${target} APPEND PROPERTY LINK_FLAGS /SAFESEH:NO) # Disable SafeSEH because the shipped libraries don't support it (would cause error LNK2026 otherwise).
  endif()
  if(TARGET_OS STREQUAL "mac")
    target_link_libraries(${target} -stdlib=libc++)
    target_link_libraries(${target} -mmacosx-version-min=10.7)
  endif()
  if((MINGW OR TARGET_OS STREQUAL "linux") AND PREFER_BUNDLED_LIBS)
    # Statically link the standard libraries with on MinGW/Linux so we don't
    # have to ship them as DLLs.
    target_link_libraries(${target} -static-libgcc)
    target_link_libraries(${target} -static-libstdc++)
    if(MINGW)
      # Link pthread library statically instead of dynamically.
      # Solution from https://stackoverflow.com/a/28001261.
      target_link_libraries(${target} -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic)
    endif()
  endif()
endforeach()

foreach(target ${TARGETS_OWN})
  if(MSVC)
    target_compile_options(${target} PRIVATE /wd4244) # Possible loss of data (float -> int, int -> float, etc.).
    target_compile_options(${target} PRIVATE /wd4267) # Possible loss of data (size_t - int on win64).
    target_compile_options(${target} PRIVATE /wd4800) # Implicit conversion of int to bool.
  endif()
  if(OUR_FLAGS_OWN)
    target_compile_options(${target} PRIVATE ${OUR_FLAGS_OWN})
  endif()
  target_include_directories(${target} PRIVATE ${PROJECT_BINARY_DIR}/src)
  target_include_directories(${target} PRIVATE src)
  target_compile_definitions(${target} PRIVATE $<$<CONFIG:Debug>:CONF_DEBUG>)
  target_include_directories(${target} PRIVATE ${CURL_INCLUDE_DIRS})
  target_include_directories(${target} PRIVATE ${ZLIB_INCLUDE_DIRS})
endforeach()

foreach(target ${TARGETS_DEP})
  if(MSVC)
    target_compile_options(${target} PRIVATE /W0)
  endif()
  if(OUR_FLAGS_DEP)
    target_compile_options(${target} PRIVATE ${OUR_FLAGS_DEP})
  endif()
endforeach()
