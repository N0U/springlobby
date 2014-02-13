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
#include <wx/textdlg.h>
#include <wx/filename.h>

#include <lslunitsync/c_api.h>

#include "../../utils/customdialogs.h"
#include "../../utils/conversion.h"
#include "../../utils/slpaths.h"
#include "hotkey_parser.h"
#include "wxSpringCommand.h"
#include "commandlist.h"
#include "../../settings.h"
#include "../../helper/slconfig.h"
#include "KeynameConverter.h"
#include "SpringDefaultProfile.h"
#include "HotkeyException.h"
#include "AddSelectionCmdDlg.h"

const wxChar sep = wxFileName::GetPathSeparator();

hotkey_panel::hotkey_panel(wxWindow *parent, wxWindowID id , const wxString &title , const wxPoint& pos , const wxSize& size, long style)
													try : wxScrolledWindow(parent, id, pos, size, style|wxTAB_TRAVERSAL|wxHSCROLL,title),
													m_uikeys_manager(SlPaths::GetUikeys() )
{
	try
	{
		m_pKeyConfigPanel = new wxKeyConfigPanel( this, -1, wxDefaultPosition, wxDefaultSize, (wxKEYBINDER_DEFAULT_STYLE & ~wxKEYBINDER_SHOW_APPLYBUTTON), wxT("HotkeyPanel"),
																			wxT("Selection"), wxCommandEventHandler(hotkey_panel::ButtonAddSelectionCommandClicked),
																			wxT("Custom"), wxCommandEventHandler(hotkey_panel::ButtonAddCustomCommandClicked),
																			wxT("Add Command:") );
		KeynameConverter::initialize();

		UpdateControls();

		//group box
		wxStaticBoxSizer* sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( this, wxID_ANY, wxT("Hotkey Configuration") ), wxVERTICAL );
		sbSizer1->Add( m_pKeyConfigPanel );

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
	catch( const HotkeyException& ex )
	{
		customMessageBox(SS_MAIN_ICON, ex.getMessage(), _("Hotkey Error"), wxOK | wxICON_HAND );
		throw;
	}
}
catch( const HotkeyException& ex )
{
	customMessageBox(SS_MAIN_ICON, ex.getMessage(), _("Hotkey Error"), wxOK | wxICON_HAND );
	throw;
}

hotkey_panel::~hotkey_panel(void)
{
}

void hotkey_panel::ButtonAddSelectionCommandClicked( wxCommandEvent& )
{
	AddSelectionCmdDlg dlg( this );

	if ( dlg.ShowModal() == wxID_OK )
	{
		wxString cmd = wxT("select ") + dlg.getCommandString();
		this->OnAddCommand( cmd );
	}
}

void hotkey_panel::ButtonAddCustomCommandClicked( wxCommandEvent& )
{
	wxString str = wxGetTextFromUser( _("Enter new command:"), _("Add Custom Command") );

	if ( str.size() > 0 )
	{
		this->OnAddCommand( str );
	}
}

void hotkey_panel::OnAddCommand( const wxString& cmd )
{
	try
	{
		CommandList::addCustomCommand( cmd );

		//refresh view
		this->updateTreeView();
		this->addCommandToAllPanelProfiles( CommandList::getCommandByName( cmd ) );
	}
	catch( const HotkeyException& ex )
	{
		customMessageBox(SS_MAIN_ICON, ex.getMessage(), _("Add Command Error"), wxOK | wxICON_HAND );
	}

	//auto-select new command (even try after exception)
	this->m_pKeyConfigPanel->SelectCommand( cmd );
}
/*
bool hotkey_panel::isBindingInProfile( const key_binding_k2c& springprofile, const wxString& command, const wxString& springkey )
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
*/
key_binding hotkey_panel::getBindingsFromProfile( const wxKeyProfile& profile )
{
	//sort the stuff
	typedef std::map<size_t, std::pair<wxString, wxString> >	SortedKeyCommands;
	SortedKeyCommands tmpSorted;
	key_binding binding;

	{	//add non-any keys
		const wxCmdArray* pCmdArr = profile.GetArray();
		for( size_t j=0; j < pCmdArr->GetCount(); ++j )
		{
			const wxCmd& cmd = *pCmdArr->Item(j);
			for( int k=0; k < cmd.GetShortcutCount(); ++k )
			{
				const wxKeyBind* keys = cmd.GetShortcut( k );
				if ( keys->HasAnyModifier() )
					continue;

				tmpSorted[ keys->GetOrderIndex() ] = std::make_pair(
					KeynameConverter::spring2wxKeybinder( keys->GetStr(), true ),
					cmd.GetName() );
			}
		}

		for( SortedKeyCommands::const_iterator iter = tmpSorted.begin(); iter != tmpSorted.end(); ++iter )
		{
			binding.bind( iter->second.second, iter->second.first );
		}
	}

	tmpSorted.clear();
	{	//add any-keys
		const wxCmdArray* pCmdArr = profile.GetArray();
		for( size_t j=0; j < pCmdArr->GetCount(); ++j )
		{
			const wxCmd& cmd = *pCmdArr->Item(j);
			for( int k=0; k < cmd.GetShortcutCount(); ++k )
			{
				const wxKeyBind* keys = cmd.GetShortcut( k );
				if ( !keys->HasAnyModifier() )
					continue;

				tmpSorted[ keys->GetOrderIndex() ] = std::make_pair(
					KeynameConverter::spring2wxKeybinder( keys->GetStr(),true ),
					cmd.GetName() );
			}
		}

		for( SortedKeyCommands::const_iterator iter = tmpSorted.begin(); iter != tmpSorted.end(); ++iter )
		{
			binding.bind( iter->second.second, iter->second.first );
		}
	}

	//add keysyms
	binding.setKeySyms( profile.GetKeySyms() );
	binding.setKeySymsSet( profile.GetKeySymsSet() );
	binding.setMetaKey( KeynameConverter::spring2wxKeybinder( profile.GetMetaKey(), true ) );

	return binding;
}
/*
bool hotkey_panel::isDefaultBinding( const wxString& command, const wxString& springKey )
{
	const key_binding& defBindings = SpringDefaultProfile::getAllBindingsC2K();

	return hotkey_panel::isBindingInProfile( defBindings, command, springKey );
}*/

/*
*	We do only save the bindings that differ from the standard spring keybindings
*/
void hotkey_panel::SaveSettings()
{
	if ( this->m_uikeys_manager.isDontTouchMode() )
		return;

	try
	{
		DeleteHotkeyProfiles();

		wxKeyProfileArray profileArr = m_pKeyConfigPanel->GetProfiles();
		for( size_t i=0; i < profileArr.GetCount(); ++i )
		{
			const wxKeyProfile& profile = *profileArr.Item(i);
			if ( profile.IsNotEditable() )
			{
				//skip build-in profiles
				continue;
			}

			const key_binding profileBinds = hotkey_panel::getBindingsFromProfile( profile );
			const key_binding defaultBinds = SpringDefaultProfile::getBindings();

			//check if any bindings from the default bindings have been unbind. and save that info to the hotkey_panel
			{
				const key_binding removedBinds = defaultBinds - profileBinds;

				const key_commands_sorted& delCmds = removedBinds.getBinds();
				int orderIdx = -1;
				for( key_commands_sorted::const_iterator iter = delCmds.begin(); iter != delCmds.end(); ++iter )
				{
					//check now if this default key binding has been deleted from this profile
					SetHotkey( profile.GetName(), iter->second, iter->first, orderIdx-- );
				}
			}

			//add bindings
			{
				const key_binding addedBinds = profileBinds - defaultBinds;

				const key_commands_sorted& addCmds = addedBinds.getBinds();
				int orderIdx = 1;
				for( key_commands_sorted::const_iterator iter = addCmds.begin(); iter != addCmds.end(); ++iter )
				{
					//only write non-default bindings to hotkey_panel
					SetHotkey( profile.GetName(), iter->second, iter->first, orderIdx++ );
				}
			}

			//add key symbols
			{
				for( key_sym_map::const_iterator iter = profileBinds.getKeySyms().begin(); iter != profileBinds.getKeySyms().end(); ++iter )
				{
					SetHotkeyKeySym( profile.GetName(), iter->first, iter->second );
				}
			}

			//add key set symbols
			{
				for( key_sym_set_map::const_iterator iter = profileBinds.getKeySymsSet().begin(); iter != profileBinds.getKeySymsSet().end(); ++iter )
				{
					SetHotkeyKeySymSet( profile.GetName(), iter->first, iter->second );
				}
			}

			//add meta
			if ( defaultBinds.getMetaKey() != profileBinds.getMetaKey() )
			{
				SetHotkeyMeta( profile.GetName(), profileBinds.getMetaKey() );
			}
		}

		SaveSettings();

		//Write bindings to file
		const wxKeyProfile& selProfile = *this->m_pKeyConfigPanel->GetSelProfile();
		key_binding bindings = hotkey_panel::getBindingsFromProfile( selProfile );

		this->m_uikeys_manager.writeBindingsToFile( bindings );

		this->m_pKeyConfigPanel->ResetProfileBeenModifiedOrSelected();
	}
	catch( const HotkeyException& ex )
	{
		customMessageBox(SS_MAIN_ICON, ex.getMessage(), _("Hotkey SaveSettings error"), wxOK | wxICON_HAND );
	}
}

wxKeyProfile hotkey_panel::buildNewProfile( const wxString& name, const wxString& description, bool readOnly )
{
	wxKeyProfile profile( name, description, readOnly, readOnly );

	const CommandList::CommandMap& commands = CommandList::getCommands();
	for( CommandList::CommandMap::const_iterator iter = commands.begin(); iter != commands.end(); ++iter )
	{
		const CommandList::Command& cmd = iter->second;

		wxSpringCommand* pCmd = new wxSpringCommand( cmd.m_command, cmd.m_description, cmd.m_id );
		profile.AddCmd( pCmd );
	}

	return profile;
}

void hotkey_panel::addCommandToAllPanelProfiles( const CommandList::Command& cmd )
{
	//backup current selected profile id
	int curProfId = this->m_pKeyConfigPanel->GetSelProfileIdx();

	wxKeyProfileArray profileArr = this->m_pKeyConfigPanel->GetProfiles();
	this->m_pKeyConfigPanel->RemoveAllProfiles();
	for( size_t i=0; i < profileArr.GetCount(); ++i )
	{
		wxKeyProfile& profile = *profileArr.Item(i);
		wxSpringCommand* pCmd = new wxSpringCommand( cmd.m_command, cmd.m_description, cmd.m_id );
		profile.AddCmd( pCmd );
		this->m_pKeyConfigPanel->AddProfile( profile );
	}

	//selected the previously selected profile
	this->m_pKeyConfigPanel->SetSelProfile( curProfId );
}

void hotkey_panel::putKeybindingsToProfile( wxKeyProfile& profile, const key_binding& bindings )
{
	const key_commands_sorted& keys = bindings.getBinds();
	for( key_commands_sorted::const_iterator iter = keys.begin(); iter != keys.end(); ++iter )
	{
		profile.AddShortcut( CommandList::getCommandByName(iter->second).m_id,
								KeynameConverter::spring2wxKeybinder( iter->first ) );
	}

	profile.SetKeySyms( bindings.getKeySyms() );
	profile.SetKeySymsSet( bindings.getKeySymsSet() );
	profile.SetMetaKey( KeynameConverter::spring2wxKeybinder( bindings.getMetaKey() ) );
}

wxString hotkey_panel::getNextFreeProfileName()
{
	const wxString profNameTempl = wxT("UserProfile ");
	wxString curProfTryName;
	const wxKeyProfileArray profileArr = this->m_pKeyConfigPanel->GetProfiles();

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
	const key_binding& curBinding = this->m_uikeys_manager.getBindings();

	const wxKeyProfileArray profileArr = this->m_pKeyConfigPanel->GetProfiles();

	static int noProfileFound = -1;
	int foundIdx = noProfileFound;
	for( size_t i=0; i < profileArr.GetCount(); ++i )
	{
		const wxKeyProfile& profile = *profileArr.Item(i);
		const key_binding proBinds = hotkey_panel::getBindingsFromProfile( profile );

 		if ( curBinding == proBinds )
		{
			foundIdx = i;
			break;
		}
	}

	if ( foundIdx == noProfileFound )
	{
		if ( !this->m_uikeys_manager.isDontTouchMode() )
		{
			const wxString profName = this->getNextFreeProfileName();
			wxKeyProfile profile = buildNewProfile( profName, wxT("User hotkey profile"), false );
			this->putKeybindingsToProfile( profile, curBinding );
			this->m_pKeyConfigPanel->AddProfile( profile, true );

			customMessageBox(SS_MAIN_ICON, _("Your current hotkey configuration does not match any known profile.\n A new profile with the name '" + profName + _("' has been created.")),
				_("New hotkey profile found"), wxOK );

			foundIdx = this->m_pKeyConfigPanel->GetProfiles().GetCount() - 1;
		}
		else
		{
			foundIdx = 0;
		}
	}

	this->m_pKeyConfigPanel->SetSelProfile( foundIdx );
}

wxArrayString hotkey_panel::sortArrayStringNumerical( const wxArrayString& arr )
{
	//we get the array here ordered as strings. e.g. "1","10","2","3","4"
	//but we actually do want it ordered as numbers "1","2","3",..."9","10","11","12"
	std::list<long> sortList;
	for( size_t k=0; k < arr.GetCount(); ++k )
	{
		wxString idx = arr.Item(k);
		long v = 0;
		idx.ToLong( &v );
		sortList.push_back( v );
	}
	sortList.sort(); //sort it numerically

	wxArrayString sortArr;
	for( std::list<long>::const_iterator iter = sortList.begin(); iter != sortList.end(); ++iter )
	{
		sortArr.Add( wxString::Format(wxT("%i"),(*iter)) );
	}

	return sortArr;
}

key_binding_collection hotkey_panel::getProfilesFromSettings()
{
	key_binding_collection coll;

	wxArrayString profiles = GetHotkeyProfiles();
	for( size_t i=0; i < profiles.GetCount(); ++i)
	{
		wxString profName = profiles.Item(i);

		//fill with the default bindings
		coll[profName] = SpringDefaultProfile::getBindings();

		//add keybindings
		wxArrayString orderIdxs = hotkey_panel::sortArrayStringNumerical( GetHotkeyProfileOrderIndices( profName ) );
		for( size_t k=0; k < orderIdxs.GetCount(); ++k )
		{
			wxString idx = orderIdxs.Item(k);
			wxArrayString keys = GetHotkeyProfileCommandKeys( profName, idx );
			for( size_t j=0; j < keys.GetCount(); ++j )
			{
				const wxString& cmd = GetHotkey( profName, idx, keys.Item(j) );

				const wxString& springKey = keys.Item(j); //KeynameConverter::spring2wxKeybinder( keys.Item(j), true );

				long lIdx = 0;
				if ( idx.ToLong( &lIdx ) == false )
				{
					throw HotkeyException( _("Error parsing springsettings hotkey profile. Invalid order index: ") + idx );
				}

				if ( lIdx < 0 )
				{
					coll[profName].unbind( cmd, springKey );
				}
				else
				{
					coll[profName].bind( cmd, springKey );
				}
			}
		}

		//add keySyms
		wxArrayString keySyms = GetHotkeyKeySymNames( profName );
		for( size_t k=0; k < keySyms.GetCount(); ++k )
		{
			const wxString& value = GetHotkeyKeySym( profName, keySyms.Item(k) );
			coll[profName].addKeySym( keySyms.Item(k), value );
		}

		//add keySets
		wxArrayString keySymsSet = GetHotkeyKeySymSetNames( profName );
		for( size_t k=0; k < keySymsSet.GetCount(); ++k )
		{
			const wxString& value = GetHotkeyKeySymSet( profName, keySymsSet.Item(k) );
			coll[profName].addKeySymSet( keySymsSet.Item(k), value );
		}

		//add meta
		wxString meta = GetHotkeyMeta( profName );
		if ( meta.size() > 0 )
		{
			coll[profName].setMetaKey( meta );
		}
	}

	return coll;
}

void hotkey_panel::UpdateControls(int /*unused*/)
{
	m_uikeys_manager.setUiKeys(SlPaths::GetUikeys());
	this->updateTreeView();

	//Fetch the profiles
	this->m_pKeyConfigPanel->RemoveAllProfiles();

	//put default profile
	wxKeyProfile defProf = this->buildNewProfile( wxT("Spring default"), wxT("Spring's default keyprofile"),true);
	const key_binding defBinds = SpringDefaultProfile::getBindings();
	this->putKeybindingsToProfile( defProf, defBinds );
	this->m_pKeyConfigPanel->AddProfile( defProf );

	//put user profiles from springsettings configuration
	{
		key_binding_collection profiles = hotkey_panel::getProfilesFromSettings();
		for( key_binding_collection::const_iterator piter = profiles.begin(); piter != profiles.end(); ++piter )
		{
			wxString profName = piter->first;

			//create profile and add bindings
			wxKeyProfile profile = buildNewProfile(profName, wxT("User hotkey profile"),false);
			this->putKeybindingsToProfile( profile, piter->second );

			this->m_pKeyConfigPanel->AddProfile( profile );

			getBindingsFromProfile( *this->m_pKeyConfigPanel->GetProfile(1) );
		}
	}

	this->m_pKeyConfigPanel->ResetProfileBeenModifiedOrSelected();

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
			const wxString profName = piter->first;

			//add keybindings
			const key_commands_sorted& cmds = piter->second.getBinds();
			for( key_commands_sorted::const_iterator iter = cmds.begin(); iter != cmds.end(); ++iter )
			{
				const CommandList::Command& cmd = CommandList::getCommandByName( iter->second );
				ctrlMap[ cmd.m_category ][ cmd.m_command ] = cmd.m_id;
			}
		}
	}

	{	//3. from uikeys.txt
		const key_commands_sorted& uikeys = this->m_uikeys_manager.getBindings().getBinds();

		//key_binding_collection profiles = hotkey_panel::getProfilesFromSettings();
		for( key_commands_sorted::const_iterator iter = uikeys.begin(); iter != uikeys.end(); ++iter )
		{
			const CommandList::Command& cmd = CommandList::getCommandByName( iter->second );
			ctrlMap[ cmd.m_category ][ cmd.m_command ] = cmd.m_id;
		}
	}

	this->m_pKeyConfigPanel->ImportRawList( ctrlMap, wxT("Commands") );
}

bool hotkey_panel::HasProfileBeenModifiedOrSelected() const
{
	if ( this->m_uikeys_manager.isDontTouchMode() )
		return false;

	return this->m_pKeyConfigPanel &&  this->m_pKeyConfigPanel ->HasProfileBeenModifiedOrSelected();
}

void hotkey_panel::ResetProfileBeenModifiedOrSelected()
{
	this->m_pKeyConfigPanel->ResetProfileBeenModifiedOrSelected();
}

//Hotkeys stuff (for springsettings)
void hotkey_panel::SetHotkeyMeta( const wxString& profileName, const wxString& keyStr )
{
	cfg().Write(_T( "/HotkeyProfiles/") + profileName + _T("/Meta"), keyStr);
}

wxString hotkey_panel::GetHotkeyMeta( const wxString& profileName )
{
	return cfg().Read(_T( "/HotkeyProfiles/") + profileName + _T("/Meta"), wxEmptyString );
}

void hotkey_panel::SetHotkeyKeySymSet( const wxString& profileName, const wxString& setName, const wxString& keyStr )
{
	cfg().Write(_T( "/HotkeyProfiles/") + profileName + _T("/KeySets/") + setName, keyStr);
}

wxString hotkey_panel::GetHotkeyKeySymSet( const wxString& profileName, const wxString& setName )
{
	return cfg().Read( _T( "/HotkeyProfiles/") + profileName + _T("/KeySets/") + setName, wxEmptyString );
}

wxArrayString hotkey_panel::GetHotkeyKeySymSetNames( const wxString& profileName )
{
	return cfg().GetEntryList( _T( "/HotkeyProfiles/" ) + profileName + _T("/KeySets/") );
}

void hotkey_panel::SetHotkeyKeySym( const wxString& profileName, const wxString& symName, const wxString& keyStr )
{
	cfg().Write(_T( "/HotkeyProfiles/") + profileName + _T("/KeySyms/") + symName, keyStr);
}

wxString hotkey_panel::GetHotkeyKeySym( const wxString& profileName, const wxString& symName )
{
	return cfg().Read( _T( "/HotkeyProfiles/") + profileName + _T("/KeySyms/") + symName, wxEmptyString );
}

wxArrayString hotkey_panel::GetHotkeyKeySymNames( const wxString& profileName )
{
	return cfg().GetEntryList( _T( "/HotkeyProfiles/" ) + profileName + _T("/KeySyms/") );
}

// oderidx == -1 means unbind
void hotkey_panel::SetHotkey( const wxString& profileName, const wxString& command, const wxString& key, int orderIdx )
{
	cfg().Write(_T( "/HotkeyProfiles/") + profileName + _T("/Bindings/") + (wxFormat(wxT("%i") ) % orderIdx) + _T("/") + key, command );
}

wxString hotkey_panel::GetHotkey( const wxString& profileName, const wxString& orderIdx, const wxString& key )
{
	return cfg().Read( _T( "/HotkeyProfiles/") + profileName + _T("/Bindings/") + orderIdx + _T("/") + key, wxEmptyString );
}

wxArrayString hotkey_panel::GetHotkeyProfiles()
{
	return cfg().GetGroupList( _T( "/HotkeyProfiles/" ) );
}

wxArrayString hotkey_panel::GetHotkeyProfileOrderIndices( const wxString& profileName )
{
	return cfg().GetGroupList( _T( "/HotkeyProfiles/" ) + profileName + _T("/Bindings/") );
}

wxArrayString hotkey_panel::GetHotkeyProfileCommandKeys( const wxString& profileName, const wxString& orderIdx )
{
	return cfg().GetEntryList( _T( "/HotkeyProfiles/" ) + profileName + _T("/Bindings/") + orderIdx + _T("/") );
}

void hotkey_panel::DeleteHotkeyProfiles()
{
	cfg().DeleteGroup( _T( "/HotkeyProfiles/" ) );
}



//END OF Hotkeys stuff (for springsettings)

