#include "test.h"

#include <gtest/gtest.h>

#include <engine/storage.h>

TEST(Storage, FindFile)
{
	CTestInfo Info;
	char aFilenameWithDot[128];
	str_format(aFilenameWithDot, sizeof(aFilenameWithDot), "./%s", Info.m_aFilename);

	IStorage *pStorage = CreateTestStorage();
	IOHANDLE File = pStorage->OpenFile(Info.m_aFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	ASSERT_TRUE(File);
	EXPECT_EQ(io_write(File, "test\n", 5), 5);
	EXPECT_FALSE(io_close(File));

	char aFound[128];

	EXPECT_TRUE(pStorage->FindFile(Info.m_aFilename, ".", IStorage::TYPE_ALL, aFound, sizeof(aFound)));
	EXPECT_STREQ(aFound, aFilenameWithDot);

	EXPECT_TRUE(pStorage->FindFile(Info.m_aFilename, ".", IStorage::TYPE_ALL, aFound, sizeof(aFound), 0x3bb935c6, 5));
	EXPECT_STREQ(aFound, aFilenameWithDot);

	EXPECT_FALSE(pStorage->FindFile(Info.m_aFilename, ".", IStorage::TYPE_ALL, aFound, sizeof(aFound), 0, 0));
	EXPECT_FALSE(pStorage->FindFile(Info.m_aFilename, ".", IStorage::TYPE_ALL, aFound, sizeof(aFound), 0x3bb935c6, 0));
	EXPECT_FALSE(pStorage->FindFile(Info.m_aFilename, ".", IStorage::TYPE_ALL, aFound, sizeof(aFound), 0, 5));
	EXPECT_FALSE(pStorage->FindFile(Info.m_aFilename, ".", IStorage::TYPE_ALL, aFound, sizeof(aFound), 0x3bb935c5, 5));
	EXPECT_FALSE(pStorage->FindFile(Info.m_aFilename, ".", IStorage::TYPE_ALL, aFound, sizeof(aFound), 0x3bb935c6, 6));
}
