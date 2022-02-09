/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <engine/shared/datafile.h>
#include <engine/storage.h>

int main(int argc, const char **argv)
{
	IStorage *pStorage = CreateStorage("Teeworlds", IStorage::STORAGETYPE_BASIC, argc, argv);
	int Index, ID = 0, Type = 0, Size;
	void *pPtr;
	char aFileName[1024];
	CDataFileReader DataFile;
	CDataFileWriter df;

	if(!pStorage || argc != 3)
		return -1;

	str_format(aFileName, sizeof(aFileName), "%s", argv[2]);

	if(!DataFile.Open(pStorage, argv[1], IStorage::TYPE_ALL))
		return -1;
	if(!df.Open(pStorage, aFileName))
		return -1;

	// add all items
	for(Index = 0; Index < DataFile.NumItems(); Index++)
	{
		pPtr = DataFile.GetItem(Index, &Type, &ID);
		Size = DataFile.GetItemSize(Index);
		df.AddItem(Type, ID, Size, pPtr);
	}

	// add all data
	for(Index = 0; Index < DataFile.NumData(); Index++)
	{
		pPtr = DataFile.GetData(Index);
		Size = DataFile.GetDataSize(Index);
		df.AddData(Size, pPtr);
	}

	DataFile.Close();
	df.Finish();
	return 0;
}
