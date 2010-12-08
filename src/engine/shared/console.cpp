/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <new>
#include <base/system.h>
#include <base/math.h>
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

int CConsole::CResult::GetVictim()
{
	return m_Victim;
}

void CConsole::CResult::ResetVictim()
{
	m_Victim = VICTIM_NONE;
}

bool CConsole::CResult::HasVictim()
{
	return m_Victim != VICTIM_NONE;
}

void CConsole::CResult::SetVictim(int Victim)
{
	m_Victim = clamp<int>(Victim, 0, MAX_CLIENTS);
}

void CConsole::CResult::SetVictim(const char *pVictim)
{
	if(!str_comp(pVictim, "me"))
		m_Victim = VICTIM_ME;
	else if(!str_comp(pVictim, "all"))
		m_Victim = VICTIM_ALL;
	else
		m_Victim = clamp<int>(str_toint(pVictim), 0, MAX_CLIENTS - 1);
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

	pResult->ResetVictim();
	
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
				char* pVictim = 0;
				
				if (Command != 'v')
					pResult->AddArgument(pStr);
				else
					pVictim = pStr;

				if(Command == 'r') // rest of the string
					break;
				else if(Command == 'v')
					pStr = str_skip_to_whitespace(pStr);
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
				
				if (pVictim)
					pResult->SetVictim(pVictim);
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

void CConsole::RegisterClientOnlineCallback(FClientOnlineCallback pfnCallback, void *pUserData)
{
	m_pfnClientOnlineCallback = pfnCallback;
	m_pClientOnlineUserdata = pUserData;
}

void CConsole::RegisterCompareClientsCallback(FCompareClientsCallback pfnCallback, void *pUserData)
{
	m_pfnCompareClientsCallback = pfnCallback;
	m_pCompareClientsUserdata = pUserData;
}

bool CConsole::ClientOnline(int ClientId)
{
	if(!m_pfnClientOnlineCallback)
		return true;
	
	return m_pfnClientOnlineCallback(ClientId, m_pClientOnlineUserdata);
}

bool CConsole::CompareClients(int ClientId, int Victim)
{
	if(!m_pfnCompareClientsCallback)
		return true;
	
	return m_pfnCompareClientsCallback(ClientId, Victim, m_pCompareClientsUserdata);	
}

void CConsole::Print(int Level, const char *pFrom, const char *pStr)
{
	dbg_msg(pFrom ,"%s", pStr);
	if(Level <= g_Config.m_ConsoleOutputLevel && m_pfnPrintCallback)
	{
		char aBuf[1024];
		str_format(aBuf, sizeof(aBuf), "[%s]: %s", pFrom, pStr);
		if (!m_pfnAlternativePrintCallback || m_PrintUsed == 0)
  		m_pfnPrintCallback(aBuf, m_pPrintCallbackUserdata);
  	else
  	  m_pfnAlternativePrintCallback(aBuf, m_pAlternativePrintCallbackUserdata);
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
	while(pStr && *pStr)
	{
		CResult *pResult = new(&m_ExecutionQueue.m_pLast->m_Result) CResult;
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
				else 
				{
					if(pResult->GetVictim() == CResult::VICTIM_ME)
						pResult->SetVictim(ClientId);
					
					if((ClientLevel < pCommand->m_Level && !(pCommand->m_Flags & CMDFLAG_HELPERCMD)) || (ClientLevel < 1 && (pCommand->m_Flags & CMDFLAG_HELPERCMD)))
					{
						RegisterAlternativePrintResponseCallback(pfnAlternativePrintResponseCallback, pResponseUserData);

						char aBuf[256];
						if (pCommand->m_Level == 100 && ClientLevel < 100)
						{
							str_format(aBuf, sizeof(aBuf), "You can't use this command: %s", pCommand->m_pName);
						}
						else
						{
							str_format(aBuf, sizeof(aBuf), "You have too low level to use command: %s. Your level: %d. Need level: %d", pCommand->m_pName, ClientLevel, pCommand->m_Level);
							dbg_msg("server", "client tried rcon command ('%s') without permisson. ClientId=%d ", pCommand->m_pName, ClientId);
						}
						PrintResponse(OUTPUT_LEVEL_STANDARD, "Console", aBuf);

						ReleaseAlternativePrintResponseCallback();
					}					
					else if(ClientLevel == 1 && (pCommand->m_Flags & CMDFLAG_HELPERCMD) && pResult->GetVictim() != ClientId)
					{
						RegisterAlternativePrintResponseCallback(pfnAlternativePrintResponseCallback, pResponseUserData);
						PrintResponse(OUTPUT_LEVEL_STANDARD, "Console", "As a helper you can't use commands on others.");
						dbg_msg("server", "helper tried rcon command ('%s') on others without permission. ClientId=%d", pCommand->m_pName, ClientId);
						ReleaseAlternativePrintResponseCallback();
					}
					else if(!g_Config.m_SvCheats && (pCommand->m_Flags & CMDFLAG_CHEAT))
					{
						RegisterAlternativePrintResponseCallback(pfnAlternativePrintResponseCallback, pResponseUserData);
						PrintResponse(OUTPUT_LEVEL_STANDARD, "Console", "Cheats are not available on this server");
						dbg_msg("server", "client tried rcon cheat ('%s') with cheats off. ClientId=%d", pCommand->m_pName, ClientId);
						ReleaseAlternativePrintResponseCallback();
					}
					else if(!g_Config.m_SvTimer && (pCommand->m_Flags & CMDFLAG_TIMER))
					{
						RegisterAlternativePrintResponseCallback(pfnAlternativePrintResponseCallback, pResponseUserData);
						PrintResponse(OUTPUT_LEVEL_STANDARD, "Console", "Timer commands are not available on this server");
						dbg_msg("server", "client tried timer command ('%s') with timer commands off. ClientId=%d", pCommand->m_pName, ClientId);
						ReleaseAlternativePrintResponseCallback();
					}
					else
					{
						if (pResult->HasVictim())
						{
							if(pResult->GetVictim() == CResult::VICTIM_ALL)
							{
								for (int i = 0; i < MAX_CLIENTS; i++)
								{
									if (ClientOnline(i) && CompareClients(ClientLevel, i))
									{
										pResult->SetVictim(i);
										RegisterAlternativePrintCallback(pfnAlternativePrintCallback, pUserData);
										RegisterAlternativePrintResponseCallback(pfnAlternativePrintResponseCallback, pResponseUserData);

										pCommand->m_pfnCallback(pResult, pCommand->m_pUserData, ClientId);

										ReleaseAlternativePrintResponseCallback();
										ReleaseAlternativePrintCallback();
									}
								}
							}
							else
							{
								if (!ClientOnline(pResult->GetVictim()))
								{
									RegisterAlternativePrintResponseCallback(pfnAlternativePrintResponseCallback, pResponseUserData);
									PrintResponse(OUTPUT_LEVEL_STANDARD, "Console", "client is offline");
									ReleaseAlternativePrintResponseCallback();
								}
								else if (!CompareClients(ClientLevel, pResult->GetVictim()) && ClientId != pResult->GetVictim())
								{
									RegisterAlternativePrintResponseCallback(pfnAlternativePrintResponseCallback, pResponseUserData);
									PrintResponse(OUTPUT_LEVEL_STANDARD, "Console", "you can not use commands on players with the same or higher level than you");
									ReleaseAlternativePrintResponseCallback();
								}
								else
								{
									RegisterAlternativePrintCallback(pfnAlternativePrintCallback, pUserData);
									RegisterAlternativePrintResponseCallback(pfnAlternativePrintResponseCallback, pResponseUserData);

									pCommand->m_pfnCallback(pResult, pCommand->m_pUserData, ClientId);

									ReleaseAlternativePrintResponseCallback();
									ReleaseAlternativePrintCallback();
								}
							}
						}
						else
						{
							RegisterAlternativePrintCallback(pfnAlternativePrintCallback, pUserData);
							RegisterAlternativePrintResponseCallback(pfnAlternativePrintResponseCallback, pResponseUserData);

							pCommand->m_pfnCallback(pResult, pCommand->m_pUserData, ClientId);

							ReleaseAlternativePrintResponseCallback();
							ReleaseAlternativePrintCallback();
						}
					}
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

void CConsole::ExecuteLine(const char *pStr, const int ClientLevel, const int ClientId, FPrintCallback pfnAlternativePrintCallback, void *pUserData, FPrintCallback pfnAlternativePrintResponseCallback, void *pResponseUserData)
{
	CConsole::ExecuteLineStroked(1, pStr, ClientLevel, ClientId, pfnAlternativePrintCallback, pUserData, pfnAlternativePrintResponseCallback, pResponseUserData); // press it
	CConsole::ExecuteLineStroked(0, pStr, ClientLevel, ClientId, pfnAlternativePrintCallback, pUserData, pfnAlternativePrintResponseCallback, pResponseUserData); // then release it
}


void CConsole::ExecuteFile(const char *pFilename, FPrintCallback pfnAlternativePrintCallback, void *pUserData, FPrintCallback pfnAlternativePrintResponseCallback, void *pResponseUserData, int Level)
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
			ExecuteLine(pLine, Level, -1, pfnAlternativePrintCallback, pUserData, pfnAlternativePrintResponseCallback, pResponseUserData);

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

	pSelf->ExecuteFile(pResult->GetString(0), pSelf->m_pfnAlternativePrintCallback, pSelf->m_pAlternativePrintCallbackUserdata, pSelf->m_pfnAlternativePrintResponseCallback, pSelf->m_pAlternativePrintResponseCallbackUserdata);
}

struct CIntVariableData
{
	IConsole *m_pConsole;
	const char *m_Name;
	int *m_pVariable;
	int m_Min;
	int m_Max;
};

struct CStrVariableData
{
	IConsole *m_pConsole;
	const char *m_Name;
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

	for (int i = 0; i < 5; ++i)
	{
		m_aCommandCount[i] = 0;
	}

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
	
	m_pfnClientOnlineCallback = 0;
	m_pfnCompareClientsCallback = 0;
	m_pClientOnlineUserdata = 0;
	m_pCompareClientsUserdata = 0;
	
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
		if(ppArguments[i][0] == '-' && ppArguments[i][1] == 'f' && ppArguments[i][2] == 0)
		{
			if(NumArgs - i > 1)
				ExecuteFile(ppArguments[i+1], 0, 0, 0, 0, 4);
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
	m_aCommandCount[pCommand->m_Level]++;
}

void CConsole::List(const int Level, int Flags)
{
	switch(Level)
	{
		case 4: PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "console", "command cmdlist is not allowed for config files"); return;
		case 3: PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "console", "=== cmdlist for admins ==="); break;
		case 2: PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "console", "=== cmdlist for mods ==="); break;
		case 1: PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "console", "=== cmdlist for helpers ==="); break;
		default: PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "console", "=== cmdlist ==="); break;
	}
	
	char aBuf[50 + 1] = { 0 };
	CCommand *pCommand = m_pFirstCommand;
	unsigned Length = 0;
	
	while(pCommand)
	{
		if(str_comp_num(pCommand->m_pName, "sv_", 3) && str_comp_num(pCommand->m_pName, "dbg_", 4))	// ignore configs and debug commands
		{
			if((pCommand->m_Flags & Flags) == Flags && (pCommand->m_Level == Level || (Level == 1 && (pCommand->m_Flags & CMDFLAG_HELPERCMD))))
			{
				unsigned CommandLength = str_length(pCommand->m_pName);
				if(Length + CommandLength + 2 >= sizeof(aBuf) || aBuf[0] == 0)
				{
					if(aBuf[0])
						PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "console", aBuf);
					aBuf[0] = 0;
					Length = CommandLength;
					str_copy(aBuf, pCommand->m_pName, sizeof(aBuf));
				}
				else
				{
					str_format(aBuf, sizeof(aBuf), "%s, %s", aBuf, pCommand->m_pName);
					Length += CommandLength + 2;
				}
			}
		}
		pCommand = pCommand->m_pNext;
	}
	
	if (aBuf[0])
		PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "Console", aBuf);

	switch(Level)
	{
		case 3: PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "console", "see 'cmdlist 0,1,2' for more commands, which don't require admin rights"); break;
		case 2: PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "console", "see 'cmdlist 0,1' for more commands, which don't require mod rights"); break;
		case 1: PrintResponse(IConsole::OUTPUT_LEVEL_STANDARD, "console", "see 'cmdlist 0' for more commands, which don't require helper rights"); break;
		default: break;
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
