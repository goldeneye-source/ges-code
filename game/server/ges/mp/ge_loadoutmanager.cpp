//////////  Copyright © 2008, Goldeneye Source. All rights reserved. ///////////
// 
// File: ge_spawnermanager.cpp
// Description: Manages all spawner types and links them with the loadouts
//
// Created On: 3/22/2008
// Created By: KillerMonkey
///////////////////////////////////////////////////////////////////////////////
#include "cbase.h"

#include "ge_gamerules.h"
#include "gemp_gamerules.h"
#include "gemp_player.h"
#include "ge_spawner.h"
#include "ge_gameplayresource.h"

#include "ge_loadoutmanager.h"
#include "ge_tokenmanager.h"

#include "ge_utils.h"
#include "script_parser.h"
#include "filesystem.h"

#include "ge_playerspawn.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define RANDOM_LOADOUT	"random_loadout"

void GEWeaponSet_Callback( IConVar *var, const char *pOldString, float flOldValue )
{
	static bool noreentrant = false;

	if ( !GEMPRules() || noreentrant )
		return;

	noreentrant = true;
	ConVar *cVar = static_cast<ConVar*>(var);

	if ( !Q_stricmp( cVar->GetString(), RANDOM_LOADOUT ) )
	{
		// Notify of the pending change
		UTIL_ClientPrintAll( HUD_PRINTTALK, "#GES_Weaponset_Random" );
	}
	else
	{
		// Reset the CVar to the old string if we entered a bogus value
		const CGELoadout *pLoadout = GEMPRules()->GetLoadoutManager()->GetLoadout( cVar->GetString() );

		if( !pLoadout )
		{
			Msg("Invalid loadout specified\n");
			cVar->SetValue( pOldString );
		}
		else
		{
			// Notify of the pending change
			UTIL_ClientPrintAll( HUD_PRINTTALK, "#GES_Weaponset_Future", pLoadout->GetPrintName() );
		}
	}

	noreentrant = false;
}

ConVar ge_weaponset( "ge_weaponset", "random_loadout", FCVAR_GAMEDLL, "Changes the weapon set when the round restarts\nUSAGE: ge_weaponset [identity]", GEWeaponSet_Callback );

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

class CGELoadoutParser : public CScriptParser
{
public:
	void SetLoadoutManager( CGELoadoutManager* mgr ) { m_pMgr = mgr; };

protected:
	void Parse( KeyValues *pKeyValuesData, const char *szFileWithoutEXT )
	{
		for ( KeyValues *pKVLoadout = pKeyValuesData->GetFirstSubKey(); pKVLoadout; pKVLoadout = pKVLoadout->GetNextKey() )
		{
			const char *szName = pKVLoadout->GetName();
			if ( !Q_strstr(szName," ") && !Q_strstr(szName,",") )
			{
				//Walk through each loadout and define it.
				CGELoadout *pLoadout = new CGELoadout();

				// Set the Ident and the Print Name
				pLoadout->SetIdent( szName );
				pLoadout->SetPrintName( pKVLoadout->GetString("print_name", pLoadout->GetIdent()) );
				pLoadout->SetWeight( pKVLoadout->GetInt("weight", 0) );
				pLoadout->LoadWeapons( pKVLoadout->FindKey("weapons") );
			
				m_pMgr->AddLoadout( pLoadout );
			}
			else
			{
				DevWarning( "[LoadoutMgr] Loadout ident `%s` contains invalid characters!\n", szName );
			}
		}
	};

	CGELoadoutManager *m_pMgr;

} GELoadoutParser;

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

CGELoadoutManager::CGELoadoutManager( void )
{
	m_pCurrentLoadout = NULL;
	m_bKeepCurrLoadout = false;
}

CGELoadoutManager::~CGELoadoutManager( void )
{
	ClearLoadouts();
}

void CGELoadoutManager::ParseLoadouts( void )
{
	// Remove existing loadouts
	ClearLoadouts();

	// Parsing individually allows us to overwrite the default sets with custom ones
	// Multiple custom sets can be defined as needed (can even make sets per gameplay)
	GELoadoutParser.SetLoadoutManager(this);
	GELoadoutParser.SetHasBeenParsed( false );
	GELoadoutParser.InitParser("scripts/loadouts/weapon_sets_default.X");
	GELoadoutParser.SetHasBeenParsed( false );
	GELoadoutParser.InitParser("scripts/loadouts/weapon_sets_custom*.X");

	// Load gameplay affinity last to ensure we can make links to loadouts
	ParseGameplayAffinity();
}

void CGELoadoutManager::ParseGameplayAffinity( void )
{
	const char *baseDir = "scripts/loadouts/";
	char filePath[256] = "";

	FileFindHandle_t finder;
	const char *filename = filesystem->FindFirstEx( "scripts/loadouts/gameplay_*.txt", "MOD", &finder );

	while ( filename )
	{
		// Chomp off the 'gameplay_'
		char gameplay[32];
		Q_StrSlice( filename, 9, -4, gameplay, 32 );
		Q_strlower( gameplay );

		// Remove any existing entry
		int idx = m_GameplaySets.Find( gameplay );
		if ( idx != m_GameplaySets.InvalidIndex() )
		{
			ClearStringVector( m_GameplaySets[idx]->loadouts );
			delete m_GameplaySets[idx];
			m_GameplaySets.RemoveAt( idx );
		}

		// Load the file
		Q_strncpy( filePath, baseDir, 256 );
		Q_strncat( filePath, filename, 256 );
		char *contents = (char*) UTIL_LoadFileForMe( filePath, NULL );

		if ( contents )
		{
			GameplaySet *pGameplaySet = new GameplaySet;

			CUtlVector<char*> lines;
			CUtlVector<char*> data;
			Q_SplitString( contents, "\n", lines );

			for ( int i=0; i < lines.Count(); i++ )
			{
				// Ignore comments
				if ( !Q_strncmp( lines[i], "//", 2 ) )
					continue;

				// Parse the lines: [loadout]\s+[weight]
				if ( Q_ExtractData( lines[i], data ) )
				{
					if ( IsLoadout( data[0] ) )
					{
						pGameplaySet->loadouts.AddToTail( data[0] );
						if ( data.Count() > 1 )
							pGameplaySet->weights.AddToTail( atoi(data[1]) );
						else
							pGameplaySet->weights.AddToTail( 500 );
					}
				}

				// Clear the data set (do not purge!)
				data.RemoveAll();
			}

			// Make sure we actually loaded something
			if ( pGameplaySet->loadouts.Count() )
				m_GameplaySets.Insert( gameplay, pGameplaySet );
			else
				delete pGameplaySet;

			// NOTE: We do not purge the data!
			ClearStringVector( lines );
		}

		delete [] contents;

		// Get the next one
		filename = filesystem->FindNext( finder );
	}

	filesystem->FindClose( finder );
}

void CGELoadoutManager::AddLoadout( CGELoadout *pNew )
{
	int idx = m_Loadouts.Find( pNew->GetIdent() );
	if ( idx != m_Loadouts.InvalidIndex() )
	{
		// Replace the existing loadout with this one
		delete m_Loadouts[idx];
		m_Loadouts[idx] = pNew;
		DevMsg( "[LoadoutMgr] Loadout was overwritten: %s\n", pNew->GetIdent() );
	}
	else
	{
		// Otherwise insert the new one
		m_Loadouts.Insert( pNew->GetIdent(), pNew );
	}
}

void CGELoadoutManager::ClearLoadouts( void )
{
	m_pCurrentLoadout = NULL;
	m_Loadouts.PurgeAndDeleteElements();

	// Clear out the gameplay set loading
	FOR_EACH_DICT( m_GameplaySets, idx )
		ClearStringVector( m_GameplaySets[idx]->loadouts );

	m_GameplaySets.PurgeAndDeleteElements();
}

bool CGELoadoutManager::IsLoadout( const char* szIdent )
{
	if ( GetLoadout( szIdent ) )
		return true;
	else
		return false;
}

void CGELoadoutManager::GetLoadouts( CUtlVector<CGELoadout*> &loadouts ) {
	loadouts.RemoveAll();
	for ( int i=m_Loadouts.First(); i != m_Loadouts.InvalidIndex(); i=m_Loadouts.Next(i) )
		loadouts.AddToTail( m_Loadouts[i] );
}

CGELoadout *CGELoadoutManager::GetLoadout( const char* szIdent )
{
	int idx = m_Loadouts.Find( szIdent );
	if ( idx != m_Loadouts.InvalidIndex() )
		return m_Loadouts[idx];

	return NULL;
}

bool CGELoadoutManager::SpawnWeapons( void )
{
	if ( m_Loadouts.Count() == 0 )
	{
		Warning("[LoadoutMgr] No loadouts were loaded therefore cannot choose one!\n");
		return false;
	}

	CGELoadout *pNewLoadout = NULL;

	if ( m_pCurrentLoadout != NULL && m_bKeepCurrLoadout )
	{
		// Keep our current loadout and reset the flag
		pNewLoadout = m_pCurrentLoadout;
		m_bKeepCurrLoadout = false;
	}
	else if ( !Q_stricmp( RANDOM_LOADOUT, ge_weaponset.GetString() ) )
	{
		// Build our selection list
		Assert( GEGameplay()->GetScenario() );

		char ident[32] = "\0";
		Q_strncpy( ident, GEGameplay()->GetScenario()->GetIdent(), 32 );
		Q_strlower( ident );

		// First see if we have a set for the current scenario
		int idx = m_GameplaySets.Find( ident );
		if ( idx != m_GameplaySets.InvalidIndex() )
		{
			// We have a set, go through and find a random loadout from the set
			const GameplaySet *set = m_GameplaySets[idx];
			do {
				char* const choice = GERandomWeighted<char* const,int>( set->loadouts.Base(), set->weights.Base(), set->loadouts.Count() );
				pNewLoadout = GetLoadout( choice );
			} while ( set->loadouts.Count() > 1 && pNewLoadout == m_pCurrentLoadout );
		}
		else
		{
			// No set, choose from the global pool (weight != 0)
			CUtlVector<CGELoadout*> loadouts;
			CUtlVector<int> weights;

			GetLoadouts( loadouts );

			for ( int i=0; i < loadouts.Count(); i++ )
				weights.AddToTail( loadouts[i]->GetWeight() );

			do {
				pNewLoadout = GERandomWeighted<CGELoadout*>( loadouts.Base(), weights.Base(), loadouts.Count() );
			} while ( m_Loadouts.Count() > 1 && pNewLoadout == m_pCurrentLoadout );
		}
	}
	else
	{
		// Make sure the referenced loadout exists, otherwise issue a warning and bail out
		if ( !IsLoadout( ge_weaponset.GetString() ) )
		{
			Warning("[LoadoutMgr] Invalid loadout specified, no weapons will be spawned!\n");
			return false;
		}

		pNewLoadout = GetLoadout( ge_weaponset.GetString() );
	}

	if ( !pNewLoadout )
	{
		Warning("[LoadoutMgr] Unable to resolve a weapon loadout!\n");
		return false;
	}

	// If we have changed our weaponset and not yet recorded that do so now
	if ( m_pCurrentLoadout != pNewLoadout )
		UTIL_ClientPrintAll( HUD_PRINTTALK, "#GES_Weaponset_Changed", pNewLoadout->GetPrintName() );

	// Make it official
	m_pCurrentLoadout = pNewLoadout;

	// Remove excess weapons
	RemoveWeapons();

	// Activate current loadout
	m_pCurrentLoadout->Activate();

	// Check for pre-registered tokens
	for ( int i=0; i < MAX_WEAPON_SPAWN_SLOTS; i++ )
	{
		const char *alias = WeaponIDToAlias(m_pCurrentLoadout->GetWeapon(i));
		if ( GEMPRules()->GetTokenManager()->IsValidToken( alias ) )
			m_pCurrentLoadout->SwitchActiveWeapon( i );
	}

	// Populate our spawners
	CUtlVector<EHANDLE> spawners;
	spawners.AddVectorToTail( *(GERules()->GetSpawnersOfType( SPAWN_WEAPON )) );
	spawners.AddVectorToTail( *(GERules()->GetSpawnersOfType( SPAWN_AMMO )) );

	for ( int i=0; i < spawners.Count(); i++ )
	{
		CGESpawner *pSpawner = (CGESpawner*) spawners[i].Get();
		if ( !pSpawner )
			continue;
		// Initialize the spawner entity
		pSpawner->Init();
	}

	g_pGameplayResource->OnLoadoutChanged( m_pCurrentLoadout->GetIdent(), m_pCurrentLoadout->GetPrintName(), m_pCurrentLoadout->GetWeaponList() );
	return true;
}

void CGELoadoutManager::RemoveWeapons( const char *weap /*= NULL*/ )
{
	char search[LOADOUT_WEAP_LEN] = "weapon_*";
	if ( weap )
		Q_strncpy( search, weap, LOADOUT_WEAP_LEN );

	// If we didn't specify a certain weapon strip everything from the players
	FOR_EACH_MPPLAYER( pPlayer )
		if ( !weap )
		{
			pPlayer->RemoveAllItems(false);
			pPlayer->RemoveLeftObjects();
			if ( !GEMPRules()->IsIntermission() )
				pPlayer->GiveDefaultItems();
		}
	END_OF_PLAYER_LOOP()

	CUtlVector<EHANDLE> spawners;
	spawners.AddVectorToTail( *GERules()->GetSpawnersOfType( SPAWN_WEAPON ) );
	for ( int i=0; i < spawners.Count(); i++ )
	{
		CGESpawner *pSpawn = (CGESpawner*) spawners[i].Get();
		if ( pSpawn )
			pSpawn->RemoveEnt();
	}
}

int CGELoadoutManager::GetWeaponInSlot( int slot )
{
	if ( slot >= 0 && slot < MAX_WEAPON_SPAWN_SLOTS )
	{
		if ( CurrLoadout() )
			return CurrLoadout()->GetWeapon( slot );
	}

	return WEAPON_NONE;
}

void CGELoadoutManager::OnTokenAdded( const char *szClassname )
{
	int weapid = AliasToWeaponID( szClassname );

	if ( m_pCurrentLoadout && weapid != WEAPON_NONE )
	{
		CUtlVector<EHANDLE> spawners;
		spawners.AddVectorToTail( *(GERules()->GetSpawnersOfType( SPAWN_WEAPON )) );
		spawners.AddVectorToTail( *(GERules()->GetSpawnersOfType( SPAWN_AMMO )) );

		for ( int i=0; i < MAX_WEAPON_SPAWN_SLOTS; i++ )
		{
			if ( m_pCurrentLoadout->GetWeapon(i) == weapid )
			{
				// Find a new weapon to use (does not change underlying set)
				m_pCurrentLoadout->SwitchActiveWeapon( i );

				// Go through each spawner with this slot and update it's weapon
				for ( int k=0; k < spawners.Count(); k++ )
				{
					CGESpawner *pSpawner = (CGESpawner*) spawners[k].Get();
					if ( !pSpawner || pSpawner->GetSlot() != i )
						continue;
					// Initialize the spawner entity to repopulate the weapon
					pSpawner->Init();
				}
			}
		}
	}
}

bool CGELoadoutManager::GetWeaponSet( CUtlVector<int> &set )
{
	set.RemoveAll();

	if ( CurrLoadout() )
	{
		for ( int i=0; i < MAX_WEAPON_SPAWN_SLOTS; i++ )
			set.AddToTail( CurrLoadout()->GetWeapon( i ) );

		return true;
	}

	return false;
}

void CGELoadoutManager::GetWeaponLists( CUtlVector<int> &vWeaponIds, CUtlVector<int> &vWeaponWeights )
{
	// Weapon lists for choosing randomly
	static CUtlVector<int> sWeaponList;
	static CUtlVector<int> sWeaponWeights;

	if ( !sWeaponList.Count() )
	{
		for ( int i = WEAPON_NONE+1; i < WEAPON_RANDOM_MAX; i++ )
		{
			// Ensure we are a valid weapon
			if ( WeaponIDToAlias(i) )
			{
				sWeaponList.AddToTail( i );
		
				sWeaponWeights.AddToTail( GetRandWeightForWeapon( i ) );
			}
		}
	}

	vWeaponIds.CopyArray( sWeaponList.Base(), sWeaponList.Count() );
	vWeaponWeights.CopyArray( sWeaponWeights.Base(), sWeaponWeights.Count() );
}

CON_COMMAND( ge_weaponset_reload, "Reloads the weaponset script files" )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
	{
		Msg( "You must be a server admin to use this command\n" );
		return;
	}

	if ( GEMPRules() )
		GEMPRules()->GetLoadoutManager()->ParseLoadouts();
}

CON_COMMAND( ge_weaponset_list, "Prints out a list of the available sets" )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
	{
		Msg( "You must be a server admin to use this command\n" );
		return;
	}

	if ( GEMPRules() )
	{
		CGELoadoutManager *pSpwnMgr = GEMPRules()->GetLoadoutManager();

		Msg( "Available Weapon Loadouts: \n" );
		Msg( "--Name--  (--Identity--)\n" );

		// Retrieve a listing of loadouts
		// TODO: this can be sorted and filtered?
		CUtlVector<CGELoadout*> loadouts;
		pSpwnMgr->GetLoadouts( loadouts );

		for ( int i=0; i < loadouts.Count(); i++ )
			Msg( "%s  (%s)\n", loadouts[i]->GetPrintName(), loadouts[i]->GetIdent() );
	}
}
