/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>
#include <base/system.h>

#include <engine/kernel.h>
#include <engine/map.h>
#include <engine/storage.h>


static IOHANDLE s_File = 0;
static IStorage *s_pStorage = 0;
static IEngineMap *s_pEngineMap = 0;

int MaplistCallback(const char *pName, int IsDir, int DirType, void *pUser)
{
	int l = str_length(pName);
	if(l < 4 || IsDir || str_comp(pName+l-4, ".map") != 0)
		return 0;

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "maps/%s", pName);
	if(!s_pEngineMap->Load(aBuf))
		return 0;

	unsigned MapCrc = s_pEngineMap->Crc();
	s_pEngineMap->Unload();

	IOHANDLE MapFile = s_pStorage->OpenFile(aBuf, IOFLAG_READ, DirType);
	unsigned MapSize = io_length(MapFile);
	io_close(MapFile);

	char aMapName[8];
	str_copy(aMapName, pName, min((int)sizeof(aMapName),l-3));

	str_format(aBuf, sizeof(aBuf), "\t{\"%s\", {0x%02x, 0x%02x, 0x%02x, 0x%02x}, {0x%02x, 0x%02x, 0x%02x, 0x%02x}},\n", aMapName,
		(MapCrc>>24)&0xff, (MapCrc>>16)&0xff, (MapCrc>>8)&0xff, MapCrc&0xff,
		(MapSize>>24)&0xff, (MapSize>>16)&0xff, (MapSize>>8)&0xff, MapSize&0xff);
	io_write(s_File, aBuf, str_length(aBuf));

	return 0;
}

int main(int argc, const char **argv) // ignore_convention
{
	IKernel *pKernel = IKernel::Create();
	s_pStorage = CreateStorage("Teeworlds", IStorage::STORAGETYPE_BASIC, argc, argv);
	s_pEngineMap = CreateEngineMap();

	bool RegisterFail = !pKernel->RegisterInterface(s_pStorage);
	RegisterFail |= !pKernel->RegisterInterface(s_pEngineMap);

	if(RegisterFail)
		return -1;

	s_File = s_pStorage->OpenFile("map_version.txt", IOFLAG_WRITE, 1);
	if(s_File)
	{
		io_write(s_File, "static CMapVersion s_aMapVersionList[] = {\n", str_length("static CMapVersion s_aMapVersionList[] = {\n"));
		s_pStorage->ListDirectory(1, "maps", MaplistCallback, 0);
		io_write(s_File, "};\n", str_length("};\n"));
		io_close(s_File);
	}

	return 0;
}
