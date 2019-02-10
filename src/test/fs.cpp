#include "test.h"
#include <gtest/gtest.h>

#include <base/system.h>

TEST(Filesystem, CreateCloseDelete)
{
	CTestInfo Info;

	IOHANDLE File = io_open(Info.m_aFilename, IOFLAG_WRITE);
	ASSERT_TRUE(File);
	EXPECT_FALSE(io_close(File));
	EXPECT_FALSE(fs_remove(Info.m_aFilename));
}
