/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <base/system.h>
#include <engine/shared/protocol.h>
#include <engine/storage.h>
#include "console.h"
#include "config.h"
#include "linereader.h"

const char *CConsole::CResult::GetString(unsigned Index)
{
	if (Index < 0 || Index >= m_NumArgs)
		return "";
	return m_apArgs[Index];
}

int CConsole::CResult::GetInteger(unsigned Index)
{
	if (Index < 0 || Index >= m_NumArgs)
		return 0;
	return str_toint(m_apArgs[Index]);
}

float CConsole::CResult::GetFloat(unsigned Index)
{
	if (Index < 0 || Index >= m_NumArgs)
		return 0.0f;
	return str_tofloat(m_apArgs[Index]);
}

// the maximum number of tokens occurs in a string of length CONSOLE_MAX_STR_LENGTH with tokens size 1 separated by single spaces


int CConsole::ParseStart(CResult *pResult, const char *pString, int Length)
{
	char *pStr;
	int Len = sizeof(pResult->m_aStringStorage);
	if(Length < Len)
		Len = Length;

	str_copy(pResult->m_aStringStorage, pString, Length);
	pStr = pResult->m_aStringStorage;

	// get command
	pStr = str_skip_whitespaces(pStr);
	pResult->m_pCommand = pStr;
	pStr = str_skip_to_whitespace(pStr);

	if(*pStr)
	{
		pStr[0] = 0;
		pStr++;
	}

	pResult->m_pArgsStart = pStr;
	return 0;
}

int CConsole::ParseArgs(CResult *pResult, const char *pFormat)
{
	char Command;
	char *pStr;
	int Optional = 0;
	int Error = 0;

	pStr = pResult->m_pArgsStart;

	while(1)
	{
		// fetch command
		Command = *pFormat;
		pFormat++;

		if(!Command)
			break;

		if(Command == '?')
			Optional = 1;
		else
		{
			pStr = str_skip_whitespaces(pStr);

			if(!(*pStr)) // error, non optional command needs value
			{
				if(!Optional)
					Error = 1;
				break;
			}

			// add token
			if(*pStr == '"')
			{
				char *pDst;
				pStr++;
				pResult->AddArgument(pStr);

				pDst = pStr; // we might have to process escape data
				while(1)
				{
					if(pStr[0] == '"')
						break;
					else if(pStr[0] == '\\')
					{
						if(pStr[1] == '\\')
							pStr++; // skip due to escape
						else if(pStr[1] == '"')
							pStr++; // skip due to escape
					}
					else if(pStr[0] == 0)
						return 1; // return error

					*pDst = *pStr;
					pDst++;
					pStr++;
				}

				// write null termination
				*pDst = 0;


				pStr++;
			}
			else
			{
				pResult->AddArgument(pStr);

				if(Command == 'r') // rest of the string
					break;
				else if(Command == 'i') // validate int
					pStr = str_skip_to_whitespace(pStr);
				else if(Command == 'f') // validate float
					pStr = str_skip_to_whitespace(pStr);
				else if(Command == 's') // validate string
					pStr = str_skip_to_whitespace(pStr);

				if(pStr[0] != 0) // check for end of string
				{
					pStr[0] = 0;
					pStr++;
				}
			}
		}
	}

	return Error;
}

void CConsole::RegisterPrintCallback(FPrintCallback pfnPrintCallback, void *pUserData)
{
	m_pfnPrintCallback = pfnPrintCallback;
	m_pPrintCallbackUserdata = pUserData;
}

void CConsole::Print(int Level, const char *pFrom, const char *pStr)
{
	dbg_msg(pFrom ,"%s", pStr);
	if(Level <= g_Config.m_ConsoleOutputLevel && m_pfnPrintCallback)
	{
		char aBuf[1024];
		str_format(aBuf, sizeof(aBuf), "[%s]: %s", pFrom, pStr);
		m_pfnPrintCallback(aBuf, m_pPrintCallbackUserdata);
	}
}

bool CConsole::LineIsValid(const char *pStr)
{
	if(!pStr || *pStr == 0)
		return false;

	do
	{
		CResult Result;
		const char *pEnd = pStr;
		const char *pNextPart = 0;
		int InString = 0;

		while(*pEnd)
		{
			if(*pEnd == '"')
				InString ^= 1;
			else if(*pEnd == '\\') // escape sequences
			{
				if(pEnd[1] == '"')
					pEnd++;
			}
			else if(!InString)
			{
				if(*pEnd == ';') // command separator
				{
					pNextPart = pEnd+1;
					break;
				}
				else if(*pEnd == '#') // comment, no need to do anything more
					break;
			}

			pEnd++;
		}

		if(ParseStart(&Result, pStr, (pEnd-pStr) + 1) != 0)
			return false;

		CCommand *pCommand = FindCommand(Result.m_pCommand, m_FlagMask);
		if(!pCommand || ParseArgs(&Result, pCommand->m_pParams))
			return false;

		pStr = pNextPart;
	}
	while(pStr && *pStr);

	return true;
}

void CConsole::ExecuteLineStroked(int Stroke, const char *pStr)
{
	while(pStr && *pStr)
	{
		CResult Result;
		const char *pEnd = pStr;
		const char *pNextPart = 0;
		int InString = 0;

		while(*pEnd)
		{
			if(*pEnd == '"')
				InString ^= 1;
			else if(*pEnd == '\\') // escape sequences
			{
				if(pEnd[1] == '"')
					pEnd++;
			}
			else if(!InString)
			{
				if(*pEnd == ';') // command separator
				{
					pNextPart = pEnd+1;
					break;
				}
				else if(*pEnd == '#') // comment, no need to do anything more
					break;
			}

			pEnd++;
		}

		if(ParseStart(&Result, pStr, (pEnd-pStr) + 1) != 0)
			return;

		CCommand *pCommand = FindCommand(Result.m_pCommand, m_FlagMask);

		if(pCommand)
		{
			int IsStrokeCommand = 0;
			if(Result.m_pCommand[0] == '+')
			{
				// insert the stroke direction token
				Result.AddArgument(m_paStrokeStr[Stroke]);
				IsStrokeCommand = 1;
			}

			if(Stroke || IsStrokeCommand)
			{
				if(ParseArgs(&Result, pCommand->m_pParams))
				{
					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "Invalid arguments... Usage: %s %s", pCommand->m_pName, pCommand->m_pParams);
					Print(OUTPUT_LEVEL_STANDARD, "Console", aBuf);
				}
				else if(m_StoreCommands && pCommand->m_Flags&CFGFLAG_STORE)
				{
					m_ExecutionQueue.AddEntry();
					m_ExecutionQueue.m_pLast->m_pfnCommandCallback = pCommand->m_pfnCallback;
					m_ExecutionQueue.m_pLast->m_pCommandUserData = pCommand->m_pUserData;
					m_ExecutionQueue.m_pLast->m_Result = Result;
				}
				else
					pCommand->m_pfnCallback(&Result, pCommand->m_pUserData);
			}
		}
		else if(Stroke)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "No such command: %s.", Result.m_pCommand);
			Print(OUTPUT_LEVEL_STANDARD, "Console", aBuf);
		}

		pStr = pNextPart;
	}
}

void CConsole::PossibleCommands(const char *pStr, int FlagMask, FPossibleCallback pfnCallback, void *pUser)
{
	CCommand *pCommand;
	for(pCommand = m_pFirstCommand; pCommand; pCommand = pCommand->m_pNext)
	{
		if(pCommand->m_Flags&FlagMask)
		{
			if(str_find_nocase(pCommand->m_pName, pStr))
				pfnCallback(pCommand->m_pName, pUser);
		}
	}
}

CConsole::CCommand *CConsole::FindCommand(const char *pName, int FlagMask)
{
	CCommand *pCommand;
	for (pCommand = m_pFirstCommand; pCommand; pCommand = pCommand->m_pNext)
	{
		if(pCommand->m_Flags&FlagMask)
		{
			if(str_comp_nocase(pCommand->m_pName, pName) == 0)
				return pCommand;
		}
	}

	return 0x0;
}

void CConsole::ExecuteLine(const char *pStr)
{
	CConsole::ExecuteLineStroked(1, pStr); // press it
	CConsole::ExecuteLineStroked(0, pStr); // then release it
}


void CConsole::ExecuteFile(const char *pFilename)
{
	// make sure that this isn't being executed already
	for(CExecFile *pCur = m_pFirstExec; pCur; pCur = pCur->m_pPrev)
		if(str_comp(pFilename, pCur->m_pFilename) == 0)
			return;

	if(!m_pStorage)
		m_pStorage = Kernel()->RequestInterface<IStorage>();
	if(!m_pStorage)
		return;

	// push this one to the stack
	CExecFile ThisFile;
	CExecFile *pPrev = m_pFirstExec;
	ThisFile.m_pFilename = pFilename;
	ThisFile.m_pPrev = m_pFirstExec;
	m_pFirstExec = &ThisFile;

	// exec the file
	IOHANDLE File = m_pStorage->OpenFile(pFilename, IOFLAG_READ, IStorage::TYPE_ALL);

	char aBuf[256];
	if(File)
	{
		char *pLine;
		CLineReader lr;

		str_format(aBuf, sizeof(aBuf), "executing '%s'", pFilename);
		Print(IConsole::OUTPUT_LEVEL_STANDARD, "console", aBuf);
		lr.Init(File);

		while((pLine = lr.Get()))
			ExecuteLine(pLine);

		io_close(File);
	}
	else
	{
		str_format(aBuf, sizeof(aBuf), "failed to open '%s'", pFilename);
		Print(IConsole::OUTPUT_LEVEL_STANDARD, "console", aBuf);
	}

	m_pFirstExec = pPrev;
}

void CConsole::Con_Echo(IResult *pResult, void *pUserData)
{
	((CConsole*)pUserData)->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Console", pResult->GetString(0));
}

void CConsole::Con_Exec(IResult *pResult, void *pUserData)
{
	((CConsole*)pUserData)->ExecuteFile(pResult->GetString(0));
}

struct CIntVariableData
{
	IConsole *m_pConsole;
	int *m_pVariable;
	int m_Min;
	int m_Max;
};

struct CStrVariableData
{
	IConsole *m_pConsole;
	char *m_pStr;
	int m_MaxSize;
};

static void IntVariableCommand(IConsole::IResult *pResult, void *pUserData)
{
	CIntVariableData *pData = (CIntVariableData *)pUserData;

	if(pResult->NumArguments())
	{
		int Val = pResult->GetInteger(0);

		// do clamping
		if(pData->m_Min != pData->m_Max)
		{
			if (Val < pData->m_Min)
				Val = pData->m_Min;
			if (pData->m_Max != 0 && Val > pData->m_Max)
				Val = pData->m_Max;
		}

		*(pData->m_pVariable) = Val;
	}
	else
	{
		char aBuf[1024];
		str_format(aBuf, sizeof(aBuf), "Value: %d", *(pData->m_pVariable));
		pData->m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Console", aBuf);
	}
}

static void StrVariableCommand(IConsole::IResult *pResult, void *pUserData)
{
	CStrVariableData *pData = (CStrVariableData *)pUserData;

	if(pResult->NumArguments())
	{
		const char *pString = pResult->GetString(0);
		if(!str_utf8_check(pString))
		{
			char Temp[4];
			int Length = 0;
			while(*pString)
			{
				int Size = str_utf8_encode(Temp, static_cast<const unsigned char>(*pString++));
				if(Length+Size < pData->m_MaxSize)
				{
					mem_copy(pData->m_pStr+Length, &Temp, Size);
					Length += Size;
				}
				else
					break;
			}
			pData->m_pStr[Length] = 0;
		}
		else
			str_copy(pData->m_pStr, pString, pData->m_MaxSize);
	}
	else
	{
		char aBuf[1024];
		str_format(aBuf, sizeof(aBuf), "Value: %s", pData->m_pStr);
		pData->m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Console", aBuf);
	}
}

CConsole::CConsole(int FlagMask)
{
	m_FlagMask = FlagMask;
	m_StoreCommands = true;
	m_paStrokeStr[0] = "0";
	m_paStrokeStr[1] = "1";
	m_ExecutionQueue.Reset();
	m_pFirstCommand = 0;
	m_pFirstExec = 0;
	m_pPrintCallbackUserdata = 0;
	m_pfnPrintCallback = 0;

	m_pStorage = 0;

	// register some basic commands
	Register("echo", "r", CFGFLAG_SERVER|CFGFLAG_CLIENT, Con_Echo, this, "Echo the text");
	Register("exec", "r", CFGFLAG_SERVER|CFGFLAG_CLIENT, Con_Exec, this, "Execute the specified file");

	// TODO: this should disappear
	#define MACRO_CONFIG_INT(Name,ScriptName,Def,Min,Max,Flags,Desc) \
	{ \
		static CIntVariableData Data = { this, &g_Config.m_##Name, Min, Max }; \
		Register(#ScriptName, "?i", Flags, IntVariableCommand, &Data, Desc); \
	}

	#define MACRO_CONFIG_STR(Name,ScriptName,Len,Def,Flags,Desc) \
	{ \
		static CStrVariableData Data = { this, g_Config.m_##Name, Len }; \
		Register(#ScriptName, "?r", Flags, StrVariableCommand, &Data, Desc); \
	}

	#include "config_variables.h"

	#undef MACRO_CONFIG_INT
	#undef MACRO_CONFIG_STR
}

void CConsole::ParseArguments(int NumArgs, const char **ppArguments)
{
	for(int i = 0; i < NumArgs; i++)
	{
		// check for scripts to execute
		if(ppArguments[i][0] == '-' && ppArguments[i][1] == 'f' && ppArguments[i][2] == 0)
		{
			if(NumArgs - i > 1)
				ExecuteFile(ppArguments[i+1]);
			i++;
		}
		else if(!str_comp("-s", ppArguments[i]) || !str_comp("--silent", ppArguments[i]))
		{
			// skip silent param
			continue;
		}
		else
		{
			// search arguments for overrides
			ExecuteLine(ppArguments[i]);
		}
	}
}

void CConsole::Register(const char *pName, const char *pParams,
	int Flags, FCommandCallback pfnFunc, void *pUser, const char *pHelp)
{
	CCommand *pCommand = (CCommand *)mem_alloc(sizeof(CCommand), sizeof(void*));
	pCommand->m_pfnCallback = pfnFunc;
	pCommand->m_pUserData = pUser;
	pCommand->m_pHelp = pHelp;
	pCommand->m_pName = pName;
	pCommand->m_pParams = pParams;
	pCommand->m_Flags = Flags;


	pCommand->m_pNext = m_pFirstCommand;
	m_pFirstCommand = pCommand;
}

void CConsole::Con_Chain(IResult *pResult, void *pUserData)
{
	CChain *pInfo = (CChain *)pUserData;
	pInfo->m_pfnChainCallback(pResult, pInfo->m_pUserData, pInfo->m_pfnCallback, pInfo->m_pCallbackUserData);
}

void CConsole::Chain(const char *pName, FChainCommandCallback pfnChainFunc, void *pUser)
{
	CCommand *pCommand = FindCommand(pName, m_FlagMask);

	if(!pCommand)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "failed to chain '%s'", pName);
		Print(IConsole::OUTPUT_LEVEL_DEBUG, "console", aBuf);
		return;
	}

	CChain *pChainInfo = (CChain *)mem_alloc(sizeof(CChain), sizeof(void*));

	// store info
	pChainInfo->m_pfnChainCallback = pfnChainFunc;
	pChainInfo->m_pUserData = pUser;
	pChainInfo->m_pfnCallback = pCommand->m_pfnCallback;
	pChainInfo->m_pCallbackUserData = pCommand->m_pUserData;

	// chain
	pCommand->m_pfnCallback = Con_Chain;
	pCommand->m_pUserData = pChainInfo;
}

void CConsole::StoreCommands(bool Store)
{
	if(!Store)
	{
		for(CExecutionQueue::CQueueEntry *pEntry = m_ExecutionQueue.m_pFirst; pEntry; pEntry = pEntry->m_pNext)
			pEntry->m_pfnCommandCallback(&pEntry->m_Result, pEntry->m_pCommandUserData);
		m_ExecutionQueue.Reset();
	}
	m_StoreCommands = Store;
}


IConsole::CCommandInfo *CConsole::GetCommandInfo(const char *pName, int FlagMask)
{
	return FindCommand(pName, FlagMask);
}


extern IConsole *CreateConsole(int FlagMask) { return new CConsole(FlagMask); }
