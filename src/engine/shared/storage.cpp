// copyright (c) 2007 magnus auvinen, see licence.txt for more info
#include <base/system.h>
#include <engine/storage.h>
#include "engine.h"

// compiled-in data-dir path
#define DATA_DIR "data"

class CStorage : public IStorage
{
public:
	char m_aApplicationSavePath[512];
	char m_aDatadir[512];
	
	CStorage()
	{
		m_aApplicationSavePath[0] = 0;
		m_aDatadir[0] = 0;
	}
	
	int Init(const char *pApplicationName, const char *pArgv0)
	{
		char aPath[1024] = {0};
		fs_storage_path(pApplicationName, m_aApplicationSavePath, sizeof(m_aApplicationSavePath));
		if(fs_makedir(m_aApplicationSavePath) == 0)
		{		
			str_format(aPath, sizeof(aPath), "%s/screenshots", m_aApplicationSavePath);
			fs_makedir(aPath);

			str_format(aPath, sizeof(aPath), "%s/maps", m_aApplicationSavePath);
			fs_makedir(aPath);

			str_format(aPath, sizeof(aPath), "%s/dumps", m_aApplicationSavePath);
			fs_makedir(aPath);

			str_format(aPath, sizeof(aPath), "%s/downloadedmaps", m_aApplicationSavePath);
			fs_makedir(aPath);

			str_format(aPath, sizeof(aPath), "%s/demos", m_aApplicationSavePath);
			fs_makedir(aPath);
		}
		
		return FindDatadir(pArgv0);
	}
		
	int FindDatadir(const char *pArgv0)
	{
		// 1) use provided data-dir override
		if(m_aDatadir[0])
		{
			if(fs_is_dir(m_aDatadir))
				return 0;
			else
			{
				dbg_msg("engine/datadir", "specified data-dir '%s' does not exist", m_aDatadir);
				return -1;
			}
		}
		
		// 2) use data-dir in PWD if present
		if(fs_is_dir("data/mapres"))
		{
			str_copy(m_aDatadir, "data", sizeof(m_aDatadir));
			return 0;
		}
		
		// 3) use compiled-in data-dir if present
		if (fs_is_dir(DATA_DIR "/mapres"))
		{
			str_copy(m_aDatadir, DATA_DIR, sizeof(m_aDatadir));
			return 0;
		}
		
		// 4) check for usable path in argv[0]
		{
			unsigned int Pos = ~0U;
			for(unsigned i = 0; pArgv0[i]; i++)
				if(pArgv0[i] == '/')
					Pos = i;
			
			if (Pos < sizeof(m_aDatadir))
			{
				char aBaseDir[sizeof(m_aDatadir)];
				str_copy(aBaseDir, pArgv0, Pos);
				aBaseDir[Pos] = '\0';
				str_format(m_aDatadir, sizeof(m_aDatadir), "%s/data", aBaseDir);
				
				if (fs_is_dir(m_aDatadir))
					return 0;
			}
		}
		
	#if defined(CONF_FAMILY_UNIX)
		// 5) check for all default locations
		{
			const char *aDirs[] = {
				"/usr/share/teeworlds/data",
				"/usr/share/games/teeworlds/data",
				"/usr/local/share/teeworlds/data",
				"/usr/local/share/games/teeworlds/data",
				"/opt/teeworlds/data"
			};
			const int DirsCount = sizeof(aDirs) / sizeof(aDirs[0]);
			
			int i;
			for (i = 0; i < DirsCount; i++)
			{
				if (fs_is_dir(aDirs[i]))
				{
					str_copy(m_aDatadir, aDirs[i], sizeof(m_aDatadir));
					return 0;
				}
			}
		}
	#endif
		
		// no data-dir found
		dbg_msg("engine/datadir", "warning no data directory found");
		return -1;
	}

	virtual void ListDirectory(int Types, const char *pPath, FS_LISTDIR_CALLBACK pfnCallback, void *pUser)
	{
		char aBuffer[1024];
		
		// list current directory
		if(Types&TYPE_CURRENT)
		{
			fs_listdir(pPath, pfnCallback, pUser);
		}
		
		// list users directory
		if(Types&TYPE_SAVE)
		{
			str_format(aBuffer, sizeof(aBuffer), "%s/%s", m_aApplicationSavePath, pPath);
			fs_listdir(aBuffer, pfnCallback, pUser);
		}
		
		// list datadir directory
		if(Types&TYPE_DATA)
		{
			str_format(aBuffer, sizeof(aBuffer), "%s/%s", m_aDatadir, pPath);
			fs_listdir(aBuffer, pfnCallback, pUser);
		}		
	}
	
	virtual IOHANDLE OpenFile(const char *pFilename, int Flags, char *pBuffer = 0, int BufferSize = 0)
	{
		char aBuffer[1024];
		if(!pBuffer)
		{
			pBuffer = aBuffer;
			BufferSize = sizeof(aBuffer);
		}
		
		if(Flags&IOFLAG_WRITE)
		{
			str_format(pBuffer, BufferSize, "%s/%s", m_aApplicationSavePath, pFilename);
			return io_open(pBuffer, Flags);
		}
		else
		{
			IOHANDLE Handle = 0;
			
			// check current directory
			Handle = io_open(pFilename, Flags);
			if(Handle)
				return Handle;
				
			// check user directory
			str_format(pBuffer, BufferSize, "%s/%s", m_aApplicationSavePath, pFilename);
			Handle = io_open(pBuffer, Flags);
			if(Handle)
				return Handle;
				
			// check normal data directory
			str_format(pBuffer, BufferSize, "%s/%s", m_aDatadir, pFilename);
			Handle = io_open(pBuffer, Flags);
			if(Handle)
				return Handle;
		}
		
		pBuffer[0] = 0;
		return 0;		
	}

	static IStorage *Create(const char *pApplicationName, const char *pArgv0)
	{
		CStorage *p = new CStorage();
		if(p->Init(pApplicationName, pArgv0))
		{
			delete p;
			p = 0;
		}
		return p;
	}
};

IStorage *CreateStorage(const char *pApplicationName, const char *pArgv0) { return CStorage::Create(pApplicationName, pArgv0); }
