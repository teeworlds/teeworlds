#include <new>
#include <base/system.h>
#include <engine/shared/protocol.h>
#include <engine/storage.h>
#include "console.h"
#include "config.h"
#include "engine.h"
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
static char *SkipToBlank(char *pStr)
{
	while(*pStr && (*pStr != ' ' && *pStr != '\t' && *pStr != '\n'))
		pStr++;
	return pStr;
}


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
	pStr = SkipToBlank(pStr);
	
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
					pStr = SkipToBlank(pStr);
				else if(Command == 'f') // validate float
					pStr = SkipToBlank(pStr);
				else if(Command == 's') // validate string
					pStr = SkipToBlank(pStr);

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

void CConsole::Print(const char *pStr)
{
	dbg_msg("console" ,"%s", pStr);
	if (m_pfnPrintCallback)
		m_pfnPrintCallback(pStr, m_pPrintCallbackUserdata);
}

void CConsole::ExecuteLineStroked(int Stroke, const char *pStr)
{
	CResult *pResult = new(&m_ExecutionQueue.m_pLast->m_Result) CResult;
	
	while(pStr && *pStr)
	{
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
				if(*pEnd == ';')  // command separator
				{
					pNextPart = pEnd+1;
					break;
				}
				else if(*pEnd == '#')  // comment, no need to do anything more
					break;
			}
			
			pEnd++;
		}
		
		if(ParseStart(pResult, pStr, (pEnd-pStr) + 1) != 0)
			return;

		CCommand *pCommand = FindCommand(pResult->m_pCommand, m_FlagMask);

		if(pCommand)
		{
			int IsStrokeCommand = 0;
			if(pResult->m_pCommand[0] == '+')
			{
				// insert the stroke direction token
				pResult->AddArgument(m_paStrokeStr[Stroke]);
				IsStrokeCommand = 1;
			}
			
			if(Stroke || IsStrokeCommand)
			{
				if(ParseArgs(pResult, pCommand->m_pParams))
				{
					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "Invalid arguments... Usage: %s %s", pCommand->m_pName, pCommand->m_pParams);
					Print(aBuf);
				}
				else if(m_StoreCommands && pCommand->m_Flags&CFGFLAG_STORE)
				{
					m_ExecutionQueue.m_pLast->m_pfnCommandCallback = pCommand->m_pfnCallback;
					m_ExecutionQueue.m_pLast->m_pCommandUserData = pCommand->m_pUserData;
					m_ExecutionQueue.AddEntry();
				}
				else
					pCommand->m_pfnCallback(pResult, pCommand->m_pUserData);
			}
		}
		else if(Stroke)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "No such command: %s.", pResult->m_pCommand);
			Print(aBuf);
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
	IOHANDLE File = m_pStorage->OpenFile(pFilename, IOFLAG_READ);
	
	if(File)
	{
		char *pLine;
		CLineReader lr;
		
		dbg_msg("console", "executing '%s'", pFilename);
		lr.Init(File);

		while((pLine = lr.Get()))
			ExecuteLine(pLine);

		io_close(File);
	}
	else
		dbg_msg("console", "failed to open '%s'", pFilename);
	
	m_pFirstExec = pPrev;
}

void CConsole::Con_Echo(IResult *pResult, void *pUserData)
{
	((CConsole*)pUserData)->Print(pResult->GetString(0));
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
		pData->m_pConsole->Print(aBuf);
	}
}

static void StrVariableCommand(IConsole::IResult *pResult, void *pUserData)
{
	CStrVariableData *pData = (CStrVariableData *)pUserData;

	if(pResult->NumArguments())
		str_copy(pData->m_pStr, pResult->GetString(0), pData->m_MaxSize);
	else
	{
		char aBuf[1024];
		str_format(aBuf, sizeof(aBuf), "Value: %s", pData->m_pStr);
		pData->m_pConsole->Print(aBuf);
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
		if(ppArguments[i][0] == '-' && ppArguments[i][1] == 'f' && ppArguments[i][2] == 0 && NumArgs - i > 1)
		{
			ExecuteFile(ppArguments[i+1]);
			i++;
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
		dbg_msg("console", "failed to chain '%s'", pName);
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
		for(CExecutionQueue::CQueueEntry *pEntry = m_ExecutionQueue.m_pFirst; pEntry != m_ExecutionQueue.m_pLast; pEntry = pEntry->m_pNext)
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
