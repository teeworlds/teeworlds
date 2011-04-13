/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CONSOLE_H
#define ENGINE_CONSOLE_H

#include "kernel.h"

class IConsole : public IInterface
{
	MACRO_INTERFACE("console", 0)
public:

	enum
	{
		OUTPUT_LEVEL_STANDARD=0,
		OUTPUT_LEVEL_ADDINFO,
		OUTPUT_LEVEL_DEBUG
	};

	// TODO: rework this interface to reduce the amount of virtual calls
	class IResult
	{
	protected:
		unsigned m_NumArgs;
	public:
		IResult() { m_NumArgs = 0; }
		virtual ~IResult() {}

		virtual int GetInteger(unsigned Index) = 0;
		virtual float GetFloat(unsigned Index) = 0;
		virtual const char *GetString(unsigned Index) = 0;

		int NumArguments() const { return m_NumArgs; }
	};

	class CCommandInfo
	{
	public:
		const char *m_pName;
		const char *m_pHelp;
		const char *m_pParams;
	};

	typedef void (*FPrintCallback)(const char *pStr, void *pUser);
	typedef void (*FPossibleCallback)(const char *pCmd, void *pUser);
	typedef void (*FCommandCallback)(IResult *pResult, void *pUserData);
	typedef void (*FChainCommandCallback)(IResult *pResult, void *pUserData, FCommandCallback pfnCallback, void *pCallbackUserData);

	virtual CCommandInfo *GetCommandInfo(const char *pName, int FlagMask) = 0;
	virtual void PossibleCommands(const char *pStr, int FlagMask, FPossibleCallback pfnCallback, void *pUser) = 0;
	virtual void ParseArguments(int NumArgs, const char **ppArguments) = 0;

	virtual void Register(const char *pName, const char *pParams,
		int Flags, FCommandCallback pfnFunc, void *pUser, const char *pHelp) = 0;
	virtual void Chain(const char *pName, FChainCommandCallback pfnChainFunc, void *pUser) = 0;
	virtual void StoreCommands(bool Store) = 0;

	virtual bool LineIsValid(const char *pStr) = 0;
	virtual void ExecuteLine(const char *Sptr) = 0;
	virtual void ExecuteLineStroked(int Stroke, const char *pStr) = 0;
	virtual void ExecuteFile(const char *pFilename) = 0;

	virtual void RegisterPrintCallback(FPrintCallback pfnPrintCallback, void *pUserData) = 0;
	virtual void Print(int Level, const char *pFrom, const char *pStr) = 0;
};

extern IConsole *CreateConsole(int FlagMask);

#endif // FILE_ENGINE_CONSOLE_H
