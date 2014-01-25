#include "autohostmanager.h"

#include "../battle.h"
#include "../user.h"
#include "../mainwindow.h"

AutohostHandler::AutohostHandler():m_battle(0)
{
}

AutohostHandler::~AutohostHandler()
{

}

void AutohostHandler::SetBattle(Battle* battle)
{
    m_battle=battle;
}

//========================
//-------- Springie ------
//========================

SpringieHandler::SpringieHandler():AutohostHandler()
{

}

SpringieHandler::~SpringieHandler()
{

}

void SpringieHandler::Balance() const
{
    m_battle->Say(_T("!balance"));
}

void SpringieHandler::SetRandomMap() const
{
    m_battle->Say(_T("!map"));
}

void SpringieHandler::SetMap(const wxString& map) const
{
    m_battle->Say(_T("!map ")+map);
}

void SpringieHandler::ClearStartBoxes() const
{
    m_battle->Say(_T("!clear")); /// will check
}

void SpringieHandler::AddStartBox(int posx,int posy,int w,int h) const
{
    m_battle->Say(wxString::Format(wxT("!addbox %i %i %i %i"),posx,posy,w,h));
}

void SpringieHandler::Notify() const
{
    m_battle->Say(_T("!notify"));
}

void SpringieHandler::Start() const
{
    m_battle->Say(_T("!start"));
}

//------------------------------

//========================
//-------- Spads ---------
//========================
SpadsHandler::SpadsHandler():AutohostHandler()
{

}

SpadsHandler::~SpadsHandler()
{

}

void SpadsHandler::Balance() const
{
    m_battle->Say(_T("!balance"));
}

void SpadsHandler::SetRandomMap() const
{
    m_battle->Say(_T("!map 1")); //not so random
}

void SpadsHandler::SetMap(const wxString& map) const
{
    m_battle->Say(_T("!map ")+map);
}

void SpadsHandler::ClearStartBoxes() const
{

}

void SpadsHandler::AddStartBox(int /*posx*/,int /*posy*/,int /*w*/,int /*h*/) const
{

}

void SpadsHandler::Notify() const
{
    m_battle->Say(_T("!notify"));
}

void SpadsHandler::Start() const
{
    m_battle->Say(_T("!start"));
}
//-------------


AutohostManager::AutohostManager():m_type(AUTOHOSTTYPE_NONE), m_battle(0)
{

}

AutohostManager::~AutohostManager()
{

}

void AutohostManager::SetBattle(Battle* battle)
{
    m_battle=battle;
    m_springie.SetBattle(battle);
    m_spads.SetBattle(battle);
    m_emptyhandler.SetBattle(battle);
    m_type=AutohostManager::AUTOHOSTTYPE_NONE;
}

const AutohostHandler& AutohostManager::GetAutohostHandler() const
{
    switch(m_type)
    {
        case AUTOHOSTTYPE_SPRINGIE:
            return GetSpringie();
        case AUTOHOSTTYPE_SPADS:
            return GetSpads();
		case AUTOHOSTTYPE_NONE:
		case AUTOHOSTTYPE_UNKNOWN:
			return m_emptyhandler;
    }
    return m_emptyhandler;
}

const SpringieHandler& AutohostManager::GetSpringie() const
{
    return m_springie;
}

const SpadsHandler& AutohostManager::GetSpads() const
{
    return m_spads;
}

bool AutohostManager::RecnognizeAutohost()
{
    m_type=AutohostManager::AUTOHOSTTYPE_UNKNOWN;
    return false;
}

bool AutohostManager::RecnognizeAutohost(const wxString& game_hosttype_val)
{
	if(game_hosttype_val==_T("SPADS"))
	{
		m_type=AutohostManager::AUTOHOSTTYPE_SPADS;
		return true;
	}
	else if(game_hosttype_val==_T("SPRINGIE"))
	{
		m_type=AutohostManager::AUTOHOSTTYPE_SPRINGIE;
		return true;
	}

	return false;
}

bool AutohostManager::RecnognizeAutohost(const wxString& who, const wxString& message)
{
    if(m_battle)
    {
        User& founder=m_battle->GetFounder();
        UserStatus status=founder.GetStatus();

        if(status.bot)
        {
            wxString nick=founder.GetNick();
            if(who.Upper() ==nick.Upper())
            {
                if(message.Find(_T("welcome to Springie"))!=wxNOT_FOUND)
                {
                    m_type=AutohostManager::AUTOHOSTTYPE_SPRINGIE;
                    return true;
                }
                else if(message.Find(_T("welcome to Spads0"))!=wxNOT_FOUND)
                {
                    m_type=AutohostManager::AUTOHOSTTYPE_SPADS;
                    return true;
                }
            }
        }
    }
    m_type=AutohostManager::AUTOHOSTTYPE_UNKNOWN;
    return false;
}

AutohostManager::AutohostType AutohostManager::GetAutohostType() const
{
    return m_type;
}
