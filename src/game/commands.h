#ifndef GAME_COMMANDS_H
#define GAME_COMMANDS_H

#include <base/tl/array.h>
#include <engine/console.h>
#include <base/system.h>

class CCommandManager
{
public:
    class CCommand
    {
    public:
        char m_aName[16];
        char m_aHelpText[64];
        char m_aArgsFormat[64];

        IConsole::FCommandCallback m_pfnCallback;
        void *m_pContext;

        CCommand()
        {
            m_aName[0] = '\0';
            m_aHelpText[0] = '\0';
            m_aArgsFormat[0] = '\0';

            m_pfnCallback = 0;
            m_pContext = 0;
        }

        CCommand(const char *pName, const char *pHelpText, const char *pArgsFormat, IConsole::FCommandCallback pfnCallback, void *pContext)
        {
            str_copy(m_aName, pName, sizeof(m_aName));
            str_copy(m_aHelpText, pHelpText, sizeof(m_aHelpText));
            str_copy(m_aArgsFormat, pArgsFormat, sizeof(m_aArgsFormat));
            m_pfnCallback = pfnCallback;
            m_pContext = pContext;
        }
    };

    typedef void (*FNewCommandHook)(const CCommand *pCommand, void *pContext);
    typedef void (*FRemoveCommandHook)(const CCommand *pCommand, void *pContext);

private:
    array<CCommand> m_aCommands;

    IConsole *m_pConsole;
    void *m_pHookContext;
    FNewCommandHook m_pfnNewCommandHook;
    FRemoveCommandHook m_pfnRemoveCommandHook;

public:
    CCommandManager()
    {
        m_pConsole = 0;
        m_aCommands.clear();
    }

    void Init(IConsole *pConsole, void *pHookContext = 0, FNewCommandHook pfnNewCommandHook = 0, FRemoveCommandHook pfnRemoveCommandHook = 0)
    {
        m_pConsole = pConsole;
        m_pHookContext = pHookContext;
        m_pfnNewCommandHook = pfnNewCommandHook;
        m_pfnRemoveCommandHook = pfnRemoveCommandHook;
    }

    const CCommand *GetCommand(const char *pCommand)
    {
        for(int i = 0; i < m_aCommands.size(); i++)
            if(!str_comp(m_aCommands[i].m_aName, pCommand))
                return &m_aCommands[i];

        return 0;
    }

    const CCommand *GetCommand(int Index)
    {
        if(Index < 0 || Index >= m_aCommands.size())
            return 0;

        return &m_aCommands[Index];
    }

    int AddCommand(const char *pCommand, const char *pHelpText, const char *pArgsFormat, IConsole::FCommandCallback pfnCallback, void *pContext)
    {
        const CCommand *pCom = GetCommand(pCommand);
        if(pCom)
            return 1;

        int Index = m_aCommands.add(CCommand(pCommand, pHelpText, pArgsFormat, pfnCallback, pContext));
        if(m_pfnNewCommandHook)
            m_pfnNewCommandHook(&m_aCommands[Index], m_pHookContext);

        return 0;
    }

    int RemoveCommand(const char *pCommand)
    {
        for(int i = 0; i < m_aCommands.size(); i++)
        {
            if(!str_comp(m_aCommands[i].m_aName, pCommand))
            {
                if(m_pfnRemoveCommandHook)
                    m_pfnRemoveCommandHook(&m_aCommands[i], m_pHookContext);

                m_aCommands.remove_index(i);
                return 0;
            }
        }

        return 1;
    }

    void ClearCommands()
    {
        m_aCommands.clear();
    }

    int CommandCount()
    {
        return m_aCommands.size();
    }

    struct SCommandContext
    {
        const char *m_pCommand;
        const char *m_pArgs;
        int m_ClientID;
        void *m_pContext;
    };

    int OnCommand(const char *pCommand, const char *pArgs, int ClientID)
    {
        dbg_msg("chat-command", "calling '%s' with args '%s'", pCommand, pArgs);
        const CCommand *pCom = GetCommand(pCommand);
        if(!pCom)
            return 1;

        SCommandContext Context = {pCom->m_aName, pArgs, ClientID, pCom->m_pContext};
        return m_pConsole->ParseCommandArgs(pArgs, pCom->m_aArgsFormat, pCom->m_pfnCallback, &Context);
    }

    int Filter(array<bool> &aFilter, const char *pStr)
    {
        dbg_assert(aFilter.size() == m_aCommands.size(), "filter size must match command count");
        if(!*pStr)
        {
            for(int i = 0; i < aFilter.size(); i++)
                aFilter[i] = false;
            return 0;
        }

        int Filtered = 0;
        for(int i = 0; i < m_aCommands.size(); i++)
        {
            Filtered += (aFilter[i] = str_find_nocase(m_aCommands[i].m_aName, pStr) != m_aCommands[i].m_aName);
        }

        return Filtered;
    }
};

#endif //GAME_COMMANDS_H
