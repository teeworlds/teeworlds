/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/config.h>
#include <engine/shared/config.h>
#include "binds.h"

const int CBinds::s_aDefaultBindKeys[] = { // only simple binds
	KEY_F1, KEY_F2, KEY_TAB, 'u', KEY_F10,
	'a', 'd',
	KEY_SPACE, KEY_MOUSE_1, KEY_MOUSE_2, KEY_LSHIFT, KEY_RSHIFT, KEY_RIGHT, KEY_LEFT,
	'1', '2', '3', '4', '5',
	KEY_MOUSE_WHEEL_UP, KEY_MOUSE_WHEEL_DOWN,
	't', 'y', 'x',
	KEY_F3, KEY_F4,
	'r',
};
const char CBinds::s_aaDefaultBindValues[][32] = {
	"toggle_local_console", "toggle_remote_console", "+scoreboard", "+show_chat", "screenshot",
	"+left", "+right",
	"+jump", "+fire", "+hook", "+emote", "+spectate", "spectate_next", "spectate_previous",
	"+weapon1", "+weapon2", "+weapon3", "+weapon4", "+weapon5",
	"+prevweapon", "+nextweapon",
	"chat all", "chat team", "chat whisper",
	"vote yes", "vote no",
	"ready_change",
};

CBinds::CBinds()
{
	mem_zero(m_aaaKeyBindings, sizeof(m_aaaKeyBindings));
	m_SpecialBinds.m_pBinds = this;
}

void CBinds::Bind(int KeyID, int Modifier, const char *pStr)
{
	if(KeyID < 0 || KeyID >= KEY_LAST)
		return;

	// skip modifiers for +xxx binds
	if(pStr[0] == '+')
		Modifier = 0;
	str_copy(m_aaaKeyBindings[KeyID][Modifier], pStr, sizeof(m_aaaKeyBindings[KeyID][Modifier]));
	char aBuf[256];
	if(!m_aaaKeyBindings[KeyID][Modifier][0])
		str_format(aBuf, sizeof(aBuf), "unbound %s%s (%d)", GetModifierName(Modifier),Input()->KeyName(KeyID), KeyID);
	else
		str_format(aBuf, sizeof(aBuf), "bound %s%s (%d) = %s", GetModifierName(Modifier),Input()->KeyName(KeyID), KeyID, m_aaaKeyBindings[KeyID][Modifier]);
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "binds", aBuf);
}

int CBinds::GetModifierMask(IInput *i)
{
	int Mask = 0;
	// since we only handle one modifier, when doing ctrl+q and shift+q, execute both
	Mask |= i->KeyIsPressed(KEY_LSHIFT) << CBinds::MODIFIER_SHIFT;
	Mask |= i->KeyIsPressed(KEY_RSHIFT) << CBinds::MODIFIER_SHIFT;
	Mask |= i->KeyIsPressed(KEY_LCTRL) << CBinds::MODIFIER_CTRL;
	Mask |= i->KeyIsPressed(KEY_RCTRL) << CBinds::MODIFIER_CTRL;
	Mask |= i->KeyIsPressed(KEY_LALT) << CBinds::MODIFIER_ALT;
	if(Mask == 0)
		return 1; // if no modifier, flag with MODIFIER_NONE
	return Mask;
}

int CBinds::GetModifierMaskOfKey(int Key)
{
	switch(Key)
	{
		case KEY_LSHIFT:
		case KEY_RSHIFT:
			return 1 << CBinds::MODIFIER_SHIFT;
		case KEY_LCTRL:
		case KEY_RCTRL:
			return 1 << CBinds::MODIFIER_CTRL;
		case KEY_LALT:
			return 1 << CBinds::MODIFIER_ALT;
		default:
			return 0;
	}
}

bool CBinds::ModifierMatchesKey(int Modifier, int Key)
{
	switch(Modifier)
	{
		case MODIFIER_SHIFT:
			return Key == KEY_LSHIFT || Key == KEY_RSHIFT;
		case MODIFIER_CTRL:
			return Key == KEY_LCTRL || Key == KEY_RCTRL;
		case MODIFIER_ALT:
			return Key == KEY_LALT;
		case MODIFIER_NONE:
		default:
			return false;
	}
}

bool CBinds::CBindsSpecial::OnInput(IInput::CEvent Event)
{
	int Mask = GetModifierMask(Input());

	// don't handle anything but FX and composed FX binds
	if(/*Mask == 1 && */!(Event.m_Key >= KEY_F1 && Event.m_Key <= KEY_F12) && !(Event.m_Key >= KEY_F13 && Event.m_Key <= KEY_F24))
		return false;

	bool rtn = false;
	for(int m = 0; m < MODIFIER_COUNT; m++)
	{
		if((Mask&(1 << m)) && m_pBinds->m_aaaKeyBindings[Event.m_Key][m][0] != 0)
		{
			if(Event.m_Flags&IInput::FLAG_PRESS)
				m_pBinds->GetConsole()->ExecuteLineStroked(1, m_pBinds->m_aaaKeyBindings[Event.m_Key][m]);
			if(Event.m_Flags&IInput::FLAG_RELEASE)
				m_pBinds->GetConsole()->ExecuteLineStroked(0, m_pBinds->m_aaaKeyBindings[Event.m_Key][m]);
			rtn = true;
		}
	}
	return rtn;
}

bool CBinds::OnInput(IInput::CEvent Event)
{
	// don't handle invalid events and keys that aren't set to anything
	if(Event.m_Key <= 0 || Event.m_Key >= KEY_LAST)
		return false;

	int Mask = GetModifierMask(Input());
	int KeyModifierMask = GetModifierMaskOfKey(Event.m_Key); // returns 0 if m_Key is not a modifier
	if(KeyModifierMask)
	{
		// avoid to have e.g. key press "lshift" be treated as "shift+lshift"
		Mask -= KeyModifierMask;
		if(!Mask)
			Mask = 1 << MODIFIER_NONE;
	}

	bool rtn = false;
	for(int m = 0; m < MODIFIER_COUNT; m++)
	{
		if(((Mask&(1 << m)) || (m == 0 && m_aaaKeyBindings[Event.m_Key][0][0] == '+')) && m_aaaKeyBindings[Event.m_Key][m][0] != 0)	// always trigger +xxx binds despite any modifier
		{
			if(Event.m_Flags&IInput::FLAG_PRESS)
				Console()->ExecuteLineStroked(1, m_aaaKeyBindings[Event.m_Key][m]);
			if(Event.m_Flags&IInput::FLAG_RELEASE)
				Console()->ExecuteLineStroked(0, m_aaaKeyBindings[Event.m_Key][m]);
			rtn = true;
		}
	}
	return rtn;
}

void CBinds::UnbindAll()
{
	for(int i = 0; i < KEY_LAST; i++)
		for(int m = 0; m < MODIFIER_COUNT; m++)
			m_aaaKeyBindings[i][m][0] = 0;
}

const char *CBinds::Get(int KeyID, int Modifier)
{
	if(KeyID > 0 && KeyID < KEY_LAST)
		return m_aaaKeyBindings[KeyID][Modifier];
	return "";
}

void CBinds::GetKey(const char *pBindStr, char aKey[64], unsigned BufSize)
{
	aKey[0] = 0;
	for(int KeyID = 0; KeyID < KEY_LAST; KeyID++)
	{
		for(int m = 0; m < MODIFIER_COUNT; m++)
		{
			const char *pBind = Get(KeyID, m);
			if(!pBind[0])
				continue;

			if(str_comp(pBind, pBindStr) == 0)
			{
				str_format(aKey, BufSize, "%s%s", GetModifierName(m), Input()->KeyName(KeyID));
				return;
			}
		}
	}
	str_copy(aKey, "key not found", BufSize);
}

void CBinds::SetDefaults()
{
	// set default key bindings
	UnbindAll();
	const int count = sizeof(s_aDefaultBindKeys)/sizeof(int);
	dbg_assert(count == sizeof(s_aaDefaultBindValues)/32, "the count of bind keys differs from that of bind values!");
	for(int i = 0; i < count; i++)
		Bind(s_aDefaultBindKeys[i], MODIFIER_NONE, s_aaDefaultBindValues[i]);
}

void CBinds::OnConsoleInit()
{
	// bindings
	IConfig *pConfig = Kernel()->RequestInterface<IConfig>();
	if(pConfig)
		pConfig->RegisterCallback(ConfigSaveCallback, this);

	Console()->Register("bind", "sr", CFGFLAG_CLIENT, ConBind, this, "Bind key to execute the command");
	Console()->Register("unbind", "s", CFGFLAG_CLIENT, ConUnbind, this, "Unbind key");
	Console()->Register("unbindall", "", CFGFLAG_CLIENT, ConUnbindAll, this, "Unbind all keys");
	Console()->Register("dump_binds", "", CFGFLAG_CLIENT, ConDumpBinds, this, "Dump binds");

	// default bindings
	SetDefaults();
}

void CBinds::ConBind(IConsole::IResult *pResult, void *pUserData)
{
	CBinds *pBinds = (CBinds *)pUserData;
	const char *pKeyName = pResult->GetString(0);
	int Modifier;
	int id = pBinds->DecodeBindString(pKeyName, &Modifier);

	if(!id)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "key %s not found", pKeyName);
		pBinds->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "binds", aBuf);
		return;
	}

	pBinds->Bind(id, Modifier, pResult->GetString(1));
}


void CBinds::ConUnbind(IConsole::IResult *pResult, void *pUserData)
{
	CBinds *pBinds = (CBinds *)pUserData;
	const char *pKeyName = pResult->GetString(0);
	int Modifier;
	int id = pBinds->DecodeBindString(pKeyName, &Modifier);

	if(!id)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "key %s not found", pKeyName);
		pBinds->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "binds", aBuf);
		return;
	}

	pBinds->Bind(id, Modifier, "");
}


void CBinds::ConUnbindAll(IConsole::IResult *pResult, void *pUserData)
{
	CBinds *pBinds = (CBinds *)pUserData;
	pBinds->UnbindAll();
}


void CBinds::ConDumpBinds(IConsole::IResult *pResult, void *pUserData)
{
	CBinds *pBinds = (CBinds *)pUserData;
	char aBuf[1024];
	for(int i = 0; i < KEY_LAST; i++)
	{
		for(int m = 0; m < MODIFIER_COUNT; m++)
		{
			if(pBinds->m_aaaKeyBindings[i][m][0] == 0)
				continue;
			str_format(aBuf, sizeof(aBuf), "%s%s (%d) = %s", GetModifierName(m), pBinds->Input()->KeyName(i), i, pBinds->m_aaaKeyBindings[i][m]);
			pBinds->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "binds", aBuf);
		}
	}
}

int CBinds::DecodeBindString(const char *pKeyName, int* pModifier)
{
	// check for modifier of type xxx+xxx
	char aBuf[64];
	str_copy(aBuf, pKeyName, sizeof(aBuf));
	const char* pStr = str_find(aBuf, "+");
	*pModifier = 0;
	if(pStr && *(pStr+1)) // make sure the '+' isn't the last character
	{
		aBuf[pStr-aBuf] = 0; // *pStr=0 (split the string where the + is)
		char* pModifierStr = aBuf;
		if(str_comp(pModifierStr, "shift") == 0)
			*pModifier = MODIFIER_SHIFT;
		else if(str_comp(pModifierStr, "ctrl") == 0)
			*pModifier = MODIFIER_CTRL;
		else if(str_comp(pModifierStr, "alt") == 0)
			*pModifier = MODIFIER_ALT;
		else
			return 0;
		pKeyName = pStr + 1;
		if(!pKeyName)
			return 0;
	}

	// check for numeric
	if(pKeyName[0] == '&')
	{
		int i = str_toint(pKeyName+1);
		if(i > 0 && i < KEY_LAST)
			return i; // numeric
	}

	// search for key
	for(int i = 0; i < KEY_LAST; i++)
	{
		if(str_comp(pKeyName, Input()->KeyName(i)) == 0)
			return i;
	}

	return 0;
}


const char *CBinds::GetModifierName(int m)
{
	switch(m)
	{
		case 0:
			return "";
		case MODIFIER_SHIFT:
			return "shift+";
		case MODIFIER_CTRL:
			return "ctrl+";
		case MODIFIER_ALT:
			return "alt+";
		default:
			return "";
	}
}

void CBinds::ConfigSaveCallback(IConfig *pConfig, void *pUserData)
{
	CBinds *pSelf = (CBinds *)pUserData;

	char aBuffer[256];
	char *pEnd = aBuffer+sizeof(aBuffer)-8;
	for(int i = 0; i < KEY_LAST; i++)
	{
		for(int m = 0; m < MODIFIER_COUNT; m++)
		{
			if(pSelf->m_aaaKeyBindings[i][m][0] == 0)
				continue;

			str_format(aBuffer, sizeof(aBuffer), "bind %s%s ", GetModifierName(m), pSelf->Input()->KeyName(i));

			// process the string. we need to escape some characters
			const char *pSrc = pSelf->m_aaaKeyBindings[i][m];
			char *pDst = aBuffer + str_length(aBuffer);
			*pDst++ = '"';
			while(*pSrc && pDst < pEnd)
			{
				if(*pSrc == '"' || *pSrc == '\\') // escape \ and "
					*pDst++ = '\\';
				*pDst++ = *pSrc++;
			}
			*pDst++ = '"';
			*pDst++ = 0;

			pConfig->WriteLine(aBuffer);
		}
	}

	// default binds can only be non-composed right now, so only check for that
	for(unsigned j = 0; j < sizeof(s_aDefaultBindKeys)/sizeof(int); j++)
	{
		const int i = s_aDefaultBindKeys[j];
		if(pSelf->m_aaaKeyBindings[i][0][0] == 0)
		{
			// explicitly unbind keys that were unbound by the user
			str_format(aBuffer, sizeof(aBuffer), "unbind %s ", pSelf->Input()->KeyName(i));
			pConfig->WriteLine(aBuffer);
		}
	}
}
