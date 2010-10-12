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

void CConsole::RegisterAlternativePrintCallback(FPrintCallback pfnAlternativePrintCallback, void *pAlternativeUserData)
{
  while (m_pfnAlternativePrintCallback != pfnAlternativePrintCallback && m_PrintUsed)
    ; // wait for other threads to finish their commands, TODO: implement this with LOCK

	m_pfnAlternativePrintCallback = pfnAlternativePrintCallback;
	m_pAlternativePrintCallbackUserdata = pAlternativeUserData;

  m_PrintUsed++;
}

void CConsole::ReleaseAlternativePrintCallback()
{
  m_PrintUsed--;
}


void CConsole::Print(int Level, const char *pFrom, const char *pStr)
{
	dbg_msg(pFrom ,"%s", pStr);
	if (Level <= g_Config.m_ConsoleOutputLevel && m_pfnPrintCallback)
	{
		char aBuf[1024];
		str_format(aBuf, sizeof(aBuf), "[%s]: %s", pFrom, pStr);
		if (!m_pfnAlternativePrintCallback || m_PrintUsed == 0)
  		m_pfnPrintCallback(aBuf, m_pPrintCallbackUserdata);
  	else
  	  m_pfnAlternativePrintCallback(aBuf, m_pAlternativePrintCallbackUserdata);
	}
}

void CConsole::RegisterPrintResponseCallback(FPrintCallback pfnPrintResponseCallback, void *pUserData)
{
	m_pfnPrintResponseCallback = pfnPrintResponseCallback;
	m_pPrintResponseCallbackUserdata = pUserData;
}

void CConsole::RegisterAlternativePrintResponseCallback(FPrintCallback pfnAlternativePrintResponseCallback, void *pAlternativeUserData)
{
  while (m_pfnAlternativePrintResponseCallback != pfnAlternativePrintResponseCallback && m_PrintResponseUsed)
    ; // wait for other threads to finish their commands, TODO: implement this with LOCK

	m_pfnAlternativePrintResponseCallback = pfnAlternativePrintResponseCallback;
	m_pAlternativePrintResponseCallbackUserdata = pAlternativeUserData;

  m_PrintResponseUsed++;
}

void CConsole::ReleaseAlternativePrintResponseCallback()
{
  m_PrintResponseUsed--;
}


void CConsole::PrintResponse(int Level, const char *pFrom, const char *pStr)
{
	dbg_msg(pFrom ,"%s", pStr);
	if (Level <= g_Config.m_ConsoleOutputLevel && m_pfnPrintResponseCallback)
	{
		char aBuf[1024];
		str_format(aBuf, sizeof(aBuf), "[%s]: %s", pFrom, pStr);
		if (!m_pfnAlternativePrintResponseCallback || m_PrintResponseUsed == 0)
  		m_pfnPrintResponseCallback(aBuf, m_pPrintResponseCallbackUserdata);
  	else
  	  m_pfnAlternativePrintResponseCallback(aBuf, m_pAlternativePrintResponseCallbackUserdata);
	}
}

void CConsole::ExecuteLineStroked(int Stroke, const char *pStr, const int ClientLevel, const int ClientId,
	  FPrintCallback pfnAlternativePrintCallback, void *pUserData,
	  FPrintCallback pfnAlternativePrintResponseCallback, void *pResponseUserData)
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
          RegisterAlternativePrintResponseCallback(pfnAlternativePrintResponseCallback, pResponseUserData);

					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "Invalid arguments... Usage: %s %s", pCommand->m_pName, pCommand->m_pParams);
					PrintResponse(OUTPUT_LEVEL_STANDARD, "Console", aBuf);

      		ReleaseAlternativePrintResponseCallback();
				}
				else if(m_StoreCommands && pCommand->m_Flags&CFGFLAG_STORE)
				{
					m_ExecutionQueue.m_pLast->m_pfnCommandCallback = pCommand->m_pfnCallback;
					m_ExecutionQueue.m_pLast->m_pCommandUserData = pCommand->m_pUserData;
					m_ExecutionQueue.AddEntry();
				}
				else if(pCommand->m_Level <= ClientLevel)
				{
          RegisterAlternativePrintCallback(pfnAlternativePrintCallback, pUserData);
          RegisterAlternativePrintResponseCallback(pfnAlternativePrintResponseCallback, pResponseUserData);

					pCommand->m_pfnCallback(pResult, pCommand->m_pUserData, ClientId);

          ReleaseAlternativePrintResponseCallback();
      		ReleaseAlternativePrintCallback();
				}
				else
				{
          RegisterAlternativePrintResponseCallback(pfnAlternativePrintResponseCallback, pResponseUserData);

					char aBuf[256];
					if (pCommand->m_Level == 100 && ClientLevel < 100)
					{
						str_format(aBuf, sizeof(aBuf), "You can't use this command: %s", pCommand->m_pName);
					}
					else
					{
						str_format(aBuf, sizeof(aBuf), "You have low level to use command: %s. Your level: %d. Need level: %d", pCommand->m_pName, ClientLevel, pCommand->m_Level);
					}
					PrintResponse(OUTPUT_LEVEL_STANDARD, "Console", aBuf);

      		ReleaseAlternativePrintResponseCallback();
				}
			}
		}
		else if(Stroke)
		{
      RegisterAlternativePrintResponseCallback(pfnAlternativePrintResponseCallback, pResponseUserData);

			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "No such command: %s.", pResult->m_pCommand);
			PrintResponse(OUTPUT_LEVEL_STANDARD, "Console", aBuf);

  		ReleaseAlternativePrintResponseCallback();
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

void CConsole::ExecuteLine(const char *pStr, const int ClientLevel, const int ClientId,
	  FPrintCallback pfnAlternativePrintCallback, void *pUserData,
	  FPrintCallback pfnAlternativePrintResponseCallback, void *pResponseUserData)
{
	CConsole::ExecuteLineStroked(1, pStr, ClientLevel, ClientId,
	  pfnAlternativePrintCallback, pUserData, pfnAlternativePrintResponseCallback, pResponseUserData); // press it
	CConsole::ExecuteLineStroked(0, pStr, ClientLevel, ClientId,
	  pfnAlternativePrintCallback, pUserData, pfnAlternativePrintResponseCallback, pResponseUserData); // then release it
}


void CConsole::ExecuteFile(const char *pFilename,
	  FPrintCallback pfnAlternativePrintCallback, void *pUserData,
	  FPrintCallback pfnAlternativePrintResponseCallback, void *pResponseUserData)
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
		
		RegisterAlternativePrintCallback(pfnAlternativePrintCallback, pUserData);

		str_format(aBuf, sizeof(aBuf), "executing '%s'", pFilename);
		PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "console", aBuf);
		lr.Init(File);
		ReleaseAlternativePrintCallback();

		while((pLine = lr.Get()))
			ExecuteLine(pLine, 4, -1, pfnAlternativePrintCallback, pUserData, pfnAlternativePrintResponseCallback, pResponseUserData);

		io_close(File);
	}
	else
	{
		RegisterAlternativePrintCallback(pfnAlternativePrintCallback, pUserData);

		str_format(aBuf, sizeof(aBuf), "failed to open '%s'", pFilename);
		PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "console", aBuf);

		ReleaseAlternativePrintCallback();
	}
	
	m_pFirstExec = pPrev;
}

void CConsole::Con_Echo(IResult *pResult, void *pUserData, int ClientId)
{
	((CConsole*)pUserData)->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Console", pResult->GetString(0));
}

void CConsole::Con_Exec(IResult *pResult, void *pUserData, int ClientId)
{
	CConsole *pSelf = (CConsole *)pUserData;
  
	pSelf->ExecuteFile(pResult->GetString(0), pSelf->m_pfnAlternativePrintCallback, pSelf->m_pAlternativePrintCallbackUserdata, 
			pSelf->m_pfnAlternativePrintResponseCallback, pSelf->m_pAlternativePrintResponseCallbackUserdata);
}

struct CIntVariableData
{
	IConsole *m_pConsole;
	char *m_Name;
	int *m_pVariable;
	int m_Min;
	int m_Max;
};

struct CStrVariableData
{
	IConsole *m_pConsole;
	char *m_Name;
	char *m_pStr;
	int m_MaxSize;
};

static void IntVariableCommand(IConsole::IResult *pResult, void *pUserData, int ClientId)
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

		char aBuf[1024];
		str_format(aBuf, sizeof(aBuf), "%s changed to %d", pData->m_Name, *(pData->m_pVariable));
		pData->m_pConsole->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "Console", aBuf);
	}
	else
	{
		char aBuf[1024];
		str_format(aBuf, sizeof(aBuf), "%s is %d", pData->m_Name, *(pData->m_pVariable));
		pData->m_pConsole->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "Console", aBuf);
	}
}

static void StrVariableCommand(IConsole::IResult *pResult, void *pUserData, int ClientId)
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

		char aBuf[1024];
		str_format(aBuf, sizeof(aBuf), "%s changed to '%s'", pData->m_Name, pData->m_pStr);
		pData->m_pConsole->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "Console", aBuf);
  }
	else
	{
		char aBuf[1024];
		str_format(aBuf, sizeof(aBuf), "%s is '%s'", pData->m_Name, pData->m_pStr);
		pData->m_pConsole->PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "Console", aBuf);
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
	m_pAlternativePrintCallbackUserdata = 0;
	m_pfnAlternativePrintCallback = 0;
	m_PrintUsed = 0;

	m_pPrintResponseCallbackUserdata = 0;
	m_pfnPrintResponseCallback = 0;
	m_pAlternativePrintResponseCallbackUserdata = 0;
	m_pfnAlternativePrintResponseCallback = 0;
	m_PrintResponseUsed = 0;
	
	m_pStorage = 0;
	
	// register some basic commands
	Register("echo", "r", CFGFLAG_SERVER|CFGFLAG_CLIENT, Con_Echo, this, "Echo the text", 3);
	Register("exec", "r", CFGFLAG_SERVER|CFGFLAG_CLIENT, Con_Exec, this, "Execute the specified file", 3);
	
	// TODO: this should disappear
	#define MACRO_CONFIG_INT(Name,ScriptName,Def,Min,Max,Flags,Desc,Level) \
	{ \
		static CIntVariableData Data = { this, #ScriptName, &g_Config.m_##Name, Min, Max }; \
		Register(#ScriptName, "?i", Flags, IntVariableCommand, &Data, Desc, Level); \
	}
	
	#define MACRO_CONFIG_STR(Name,ScriptName,Len,Def,Flags,Desc,Level) \
	{ \
		static CStrVariableData Data = { this, #ScriptName, g_Config.m_##Name, Len }; \
		Register(#ScriptName, "?r", Flags, StrVariableCommand, &Data, Desc, Level); \
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
			ExecuteLine(ppArguments[i], 100, -1);
		}
	}
}

void CConsole::Register(const char *pName, const char *pParams, 
	int Flags, FCommandCallback pfnFunc, void *pUser, const char *pHelp, const int Level)
{
	CCommand *pCommand = (CCommand *)mem_alloc(sizeof(CCommand), sizeof(void*));
	pCommand->m_pfnCallback = pfnFunc;
	pCommand->m_pUserData = pUser;
	pCommand->m_pHelp = pHelp;
	pCommand->m_pName = pName;
	pCommand->m_pParams = pParams;
	pCommand->m_Flags = Flags;
	pCommand->m_Level = Level;
	
	pCommand->m_pNext = m_pFirstCommand;
	m_pFirstCommand = pCommand;
}

void CConsole::List(const int Level, int Flags)
{
	CCommand *pCommand = m_pFirstCommand;
	while(pCommand)
	{
		if(pCommand)
			if((pCommand->m_Level <= Level))
					if((!Flags)?true:pCommand->m_Flags&Flags)
					{
						char aBuf[128];
						str_format(aBuf,sizeof(aBuf),"Name: %s, Parameters: %s, Help: %s",pCommand->m_pName, (!str_length(pCommand->m_pParams))?"None.":pCommand->m_pParams, (!str_length(pCommand->m_pHelp))?"No Help String Given":pCommand->m_pHelp);
						PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "Console", aBuf);
					}
		pCommand = pCommand->m_pNext;
	}
}

void CConsole::Con_Chain(IResult *pResult, void *pUserData, int ClientId)
{
	CChain *pInfo = (CChain *)pUserData;
	pInfo->m_pfnChainCallback(pResult, pInfo->m_pUserData, pInfo->m_pfnCallback, pInfo->m_pCallbackUserData);
}

void CConsole::Chain(const char *pName, FChainCommandCallback pfnChainFunc, void *pUser)
{
	CCommand *pCommand = FindCommand(pName, m_FlagMask);
	
	if(!pCommand)
	{
		RegisterAlternativePrintCallback(0, 0);
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "failed to chain '%s'", pName);
		Print(IConsole::OUTPUT_LEVEL_DEBUG, "console", aBuf);
		ReleaseAlternativePrintCallback();
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

void CConsole::StoreCommands(bool Store, int ClientId)
{
	if(!Store)
	{
		for(CExecutionQueue::CQueueEntry *pEntry = m_ExecutionQueue.m_pFirst; pEntry != m_ExecutionQueue.m_pLast; pEntry = pEntry->m_pNext)
			pEntry->m_pfnCommandCallback(&pEntry->m_Result, pEntry->m_pCommandUserData, ClientId);
		m_ExecutionQueue.Reset();
	}
	m_StoreCommands = Store;
}


IConsole::CCommandInfo *CConsole::GetCommandInfo(const char *pName, int FlagMask)
{
	return FindCommand(pName, FlagMask);
}


extern IConsole *CreateConsole(int FlagMask) { return new CConsole(FlagMask); }
