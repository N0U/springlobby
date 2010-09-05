#include "hotkey_panel.h"

#include <iostream>

#include <wx/log.h>
#include <wx/string.h>
#include <wx/button.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/panel.h>

#include "../../utils/customdialogs.h"
#include "hotkey_parser.h"
#include "spring_command.h"
#include "commandlist.h"
#include "../../settings.h"
#include "KeynameConverter.h"
#include "SpringDefaultProfile.h"
#include "AddSelectionCmdDlg.h"

hotkey_panel::hotkey_panel(wxWindow *parent, wxWindowID id , const wxString &title , const wxPoint& pos , const wxSize& size, long style)
													: wxScrolledWindow(parent, id, pos, size, style|wxTAB_TRAVERSAL|wxHSCROLL,title),
													m_keyConfigPanel( this, -1, wxDefaultPosition, wxDefaultSize, (wxKEYBINDER_DEFAULT_STYLE & ~wxKEYBINDER_SHOW_APPLYBUTTON), wxT("HotkeyPanel"), 
																		wxT("Selection"), wxCommandEventHandler(hotkey_panel::ButtonAddSelectionCommandClicked),
																		wxT("Custom"), wxCommandEventHandler(hotkey_panel::ButtonAddCustomCommandClicked)),
													m_uikeys_manager(sett().GetCurrentUsedUikeys() )
{
	KeynameConverter::initialize();

	UpdateControls();

	//group box
	wxStaticBoxSizer* sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Hotkey Configuration") ), wxVERTICAL );
	sbSizer1->Add( &m_keyConfigPanel );

	//borders
	wxSizer * parentSizer = new wxBoxSizer(wxHORIZONTAL);
	wxSizer * childLSizer = new wxBoxSizer(wxVERTICAL);

	childLSizer->Add(0, 5, 0);
	childLSizer->Add(sbSizer1,0,wxEXPAND|wxALL,5);

	parentSizer->Add(10, 0, 0);
	parentSizer->Add(childLSizer,0,wxEXPAND|wxTOP,5);

	
	//content
	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 1, 1, 0, 0 ); 
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	fgSizer1->Add( parentSizer, 1, wxEXPAND, 5 );

	this->SetSizer( fgSizer1 );
	
	this->Layout();
}


hotkey_panel::~hotkey_panel(void)
{
}

void hotkey_panel::ButtonAddSelectionCommandClicked( wxCommandEvent& event )
{
	AddSelectionCmdDlg dlg( this );
	
	if ( dlg.ShowModal() == wxID_OK )
	{
		wxString cmd = wxT("select ") + dlg.getCommandString();
		CommandList::addCustomCommand( cmd );

		this->UpdateControls();
	}
}

void hotkey_panel::ButtonAddCustomCommandClicked( wxCommandEvent& event )
{

}

bool hotkey_panel::isBindingInProfile( const key_binding& springprofile, const wxString& command, const wxString& springkey )
{
	key_binding::const_iterator citer = springprofile.find( command );
	if ( citer == springprofile.end() )
	{
		return false;
	}

	key_set::const_iterator kiter = citer->second.find( springkey );
	if ( kiter == citer->second.end() )
	{
		return false;
	}

	return true;
}

key_binding hotkey_panel::getBindingsFromProfile( const wxKeyProfile& profile )
{
	key_binding binding;
	const wxCmdArray* pCmdArr = profile.GetArray();
	for( size_t j=0; j < pCmdArr->GetCount(); ++j )
	{
		const wxCmd& cmd = *pCmdArr->Item(j);
		wxArrayString keys = cmd.GetShortcutsList();
		for( size_t k=0; k < keys.GetCount(); ++k )
		{
			binding[ cmd.GetName() ].insert( KeynameConverter::spring2wxKeybinder( keys.Item( k ),true ) );
		}
	}
	return binding;
}

bool hotkey_panel::isDefaultBinding( const wxString& command, const wxString& springKey )
{
	const key_binding& defBindings = SpringDefaultProfile::getAllBindingsC2K();

	return hotkey_panel::isBindingInProfile( defBindings, command, springKey );
}

/*
*	We do only save the bindings that differ from the standard spring keybindings
*/
void hotkey_panel::SaveSettings()
{
	try
	{
		sett().DeleteHotkeyProfiles();

		wxKeyProfileArray profileArr = m_keyConfigPanel.GetProfiles();
		for( size_t i=0; i < profileArr.GetCount(); ++i )
		{
			const wxKeyProfile& profile = *profileArr.Item(i);
			if ( profile.IsNotEditable() )
			{
				//skip build-in profiles
				continue;
			}

			//add bindings
			const wxCmdArray* pCmdArr = profile.GetArray();
			for( size_t j=0; j < pCmdArr->GetCount(); ++j )
			{
				const wxCmd& cmd = *pCmdArr->Item(j);
				wxArrayString keys = cmd.GetShortcutsList();
				for( size_t k=0; k < keys.GetCount(); ++k )
				{
					//only write non-default bindings to settings
					const wxString springkey = KeynameConverter::spring2wxKeybinder( keys.Item( k ), true );
					if ( false == hotkey_panel::isDefaultBinding( cmd.GetName(), springkey ) )
					{
						sett().SetHotkey( profile.GetName(), cmd.GetName(), keys.Item(k), false );
					}
				}
			}

			//check if any bindings from the default bindings have been unbind. and save that info to the settings
			const key_binding& defBindings = SpringDefaultProfile::getAllBindingsC2K();
			const key_binding& proBindings = hotkey_panel::getBindingsFromProfile( profile  );
			for( key_binding::const_iterator iter = defBindings.begin(); iter != defBindings.end(); ++iter )
			{
				//check now if this default key binding has been deleted from this profile
				for ( key_set::const_iterator iiter = iter->second.begin(); iiter != iter->second.end(); ++iiter )
				{
					if ( false == hotkey_panel::isBindingInProfile( proBindings, iter->first, (*iiter) ) )
					{
						sett().SetHotkey( profile.GetName(), iter->first, KeynameConverter::spring2wxKeybinder( (*iiter) ), true );
					}
				}
			}
		}

		sett().SaveSettings();

		//Write bindings to file
		const wxKeyProfile& selProfile = *this->m_keyConfigPanel.GetSelProfile();
		key_binding bindings = hotkey_panel::getBindingsFromProfile( selProfile );

		this->m_uikeys_manager.writeBindingsToFile( bindings );

		this->m_keyConfigPanel.ResetProfileBeenModifiedOrSelected();
	}
	catch( const std::exception& ex )
	{
		customMessageBox(SS_MAIN_ICON, _("Hotkey SaveSettings error"), _("Hotkey SaveSettings error"), wxOK | wxICON_HAND );
	}
}

wxKeyProfile hotkey_panel::buildNewProfile( const wxString& name, const wxString& description, bool readOnly )
{
	wxKeyProfile profile( name, description, readOnly, readOnly );

	const CommandList::CommandMap& commands = CommandList::getCommands();
	for( CommandList::CommandMap::const_iterator iter = commands.begin(); iter != commands.end(); ++iter )
	{
		const CommandList::Command& cmd = iter->second;

		spring_command* pCmd = new spring_command( cmd.m_command, cmd.m_description, cmd.m_id );
		profile.AddCmd( pCmd );
	}

	return profile;
}

void hotkey_panel::putKeybindingsToProfile( wxKeyProfile& profile, const key_binding& bindings )
{
	for( key_binding::const_iterator iter = bindings.begin(); iter != bindings.end(); ++iter )
	{
		try
		{
			wxCmd& cmd = *(profile.GetCmd( CommandList::getCommandByName(iter->first).m_id ));

			const key_set& keys = iter->second;
			for( key_set::const_iterator iiter = keys.begin(); iiter != keys.end(); iiter++ )
			{
				cmd.AddShortcut( KeynameConverter::spring2wxKeybinder( (*iiter) ) );
			}
		}
		catch( const std::exception& ex )
		{
			wxString msg = _( "Warning: Error reading uikeys.txt!" ); // + wxString(ex.what()); 
			wxLogWarning( msg );
			customMessageBox(SS_MAIN_ICON, msg, _("Hotkey warning"), wxOK | wxICON_WARNING );
		}
	}
}

unsigned hotkey_panel::getShortcutCountFromBinding( const key_binding& bindings )
{
	unsigned count = 0;
	for ( key_binding::const_iterator iter = bindings.begin(); iter != bindings.end(); ++iter )
	{
		count += iter->second.size();
	}
	return count;
}

/*
bool hotkey_panel::compareBindings( const key_binding& springBindings, const key_binding& kbBindings )
{
//first, compare total keycount, that will do for most
if ( hotkey_panel::getShortcutCountFromBinding( springBindings ) !=
hotkey_panel::getShortcutCountFromBinding( kbBindings ) )
{
return false;
}

for ( key_binding::const_iterator iter = kbBindings.begin(); iter != kbBindings.end(); ++iter )
{
key_binding::const_iterator cmdIter = springBindings.find( iter->first );
if ( cmdIter == springBindings.end() )
{
//command not found in this profile
return false;
}

for( key_set::const_iterator iiter = iter->second.begin(); iiter != iter->second.end(); ++iiter )
{
if ( cmdIter->second.find( KeynameConverter::spring2wxKeybinder( *iiter, true ) ) == cmdIter->second.end() )
{
return false;
}
}
}

return true;
}
*/

wxString hotkey_panel::getNextFreeProfileName()
{
	const wxString profNameTempl = wxT("UserProfile ");
	wxString curProfTryName;
	const wxKeyProfileArray profileArr = m_keyConfigPanel.GetProfiles();

	for( unsigned k=1; k < 99999u; ++k )
	{
		bool found = false;
		curProfTryName = profNameTempl;
		curProfTryName << k;
		for( size_t i=0; i < profileArr.GetCount(); ++i )
		{
			const wxKeyProfile& profile = *profileArr.Item(i);
			if ( profile.GetName() == curProfTryName )
			{
				found = true;
				break;
			}
		}
		if ( found == false )
		{
			break;
		}
	}
	return curProfTryName;
}

void hotkey_panel::selectProfileFromUikeys()
{
	const key_binding& curBinding = this->m_uikeys_manager.getBindingsC2K();

	const wxKeyProfileArray profileArr = m_keyConfigPanel.GetProfiles();

	static int noProfileFound = -1;
	int foundIdx = noProfileFound;
	for( size_t i=0; i < profileArr.GetCount(); ++i )
	{
		const wxKeyProfile& profile = *profileArr.Item(i);
		const key_binding proBinds = hotkey_panel::getBindingsFromProfile( profile );

		if ( curBinding == proBinds ) //hotkey_panel::compareBindings( curBinding, proBinds ) )
		{
			foundIdx = i;
			break;
		}
	}

	if ( foundIdx == noProfileFound )
	{
		const wxString profName = this->getNextFreeProfileName();
		wxKeyProfile profile = buildNewProfile( profName, wxT("User hotkey profile"), false );
		this->putKeybindingsToProfile( profile, curBinding );	
		m_keyConfigPanel.AddProfile( profile, true );


		customMessageBox(SS_MAIN_ICON, _("Your current hotkey configuration does not match any known profile.\n A new profile with the name '" + profName + _("' has been created.")), 
			_("New hotkey profile found"), wxOK );

		foundIdx = m_keyConfigPanel.GetProfiles().GetCount() - 1;
	}

	m_keyConfigPanel.SetSelProfile( foundIdx );
}

key_binding_collection hotkey_panel::getProfilesFromSettings( )
{
	key_binding_collection coll;

	wxArrayString profiles = sett().GetHotkeyProfiles();
	for( size_t i=0; i < profiles.GetCount(); ++i)
	{
		wxString profName = profiles.Item(i);

		//fill with the default bindings
		coll[profName] = SpringDefaultProfile::getAllBindingsC2K();

		//add keybindings
		wxArrayString commands = sett().GetHotkeyProfileCommands( profName );
		for( size_t k=0; k < commands.GetCount(); ++k )
		{
			wxString cmd = commands.Item(k);
			wxArrayString keys = sett().GetHotkeyProfileCommandKeys( profName, cmd );
			for( size_t j=0; j < keys.GetCount(); ++j )
			{
				const wxString& key = sett().GetHotkey( profName, cmd, keys.Item(j) );
				const wxString& springKey = KeynameConverter::spring2wxKeybinder( keys.Item(j), true );
				if ( key == wxT("bind") )
				{
					coll[profName][cmd].insert( springKey );
				}
				else if ( key == wxT("unbind") )
				{
					coll[profName][cmd].erase( springKey );
				}
				else
				{
					const wxString msg = wxT("Unknown key action: ") + key;
					throw std::runtime_error( msg.mb_str(wxConvUTF8) );
				}
			}
		}
	}

	return coll;
}

void hotkey_panel::UpdateControls(int /*unused*/)
{
	this->updateTreeView();

	//Fetch the profiles
	m_keyConfigPanel.RemoveAllProfiles();

	//put default profile
	wxKeyProfile defProf = this->buildNewProfile( wxT("Spring default"), wxT("Spring's default keyprofile"),true);
	const key_binding defBinds = SpringDefaultProfile::getAllBindingsC2K();
	this->putKeybindingsToProfile( defProf, defBinds );		
	m_keyConfigPanel.AddProfile( defProf );

	//put user profiles from springsettings configuration
	{
		key_binding_collection profiles = hotkey_panel::getProfilesFromSettings();
		for( key_binding_collection::const_iterator piter = profiles.begin(); piter != profiles.end(); ++piter )
		{
			wxString profName = piter->first;

			wxKeyProfile profile = buildNewProfile(profName, wxT("User hotkey profile"),false);

			//add keybindings
			for( key_binding::const_iterator citer = piter->second.begin(); citer != piter->second.end(); ++citer )
			{
				const wxString cmd = citer->first;
				for ( key_set::const_iterator kiter = citer->second.begin(); kiter != citer->second.end(); ++kiter )
				{
					wxCmd* pCmd = profile.GetCmd( CommandList::getCommandByName(cmd).m_id );
					pCmd->AddShortcut( KeynameConverter::spring2wxKeybinder( (*kiter) ) );
				}
			}

			m_keyConfigPanel.AddProfile( profile );

			getBindingsFromProfile( *m_keyConfigPanel.GetProfile(1) );
		}
	}

	this->m_keyConfigPanel.ResetProfileBeenModifiedOrSelected();

	selectProfileFromUikeys();
}

void hotkey_panel::updateTreeView()
{
	wxKeyConfigPanel::ControlMap ctrlMap;

	{	//1. import control map
		const CommandList::CommandMap& commands = CommandList::getCommands();
		for( CommandList::CommandMap::const_iterator iter = commands.begin(); iter != commands.end(); ++iter )
		{
			const CommandList::Command& cmd = iter->second;
			ctrlMap[ cmd.m_category ][ cmd.m_command ] = cmd.m_id;
		}
	}

	{	//2. import springsettings-config-profiles
		key_binding_collection profiles = hotkey_panel::getProfilesFromSettings();
		for( key_binding_collection::const_iterator piter = profiles.begin(); piter != profiles.end(); ++piter )
		{
			wxString profName = piter->first;

			//add keybindings
			for( key_binding::const_iterator citer = piter->second.begin(); citer != piter->second.end(); ++citer )
			{
				const CommandList::Command& cmd = CommandList::getCommandByName( citer->first );
				ctrlMap[ cmd.m_category ][ cmd.m_command ] = cmd.m_id;
			}
		}
	}

	{	//3. from uikeys.txt
		key_binding uikeys = this->m_uikeys_manager.getBindingsC2K();

		key_binding_collection profiles = hotkey_panel::getProfilesFromSettings();
		for( key_binding::const_iterator iter = uikeys.begin(); iter != uikeys.end(); ++iter )
		{
			const CommandList::Command& cmd = CommandList::getCommandByName( iter->first );
			ctrlMap[ cmd.m_category ][ cmd.m_command ] = cmd.m_id;
		}
	}

	m_keyConfigPanel.ImportRawList( ctrlMap, wxT("Commands") ); 
}

bool hotkey_panel::HasProfileBeenModifiedOrSelected() const
{
	return this->m_keyConfigPanel.HasProfileBeenModifiedOrSelected();
}

void hotkey_panel::ResetProfileBeenModifiedOrSelected()
{
	this->m_keyConfigPanel.ResetProfileBeenModifiedOrSelected();
}
