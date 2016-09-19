///////////// Copyright 2008, Jonathan White. All rights reserved. /////////////
// 
// File: gemp_gamerules.cpp
// Description:
//      GoldenEye Game Rules for Multiplayer
//
// Created On: 28 Feb 08, 22:55
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#include "cbase.h"

//#include "viewport_panel_names.h"
#include "ammodef.h"
#include "utlbuffer.h"

#include "ge_shareddefs.h"
#include "ge_utils.h"
#include "ge_game_timer.h"

#ifdef CLIENT_DLL
	#include "c_ge_player.h"
	#include "c_ge_gameplayresource.h"
#else
	#include "ge_ai.h"
	#include "ai_network.h"
	#include "npc_gebase.h"
	#include "ge_gameplay.h"
	#include "eventqueue.h"
	#include "items.h"
	#include "voice_gamemgr.h"

	#include "gemp_player.h"
	#include "ge_weapon.h"
	#include "grenade_gebase.h"
	#include "team.h"
	#include "ge_gameinterface.h"
	#include "ge_playerspawn.h"
	#include "ge_playerresource.h"
	#include "ge_radarresource.h"
	#include "ge_gameplayresource.h"
	#include "ge_tokenmanager.h"
	#include "ge_loadoutmanager.h"
	#include "ge_mapmanager.h"
	#include "ge_stats_recorder.h"

	#include "ge_triggers.h"
	#include "ge_door.h"

	extern void respawn(CBaseEntity *pEdict, bool fCopyCorpse);
	extern void ClientActive( edict_t *pEdict, bool bLoadGame );

	class CGEReloadEntityFilter;

	extern ConVar ge_weaponset;
	extern ConVar ge_roundtime;

	extern CBaseEntity *g_pLastMI6Spawn;
	extern CBaseEntity *g_pLastJanusSpawn;

	void GERoundTime_Callback( IConVar *var, const char *pOldString, float flOldValue );
	void GERoundCount_Callback( IConVar *var, const char *pOldString, float flOldValue );
	void GETeamplay_Callback( IConVar *var, const char *pOldString, float flOldValue );
	void GEBotThreshold_Callback( IConVar *var, const char *pOldString, float fOldValue );
	void GEVelocity_Callback( IConVar *var, const char *pOldString, float fOldValue );
	void GEInfAmmo_Callback(IConVar *var, const char *pOldString, float fOldValue);
	void GEAlertCode_Callback(IConVar *var, const char *pOldString, float fOldValue);
#endif

#include "gemp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define GES_DEFAULT_SPAWNINVULN		3.0f
#define GES_DEFAULT_SPAWNBREAK		true

// -----------------------
// Console Variables
//
extern ConVar fraglimit;
extern ConVar mp_chattime;

ConVar ge_allowradar		( "ge_allowradar", "1", FCVAR_REPLICATED|FCVAR_NOTIFY, "Allow clients to use their radars." );
ConVar ge_allowjump			( "ge_allowjump",  "1", FCVAR_REPLICATED|FCVAR_NOTIFY, "Toggles allowing players to jump." );
ConVar ge_startarmed		( "ge_startarmed", "1", FCVAR_REPLICATED|FCVAR_NOTIFY, "Configures the armed level of spawned players [0, 1, 2]" );
ConVar ge_paintball			( "ge_paintball",  "0", FCVAR_REPLICATED|FCVAR_NOTIFY, "The famous paintball mode." );

ConVar ge_teamautobalance	( "ge_teamautobalance", "1", FCVAR_REPLICATED|FCVAR_NOTIFY, "Turns on the auto balancer for teamplay" );
ConVar ge_tournamentmode	( "ge_tournamentmode",	"0", FCVAR_REPLICATED|FCVAR_NOTIFY, "Turns on tournament mode that disables certain gameplay checks" );
ConVar ge_respawndelay		( "ge_respawndelay",	"6", FCVAR_REPLICATED, "Changes the minimum delay between respawns" );

ConVar ge_itemrespawntime	( "ge_itemrespawntime",	  "10", FCVAR_REPLICATED|FCVAR_NOTIFY, "Time in seconds between ammo respawns (ge_dynamicweaponrespawn must be off!)" );
ConVar ge_weaponrespawntime	( "ge_weaponrespawntime", "10", FCVAR_REPLICATED|FCVAR_NOTIFY, "Time in seconds between weapon respawns (ge_dynamicweaponrespawn must be off!)" );

ConVar ge_dynweaponrespawn		 ( "ge_dynamicweaponrespawn", "1", FCVAR_REPLICATED, "Changes the respawn delay for weapons and ammo to be based on how many players are connected" );
ConVar ge_dynweaponrespawn_scale ( "ge_dynamicweaponrespawn_scale", "1.0", FCVAR_REPLICATED, "Changes the dynamic respawn delay scale for weapons and ammo" );

ConVar ge_radar_range			 ( "ge_radar_range", "1500", FCVAR_REPLICATED|FCVAR_NOTIFY, "Change the radar range (in inches), default is 125ft" );
ConVar ge_radar_showenemyteam	 ( "ge_radar_showenemyteam", "1", FCVAR_REPLICATED|FCVAR_NOTIFY, "Allow the radar to show enemies during teamplay (useful for tournaments)" );

#ifdef GAME_DLL
// Bot related console variables
ConVar ge_bot_threshold			( "ge_bot_threshold", "0", FCVAR_REPLICATED|FCVAR_NOTIFY, "Server tries to maintain the number of players given using bots if needed, use anything less than 2 to disable this mechanism", GEBotThreshold_Callback );
ConVar ge_bot_openslots			( "ge_bot_openslots", "0", FCVAR_REPLICATED, "Number of open slots to leave for incoming players." );
ConVar ge_bot_strict_openslot	( "ge_bot_strict_openslot", "0", FCVAR_REPLICATED, "Count spectators in determining whether to leave an open player slot." );

ConVar ge_roundtime	( "ge_roundtime", "0", FCVAR_REPLICATED, "Round time in seconds that can be played.", true, 0, true, 3000, GERoundTime_Callback );
ConVar ge_rounddelay( "ge_rounddelay", "10", FCVAR_GAMEDLL, "Delay, in seconds, between rounds.", true, 2, true, 40 );
ConVar ge_roundcount( "ge_roundcount", "0", FCVAR_REPLICATED, "Number of rounds that should be held in the given match time (calculates ge_roundtime), use 0 to disable", GERoundCount_Callback );
ConVar ge_teamplay	( "ge_teamplay", "0", FCVAR_REPLICATED, "Turns on team play if the current scenario supports it.", GETeamplay_Callback );
ConVar ge_velocity	( "ge_velocity", "1.0", FCVAR_REPLICATED|FCVAR_NOTIFY, "Player movement velocity multiplier, applies in multiples of 0.25 [0.7 to 2.0]", true, 0.7, true, 2.0, GEVelocity_Callback );
ConVar ge_infiniteammo( "ge_infiniteammo", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Players never run out of ammo!", GEInfAmmo_Callback );
ConVar ge_nextnextmap( "ge_nextnextmap", "", FCVAR_GAMEDLL, "Map to switch to immediately upon loading next map");
ConVar ge_alertcodeoverride("ge_alertcodeoverride", "-1", FCVAR_GAMEDLL, "Override alert code with this value. bit 1 = desynch spread, bit 2 = use mersine twister, bit 3 = check player speed", GEAlertCode_Callback);
ConVar ge_votekickpreference("ge_votekickpreference", "-1", FCVAR_GAMEDLL, "-1 = allow alertcode to dictate votekick behavior, 0 = disable, 1 = enable.");

void GEAlertCode_Callback(IConVar *var, const char *pOldString, float flOldValue)
{
	ConVar *cVar = static_cast<ConVar*>(var);
	int value = cVar->GetInt();

	if ( value >= 0 )
		iAlertCode = value;
}

void GERoundTime_Callback( IConVar *var, const char *pOldString, float flOldValue )
{
	static bool in_func = false;
	if ( in_func || !GEMPRules() )
		return;

	// Guard against recursion
	in_func = true;
	// Grab the desired round time
	ConVar *cVar = static_cast<ConVar*>(var);
	int round_time = cVar->GetInt();

#if !defined(_DEBUG) && !defined(GES_TESTING)
	if ( round_time > 0 && round_time < 60 )
	{
		round_time = 60;
		cVar->SetValue( round_time );
		Warning( "Minimum round time is 60 seconds (use 0 to disable rounds)!\n" );
	}
#endif

	GEMPRules()->ChangeRoundTimer( round_time );

	// Reset our guard
	in_func = false;
}

void GERoundCount_Callback( IConVar *var, const char *pOldString, float flOldValue )
{
	ConVar *cVar = static_cast<ConVar*>(var);
	int num_rounds = cVar->GetInt();

	// If numrounds is 0 then ignore this and use ge_roundtime
	if (num_rounds <= 0) {
		return;
	} else if ( num_rounds == 1 ) {
		// Set our round time to the match time since we only want 1 round
		ge_roundtime.SetValue( mp_timelimit.GetInt() * 60 );
	} else {
		// Figure out the amount of inter-match round delay to account for it
		int match_time = mp_timelimit.GetInt() * 60;
		int total_round_delay = MAX( ge_rounddelay.GetInt(), 3 ) * (num_rounds - 1);
		int round_time = ge_roundtime.GetInt();

		// If our match time exceeds the total round delay, divide the diff by the number of desired rounds
		if ( match_time > total_round_delay )
			round_time = (match_time - total_round_delay) / num_rounds;
		else
			round_time = MAX( atoi( ge_roundtime.GetDefault() ), round_time );

		// Set the round time based on our calcs, ge_roundtime ensures at least 30 second rounds
		ge_roundtime.SetValue( round_time );
	}
}

void GETeamplay_Callback( IConVar *var, const char *pOldString, float flOldValue )
{
	// Don't announce if we are just going to ignore the change
	if ( GEMPRules() && GEMPRules()->GetTeamplayMode() != TEAMPLAY_TOGGLE )
		return;

	ConVar *cVar = static_cast<ConVar*>(var);
	UTIL_ClientPrintAll( HUD_PRINTTALK, "#GES_Teamplay_Announce", cVar->GetBool() ? "#Enabled" : "#Disabled" );
}

void GEVelocity_Callback( IConVar *var, const char *pOldString, float fOldValue )
{
	static bool denyReentrant = false;
	if ( denyReentrant || !GEMPRules() )
		return;

	denyReentrant = true;
	
	ConVar *cVar = static_cast<ConVar*>(var);
	float value = cVar->GetFloat();

	// Make sure we only go to the nearest 0.1 value if < 1 or 0.25 value if > 1
	if ( value < 1.0f )
		value = floor( value * 10.0f + 0.5f ) / 10.0f;
	else
		value = floor( value * 4.0f + 0.5f ) / 4.0f;

	cVar->SetValue( value );

	Msg( "Velocity set to %0.2f times normal speed!\n", value );

	FOR_EACH_PLAYER(pPlayer)
		pPlayer->SetMaxSpeed( GE_NORM_SPEED * GEMPRules()->GetSpeedMultiplier(pPlayer) );
	END_OF_PLAYER_LOOP()

	denyReentrant = false;
}

void GEInfAmmo_Callback(IConVar *var, const char *pOldString, float flOldValue)
{
	if (!GEMPRules())
		return;

	ConVar *cVar = static_cast<ConVar*>(var);

	bool newstate = cVar->GetBool();

	GEMPRules()->SetGlobalInfAmmoState(newstate);

	if (!newstate)
		return;

	const char* ammotypes[13] = { AMMO_9MM, AMMO_RIFLE, AMMO_BUCKSHOT, AMMO_MAGNUM, AMMO_GOLDENGUN, AMMO_MOONRAKER, AMMO_ROCKET, AMMO_SHELL, AMMO_TIMEDMINE, AMMO_REMOTEMINE, AMMO_TKNIFE, AMMO_GRENADE, AMMO_PROXIMITYMINE };
	// Somewhat dinky way of still allowing ammo pickups for mines/knives/grenades during inf ammo.  Just give the player one less than the max!
	int ammoamounts[13] = { AMMO_9MM_MAX, AMMO_RIFLE_MAX, AMMO_BUCKSHOT_MAX, AMMO_MAGNUM_MAX, AMMO_GOLDENGUN_MAX, AMMO_MOONRAKER_MAX, AMMO_ROCKET_MAX,
							AMMO_SHELL_MAX, AMMO_TIMEDMINE_MAX - 1, AMMO_REMOTEMINE_MAX - 1, AMMO_TKNIFE_MAX - 1, AMMO_GRENADE_MAX - 1, AMMO_PROXIMITYMINE_MAX - 1};

	FOR_EACH_PLAYER(pPlayer)
		pPlayer->RemoveAllAmmo(); // First strip their ammo so if someone spazzes out on the console command they don't end up with max ammo for throwables.

		for (int i = 0; i < 13; i++)
		{
			pPlayer->GiveAmmo(ammoamounts[i], GetAmmoDef()->Index(ammotypes[i]), true);
		}
	END_OF_PLAYER_LOOP()
}

void GEBotThreshold_Callback( IConVar *var, const char *pOldString, float fOldValue )
{
	static bool denyreentrant = false;
	if ( denyreentrant || !GEMPRules() )
		return;

	denyreentrant = true;
	ConVar *cVar = (ConVar*) var;
	if ( cVar->GetInt() < 2 )
	{
		engine->ServerCommand( "ge_bot_remove\n" );
		cVar->SetValue( 0 );
	}
	denyreentrant = false;
}

CON_COMMAND( ge_bot, "Creates a full fledged AI Bot" )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
	{
		Msg( "You must be a server admin to use that command\n" );
		return;
	}

	CGEMPRules::CreateBot();
}

CON_COMMAND( ge_bot_remove, "Removes the number of specified bots, if no number is supplied it removes them all!" )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	int to_remove = -1;
	if ( args.ArgC() > 1 )
		to_remove = atoi( args[1] );
	
	FOR_EACH_BOTPLAYER( pBot )
		if ( to_remove != 0 )
		{
			// Try to kick it, otherwise remove outright
			if ( !CGEMPRules::RemoveBot( pBot->GetPlayerName() ) )
				UTIL_Remove( pBot );

			--to_remove;
		}
	END_OF_PLAYER_LOOP()
}

#endif // GAME_DLL


// -----------------------
// Network Definitions
//
REGISTER_GAMERULES_CLASS( CGEMPRules );
LINK_ENTITY_TO_CLASS( gemp_gamerules, CGEMPGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( GEMPGameRulesProxy, DT_GEMPGameRulesProxy )

BEGIN_NETWORK_TABLE_NOBASE( CGEMPRules, DT_GEMPRules )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iRandomSeedOffset ) ),
	RecvPropBool( RECVINFO( m_bTeamPlayDesired ) ),
	RecvPropBool( RECVINFO( m_bGlobalInfAmmo )),
	RecvPropBool( RECVINFO( m_bGamemodeInfAmmo )),
	RecvPropInt( RECVINFO( m_iTeamplayMode ) ),
	RecvPropInt( RECVINFO( m_iScoreboardMode )),
	RecvPropInt( RECVINFO( m_iAwardEventCode )),
	RecvPropFloat(RECVINFO( m_flMapFloorHeight )),
	RecvPropEHandle( RECVINFO( m_hMatchTimer ) ),
	RecvPropEHandle( RECVINFO( m_hRoundTimer ) ),
#else
	SendPropInt( SENDINFO( m_iRandomSeedOffset )),
	SendPropBool( SENDINFO( m_bTeamPlayDesired ) ),
	SendPropBool( SENDINFO( m_bGlobalInfAmmo ) ),
	SendPropBool( SENDINFO( m_bGamemodeInfAmmo ) ),
	SendPropInt( SENDINFO( m_iTeamplayMode ) ),
	SendPropInt( SENDINFO( m_iScoreboardMode ) ),
	SendPropInt( SENDINFO( m_iAwardEventCode )),
	SendPropFloat( SENDINFO( m_flMapFloorHeight )),
	SendPropEHandle( SENDINFO( m_hMatchTimer ) ),
	SendPropEHandle( SENDINFO( m_hRoundTimer ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
	void RecvProxy_GEMPGameRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
	{
		CGEMPRules *pRules = GEMPRules();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CGEMPGameRulesProxy, DT_GEMPGameRulesProxy )
		RecvPropDataTable( "gemp_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_GEMPRules ), RecvProxy_GEMPGameRules )
	END_RECV_TABLE()
#else
	void *SendProxy_GEMPGameRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
	{
		CGEMPRules *pRules = GEMPRules();
		Assert( pRules );
		pRecipients->SetAllRecipients();
		return pRules;
	}

	BEGIN_SEND_TABLE( CGEMPGameRulesProxy, DT_GEMPGameRulesProxy )
		SendPropDataTable( "gemp_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_GEMPRules ), SendProxy_GEMPGameRules )
	END_SEND_TABLE()
#endif


// -----------------------
// Misc Functions
//
#ifndef CLIENT_DLL
char *sTeamNames[] = {
	"Unassigned",
	"Spectator",
	"MI6",
	"Janus",
};

CUtlVector<string_t> g_vBotNames;

#define GE_CHANGELEVEL_DELAY 1.0f

static int BalanceTeamSort(  CGEMPPlayer * const *p1, CGEMPPlayer * const *p2 )
{
	static const float eps = 5.0f;
	// Bots get no love
	if ( (*p1)->IsBot() ) return -1;
	if ( (*p2)->IsBot() ) return 1;
	// Otherwise sort by join time, then by # of team swaps if w/i eps
	float diff = (*p2)->GetTeamJoinTime() - (*p1)->GetTeamJoinTime();
	if ( abs(diff) < eps )
		return ( (*p1)->GetAutoTeamSwaps() - (*p2)->GetAutoTeamSwaps() );
	else
		return (int) diff;
}
#endif

// Global store of player count last map for teamplay considerations
int g_iLastPlayerCount = 0;

// -----------------------
// CGEMPRules Implementation
//
CGEMPRules::CGEMPRules()
{
#ifndef CLIENT_DLL
	// Create the team managers
	for ( int i = 0; i < ARRAYSIZE( sTeamNames ); i++ )
	{
		CTeam *pTeam = static_cast<CTeam*>(CreateEntityByName( "team_manager" ));
		pTeam->Init( sTeamNames[i], i );
		g_Teams.AddToTail( pTeam );
	}

	m_bTeamPlayDesired	 = false;
	m_bInTeamBalance	 = false;
	m_bUseTeamSpawns	 = false;
	m_bSwappedTeamSpawns = false;

	m_bGlobalInfAmmo = false;
	m_bGamemodeInfAmmo = false;

	m_bAllowSuperfluousAreas = true;

	m_bEnableAmmoSpawns	 = true;
	m_bEnableArmorSpawns = true;
	m_bEnableWeaponSpawns= true;

	m_iScoreboardMode = 0;

	m_iAwardEventCode = 0;

	m_iPlayerWinner = m_iTeamWinner = 0;

	m_flIntermissionEndTime = 0;
	m_flNextTeamBalance		= 0;
	m_flChangeLevelTime		= 0;
	m_flNextBotCheck		= 0;
	m_flNextIntermissionCheck = 0;

	m_flSpawnInvulnDuration = GES_DEFAULT_SPAWNINVULN;
	m_bSpawnInvulnCanBreak = GES_DEFAULT_SPAWNBREAK;

	m_flMapFloorHeight = 125.0;
	m_iRandomSeedOffset = 0;

	m_szNextLevel[0] = '\0';
	m_szGameDesc[0] = '\0';

	// Find "sv_alltalk" convar and set the value accordingly
	m_pAllTalkVar = cvar->FindVar( "sv_alltalk" );

	// Copy over our override value.
	if (ge_alertcodeoverride.GetInt() >= 0)
		iAlertCode = ge_alertcodeoverride.GetInt();

	// Create entity resources
	g_pRadarResource = (CGERadarResource*) CBaseEntity::Create( "radar_resource", vec3_origin, vec3_angle );
	g_pGameplayResource = (CGEGameplayResource*) CBaseEntity::Create( "gameplay_resource", vec3_origin, vec3_angle );

	// Create timers
	m_hMatchTimer = (CGEGameTimer*) CBaseEntity::Create( "ge_game_timer", vec3_origin, vec3_angle );
	m_hRoundTimer = (CGEGameTimer*) CBaseEntity::Create( "ge_game_timer", vec3_origin, vec3_angle );

	// Make sure we setup our match time
	HandleTimeLimitChange();

	// Create the token manager
	m_pTokenManager = new CGETokenManager;

	// Create the weapon loadout manager
	m_pLoadoutManager = new CGELoadoutManager;
	m_pLoadoutManager->ParseLoadouts();

	// Create map manager
	m_pMapManager = new CGEMapManager;
	m_pMapManager->ParseMapSelectionData();

	// Figure out what day it is
	//int day, month, year;
	//GetCurrentDate(&day, &month, &year);

	// Load our bot names
	g_vBotNames.RemoveAll();
	CUtlBuffer buf;
	buf.SetBufferType( true, false );
	if ( filesystem->ReadFile( "scripts/bot_names.txt", "MOD", buf ) )
	{
		char line[32];
		buf.GetLine( line, 32 );
		while ( line[0] )
		{
			// Skip over blank lines and anything starting with less than a space
			if ( Q_strlen(line) > 0 && line[0] > 32 )
			{
				// Remove the \n and put us in the bot name register
				line[ Q_strlen(line)-1 ] = '\0';
				g_vBotNames.AddToTail( AllocPooledString( line ) );
			}

			buf.GetLine( line, 32 );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CGEMPRules::~CGEMPRules()
{
#ifdef GAME_DLL
	// Note, don't delete each team since they are in the gEntList and will 
	// automatically be deleted from there, instead.
	g_vBotNames.RemoveAll();
	g_Teams.Purge();
	delete m_pLoadoutManager;
	delete m_pTokenManager;

	// These are deleted with the rest of the entities
	g_pPlayerResource = NULL;
	g_pRadarResource = NULL;

	// Shutdown the Python Managers
	ShutdownAiManager();
	ShutdownGameplayManager();
#endif
}

#ifdef GAME_DLL
void CGEMPRules::OnGameplayEvent( GPEvent event )
{
	switch( event )
	{
	case SCENARIO_INIT:
		OnScenarioInit();
		break;

	case MATCH_START:
		OnMatchStart();
		break;

	case ROUND_START:
		OnRoundStart();
		break;

	case ROUND_END:
		OnRoundEnd();
		break;
	}
}

void CGEMPRules::OnScenarioInit()
{
	// Reset the token manager
	GetTokenManager()->Reset();

	// Reset our states
	SetAmmoSpawnState( true );
	SetWeaponSpawnState( true );
	SetArmorSpawnState( true );
	SetTeamSpawn( false ); // Only CTF uses teamspawns nowadays.

	SetGlobalInfAmmoState( ge_infiniteammo.GetBool() );
	SetGamemodeInfAmmoState( false );
	SetScoreboardMode( 0 ); //Default points.

	SetSpawnInvulnInterval( GES_DEFAULT_SPAWNINVULN );
	SetSpawnInvulnCanBreak( GES_DEFAULT_SPAWNBREAK );
	SetSuperfluousAreasState( true ); // Superlous areas allowed by default.

	// Make sure the round timer is enabled
	// NOTE: The match timer can not be disabled
	SetRoundTimerEnabled( true );

	// Reset the radar
	g_pRadarResource->SetForceRadar( false );
	g_pRadarResource->DropAllContacts();

	// Reset the excluded characters
	g_pGameplayResource->SetCharacterExclusion( "" );
}

void CGEMPRules::OnMatchStart()
{
	// Reset player round/match scores and stats
	ResetPlayerScores( true );
	ResetTeamScores( true );
	GEStats()->ResetMatchStats();

	// Set our teamplay variable (force it)
	SetTeamplay( ge_teamplay.GetBool(), true );

	// Reload bot's brains based on the current gameplay
	FOR_EACH_BOTPLAYER( pBot )
		pBot->LoadBrain();
	END_OF_PLAYER_LOOP()

	// Reset this value since we already checked it in ge_gameplay.cpp
	g_iLastPlayerCount = 0;
}

void CGEMPRules::OnRoundStart()
{
	// Drop all contacts
	g_pRadarResource->DropAllContacts();

	// Remove all left over tokens from the field
	GetTokenManager()->RemoveTokens();
	GetTokenManager()->RemoveCaptureAreas();

	// Spawn weapon set, gameplay tokens, and capture areas
	GetLoadoutManager()->SpawnWeapons();

	// Reset player spawn metrics
	CUtlVector<EHANDLE> spawners;
	spawners.AddVectorToTail( *GetSpawnersOfType( SPAWN_PLAYER ) );
	spawners.AddVectorToTail( *GetSpawnersOfType( SPAWN_PLAYER_MI6 ) );
	spawners.AddVectorToTail( *GetSpawnersOfType( SPAWN_PLAYER_JANUS ) );

	for ( int i=0; i < spawners.Count(); i++ )
		((CGESpawner*) spawners[i].Get())->Init();

	// Reset team round scores
	ResetTeamScores( false );
	// Reset round stats
	GEStats()->ResetRoundStats();

	// Reset winner information
	SetRoundWinner( 0 );
	SetRoundTeamWinner( TEAM_UNASSIGNED );

	// Update the teamplay global variable to make sure it's right.
	// If the game thinks it's teamplay when it isn't, it won't do any clientside prediction since it thinks both players
	// are on the same team and shouldn't be able to damage each other.
	gpGlobals->teamplay = IsTeamplay();

	// Start the round timer
	StartRoundTimer( ge_roundtime.GetFloat() );

	// Spawn these once everything is settled
	GetTokenManager()->SpawnTokens();
	GetTokenManager()->SpawnCaptureAreas();
}

void CGEMPRules::OnRoundEnd()
{
	// Stop the round timer
	m_hRoundTimer->Stop();

	// Add round score to the match score for teams and players

	FOR_EACH_MPPLAYER( pPlayer )
		// HACK HACK: Fake a kill so we can record their inning time and weapon held time
		GEStats()->Event_PlayerKilled( pPlayer, CTakeDamageInfo() );

		// Add the player's round scores to their match scores
		pPlayer->AddMatchScore( pPlayer->GetRoundScore() );
		pPlayer->AddMatchDeaths( pPlayer->DeathCount() );
	END_OF_PLAYER_LOOP()

	// Add the team round scores to the match scores
	for ( int i = FIRST_GAME_TEAM; i < MAX_GE_TEAMS; i++ )
	{
		CTeam *team = GetGlobalTeam( i );
		if ( team )
			team->AddMatchScore( team->GetRoundScore() );
	}

	// Make sure we capture the latest scores and send them to the clients
	if ( g_pPlayerResource )
		g_pPlayerResource->UpdatePlayerData();

	// Calculate the player's favorite weapons and set them
	GEStats()->SetFavoriteWeapons();
}

// This is called prior to OnRoundStart from GEGameplay
// the baseclass handles the actual reload, this class
// handles special particulars like swapping team spawns
void CGEMPRules::SetupRound()
{
	// Purge the trap list so it can be reconstructed on world reload.
	m_vTrapList.RemoveAll();

	// Get the system time for the random seed selector.
	tm sysTime;
	VCRHook_LocalTime(&sysTime);

	// Pick a new random seed offset so that any entities using it won't just get the same value over and over again.
	// Pgameplay and date affect this.
	m_iRandomSeedOffset = GetSpawnInvulnInterval() + sysTime.tm_sec + sysTime.tm_min * 60 + sysTime.tm_hour * 3600;

	// Reload the world entities (unless protected)
	WorldReload();

	// Swap the team spawns
	SwapTeamSpawns();

	// Setup teamplay from our scenario
	SetTeamplayMode( GetScenario()->GetTeamPlay() );
	SetTeamplay( ge_teamplay.GetBool() );
}

bool CGEMPRules::AmmoShouldRespawn()
{
	return m_bEnableAmmoSpawns;
}

bool CGEMPRules::ArmorShouldRespawn()
{
	return m_bEnableArmorSpawns;
}

bool CGEMPRules::WeaponShouldRespawn( const char *classname )
{
	if ( Q_stristr( classname, "mine" ) || 
		 !Q_stricmp( classname, "weapon_grenade" ) || 
		 !Q_stricmp( classname, "weapon_knife_throwing" ) )
		return false;

	return m_bEnableWeaponSpawns;
}

bool CGEMPRules::ItemShouldRespawn( const char *name )
{
	return true;
}

//Tell our items to respawn where they originated from!
Vector CGEMPRules::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->GetOriginalSpawnOrigin();
}

QAngle CGEMPRules::VecItemRespawnAngles( CItem *pItem )
{
	return pItem->GetOriginalSpawnAngles();
}

float CGEMPRules::FlItemRespawnTime( CItem *pItem )
{
	// Use dynamic respawn calculation if provided
	if ( ge_dynweaponrespawn.GetBool() )
		return (15.4 - 1.8 * sqrt( (float)MAX(GetNumAlivePlayers(), 8) )) * ge_dynweaponrespawn_scale.GetFloat();

	// Otherwise return the static delay
	return ge_itemrespawntime.GetFloat();
}

float CGEMPRules::FlWeaponRespawnTime( CBaseCombatWeapon *pWeapon )
{
	// Use dynamic respawn calculation if provided
	if ( ge_dynweaponrespawn.GetBool() )
		return (15.4 - 1.8 * sqrt( (float)MAX(GetNumAlivePlayers(), 8) )) * ge_dynweaponrespawn_scale.GetFloat();

	// Otherwise return the static delay
	return ge_weaponrespawntime.GetFloat();
}

bool CGEMPRules::FPlayerCanRespawn( CBasePlayer *pPlayer )
{
	// Don't respawn if we are in an intermission
	if ( IsIntermission() )
		return false;

	return BaseClass::FPlayerCanRespawn( pPlayer );
}

void CGEMPRules::CreateStandardEntities()
{
	// Create the player resource
	g_pPlayerResource = (CGEPlayerResource*) CBaseEntity::Create( "player_manager", vec3_origin, vec3_angle );
	g_pPlayerResource->AddEFlags( EFL_KEEP_ON_RECREATE_ENTITIES );

	//Setup our network entity
	CBaseEntity::Create( "gemp_gamerules", vec3_origin, vec3_angle );
}

void CGEMPRules::RemoveWeaponsFromWorld( const char *szClassName /*= NULL*/ )
{
	m_pLoadoutManager->RemoveWeapons( szClassName );
}

int CGEMPRules::GetNumActivePlayers()
{
	if ( m_iNumActivePlayers == -1 )
	{
		m_iNumActivePlayers = 0;

		FOR_EACH_MPPLAYER( pPlayer )
			if ( pPlayer->IsActive() )
				m_iNumActivePlayers++;
		END_OF_PLAYER_LOOP()
	}

	return m_iNumActivePlayers;
}

int CGEMPRules::GetNumAlivePlayers()
{
	if ( m_iNumAlivePlayers == -1 )
	{
		m_iNumAlivePlayers = 0;

		FOR_EACH_MPPLAYER( pPlayer )
			if ( !pPlayer->IsObserver() )
				m_iNumAlivePlayers++;
		END_OF_PLAYER_LOOP()
	}
	
	return m_iNumAlivePlayers;
}

int CGEMPRules::GetNumInRoundPlayers()
{
	if ( m_iNumInRoundPlayers == -1 )
	{
		m_iNumInRoundPlayers = 0;

		FOR_EACH_MPPLAYER( pPlayer )
			if ( pPlayer->IsInRound() )
				m_iNumInRoundPlayers++;
		END_OF_PLAYER_LOOP()
	}

	return m_iNumInRoundPlayers;
}

int CGEMPRules::GetRoundTeamWinner()
{
	int winnerid = TEAM_UNASSIGNED;
	bool game_over = GEGameplay()->IsInFinalIntermission();

	if ( !IsTeamplay() )
		return winnerid;

	// See if we set the team winner already from python
	if ( m_iTeamWinner >= FIRST_GAME_TEAM && m_iTeamWinner < MAX_GE_TEAMS  && !game_over )
		return m_iTeamWinner;

	if ( game_over )
	{
		if ( g_Teams[TEAM_MI6]->GetMatchScore() > g_Teams[TEAM_JANUS]->GetMatchScore() )
			winnerid = TEAM_MI6;
		else if ( g_Teams[TEAM_MI6]->GetMatchScore() < g_Teams[TEAM_JANUS]->GetMatchScore() )
			winnerid = TEAM_JANUS;
	}
	else
	{
		if ( g_Teams[TEAM_MI6]->GetRoundScore() > g_Teams[TEAM_JANUS]->GetRoundScore() )
			winnerid = TEAM_MI6;
		else if ( g_Teams[TEAM_MI6]->GetRoundScore() < g_Teams[TEAM_JANUS]->GetRoundScore() )
			winnerid = TEAM_JANUS;
	}

	return winnerid;
}

int CGEMPRules::GetRoundWinner()
{
	bool game_over = GEGameplay()->IsInFinalIntermission();
	int score  = 0, maxscore  = 0, 
		deaths = 0, maxdeaths = 0, 
		winnerid = 0;

	if ( UTIL_PlayerByIndex( m_iPlayerWinner ) && !game_over )
		return m_iPlayerWinner;

	FOR_EACH_MPPLAYER( pPlayer )

		// Gather scores based on our match state
		if ( game_over )
		{
			score = pPlayer->GetMatchScore();
			deaths = pPlayer->GetMatchDeaths();
		}
		else
		{
			score = pPlayer->GetRoundScore();
			deaths = pPlayer->GetRoundDeaths();
		}

		// Check win conditions
		if ( score > maxscore )
		{
			maxscore = score;
			maxdeaths = deaths;
			winnerid = pPlayer->entindex();
		} 
		else if ( score == maxscore && deaths < maxdeaths )
		{
			maxdeaths = deaths;
			winnerid = pPlayer->entindex();
		}
		else if ( score == maxscore && deaths == maxdeaths )
		{
			// We have a tie
			winnerid = 0;
		}

	END_OF_PLAYER_LOOP()

	return winnerid;
}

static int playerRoundRankSort(CGEMPPlayer* const *a, CGEMPPlayer* const *b)
{
	int ascore = (*a)->GetRoundScore();
	int bscore = (*b)->GetRoundScore();

	if (ascore > bscore)
		return -1;
	else if (ascore == bscore)
		return 0;
	else return 1;
}

static int playerMatchRankSort(CGEMPPlayer* const *a, CGEMPPlayer* const *b)
{
	int ascore = (*a)->GetMatchScore();
	int bscore = (*b)->GetMatchScore();

	if (ascore > bscore)
		return -1;
	else if (ascore == bscore)
		return 0;
	else return 1;
}

void CGEMPRules::GetRankSortedPlayers( CUtlVector<CGEMPPlayer*> &sortedplayers, bool matchRank )
{
	sortedplayers.RemoveAll();

	// Add up all the players that have played in or are playing in the round.
	FOR_EACH_MPPLAYER(pPlayer)
		if (pPlayer->IsActive() || pPlayer->GetRoundScore() || pPlayer->GetRoundDeaths())
			sortedplayers.AddToTail(pPlayer);
	END_OF_PLAYER_LOOP()

	if (matchRank)
		sortedplayers.Sort(playerMatchRankSort);
	else
		sortedplayers.Sort(playerRoundRankSort);
}

void CGEMPRules::GetTaggedConVarList( KeyValues *pCvarTagList )
{
	BaseClass::GetTaggedConVarList( pCvarTagList );

	KeyValues *key = new KeyValues( "ge_allowradar" );
	key->SetString( "convar", "ge_allowradar" );
	key->SetString( "tag", "noradar" );
	pCvarTagList->AddSubKey( key );

	key = new KeyValues( "ge_allowjump" );
	key->SetString( "convar", "ge_allowjump" );
	key->SetString( "tag", "nojump" );
	pCvarTagList->AddSubKey( key );

	key = new KeyValues( "ge_autoteam" );
	key->SetString( "convar", "ge_autoteam" );
	key->SetString( "tag", "autoteamplay" );
	pCvarTagList->AddSubKey( key );

	key = new KeyValues( "ge_teamplay" );
	key->SetString( "convar", "ge_teamplay" );
	key->SetString( "tag", "teamplay" );
	pCvarTagList->AddSubKey( key );

	key = new KeyValues( "ge_paintball" );
	key->SetString( "convar", "ge_paintball" );
	key->SetString( "tag", "paintball" );
	pCvarTagList->AddSubKey( key );

	key = new KeyValues( "ge_bot_threshold" );
	key->SetString( "convar", "ge_bot_threshold" );
	key->SetString( "tag", "BOTS" );
	pCvarTagList->AddSubKey( key );
}

const char *CGEMPRules::GetGameDescription()
{
	if ( GEGameplay() )
	{
		const char *gp_desc = GetScenario()->GetGameDescription();

		if ( ge_velocity.GetFloat() < 1.0f )
			Q_snprintf( m_szGameDesc, 32, "Slow %s", gp_desc );
		else if ( ge_velocity.GetFloat() > 1.0f )
			Q_snprintf( m_szGameDesc, 32, "Turbo %s", gp_desc );
		else
			Q_strncpy( m_szGameDesc, gp_desc, 32 );

		return m_szGameDesc;
	}
	else
	{
		return "GoldenEye: Source";
	}
}

const char *CGEMPRules::GetChatPrefix( bool bTeamOnly, CBasePlayer *pPlayer )
{
	static char *sTeamChatPrefix[] = {
		"(unassigned)",
		"(spec)",
		"(MI6)",
		"(Janus)"
	};

	if( bTeamOnly && IsTeamplay() )
		return sTeamChatPrefix[ pPlayer->GetTeamNumber() ];
	else
		return "";
}

void CGEMPRules::LevelInitPreEntity()
{
	// Tell the map manager to grab the map settings.
	if (m_pMapManager)
		m_pMapManager->ParseCurrentMapData();
	else
		Warning( "No Map Manager On Level Init!\n" );

	// Create the Gameplay Manager if not existing already
	if ( !GEGameplay() )
		CreateGameplayManager();
	else
		Warning( "[GERules] Gameplay manager already existed!\n" );

	// Create the AI Manager if not existing already
	if ( !GEAi() )
		CreateAiManager();
	else
		Warning( "[GERules] Ai manager already existed!\n" );


	if ( Q_strcmp(ge_nextnextmap.GetString(), "") )
	{
		const char *nextmapname = ge_nextnextmap.GetString();

		if (engine->IsMapValid(nextmapname))
		{
			Msg("Nextnextmap switch triggered, going to %s\n", nextmapname);
			GEMPRules()->SetupChangeLevel(nextmapname);
			ge_nextnextmap.SetValue("");
		}
	}
}

void CGEMPRules::FrameUpdatePreEntityThink()
{
	m_iNumActivePlayers = -1;
	m_iNumAlivePlayers = -1;
	m_iNumInRoundPlayers = -1;
}

void CGEMPRules::Think()
{
	BaseClass::Think();

	// Let our gameplay think
	Assert( GEGameplay() != NULL );
	GEGameplay()->OnThink();

	if ( m_flChangeLevelTime > 0 && gpGlobals->curtime > m_flChangeLevelTime )
	{
		ChangeLevel();
		return;
	}

	// TODO: We might not need this anymore (just the balancer)
	EnforceTeamplay();

	EnforceBotCount();

	if (m_flKickEndTime && m_flKickEndTime < gpGlobals->curtime)
	{
		if ( !CheckVotekick() )
		{
			CRecipientFilter *filter = new CReliableBroadcastRecipientFilter;

			UTIL_ClientPrintFilter(*filter, 3, "Votekick Failed!");

			Q_strcpy(m_pKickTargetID, "NULLID"); // No more votekick.
			m_flKickEndTime = 0;
			m_flLastKickCall = gpGlobals->curtime;
		}
	}

	// Enforce token management
	m_pTokenManager->EnforceTokens();
}

void CGEMPRules::PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
	CTakeDamageInfo modinfo = info;

	CBaseEntity *weap = info.GetWeapon();
	if ( !weap )
		weap = info.GetInflictor();

	CBaseEntity *pKiller = info.GetAttacker();
	CBaseEntity *pLastAttacker = ToGEPlayer(pVictim)->GetLastAttacker();
	CBaseEntity *pInflictor = info.GetInflictor();
	const char *inflictor_name = NULL;

	if (pInflictor)
		inflictor_name = pInflictor->GetClassname();

	bool trapInflictor = (Q_strncmp(inflictor_name, "func_ge_door", 12) == 0 || Q_strncmp(inflictor_name, "trigger_trap", 12) == 0);

	// First try to give credit to players who activated an entity system that killed this player.
	if ( pKiller && !pKiller->IsPlayer() && trapInflictor )
	{
		CBaseEntity *pTrapActivator = NULL;

		if (Q_strncmp(inflictor_name, "trigger_trap", 12) == 0)
		{
			CTriggerTrap *traptrigger = static_cast<CTriggerTrap*>(pInflictor);

			pTrapActivator = traptrigger->GetTrapOwner();
		}
		else
		{
			CGEDoor *trapdoor = static_cast<CGEDoor*>(pInflictor);

			pTrapActivator = trapdoor->GetLastActivator();
		}

		// If we found an activator of the trap, give them credit for the kill.
		if (pTrapActivator)
		{
			modinfo.SetAttacker(pTrapActivator);
			pKiller = pTrapActivator; // Set this so the rest of the death code knows we have a new killer.
		}
	}


	// Try to give credit for suicides to someone.
	if ((!pKiller || !pKiller->IsPlayer() || pKiller == pVictim) && pLastAttacker)
	{
		modinfo.SetAttacker(pLastAttacker);
		modinfo.SetWeapon(NULL); // Set the weapon to weapon_none so transfered suicides with a given weapon don't just count as kills with that weapon.
		if (!trapInflictor)
			modinfo.AddDamageType(DMG_NERVEGAS); //Add the NerveGas damage type as a flag to tell other parts of the code that this was a transfered kill.
	}

	BaseClass::PlayerKilled( pVictim, modinfo );
	GetScenario()->OnPlayerKilled( ToGEPlayer(pVictim), ToGEPlayer(modinfo.GetAttacker()), weap );
}


void CGEMPRules::ClientActive( CBasePlayer *pPlayer )
{
	BaseClass::ClientActive( pPlayer );

	CGEMPPlayer *pGEPlayer = ToGEMPPlayer( pPlayer );
	if ( !pGEPlayer )
		return;

	// Register us with our botlist if we are a bot
	if ( pGEPlayer->IsBotPlayer() )
		m_vBotList.AddToTail( pGEPlayer );

	if (Q_strcmp("NULLID", m_pKickTargetID)) // Votekick currently going on.
	{
		int curplayercount = 0;

		FOR_EACH_MPPLAYER(pCountPlayer)
			curplayercount++;
		END_OF_PLAYER_LOOP()

		m_iVoteGoal = round(curplayercount * m_flVoteFrac); // Need a certain percent of players to agree.
	}

	GetScenario()->ClientConnect( pGEPlayer );
}

// a client just disconnected from the server
void CGEMPRules::ClientDisconnected( edict_t *pEntity )
{
	CGEMPPlayer *player = (CGEMPPlayer*) CBaseEntity::Instance( pEntity );
	if ( player )
	{
		// If a player has this player as their last attacker, nullify it to prevent invalid pointer.
		FOR_EACH_PLAYER(pPlayer)
			if (pPlayer->GetLastAttacker() == player)
				pPlayer->SetLastAttacker(NULL);
		END_OF_PLAYER_LOOP()

		// Next wipe any traps this player may own.
		for (int i = 0; i < m_vTrapList.Count(); i++)
		{
			const char *inflictor_name = m_vTrapList[i]->GetClassname();

			if (Q_strncmp(inflictor_name, "trigger_trap", 12) == 0)
			{
				CTriggerTrap *traptrigger = static_cast<CTriggerTrap*>(m_vTrapList[i]);

				if (traptrigger->GetTrapOwner() == player)
					traptrigger->SetTrapOwner(NULL);
			}

			if (Q_strncmp(inflictor_name, "func_ge_door", 12) == 0)
			{
				CGEDoor *trapdoor = static_cast<CGEDoor*>(m_vTrapList[i]);

				if (trapdoor->GetLastActivator() == player)
					trapdoor->SetLastActivator(NULL);
			}
		}

		if (Q_strcmp("NULLID", m_pKickTargetID)) // Votekick currently going on.
		{
			if ( m_vVoterIDs.Find(player->GetSteamHash()) != -1 )
			{
				m_vVoterIDs.FindAndRemove(player->GetSteamHash());
				m_iVoteCount--; // Player voted, so take that vote away.
			}

			int curplayercount = -1; // Account for the fact that this player is leaving.

			FOR_EACH_MPPLAYER(pCountPlayer)
				curplayercount++;
			END_OF_PLAYER_LOOP()

			m_iVoteGoal = round( curplayercount * m_flVoteFrac ); // Need a certain percent of players to agree.

			CheckVotekick(); // Could have pushed us over the threshold.
		}

		player->RemoveLeftObjects();
		GetScenario()->ClientDisconnect( player );

		// Stop tracking me if I am a Bot
		if ( player->IsBotPlayer() )
			m_vBotList.FindAndRemove( player->GetRefEHandle() );
	}
	else
	{
		Warning( "Unable to resolve player from disconnected client!\n" );
	}

	BaseClass::ClientDisconnected( pEntity );
}

void CGEMPRules::CalculateCustomDamage( CBasePlayer *pPlayer, CTakeDamageInfo &info, float &health, float &armor  )
{
	GetScenario()->CalculateCustomDamage( ToGEPlayer(pPlayer), info, health, armor );
}

void CGEMPRules::HandleTimeLimitChange()
{
	// Change our match timer
	ChangeMatchTimer( mp_timelimit.GetInt() * 60.0f );

	// Recalculate our round times based on the new map time
	if (mp_timelimit.GetInt() > 0)
		GERoundCount_Callback( &ge_roundcount, ge_roundcount.GetString(), ge_roundcount.GetFloat() );
}

void CGEMPRules::SetupChangeLevel( const char *next_level /*= NULL*/ )
{
#ifdef GES_TESTING
	if ( IsTesting() )
		return;
#endif

	if ( next_level != NULL )
	{
		// We explicitly set our next level
		nextlevel.SetValue( next_level );
		Q_strncpy( m_szNextLevel, next_level, sizeof(m_szNextLevel) );
	}
	else
	{
		// Check if we already setup for the next level
		if ( m_flChangeLevelTime > 0 )
			return;

		// If we have defined a "nextlevel" use that instead of choosing one
		if ( *nextlevel.GetString() )
			Q_strncpy( m_szNextLevel, nextlevel.GetString(), sizeof(m_szNextLevel) );
		else
		{
			const char* GESNewMap = m_pMapManager->SelectNewMap();

			if (GESNewMap)
				Q_strncpy(m_szNextLevel, GESNewMap, sizeof(m_szNextLevel));
			else
				GetNextLevelName(m_szNextLevel, sizeof(m_szNextLevel));
		}
	}

	// Tell our players what the next map will be
	IGameEvent *event = gameeventmanager->CreateEvent("server_mapannounce");
	if (event)
	{
		event->SetString("levelname", m_szNextLevel);
		gameeventmanager->FireEvent(event);
	}

	MapSelectionData *curMapData = m_pMapManager->GetCurrentMapSelectionData();
	MapSelectionData *nextMapData = m_pMapManager->GetMapSelectionData(m_szNextLevel);

	if ( nextMapData && curMapData && nextMapData->resintensity + curMapData->resintensity >= 10 )
	{
		ge_nextnextmap.SetValue(m_szNextLevel);
		Q_strncpy(m_szNextLevel, "ge_transition", sizeof(m_szNextLevel));
	}
	
	// Record our rotation log, but only if this isn't a transistion.
	if (Q_strcmp(curMapData->mapname, "ge_transition"))
	{
		CUtlVector<const char*> vMaps;
		CUtlVector<const char*> vModes;
		CUtlVector<const char*> vWepnames;
		CUtlVector<CGELoadout*> vWeapons;
		char linebuffer[128];

		GetMapManager()->GetRecentMaps(vMaps);
		GEGameplay()->GetRecentModes(vModes);
		GetLoadoutManager()->GetRecentLoadouts(vWeapons);

		for (int i = 0; i < vWeapons.Count(); i++)
			vWepnames.AddToTail(vWeapons[i]->GetIdent());

		FileHandle_t file = filesystem->Open("gamesetuprecord.txt", "w", "MOD");

		if (file)
		{
			filesystem->Write("Maps:\n", V_strlen("Maps:\n"), file);
			for (int i = 0; i < vMaps.Count(); i++)
			{
				Q_snprintf(linebuffer, 128, "%s\n", vMaps[i]);
				filesystem->Write(linebuffer, V_strlen(linebuffer), file);
			}
			filesystem->Write("-\n", V_strlen("-\n"), file);

			filesystem->Write("Modes:\n", V_strlen("Modes:\n"), file);
			for (int i = 0; i < vModes.Count(); i++)
			{
				Q_snprintf(linebuffer, 128, "%s\n", vModes[i]);
				filesystem->Write(linebuffer, V_strlen(linebuffer), file);
			}
			filesystem->Write("-\n", V_strlen("-\n"), file);

			filesystem->Write("Weps:\n", V_strlen("Weps:\n"), file);
			for (int i = 0; i < vWepnames.Count(); i++)
			{
				Q_snprintf(linebuffer, 128, "%s\n", vWepnames[i]);
				filesystem->Write(linebuffer, V_strlen(linebuffer), file);
			}
			filesystem->Write("-\n", V_strlen("-\n"), file);

			filesystem->Close(file);
		}
	}

	// Notify everyone
	Msg( "CHANGE LEVEL: %s\n", m_szNextLevel );

	m_flChangeLevelTime = gpGlobals->curtime + GE_CHANGELEVEL_DELAY;
}

void CGEMPRules::ChangeLevel()
{
	// Arbitrarily move the timer into infinity so we never call us again
	m_flChangeLevelTime += 9999.0f;

	// Record our player count
	g_iLastPlayerCount = GetNumActivePlayers();

	// Actually change the level by calling the original command
	engine->ServerCommand( UTIL_VarArgs( "__real_changelevel %s\n", m_szNextLevel ) );
}

void CGEMPRules::GoToIntermission()
{
	// We should never get here since we are not using this function anymore
	Assert( 0 );
}

void CGEMPRules::ResetPlayerScores( bool resetmatch /* = false */ )
{
	// Reset all the player's scores
	FOR_EACH_MPPLAYER( pPlayer )
		pPlayer->ResetScores();
		if ( resetmatch )
			pPlayer->ResetMatchScores();
	END_OF_PLAYER_LOOP()
}

void CGEMPRules::ResetTeamScores( bool resetmatch /* = false */ )
{
	for ( int i = FIRST_GAME_TEAM; i < MAX_GE_TEAMS; i++ )
	{
		GetGlobalTeam(i)->SetRoundScore(0);
		if ( resetmatch )
			GetGlobalTeam(i)->SetMatchScore(0);
	}
}

int CGEMPRules::GetSpawnPointType( CGEPlayer *pPlayer )
{
	if( (pPlayer->GetTeamNumber() == TEAM_SPECTATOR || pPlayer->IsObserver()) && GetSpawnersOfType(SPAWN_PLAYER_SPECTATOR)->Count() )
	{
		return SPAWN_PLAYER_SPECTATOR;
	}
	else if ( IsTeamplay() && IsTeamSpawn() && GetSpawnersOfType(SPAWN_PLAYER_MI6)->Count() && GetSpawnersOfType(SPAWN_PLAYER_JANUS)->Count() )
	{
		// If we are in swapped spawns spawn here if you are Janus
		// Since we are spawning in our team area just select the next spawn point and go with it
		if ( (pPlayer->GetTeamNumber() == TEAM_MI6 && !IsTeamSpawnSwapped()) || 
			(pPlayer->GetTeamNumber() == TEAM_JANUS && IsTeamSpawnSwapped()) )
		{
			return SPAWN_PLAYER_MI6;
		}
		else
		{
			return SPAWN_PLAYER_JANUS;
		}
	}
	
	return SPAWN_PLAYER;
}


float CGEMPRules::GetSpeedMultiplier( CGEPlayer *pPlayer )
{
	if ( pPlayer )
		return pPlayer->GetSpeedMultiplier() * ge_velocity.GetFloat();
	else
		return ge_velocity.GetFloat();
}

void CGEMPRules::StartMatchTimer( float time_sec /*=-1*/ )
{
	Assert( m_hMatchTimer.Get() != NULL );
	if ( time_sec < 0 )
		time_sec = mp_timelimit.GetInt() * 60;
	m_hMatchTimer->Start( time_sec );
}

void CGEMPRules::ChangeMatchTimer( float new_time_sec )
{
	Assert( m_hMatchTimer.Get() != NULL );

	// Check to make sure we will actually make a change
	if ( new_time_sec != m_hMatchTimer->GetLength() )
	{
		// Notify if we extended or shortened the round
		if ( new_time_sec > 0 )
		{
			// Set the new time
			m_hMatchTimer->ChangeLength( new_time_sec );
		}
		else
		{
			// The round timer has been disabled
			StopMatchTimer();
		}
	}
}

void CGEMPRules::SetMatchTimerPaused( bool state )
{
	Assert( m_hMatchTimer.Get() != NULL );
	state ? m_hMatchTimer->Pause() : m_hMatchTimer->Resume();
}

void CGEMPRules::StopMatchTimer()
{
	Assert( m_hMatchTimer.Get() != NULL );
	m_hMatchTimer->Stop();
}

void CGEMPRules::SetRoundTimerEnabled( bool state )
{
	Assert( m_hRoundTimer.Get() != NULL );

	if (m_hRoundTimer->IsEnabled() == state)
		return; // We're already in this state so we don't need to do anything.

	if ( state ) {
		m_hRoundTimer->Enable();
		StartRoundTimer( ge_roundtime.GetFloat() );
	} else {
		m_hRoundTimer->Disable();
	}
}

void CGEMPRules::StartRoundTimer( float time_sec /*=-1*/ )
{
	Assert( m_hRoundTimer.Get() != NULL );
	if ( time_sec < 0 )
		time_sec = ge_roundtime.GetInt();
	m_hRoundTimer->Start( time_sec );
}

void CGEMPRules::ChangeRoundTimer( float new_time_sec, bool announce )
{
	Assert( m_hRoundTimer.Get() != NULL );

	// Check to make sure we will actually make a change
	if ( new_time_sec != m_hRoundTimer->GetLength() )
	{
		// Notify if we extended or shortened the round
		if ( new_time_sec > 0 )
		{
			if (announce)
			{
				char szMinSec[8], szDir[16];
				float min = new_time_sec / 60.0f;
				int sec = (int)(60 * (min - (int)min));
				Q_snprintf(szMinSec, 8, "%d:%02d", (int)min, sec);

				// Tell the everyone that we've extended or shortened the round.
				if (new_time_sec > m_hRoundTimer->GetLength())
					Q_strncpy(szDir, "#Extended", 16);
				else
					Q_strncpy(szDir, "#Shortened", 16);

				UTIL_ClientPrintAll(HUD_PRINTTALK, "#GES_RoundTime_Changed", szDir, szMinSec);
			}
			// Set the new time
			m_hRoundTimer->ChangeLength( new_time_sec );
		}
		else
		{
			// The round timer has been disabled
			StopRoundTimer();
		}
	}
}

void CGEMPRules::SetRoundTimer(float new_time_sec, bool announce)
{
	Assert(m_hRoundTimer.Get() != NULL);

	// Check to make sure we will actually make a change
	if (new_time_sec != m_hRoundTimer->GetTimeRemaining())
	{
		// Notify if we extended or shortened the round
		if (new_time_sec > 0)
		{
			if (announce)
			{
				char szMinSec[8], szDir[16];
				float min = new_time_sec / 60.0f;
				int sec = (int)(60 * (min - (int)min));
				Q_snprintf(szMinSec, 8, "%d:%02d", (int)min, sec);

				// Tell the everyone that we've extended or shortened the round.
				if (new_time_sec > m_hRoundTimer->GetTimeRemaining())
					Q_strncpy(szDir, "#Extended", 16);
				else
					Q_strncpy(szDir, "#Shortened", 16);

				UTIL_ClientPrintAll(HUD_PRINTTALK, "#GES_RoundTime_Changed", szDir, szMinSec);
			}
			// Set the new time
			m_hRoundTimer->SetCurrentLength( new_time_sec );
		}
		else
		{
			// The round timer has been disabled
			StopRoundTimer();
		}
	}
}

CON_COMMAND(ge_setcurrentroundtime, "Sets the amount of seconds left in the round.  Does not affect subsequent rounds.")
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
		return;

	GEMPRules()->SetRoundTimer(atoi(args[1]));
}

void CGEMPRules::AddToRoundTimer(float new_time_sec, bool announce)
{
	Assert(m_hRoundTimer.Get() != NULL);

	// If we don't have any time on the clock, we don't have any time to add to.
	if (m_hRoundTimer->IsStarted())
	{
		// Notify if we extended or shortened the round
		if (announce)
		{
			char szMinSec[8], szDir[16];
			float min = abs(new_time_sec) / 60.0f;
			int sec = (int)abs(60 * (min - (int)min));
			Q_snprintf(szMinSec, 8, "%d:%02d", (int)min, sec);

			// Tell the everyone that we've extended or shortened the round.
			if (new_time_sec > 0)
				Q_strncpy(szDir, "#Extended", 16);
			else
				Q_strncpy(szDir, "#Shortened", 16);

			UTIL_ClientPrintAll(HUD_PRINTTALK, "#GES_Roundtime_Added", szDir, szMinSec);
		}
		// Set the new time
		m_hRoundTimer->AddToLength(new_time_sec);
	}
}

CON_COMMAND(ge_addtoroundtime, "Add the given number of seconds to the current roundtime.  Does not affect subsequent rounds.")
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
		return;

	GEMPRules()->AddToRoundTimer(atoi(args[1]));
}

void CGEMPRules::SetRoundTimerPaused( bool state )
{
	Assert( m_hRoundTimer.Get() != NULL );
	state ? m_hRoundTimer->Pause() : m_hRoundTimer->Resume();
}

void CGEMPRules::StopRoundTimer()
{
	Assert( m_hRoundTimer.Get() != NULL );
	UTIL_ClientPrintAll( HUD_PRINTTALK, "#GES_RoundTime_Disabled" );
	m_hRoundTimer->Stop();
}

void CGEMPRules::SetSpawnInvulnInterval(float duration)
{
	m_flSpawnInvulnDuration = duration;
}


void CGEMPRules::SetSpawnInvulnCanBreak(bool canbreak)
{
	m_bSpawnInvulnCanBreak = canbreak;
}

float CGEMPRules::GetSpawnInvulnInterval()
{
	return m_flSpawnInvulnDuration;
}

bool CGEMPRules::GetSpawnInvulnCanBreak()
{
	return m_bSpawnInvulnCanBreak;
}

void CGEMPRules::SetTeamplay( bool state, bool force /*= false*/ )
{
	// Don't do anything if there is no change or we don't want it
	if ( m_bTeamPlayDesired == state )
		return;

	// If we don't want to change ignore this, unless forced
	if ( !force && GetTeamplayMode() != TEAMPLAY_TOGGLE )
		return;

	// Set our desired state
	m_bTeamPlayDesired = state;

	// Update the global variable
	gpGlobals->teamplay = IsTeamplay();

	// Let everyone know of the change
	UTIL_ClientPrintAll( HUD_PRINTTALK, "#GES_Teamplay_Changed", state ? "#Enabled" : "#Disabled" );

	// Reset our scores, players retain their match score, teams do not
	ResetPlayerScores();
	ResetTeamScores( true );
}

bool CGEMPRules::CreateBot()
{
	if ( g_pBigAINet->NumNodes() < 10 )
	{
		Warning( "Not enough AI Nodes in this level to spawn bots!\n" );
		return false;
	}

	string_t name = MAKE_STRING( "GEBot" );
	if ( g_vBotNames.Count() > 1 )
	{
		int i = GERandom<int>( g_vBotNames.Count() );
		name = g_vBotNames[i];
		g_vBotNames.FastRemove( i );
	}
	else if ( g_vBotNames.Count() == 1 )
	{
		name = g_vBotNames[0];
	}

	edict_t *pEdict = engine->CreateFakeClient( name.ToCStr() );
	return pEdict != NULL;
}

bool CGEMPRules::RemoveBot( const char *name )
{
	CBasePlayer *pBot = UTIL_PlayerByName( name );
	if ( pBot )
	{
		// Return the name to that stack
		g_vBotNames.AddToTail( AllocPooledString( name ) );
		engine->ServerCommand( UTIL_VarArgs( "kickid %d\n", pBot->GetUserID() ) );
		return true;
	}
	
	return false;
}

void CGEMPRules::EnforceTeamplay()
{
	// Always try to balance teams
	BalanceTeams();

	// TODO: Is this even needed anymore?
	if ( IsTeamplay() )
	{
		// Forcibly remove players from the unassigned team
		CTeam *pTeam = g_Teams[TEAM_UNASSIGNED];
		CGEMPPlayer *pPlayer = NULL;

		for (int i=0; i < pTeam->GetNumPlayers(); i++)
		{
			pPlayer = ToGEMPPlayer( pTeam->GetPlayer(i) );
			pPlayer->PickDefaultSpawnTeam();
		}
	}
	else
	{
		for ( int i=FIRST_GAME_TEAM; i < MAX_GE_TEAMS; i++ )
		{
			CTeam *pTeam = g_Teams[i];
			CGEMPPlayer *pPlayer = NULL;

			for (int i=0; i < pTeam->GetNumPlayers(); i++)
			{
				pPlayer = ToGEMPPlayer( pTeam->GetPlayer(i) );
				pPlayer->ChangeTeam( TEAM_UNASSIGNED, true );
			}
		}
	}
}

void CGEMPRules::EnforceBotCount()
{
	// We have to have nodes in the map!
	if ( g_pBigAINet->NumNodes() < 10 )
		return;

	if ( gpGlobals->curtime < m_flNextBotCheck )
		return;

	// Don't run this routine if we are ignoring the threshold
	if ( ge_bot_threshold.GetInt() <= 0 )
		return;

	int bot_thresh = MIN( ge_bot_threshold.GetInt(), gpGlobals->maxClients - ge_bot_openslots.GetInt() );
	int bot_count = m_vBotList.Count();
	int player_count = 0;
	int spec_count = 0;
	int total_count = 0;

	FOR_EACH_MPPLAYER( pPlayer )
		if ( pPlayer->IsBotPlayer() )
			continue;

		if ( pPlayer->IsActive() )
			player_count++;
		else
			spec_count++;
	END_OF_PLAYER_LOOP()

	total_count = player_count + bot_count;

	// If we are at our client max with spectators accounted for and we are enforcing
	// a strict slot count, force our threshold lower to allow a player to join
	if ( ge_bot_strict_openslot.GetBool() )
		bot_thresh -= spec_count;

	// Remove all the bots if we have no players (we only add bots if we have plaeyers)
	if ( player_count == 0 && bot_count > 0 )
		bot_thresh = 0;

	if ( total_count > bot_thresh && bot_count > 0 )
	{
		int to_remove = MIN( total_count - bot_thresh, bot_count );

		// Remove bots
		for ( int i=0; i < to_remove; i++ )
		{
			CGEBotPlayer *pBot = (CGEBotPlayer*) m_vBotList[bot_count-1].Get();
			if ( pBot )
			{
				// Try to kick it, otherwise remove outright
				if ( !CGEMPRules::RemoveBot( pBot->GetPlayerName() ) )
					UTIL_Remove( pBot );
			}
			else
			{
				m_vBotList.Remove( bot_count-1 );
			}

			--bot_count;
		}
	}
	else if ( total_count < bot_thresh && player_count > 0 )
	{
		int to_add = bot_thresh - total_count;

		// Add bots (only if there are players present)
		for ( int i=0; i < to_add; i++ )
		{
			if ( !CGEMPRules::CreateBot() )
				Warning( "Failed to create bot!\n" );
		}
	}

	m_flNextBotCheck = gpGlobals->curtime + 3.5f;
}

void CGEMPRules::BalanceTeams()
{
	// Don't balance in set tournaments or if we turn it off
	if ( !ge_teamautobalance.GetBool() || !IsTeamplay() || ge_tournamentmode.GetBool() )
		return;

	// Don't do switches if we have less than a minute left in the round/match!
	if ( (IsRoundTimeRunning() && GetRoundTimeRemaining() < 45.0f) || (IsMatchTimeRunning() && GetMatchTimeRemaining() < 45.0f) )
		return;

	// Is it even time to check yet?
	if ( gpGlobals->curtime < m_flNextTeamBalance )
		return;

	CTeam *pJanus = g_Teams[TEAM_JANUS];
	CTeam *pMI6 = g_Teams[TEAM_MI6];
	int plDiff = pJanus->GetNumPlayers() - pMI6->GetNumPlayers();

	// We issued the warning already, now we do it!
	if ( m_bInTeamBalance )
	{
		CTeam *pHeavyTeam = NULL;
		CTeam *pLightTeam = NULL;

		// Imbalance is on Janus's side
		if ( plDiff > 1 ) {
			pHeavyTeam = pJanus;
			pLightTeam = pMI6;
		// Imbalance is on MI6's side
		} else if ( plDiff < -1 ) {
			pHeavyTeam = pMI6;
			pLightTeam = pJanus;
		}

		// If the situation hasn't corrected itself in the 5 seconds
		// we take action!
		if ( pHeavyTeam )
		{
			CGEMPPlayer *pPlayer = NULL;
			CUtlVector<CGEMPPlayer*> players;

			for ( int i=0; i < pHeavyTeam->GetNumPlayers(); ++i )
			{
				pPlayer = ToGEMPPlayer( pHeavyTeam->GetPlayer(i) );
				if ( !pPlayer || !GetScenario()->CanPlayerChangeTeam( pPlayer, pHeavyTeam->GetTeamNumber(), pLightTeam->GetTeamNumber(), true ) ) // Autobalance is always forced.
					continue;

				players.AddToTail( pPlayer );
			}

			if ( players.Size() > 0 )
			{
				players.Sort( BalanceTeamSort );
				int switches = MIN( abs(plDiff) - 1, players.Size() );

				// Switch the required number of players to the light team
				for ( int j=0; j < switches; j++ )
				{
					players[j]->ChangeTeam( pLightTeam->GetTeamNumber(), true );
					// tell people that we have switched this player
					UTIL_ClientPrintAll( HUD_PRINTTALK, "#GES_TeamBalanced_Player", players[j]->GetPlayerName() );
				}
			}
			else
			{
				// We don't have anyone to switch, set a check for sometime in the future
				m_flNextTeamBalance = gpGlobals->curtime + 30.0f;
			}
		}

		// We are done, proceed as normal now
		m_bInTeamBalance = false;
	}
	else if ( abs(plDiff) > 1 )
	{
		UTIL_ClientPrintAll( HUD_PRINTTALK, "#GES_TeamBalance", "5" );
		m_flNextTeamBalance = gpGlobals->curtime + 5.0f;
		m_bInTeamBalance = true;
		return;
	}

	m_flNextTeamBalance = gpGlobals->curtime + 15.0f;
}


void CGEMPRules::DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
	// Work out what killed the player, and send a message to all clients about it
	const char *default_killer = "#GE_WorldKill";		// by default, the player is killed by the world
	const char *killer_weapon_name = NULL;
	int weaponid = WEAPON_NONE;
	int killer_ID = 0;
	int custom = 0;
	bool VictimInPVS = true;
	int dist = 0;
	int wepSkin = 0;
	int dmgType = 0;

	// Find the killer & the scorer
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBaseEntity *pWeapon = info.GetWeapon();

	if (info.GetDamageType())
		dmgType = info.GetDamageType();

	// Is the killer a player?
	if ( pKiller->IsPlayer() )
	{
		killer_ID = ToBasePlayer( pKiller )->GetUserID();
		
		if ( pWeapon )
		{
			CGEWeapon *pGEWeapon = ToGEWeapon( (CBaseCombatWeapon*)pWeapon );
			killer_weapon_name = pGEWeapon->GetPrintName();
			weaponid = pGEWeapon->GetWeaponID();
			if (weaponid < WEAPON_RANDOM)
				wepSkin = pGEWeapon->GetSkin();
		}
		else
		{
			killer_weapon_name = pInflictor->GetClassname();  // it's just that easy
		}

		dist = floor((pKiller->GetAbsOrigin() - pVictim->GetAbsOrigin()).Length());

		CGEPlayer *pGEKiller = ToGEPlayer(pKiller);

		if (pGEKiller)
			VictimInPVS = pGEKiller->CheckInPVS(pVictim);
	}
	else
	{
		killer_weapon_name = pInflictor->GetClassname();
	}

	// We only need to check the classname if we didn't explicitly resolve the weapon above
	if ( weaponid == WEAPON_NONE )
	{
		if (!Q_stricmp( killer_weapon_name, "player" ))
		{
			// Detects killbinds and displays a special message for them
			dmgType = DMG_GENERIC;
			killer_weapon_name = "self";
		}
		else if (dmgType & DMG_NERVEGAS) // Make sure we're actually looking for a weapon first, if this flag is set we want to pretend there isn't one.
		{
			killer_weapon_name = default_killer;
		}
		else if ( Q_stristr( killer_weapon_name, "npc_" ) && !pInflictor->IsNPC() )
		{
			CGEBaseGrenade *pNPC = ToGEGrenade( pInflictor );
			killer_weapon_name = pNPC->GetPrintName();
			weaponid = pNPC->GetWeaponID();
			custom = pNPC->GetCustomData();
		}
		else if (!Q_stricmp(killer_weapon_name, "trigger_trap")) // Use the trigger's name as the weapon name if it's a kill, or replace the death message with a special one on suicide.
		{
			CTriggerTrap *traptrigger = static_cast<CTriggerTrap*>(pInflictor);

			if (!pKiller->IsPlayer())
			{
				string_t sDeathMsg = traptrigger->GetTrapDeathString();

				if (sDeathMsg != NULL_STRING)
					killer_weapon_name = UTIL_VarArgs("-TD-%s", sDeathMsg);
				else
					killer_weapon_name = "#GE_Trap";
			}
			else if (pKiller == pVictim)
			{
				string_t sDeathMsg = traptrigger->GetTrapSuicideString();

				if (sDeathMsg != NULL_STRING)
					killer_weapon_name = UTIL_VarArgs("-TD-%s", sDeathMsg);
				else
					killer_weapon_name = "#GE_Trap";
			}
			else
			{
				string_t sDeathMsg = traptrigger->GetTrapKillString();

				if (sDeathMsg != NULL_STRING) // We have a kill message to go along with our kill
					killer_weapon_name = UTIL_VarArgs("-TD-%s", sDeathMsg);
				else // We don't have a kill message, but we might be able to use the entity name
				{
					string_t sEntName = pInflictor->GetEntityName();

					if (sEntName != NULL_STRING)
						killer_weapon_name = UTIL_VarArgs("%s", sEntName);
					else
						killer_weapon_name = "#GE_Trap";
				}
			}

			weaponid = WEAPON_TRAP;
		}
		else if (Q_strncmp(killer_weapon_name, "func_ge_door", 12) == 0)
		{
			// Detects ge door kills and displays the entity name in the killfeed if it has one.
			string_t sEntName = pInflictor->GetEntityName();

			if (sEntName != NULL_STRING)
				killer_weapon_name = UTIL_VarArgs("%s", sEntName);
			else
				killer_weapon_name = "#GE_Trap";

			weaponid = WEAPON_TRAP;
		}
		else if (dmgType & DMG_BLAST)
		{
			// Special case if we didn't have a weapon check if we were damaged by an explosion
			killer_weapon_name = "#GE_Explosion";
			weaponid = WEAPON_EXPLOSION;
		}
		else
		{
			// The "world" killed us if we can't figure out who did
			killer_weapon_name = default_killer;
		}
	}

	if ( !killer_weapon_name )
		killer_weapon_name = default_killer;

	IGameEvent *event = gameeventmanager->CreateEvent( "player_death" );
	if( event )
	{
		event->SetInt( "userid", pVictim->GetUserID() );
		event->SetInt( "attacker", killer_ID );
		event->SetString( "weapon", killer_weapon_name );
		event->SetInt( "weaponid", weaponid );
		event->SetBool( "headshot", pVictim->LastHitGroup() == HITGROUP_HEAD );
		event->SetBool( "penetrated", FBitSet(info.GetDamageStats(), FIRE_BULLETS_PENETRATED_SHOT)?true:false );
		event->SetInt( "dist", dist);
		event->SetInt( "weaponskin", wepSkin);
		event->SetInt( "dmgtype", dmgType);
		event->SetBool( "InPVS", VictimInPVS);
		event->SetInt( "custom", custom );
		gameeventmanager->FireEvent( event );
	}
}

bool CGEMPRules::OnPlayerSay(CBasePlayer* player, const char* text)
{
	CGEMPPlayer* pPlayer = ToGEMPPlayer(player);

	if (!pPlayer || !text)
		return false;

	if (!Q_strncmp("!gevotekick", text, 11))
	{
		CRecipientFilter *filter = NULL;

		int curplayercount = 0;

		FOR_EACH_MPPLAYER(pCountPlayer)
			curplayercount++;
		END_OF_PLAYER_LOOP()

		if (ge_votekickpreference.GetInt() == 0 || (ge_votekickpreference.GetInt() == -1 && !(iAlertCode & 8)))
		{
			filter = new CSingleUserRecipientFilter(player);

			UTIL_ClientPrintFilter(*filter, 3, "Vote kick disabled on this server.");
		}
		else if (Q_strcmp("NULLID", m_pKickTargetID))
		{
			filter = new CSingleUserRecipientFilter(player);

			UTIL_ClientPrintFilter(*filter, 3, "There is already someone being targeted with votekick!");
		}
		else if (m_flLastKickCall && (m_flLastKickCall + 120 > gpGlobals->curtime || (m_pLastKickCaller == player && m_flLastKickCall + 240 > gpGlobals->curtime)))
		{
			filter = new CSingleUserRecipientFilter(player);

			char sKickTime[128];

			int votewait = m_flLastKickCall + 120 - gpGlobals->curtime;

			if (m_pLastKickCaller == player)
				votewait += 120;

			Q_snprintf(sKickTime, sizeof(sKickTime), "You must wait %d seconds before starting another votekick!", votewait);

			UTIL_ClientPrintFilter(*filter, 3, sKickTime);
		}
		else if ( curplayercount < 4 )
		{
			filter = new CSingleUserRecipientFilter(player);

			UTIL_ClientPrintFilter(*filter, 3, "Need at least 4 players in the server to call a votekick!");
		}
		else
		{
			filter = new CReliableBroadcastRecipientFilter;

			char targetname[32];
			CBasePlayer *pKickTarget;

			if (!Q_strncmp("!gevotekickid", text, 13))
			{
				Q_StrRight(text, -14, targetname, 32);

				pKickTarget = UTIL_PlayerByUserId( Q_atoi(targetname) );
			}
			else
			{
				Q_StrRight(text, -12, targetname, 32);

				pKickTarget = UTIL_PlayerByName(targetname);
			}

			if (pKickTarget)
				Q_strcpy(m_pKickTargetID, pKickTarget->GetNetworkIDString());

			if (Q_strcmp("NULLID", m_pKickTargetID) && pKickTarget)
			{
				char sKickNotice[128];
				Q_snprintf(sKickNotice, sizeof(sKickNotice), "%s has called a votekick on %s!", player->GetPlayerName(), pKickTarget->GetPlayerName());

				UTIL_ClientPrintFilter(*filter, 3, sKickNotice);
				UTIL_ClientPrintFilter(*filter, 3, "Press g to agree!");

				m_iVoteCount = 1; // Person who called the kick of course voted yes.
				m_iVoteGoal = 0;
				m_flKickEndTime = gpGlobals->curtime + 40;
				m_pLastKickCaller = pPlayer;
				m_vVoterIDs.RemoveAll();
				m_vVoterIDs.AddToTail(pPlayer->GetSteamHash()); // Make sure they can't vote again.


				m_flVoteFrac = iVotekickThresh * 0.01; // Pretty high by default to prevent abuse.

				if (pPlayer->GetDevStatus() == GE_DEVELOPER)
					m_flVoteFrac = iVotekickThresh * 0.005;
				else if (pPlayer->GetDevStatus() == GE_BETATESTER)
					m_flVoteFrac = iVotekickThresh * 0.006;
				else if (pPlayer->GetDevStatus() == GE_CONTRIBUTOR)
					m_flVoteFrac = iVotekickThresh * 0.007;
				else if (pPlayer->GetDevStatus() > 0) // Acheivement medals get a small bonus too.
					m_flVoteFrac = iVotekickThresh * 0.009;

				if ( iAlertCode & 32 ) // Martial Law, developers can kick instantly.
				{
					if (pPlayer->GetDevStatus() == GE_DEVELOPER)
						m_flVoteFrac = 0;
					else if (pPlayer->GetDevStatus() == GE_BETATESTER)
						m_flVoteFrac = 0;
				}

				m_iVoteGoal = round( curplayercount * m_flVoteFrac ); // Need a certain percent of players to agree.

				CheckVotekick();
			}
		}

		return true;
	}
	else if (Q_strcmp("NULLID", m_pKickTargetID) && !Q_strcmp("!voodoo", text))
	{
		CRecipientFilter *filter = new CSingleUserRecipientFilter(player);

		if (m_vVoterIDs.Find(pPlayer->GetSteamHash()) != -1)
			UTIL_ClientPrintFilter(*filter, 3, "You already voted!");
		else
		{
			filter = new CReliableBroadcastRecipientFilter;

			m_iVoteCount++;
			m_vVoterIDs.AddToTail(pPlayer->GetSteamHash());

			char sKickProgress[128];
			Q_snprintf(sKickProgress, sizeof(sKickProgress), "Vote count is now %d/%d!", m_iVoteCount, m_iVoteGoal);

			UTIL_ClientPrintFilter(*filter, 3, sKickProgress);
		}

		CheckVotekick();

		return true;
	}

	return GetScenario()->OnPlayerSay(pPlayer, text);
}

#endif // GAME_DLL

void CGEMPRules::SetMapFloorHeight(float height)
{
	m_flMapFloorHeight = height;
}

float CGEMPRules::GetMapFloorHeight()
{
	return m_flMapFloorHeight;
}

// Match Timer Functions
bool CGEMPRules::IsMatchTimeRunning()
{
	if ( m_hMatchTimer.Get() )
		return m_hMatchTimer->IsStarted() && !m_hMatchTimer->IsPaused();
	return false;
}

bool CGEMPRules::IsMatchTimePaused()
{
	if ( m_hMatchTimer.Get() )
		return m_hMatchTimer->IsStarted() && m_hMatchTimer->IsPaused();
	return false;
}

float CGEMPRules::GetMatchTimeRemaining()
{
	if ( m_hMatchTimer.Get() )
		return m_hMatchTimer->GetTimeRemaining();
	return 0;
}

// Round Timer Functions
bool CGEMPRules::IsRoundTimeEnabled()
{
	if ( m_hRoundTimer.Get() )
		return m_hRoundTimer->IsEnabled();
	return false;
}

bool CGEMPRules::IsRoundTimeRunning()
{
	if ( m_hRoundTimer.Get() )
		return m_hRoundTimer->IsStarted() && !m_hRoundTimer->IsPaused();
	return false;
}

bool CGEMPRules::IsRoundTimePaused()
{
	if ( m_hRoundTimer.Get() )
		return m_hRoundTimer->IsPaused();
	return false;
}

float CGEMPRules::GetRoundTimeRemaining()
{	
	if ( m_hRoundTimer.Get() )
		return m_hRoundTimer->GetTimeRemaining();

	Warning("Could not find round timer!\n");
	return 0;
}

bool CGEMPRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		::V_swap(collisionGroup0,collisionGroup1);
	}
	
	if ( collisionGroup0 == COLLISION_GROUP_MI6 && (collisionGroup1 == COLLISION_GROUP_MI6 || collisionGroup1 == COLLISION_GROUP_GRENADE_MI6) )
		return false;

	if ( collisionGroup0 == COLLISION_GROUP_JANUS && ( collisionGroup1 == COLLISION_GROUP_JANUS || collisionGroup1 == COLLISION_GROUP_GRENADE_JANUS ) )
		return false;

	int switchlist[4] = { COLLISION_GROUP_MI6, COLLISION_GROUP_JANUS, COLLISION_GROUP_GRENADE_MI6, COLLISION_GROUP_GRENADE_JANUS };
	int fixlist[4] = { COLLISION_GROUP_PLAYER, COLLISION_GROUP_PLAYER, COLLISION_GROUP_GRENADE, COLLISION_GROUP_GRENADE };

	// remap these as to not fuck the collisions up for the lower classes.
	for (int i = 0; i < 4; i++)
	{
		if (collisionGroup0 == switchlist[i])
			collisionGroup0 = fixlist[i];

		if (collisionGroup1 == switchlist[i])
			collisionGroup1 = fixlist[i];
	}

	return BaseClass::ShouldCollide( collisionGroup0, collisionGroup1 ); 
}

bool CGEMPRules::IsTeamplay()
{
	if ( m_iTeamplayMode == TEAMPLAY_TOGGLE )
		return m_bTeamPlayDesired;
	else if ( m_iTeamplayMode == TEAMPLAY_ONLY )
		return true;
	else
		return false;
}

bool CGEMPRules::IsIntermission()
{
#ifdef GAME_DLL
	return GEGameplay()->IsInIntermission();
#else
	if ( GEGameplayRes() )
		return GEGameplayRes()->GetGameplayIntermission();
	else
		return false;
#endif
}

#if defined(GAME_DLL) && defined(DEBUG)
CON_COMMAND_F( ge_debug_playerspawnstats, "Show compiled player spawn statistics used for spawning gameplay elements", FCVAR_CHEAT )
{
	if ( !GEMPRules() )
		return;

	//Debug purposes only
	bool swapped = GEMPRules()->IsTeamSpawnSwapped();
	Vector radius, mins, maxs, centroid;

	if ( GEMPRules()->GetSpawnerStats( SPAWN_PLAYER, mins, maxs, centroid ) )
	{
		radius = (maxs-mins) / 2.0f;
		debugoverlay->AddBoxOverlay( maxs-radius, -radius - Vector(12,12,0), radius + Vector(12,12,72), vec3_angle, 0, 255, 0, 80, 15.0f );
		debugoverlay->AddBoxOverlay( centroid, Vector(-5,-5,-5), Vector(5,5,5), vec3_angle, 0, 255, 0, 220, 15.0f );
	}
	
	if ( GEMPRules()->GetSpawnerStats( SPAWN_PLAYER_MI6, mins, maxs, centroid ) )
	{
		radius = (maxs-mins) / 2.0f;
		debugoverlay->AddBoxOverlay( maxs-radius, -radius - Vector(12,12,0), radius + Vector(12,12,72), vec3_angle, swapped ? 255 : 0, 0, swapped ? 0 : 255, 80, 15.0f );
		debugoverlay->AddBoxOverlay( centroid, Vector(-5,-5,-5), Vector(5,5,5), vec3_angle, swapped ? 255 : 0, 0, swapped ? 0 : 255, 220, 15.0f );
	}

	if ( GEMPRules()->GetSpawnerStats( SPAWN_PLAYER_JANUS, mins, maxs, centroid ) )
	{
		radius = (maxs-mins) / 2.0f;
		debugoverlay->AddBoxOverlay( maxs-radius, -radius - Vector(12,12,0), radius + Vector(12,12,72), vec3_angle, swapped ? 0 : 255, 0, swapped ? 255 : 0, 80, 15.0f );
		debugoverlay->AddBoxOverlay( centroid, Vector(-5,-5,-5), Vector(5,5,5), vec3_angle, swapped ? 0 : 255, 0, swapped ? 255 : 0, 220, 15.0f );
	}
}
#endif
