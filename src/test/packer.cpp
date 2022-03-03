#include "test.h"
#include <gtest/gtest.h>

#include <base/system.h>
#include <engine/message.h>

// pExpected is NULL if an error is expected
static void ExpectAddString5(const char *pString, int Limit, const char *pExpected)
{
	static char ZEROS[CPacker::PACKER_BUFFER_SIZE] = {0};
	static const int OFFSET = CPacker::PACKER_BUFFER_SIZE - 5;
	CPacker Packer;
	Packer.Reset();
	Packer.AddRaw(ZEROS, OFFSET);
	Packer.AddString(pString, Limit);

	EXPECT_EQ(pExpected == 0, Packer.Error());
	if(pExpected)
	{
		// Include null termination.
		int ExpectedLength = str_length(pExpected) + 1;
		EXPECT_EQ(ExpectedLength, Packer.Size() - OFFSET);
		if(ExpectedLength == Packer.Size() - OFFSET)
		{
			EXPECT_STREQ(pExpected, (const char *)Packer.Data() + OFFSET);
		}
	}
}

TEST(Packer, AddString)
{
	ExpectAddString5("", 0, "");
	ExpectAddString5("a", 0, "a");
	ExpectAddString5("abcd", 0, "abcd");
	ExpectAddString5("abcde", 0, 0);
}

TEST(Packer, AddStringLimit)
{
	ExpectAddString5("", 1, "");
	ExpectAddString5("a", 1, "a");
	ExpectAddString5("aa", 1, "a");
	ExpectAddString5("ä", 1, "");

	ExpectAddString5("", 10, "");
	ExpectAddString5("a", 10, "a");
	ExpectAddString5("abcd", 10, "abcd");
	ExpectAddString5("abcde", 10, 0);

	ExpectAddString5("äöü", 5, "äö");
	ExpectAddString5("äöü", 6, 0);
}

TEST(Packer, AddStringBroken)
{
	ExpectAddString5("\x80", 0, "�");
	ExpectAddString5("\x80\x80", 0, 0);
	ExpectAddString5("a\x80", 0, "a�");
	ExpectAddString5("\x80""a", 0, "�a");
	ExpectAddString5("\x80", 1, "");
	ExpectAddString5("\x80\x80", 3, "�");
	ExpectAddString5("\x80\x80", 5, "�");
	ExpectAddString5("\x80\x80", 6, 0);
}

TEST(Packer, EmptyPacker)
{
	CPacker Packer;
	Packer.Reset();
	EXPECT_EQ(false, Packer.Error());
	EXPECT_EQ(0, Packer.Size());
	EXPECT_EQ(int(CPacker::PACKER_BUFFER_SIZE), Packer.RemainingSize());
}

TEST(Packer, PackerRawNoSize)
{
	CPacker Packer;
	Packer.Reset();
	EXPECT_EQ(false, Packer.Error());
	Packer.AddRaw("a", 0);
	EXPECT_EQ(true, Packer.Error());
}

TEST(Packer, MinimalUnpacker)
{
	unsigned char aBuf[1] = {0};
	CUnpacker Unpacker;
	Unpacker.Reset(aBuf, sizeof(aBuf));
	EXPECT_EQ(1, Unpacker.RemainingSize());
	EXPECT_EQ(1, Unpacker.CompleteSize());
	EXPECT_EQ(0, Unpacker.Size());
	EXPECT_EQ(0, Unpacker.GetInt());
	EXPECT_EQ(false, Unpacker.Error());
	EXPECT_EQ(0, Unpacker.RemainingSize());
	EXPECT_EQ(1, Unpacker.CompleteSize());
	EXPECT_EQ(1, Unpacker.Size());
	EXPECT_EQ(123, Unpacker.GetIntOrDefault(123));
	EXPECT_EQ(false, Unpacker.Error());
	EXPECT_EQ(0, Unpacker.GetInt());
	EXPECT_EQ(true, Unpacker.Error());
}

TEST(Packer, UnpackerRawNoSize)
{
	unsigned char aBuf[1] = {0};
	CUnpacker Unpacker;
	Unpacker.Reset(aBuf, sizeof(aBuf));
	EXPECT_EQ(false, Unpacker.Error());
	EXPECT_EQ((const unsigned char *)0x0, Unpacker.GetRaw(0));
	EXPECT_EQ(true, Unpacker.Error());
}

TEST(Packer, RoundtripPackerUnpacker)
{
	CPacker Packer;
	Packer.Reset();
	Packer.AddInt(12345);
	Packer.AddString("abcd");
	Packer.AddInt(-123);
	Packer.AddRaw("efgh", 4);
	EXPECT_EQ(false, Packer.Error());
	EXPECT_EQ(14, Packer.Size());

	CUnpacker Unpacker;
	Unpacker.Reset(Packer.Data(), Packer.Size());
	EXPECT_EQ(12345, Unpacker.GetInt());
	EXPECT_STREQ("abcd", Unpacker.GetString());
	EXPECT_EQ(-123, Unpacker.GetInt());
	EXPECT_EQ(0, mem_comp("efgh", Unpacker.GetRaw(4), 4));
	EXPECT_EQ(false, Unpacker.Error());
	EXPECT_EQ(0, Unpacker.RemainingSize());
}

static void TestMsgPackerUnpacker(int Type, bool System, bool ExpectError)
{
	CMsgPacker Packer(Type, System);
	EXPECT_EQ(ExpectError, Packer.Error());
	if(Packer.Error())
		return;

	CMsgUnpacker Unpacker(Packer.Data(), Packer.Size());
	EXPECT_EQ(Type, Unpacker.Type());
	EXPECT_EQ(System, Unpacker.System());
	EXPECT_EQ(false, Unpacker.Error());
	EXPECT_EQ(0, Unpacker.RemainingSize());
}

TEST(Packer, RoundtripMsgPackerUnpacker)
{
	TestMsgPackerUnpacker(0, false, false);
	TestMsgPackerUnpacker(12345, true, false);
	TestMsgPackerUnpacker(0x3FFFFFFF, true, false);
}

TEST(Packer, MsgPackerError)
{
	TestMsgPackerUnpacker(-1, false, true);
	TestMsgPackerUnpacker(-12345, true, true);
	TestMsgPackerUnpacker(0x7FFFFFFF, true, true);
}

TEST(Packer, MsgUnpackerError)
{
	CPacker Packer;
	Packer.Reset();
	Packer.AddInt(-12345);
	EXPECT_EQ(false, Packer.Error());

	CMsgUnpacker Unpacker(Packer.Data(), Packer.Size());
	EXPECT_EQ(0, Unpacker.Type());
	EXPECT_EQ(false, Unpacker.System());
	EXPECT_EQ(true, Unpacker.Error());
}
