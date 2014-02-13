#include "slpaths.h"

#include "../nonportable.h"
#include "../helper/slconfig.h"
#include "platform.h"
#include "conversion.h"
#include "debug.h"

#include <wx/string.h>
#include <wx/filename.h>
#include <wx/log.h>

#include <lslunitsync/unitsync.h>
#include <lslunitsync/c_api.h> //FIXME: unitsync.h should be only used, as it caches calls to unitsync, calls to unitsync are blocking!
#include <lslutils/config.h>

#include "pathlistfactory.h"

wxString SlPaths::m_user_defined_config_path = wxEmptyString;

const wxChar sep = wxFileName::GetPathSeparator();
const wxString sepstring = wxString(sep);


wxString SlPaths::GetLocalConfigPath ()
{
	return IdentityString( GetExecutableFolder() + sepstring + _T( "%s.conf" ), true );
}

wxString SlPaths::GetDefaultConfigPath ()
{
	return IdentityString( GetConfigfileDir() + sepstring + _T( "%s.conf" ), true );
}

bool SlPaths::IsPortableMode ()
{
	if (!m_user_defined_config_path.empty()) {
		return false;
	}
	return wxFileName::FileExists( GetLocalConfigPath() );
}

wxString SlPaths::GetConfigPath ()
{
	if (!m_user_defined_config_path.empty()) {
		return m_user_defined_config_path;
	}
	return IsPortableMode() ?
		   GetLocalConfigPath() : GetDefaultConfigPath();
}

wxString SlPaths::GetCachePath()
{
	wxString path = GetLobbyWriteDir() + sepstring + _T( "cache" ) + sep;
	if ( !wxFileName::DirExists( path ) ) {
		if ( !wxFileName::Mkdir(  path, 0755  ) ) return wxEmptyString;
	}
	return path;
}

// ========================================================
std::map<wxString, LSL::SpringBundle> SlPaths::m_spring_versions;

std::map<wxString, LSL::SpringBundle> SlPaths::GetSpringVersionList()
{
	return m_spring_versions;
}

bool SlPaths::LocateSystemInstalledSpring(LSL::SpringBundle& bundle)
{
	wxPathList paths = PathlistFactory::ConfigFileSearchPaths();
	for (const auto path: paths) {
		if (bundle.AutoComplete(STD_STRING(path))) {
			return true;
		}
	}
	return false;
}


void SlPaths::RefreshSpringVersionList(bool autosearch, const LSL::SpringBundle* additionalbundle)
{
	/*
	FIXME: move to LSL's GetSpringVersionList() which does:

		1. search system installed spring + unitsync (both paths independant)
		2. search for user installed spring + unitsync (assume bundled)
		3. search / validate paths from config
		4. search path / unitsync given from user input

	needs to change to sth like: GetSpringVersionList(std::list<LSL::Bundle>)

	*/
	wxLogDebugFunc( wxEmptyString );
	std::list<LSL::SpringBundle> usync_paths;

	if (additionalbundle != NULL) {
		usync_paths.push_back(*additionalbundle);
	}

	if (autosearch) {
		if (SlPaths::IsPortableMode()) {
			wxPathList localPaths;
			PathlistFactory::EnginePaths(localPaths, GetLobbyWriteDir());
			for (const auto path: localPaths) {
				LSL::SpringBundle bundle;
				bundle.path = STD_STRING(path);
				usync_paths.push_back(bundle);
			}
		} else {
			LSL::SpringBundle systembundle;
			if (LocateSystemInstalledSpring(systembundle)) {
				usync_paths.push_back(systembundle);
			}

			wxPathList ret;
			wxPathList paths = PathlistFactory::AdditionalSearchPaths(ret);
			for (const auto path: paths) {
				LSL::SpringBundle bundle;
				bundle.path = STD_STRING(path);
				usync_paths.push_back(bundle);
			}
		}
	}

	wxArrayString list = cfg().GetGroupList( _T( "/Spring/Paths" ) );
	const int count = list.GetCount();
	for ( int i = 0; i < count; i++ ) {
		LSL::SpringBundle bundle;
		bundle.unitsync = STD_STRING(GetUnitSync(list[i]));
		bundle.spring = STD_STRING(GetSpringBinary(list[i]));
		bundle.version = STD_STRING(list[i]);
		usync_paths.push_back(bundle);
	}

	cfg().DeleteGroup(_T("/Spring/Paths"));

	m_spring_versions.clear();
	try {
		const auto versions = LSL::usync().GetSpringVersionList( usync_paths );
		for(const auto pair : versions) {
			const LSL::SpringBundle& bundle = pair.second;
			const wxString version = TowxString(bundle.version);
			m_spring_versions[version] = bundle;
			SetSpringBinary(version, TowxString(bundle.spring));
			SetUnitSync(version, TowxString(bundle.unitsync));
			SetBundle(version, TowxString(bundle.path));
		}
	} catch (const std::runtime_error& e) {
		wxLogError(wxString::Format(_T("Couldn't get list of spring versions: %s"), e.what()));
	} catch ( ...) {
		wxLogError(_T("Unknown Execption caught in SlPaths::RefreshSpringVersionList"));
	}
}

wxString SlPaths::GetCurrentUsedSpringIndex()
{
	return cfg().Read( _T( "/Spring/CurrentIndex" ), _T( "default" ) );
}

void SlPaths::SetUsedSpringIndex( const wxString& index )
{
	cfg().Write( _T( "/Spring/CurrentIndex" ), TowxString(index) );
	// reconfigure unitsync in case it is reloaded
	LSL::Util::config().ConfigurePaths(
		boost::filesystem::path(STD_STRING(SlPaths::GetCachePath())),
		boost::filesystem::path(STD_STRING(SlPaths::GetUnitSync())),
		boost::filesystem::path(STD_STRING(SlPaths::GetSpringBinary()))
	);
}


void SlPaths::DeleteSpringVersionbyIndex( const wxString& index )
{
	cfg().DeleteGroup( _T( "/Spring/Paths/" ) + index );
	if ( GetCurrentUsedSpringIndex() == index ) SetUsedSpringIndex( wxEmptyString );
}


wxString SlPaths::GetSpringConfigFilePath(const wxString& /*FIXME: implement index*/)
{
	wxString path;
	try {
		path = TowxString(LSL::susynclib().GetConfigFilePath());
	} catch ( std::runtime_error& e) {
		wxLogError( wxString::Format( _T("Couldn't get SpringConfigFilePath, exception caught:\n %s"), e.what()  ) );
	}
	return path;
}

wxString SlPaths::GetUnitSync( const wxString& index )
{
	return cfg().Read( _T( "/Spring/Paths/" ) + index + _T( "/UnitSyncPath" ), wxEmptyString );
}

wxString SlPaths::GetSpringBinary( const wxString& index )
{
	return cfg().Read( _T( "/Spring/Paths/" ) + index + _T( "/SpringBinPath" ), wxEmptyString );
}

void SlPaths::SetUnitSync( const wxString& index, const wxString& path )
{
	cfg().Write( _T( "/Spring/Paths/" ) + index + _T( "/UnitSyncPath" ), path );
	// reconfigure unitsync in case it is reloaded
	LSL::Util::config().ConfigurePaths(
		boost::filesystem::path(STD_STRING(SlPaths::GetCachePath())),
		boost::filesystem::path(STD_STRING(SlPaths::GetUnitSync())),
		boost::filesystem::path(STD_STRING(SlPaths::GetSpringBinary()))
	);
}

void SlPaths::SetSpringBinary( const wxString& index, const wxString& path )
{
	cfg().Write( _T( "/Spring/Paths/" ) + index + _T( "/SpringBinPath" ), path );
}

void SlPaths::SetBundle( const wxString& index, const wxString& path )
{
	cfg().Write( _T( "/Spring/Paths/" ) + index + _T( "/SpringBundlePath" ), path );
}

wxString SlPaths::GetChatLogLoc()
{
	wxString path = GetLobbyWriteDir() +  sepstring + _T( "chatlog" );
	if ( !wxFileName::DirExists( path ) ) {
		if ( !wxFileName::Mkdir(  path, 0755  ) ) return wxEmptyString;
	}
	return path;
}


bool SlPaths::IsSpringBin( const wxString& path )
{
	if ( !wxFile::Exists( path ) ) return false;
	if ( !wxFileName::IsFileExecutable( path ) ) return false;
	return true;
}

wxString SlPaths::GetEditorPath( )
{
#if defined(__WXMSW__)
	wxString def = wxGetOSDirectory() + sepstring + _T("system32") + sepstring + _T("notepad.exe");
	if ( !wxFile::Exists( def ) )
		def = wxEmptyString;
#else
	wxString def = wxEmptyString;
#endif
	return cfg().Read( _T( "/GUI/Editor" ) , def );
}

void SlPaths::SetEditorPath( const wxString& path )
{
	cfg().Write( _T( "/GUI/Editor" ) , path );
}


wxString SlPaths::GetLobbyWriteDir()
{
	//FIXME: make configureable
	if (IsPortableMode()) {
		return GetExecutableFolder();
	}
	return GetConfigfileDir();
}

wxString SlPaths::GetUikeys(const wxString& index)
{
	const wxString path = GetDataDir(index);
	wxString uikeys(path);
	uikeys += _T("uikeys.txt");
	return uikeys;
}

//! copy uikeys.txt
void CopyUikeys(const wxString& currentDatadir )
{
    wxString uikeyslocation = PathlistFactory::UikeysLocations().FindValidPath( _T("uikeys.txt") );
    if ( !uikeyslocation.IsEmpty() )
    {
        wxCopyFile( uikeyslocation, currentDatadir + wxFileName::GetPathSeparator() + _T("uikeys.txt"), false );
    }
}


bool SlPaths::mkDir(const wxString& dir) {
	return wxFileName::Mkdir(dir, 0, wxPATH_MKDIR_FULL);
}

bool SlPaths::CreateSpringDataDir(const wxString& dir)
{
	if ( dir.IsEmpty() ) {
		return false;
	}
	if ( !mkDir(dir) ||
			!mkDir(dir + sep + _T("games") ) ||
			!mkDir(dir + sep + _T("maps") ) ||
			!mkDir(dir + sep + _T("demos") ) ||
			!mkDir(dir + sep + _T("screenshots"))) {
		return false;
	}
	if (LSL::usync().IsLoaded()) {
		LSL::usync().SetSpringDataPath(STD_STRING(dir));
	}
	CopyUikeys( SlPaths::GetDataDir() );
	return true;
}

wxString SlPaths::GetDataDir(const wxString& /*FIXME: implement index */)
{
	std::string dir;
	if (LSL::usync().GetSpringDataPath(dir))
		return TowxString(dir);
	return wxEmptyString;
}
