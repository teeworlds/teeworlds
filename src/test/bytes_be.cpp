/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "test.h"

#include <gtest/gtest.h>

#include <base/system.h>

static const int INT_DATA[] = {0, 1, -1, 32, 64, 256, -512, 12345, -123456, 1234567, 12345678, 123456789, 2147483647, (-2147483647 - 1)};
static const int INT_NUM = sizeof(INT_DATA) / sizeof(int);

static const unsigned UINT_DATA[] = {0u, 1u, 2u, 32u, 64u, 256u, 512u, 12345u, 123456u, 1234567u, 12345678u, 123456789u, 2147483647u, 2147483648u, 4294967295u};
static const int UINT_NUM = sizeof(INT_DATA) / sizeof(unsigned);

TEST(BytePacking, RoundtripInt)
{
	for(int i = 0; i < INT_NUM; i++)
	{
		unsigned char aPacked[4];
		int_to_bytes_be(aPacked, INT_DATA[i]);
		EXPECT_EQ(bytes_be_to_int(aPacked), INT_DATA[i]);
	}
}

TEST(BytePacking, RoundtripUnsigned)
{
	for(int i = 0; i < UINT_NUM; i++)
	{
		unsigned char aPacked[4];
		uint_to_bytes_be(aPacked, UINT_DATA[i]);
		EXPECT_EQ(bytes_be_to_uint(aPacked), UINT_DATA[i]);
	}
}
