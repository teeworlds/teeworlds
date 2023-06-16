/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <gtest/gtest.h>

#include <base/base64.h>
#include <base/system.h>

static const char *TESTDATA[] = {"", "a", "ab", "abc", "abcd", "abcde", "Heizölrückstoßabdämpfung", "поплавать", "愛愛"};
static const char *TESTDATA_ENCODED[] = {"", "YQ==", "YWI=", "YWJj", "YWJjZA==", "YWJjZGU=", "SGVpesO2bHLDvGNrc3Rvw59hYmTDpG1wZnVuZw==", "0L/QvtC/0LvQsNCy0LDRgtGM", "5oSb5oSb"};
static const int NUM_TESTDATA = sizeof(TESTDATA) / sizeof(TESTDATA[0]);

static const char *TESTDATA_SPECIAL_ENCODED[] = {"YäQö==", "=====", "YW====Jj", "Yw", "Y+", "Y++", "Y+++", "Y  Q==", "Y\tQ==", "\nY\nW\n\n\nI\n=\n", "YWJ\r\njZGU="};
static const char *TESTDATA_SPECIAL[] = {"a", "", "abc", "c", "c", "c\xEF", "c\xEF\xBE", "a", "a", "ab", "abcde"};
static const int NUM_TESTDATA_SPECIAL = sizeof(TESTDATA_SPECIAL) / sizeof(TESTDATA_SPECIAL[0]);

TEST(Base64, Encode)
{
	for(int i = 0; i < NUM_TESTDATA; i++)
	{
		char *pEncoded = base64_encode(reinterpret_cast<const unsigned char *>(TESTDATA[i]), str_length(TESTDATA[i]));
		ASSERT_TRUE(pEncoded);
		EXPECT_STREQ(TESTDATA_ENCODED[i], pEncoded);
		mem_free(pEncoded);
	}
}

TEST(Base64, Decode)
{
	for(int i = 0; i < NUM_TESTDATA; i++)
	{
		unsigned char *pData;
		EXPECT_EQ(str_length(TESTDATA[i]), base64_decode(&pData, TESTDATA_ENCODED[i]));
		EXPECT_STREQ(TESTDATA[i], reinterpret_cast<const char *>(pData));
		mem_free(pData);
	}
}

TEST(Base64, DecodeSpecial)
{
	for(int i = 0; i < NUM_TESTDATA_SPECIAL; i++)
	{
		unsigned char *pData;
		EXPECT_EQ(str_length(TESTDATA_SPECIAL[i]), base64_decode(&pData, TESTDATA_SPECIAL_ENCODED[i]));
		ASSERT_TRUE(pData);
		EXPECT_STREQ(TESTDATA_SPECIAL[i], reinterpret_cast<const char *>(pData));
		mem_free(pData);
	}
}
