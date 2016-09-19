///////////// Copyright � 2013, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_gameplay.cpp
// Description:
//      Gameplay Manager Definition
//
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"

#include "ge_shareddefs.h"
#include "ge_utils.h"
#include "ge_playerresource.h"
#include "ge_radarresource.h"
#include "ge_gameplayresource.h"
#include "ge_tokenmanager.h"
#include "ge_loadoutmanager.h"
#include "ge_stats_recorder.h"
#include "gemp_player.h"
#include "gemp_gamerules.h"
#include "ge_mapmanager.h"

#include "team.h"
#include "script_parser.h"
#include "filesystem.h"
#include "viewport_panel_names.h"

#include "ge_gameplay.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern bool g_bInGameplayReload;

extern CGEBaseGameplayManager *g_GamePlay;

CGEBaseGameplayManager *GEGameplay()
{
	return g_GamePlay;
}

CGEBaseScenario *GetScenario()
{
	Assert( g_GamePlay );
	return g_GamePlay->GetScenario();
}

void GEGameplay_Callback( IConVar *var, const char *pOldString, float flOldValue )
{
	if ( !GEMPRules() && !g_bInGameplayReload )
		return;

	ConVar *cVar = static_cast<ConVar*>(var);
	GEGameplay()->LoadScenario( cVar->GetString() );
}

void GEGPCVar_Callback( IConVar *var, const char *pOldString, float flOldValue )
{
	if ( !GEGameplay() || !GEGameplay()->GetScenario() )
		return;

	ConVar *cVar = static_cast<ConVar*>(var);
	GEGameplay()->GetScenario()->OnCVarChanged( cVar->GetName(), pOldString, cVar->GetString() );
}

ConVar ge_gp_cyclefile( "ge_gp_cyclefile", "gameplaycycle.txt", FCVAR_GAMEDLL, "The gameplay cycle to use for random gameplay or ordered gameplay" );
ConVar ge_autoteam( "ge_autoteam", "0", FCVAR_REPLICATED|FCVAR_NOTIFY, "Automatically toggles teamplay based on the player count (supplied value) [4-32]",  true, 0, true, MAX_PLAYERS );

ConVar ge_autoautoteam("ge_autoautoteam", "1", FCVAR_GAMEDLL, "If set to 1, server will set ge_autoteam to the value specified in the current map script file.");
ConVar ge_gameplay_mode( "ge_gameplay_mode", "1", FCVAR_GAMEDLL, "Mode to choose next gameplay: \n\t0=Same as last map, \n\t1=Random from current map file, \n\t2=Ordered from Gameplay Cycle file" );
ConVar ge_gameplay( "ge_gameplay", "DeathMatch", FCVAR_GAMEDLL, "Sets the current gameplay mode.\nDefault is 'deathmatch'", GEGameplay_Callback );

ConVar ge_gameplay_threshold("ge_gameplay_threshold", "4", FCVAR_GAMEDLL, "Playercount that must be exceeded before gamemodes other than Deathmatch will be randomly chosen.");

ConVar ge_gameplay_modebuffercount("ge_gameplay_modebuffercount", "5", FCVAR_GAMEDLL, "How many other maps need to be played since the last time a map was played before it can be selected randomly without penalties.");
ConVar ge_gameplay_modebufferpenalty("ge_gameplay_modebufferpenalty", "500", FCVAR_GAMEDLL, "How much to take off of the weight of a mode for each time it appears in the buffer.");

#define GAMEPLAY_MODE_FIXED		0
#define GAMEPLAY_MODE_RANDOM	1
#define GAMEPLAY_MODE_CYCLE		2

extern ConVar ge_rounddelay;
extern ConVar ge_teamplay;
// TODO: Replace this with ge_matchtime at some point
extern ConVar mp_timelimit;
extern ConVar mp_chattime;

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

CGEBaseScenario::CGEBaseScenario()
{
	m_bIsOfficial = false;
}

void CGEBaseScenario::LoadConfig()
{
	char szCommand[255];

	Msg( "Executing gamemode [%s.cfg] config file\n", GetIdent() );
	Q_snprintf( szCommand,sizeof(szCommand), "exec %s.cfg\n", GetIdent() );
	engine->ServerCommand( szCommand );
}

void CGEBaseScenario::CreateCVar( const char* name, const char* defValue, const char* help )
{
	if ( !name || !defValue || !help )
	{
		Warning( "[GES GP] Failed to create ConVar due to invalid parameters!\n" );
		return;
	}

	ConVar *var = g_pCVar->FindVar( name );
	if ( var )
	{
		Warning( "[GES GP] Attempting to create CVAR %s twice!\n", name );
		m_vCVarList.FindAndRemove( var );
		g_pCVar->UnregisterConCommand( var );
	}

	// Create the CVar (registers it) and set the value to invoke OnCVarChanged
	var = new ConVar( name, defValue, FCVAR_GAMEDLL|FCVAR_NOTIFY, help, GEGPCVar_Callback );
	var->SetValue( defValue );
	m_vCVarList.AddToTail( var );
}

void CGEBaseScenario::UnloadConfig()
{	
	// Unregister each ConVar we are tracking
	for( int i=0; i < m_vCVarList.Size(); i++ )
		g_pCVar->UnregisterConCommand( m_vCVarList[i] );

	m_vCVarList.PurgeAndDeleteElements();
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

// Main container for Gameplay Event Listeners
CUtlVector<CGEGameplayEventListener*> g_GameplayEventListeners;

// Convienance Macro for Gameplay Events
#define GP_EVENT( event ) \
	for ( int i=0; i < g_GameplayEventListeners.Count(); i++ ) { \
		Assert( g_GameplayEventListeners[i] ); \
		g_GameplayEventListeners[i]->OnGameplayEvent( event ); \
	}


CGEGameplayEventListener::CGEGameplayEventListener()
{
	if ( !g_GameplayEventListeners.HasElement( this ) )
	{
		g_GameplayEventListeners.AddToTail( this );
		return;
	}

	DevWarning( "[GPEventListener] Event listener attempted double registration!\n" );
}

CGEGameplayEventListener::~CGEGameplayEventListener()
{
	int idx = g_GameplayEventListeners.Find( this );
	if ( idx != -1 )
		g_GameplayEventListeners.Remove( idx );
}


//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

int g_iScenarioIndex = -1;

CGEBaseGameplayManager::CGEBaseGameplayManager()
{
	ResetState();
}

CGEBaseGameplayManager::~CGEBaseGameplayManager()
{
	m_vScenarioCycle.Purge();
	m_vScenarioList.PurgeAndDeleteElements();
}

void CGEBaseGameplayManager::Init()
{
	// Prepare our lists
	LoadGamePlayList( "python\\ges\\GamePlay\\*.py" );
	LoadScenarioCycle();

	// Load our past gameplays
	ParseLogData();

	// Load the scenario
	LoadScenario();
}

void CGEBaseGameplayManager::Shutdown()
{
	// Shutdown the scenario
	ShutdownScenario();
}

void CGEBaseGameplayManager::ParseLogData()
{
	char *contents = (char*)UTIL_LoadFileForMe("gamesetuprecord.txt", NULL);

	if (!contents)
	{
		Msg("No rotation log!\n");
		return;
	}

	CUtlVector<char*> lines;
	char linebuffer[64];
	Q_SplitString(contents, "\n", lines);

	bool readingmodes = false;

	for (int i = 0; i < lines.Count(); i++)
	{
		// Ignore comments
		if (!Q_strncmp(lines[i], "//", 2))
			continue;

		if (readingmodes)
		{
			if (!Q_strncmp(lines[i], "-", 1)) // Our symbol for the end of a block.
				break;

			Q_StrLeft(lines[i], -1, linebuffer, 64); // Take off the newline character.

			// We could create a bunch of new strings, or we could just make use of the scenario list which already exists.
			for (int i = 0; i < m_vScenarioList.Count(); i++)
			{
				if (!Q_strcmp(linebuffer, m_vScenarioList[i]))
				{
					m_vRecentScenarioList.AddToTail(m_vScenarioList[i]);
					break;
				}
			}
		}

		if (!Q_strncmp(lines[i], "Modes:", 6))
			readingmodes = true;
	}

	// NOTE: We do not purge the data!
	ClearStringVector(lines);
	delete[] contents;
}

void CGEBaseGameplayManager::GetRecentModes(CUtlVector<const char*> &modenames)
{
	modenames.RemoveAll();

	int buffercount = MIN(ge_gameplay_modebuffercount.GetInt(), m_vRecentScenarioList.Count());

	for (int i = 0; i < buffercount; i++)
	{
		modenames.AddToTail(m_vRecentScenarioList[i]);
	}
}

void CGEBaseGameplayManager::BroadcastMatchStart()
{
	// Let all the clients know what game mode we are in now
	IGameEvent* pEvent = gameeventmanager->CreateEvent("gamemode_change");
	if ( pEvent )
	{
		pEvent->SetString( "ident", GetScenario()->GetIdent() );
		pEvent->SetBool( "official", GetScenario()->IsOfficial() );
		gameeventmanager->FireEvent(pEvent);
	}

	// Print it to the chat
	UTIL_ClientPrintAll( HUD_PRINTTALK, "#GES_Gameplay_Changed", GetScenario()->GetPrintName() );
}

void CGEBaseGameplayManager::BroadcastRoundStart()
{
	// Let all the clients know that the round started
	IGameEvent* pEvent = gameeventmanager->CreateEvent("round_start");
	if ( pEvent )
	{
		pEvent->SetString( "gameplay", GetScenario()->GetIdent() );
		pEvent->SetBool( "teamplay", GEMPRules()->IsTeamplay() );
		pEvent->SetInt( "roundcount", m_iRoundCount );
		gameeventmanager->FireEvent(pEvent);
	}

	// Print it to the chat
	UTIL_ClientPrintAll( HUD_PRINTTALK, "#GES_RoundRestart" );
}

void CGEBaseGameplayManager::BroadcastRoundEnd( bool showreport )
{
	// Let all the clients know that the round ended
	IGameEvent* pEvent = gameeventmanager->CreateEvent("round_end");
	if ( pEvent )
	{
		int winnerindex = GEMPRules()->GetRoundWinner();
		int winnerscore = -1;
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(winnerindex);

		if (pPlayer)
			winnerscore = ToGEMPPlayer(pPlayer)->GetRoundScore();

		pEvent->SetInt( "winnerid", GEMPRules()->GetRoundWinner() );
		pEvent->SetInt( "winnerscore", winnerscore );
		pEvent->SetInt( "teamid", GEMPRules()->GetRoundTeamWinner() );
		pEvent->SetBool( "isfinal", false );
		pEvent->SetBool( "showreport", showreport );
		pEvent->SetInt( "roundlength", gpGlobals->curtime - m_flRoundStart );
		pEvent->SetInt( "roundcount", m_iRoundCount );
		GEStats()->SetAwardsInEvent( pEvent );
		gameeventmanager->FireEvent(pEvent);
	}

	// Print it to the chat
	IGameEvent *event = gameeventmanager->CreateEvent("round_ranks");
	if (event)
	{
		CUtlVector<CGEMPPlayer*> rankedPlayers;
		GEMPRules()->GetRankSortedPlayers(rankedPlayers);
		event->SetBool("isfinal", false);

		for (int i = 0; i < 16; i++)
		{
			char strbuffer[8];
			Q_snprintf(strbuffer, 8, "id%d", i + 1);
			if ( i < rankedPlayers.Count() )
				event->SetInt(strbuffer, rankedPlayers[i]->GetUserID());
			else
				event->SetInt(strbuffer, -1);

			Q_snprintf(strbuffer, 8, "sc%d", i + 1);
			if (i < rankedPlayers.Count())
				event->SetInt(strbuffer, rankedPlayers[i]->GetRoundScore());
			else
				event->SetInt(strbuffer, -1);
		}

		gameeventmanager->FireEvent(event);
	}

	// Print it to the chat
	UTIL_ClientPrintAll( HUD_PRINTTALK, "#GES_RoundEnd" );
}

void CGEBaseGameplayManager::BroadcastMatchEnd()
{
	// TODO: This should be replaced with "match_end" event
	// Let all the clients know that the round ended
	IGameEvent* pEvent = gameeventmanager->CreateEvent("round_end");
	if ( pEvent )
	{
		int winnerindex = GEMPRules()->GetRoundWinner();
		int winnerscore = -1;
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(winnerindex);

		if (pPlayer)
			winnerscore = ToGEMPPlayer(pPlayer)->GetMatchScore();


		pEvent->SetInt( "winnerid", GEMPRules()->GetRoundWinner() );
		pEvent->SetInt( "winnerscore", winnerscore );
		pEvent->SetInt( "teamid", GEMPRules()->GetRoundTeamWinner() );
		pEvent->SetBool( "isfinal", true );
		pEvent->SetBool( "showreport", true );
		pEvent->SetInt( "roundlength", gpGlobals->curtime - m_flRoundStart );
		pEvent->SetInt( "roundcount", m_iRoundCount );
		GEStats()->SetAwardsInEvent( pEvent );
		gameeventmanager->FireEvent(pEvent);
	}

	// Print it to the chat
	IGameEvent *event = gameeventmanager->CreateEvent("round_ranks");
	if (event)
	{
		CUtlVector<CGEMPPlayer*> rankedPlayers;
		GEMPRules()->GetRankSortedPlayers(rankedPlayers);
		event->SetBool("isfinal", true);

		for (int i = 0; i < 16; i++)
		{
			char strbuffer[8];
			Q_snprintf(strbuffer, 8, "id%d", i + 1);
			if (i < rankedPlayers.Count())
				event->SetInt(strbuffer, rankedPlayers[i]->GetUserID());
			else
				event->SetInt(strbuffer, -1);

			Q_snprintf(strbuffer, 8, "sc%d", i + 1);
			if (i < rankedPlayers.Count())
				event->SetInt( strbuffer, rankedPlayers[i]->GetMatchScore() );
			else
				event->SetInt( strbuffer, -1 );
		}

		gameeventmanager->FireEvent(event);
	}

	// Print it to the chat
	UTIL_ClientPrintAll( HUD_PRINTTALK, "#GES_MatchEnd" );
}

bool CGEBaseGameplayManager::LoadScenario()
{
	return LoadScenario( GetNextScenario() );
}

bool CGEBaseGameplayManager::LoadScenario( const char *ident )
{
	char prev_ident[32] = "DeathMatch";

	// Shutdown any current scenario
	if ( IsValidScenario() ) {
		Q_strncpy( prev_ident, GetScenario()->GetIdent(), 32 );
		ShutdownScenario();
	}

	Msg( "Attempting to load scenario: %s\n", ident );

	// Call up to load the new scenario
	// If this fails, we will continue with the previously loaded scenario
	if ( !DoLoadScenario( ident ) ) {
		Warning( "Scenario load failed for %s! Reverting back to %s.\n", ident, prev_ident );
		
		if ( !DoLoadScenario( prev_ident ) )
			Warning( "A catastrophic error occurred while reverting back to %s! Exit and restart the game!\n", prev_ident );
		else
			Msg( "Successfully reverted back to %s\n", prev_ident );
	} else {
		Msg( "Successfully loaded scenario %s!\n", ident );
	}

	// Set up the world and notify Python
	InitScenario();

	// Start the match
	StartMatch();

	return true;
}

extern ConVar ge_bot_threshold;

const char *CGEBaseGameplayManager::GetNextScenario()
{
	int mode = ge_gameplay_mode.GetInt();
	int count = m_vScenarioCycle.Count();

	// Figure out teamplay related stuff here since it factors into the gamemode we choose.

	MapSelectionData *pMapData = GEMPRules()->GetMapManager()->GetCurrentMapSelectionData();
	int teamthresh = -1;

	if (ge_autoautoteam.GetBool() && pMapData)
	{
		teamthresh = pMapData->teamthreshold;
		ge_autoteam.SetValue(teamthresh);
	}
	else if (!ge_autoautoteam.GetBool())
		teamthresh = ge_autoteam.GetInt();

	int iNumConnections = 0;

	// Find out how many people are actually connected to the server.
	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		if (engine->GetPlayerNetInfo(i))
			iNumConnections++;
	}

	// Consider bots too.
	iNumConnections = MAX(iNumConnections, ge_bot_threshold.GetInt());

	if (ge_teamplay.GetInt() != 1)
	{
		if (iNumConnections >= teamthresh)
			ge_teamplay.SetValue(2); // Premptively go into teamplay if we've got enough players for it.
		else
			ge_teamplay.SetValue(0);
	}

	if (mode == GAMEPLAY_MODE_RANDOM)
	{
		CUtlVector<char*>	gamemodes;
		CUtlVector<int>		weights;
		CUtlVector<const char*>	recentgamemodes;

		// Random game mode, pick a scenario in our list
		if (iNumConnections <= ge_gameplay_threshold.GetInt())
			return "deathmatch";

		// Random game mode according to map script.  
		GEMPRules()->GetMapManager()->GetMapGameplayList(gamemodes, weights, iNumConnections >= teamthresh);

		// Adjust the weight of gamemodes we just played.
		GetRecentModes(recentgamemodes);
		int deductionamount = ge_gameplay_modebufferpenalty.GetInt();
		int totalweight = 0;

		for (int i = 0; i < gamemodes.Count(); i++)
		{
			totalweight += weights[i];
		}

		if (recentgamemodes.Count())
		{
			for (int b = 0; b < recentgamemodes.Count(); b++)
			{
				if (totalweight <= deductionamount)
					break; // Make sure we'll have at least one gamemode with above 0 weight.

				for (int l = 0; l < gamemodes.Count(); l++)
				{
					if (!Q_strcmp(gamemodes[l], recentgamemodes[b]))
					{
						int deduction = MIN(weights[l], deductionamount);
						weights[l] -= deduction;
						totalweight -= deduction;
						break;
					}
				}
			}
		}

		if (gamemodes.Count())
		{
			return GERandomWeighted<char*>(gamemodes.Base(), weights.Base(), gamemodes.Count());
		}
		else
		{
			Warning("No gamemodes found in map script, defaulting to first gameplay in scenariocycle!\n");
			return m_vScenarioCycle[0].ToCStr();
		}
	}
	else if (mode == GAMEPLAY_MODE_CYCLE)
	{
		// Ordered game mode, get the next scenario in our list
		if (count > 0)
		{
			// Increment our index, rolling over if we exceed our count
			if (++g_iScenarioIndex >= count)
				g_iScenarioIndex = 0;

			return m_vScenarioCycle[g_iScenarioIndex].ToCStr();
		}
	}

	// Fixed game mode, load the scenario set in our ConVar
	return ge_gameplay.GetString();
}

void CGEBaseGameplayManager::InitScenario()
{
	// Reset our state
	ResetState();

	// Call our listeners before Python init
	GP_EVENT( SCENARIO_INIT );

	// Enable precache during scenario load
	bool precache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	// Let the scenario initialize everything
	GetScenario()->Init();

	// Reset our precache state
	CBaseEntity::SetAllowPrecache( precache );

	// Load the configuration for the scenario
	GetScenario()->LoadConfig();

	// Call into reconnect each player again to run initializations
	FOR_EACH_MPPLAYER( pPlayer )		
		GetScenario()->ClientConnect( pPlayer );
	END_OF_PLAYER_LOOP()

	m_vRecentScenarioList.AddToHead(GetScenario()->GetIdent());

	// Now let anyone who is interested know we are finished initilazing.  
	// Added to fix the help bug and avoid making any more by switching the order of things, but should be useful in its own right.
	GP_EVENT(SCENARIO_POST_INIT);
}

void CGEBaseGameplayManager::ShutdownScenario()
{
	// Disconnect all players for cleanup
	FOR_EACH_PLAYER( pPlayer )		
		GetScenario()->ClientDisconnect( pPlayer );	
	END_OF_PLAYER_LOOP()

	// Unload the configuration variables
	GetScenario()->UnloadConfig();

	// Tell everyone we are shutting down the scenario
	GP_EVENT( SCENARIO_SHUTDOWN );
	
	// Final shutdown with Python
	GetScenario()->Shutdown();
}

void CGEBaseGameplayManager::ResetState()
{
	m_bGameOver = false;
	m_bRoundLocked = false;
	m_iRoundState = CGEBaseGameplayManager::NONE;
	m_iRoundCount = 0;
	m_flNextThink = 0;
	m_flIntermissionEndTime = -1.0f;
	m_flRoundStart = -1.0f;
	m_flMatchStart = -1.0f;
}

// Various checks for round state
bool CGEBaseGameplayManager::IsInRound()
{
	return m_iRoundState == PLAYING;
}

bool CGEBaseGameplayManager::IsInIntermission()
{
	return IsInRoundIntermission() || IsInFinalIntermission();
}

bool CGEBaseGameplayManager::IsInRoundIntermission()
{
	return m_iRoundState == INTERMISSION;
}

bool CGEBaseGameplayManager::IsInFinalIntermission()
{
	return m_iRoundState == GAME_OVER;
}

// Controls the lifecycle of the scenario to include
// starting and ending rounds, ending the match and
// triggering level transitions
void CGEBaseGameplayManager::OnThink()
{
	// If we aren't ready to think don't do it
	if( m_flNextThink > gpGlobals->curtime )
		return;

	m_flNextThink = gpGlobals->curtime + 0.1f;

	// Check for round pre start
	if ( m_iRoundState == PRE_START )
	{
		StartRound();
		return;
	}

	// Check if the game over time is over
	if ( IsGameOver() && IsInFinalIntermission() && GetRemainingIntermission() <= 0 )
	{
		GEMPRules()->SetupChangeLevel();
		return;
	}

	// Check if the round intermission is over
	if ( IsInRoundIntermission() && GetRemainingIntermission() <= 0 )
	{
		// Check if the match should end instead of starting a new round
		if ( ShouldEndMatch() )
			EndMatch();
		else
			StartRound();

		return;
	}

	// Check if we are in a round
	if ( IsInRound() )
	{
		// Check if we need to end the match (ends round first)
		if ( ShouldEndMatch() )
			EndMatch();
		// Otherwise, check if we need to end the round
		else if ( ShouldEndRound() )		
			EndRound();
		// Otherwise, let the scenario think
		else		
			GetScenario()->OnThink();
	}
}

void CGEBaseGameplayManager::StartMatch()
{
	// Set our match time
	m_flMatchStart = gpGlobals->curtime;

	// Set us up to start the round on the next think cycle
	m_bGameOver = false;
	m_iRoundState = PRE_START;

	// Let our listeners know we are starting a new match
	GP_EVENT( MATCH_START );

	// Let our clients know what scenario we are playing
	BroadcastMatchStart();
}

void CGEBaseGameplayManager::StartRound()
{
	if ( ge_autoteam.GetInt() > 0 && ge_teamplay.GetInt() != 1 ) // If teamplay is 1 then it is forced on.
	{

		int iNumConnections = 0;

		// If we had 1 or more people than ge_autoteam on the last map, enforce teamplay immediately
		for (int i = 0; i < gpGlobals->maxClients; i++)
		{
			if (engine->GetPlayerNetInfo(i))
				iNumConnections++;
		}

		// If we have the requisite active players, enforce teamplay
		iNumConnections = MAX(iNumConnections, ge_bot_threshold.GetInt());

		// If we don't, and ge_teamplay > 1 (ie not set by the player), then deactivate teamplay
		if ( iNumConnections >= ge_autoteam.GetInt() )
			ge_teamplay.SetValue(2);
		else
			ge_teamplay.SetValue(0);
	}

	GetScenario()->BeforeSetupRound();

	// Reload the world sparing only level designer placed entities
	// This must be called before the ROUND_START event
	GEMPRules()->SetupRound();

	// Officially start this round
	m_iRoundCount++;
	m_iRoundState = PLAYING;
	m_flRoundStart = gpGlobals->curtime;

	// Let everyone setup for the round
	GP_EVENT( ROUND_START );
	
	// Call into our scenario before spawning players
	GetScenario()->OnRoundBegin();

	// Respawn all active players
	GEMPRules()->SpawnPlayers();

	// Let everything know we are starting a new round
	BroadcastRoundStart();
}

void CGEBaseGameplayManager::EndRound( bool showreport /*=true*/ )
{
	// Ignore this call if we are not playing a round
	if ( !IsInRound() )
		return;

	// Call into python to do post round cleanup and score setting
	GetScenario()->OnRoundEnd();

	// Set us in intermission
	m_iRoundState = INTERMISSION;

	if ( showreport )
		// Delay enough so that the scores don't get reset before the round report is visible
		m_flIntermissionEndTime = gpGlobals->curtime + MAX( ge_rounddelay.GetInt(), 0.5f );
	else
		// Only give 3 second delay if we didn't count this round
		m_flIntermissionEndTime = gpGlobals->curtime + 3.0f;

	GP_EVENT( ROUND_END );

	// Tell players we finished the round
	BroadcastRoundEnd( showreport );
}

void CGEBaseGameplayManager::EndMatch()
{
	// Set this upfront
	m_bGameOver = true;

	// If we are currently in a round, we need to end it first
	if ( IsInRound() )
	{
		if ( m_iRoundCount > 1 )
		{
			// Do a full round ending since we played more than 1 round
			EndRound();
			return;
		}
		else
		{
			// Only perform a score calculation since we haven't played more than 1 round
			// We'll only show the match report to players

			// Call into python to do post round cleanup and score setting
			GetScenario()->OnRoundEnd();

			// Cleanup the round (does scores)
			GP_EVENT( ROUND_END );
		}
	}

	// Set our game over status
	m_iRoundState = GAME_OVER;
	m_flIntermissionEndTime = gpGlobals->curtime + mp_chattime.GetInt();

	GP_EVENT( MATCH_END );

	// Tell clients we finished the round
	BroadcastMatchEnd();
}

float CGEBaseGameplayManager::GetRemainingIntermission()
{
	return MAX(m_flIntermissionEndTime - gpGlobals->curtime, 0 );
}

bool CGEBaseGameplayManager::ShouldEndRound()
{
	// Check time constraints
	if ( GEMPRules()->IsRoundTimeRunning() && GEMPRules()->GetRoundTimeRemaining() <= 0 )
	{
		// We ran out of time and our scenario says we can end
		if ( GetScenario()->CanRoundEnd() )
			return true;
	}

	return false;
}

bool CGEBaseGameplayManager::ShouldEndMatch()
{
	// We are already over!
	if ( IsGameOver() )
		return true;

	// We must be able to end our round to end the match
	if ( IsInRound() && GEMPRules()->IsRoundTimeRunning() && !ShouldEndRound() )
		return false;

	// Check time constraints
	if ( GEMPRules()->IsMatchTimeRunning() && (GEMPRules()->GetMatchTimeRemaining() <= 0 
		|| (IsInRoundIntermission() && GEMPRules()->GetMatchTimeRemaining() <= 30.0f)) )
	{
		// We ran out of time and our scenario says we can end
		if ( GetScenario()->CanMatchEnd() )
			return true;
	}

	return false;
}

void CGEBaseGameplayManager::LoadScenarioCycle()
{
	// Clear out the existing cycle and index
	m_vScenarioCycle.Purge();

	const char *curr_scenario = ge_gameplay.GetString();
	const char *cycle_file = ge_gp_cyclefile.GetString();
	Assert(cycle_file != NULL);

	CUtlBuffer buf;
	buf.SetBufferType(true, false);
	if (filesystem->ReadFile(cycle_file, "MOD", buf))
	{
		char line[32];
		buf.GetLine(line, 32);
		while (line[0])
		{
			// Strip out the spaces in the name
			GEUTIL_StripWhitespace(line);

			if (!IsValidGamePlay(line))
			{ Warning("Invalid scenario '%s' included in gameplay cycle file. Ignored.\n", line); }
			else if (!Q_strncmp(line, "//", 2))
			{ /* Ignore this line */ }
			else
			{ m_vScenarioCycle.AddToTail(AllocPooledString(line)); }

			// Load next line
			buf.GetLine(line, 32);
				
		}

		// TODO: This might cause issues when running random or cycled scenarios on first load...
		// Reset the gameplay index 
		if (g_iScenarioIndex < 0 || g_iScenarioIndex >= m_vScenarioCycle.Count())
		{
			for (int i = 0; i < m_vScenarioCycle.Count(); i++)
			{
				// Find the first match for our current game mode if it exists in the list
				if (curr_scenario && Q_stricmp(m_vScenarioCycle[i].ToCStr(), curr_scenario) == 0)
				{
					g_iScenarioIndex = i;
					break;
				}
			}
		}
	}

	// If somehow we have no scenarios in the list then add the current one
	if (m_vScenarioCycle.Count() == 0)
	{
		m_vScenarioCycle.AddToTail(AllocPooledString(curr_scenario));
	}
}

bool CGEBaseGameplayManager::IsValidGamePlay( const char *ident )
{
	for ( int i=0; ident != NULL && i < m_vScenarioList.Count(); i++ ) 
	{
		if ( !Q_stricmp( ident, m_vScenarioList[i] ) )
			return true;
	}

	return false;
}

void CGEBaseGameplayManager::PrintGamePlayList()
{
	Msg("Available Game Modes: \n");
	for (int i=0; i < m_vScenarioList.Count(); i++) 
	{
		Msg( "%s\n", m_vScenarioList[i] );
	}
}

void CGEBaseGameplayManager::LoadGamePlayList(const char* path)
{
	for ( int i=0; i < m_vScenarioList.Count(); i++ )
		delete [] m_vScenarioList[i];
	m_vScenarioList.RemoveAll();

	// TODO: This is on a fast-track to deletion. This stuff should be handled in Python...
	FileFindHandle_t finder;
	const char *fileName = filesystem->FindFirstEx( path, "MOD", &finder );
	while ( fileName )
	{
		if ( Q_strncmp( fileName, "__", 2 ) )
		{
			char *fileNameNoExt = new char[64];
			Q_StripExtension( fileName, fileNameNoExt, 64 );
			m_vScenarioList.AddToTail( fileNameNoExt );
		}

		fileName = filesystem->FindNext( finder );
	}
	filesystem->FindClose( finder );
}

extern ConVar nextlevel;
static CUtlVector<char*> gMapList;
static void LoadMapList() {
	FileFindHandle_t findHandle; // note: FileFINDHandle
	char *file;

	const char *pFilename = filesystem->FindFirstEx( "maps\\*.bsp", "MOD", &findHandle );
	while ( pFilename )
	{
		file = new char[32];
		Q_strncpy( file, pFilename, MIN( Q_strlen(pFilename) - 3, 32 ) );
		gMapList.AddToTail( file );

		pFilename = filesystem->FindNext( findHandle );
	}

	filesystem->FindClose( findHandle );
}

// Map auto completion for ge_endmatch entries
static int MapAutoComplete( char const *partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] )
{
	int i, k;
	CUtlVector<char*> tokens;
	Q_SplitString( partial, " ", tokens );
	if ( tokens.Count() < 2 )
		return 0;

	if ( gMapList.Count() == 0 )
		LoadMapList();

	for ( i=0, k=0; (i < gMapList.Count() && k < COMMAND_COMPLETION_MAXITEMS); i++ )
	{
		if ( StringHasPrefix( gMapList[i], tokens[1] ) ) {
			Q_strncpy( commands[k], tokens[0], COMMAND_COMPLETION_ITEM_LENGTH );
			Q_strncat( commands[k], " ", COMMAND_COMPLETION_ITEM_LENGTH );
			Q_strncat( commands[k], gMapList[i], COMMAND_COMPLETION_ITEM_LENGTH );
			k++;
		}
	}

	ClearStringVector( tokens );

	return k; // number of entries
}

// Override for changelevel, this calls ge_endmatch
CON_COMMAND_F_COMPLETION(__ovr_changelevel, "Change the current level after ending the current round/match. Use `changelevel [mapname] 0` to change immediately.", 0, MapAutoComplete)
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
	{
		Msg( "You must be a server admin to use that command\n" );
		return;
	}

	if ( !GEMPRules() || !GEGameplay() )
	{
		Warning( "Failed to run command, not playing a game!\n" );
		return;
	}

	if ( !engine->IsMapValid(args[1]) )
	{
		Warning( "Failed to run command, invalid map name supplied!\n" );
		return;
	}

	// If we are in the final intermission or supplied a second argument, change immediately
	if ( GEGameplay()->IsInFinalIntermission() || args.ArgC() > 2 )
	{
		GEMPRules()->SetupChangeLevel( args[1] );
	}
	else
	{
		// Let ge_endmatch command handle the sequencing
		engine->ServerCommand( UTIL_VarArgs( "ge_endmatch %s\n", args[1] ) );
	}
}

CON_COMMAND(ge_gameplaylistrefresh, "Refreshes the list of known gameplays, useful for servers who add gameplays while the server is still running")
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
	{
		Msg( "You must be a server admin to use that command\n" );
		return;
	}
	
	if ( !GEGameplay() )
	{
		Warning( "Failed to run command, not playing a game!\n" );
		return;
	}

	// Reload the gameplay without using the cache
	GEGameplay()->LoadGamePlayList( "scripts/python/GamePlay/*.py" );
}

// WARNING: Deprecated! Use ge_endround instead!
CON_COMMAND(ge_restartround, "Restart the current round showing scores always")
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() || !GEGameplay() )
	{
		Msg( "You must be a server admin to use that command\n" );
		return;
	}
	
	if ( !GEMPRules() || !GEGameplay() )
	{
		Warning( "Failed to run command, not playing a game!\n" );
		return;
	}

	Msg( "Warning! This command is deprecated, use ge_endround instead!\n" );
	engine->ServerCommand( "ge_endround\n" );
}

CON_COMMAND(ge_endround, "End the current round, use `ge_endround 0` to skip scores" )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
	{
		Msg( "You must be a server admin to use that command\n" );
		return;
	}
	
	if ( !GEMPRules() || !GEGameplay() )
	{
		Warning( "Failed to run command, not playing a game!\n" );
		return;
	}

	// Ignore if we are not in a round
	if ( !GEGameplay()->IsInRound() )
		return;

	bool showreport = true;
	if ( args.ArgC() > 1 && args[1][0] == '0' )
		showreport = false;

	GEGameplay()->EndRound( showreport );
}

CON_COMMAND(ge_endround_keepweapons, "End the current round but keep the same weapon set even if randomized.")
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
	{
		Msg( "You must be a server admin to use that command\n" );
		return;
	}
	
	if ( !GEMPRules() || !GEGameplay() )
	{
		Warning( "Failed to run command, not playing a game!\n" );
		return;
	}

	// Ignore if we are not in a round
	if ( !GEGameplay()->IsInRound() )
		return;

	// This is where the magic happens
	GEMPRules()->GetLoadoutManager()->KeepLoadoutOnNextChange();

	bool showreport = true;
	if ( args.ArgC() > 1 && args[1][0] == '0' )
		showreport = false;

	GEGameplay()->EndRound( showreport );
}

CON_COMMAND_F_COMPLETION(ge_endmatch, "Ends the match loading the next map or one specified (eg ge_endmatch [mapname]).", 0, MapAutoComplete)
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
	{
		Msg( "You must be a server admin to use that command\n" );
		return;
	}

	// Don't do anything if we are already game over
	if ( GEGameplay()->IsGameOver() )
		return;

	// Set the next level if given
	if ( args.ArgC() > 1 ) {
		if ( engine->IsMapValid( args[1] ) )
			nextlevel.SetValue( args[1] );
		else
			Warning( "Invalid map provided to ge_endmatch, ignoring.\n" );
	}

	GEGameplay()->EndMatch();
}

CON_COMMAND(ge_gameplaylist, "Lists the possible gameplay selections.")
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
	{
		Msg( "You must be a server admin to use that command\n" );
		return;
	}

	if ( !GEGameplay() )
	{
		Warning( "Failed to run command, not playing a game!\n" );
		return;
	}

	GEGameplay()->PrintGamePlayList();
}

#ifdef _DEBUG
CON_COMMAND( ge_cyclegameplay, "Simulates a map transition" )
{
	GEGameplay()->LoadScenario();
}
#endif
