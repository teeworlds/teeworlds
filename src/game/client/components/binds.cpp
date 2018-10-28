/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/config.h>
#include <engine/shared/config.h>
#include "binds.h"

const int CBinds::s_aDefaultBindKeys[] = {
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

bool CBinds::CBindsSpecial::OnInput(IInput::CEvent Event)
{
	// don't handle invalid events and keys that arn't set to anything
	if(((Event.m_Key >= KEY_F1 && Event.m_Key <= KEY_F12) || (Event.m_Key >= KEY_F13 && Event.m_Key <= KEY_F24)) && m_pBinds->m_aaKeyBindings[Event.m_Key][0] != 0)
	{
		int Stroke = 0;
		if(Event.m_Flags&IInput::FLAG_PRESS)
			Stroke = 1;

		m_pBinds->GetConsole()->ExecuteLineStroked(Stroke, m_pBinds->m_aaKeyBindings[Event.m_Key]);
		return true;
	}

	return false;
}

CBinds::CBinds()
{
	mem_zero(m_aaKeyBindings, sizeof(m_aaKeyBindings));
	m_SpecialBinds.m_pBinds = this;
}

void CBinds::Bind(int KeyID, const char *pStr)
{
	if(KeyID < 0 || KeyID >= KEY_LAST)
		return;

	str_copy(m_aaKeyBindings[KeyID], pStr, sizeof(m_aaKeyBindings[KeyID]));
	char aBuf[256];
	if(!m_aaKeyBindings[KeyID][0])
		str_format(aBuf, sizeof(aBuf), "unbound %s (%d)", Input()->KeyName(KeyID), KeyID);
	else
		str_format(aBuf, sizeof(aBuf), "bound %s (%d) = %s", Input()->KeyName(KeyID), KeyID, m_aaKeyBindings[KeyID]);
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "binds", aBuf);
}


bool CBinds::OnInput(IInput::CEvent e)
{
	// don't handle invalid events and keys that arn't set to anything
	if(e.m_Key <= 0 || e.m_Key >= KEY_LAST || m_aaKeyBindings[e.m_Key][0] == 0)
		return false;

	if(e.m_Flags&IInput::FLAG_PRESS)
		Console()->ExecuteLineStroked(1, m_aaKeyBindings[e.m_Key]);
	if(e.m_Flags&IInput::FLAG_RELEASE)
		Console()->ExecuteLineStroked(0, m_aaKeyBindings[e.m_Key]);
	return true;
}

void CBinds::UnbindAll()
{
	for(int i = 0; i < KEY_LAST; i++)
		m_aaKeyBindings[i][0] = 0;
}

const char *CBinds::Get(int KeyID)
{
	if(KeyID > 0 && KeyID < KEY_LAST)
		return m_aaKeyBindings[KeyID];
	return "";
}

const char *CBinds::GetKey(const char *pBindStr)
{
	for(int KeyId = 0; KeyId < KEY_LAST; KeyId++)
	{
		const char *pBind = Get(KeyId);
		if(!pBind[0])
			continue;

		if(str_comp(pBind, pBindStr) == 0)
			return Input()->KeyName(KeyId);
	}

	return "";
}

void CBinds::SetDefaults()
{
	// set default key bindings
	UnbindAll();
	const int count = sizeof(s_aDefaultBindKeys)/sizeof(int);
	dbg_assert(count == sizeof(s_aaDefaultBindValues)/32, "the count of bind keys differs from that of bind values!");
	for(int i = 0; i < count; i++)
		Bind(s_aDefaultBindKeys[i], s_aaDefaultBindValues[i]);
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
	int id = pBinds->GetKeyID(pKeyName);

	if(!id)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "key %s not found", pKeyName);
		pBinds->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "binds", aBuf);
		return;
	}

	pBinds->Bind(id, pResult->GetString(1));
}


void CBinds::ConUnbind(IConsole::IResult *pResult, void *pUserData)
{
	CBinds *pBinds = (CBinds *)pUserData;
	const char *pKeyName = pResult->GetString(0);
	int id = pBinds->GetKeyID(pKeyName);

	if(!id)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "key %s not found", pKeyName);
		pBinds->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "binds", aBuf);
		return;
	}

	pBinds->Bind(id, "");
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
		if(pBinds->m_aaKeyBindings[i][0] == 0)
			continue;
		str_format(aBuf, sizeof(aBuf), "%s (%d) = %s", pBinds->Input()->KeyName(i), i, pBinds->m_aaKeyBindings[i]);
		pBinds->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "binds", aBuf);
	}
}

int CBinds::GetKeyID(const char *pKeyName)
{
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

void CBinds::ConfigSaveCallback(IConfig *pConfig, void *pUserData)
{
	CBinds *pSelf = (CBinds *)pUserData;

	char aBuffer[256];
	char *pEnd = aBuffer+sizeof(aBuffer)-8;
	for(int i = 0; i < KEY_LAST; i++)
	{
		if(pSelf->m_aaKeyBindings[i][0] == 0)
			continue;

		str_format(aBuffer, sizeof(aBuffer), "bind %s ", pSelf->Input()->KeyName(i));

		// process the string. we need to escape some characters
		const char *pSrc = pSelf->m_aaKeyBindings[i];
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

	for(unsigned j = 0; j < sizeof(s_aDefaultBindKeys)/sizeof(int); j++)
	{
		const int i = s_aDefaultBindKeys[j];
		if(pSelf->m_aaKeyBindings[i][0] == 0)
		{
			// explicitly unbind keys that were unbound by the user
			str_format(aBuffer, sizeof(aBuffer), "unbind %s ", pSelf->Input()->KeyName(i));
			pConfig->WriteLine(aBuffer);
		}
	}
}
