#include "test.h"

#include <gtest/gtest.h>

#include <engine/shared/datafile.h>
#include <engine/storage.h>

TEST(Datafile, RoundtripItemDataAndSize)
{
	CTestInfo Info;
	char aFilename[64];
	Info.Filename(aFilename, sizeof(aFilename), ".datafile");
	IStorage *pStorage = CreateTestStorage();
	CDataFileWriter Writer;
	ASSERT_TRUE(Writer.Open(pStorage, aFilename));

	static const char TEST_DATA[] = "Hello World!";
	int Index = Writer.AddData(sizeof(TEST_DATA), TEST_DATA);
	int Index4 = Writer.AddData(4, TEST_DATA);
	int aItem[2] = {Index, Index4};
	Writer.AddItem(12, 34, sizeof(aItem), aItem);
	EXPECT_TRUE(Writer.Finish());

	CDataFileReader Reader;
	EXPECT_FALSE(Reader.IsOpen());
	ASSERT_TRUE(Reader.Open(pStorage, aFilename, IStorage::TYPE_ALL));
	EXPECT_TRUE(Reader.IsOpen());

	ASSERT_EQ(Reader.GetDataSize(Index), sizeof(TEST_DATA));
	EXPECT_TRUE(mem_comp(Reader.GetData(Index), TEST_DATA, sizeof(TEST_DATA)) == 0);
	ASSERT_EQ(Reader.GetDataSize(Index4), 4);
	EXPECT_TRUE(mem_comp(Reader.GetData(Index), TEST_DATA, 4) == 0);

	static const char REPL_DATA[] = "Replacement";
	char *pReplace4 = (char *)mem_alloc(4, 1);
	mem_copy(pReplace4, REPL_DATA, 4);
	char *pReplace = (char *)mem_alloc(sizeof(REPL_DATA), 1);
	mem_copy(pReplace, REPL_DATA, sizeof(REPL_DATA));
	Reader.ReplaceData(Index, pReplace4, 4);
	Reader.ReplaceData(Index4, pReplace, sizeof(REPL_DATA));

	ASSERT_EQ(Reader.GetDataSize(Index), 4);
	EXPECT_TRUE(mem_comp(Reader.GetData(Index), REPL_DATA, 4) == 0);
	ASSERT_EQ(Reader.GetDataSize(Index4), sizeof(REPL_DATA));
	EXPECT_TRUE(mem_comp(Reader.GetData(Index4), REPL_DATA, sizeof(REPL_DATA)) == 0);

	bool FoundItem = false;
	for(int i = 0; i < Reader.NumItems(); i++)
	{
		int Type;
		int ID;
		void *pItem = Reader.GetItem(i, &Type, &ID);
		ASSERT_EQ(Reader.GetItemSize(i), sizeof(aItem));
		EXPECT_TRUE(mem_comp(pItem, aItem, sizeof(aItem)) == 0);
		EXPECT_EQ(Type, 12);
		EXPECT_EQ(ID, 34);

		FoundItem = true;
	}
	EXPECT_TRUE(FoundItem);

	EXPECT_TRUE(Reader.Close());

	EXPECT_TRUE(pStorage->RemoveFile(aFilename, IStorage::TYPE_SAVE));
}
