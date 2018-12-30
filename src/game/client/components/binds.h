/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_BINDS_H
#define GAME_CLIENT_COMPONENTS_BINDS_H
#include <game/client/component.h>
#include <engine/keys.h>

class CBinds : public CComponent
{
	int DecodeBindString(const char *pKeyName, int* pModifier);

	static void ConBind(IConsole::IResult *pResult, void *pUserData);
	static void ConUnbind(IConsole::IResult *pResult, void *pUserData);
	static void ConUnbindAll(IConsole::IResult *pResult, void *pUserData);
	static void ConDumpBinds(IConsole::IResult *pResult, void *pUserData);
	class IConsole *GetConsole() const { return Console(); }

	static void ConfigSaveCallback(class IConfig *pConfig, void *pUserData);

public:
	CBinds();

	class CBindsSpecial : public CComponent
	{
	public:
		CBinds *m_pBinds;
		virtual bool OnInput(IInput::CEvent Event);
	};

	enum
	{
		MODIFIER_NONE=0,
		MODIFIER_SHIFT,
		MODIFIER_CTRL,
		MODIFIER_ALT,
		MODIFIER_COUNT
	};

	CBindsSpecial m_SpecialBinds;

	void Bind(int KeyID, int Modifier, const char *pStr);
	void SetDefaults();
	void UnbindAll();
	const char *Get(int KeyID, int Modifier);
	void GetKey(const char *pBindStr, char aKey[64], unsigned BufSize);
	static const char *GetModifierName(int m);
	static int GetModifierMask(IInput *i);
	static int GetModifierMaskOfKey(int Key);
	static bool ModifierMatchesKey(int Modifier, int Key);

	virtual void OnConsoleInit();
	virtual bool OnInput(IInput::CEvent Event);

private:
	char m_aaaKeyBindings[KEY_LAST][MODIFIER_COUNT][128];
	static const int s_aDefaultBindKeys[];
	static const char s_aaDefaultBindValues[][32];
};
#endif
