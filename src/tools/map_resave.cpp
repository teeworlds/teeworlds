/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <engine/shared/datafile.h>
#include <engine/storage.h>

int main(int argc, const char **argv)
{
	cmdline_fix(&argc, &argv);

	IStorage *pStorage = CreateStorage("Teeworlds", IStorage::STORAGETYPE_BASIC, argc, argv);
	if(!pStorage || argc != 3)
		return -1;

	CDataFileReader Reader;
	if(!Reader.Open(pStorage, argv[1], IStorage::TYPE_ALL))
		return -1;

	CDataFileWriter Writer;
	if(!Writer.Open(pStorage, argv[2]))
		return -1;

	// add all items
	for(int Index = 0; Index < Reader.NumItems(); Index++)
	{
		int Type, ID;
		void *pPtr = Reader.GetItem(Index, &Type, &ID);
		int Size = Reader.GetItemSize(Index);
		Writer.AddItem(Type, ID, Size, pPtr);
	}

	// add all data
	for(int Index = 0; Index < Reader.NumData(); Index++)
	{
		void *pPtr = Reader.GetData(Index);
		int Size = Reader.GetDataSize(Index);
		Writer.AddData(Size, pPtr);
	}

	Reader.Close();
	Writer.Finish();

	cmdline_free(argc, argv);
	return 0;
}
