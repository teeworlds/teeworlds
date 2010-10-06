// copyright (c) 2007 magnus auvinen, see licence.txt for more info
#include <base/system.h>
#include <engine/shared/datafile.h>
#include <engine/storage.h>

int main(int argc, const char **argv)
{
	IStorage *pStorage = CreateStorage("Teeworlds", argc, argv);
	int Index, Id = 0, Type = 0, Size;
	void *pPtr;
	char aFileName[1024];
	CDataFileReader DataFile;
	CDataFileWriter df;

	if(!pStorage || argc != 3)
		return -1;

	str_format(aFileName, sizeof(aFileName), "maps/%s", argv[2]);

	if(!DataFile.Open(pStorage, argv[1], IStorage::TYPE_ALL))
		return -1;
	if(!df.Open(pStorage, aFileName))
		return -1;

	// add all items
	for(Index = 0; Index < DataFile.NumItems(); Index++)
	{	
		pPtr = DataFile.GetItem(Index, &Type, &Id);
		Size = DataFile.GetItemSize(Index);
		df.AddItem(Type, Id, Size, pPtr);
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
