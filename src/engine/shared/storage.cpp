/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/hash_ctxt.h>
#include <base/system.h>
#include <engine/storage.h>
#include "linereader.h"
#include <zlib.h>

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
	char m_aDataDir[MAX_PATH_LENGTH];
	char m_aUserDir[MAX_PATH_LENGTH];
	char m_aCurrentDir[MAX_PATH_LENGTH];
	char m_aAppDir[MAX_PATH_LENGTH];
	
	CStorage()
	{
		mem_zero(m_aaStoragePaths, sizeof(m_aaStoragePaths));
		m_NumPaths = 0;
		m_aDataDir[0] = 0;
		m_aUserDir[0] = 0;
		m_aCurrentDir[0] = 0;
		m_aAppDir[0] = 0;
	}

	int Init(const char *pApplicationName, int StorageType, int NumArgs, const char **ppArguments)
	{
		// get userdir
		fs_storage_path(pApplicationName, m_aUserDir, sizeof(m_aUserDir));
		
		// get appdir
		FindAppDir(ppArguments[0]);

		// get datadir
		FindDataDir();

		// get currentdir
		if(!fs_getcwd(m_aCurrentDir, sizeof(m_aCurrentDir)))
			m_aCurrentDir[0] = 0;

		// load paths from storage.cfg
		LoadPaths();

		if(!m_NumPaths)
		{
			dbg_msg("storage", "using standard paths");
			AddDefaultPaths();
		}

		// add save directories
		if(StorageType != STORAGETYPE_BASIC)
		{
			if(m_NumPaths && (!m_aaStoragePaths[TYPE_SAVE][0] || !fs_makedir_recursive(m_aaStoragePaths[TYPE_SAVE])))
			{
				char aPath[MAX_PATH_LENGTH];
				if(StorageType == STORAGETYPE_CLIENT)
				{
					fs_makedir(GetPath(TYPE_SAVE, "screenshots", aPath, sizeof(aPath)));
					fs_makedir(GetPath(TYPE_SAVE, "screenshots/auto", aPath, sizeof(aPath)));
					fs_makedir(GetPath(TYPE_SAVE, "maps", aPath, sizeof(aPath)));
					fs_makedir(GetPath(TYPE_SAVE, "downloadedmaps", aPath, sizeof(aPath)));
					fs_makedir(GetPath(TYPE_SAVE, "skins", aPath, sizeof(aPath)));
				}
				fs_makedir(GetPath(TYPE_SAVE, "dumps", aPath, sizeof(aPath)));
				fs_makedir(GetPath(TYPE_SAVE, "demos", aPath, sizeof(aPath)));
				fs_makedir(GetPath(TYPE_SAVE, "demos/auto", aPath, sizeof(aPath)));
				fs_makedir(GetPath(TYPE_SAVE, "configs", aPath, sizeof(aPath)));
			}
			else
			{
				dbg_msg("storage", "unable to create save directory");
				return 1;
			}
		}

		return m_NumPaths ? 0 : 1;
	}

	void LoadPaths()
	{
		// check current directory
		IOHANDLE File = io_open("storage.cfg", IOFLAG_READ);
		if(!File)
		{
			// check usable path in argv[0]
			char aBuffer[MAX_PATH_LENGTH];
			str_copy(aBuffer, m_aAppDir, sizeof(aBuffer));
			str_append(aBuffer, "/storage.cfg", sizeof(aBuffer));
			File = io_open(aBuffer, IOFLAG_READ);
			if(!File)
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
			const char *pLineWithoutPrefix = str_startswith(pLine, "add_path ");
			if(pLineWithoutPrefix)
			{
				AddPath(pLineWithoutPrefix);
			}
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
		AddPath("$APPDIR");
	}

	bool IsDuplicatePath(const char *pPath)
	{
		for(int i = 0; i < m_NumPaths; ++i)
		{
			if(!str_comp(m_aaStoragePaths[i], pPath))
				return true;
		}
		return false;
	}

	void AddPath(const char *pPath)
	{
		if(m_NumPaths >= MAX_PATHS || !pPath[0])
			return;

		if(!str_comp(pPath, "$USERDIR"))
		{
			if(m_aUserDir[0])
			{
				if(!IsDuplicatePath(m_aUserDir))
				{
					str_copy(m_aaStoragePaths[m_NumPaths++], m_aUserDir, MAX_PATH_LENGTH);
					dbg_msg("storage", "added path '$USERDIR' ('%s')", m_aUserDir);
				}
				else
					dbg_msg("storage", "skipping duplicate path '$USERDIR' ('%s')", m_aUserDir);
			}
		}
		else if(!str_comp(pPath, "$DATADIR"))
		{
			if(m_aDataDir[0])
			{
				if(!IsDuplicatePath(m_aDataDir))
				{
					str_copy(m_aaStoragePaths[m_NumPaths++], m_aDataDir, MAX_PATH_LENGTH);
					dbg_msg("storage", "added path '$DATADIR' ('%s')", m_aDataDir);
				}
				else
					dbg_msg("storage", "skipping duplicate path '$DATADIR' ('%s')", m_aDataDir);
			}
		}
		else if(!str_comp(pPath, "$CURRENTDIR"))
		{
			if(m_aCurrentDir[0])
			{
				if(!IsDuplicatePath(m_aCurrentDir))
				{
					str_copy(m_aaStoragePaths[m_NumPaths++], m_aCurrentDir, MAX_PATH_LENGTH);
					dbg_msg("storage", "added path '$CURRENTDIR' ('%s')", m_aCurrentDir);
				}
				else
					dbg_msg("storage", "skipping duplicate path '$CURRENTDIR' ('%s')", m_aCurrentDir);
			}
		}
		else if(!str_comp(pPath, "$APPDIR"))
		{
			if(m_aAppDir[0])
			{
				if(!IsDuplicatePath(m_aAppDir))
				{
					str_copy(m_aaStoragePaths[m_NumPaths++], m_aAppDir, MAX_PATH_LENGTH);
					dbg_msg("storage", "added path '$APPDIR' ('%s')", m_aAppDir);
				}
				else
					dbg_msg("storage", "skipping duplicate path '$APPDIR' ('%s')", m_aAppDir);
			}
		}
		else
		{
			if(fs_is_dir(pPath))
			{
				if(!IsDuplicatePath(pPath))
				{
					str_copy(m_aaStoragePaths[m_NumPaths++], pPath, MAX_PATH_LENGTH);
					dbg_msg("storage", "added path '%s'", pPath);
				}
				else
					dbg_msg("storage", "skipping duplicate path '%s'", pPath);
			}
		}
	}
	
	void FindAppDir(const char *pArgv0)
	{
		// check for usable path in argv[0]
		unsigned int Pos = ~0U;
		for(unsigned i = 0; pArgv0[i]; ++i)
			if(pArgv0[i] == '/' || pArgv0[i] == '\\')
				Pos = i;
		
		if(Pos < MAX_PATH_LENGTH)
		{
			str_copy(m_aAppDir, pArgv0, Pos+1);
			if(!fs_is_dir(m_aAppDir))
				m_aAppDir[0] = 0;
		}
	}

	void FindDataDir()
	{
		// 1) use data-dir in PWD if present
		if(fs_is_dir("data/mapres"))
		{
			str_copy(m_aDataDir, "data", sizeof(m_aDataDir));
			return;
		}

		// 2) use compiled-in data-dir if present
		if(fs_is_dir(DATA_DIR "/mapres"))
		{
			str_copy(m_aDataDir, DATA_DIR, sizeof(m_aDataDir));
			return;
		}

		// 3) check for usable path in argv[0]
		{
			char aBaseDir[MAX_PATH_LENGTH];
			str_copy(aBaseDir, m_aAppDir, sizeof(aBaseDir));
			str_format(m_aDataDir, sizeof(m_aDataDir), "%s/data", aBaseDir);
			str_append(aBaseDir, "/data/mapres", sizeof(aBaseDir));

			if(fs_is_dir(aBaseDir))
				return;
			else
				m_aDataDir[0] = 0;
		}

	#if defined(CONF_FAMILY_UNIX)
		// 4) check for all default locations
		{
			const char *aDirs[] = {
				"/usr/share/teeworlds/data",
				"/usr/share/games/teeworlds/data",
				"/usr/local/share/teeworlds/data",
				"/usr/local/share/games/teeworlds/data",
				"/usr/pkg/share/teeworlds/data",
				"/usr/pkg/share/games/teeworlds/data",
				"/opt/teeworlds/data"
			};
			const int DirsCount = sizeof(aDirs) / sizeof(aDirs[0]);

			int i;
			for (i = 0; i < DirsCount; i++)
			{
				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), "%s/mapres", aDirs[i]);
				if(fs_is_dir(aBuf))
				{
					str_copy(m_aDataDir, aDirs[i], sizeof(m_aDataDir));
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

	// Open a file. This checks that the path appears to be a subdirectory
	// of one of the storage paths.
	virtual IOHANDLE OpenFile(const char *pFilename, int Flags, int Type, char *pBuffer = 0, int BufferSize = 0, FCheckCallback pfnCheckCB = 0, const void *pCheckCBData = 0)
	{
		char aBuffer[MAX_PATH_LENGTH];
		if(!pBuffer)
		{
			pBuffer = aBuffer;
			BufferSize = sizeof(aBuffer);
		}

		// Check whether the path contains '..' (parent directory) paths. We'd
		// normally still have to check whether it is an absolute path
		// (starts with a path separator (or a drive name on windows)),
		// but since we concatenate this path with another path later
		// on, this can't become absolute.
		//
		// E. g. "/etc/passwd" => "/path/to/storage//etc/passwd", which
		// is safe.
		if(str_check_pathname(pFilename) != 0)
		{
			pBuffer[0] = 0;
			return 0;
		}

		// open file
		if(Flags&IOFLAG_WRITE)
		{
			return io_open(GetPath(TYPE_SAVE, pFilename, pBuffer, BufferSize), Flags);
		}
		else
		{
			IOHANDLE Handle = 0;
			int LB = 0, UB = m_NumPaths;	// check all available directories
			
			if(Type >= 0 && Type < m_NumPaths)	// check wanted directory
			{
				LB = Type;
				UB = Type + 1;
			}
			else
				dbg_assert(Type == TYPE_ALL, "invalid storage type");

			for(int i = LB; i < UB; ++i)
			{
				Handle = io_open(GetPath(i, pFilename, pBuffer, BufferSize), Flags);
				if(Handle)
				{
					// do an additional check on the file
					if(pfnCheckCB && !pfnCheckCB(Handle, pCheckCBData))
					{
						io_close(Handle);
						Handle = 0;
					}
					else
						return Handle;
				}
			}
		}

		pBuffer[0] = 0;
		return 0;
	}

	struct CFindCBData
	{
		CStorage *m_pStorage;
		const char *m_pFilename;
		const char *m_pPath;
		char *m_pBuffer;
		int m_BufferSize;
		const SHA256_DIGEST *m_pWantedSha256;
		unsigned m_WantedCrc;
		unsigned m_WantedSize;
		bool m_CheckHashAndSize;
	};

	static int FindFileCallback(const char *pName, int IsDir, int Type, void *pUser)
	{
		CFindCBData Data = *static_cast<CFindCBData *>(pUser);
		if(IsDir)
		{
			if(pName[0] == '.')
				return 0;

			// search within the folder
			char aBuf[MAX_PATH_LENGTH];
			char aPath[MAX_PATH_LENGTH];
			str_format(aPath, sizeof(aPath), "%s/%s", Data.m_pPath, pName);
			Data.m_pPath = aPath;
			fs_listdir(Data.m_pStorage->GetPath(Type, aPath, aBuf, sizeof(aBuf)), FindFileCallback, Type, &Data);
			if(Data.m_pBuffer[0])
				return 1;
		}
		else if(!str_comp(pName, Data.m_pFilename))
		{
			// found the file
			str_format(Data.m_pBuffer, Data.m_BufferSize, "%s/%s", Data.m_pPath, Data.m_pFilename);

			if(Data.m_CheckHashAndSize)
			{
				// check crc and size
				SHA256_DIGEST Sha256;
				unsigned Crc = 0;
				unsigned Size = 0;
				if(!Data.m_pStorage->GetHashAndSize(Data.m_pBuffer, Type, &Sha256, &Crc, &Size) || (Data.m_pWantedSha256 && Sha256 != *Data.m_pWantedSha256) || Crc != Data.m_WantedCrc || Size != Data.m_WantedSize)
				{
					Data.m_pBuffer[0] = 0;
					return 0;
				}
			}
			
			return 1;
		}

		return 0;
	}

	bool FindFileImpl(int Type, CFindCBData *pCBData)
	{
		if(pCBData->m_BufferSize < 1)
			return false;

		pCBData->m_pBuffer[0] = 0;

		char aBuf[MAX_PATH_LENGTH];
		
		if(Type == TYPE_ALL)
		{
			// search within all available directories
			for(int i = 0; i < m_NumPaths; ++i)
			{
				fs_listdir(GetPath(i, pCBData->m_pPath, aBuf, sizeof(aBuf)), FindFileCallback, i, pCBData);
				if(pCBData->m_pBuffer[0])
					return true;
			}
		}
		else if(Type >= 0 && Type < m_NumPaths)
		{
			// search within wanted directory
			fs_listdir(GetPath(Type, pCBData->m_pPath, aBuf, sizeof(aBuf)), FindFileCallback, Type, pCBData);
		}

		return pCBData->m_pBuffer[0] != 0;
	}

	virtual bool FindFile(const char *pFilename, const char *pPath, int Type, char *pBuffer, int BufferSize)
	{
		CFindCBData Data;
		Data.m_pStorage = this;
		Data.m_pFilename = pFilename;
		Data.m_pPath = pPath;
		Data.m_pBuffer = pBuffer;
		Data.m_BufferSize = BufferSize;
		Data.m_pWantedSha256 = 0;
		Data.m_WantedCrc = 0;
		Data.m_WantedSize = 0;
		Data.m_CheckHashAndSize = false;
		return FindFileImpl(Type, &Data);
	}

	virtual bool FindFile(const char *pFilename, const char *pPath, int Type, char *pBuffer, int BufferSize, const SHA256_DIGEST *pWantedSha256, unsigned WantedCrc, unsigned WantedSize)
	{
		CFindCBData Data;
		Data.m_pStorage = this;
		Data.m_pFilename = pFilename;
		Data.m_pPath = pPath;
		Data.m_pBuffer = pBuffer;
		Data.m_BufferSize = BufferSize;
		Data.m_pWantedSha256 = pWantedSha256;
		Data.m_WantedCrc = WantedCrc;
		Data.m_WantedSize = WantedSize;
		Data.m_CheckHashAndSize = true;
		return FindFileImpl(Type, &Data);
	}

	virtual bool RemoveFile(const char *pFilename, int Type)
	{
		if(Type < 0 || Type >= m_NumPaths)
			return false;

		char aBuffer[MAX_PATH_LENGTH];
		return !fs_remove(GetPath(Type, pFilename, aBuffer, sizeof(aBuffer)));
	}

	virtual bool RenameFile(const char* pOldFilename, const char* pNewFilename, int Type)
	{
		if(Type < 0 || Type >= m_NumPaths)
			return false;
		char aOldBuffer[MAX_PATH_LENGTH];
		char aNewBuffer[MAX_PATH_LENGTH];
		return !fs_rename(GetPath(Type, pOldFilename, aOldBuffer, sizeof(aOldBuffer)), GetPath(Type, pNewFilename, aNewBuffer, sizeof (aNewBuffer)));
	}

	virtual bool CreateFolder(const char *pFoldername, int Type)
	{
		if(Type < 0 || Type >= m_NumPaths)
			return false;

		char aBuffer[MAX_PATH_LENGTH];
		return !fs_makedir(GetPath(Type, pFoldername, aBuffer, sizeof(aBuffer)));
	}

	virtual void GetCompletePath(int Type, const char *pDir, char *pBuffer, unsigned BufferSize)
	{
		if(Type < 0 || Type >= m_NumPaths)
		{
			if(BufferSize > 0)
				pBuffer[0] = 0;
			return;
		}

		GetPath(Type, pDir, pBuffer, BufferSize);
	}
	
	virtual bool GetHashAndSize(const char *pFilename, int StorageType, SHA256_DIGEST *pSha256, unsigned *pCrc, unsigned *pSize)
	{
		IOHANDLE File = OpenFile(pFilename, IOFLAG_READ, StorageType);
		if(!File)
			return false;

		// get hash and size
		SHA256_CTX Sha256Ctx;
		sha256_init(&Sha256Ctx);
		unsigned Crc = 0;
		unsigned Size = 0;
		unsigned char aBuffer[64*1024];
		while(1)
		{
			unsigned Bytes = io_read(File, aBuffer, sizeof(aBuffer));
			if(Bytes <= 0)
				break;
			sha256_update(&Sha256Ctx, aBuffer, Bytes);
			Crc = crc32(Crc, aBuffer, Bytes); // ignore_convention
			Size += Bytes;
		}

		io_close(File);

		*pSha256 = sha256_finish(&Sha256Ctx);
		*pCrc = Crc;
		*pSize = Size;
		return true;
	}

	static IStorage *Create(const char *pApplicationName, int StorageType, int NumArgs, const char **ppArguments)
	{
		CStorage *p = new CStorage();
		if(p && p->Init(pApplicationName, StorageType, NumArgs, ppArguments))
		{
			dbg_msg("storage", "initialisation failed");
			delete p;
			p = 0;
		}
		return p;
	}

	static IStorage *CreateTest()
	{
		CStorage *p = new CStorage();
		if(!p)
		{
			return 0;
		}
		p->AddPath(".");
		return p;
	}
};

IStorage *CreateStorage(const char *pApplicationName, int StorageType, int NumArgs, const char **ppArguments) { return CStorage::Create(pApplicationName, StorageType, NumArgs, ppArguments); }
IStorage *CreateTestStorage() { return CStorage::CreateTest(); }
