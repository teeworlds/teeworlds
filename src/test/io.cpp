/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "test.h"
#include <gtest/gtest.h>

#include <base/system.h>

void TestFileRead(const char *pWritten, bool SkipBom, const char *pRead)
{
	CTestInfo Info;
	char aBuf[512] = {0};
	IOHANDLE File = io_open(Info.m_aFilename, IOFLAG_WRITE);
	ASSERT_TRUE(File);
	EXPECT_EQ(io_write(File, pWritten, str_length(pWritten)), str_length(pWritten));
	EXPECT_FALSE(io_close(File));
	File = io_open(Info.m_aFilename, IOFLAG_READ | (SkipBom ? IOFLAG_SKIP_BOM : 0));
	ASSERT_TRUE(File);
	EXPECT_EQ(io_read(File, aBuf, sizeof(aBuf)), str_length(pRead));
	EXPECT_TRUE(mem_comp(aBuf, pRead, str_length(pRead)) == 0);
	EXPECT_FALSE(io_close(File));

	fs_remove(Info.m_aFilename);
}

TEST(Io, Read1)
{
	TestFileRead("", false, "");
}
TEST(Io, Read2)
{
	TestFileRead("abc", false, "abc");
}
TEST(Io, Read3)
{
	TestFileRead("\xef\xbb\xbf", false, "\xef\xbb\xbf");
}
TEST(Io, Read4)
{
	TestFileRead("\xef\xbb\xbfxyz", false, "\xef\xbb\xbfxyz");
}

TEST(Io, ReadBom1)
{
	TestFileRead("", true, "");
}
TEST(Io, ReadBom2)
{
	TestFileRead("abc", true, "abc");
}
TEST(Io, ReadBom3)
{
	TestFileRead("\xef\xbb\xbf", true, "");
}
TEST(Io, ReadBom4)
{
	TestFileRead("\xef\xbb\xbfxyz", true, "xyz");
}
TEST(Io, ReadBom5)
{
	TestFileRead("\xef\xbb\xbf\xef\xbb\xbf", true, "\xef\xbb\xbf");
}
TEST(Io, ReadBom6)
{
	TestFileRead("\xef\xbb\xbfxyz\xef\xbb\xbf", true, "xyz\xef\xbb\xbf");
}
