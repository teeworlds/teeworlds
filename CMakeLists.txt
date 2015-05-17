cmake_minimum_required(VERSION 3.2)

# Project name
project(teeworld_compilation)

# Include pkg_search_module
include(FindPkgConfig)
# Find sdl2 with pkg
pkg_search_module(SDL2 REQUIRED sdl2)
# Find package
find_package(Freetype REQUIRED)

# Add warrings flag
add_compile_options(-Wall -fno-exceptions -c -g -DCONF_DEBUG)

# Include sdl2 header
include_directories(${SDL2_INCLUDE_DIRS})

# Add some flage for the include
add_definitions(`freetype-config --cflags`)
add_definitions(-I src -I src/engine/external/zlib -I other/freetype/include -I other/sdl/include)

# Difference between 32 and 64 bits
set(BITS -m32)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(BITS -m64)
endif()

# Generate all files with pythons scripts
execute_process(COMMAND mkdir -p ./src/generated)
execute_process(COMMAND python datasrc/compile.py network_source OUTPUT_FILE protocol.cpp)
execute_process(COMMAND python datasrc/compile.py network_header OUTPUT_FILE protocol.h)
execute_process(COMMAND python scripts/cmd5.py src/engine/shared/protocol.h src/game/tuning.h src/game/gamecore.cpp protocol.h OUTPUT_FILE nethash.cpp)
execute_process(COMMAND python datasrc/compile.py server_content_source OUTPUT_FILE server_data.cpp)
execute_process(COMMAND python datasrc/compile.py server_content_header OUTPUT_FILE server_data.h)
execute_process(COMMAND python datasrc/compile.py client_content_source OUTPUT_FILE client_data.cpp)
execute_process(COMMAND python datasrc/compile.py client_content_header OUTPUT_FILE client_data.h)

# Move all generated files
execute_process(COMMAND mv protocol.cpp ./src/generated)
execute_process(COMMAND mv protocol.h ./src/generated)
execute_process(COMMAND mv nethash.cpp ./src/generated)
execute_process(COMMAND mv server_data.cpp ./src/generated)
execute_process(COMMAND mv server_data.h ./src/generated)
execute_process(COMMAND mv client_data.cpp ./src/generated)
execute_process(COMMAND mv client_data.h ./src/generated)

### TODO ###
# Move all others files like sounds, sprites ect.

# Client exectuable
add_executable(
	teeworlds
	src/engine/external/zlib/uncompr.c
	src/engine/external/zlib/deflate.c
	src/engine/external/zlib/zutil.c
	src/engine/external/zlib/crc32.c
	src/engine/external/zlib/adler32.c
	src/engine/external/zlib/inffast.c
	src/engine/external/zlib/compress.c
	src/engine/external/zlib/infback.c
	src/engine/external/zlib/trees.c
	src/engine/external/zlib/inflate.c
	src/engine/external/zlib/inftrees.c
	src/engine/external/wavpack/float.c
	src/engine/external/wavpack/words.c
	src/engine/external/wavpack/unpack.c
	src/engine/external/wavpack/bits.c
	src/engine/external/wavpack/metadata.c
	src/engine/external/wavpack/wputils.c
	src/engine/external/pnglite/pnglite.c
	src/engine/external/json-parser/json.c
	src/engine/client/text.cpp
	src/engine/client/serverbrowser.cpp
	src/engine/client/friends.cpp
	src/engine/client/sound.cpp
	src/engine/client/graphics_threaded.cpp
	src/engine/client/client.cpp
	src/engine/client/input.cpp
	src/engine/client/backend_sdl.cpp
	src/game/client/gameclient.cpp
	src/game/client/render.cpp
	src/game/client/ui.cpp
	src/game/client/animstate.cpp
	src/game/client/localization.cpp
	src/game/client/components/camera.cpp
	src/game/client/components/console.cpp
	src/engine/shared/console.cpp
	src/game/client/components/broadcast.cpp
	src/game/client/components/items.cpp
	src/game/client/components/motd.cpp
	src/game/client/components/maplayers.cpp
	src/game/client/components/hud.cpp
	src/game/client/components/binds.cpp
	src/game/client/components/menus_popups.cpp
	src/game/client/components/mapimages.cpp
	src/game/client/components/nameplates.cpp
	src/game/client/components/particles.cpp
	src/game/client/components/effects.cpp
	src/game/client/components/debughud.cpp
	src/game/client/components/menus_browser.cpp
	src/game/client/components/chat.cpp
	src/game/client/components/menus_settings.cpp
	src/game/client/components/skins.cpp
	src/game/client/components/menus.cpp
	src/game/client/components/emoticon.cpp
	src/game/client/components/scoreboard.cpp
	src/game/client/components/menus_demo.cpp
	src/game/client/components/flow.cpp
	src/game/client/components/players.cpp
	src/game/client/components/menus_callback.cpp
	src/game/client/components/menus_start.cpp
	src/game/client/components/damageind.cpp
	src/game/client/components/menus_ingame.cpp
	src/game/client/components/controls.cpp
	src/game/client/components/voting.cpp
	src/game/client/components/killmessages.cpp
	src/game/client/components/spectator.cpp
	src/game/client/components/countryflags.cpp
	src/game/client/components/sounds.cpp
	src/game/client/render_map.cpp
	src/game/client/lineinput.cpp
	src/generated/client_data.cpp
	src/game/editor/layer_tiles.cpp
	src/game/editor/popups.cpp
	src/game/editor/io.cpp
	src/game/editor/editor.cpp
	src/game/editor/layer_quads.cpp
	src/game/editor/layer_game.cpp
	src/game/editor/auto_map.cpp
	src/engine/shared/netban.cpp
	src/engine/shared/network.cpp
	src/engine/shared/datafile.cpp
	src/engine/shared/filecollection.cpp
	src/engine/shared/network_client.cpp
	src/engine/shared/jobs.cpp
	src/engine/shared/kernel.cpp
	src/engine/shared/network_conn.cpp
	src/engine/shared/network_console.cpp
	src/engine/shared/snapshot.cpp
	src/engine/shared/econ.cpp
	src/engine/shared/packer.cpp
	src/engine/shared/compression.cpp
	src/engine/shared/config.cpp
	src/engine/shared/network_server.cpp
	src/engine/shared/mapchecker.cpp
	src/engine/shared/huffman.cpp
	src/engine/shared/linereader.cpp
	src/engine/shared/memheap.cpp
	src/engine/shared/engine.cpp
	src/engine/shared/masterserver.cpp
	src/engine/shared/map.cpp
	src/engine/shared/ringbuffer.cpp
	src/engine/shared/network_console_conn.cpp
	src/engine/shared/demo.cpp
	src/engine/shared/storage.cpp
	src/base/system.c
	src/game/layers.cpp
	src/game/collision.cpp
	src/game/gamecore.cpp
	src/generated/protocol.cpp
	src/generated/nethash.cpp
) # Link libraries
target_link_libraries(teeworlds -lpthread -lX11 -lGL -lGLU ${BITS} ${FREETYPE_LIBRARIES} ${SDL2_LIBRARIES})

# Server exectuable
add_executable(
	teeworlds_srv
	src/engine/external/zlib/uncompr.c
	src/engine/external/zlib/deflate.c
	src/engine/external/zlib/zutil.c
	src/engine/external/zlib/crc32.c
	src/engine/external/zlib/adler32.c
	src/engine/external/zlib/inffast.c
	src/engine/external/zlib/compress.c
	src/engine/external/zlib/infback.c
	src/engine/external/zlib/trees.c
	src/engine/external/zlib/inflate.c
	src/engine/external/zlib/inftrees.c
	src/engine/server/register.cpp
	src/engine/server/server.cpp
	src/game/server/eventhandler.cpp
	src/game/server/player.cpp
	src/game/server/gamecontroller.cpp
	src/game/server/gamecontext.cpp
	src/game/server/entities/pickup.cpp
	src/game/server/entities/projectile.cpp
	src/game/server/entities/character.cpp
	src/game/server/entities/flag.cpp
	src/game/server/entities/laser.cpp
	src/game/server/gameworld.cpp
	src/game/server/gamemodes/ctf.cpp
	src/game/server/gamemodes/tdm.cpp
	src/game/server/gamemodes/mod.cpp
	src/game/server/gamemodes/dm.cpp
	src/game/server/gamemodes/sur.cpp
	src/game/server/gamemodes/lms.cpp
	src/game/server/entity.cpp
	src/generated/server_data.cpp
	src/engine/shared/netban.cpp
	src/engine/shared/network.cpp
	src/engine/shared/datafile.cpp
	src/engine/shared/filecollection.cpp
	src/engine/shared/console.cpp
	src/engine/shared/network_client.cpp
	src/engine/shared/jobs.cpp
	src/engine/shared/kernel.cpp
	src/engine/shared/network_conn.cpp
	src/engine/shared/network_console.cpp
	src/engine/shared/snapshot.cpp
	src/engine/shared/econ.cpp
	src/engine/shared/packer.cpp
	src/engine/shared/compression.cpp
	src/engine/shared/config.cpp
	src/engine/shared/network_server.cpp
	src/engine/shared/mapchecker.cpp
	src/engine/shared/huffman.cpp
	src/engine/shared/linereader.cpp
	src/engine/shared/memheap.cpp
	src/engine/shared/engine.cpp
	src/engine/shared/masterserver.cpp
	src/engine/shared/map.cpp
	src/engine/shared/ringbuffer.cpp
	src/engine/shared/network_console_conn.cpp
	src/engine/shared/demo.cpp
	src/engine/shared/storage.cpp
	src/base/system.c
	src/game/layers.cpp
	src/game/collision.cpp
	src/game/gamecore.cpp
	src/generated/protocol.cpp
	src/generated/nethash.cpp
)
target_link_libraries(teeworlds_srv -lpthread ${BITS})
