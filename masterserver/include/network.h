#ifndef _NETWORK_H
#define _NETWORK_H

#include <cstring>
#include "common.h"

char *WriteInt32(char *buffer, int32 value);
char *WriteFixedString(char *buffer, const char *string, int strlen);

char *ReadInt32(char *buffer, int32 *value);
char *ReadFixedString(char *buffer, char *string, int strlen);

#endif
