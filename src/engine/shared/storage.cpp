// copyright (c) 2007 magnus auvinen, see licence.txt for more info
#include <stdio.h> //remove()
#include <base/system.h>
#include <engine/storage.h>
#include "engine.h"
#include "linereader.h"

// compiled-in data-dir path
#define DATA_DIR "data"

class CStorage : public IStorage
{
public:
	enum
	{
		MAX_PATHS = 16,
		MAX_PATH_LENGTH = 512
	};

	char m_aaStoragePaths[MAX_PATHS][MAX_PATH_LENGTH];
	int m_NumPaths;
	char m_aDatadir[MAX_PATH_LENGTH];
	char m_aUserdir[MAX_PATH_LENGTH];
	
	CStorage()
	{
		mem_zero(m_aaStoragePaths, sizeof(m_aaStoragePaths));
		m_NumPaths = 0;
		m_aDatadir[0] = 0;
		m_aUserdir[0] = 0;
	}
	
	int Init(const char *pApplicationName, int NumArgs, const char **ppArguments)
	{
		// get userdir
		fs_storage_path(pApplicationName, m_aUserdir, sizeof(m_aUserdir));

		// check for datadir override
		for(int i = 1; i < NumArgs; i++)
		{
			if(ppArguments[i][0] == '-' && ppArguments[i][1] == 'd' && ppArguments[i][2] == 0 && NumArgs - i > 1)
			{
				str_copy(m_aDatadir, ppArguments[i+1], sizeof(m_aDatadir));
				break;
			}
		}
		// get datadir
		FindDatadir(ppArguments[0]);

		// load paths from storage.cfg
		LoadPaths(ppArguments[0]);

		if(!m_NumPaths)
		{
			dbg_msg("storage", "using standard paths");
			AddDefaultPaths();
		}

		// add save directories
		if(m_NumPaths && (!m_aaStoragePaths[TYPE_SAVE][0] || !fs_makedir(m_aaStoragePaths[TYPE_SAVE])))
		{
			char aPath[MAX_PATH_LENGTH];
			fs_makedir(GetPath(TYPE_SAVE, "screenshots", aPath, sizeof(aPath)));
			fs_makedir(GetPath(TYPE_SAVE, "maps", aPath, sizeof(aPath)));
			fs_makedir(GetPath(TYPE_SAVE, "dumps", aPath, sizeof(aPath)));
			fs_makedir(GetPath(TYPE_SAVE, "downloadedmaps", aPath, sizeof(aPath)));
			fs_makedir(GetPath(TYPE_SAVE, "demos", aPath, sizeof(aPath)));
		}

		return m_NumPaths ? 0 : 1;
	}

	void LoadPaths(const char *pArgv0)
	{
		// check current directory
		IOHANDLE File = io_open("storage.cfg", IOFLAG_READ);
		if(!File)
		{
			// check usable path in argv[0]
			unsigned int Pos = ~0U;
			for(unsigned i = 0; pArgv0[i]; i++)
				if(pArgv0[i] == '/' || pArgv0[i] == '\\')
					Pos = i;
			if(Pos < MAX_PATH_LENGTH)
			{
				char aBuffer[MAX_PATH_LENGTH];
				str_copy(aBuffer, pArgv0, Pos+1);
				str_append(aBuffer, "/storage.cfg", sizeof(aBuffer));
				File = io_open(aBuffer, IOFLAG_READ);
			}
			
			if(Pos >= MAX_PATH_LENGTH || !File)
			{
				dbg_msg("storage", "couldn't open storage.cfg");
				return;
			}
		}

		char *pLine;
		CLineReader LineReader;
		LineReader.Init(File);

		while((pLine = LineReader.Get()))
		{
			if(str_length(pLine) > 9 && !str_comp_num(pLine, "add_path ", 9))
				AddPath(pLine+9);
		}

		io_close(File);

		if(!m_NumPaths)
			dbg_msg("storage", "no paths found in storage.cfg");
	}

	void AddDefaultPaths()
	{
		AddPath("$USERDIR");
		AddPath("$DATADIR");
		AddPath("$CURRENTDIR");
	}

	void AddPath(const char *pPath)
	{
		if(m_NumPaths >= MAX_PATHS || !pPath[0])
			return;

		int OldNum = m_NumPaths;

		if(!str_comp(pPath, "$USERDIR"))
		{
			if(m_aUserdir[0])
				str_copy(m_aaStoragePaths[m_NumPaths++], m_aUserdir, MAX_PATH_LENGTH);
		}
		else if(!str_comp(pPath, "$DATADIR"))
		{
			if(m_aDatadir[0])
				str_copy(m_aaStoragePaths[m_NumPaths++], m_aDatadir, MAX_PATH_LENGTH);
		}
		else if(!str_comp(pPath, "$CURRENTDIR"))
		{
			m_aaStoragePaths[m_NumPaths++][0] = 0;
		}
		else
		{
			if(fs_is_dir(pPath))
				str_copy(m_aaStoragePaths[m_NumPaths++], pPath, MAX_PATH_LENGTH);
		}

		if(OldNum != m_NumPaths)
			dbg_msg("storage", "added path '%s'", pPath);
	}
		
	void FindDatadir(const char *pArgv0)
	{
		// 1) use provided data-dir override
		if(m_aDatadir[0])
		{
			char aBuffer[MAX_PATH_LENGTH];
			str_format(aBuffer, sizeof(aBuffer), "%s/mapres", m_aDatadir);
			if(!fs_is_dir(aBuffer))
			{
				dbg_msg("storage", "specified data directory '%s' does not exist", m_aDatadir);
				m_aDatadir[0] = 0;
			}
			else
				return;
		}
		
		// 2) use data-dir in PWD if present
		if(fs_is_dir("data/mapres"))
		{
			str_copy(m_aDatadir, "data", sizeof(m_aDatadir));
			return;
		}
		
		// 3) use compiled-in data-dir if present
		if(fs_is_dir(DATA_DIR "/mapres"))
		{
			str_copy(m_aDatadir, DATA_DIR, sizeof(m_aDatadir));
			return;
		}
		
		// 4) check for usable path in argv[0]
		{
			unsigned int Pos = ~0U;
			for(unsigned i = 0; pArgv0[i]; i++)
				if(pArgv0[i] == '/' || pArgv0[i] == '\\')
					Pos = i;
			
			if(Pos < MAX_PATH_LENGTH)
			{
				char aBaseDir[MAX_PATH_LENGTH];
				str_copy(aBaseDir, pArgv0, Pos+1);
				str_format(m_aDatadir, sizeof(m_aDatadir), "%s/data", aBaseDir);
				str_append(aBaseDir, "/data/mapres", sizeof(aBaseDir));
				
				if(fs_is_dir(aBaseDir))
					return;
				else
					m_aDatadir[0] = 0;
			}
		}
		
	#if defined(CONF_FAMILY_UNIX)
		// 5) check for all default locations
		{
			const char *aDirs[] = {
				"/usr/share/teeworlds/data/mapres",
				"/usr/share/games/teeworlds/data/mapres",
				"/usr/local/share/teeworlds/data/mapres",
				"/usr/local/share/games/teeworlds/data/mapres",
				"/opt/teeworlds/data/mapres"
			};
			const int DirsCount = sizeof(aDirs) / sizeof(aDirs[0]);
			
			int i;
			for (i = 0; i < DirsCount; i++)
			{
				if (fs_is_dir(aDirs[i]))
				{
					str_copy(m_aDatadir, aDirs[i], sizeof(m_aDatadir));
					return;
				}
			}
		}
	#endif
		
		// no data-dir found
		dbg_msg("storage", "warning no data directory found");
	}

	virtual void ListDirectory(int Type, const char *pPath, FS_LISTDIR_CALLBACK pfnCallback, void *pUser)
	{
		char aBuffer[MAX_PATH_LENGTH];
		if(Type == TYPE_ALL)
		{
			// list all available directories
			for(int i = 0; i < m_NumPaths; ++i)
				fs_listdir(GetPath(i, pPath, aBuffer, sizeof(aBuffer)), pfnCallback, i, pUser);
		}
		else if(Type >= 0 && Type < m_NumPaths)
		{
			// list wanted directory
			fs_listdir(GetPath(Type, pPath, aBuffer, sizeof(aBuffer)), pfnCallback, Type, pUser);
		}
	}

	const char *GetPath(int Type, const char *pDir, char *pBuffer, unsigned BufferSize)
	{
		str_format(pBuffer, BufferSize, "%s%s%s", m_aaStoragePaths[Type], !m_aaStoragePaths[Type][0] ? "" : "/", pDir);
		return pBuffer;
	}
	
	virtual IOHANDLE OpenFile(const char *pFilename, int Flags, int Type, char *pBuffer = 0, int BufferSize = 0)
	{
		char aBuffer[MAX_PATH_LENGTH];
		if(!pBuffer)
		{
			pBuffer = aBuffer;
			BufferSize = sizeof(aBuffer);
		}
		
		if(Flags&IOFLAG_WRITE)
		{
			return io_open(GetPath(TYPE_SAVE, pFilename, pBuffer, BufferSize), Flags);
		}
		else
		{
			IOHANDLE Handle = 0;

			if(Type == TYPE_ALL)
			{
				// check all available directories
				for(int i = 0; i < m_NumPaths; ++i)
				{
					Handle = io_open(GetPath(i, pFilename, pBuffer, BufferSize), Flags);
					if(Handle)
						return Handle;
				}
			}
			else if(Type >= 0 && Type < m_NumPaths)
			{
				// check wanted directory
				Handle = io_open(GetPath(Type, pFilename, pBuffer, BufferSize), Flags);
				if(Handle)
					return Handle;
			}
		}
		
		pBuffer[0] = 0;
		return 0;		
	}
 	
	virtual bool RemoveFile(const char *pFilename, int Type)
	{
		if(Type < 0 || Type >= m_NumPaths)
			return false;

		char aBuffer[MAX_PATH_LENGTH];
		return remove(GetPath(Type, pFilename, aBuffer, sizeof(aBuffer)));
	}

	virtual bool RenameFile(const char *pOldFilename, const char *pNewFilename, int Type)
	{
		if(Type < 0 || Type >= m_NumPaths)
			return false;

		char aOldBuffer[MAX_PATH_LENGTH];
		char aNewBuffer[MAX_PATH_LENGTH];
		rename(GetPath(Type, pOldFilename, aOldBuffer, sizeof(aOldBuffer)), GetPath(Type, pNewFilename, aNewBuffer, sizeof(aNewBuffer)));
		return remove(GetPath(Type, pOldFilename, aOldBuffer, sizeof(aOldBuffer)));
	}
	
	static IStorage *Create(const char *pApplicationName, int NumArgs, const char **ppArguments)
	{
		CStorage *p = new CStorage();
		if(p && p->Init(pApplicationName, NumArgs, ppArguments))
		{
			dbg_msg("storage", "initialisation failed");
			delete p;
			p = 0;
		}
		return p;
	}
};

IStorage *CreateStorage(const char *pApplicationName, int NumArgs, const char **ppArguments) { return CStorage::Create(pApplicationName, NumArgs, ppArguments); }
