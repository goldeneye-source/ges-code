///////////// Copyright © 2009 LodleNet. All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_pygameplay.cpp
//   Description :
//      [TODO: Write the purpose of ge_pygameplay.cpp.]
//
//   Created On: 8/24/2009 3:39:13 PM
//   Created By: Mark Chandler <mailto:mark@moddb.com>
////////////////////////////////////////////////////////////////////////////

#include "ge_pyprecom.h"

#include "networkstringtable_gamedll.h"
#include "filesystem.h"
#include "utlbuffer.h"

#include "ge_utils.h"
#include "gemp_gamerules.h"
#include "ge_gameplay.h"
#include "ge_tokenmanager.h"

#include "ge_pymanager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar ge_gameplay_mode;
extern ConVar ge_gameplay;

bool g_bInGameplayReload = false;

class CGEPyScenarioHelp
{
public:
	CGEPyScenarioHelp()
	{
		SetDefLessFunc( m_vHelpPanes );
		ClearHelp();
	}
	~CGEPyScenarioHelp()
	{
		ClearHelp();
	}

	// Generic Setup
	void SetInfo( const char *tagLine, const char *website )
	{
		Q_strncpy( m_szHelpTagLine, tagLine ? tagLine : "", 64 );
		Q_strncpy( m_szHelpWebsite, website ? website : "", 128 );
	}

	void SetDescription( const char *desc )
	{
		int nIndex = g_pStringTableGameplay->FindStringIndex( desc );
		if ( nIndex != INVALID_STRING_INDEX )
		{
			m_iDescription = nIndex;
		}
		else
		{
			bool save = engine->LockNetworkStringTables( false );
			m_iDescription = g_pStringTableGameplay->AddString( true, desc );
			engine->LockNetworkStringTables( save );
		}
	}

	// Adding panes and help items
	int AddPane( const char *id )
	{
		if ( !id || !id[0] )
		{
			Warning( "[ScenarioHelp] Pane attempted creation with no id, ignored\n" );
			return -1;
		}

		if ( FindPane( id ) != m_vHelpPanes.InvalidIndex() )
		{
			Warning( "[ScenarioHelp] Attempted to create duplicate pane %s, ignored\n", id );
			return -1;
		}

		// Create a new index based on our count
		int pane_key = m_vHelpPanes.Count() + 1;
		// A maximum of 3 panes is allowed
		if ( pane_key > 3 )
		{
			Warning( "[ScenarioHelp] No more than 3 help panes are allowed, ignoring pane %i!\n", pane_key );
			return -1;
		}

		sHelpPane *pPane = new sHelpPane;
		pPane->szHelpString[0] = '\0';
		pPane->nHelpString = INVALID_STRING_INDEX;
		pPane->nHelpCount = 0;
		Q_strncpy( pPane->szHelpId, id, 32 );

		m_vHelpPanes.Insert( pane_key, pPane );
		return pane_key;
	}

	void AddHelp( int pane_key, const char *image, const char *desc )
	{
		int index = m_vHelpPanes.Find( pane_key );
		if ( index == m_vHelpPanes.InvalidIndex() )
		{
			Warning( "[ScenarioHelp] Invalid pane id provided to add help to, Pane %i\n", pane_key );
			return;
		}

		sHelpPane *pPane = m_vHelpPanes[ index ];

		if ( pPane->nHelpCount >= 4 )
		{
			Warning( "[ScenarioHelp] No more than 4 help items are allowed per pane, Pane %i\n", pane_key );
			return;
		}

		pPane->nHelpCount++;

		// Restrict string lengths
		char desc_clip[128];
		char image_clip[32];

		Q_strncpy( desc_clip,  (desc && desc[0])  ? desc  : " ", 128 );
		Q_strncpy( image_clip, (image && image[0]) ? image : " ", 32 );

		if ( pPane->szHelpString[0] != '\0' )
			Q_strncat( pPane->szHelpString, ";;;", 1024 );

		// Build and append the string to our help
		char szHelp[256];
		Q_snprintf( szHelp, 256, "%s::%s", desc_clip, image_clip );
		Q_strncat( pPane->szHelpString, szHelp, 1024 );
	}

	// Default pane explicit definition (Default == 1)
	void SetDefaultPane( int pane_key )
	{
		int idx = m_vHelpPanes.Find( pane_key );
		if ( idx != m_vHelpPanes.InvalidIndex() )
			m_iDefaultPane = pane_key;
		else
			Warning( "[ScenarioHelp] Attempted to set invalid pane as default, Pane %i\n", pane_key );
	}

	// Non-Python functions
	int GetDescriptionIndex() { return m_iDescription; }

	void SendHelp( CGEPlayer *pPlayer )
	{
		if ( !pPlayer || pPlayer->IsBotPlayer() )
			return;

		CSingleUserRecipientFilter filter( pPlayer );
		filter.MakeReliable();

		// Send overall scenario help information
		UserMessageBegin( filter, "ScenarioHelp" );
			WRITE_STRING( GEGameplay()->GetScenario()->GetIdent() );
			WRITE_STRING( m_szHelpTagLine );
			WRITE_STRING( m_szHelpWebsite );
			WRITE_BYTE( m_iDefaultPane );
		MessageEnd();

		// Output each help item from each pane
		FOR_EACH_MAP( m_vHelpPanes, idx )
		{
			sHelpPane *pPane = m_vHelpPanes[idx];

			if ( pPane->nHelpString == INVALID_STRING_INDEX )
			{
				bool save = engine->LockNetworkStringTables( false );
				pPane->nHelpString = g_pStringTableGameplay->AddString( true, pPane->szHelpString );
				engine->LockNetworkStringTables( save );
			}

			UserMessageBegin( filter, "ScenarioHelpPane" );
				WRITE_BYTE( m_vHelpPanes.Key(idx) );
				WRITE_STRING( pPane->szHelpId );
				WRITE_SHORT( pPane->nHelpString );
			MessageEnd();
		}
	}

	void ShowHelp( CGEPlayer *pPlayer, bp::object id_or_key )
	{
		if ( !pPlayer || pPlayer->IsBotPlayer() )
			return;

		bp::extract<char*> to_id( id_or_key );
		bp::extract<int> to_key( id_or_key );

		int idx = -1;
		if ( to_key.check() )
			idx = m_vHelpPanes.Find( to_key() );
		else if ( to_id.check() )
			idx = FindPane( to_id() );

		if ( idx == m_vHelpPanes.InvalidIndex() )
		{
			Warning( "[ScenarioHelp] Attempted to show invalid help pane ID to player, Pane: %i\n", m_vHelpPanes.Key(idx) );
			return;
		}

		CSingleUserRecipientFilter filter( pPlayer );
		filter.MakeReliable();

		UserMessageBegin( filter, "ScenarioHelpShow" );
			WRITE_BYTE( m_vHelpPanes.Key(idx) );
		MessageEnd();
	}

	void ClearHelp()
	{
		// Clear the title, etc.
		m_szHelpTagLine[0] = '\0';
		m_szHelpWebsite[0] = '\0';
		m_iDescription = INVALID_STRING_INDEX;
		m_iDefaultPane = 1;

		// Clear our help pane information
		FOR_EACH_MAP( m_vHelpPanes, idx )
			delete m_vHelpPanes[idx];

		m_vHelpPanes.RemoveAll();
	}

	int FindPane( const char *id )
	{
		FOR_EACH_MAP( m_vHelpPanes, idx )
		{
			if ( !Q_stricmp( m_vHelpPanes[idx]->szHelpId, id ) )
				return m_vHelpPanes.Key( idx );
		}
		return m_vHelpPanes.InvalidIndex();
	}

private:
	char m_szHelpTagLine[64];
	char m_szHelpWebsite[128];
	// String table index
	int	 m_iDescription;
	int	 m_iDefaultPane;

	struct sHelpPane
	{
		char szHelpId[32];
		char szHelpString[1024];
		int  nHelpString;
		int	 nHelpCount;
	};

	CUtlMap<int, sHelpPane*> m_vHelpPanes;
};

class CGEPyScenario : public CGEBaseScenario, public bp::wrapper<CGEPyScenario>
{
public:
	CGEPyScenario()
	{
		m_szDescription[0] = '\0';
	}

	~CGEPyScenario()
	{
		DevWarning( 2, "Scenario Deleted!\n" );
	}

	bp::object self;

protected:
	virtual void Init()
	{
		TRYFUNC( this->get_override("OnLoadGamePlay")() );

		// Load scenario help from python
		TRYFUNC( this->get_override("GetScenarioHelp")(bp::ptr(&m_ScenarioHelp)) );
	}

	virtual void Shutdown()
	{
		TRYFUNC( this->get_override("OnUnloadGamePlay")() );

		// Clear us out in case we get reloaded
		m_ScenarioHelp.ClearHelp();

		// Remove our instance
		self = bp::object();
	}

public:
	const char* GetIdent()
	{
		try {
			return bp::extract<const char*>( self.attr( "__class__" ).attr( "__name__" ) );
		} catch ( bp::error_already_set const & ) {
			HandlePythonException();
			return "__NONAME__";
		}
	}

	virtual const char* GetGameDescription()
	{
		if ( m_szDescription[0] )
			return m_szDescription;

		try {
			Q_strncpy( m_szDescription, get_override("GetGameDescription")(), 32 );
			if ( !IsOfficial() )
				Q_strncat( m_szDescription, " (MOD)", 38 );

			return m_szDescription;
		} catch( bp::error_already_set const & ) {
			HandlePythonException();
			return "__NONAME__ (MOD)";
		}
	}

	virtual const char* GetPrintName()
	{
		TRYFUNCRET( this->get_override("GetPrintName")(), "NULL");
	}

	virtual int GetHelpString()
	{
		return m_ScenarioHelp.GetDescriptionIndex();
	}

	// Simple function exposed to Python to show a particular help pane to a player
	virtual void ShowScenarioHelp( CGEPlayer *pPlayer, bp::object id_or_key )
	{
		m_ScenarioHelp.ShowHelp( pPlayer, id_or_key );
	}

	virtual void ClientConnect( CGEPlayer *pPlayer )
	{
		// Send scenario help to this player
		m_ScenarioHelp.SendHelp( pPlayer );

		PY_CALLHOOKS( FUNC_GP_PLAYERCONNECT, bp::make_tuple(bp::ptr(pPlayer)) );
		TRYFUNC( this->get_override("OnPlayerConnect")(bp::ptr(pPlayer)) );
	}

	virtual void ClientDisconnect( CGEPlayer *pPlayer )
	{
		TRYFUNC( this->get_override("OnPlayerDisconnect")(bp::ptr(pPlayer)) );
		PY_CALLHOOKS( FUNC_GP_PLAYERDISCONNECT, bp::make_tuple(bp::ptr(pPlayer)) );
	}

	virtual void OnThink()
	{
		PY_CALLHOOKS( FUNC_GP_THINK, bp::make_tuple() );
		TRYFUNC( this->get_override("OnThink")() );
	}

	virtual void BeforeSetupRound()
	{
		TRYFUNC( this->get_override("BeforeSetupRound")() );
	}

	virtual void OnRoundBegin()
	{
		PY_CALLHOOKS( FUNC_GP_ROUNDBEGIN, bp::make_tuple() );
		TRYFUNC( this->get_override("OnRoundBegin")() );
	}

	virtual void OnRoundEnd()
	{
		PY_CALLHOOKS( FUNC_GP_ROUNDEND, bp::make_tuple() );
		TRYFUNC( this->get_override("OnRoundEnd")() );
	}

	virtual void OnCVarChanged(const char* name, const char* oldvalue, const char* newvalue)
	{
		TRYFUNC( this->get_override("OnCVarChanged")(name, oldvalue, newvalue) );
	}


	virtual bool CanPlayerRespawn(CGEPlayer *pPlayer)
	{
		TRYFUNCRET( this->get_override("CanPlayerRespawn")(bp::ptr(pPlayer)), true );
	}

	virtual bool CanPlayerHaveItem(CGEPlayer *pPlayer, CBaseEntity *pEntity)
	{
		TRYFUNCRET( this->get_override("CanPlayerHaveItem")(bp::ptr(pPlayer), bp::ptr(pEntity)), true );
	}

	virtual bool CanPlayerChangeChar(CGEPlayer* pPlayer, const char* szIdent)
	{
		TRYFUNCRET( this->get_override("CanPlayerChangeChar")(bp::ptr(pPlayer), szIdent), true );
	}

	virtual void CalculateCustomDamage(CGEPlayer *pVictim, const CTakeDamageInfo &inputInfo, float &health, float &armor)
	{
		try {
			bp::tuple tpl = this->get_override("CalculateCustomDamage")(bp::ptr(pVictim), inputInfo, health, armor);
			health = bp::extract<float>(tpl[0]);
			armor = bp::extract<float>(tpl[1]);
		} catch( bp::error_already_set const & ) {
			HandlePythonException();
		}
	}	

	virtual bool ShouldForcePickup(CGEPlayer *pPlayer, CBaseEntity *pEntity)
	{
		TRYFUNCRET( this->get_override("ShouldForcePickup")(bp::ptr(pPlayer), bp::ptr(pEntity)), false );
	}
	
	virtual void OnPlayerSpawn(CGEPlayer *pPlayer)
	{
		PY_CALLHOOKS( FUNC_GP_PLAYERSPAWN, bp::make_tuple(bp::ptr(pPlayer)) );
		TRYFUNC( this->get_override("OnPlayerSpawn")(bp::ptr(pPlayer)) );
	}

	virtual void OnPlayerObserver(CGEPlayer *pPlayer)
	{
		PY_CALLHOOKS( FUNC_GP_PLAYEROBSERVER, bp::make_tuple(bp::ptr(pPlayer)) );
		TRYFUNC( this->get_override("OnPlayerObserver")(bp::ptr(pPlayer)) );
	}

	virtual void OnPlayerKilled(CGEPlayer *pVictim, CGEPlayer *pKiller, CBaseEntity *pWeapon)
	{
		PY_CALLHOOKS( FUNC_GP_PLAYERKILLED, bp::make_tuple(bp::ptr(pVictim), bp::ptr(pKiller), bp::ptr(pWeapon)) );
		TRYFUNC( this->get_override("OnPlayerKilled")(bp::ptr(pVictim), bp::ptr(pKiller), bp::ptr(pWeapon)) );
	}

	virtual bool OnPlayerSay(CGEPlayer* pPlayer, const char* text)
	{
		TRYFUNCRET( this->get_override("OnPlayerSay")(bp::ptr(pPlayer), text), false );
	}

	virtual bool CanPlayerChangeTeam(CGEPlayer *pPlayer, int iOldTeam, int iNewTeam, bool wasForced )
	{
		bool ret = true;
		TRYFUNC( ret = this->get_override("CanPlayerChangeTeam")(bp::ptr(pPlayer), iOldTeam, iNewTeam, wasForced) );
		// Call our hook only if we actually changed teams
		if ( ret )
			PY_CALLHOOKS( FUNC_GP_PLAYERTEAM, bp::make_tuple(bp::ptr(pPlayer), iOldTeam, iNewTeam, wasForced ) );

		return ret;
	}


	virtual void OnCaptureAreaSpawned( CGECaptureArea *pCapture )
	{
		TRYFUNC( this->get_override("OnCaptureAreaSpawned")(bp::ptr(pCapture)) );
	}

	virtual void OnCaptureAreaRemoved( CGECaptureArea *pCapture )
	{
		TRYFUNC( this->get_override("OnCaptureAreaRemoved")(bp::ptr(pCapture)) );
	}

	virtual void OnCaptureAreaEntered( CGECaptureArea *pCapture, CGEPlayer *pPlayer, CGEWeapon *pToken )
	{
		TRYFUNC( this->get_override("OnCaptureAreaEntered")(bp::ptr(pCapture), bp::ptr(pPlayer), bp::ptr(pToken)) );
	}
	
	virtual void OnCaptureAreaExited( CGECaptureArea *pCapture, CGEPlayer *pPlayer )
	{
		TRYFUNC( this->get_override("OnCaptureAreaExited")(bp::ptr(pCapture), bp::ptr(pPlayer)) );
	}

	virtual void OnWeaponSpawned( CGEWeapon *pWeapon )
	{
		PY_CALLHOOKS( FUNC_GP_WEAPONSPAWNED, bp::make_tuple(bp::ptr(pWeapon)) );
		TRYFUNC( this->get_override("OnWeaponSpawned")(bp::ptr(pWeapon)) );
	}

	virtual void OnWeaponRemoved( CGEWeapon *pWeapon )
	{
		PY_CALLHOOKS( FUNC_GP_WEAPONREMOVED, bp::make_tuple(bp::ptr(pWeapon)) );
		TRYFUNC( this->get_override("OnWeaponRemoved")(bp::ptr(pWeapon)) );
	}

	virtual void OnArmorSpawned( CBaseEntity *pArmor )
	{
		PY_CALLHOOKS( FUNC_GP_ARMORSPAWNED, bp::make_tuple(bp::ptr(pArmor)) );
		TRYFUNC( this->get_override("OnArmorSpawned")(bp::ptr(pArmor)) );
	}

	virtual void OnArmorRemoved( CBaseEntity *pArmor )
	{
		PY_CALLHOOKS( FUNC_GP_ARMORREMOVED, bp::make_tuple(bp::ptr(pArmor)) );
		TRYFUNC( this->get_override("OnArmorRemoved")(bp::ptr(pArmor)) );
	}

	virtual void OnAmmoSpawned( CBaseEntity *pAmmo )
	{
		PY_CALLHOOKS( FUNC_GP_AMMOSPAWNED, bp::make_tuple(bp::ptr(pAmmo)) );
		TRYFUNC( this->get_override("OnAmmoSpawned")(bp::ptr(pAmmo)) );
	}

	virtual void OnAmmoRemoved( CBaseEntity *pAmmo )
	{
		PY_CALLHOOKS( FUNC_GP_AMMOREMOVED, bp::make_tuple(bp::ptr(pAmmo)) );
		TRYFUNC( this->get_override("OnAmmoRemoved")(bp::ptr(pAmmo)) );
	}

	virtual void OnTokenSpawned( CGEWeapon *pToken )
	{
		TRYFUNC( this->get_override("OnTokenSpawned")(bp::ptr(pToken)) );
	}

	virtual void OnTokenRemoved( CGEWeapon *pToken )
	{
		TRYFUNC( this->get_override("OnTokenRemoved")(bp::ptr(pToken)) );
	}

	virtual void OnTokenPicked( CGEWeapon *pToken, CGEPlayer *pPlayer )
	{
		TRYFUNC( this->get_override("OnTokenPicked")(bp::ptr(pToken), bp::ptr(pPlayer)) );
	}

	virtual void OnTokenDropped( CGEWeapon *pToken, CGEPlayer *pPlayer )
	{
		TRYFUNC( this->get_override("OnTokenDropped")(bp::ptr(pToken), bp::ptr(pPlayer)) );
	}

	virtual void OnTokenAttack( CGEWeapon *pToken, CGEPlayer *pPlayer, Vector position, Vector forward )
	{
		TRYFUNC( this->get_override("OnTokenAttack")(bp::ptr(pToken), bp::ptr(pPlayer), position, forward) );
	}

	virtual void OnEnemyTokenTouched(CGEWeapon *pToken, CGEPlayer *pPlayer)
	{
		TRYFUNC( this->get_override("OnEnemyTokenTouched")(bp::ptr(pToken), bp::ptr(pPlayer)) );
	}

	virtual bool CanRoundEnd()
	{
		TRYFUNCRET( this->get_override("CanRoundEnd")(), true );
	}

	virtual bool CanMatchEnd()
	{
		TRYFUNCRET( this->get_override("CanMatchEnd")(), true );
	}

	virtual int GetTeamPlay()
	{
		TRYFUNCRET( this->get_override("GetTeamPlay")() , 0 );
	}

private:
	// Cached gameplay desc (might include (MOD) if it is not official)
	char m_szDescription[38];
	// Full featured help module
	CGEPyScenarioHelp m_ScenarioHelp;
};


class CGEGameplayManager : public CGEBaseGameplayManager, public IPythonListener, public bp::wrapper<CGEGameplayManager>
{
public:
	CGEGameplayManager()
	{
		m_BlankScenario = new CGEPyScenario();
		LoadScenarioMD5File();
	}
	
	~CGEGameplayManager()
	{
		delete m_BlankScenario;
		m_Scenario = bp::object();
	}

	virtual void Init()
	{
		try
		{
			// Call down to initialize everything
			CGEBaseGameplayManager::Init();
		}
		catch (bp::error_already_set const &)
		{
			// Something didn't load properly, notify the user
			HandlePythonException();
			Warning( "Scenario failed to load on initialization!\n" );

			try
			{
				// Try to load the deathmatch scenario (default)
				LoadScenario( "deathmatch" );
			}
			catch (bp::error_already_set const &)
			{
				// Something went terribly wrong, we cannot proceed
				HandlePythonException();
				AssertFatalMsg( false, "Failed to load deathmatch scenario, check your files!" );
			}
		}
	}

	virtual CGEBaseScenario* GetScenario()
	{
		if ( IsValidScenario() )
		{
			try {
				return bp::extract<CGEPyScenario*>( m_Scenario );
			} catch( bp::error_already_set const & ) {
				HandlePythonException();
			}
		}

#ifdef _DEBUG
		AssertMsg( false, "Using the default scenario!" );
#endif

		return m_BlankScenario;
	}

	virtual bool IsValidScenario()
	{
		return m_Scenario.is_none() == false;
	}

	virtual bool DoLoadScenario( const char* ident )
	{
		CGEPyScenario *pScenario = NULL;
		bp::object scenario;

		try
		{
			// Load the scenario from Python Gameplay Manager
			scenario = bp::call<bp::object>( this->get_override("LoadScenario").ptr(), ident );
			
			// Check for load failure
			if ( scenario.is_none() )
				return false;

			// Extract the C++ instance
			pScenario = bp::extract<CGEPyScenario*>( scenario );
		}
		catch (bp::error_already_set const &)
		{
			HandlePythonException();
			return false;
		}
		
		// Officially set the new scenario
		m_Scenario = scenario;
		pScenario->self = scenario;

		// Realign our ident
		ident = pScenario->GetIdent();

		// Check to see if this scenario is "official"
		CUtlBuffer buf;
		char md5hash[33] = "nogood";
		bool is_official = false;

		char pyFile[128];
		Q_snprintf( pyFile, 128, "%s\\ges\\GamePlay\\%s.py", GEPy()->GetRootPath(), ident );

		// Calculate the MD5 sum of the Python file
		if ( filesystem->ReadFile( pyFile, "MOD", buf ) )
			GEUTIL_MD5( (char*)buf.Base(), md5hash, 33 );

		// Compare the MD5 hash to our known hash list
		for( int i=0; i < bp::len( m_md5Hashes ); i++ )
		{
			if ( Q_stricmp( bp::extract<const char*>(m_md5Hashes[i]), md5hash ) == 0 )
			{
				is_official = true;
				break;
			}
		}

		DevMsg( "Scenario MD5 Sum: %s (%s)\n", md5hash, is_official ? "OFFICIAL" : "MODDED" );

		// Internally mark the scenario as official or not
		GetScenario()->SetIsOfficial( is_official );

		return true;
	}

	void LoadScenarioMD5File( void )
	{
		// Clear out the MD5 list
		m_md5Hashes = bp::list();

		KeyValues *pKV = new KeyValues("Hashes" );
		char *szFilename = "python/gphashes.txt";

		CUtlBuffer buffer;
		// Decrypt the file so we can read it
		GEUTIL_DEncryptFile( szFilename, GERules()->GetEncryptionKey(), false );
		pKV->LoadFromFile( filesystem, szFilename, "MOD" );
		// Don't let people edit their file while in game
		GEUTIL_DEncryptFile( szFilename, GERules()->GetEncryptionKey() );

		KeyValues *pKey = pKV->GetFirstSubKey();
		if ( pKey )
		{
			while ( pKey )
			{
				if ( !Q_strcasecmp(pKey->GetName(), "hash") )
					m_md5Hashes.append(pKey->GetString());

				pKey = pKey->GetNextKey();
			}
		}
	}

private:
	// Storing the python instance of the loaded scenario
	bp::object m_Scenario;
	// A "blank" scenario to prevent null pointers
	CGEPyScenario *m_BlankScenario;
	// The scenario MD5 list
	bp::list m_md5Hashes;
};

CGEPyScenario *pyGetScenario( void )
{
	return (CGEPyScenario*) GEGameplay()->GetScenario();
}

BOOST_PYTHON_MODULE(GEGamePlay)
{
	bp::def("GetScenario", pyGetScenario, bp::return_value_policy<bp::reference_existing_object>());

	bp::class_<CGEPyScenario, boost::noncopyable>("CBaseScenario")
		.def("GetIdent", &CGEPyScenario::GetIdent)
		.def("CreateCVar", &CGEPyScenario::CreateCVar)
		.def("ShowScenarioHelp", &CGEPyScenario::ShowScenarioHelp);

	bp::class_<CGEPyScenarioHelp, boost::noncopyable>("CScenarioHelp", bp::no_init)
		.def("SetInfo", &CGEPyScenarioHelp::SetInfo)
		.def("SetDescription", &CGEPyScenarioHelp::SetDescription)
		.def("SetDefaultPane", &CGEPyScenarioHelp::SetDefaultPane)
		.def("AddPane", &CGEPyScenarioHelp::AddPane)
		.def("AddHelp", &CGEPyScenarioHelp::AddHelp);

	bp::class_<CGEGameplayManager>("CGamePlayManager");
}

CGEBaseGameplayManager *g_GamePlay = NULL;

void CreateGameplayManager()
{
	if ( GEGameplay() )
	{
		DevWarning( "GamePlay Manager already exists!\n" );
		return;
	}

	try
	{
		// Load our manager
		bp::object init = bp::import( "GESInit" );
		bp::object gpmgr = init.attr( "LoadManager" )( "GamePlayManager" );

		// Extract the manager
		g_GamePlay = bp::extract<CGEGameplayManager*>( gpmgr );
	}
	catch (bp::error_already_set const &)
	{
		HandlePythonException();
		AssertFatal( "Failed to load python game play manager! Check the Python log in 'scripts/python'!" );
	}

	if ( GEGameplay() )
		GEGameplay()->Init();
	else
		AssertFatal( "Failed to load python game play manager! Check the Python log in 'scripts/python'!" );
}

void ShutdownGameplayManager()
{
	if ( GEGameplay() )
		GEGameplay()->Shutdown();

	try {
		bp::object init = bp::import( "GESInit" );
		init.attr( "UnloadManager" )( "GamePlayManager" );
	} catch (bp::error_already_set const &) { }

	g_GamePlay = NULL;
}

// THIS IS A DEVELOPER COMMAND, IT DOES NOTHING FOR THE GAME ITSELF
// LEAVE IN FOR PEOPLE MAKING THEIR OWN PYTHON GAMEPLAY SO THEY CAN TEST EASILY
CON_COMMAND(ge_gameplayreload, "Reloads the current gameplay (FOR DEVELOPERS!)")
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
	{
		Msg( "You must be a server admin to use that command\n" );
		return;
	}

	if ( !GEGameplay() )
	{
		Warning( "Gameplay is not active!\n" );
		return;
	}

	g_bInGameplayReload = true;

	// Cache our mode and set to "no rotate"
	int mode = ge_gameplay_mode.GetInt();
	ge_gameplay_mode.SetValue( 0 );

	// Try to extract a passed in scenario for loading
	if ( args.ArgC() > 1 )
	{
		char gameplay[32];
		Q_strncpy( gameplay, args.Arg(1), 32 );

		if ( GEGameplay()->IsValidGamePlay( gameplay ) )
		{
			// A valid gameplay was specified, so load this after our reboot
			ge_gameplay.SetValue( gameplay );
		}
	}

	// Perform the reboot
	ShutdownGameplayManager();
	CreateGameplayManager();

	// Set back our mode
	ge_gameplay_mode.SetValue( mode );

	g_bInGameplayReload = false;
}
