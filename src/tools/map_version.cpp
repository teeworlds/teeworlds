/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <base/math.h>

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

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "maps/%s", pName);
	if(!s_pEngineMap->Load(aBuf))
		return 0;

	unsigned MapCrc = s_pEngineMap->Crc();
	SHA256_DIGEST MapSha256 = s_pEngineMap->Sha256();
	unsigned MapSize = s_pEngineMap->FileSize();
	s_pEngineMap->Unload();

	char aMapName[8];
	str_copy(aMapName, pName, minimum((int)sizeof(aMapName),l-3));

	str_format(aBuf, sizeof(aBuf),
		"\t{\"%s\", {0x%02x, 0x%02x, 0x%02x, 0x%02x}, {0x%02x, 0x%02x, 0x%02x, 0x%02x}, {0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x, 0x%02x}},\n",
		aMapName,
		(MapCrc>>24)&0xff, (MapCrc>>16)&0xff, (MapCrc>>8)&0xff, MapCrc&0xff,
		(MapSize>>24)&0xff, (MapSize>>16)&0xff, (MapSize>>8)&0xff, MapSize&0xff,
		MapSha256.data[0], MapSha256.data[1], MapSha256.data[2], MapSha256.data[3], MapSha256.data[4], MapSha256.data[5], MapSha256.data[6], MapSha256.data[7],
		MapSha256.data[8], MapSha256.data[9], MapSha256.data[10], MapSha256.data[11], MapSha256.data[12], MapSha256.data[13], MapSha256.data[14], MapSha256.data[15],
		MapSha256.data[16], MapSha256.data[17], MapSha256.data[18], MapSha256.data[19], MapSha256.data[20], MapSha256.data[21], MapSha256.data[22], MapSha256.data[23],
		MapSha256.data[24], MapSha256.data[25], MapSha256.data[26], MapSha256.data[27], MapSha256.data[28], MapSha256.data[29], MapSha256.data[30], MapSha256.data[31]);
	io_write(s_File, aBuf, str_length(aBuf));

	return 0;
}

int main(int argc, const char **argv)
{
	cmdline_fix(&argc, &argv);

	IKernel *pKernel = IKernel::Create();
	s_pStorage = CreateStorage("Teeworlds", IStorage::STORAGETYPE_BASIC, argc, argv);
	s_pEngineMap = CreateEngineMap();

	bool RegisterFail = !pKernel->RegisterInterface(s_pStorage);
	RegisterFail |= !pKernel->RegisterInterface(s_pEngineMap);

	if(RegisterFail)
		return -1;

	const int StorageType = 1; // this tools assumes that the data-dir is the second storage path
	s_File = s_pStorage->OpenFile("map_version.txt", IOFLAG_WRITE, StorageType);
	if(s_File)
	{
		io_write(s_File, "static CMapVersion s_aMapVersionList[] = {\n", str_length("static const CMapVersion s_aMapVersionList[] = {\n"));
		s_pStorage->ListDirectory(StorageType, "maps", MaplistCallback, 0x0);
		io_write(s_File, "};\n", str_length("};\n"));
		io_close(s_File);
	}

	cmdline_free(argc, argv);
	return 0;
}
