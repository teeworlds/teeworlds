#include <string.h>
#include <engine/e_client_interface.h>
#include <game/version.hpp>

#include "gameclient.hpp"
#include "components/console.hpp"



// clean hooks
void modc_entergame() {}
void modc_shutdown() {}
void modc_console_init() { gameclient.on_console_init(); }
void modc_save_config() { gameclient.on_save(); }
void modc_init() { gameclient.on_init(); }
void modc_connected() { gameclient.on_connected(); }
void modc_predict() { gameclient.on_predict(); }
void modc_newsnapshot() { gameclient.on_snapshot(); }
int modc_snap_input(int *data) { return gameclient.on_snapinput(data); }
void modc_statechange(int state, int old) { gameclient.on_statechange(state, old); }
void modc_render() { gameclient.on_render(); }
void modc_message(int msgtype) { gameclient.on_message(msgtype); }
void modc_rcon_line(const char *line) { gameclient.console->print_line(1, line); }

const char *modc_net_version() { return GAME_NETVERSION; }
const char *modc_getitemname(int type) { return netobj_get_name(type); }

