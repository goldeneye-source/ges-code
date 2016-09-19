///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_client.cpp
// Description:
//      Defines intial spawn properties for connecting clients
//
// Created On: 23 Feb 08, 19:35
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ge_player.h"
#include "gemp_gamerules.h"
#include "ge_ai.h"
#include "entitylist.h"
#include "physics.h"
#include "game.h"
#include "player_resource.h"
#include "engine/IEngineSound.h"
#include "team.h"
#include "viewport_panel_names.h"
#include "cdll_int.h"

#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void Host_Say( edict_t *pEdict, bool teamonly );

extern CBaseEntity*	FindPickerEntityClass( CBasePlayer *pPlayer, char *classname );
extern bool			g_fGameOver;
extern ConVar		ge_teamplay;

#ifdef GES_TESTING
// Define global testing status variable
static bool g_bIsTesting = false;
bool IsTesting() { return g_bIsTesting; }

// Pull in our tests from the ServerTest.lib
bool RunTests();
CON_COMMAND( ge_run_tests, "Runs tests" )
{
	g_bIsTesting = true;

	Msg( "Running google tests...\n" );
	if ( RunTests() )
		Msg( "Completed running all tests\n" );
	else
		Warning( "You can only run tests once per session!\n" );

	g_bIsTesting = false;
}
#endif

void FinishClientPutInServer( CGEPlayer *pPlayer )
{
	if (pPlayer)
		pPlayer->FinishClientPutInServer();
}

void ClientPutInServer( edict_t *pEdict, const char *playername )
{
	player_info_t plInfo;
	engine->GetPlayerInfo( engine->IndexOfEdict(pEdict), &plInfo );

	CGEPlayer *pPlayer = NULL;

	if ( plInfo.fakeplayer && !Q_stristr( plInfo.name, "oldbot" ) )
		pPlayer = CGEPlayer::CreatePlayer( "bot_player", pEdict );
	else
		pPlayer = CGEPlayer::CreatePlayer( "player", pEdict );

	pPlayer->SetPlayerName( playername );
}

void ClientActive( edict_t *pEdict, bool bLoadGame )
{
	// Can't load games in CS!
	Assert( !bLoadGame );

	CGEPlayer *pPlayer = ToGEPlayer( CBaseEntity::Instance( pEdict ) );
	GERules()->ClientActive( pPlayer );
	FinishClientPutInServer( pPlayer );
}

const char *GetGameDescription()
{
	if ( g_pGameRules ) // this function may be called before the world has spawned, and the game rules initialized
		return g_pGameRules->GetGameDescription();
	else
		return "GoldenEye: Source";
}

CBaseEntity* FindEntity( edict_t *pEdict, char *classname )
{
	// If no name was given set bits based on the picked
	if ( FStrEq(classname, "") )
		return FindPickerEntityClass( static_cast<CBasePlayer*>(GetContainingEntity(pEdict)), classname );
	
	return NULL;
}

void ClientGamePrecache( void )
{
	CBaseEntity::PrecacheModel( "models/player.mdl" );
	CBaseEntity::PrecacheModel( "models/gibs/agibs.mdl" );
	CBaseEntity::PrecacheModel( "models/weapons/v_hands.mdl" );

	// Used by the popout help to indicate a message is shown
	CBaseEntity::PrecacheScriptSound( "GEPlayer.HudNotify" );
	CBaseEntity::PrecacheScriptSound( "GEPlayer.FallDamage" );
}


// called by ClientKill and DeadThink
extern ConVar ge_respawndelay;
void respawn( CBaseEntity *pEdict, bool fCopyCorpse )
{
	CGEPlayer *pPlayer = ToGEPlayer( pEdict );

	if ( pPlayer )
	{
		if ( gpGlobals->curtime > pPlayer->GetDeathTime() + MAX(DEATH_ANIMATION_TIME,ge_respawndelay.GetFloat()) )
		{		
			// respawn player
			pPlayer->Spawn();			
		}
		else
		{
			pPlayer->SetNextThink( gpGlobals->curtime + 0.1f );
		}
	}
}

void GameStartFrame( void )
{
	VPROF("GameStartFrame()");

#ifdef GES_ENABLE_OLD_BOTS
	extern void Bot_RunAll();
	Bot_RunAll();
#endif
}

void GEHostName_Callback( IConVar *var, const char *pOldString, float flOldValue )
{
	static bool denyReentrant = false;
	if ( denyReentrant || !GEMPRules() )
		return;

	ConVar *cVar = static_cast<ConVar*>(var);

	IGameEvent *event = gameeventmanager->CreateEvent( "hostname_change" );
	if ( event )
	{
		event->SetString( "hostname", cVar->GetString() );
		gameeventmanager->FireEvent( event );
	}
}

void InstallGameRules()
{
	ConVar *cvHostName = cvar->FindVar("hostname");
	cvHostName->InstallChangeCallback( GEHostName_Callback );

	CreateGameRulesObject( "CGEMPRules" );

	// Force a set value to make sure we write the modded string if we are modded
	cvHostName->SetValue( cvHostName->GetString() );
}

#ifdef _DEBUG
#include "networkstringtable_gamedll.h"

CON_COMMAND( ge_debug_dumpstringtable, "Dumps string table contents" )
{
	if ( args.ArgC() < 2 )
	{
		Msg( "-------------------------------------------------\n" );
		Msg( "\tNetwork String Table Dump\n" );
		Msg( "-------------------------------------------------\n" );

		int table_count = networkstringtable->GetNumTables();
		Msg( "Table Count: %i\n", table_count );

		for ( int i=0; i < table_count; i++ )
		{
			INetworkStringTable *table = networkstringtable->GetTable( i );
			if ( table )
				Msg( "%i: %s (%i/%i)\n", i, table->GetTableName(), table->GetNumStrings(), table->GetMaxStrings() );
		}
	}
	else
	{
		Msg( "-------------------------------------------------\n" );
		Msg( "\tString Table Dump of %s\n", args[1] );
		Msg( "-------------------------------------------------\n" );

		INetworkStringTable *table = networkstringtable->FindTable( args[1] );
		if ( table )
		{
			int max_index = table->GetMaxStrings();
			for ( int idx = 0; idx < max_index; idx++ )
			{
				const char *str = table->GetString( idx );
				if ( str )
					Msg( "%i: %s\n", idx, str );
			}
		}
		else
		{
			Warning( "String table not found!\n" );
		}
	}
	

	Msg( "-------------------------------------------------\n" );
	Msg( "-------------------------------------------------\n" );
}
#endif
