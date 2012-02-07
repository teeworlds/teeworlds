/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_CONSOLE_H
#define ENGINE_SHARED_CONSOLE_H

#include <engine/console.h>
#include "memheap.h"

class CConsole : public IConsole
{
	class CCommand : public CCommandInfo
	{
	public:
		CCommand *m_pNext;
		int m_Flags;
		bool m_Temp;
		FCommandCallback m_pfnCallback;
		void *m_pUserData;

		virtual const CCommandInfo *NextCommandInfo(int AccessLevel, int FlagMask) const;

		void SetAccessLevel(int AccessLevel) { m_AccessLevel = clamp(AccessLevel, (int)(ACCESS_LEVEL_ADMIN), (int)(ACCESS_LEVEL_MOD)); }
	};


	class CChain
	{
	public:
		FChainCommandCallback m_pfnChainCallback;
		FCommandCallback m_pfnCallback;
		void *m_pCallbackUserData;
		void *m_pUserData;
	};

	int m_FlagMask;
	bool m_StoreCommands;
	const char *m_paStrokeStr[2];
	CCommand *m_pFirstCommand;

	class CExecFile
	{
	public:
		const char *m_pFilename;
		CExecFile *m_pPrev;
	};

	CExecFile *m_pFirstExec;
	class IStorage *m_pStorage;
	int m_AccessLevel;

	CCommand *m_pRecycleList;
	CHeap m_TempCommands;

	static void Con_Chain(IResult *pResult, void *pUserData);
	static void Con_Echo(IResult *pResult, void *pUserData);
	static void Con_Exec(IResult *pResult, void *pUserData);
	static void ConToggle(IResult *pResult, void *pUser);
	static void ConToggleStroke(IResult *pResult, void *pUser);
	static void ConModCommandAccess(IResult *pResult, void *pUser);
	static void ConModCommandStatus(IConsole::IResult *pResult, void *pUser);

	void ExecuteFileRecurse(const char *pFilename);
	void ExecuteLineStroked(int Stroke, const char *pStr);

	struct
	{
		int m_OutputLevel;
		FPrintCallback m_pfnPrintCallback;
		void *m_pPrintCallbackUserdata;
	} m_aPrintCB[MAX_PRINT_CB];
	int m_NumPrintCB;

	enum
	{
		CONSOLE_MAX_STR_LENGTH = 1024,
		MAX_PARTS = (CONSOLE_MAX_STR_LENGTH+1)/2
	};

	class CResult : public IResult
	{
	public:
		char m_aStringStorage[CONSOLE_MAX_STR_LENGTH+1];
		char *m_pArgsStart;

		const char *m_pCommand;
		const char *m_apArgs[MAX_PARTS];

		CResult() : IResult()
		{
			mem_zero(m_aStringStorage, sizeof(m_aStringStorage));
			m_pArgsStart = 0;
			m_pCommand = 0;
			mem_zero(m_apArgs, sizeof(m_apArgs));
		}

		CResult &operator =(const CResult &Other)
		{
			if(this != &Other)
			{
				IResult::operator=(Other);
				int Offset = m_aStringStorage - Other.m_aStringStorage;
				mem_copy(m_aStringStorage, Other.m_aStringStorage, sizeof(m_aStringStorage));
				m_pArgsStart = Other.m_pArgsStart + Offset;
				m_pCommand = Other.m_pCommand + Offset;
				for(unsigned i = 0; i < Other.m_NumArgs; ++i)
					m_apArgs[i] = Other.m_apArgs[i] + Offset;
			}
			return *this;
		}

		void AddArgument(const char *pArg)
		{
			m_apArgs[m_NumArgs++] = pArg;
		}

		virtual const char *GetString(unsigned Index);
		virtual int GetInteger(unsigned Index);
		virtual float GetFloat(unsigned Index);
	};

	int ParseStart(CResult *pResult, const char *pString, int Length);
	int ParseArgs(CResult *pResult, const char *pFormat);

	class CExecutionQueue
	{
		CHeap m_Queue;

	public:
		struct CQueueEntry
		{
			CQueueEntry *m_pNext;
			FCommandCallback m_pfnCommandCallback;
			void *m_pCommandUserData;
			CResult m_Result;
		} *m_pFirst, *m_pLast;

		void AddEntry()
		{
			CQueueEntry *pEntry = static_cast<CQueueEntry *>(m_Queue.Allocate(sizeof(CQueueEntry)));
			pEntry->m_pNext = 0;
			if(!m_pFirst)
				m_pFirst = pEntry;
			if(m_pLast)
				m_pLast->m_pNext = pEntry;
			m_pLast = pEntry;
			(void)new(&(pEntry->m_Result)) CResult;
		}
		void Reset()
		{
			m_Queue.Reset();
			m_pFirst = m_pLast = 0;
		}
	} m_ExecutionQueue;

	void AddCommandSorted(CCommand *pCommand);
	CCommand *FindCommand(const char *pName, int FlagMask);

public:
	CConsole(int FlagMask);

	virtual const CCommandInfo *FirstCommandInfo(int AccessLevel, int FlagMask) const;
	virtual const CCommandInfo *GetCommandInfo(const char *pName, int FlagMask, bool Temp);
	virtual void PossibleCommands(const char *pStr, int FlagMask, bool Temp, FPossibleCallback pfnCallback, void *pUser);

	virtual void ParseArguments(int NumArgs, const char **ppArguments);
	virtual void Register(const char *pName, const char *pParams, int Flags, FCommandCallback pfnFunc, void *pUser, const char *pHelp);
	virtual void RegisterTemp(const char *pName, const char *pParams, int Flags, const char *pHelp);
	virtual void DeregisterTemp(const char *pName);
	virtual void DeregisterTempAll();
	virtual void Chain(const char *pName, FChainCommandCallback pfnChainFunc, void *pUser);
	virtual void StoreCommands(bool Store);

	virtual bool LineIsValid(const char *pStr);
	virtual void ExecuteLine(const char *pStr);
	virtual void ExecuteLineFlag(const char *pStr, int FlagMask);
	virtual void ExecuteFile(const char *pFilename);

	virtual int RegisterPrintCallback(int OutputLevel, FPrintCallback pfnPrintCallback, void *pUserData);
	virtual void SetPrintOutputLevel(int Index, int OutputLevel);
	virtual void Print(int Level, const char *pFrom, const char *pStr);

	void SetAccessLevel(int AccessLevel) { m_AccessLevel = clamp(AccessLevel, (int)(ACCESS_LEVEL_ADMIN), (int)(ACCESS_LEVEL_MOD)); }
};

#endif
