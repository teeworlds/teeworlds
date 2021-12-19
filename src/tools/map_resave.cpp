/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <engine/shared/datafile.h>
#include <engine/storage.h>

int main(int argc, const char **argv)
{
	dbg_logger_stdout();
	cmdline_fix(&argc, &argv);

	if(argc != 3)
	{
		dbg_msg("map_resave", "usage: %s old.map new.map", argv[0]);
		return -1;
	}

	IStorage *pStorage = CreateStorage("Teeworlds", IStorage::STORAGETYPE_BASIC, argc, argv);
	if(!pStorage)
		return -1;

	CDataFileReader Reader;
	if(!Reader.Open(pStorage, argv[1], IStorage::TYPE_ALL))
	{
		dbg_msg("map_resave", "failed to open input file '%s': %s", argv[1], Reader.GetError());
		return -1;
	}

	CDataFileWriter Writer;
	if(!Writer.Open(pStorage, argv[2]))
	{
		dbg_msg("map_resave", "failed to open output file '%s'", argv[2]);
		return -1;
	}

	// add all items
	for(int Index = 0; Index < Reader.NumItems(); Index++)
	{
		int Type, ID, Size;
		void *pPtr = Reader.GetItem(Index, &Type, &ID, &Size);
		if(!pPtr)
		{
			dbg_msg("map_resave", "failed to get map item %d", Index);
			continue;
		}
		if(Writer.AddItem(Type, ID, Size, pPtr) == -1)
			dbg_msg("map_resave", "failed to add map item %d (Type=%d, ID=%d, Size=%d)", Index, Type, ID, Size);
	}

	// add all data
	for(int Index = 0; Index < Reader.NumData(); Index++)
	{
		void *pPtr = Reader.GetData(Index);
		int Size = Reader.GetDataSize(Index);
		if(!pPtr || Size <= 0)
		{
			dbg_msg("map_resave", "failed to get map data %d", Index);
			continue;
		}
		if(Writer.AddData(Size, pPtr) == -1)
			dbg_msg("map_resave", "failed to add map data %d (Size=%d)", Index, Size);
	}

	Reader.Close();
	Writer.Finish();

	dbg_msg("map_resave", "map successfully resaved to '%s'", argv[2]);

	cmdline_free(argc, argv);
	return 0;
}
