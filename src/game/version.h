/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_VERSION_H
#define GAME_VERSION_H
#include <generated/nethash.cpp>
#define GAME_VERSION "0.7.2"
// INFCROYA BEGIN ------------------------------------------------------------
// #define GAME_NETVERSION "0.7 " GAME_NETVERSION_HASH
#define GAME_NETVERSION "0.7 802f1be60a05665f"; // modified gamecore.h/gamecore.cpp changed the hash (looked at Assa's Catch64 commits before)
// INFCROYA END ------------------------------------------------------------//
#define CLIENT_VERSION 0x0703
static const char GAME_RELEASE_VERSION[8] = "0.7.2";
#endif
