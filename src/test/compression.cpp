/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <gtest/gtest.h>

#include <engine/shared/compression.h>

static const int DATA[] = {0, 1, -1, 32, 64, 256, -512, 12345, -123456, 1234567, 12345678, 123456789, 2147483647, (-2147483647 - 1)};
static const int NUM = sizeof(DATA) / sizeof(int);
static const int SIZES[NUM] = {1, 1, 1, 1, 2, 2, 2, 3, 3, 4, 4, 4, 5, 5};

TEST(CVariableInt, RoundtripPackUnpack)
{
	for(int i = 0; i < NUM; i++)
	{
		unsigned char aPacked[CVariableInt::MAX_BYTES_PACKED];
		int Result;
		EXPECT_EQ(int(CVariableInt::Pack(aPacked, DATA[i], sizeof(aPacked)) - aPacked), SIZES[i]);
		EXPECT_EQ(int(CVariableInt::Unpack(aPacked, &Result, sizeof(aPacked)) - aPacked), SIZES[i]);
		EXPECT_EQ(Result, DATA[i]);
	}
}

TEST(CVariableInt, UnpackInvalid)
{
	unsigned char aPacked[CVariableInt::MAX_BYTES_PACKED];
	for(unsigned i = 0; i < sizeof(aPacked); i++)
		aPacked[i] = 0xFF;

	int Result;
	EXPECT_EQ(int(CVariableInt::Unpack(aPacked, &Result, sizeof(aPacked)) - aPacked), int(CVariableInt::MAX_BYTES_PACKED));
	EXPECT_EQ(Result, (-2147483647 - 1));

	aPacked[0] &= ~0x40; // unset sign bit

	EXPECT_EQ(int(CVariableInt::Unpack(aPacked, &Result, sizeof(aPacked)) - aPacked), int(CVariableInt::MAX_BYTES_PACKED));
	EXPECT_EQ(Result, 2147483647);
}

TEST(CVariableInt, PackBufferTooSmall)
{
	unsigned char aPacked[CVariableInt::MAX_BYTES_PACKED / 2]; // too small
	EXPECT_EQ(CVariableInt::Pack(aPacked, 2147483647, sizeof(aPacked)), (const unsigned char *)0x0);
}

TEST(CVariableInt, UnpackBufferTooSmall)
{
	unsigned char aPacked[CVariableInt::MAX_BYTES_PACKED / 2];
	for(unsigned i = 0; i < sizeof(aPacked); i++)
		aPacked[i] = 0xFF; // extended bits are set, but buffer ends too early

	int UnusedResult;
	EXPECT_EQ(CVariableInt::Unpack(aPacked, &UnusedResult, sizeof(aPacked)), (const unsigned char *)0x0);
}

TEST(CVariableInt, RoundtripCompressDecompress)
{
	unsigned char aCompressed[NUM * CVariableInt::MAX_BYTES_PACKED];
	int aDecompressed[NUM];
	long ExpectedCompressedSize = 0;
	for(int i = 0; i < NUM; i++)
		ExpectedCompressedSize += SIZES[i];

	long CompressedSize = CVariableInt::Compress(DATA, sizeof(DATA), aCompressed, sizeof(aCompressed));
	ASSERT_EQ(CompressedSize, ExpectedCompressedSize);
	long DecompressedSize = CVariableInt::Decompress(aCompressed, ExpectedCompressedSize, aDecompressed, sizeof(aDecompressed));
	ASSERT_EQ(DecompressedSize, sizeof(DATA));
	for(int i = 0; i < NUM; i++)
	{
		EXPECT_EQ(aDecompressed[i], DATA[i]);
	}
}

TEST(CVariableInt, CompressBufferTooSmall)
{
	unsigned char aCompressed[NUM]; // too small
	long CompressedSize = CVariableInt::Compress(DATA, sizeof(DATA), aCompressed, sizeof(aCompressed));
	ASSERT_EQ(CompressedSize, -1);
}

TEST(CVariableInt, DecompressBufferTooSmall)
{
	unsigned char aCompressed[] = {0x00, 0x01, 0x40, 0x20, 0x80, 0x01, 0x80, 0x04, 0xFF, 0x07, 0xB9, 0xC0, 0x01};
	int aUncompressed[4]; // too small
	long CompressedSize = CVariableInt::Decompress(aCompressed, sizeof(aCompressed), aUncompressed, sizeof(aUncompressed));
	ASSERT_EQ(CompressedSize, -1);
}
