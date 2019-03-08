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

	SHA256_DIGEST Sha256 = sha256("test\n", 5);
	SHA256_DIGEST WrongSha256 = sha256("", 0);

	char aFound[128];

	EXPECT_TRUE(pStorage->FindFile(Info.m_aFilename, ".", IStorage::TYPE_ALL, aFound, sizeof(aFound)));
	EXPECT_STREQ(aFound, aFilenameWithDot);

	EXPECT_TRUE(pStorage->FindFile(Info.m_aFilename, ".", IStorage::TYPE_ALL, aFound, sizeof(aFound), 0, 0x3bb935c6, 5));
	EXPECT_STREQ(aFound, aFilenameWithDot);

	EXPECT_TRUE(pStorage->FindFile(Info.m_aFilename, ".", IStorage::TYPE_ALL, aFound, sizeof(aFound), &Sha256, 0x3bb935c6, 5));
	EXPECT_STREQ(aFound, aFilenameWithDot);

	EXPECT_FALSE(pStorage->FindFile(Info.m_aFilename, ".", IStorage::TYPE_ALL, aFound, sizeof(aFound), 0, 0, 0));
	EXPECT_FALSE(pStorage->FindFile(Info.m_aFilename, ".", IStorage::TYPE_ALL, aFound, sizeof(aFound), 0, 0x3bb935c6, 0));
	EXPECT_FALSE(pStorage->FindFile(Info.m_aFilename, ".", IStorage::TYPE_ALL, aFound, sizeof(aFound), 0, 0, 5));
	EXPECT_FALSE(pStorage->FindFile(Info.m_aFilename, ".", IStorage::TYPE_ALL, aFound, sizeof(aFound), 0, 0x3bb935c5, 5));
	EXPECT_FALSE(pStorage->FindFile(Info.m_aFilename, ".", IStorage::TYPE_ALL, aFound, sizeof(aFound), 0, 0x3bb935c6, 6));

	EXPECT_FALSE(pStorage->FindFile(Info.m_aFilename, ".", IStorage::TYPE_ALL, aFound, sizeof(aFound), &WrongSha256, 0x3bb935c6, 5));
	EXPECT_FALSE(pStorage->FindFile(Info.m_aFilename, ".", IStorage::TYPE_ALL, aFound, sizeof(aFound), &SHA256_ZEROED, 0x3bb935c6, 5));
}
