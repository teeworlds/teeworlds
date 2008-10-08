#include <string.h>
#include <engine/e_client_interface.h>
#include <game/version.hpp>

#include "gameclient.hpp"
#include "components/console.hpp"




// clean hooks
extern "C" void modc_entergame() {}
extern "C" void modc_shutdown() {}
extern "C" void modc_console_init() { gameclient.on_console_init(); }
extern "C" void modc_save_config() { gameclient.on_save(); }
extern "C" void modc_init() { gameclient.on_init(); }
extern "C" void modc_connected() { gameclient.on_connected(); }
extern "C" void modc_predict() { gameclient.on_predict(); }
extern "C" void modc_newsnapshot() { gameclient.on_snapshot(); }
extern "C" int modc_snap_input(int *data) { return gameclient.on_snapinput(data); }
extern "C" void modc_statechange(int state, int old) { gameclient.on_statechange(state, old); }
extern "C" void modc_render() { gameclient.on_render(); }
extern "C" void modc_message(int msgtype) { gameclient.on_message(msgtype); }
extern "C" void modc_rcon_line(const char *line) { gameclient.console->print_line(1, line); }

extern "C" const char *modc_net_version() { return GAME_NETVERSION; }
extern "C" const char *modc_getitemname(int type) { return netobj_get_name(type); }

