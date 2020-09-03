#include "test.h"
#include <gtest/gtest.h>

#include <base/system.h>
#include <engine/shared/packer.h>

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
