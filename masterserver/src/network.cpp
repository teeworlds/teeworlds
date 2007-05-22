#include <cstring>
#include "common.h"
#include "network.h"

char *WriteInt32(char *buffer, int32 value)
{
	buffer[0] = value >> 24;
	buffer[1] = value >> 16;
	buffer[2] = value >> 8;
	buffer[3] = value;

	return buffer + sizeof(int32);
}

char *WriteFixedString(char *buffer, const char *string, int strlen)
{
	memcpy(buffer, string, strlen);

	return buffer + strlen;
}



char *ReadInt32(char *buffer, int32 *value)
{
	*value = buffer[0] << 24;
	*value |= buffer[1] << 16;
	*value |= buffer[2] << 8;
	*value |= buffer[3];

	return buffer + sizeof(int32);
}

char *ReadFixedString(char *buffer, char *string, int strlen)
{
	memcpy(string, buffer, strlen);

	return buffer + strlen;
}
