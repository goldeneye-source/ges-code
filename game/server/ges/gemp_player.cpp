///////////// Copyright © 2008 LodleNet. All rights reserved. /////////////
//
//   Project     : Server (GES)
//   File        : gemp_player.cpp
//   Description :
//      [TODO: Write the purpose of gemp_player.cpp.]
//
//   Created On: 2/20/2009 1:38:28 PM
//   Created By: Mark Chandler <mailto:admin@lodle.net>
////////////////////////////////////////////////////////////////////////////
#include "cbase.h"

#include "team.h"
#include "predicted_viewmodel.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "in_buttons.h"
#include "ammodef.h"
#include "viewport_panel_names.h"
#include "networkstringtable_gamedll.h"

#include "ge_utils.h"
#include "ge_playerspawn.h"
#include "ge_gameplay.h"
#include "ge_radarresource.h"
#include "ge_tokenmanager.h"
#include "ge_loadoutmanager.h"
#include "ge_weapon.h"
#include "ge_character_data.h"
#include "ge_achievement_defs.h"
#include "ge_player_shared.h"
#include "gebot_player.h"
#include "gemp_gamerules.h"

#include "gemp_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar ge_allowradar;
extern ConVar ge_radar_range;

LINK_ENTITY_TO_CLASS( mp_player, CGEMPPlayer );

IMPLEMENT_SERVERCLASS_ST(CGEMPPlayer, DT_GEMP_Player)
	SendPropFloat( SENDINFO( m_flStartJumpZ ) ),
	SendPropFloat( SENDINFO( m_flNextJumpTime ) ),
	SendPropFloat( SENDINFO( m_flLastJumpTime )),
	SendPropFloat( SENDINFO( m_flJumpPenalty )),
	SendPropFloat( SENDINFO( m_flLastLandVelocity )),
	SendPropFloat( SENDINFO( m_flRunTime )),
	SendPropInt( SENDINFO( m_flRunCode )),
	SendPropInt( SENDINFO( m_iSteamIDHash )),
	SendPropArray3( SENDINFO_ARRAY3(m_iWeaponSkinInUse), SendPropInt(SENDINFO_ARRAY(m_iWeaponSkinInUse))),
END_SEND_TABLE()

BEGIN_DATADESC( CGEMPPlayer )
END_DATADESC()

PRECACHE_REGISTER(mp_player);

extern ConVar ge_debug_playerspawns;

#define MODEL_CHANGE_INTERVAL	4.0f
#define TEAM_CHANGE_INTERVAL	4.0f
#define GE_MIN_CAMP_DIST		36.0f	// 3 ft
#define GE_MAX_THROWN_OBJECTS	10
#define GE_CLEARSPAWN_DIST		512.0f
#define GEPLAYER_PHYSDAMAGE_SCALE 1.0f

struct GESpawnLoc
{
	CBaseEntity *pSpawn;
	int idx;
	int enemyCount;
};

int SpawnSort( GESpawnLoc* const *a, GESpawnLoc* const *b ) { return ( (*a)->enemyCount - (*b)->enemyCount ); };
void ShowTeamNotice( const char *title, const char *msg, const  char *img );

CGEMPPlayer::CGEMPPlayer()
{
	m_flNextModelChangeTime = 0.0f;
	m_flNextTeamChangeTime = 0.0f;

	m_flTeamJoinTime = 0.0f;

	m_iSteamIDHash = 0;
	m_iDevStatus = 0;
	m_iSkinsCode = 0;
	m_iClientSkinsCode = 0;
	m_iCampingState = 0;

	m_szCleanName[0] = '\0';
	m_szAchList[0] = '\0';
	m_iAchCount = 0;
	m_iAutoTeamSwaps = 0;
	m_flNextSpawnWaitNotice = 0;
	m_bAchReportPending = false;

	SetSpawnState( SS_INITIAL );
	m_flNextSpawnTry = 0.0f;
	m_bPreSpawn = true;
	m_bFirstSpawn = true;
	m_bSpawnFromDeath = false;

	m_vLastDeath = Vector(-1, -1, -1);
	m_vLastSpawn = Vector(-1, -1, -1);

	for (int i = 0; i < WEAPON_RANDOM; i++)
	{
		m_iWeaponSkinInUse.Set(i, 0);
	}
}

CGEMPPlayer::~CGEMPPlayer()
{
	return;
}

void CGEMPPlayer::Precache()
{
	BaseClass::Precache();

	PrecacheScriptSound( "GEPlayer.Gasp" );
	PrecacheScriptSound( "GEPlayer.Death" );

	// Game over music!
	PrecacheScriptSound( "GEGamePlay.GameOver" );
	PrecacheScriptSound( "GEGamePlay.JanusLose" );
	PrecacheScriptSound( "GEGamePlay.JanusWin" );
	PrecacheScriptSound( "GEGamePlay.MI6Lose" );
	PrecacheScriptSound( "GEGamePlay.MI6Win" );

	// Gameplay music!
	PrecacheScriptSound( "GEGamePlay.Overtime" );
	PrecacheScriptSound( "GEGamePlay.Token_Grab" );
	PrecacheScriptSound( "GEGamePlay.Token_Grab_Enemy" );
	PrecacheScriptSound( "GEGamePlay.Token_Drop_Friend" );
	PrecacheScriptSound( "GEGamePlay.Token_Drop_Enemy" );
	PrecacheScriptSound( "GEGamePlay.Token_Capture_Friend" );
	PrecacheScriptSound( "GEGamePlay.Token_Capture_Enemy" );

	// Achievement music!
	PrecacheScriptSound( "GEAchievement.Earned" );
	PrecacheScriptSound( "GEAchievement.Unlocked" );
}

// This is called by any thrown weapon spawned by the player to prevent spamming
// the world
void CGEMPPlayer::AddThrownObject( CBaseEntity *pObj )
{
	if ( m_hThrownObjects.Count() >= GE_MAX_THROWN_OBJECTS )
	{
		CBaseEntity *pEnt = (CBaseEntity*)m_hThrownObjects[0].Get();
		// If we are at our limit, remove the first object thrown
		if ( pEnt )
			UTIL_Remove( pEnt );
		m_hThrownObjects.Remove(0);
	}

	int loc = m_hThrownObjects.AddToTail();
	m_hThrownObjects[loc] = pObj;
}

void CGEMPPlayer::UpdateCampingTime()
{
	if ( m_flNextCampCheck > gpGlobals->curtime )
		return;

	if ( IsObserver() )
	{
		m_flCampingTime = 0.0f;
		return;
	}

	float movement = m_vLastOrigin.DistTo( GetAbsOrigin() );
	// Copy it over for the next measurement
	m_vLastOrigin = GetAbsOrigin();

	if ( movement > GE_MIN_CAMP_DIST )
	{
		m_flLastMoveTime = gpGlobals->curtime;
		m_flCampingTime = MAX( 0, m_flCampingTime - 0.5f );
	}
	else
	{
		m_flCampingTime += 0.2f;
	}

	int newState, percent = GetCampingPercent();
	// Determine threshold transition and our event message appropriately
	if ( percent < 50 )
		newState = 0;
	else if ( percent < 100 )
		newState = 1;
	else
		newState = 2;

	if ( m_iCampingState != newState )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "player_camping" );
		if( event )
		{
			event->SetInt( "userid", GetUserID() );
			event->SetInt( "state", newState );
			gameeventmanager->FireEvent( event, true );
		}
	}

	m_iCampingState = newState;
	m_flNextCampCheck = gpGlobals->curtime + 0.2f;
}

int CGEMPPlayer::GetCampingPercent()
{
	if ( m_flCampingTime > RADAR_BAD_CAMP_TIME )
		return 100;
	else if ( m_flCampingTime > RADAR_MAX_CAMP_TIME )
		return (int)RemapValClamped( m_flCampingTime, RADAR_MAX_CAMP_TIME, RADAR_BAD_CAMP_TIME, 50.0f, 100.0f );
	else if ( m_flCampingTime >= RADAR_MIN_CAMP_TIME )
		return (int)RemapValClamped( m_flCampingTime, RADAR_MIN_CAMP_TIME, RADAR_MAX_CAMP_TIME, 0, 50.0f );
	
	return 0;
}

void CGEMPPlayer::UpdateJumpPenalty()
{
	// Check our penalty time decay
	if (m_flJumpPenalty > 0 && GetGroundEntity() != NULL)
	{
		//dt * 2000/x
		m_flJumpPenalty -= gpGlobals->frametime* (2000 / m_flJumpPenalty);
		m_flJumpPenalty = MAX(m_flJumpPenalty, 0.0f);
	}
}

void CGEMPPlayer::PickDefaultSpawnTeam()
{
	if ( GetTeamNumber() == TEAM_UNASSIGNED )
	{
		if ( GERules()->IsTeamplay() == false )
		{
			if ( GetModelPtr() == NULL )
			{
				BaseClass::SetPlayerModel();
				ChangeTeam( TEAM_UNASSIGNED );
			}
		}
		else
		{
			CTeam *pJanus = g_Teams[TEAM_JANUS];
			CTeam *pMI6 = g_Teams[TEAM_MI6];

			if ( pJanus == NULL || pMI6 == NULL )
			{
				ChangeTeam( random->RandomInt( TEAM_MI6, TEAM_JANUS ) );
			}
			else
			{
				if ( pJanus->GetNumPlayers() > pMI6->GetNumPlayers() )
				{
					ChangeTeam( TEAM_MI6 );
				}
				else if ( pJanus->GetNumPlayers() < pMI6->GetNumPlayers() )
				{
					ChangeTeam( TEAM_JANUS );
				}
				else
				{
					ChangeTeam( random->RandomInt( TEAM_MI6, TEAM_JANUS ) );
				}
			}
		}
	}
}

void CGEMPPlayer::InitialSpawn()
{
	m_iLastSpecSpawn = 0;

	// This sends a Clear to the hint display to flush any remaining hints out
	HintMessage( "", 0 );

	// Force name message to be sent out again
	SetPlayerName( GetPlayerName() );

	// Place the player on the spectator team until they choose a character to play with
	ChangeTeam(TEAM_SPECTATOR);
	// If there are any spectator spots or no-one is playing, use a spawn point as the intro camera posistion.
	ObserverTransistion();

	SetMaxHealth( MAX_HEALTH );
	SetMaxArmor( MAX_ARMOR );

	m_bPreSpawn = true;

	BaseClass::InitialSpawn();
}

void CGEMPPlayer::Spawn()
{
	Precache();

	m_flNextModelChangeTime = 0.0f;
	m_flNextTeamChangeTime = 0.0f;

	RemoveAllItems(true);
	KnockOffHat(true);


	if ( (m_bPreSpawn && IsSpawnState( SS_INITIAL )) || GetTeamNumber() == TEAM_SPECTATOR )
	{
		// Prespawn and spectating always observer
		if ( !IsObserver() )
		{
			State_Transition( STATE_OBSERVER_MODE );
			ObserverTransistion(); // Do this to generate valid spectator view settings.
		}
	}
	else if ( !GERules()->FPlayerCanRespawn(this) )
	{
		if ( !IsObserver() )
		{
			State_Transition( STATE_OBSERVER_MODE );
			ObserverTransistion();
		}
		// Gameplay said NO SPAWN FOR YOU! Try again every second.
		SetSpawnState( SS_BLOCKED_GAMEPLAY );
		m_flNextSpawnTry = gpGlobals->curtime + 1.0f;

		// Prevent head jerk when we respawn
		m_bSpawnFromDeath = false;
	}
	else if ( IsSpawnState( SS_BLOCKED_NO_SPOT ) && gpGlobals->curtime < m_flNextSpawnTry )
	{
		// Not time to spawn yet!
		return;
	}
	else
	{
		// We are going to attempt a spawn
		StopObserverMode();
	}

	if ( !IsObserver() )
	{
		// Early out if we can't find a valid spawn point for us so that we can try again in about 1 second
		if ( g_pGameRules->GetPlayerSpawnSpot( this ) == NULL )
		{
			if ( !IsSpawnState( SS_BLOCKED_NO_SPOT) )
			{
				// First time waiting, issue a message and find a spectator spawn spot
				GERules()->GetSpectatorSpawnSpot( this );
				m_flNextSpawnWaitNotice = gpGlobals->curtime;
			}

			if ( gpGlobals->curtime >= m_flNextSpawnWaitNotice )
			{
				HintMessage( "#GE_Hint_SpawnWait", 3.5f );
				m_flNextSpawnWaitNotice = gpGlobals->curtime + 5.0f;
			}

			// Reset our observer variables
			QuickStartObserverMode();

			SetSpawnState( SS_BLOCKED_NO_SPOT );
			m_flNextSpawnTry = gpGlobals->curtime + 1.0f;
			return;
		}

		State_Transition( STATE_ACTIVE );
		SetSpawnState( SS_ACTIVE );

		BaseClass::Spawn();

		// See if we were caught not actually spawning
		if ( !IsSpawnState( SS_ACTIVE ) )
		{
			State_Transition( STATE_OBSERVER_MODE );
			GERules()->GetSpectatorSpawnSpot( this );
			return;
		}

		int noSkins = atoi(engine->GetClientConVarValue(entindex(), "cl_ge_nowepskins"));

		// Create a dummy skin list, fill it out, and then transmit anything that changed.
		int iDummySkinList[WEAPON_RANDOM] = { 0 };

		// First decypher their skins code, if they have one.
		if (m_iClientSkinsCode && !noSkins)
		{
			DevMsg( "Parsing client skin code of %lld\n", m_iClientSkinsCode );

			for (int i = 0; i < WEAPON_RANDOM; i++)
			{
				iDummySkinList[i] = (m_iClientSkinsCode >> (i * 2)) & 3;
			}
		}

		// Then look at the server authenticated skin code, this overwrites the client skins.
		if (m_iSkinsCode && noSkins <= 1)
		{
			DevMsg( "Parsing server skin code of %lld\n", m_iSkinsCode );
			uint64 shiftvalue = 0;

			for ( int i = 0; i < WEAPON_RANDOM; i++ )
			{
				shiftvalue = (m_iSkinsCode >> (i * 2)) & 3;
				if (shiftvalue != 0)
				{
					iDummySkinList[i] = shiftvalue;
				}
			}
		}

		if (noSkins <= 2)
		{
			if (GetDevStatus() == GE_DEVELOPER)
				iDummySkinList[WEAPON_KLOBB] = 3;
			else if (GetDevStatus() == GE_BETATESTER)
				iDummySkinList[WEAPON_KLOBB] = 2;
			else if (GetDevStatus() == GE_CONTRIBUTOR)
				iDummySkinList[WEAPON_KLOBB] = 1;

			// If it's luchador, give him the watermelon KF7.  Love u 4evr luch.
			if (m_iSteamIDHash == 3135237817u)
				iDummySkinList[WEAPON_KF7] = 1;
		}

		for (int i = 0; i < WEAPON_RANDOM; i++)
		{
			if (iDummySkinList[i] != m_iWeaponSkinInUse.Get(i))
				m_iWeaponSkinInUse.Set(i, iDummySkinList[i]);
		}

		RemoveAllItems(true);
		GiveDefaultItems();
		GiveHat();

		// Create our second view model for dualies
		// CreateViewModel( GE_LEFT_HAND );

		SetHealth( GetMaxHealth() );
		
		AddFlag(FL_ONGROUND); // set the player on the ground at the start of the round.

		m_impactEnergyScale = GEPLAYER_PHYSDAMAGE_SCALE;

		if ( GEMPRules()->IsIntermission() )
			AddFlag( FL_FROZEN );
		else
			RemoveFlag( FL_FROZEN );

		// Add spawn time for Awards calculations
		m_flSpawnTime = gpGlobals->curtime;

		// Spawn system variables
		m_vLastSpawn = GetAbsOrigin();

		// Reset invuln variables
		m_pLastAttacker = NULL;
		m_iViewPunchScale = 0;

		// Camping calcs
		m_vLastOrigin = GetAbsOrigin();
		m_iCampingState = 0;
		m_flCampingTime = 0.0f;
		m_flLastMoveTime = gpGlobals->curtime;
		m_flNextCampCheck = gpGlobals->curtime;

		// Give us the invuln time defined by the gameplay, but only if it's not our first spawn since we don't want everyone invisible on the radar at the start of the round.

		if (GetRoundDeaths() > 0)
			StartInvul(GEMPRules()->GetSpawnInvulnInterval());
		else
			StopInvul();

		SetMaxSpeed( GE_NORM_SPEED * GEMPRules()->GetSpeedMultiplier(this) );
		m_Local.m_bDucked = false;
		SetPlayerUnderwater(false);
		m_flStartJumpZ = 0.0f;
		m_flNextJumpTime = 1.0f;
		m_flLastJumpTime = 0.0f;
		m_flJumpPenalty = 0.0f;
		m_flLastLandVelocity = 0.0f;

		m_flRunTime = 0;
		m_flRunCode = 0;

		pl.deadflag = false;
		RemoveSolidFlags( FSOLID_NOT_SOLID );
		RemoveEffects( EF_NODRAW | EF_NOINTERP );
		m_nRenderFX = kRenderNormal;
		m_Local.m_iHideHUD = 0;

		// Call into python to let the gameplay know we actually spawned
		GEGameplay()->GetScenario()->OnPlayerSpawn( this );

		// Touch any triggers we may spawn into
		PhysicsTouchTriggers();

		if ( m_bPreSpawn )
		{
			m_bPreSpawn = false;

			// Hide any latent round reports
			ShowViewPortPanel( PANEL_ROUNDREPORT, false );

			// Show our team notice
			if ( GetTeamNumber() >= FIRST_GAME_TEAM )
			{
				char msg[128];
				Q_snprintf( msg, 128, "#GES_GPH_JOINTEAM\r%s", GetTeam()->GetName() );
				ShowTeamNotice( "#GES_GPH_JOINTEAM_TITLE", msg, GetTeamNumber() == TEAM_MI6 ? "team_mi6" : "team_janus" );
			}

			// Give us hints
			HintMessage( "#GE_Hint_PopupHelp", 10.0f );
			HintMessage( "#GE_Hint_GameplayHelp", 10.0f );
			HintMessage( "#GE_Hint_WeaponSet", 10.0f );
		}
		else if ( !IsBotPlayer() )
		{
			// Clear our VGUI queue to prevent vguis from stacking if we are AFK
		//	ShowViewPortPanel( PANEL_CLEARQUEUE );

			int punchvar = atoi( engine->GetClientConVarValue( entindex(), "cl_ge_disablebloodscreen" ) );
			if ( punchvar == 0 && m_bSpawnFromDeath )
			{
				int yawdir = 1;
				if ( GERandom<int>(2) ) // [0,1]
					yawdir = -1;

				m_Local.m_vecPunchAngle.Set( PITCH, 55.0f );
				m_Local.m_vecPunchAngle.Set( YAW, yawdir * 30.0f );
				m_Local.m_vecPunchAngle.Set( ROLL, yawdir * -20.0f );
			}
		}

		// Reset our spawn flags
		m_bFirstSpawn = false;
		m_bSpawnFromDeath = false;
		m_flNextSpawnTry = 0.0f;
	}
	else
	{
		// We are an observer, so just try to set them up for success
		KnockOffHat( true );
		InitFogController();
	}

	if (GERules()->IsTeamplay())	
	{		
		if (GetTeamNumber() == TEAM_MI6)		
		{			
			SetCollisionGroup( COLLISION_GROUP_MI6 );		
		}		
		else if (GetTeamNumber() == TEAM_JANUS)		
		{			
			SetCollisionGroup(COLLISION_GROUP_JANUS);		
		}	
	}	
	else	
	{		
		SetCollisionGroup( COLLISION_GROUP_PLAYER );			
	}
}

void CGEMPPlayer::SetSpawnState( SpawnState state )
{
#ifdef DEBUG
	if ( m_iSpawnState != state )
	{
		char *szOldState = "INVALID", *szNewState = "INVALID";

		switch( m_iSpawnState )
		{
		case SS_INITIAL:
			szOldState = "Initial";
			break;
		case SS_ACTIVE:
			szOldState = "Active";
			break;
		case SS_BLOCKED_GAMEPLAY:
			szOldState = "Blocked by Gameplay";
			break;
		case SS_BLOCKED_NO_SPOT:
			szOldState = "Blocked No Spot";
			break;
		}

		switch( state )
		{
		case SS_INITIAL:
			szNewState = "Initial";
			break;
		case SS_ACTIVE:
			szNewState = "Active";
			break;
		case SS_BLOCKED_GAMEPLAY:
			szNewState = "Blocked by Gameplay";
			break;
		case SS_BLOCKED_NO_SPOT:
			szNewState = "Blocked No Spot";
			break;
		}
		Msg( "[SPAWN] %s: %s State => %s State\n", GetCleanPlayerName(), szOldState, szNewState );
	}
#endif

	m_iSpawnState = state;
}

void CGEMPPlayer::ForceRespawn()
{
	// If we aren't allowed to respawn yet skip the theatrics of the baseclass
	if ( !GERules()->FPlayerCanRespawn(this) || (!IsSpawnState( SS_ACTIVE ) && m_bPreSpawn) )
	{
		Spawn();
		return;
	}

	SetSpawnState( SS_ACTIVE );
	BaseClass::ForceRespawn();
}

void CGEMPPlayer::SetPlayerModel( const char* szCharName, int iCharSkin /*=0*/, bool bNoSelOverride /*=false*/ )
{
	// Copy the pointer over so we can deal with randomness
	char selCharName[MAX_CHAR_IDENT];
	Q_strncpy( selCharName, szCharName, MAX_CHAR_IDENT );

	// Deal with random characters
	if ( !Q_stricmp( selCharName, CHAR_RANDOM_IDENT ) )
	{
		CUtlVector<const CGECharData*> vChars;
		GECharacters()->GetTeamMembers( GetTeamNumber(), vChars );

		// Cannot choose a character when none exist..
		if ( vChars.Count() == 0 )
			return;

		int sel = -1;
		do {
			// Remove failed choices
			if ( sel >= 0 )
				vChars.Remove(sel);
			// Choose a random character from the list, we need to obey our gameplay though!
			sel = GERandom<int>( vChars.Count() );
		} while ( !GEGameplay()->GetScenario()->CanPlayerChangeChar(this, vChars[sel]->szIdentifier) );

		// Copy the final selection
		Q_strncpy( selCharName, vChars[sel]->szIdentifier, MAX_CHAR_IDENT );
	}

	// Do some basic checks for character adherance (if not forced)
	if ( !bNoSelOverride )
	{
		if ( !GEGameplay()->GetScenario()->CanPlayerChangeChar(this, selCharName) )
		{
			// Gameplay denied their selection, show the selection panel again
			ShowViewPortPanel( PANEL_CHARSELECT );
			return;
		}
		else if ( gpGlobals->curtime < m_flNextModelChangeTime )
		{
			// Don't allow switching characters too fast
			UTIL_SayText("You must wait a few seconds to change your character again...\n", this);
			return;
		}
	}

	const CGECharData *pChar = GECharacters()->Get( selCharName );
	if( !pChar )
		return;

	// Make sure we aren't trying to join a class that is not a part
	// of the player's active team
	if ( GERules()->IsTeamplay() && pChar->iTeam != GetTeamNumber() ) 
		return;

	BaseClass::SetPlayerModel( selCharName, iCharSkin );

	UTIL_ClientPrintAll( HUD_PRINTTALK, "#PLAYER_CHANGECHAR", GetPlayerName(), pChar->szShortName );

	m_flNextModelChangeTime = gpGlobals->curtime + MODEL_CHANGE_INTERVAL;

	// Spawn us into the game if this is our first time
	if ( m_bPreSpawn )
	{
		SetSpawnState( SS_ACTIVE );
		ForceRespawn();
	}
}

// This function is called when we switch teams so that we always have a valid character from
// the team we joined
void CGEMPPlayer::SetPlayerTeamModel()
{
	int i = GetCharIndex();
	const CGECharData *pChar = GECharacters()->Get(i);

	// Check if we already are a character on the team we have been assigned to
	if ( pChar && pChar->iTeam == GetTeamNumber() )
		return;

	pChar = GECharacters()->GetFirst( GetTeamNumber() );
	if ( pChar )
	{
		// We call the baseclass so we don't enact time limiters and checks
		BaseClass::SetPlayerModel( pChar->szIdentifier, 0 );
		return;
	}
	
	// We shouldn't get here, but if we do then just go with the first player
	BaseClass::SetPlayerModel();
}

void CGEMPPlayer::ChangeTeam( int iTeam, bool bWasForced /* = false */ )
{
	// If we haven't waited our change time tell them to slow down
	if ( gpGlobals->curtime < m_flNextTeamChangeTime && !bWasForced ) {
		UTIL_SayText("You must wait a few seconds to change your team again...\n", this);
		return;
	}

	bool bKill = false;

	if ( !GERules()->IsTeamplay() && iTeam != TEAM_SPECTATOR )
		// Don't let players join a team when teamplay is disabled
		iTeam = TEAM_UNASSIGNED;

	if ( GERules()->IsTeamplay() )
	{
		// Only suicide the player if they are switching teams and not observing and not forced to switch
		if (GetTeamNumber() != TEAM_UNASSIGNED && !IsObserver() && !bWasForced)
			bKill = true;

		// The player is trying to Auto Join a team so set them up!
		if ( iTeam == MAX_GE_TEAMS )
		{
			CTeam *pJanus = g_Teams[TEAM_JANUS];
			CTeam *pMI6 = g_Teams[TEAM_MI6];
			if ( pJanus->GetNumPlayers() > pMI6->GetNumPlayers() )
				iTeam = TEAM_MI6;
			else if ( pJanus->GetNumPlayers() < pMI6->GetNumPlayers() )
				iTeam = TEAM_JANUS;
			else
				iTeam = random->RandomInt( TEAM_MI6, TEAM_JANUS );
		}
	}
	else if ( GetTeamNumber() == TEAM_UNASSIGNED && !m_bPreSpawn && IsAlive() && !bWasForced ) //Use alternate rules for non-teamplay.
		bKill = true;
	
	// Coming on spectator we end pre spawn, but going from spectator
	// to another team we initiate pre spawn
	if ( iTeam == TEAM_SPECTATOR )
		m_bPreSpawn = false;
	else if ( GetTeamNumber() == TEAM_SPECTATOR )
		m_bPreSpawn = true;

	// Don't worry about this switch if they aren't actually switching teams
	if ( iTeam == GetTeamNumber() )
		return;

	// Let the gameplay have a say (Warnings left up to GamePlay)
	if ( !GEGameplay()->GetScenario()->CanPlayerChangeTeam( this, GetTeamNumber(), iTeam, bWasForced ))
		return;

	// If we didn't just join the server, we're alive, and this was by choice, force a suicide.
	if ( iTeam == TEAM_SPECTATOR && bKill )
		CommitSuicide(false, true);

	BaseClass::ChangeTeam( iTeam );

	m_flNextTeamChangeTime = gpGlobals->curtime + TEAM_CHANGE_INTERVAL;
	m_flNextModelChangeTime = gpGlobals->curtime - 1.0f;

	if ( GEMPRules()->IsTeamplay() == true )
		SetPlayerTeamModel();
	else
		BaseClass::SetPlayerModel();

	// We are changing teams, drop any tokens we might have
	DropAllTokens(); // Mostly just a failsafe against force switches at this point since we usually force suicide.

	if ( iTeam == TEAM_SPECTATOR )
	{
		RemoveAllItems( true );
		ResetObserverMode();
		State_Transition( STATE_OBSERVER_MODE );
	}
	else if ( !GEMPRules()->IsIntermission() && !m_bPreSpawn )
	{
		// Let the player know what team they joined
		if ( GEMPRules()->IsTeamplay() )
		{
			char msg[128];
			const char *teaming = (iTeam == TEAM_MI6 ? "team_mi6" : "team_janus");
			if ( bWasForced )
			{
				Q_snprintf( msg, 128, "#GES_GPH_AUTOTEAM\r%s", GetTeam()->GetName() );
				ShowTeamNotice( "#GES_GPH_AUTOTEAM_TITLE", msg, teaming );
				m_iAutoTeamSwaps++;
			}
			else
			{
				Q_snprintf( msg, 128, "#GES_GPH_JOINTEAM\r%s", GetTeam()->GetName() );
				ShowTeamNotice( "#GES_GPH_JOINTEAM_TITLE", msg, teaming );
				m_iAutoTeamSwaps = 0;
				m_flTeamJoinTime = gpGlobals->curtime;
			}
		}

		// Give us a chance to select our character
		if ( !IsBot() )
			SetPreSpawn();

		// Kill us if we were told to by above criteria (respawn will occur later)
		// otherwise, force a respawn which won't make us go through the death sequence
		// and cost a score point
		if ( bKill )
			CommitSuicide();
		else
			ForceRespawn();
	}
}

void CGEMPPlayer::ShowTeamNotice( const char *title, const char *msg, const char *img )
{
	int nMsg = g_pStringTableGameplay->FindStringIndex( msg );
	if ( nMsg == INVALID_STRING_INDEX )
	{
		bool save = engine->LockNetworkStringTables( false );
		nMsg = g_pStringTableGameplay->AddString( true, msg );
		engine->LockNetworkStringTables( save );
	}

	CSingleUserRecipientFilter filter( this );
	filter.MakeReliable();

	UserMessageBegin( filter, "PopupMessage" );
		WRITE_STRING( title );
		WRITE_STRING( img );
		WRITE_SHORT( nMsg );
	MessageEnd();
}

CBaseEntity* CGEMPPlayer::EntSelectSpawnPoint()
{
	// This call ensures we always default to info_player_deathmatch if none of the other spawn types exist
	int iSpawnerType = GEMPRules()->GetSpawnPointType( this );
	CGEPlayerSpawn *pSpot = NULL;

	if (iSpawnerType == SPAWN_PLAYER_SPECTATOR || IsObserver())
	{
		if (GEMPRules()->GetSpawnersOfType(SPAWN_PLAYER_SPECTATOR)->Count()) // Find us the next spectator spawn, skip the theatrics below
		{
			// Find one that isn't disabled, unless all of them are in which case use the first.
			for (int i = 0; i <= GEMPRules()->GetSpawnersOfType(SPAWN_PLAYER_SPECTATOR)->Count(); i++)
			{
				pSpot = (CGEPlayerSpawn*)EntFindSpawnPoint(m_iLastSpecSpawn, iSpawnerType, m_iLastSpecSpawn);

				if (pSpot && pSpot->IsEnabled())
					break;
			}
		}
		else  // Didn't find one but we don't want to see the inside of dr.dean's head anymore so let's just grab the first deathmatch one.
		{
			DevMsg("Looking for DM Spawn!\n");
			pSpot = (CGEPlayerSpawn*)GERules()->GetSpawnersOfType(SPAWN_PLAYER)->Element(0).Get();
		}

		if (pSpot)
		{
			DevMsg("Returning spectator spawn point!\n");
			return pSpot;
		}
	}

	if ( ge_debug_playerspawns.GetBool() )
		Msg( "Started Spawn Point Selection!\n" );

	const CUtlVector<EHANDLE> *vSpots = GERules()->GetSpawnersOfType( iSpawnerType );
	CUtlVector<CGEPlayerSpawn*> vSpawners;
	CUtlVector<CGEPlayerSpawn*> vLeftovers;
	CUtlVector<float> vWeights;

	// Figure out the spawn with the highest weight

	int bestweight = 2;

	for (int i = 0; i < vSpots->Count(); i++)
	{
		CGEPlayerSpawn *pSpawn = (CGEPlayerSpawn*)vSpots->Element(i).Get();
		if (GERules()->IsSpawnPointValid(pSpawn, this))
		{
			int weight = pSpawn->GetDesirability(this);
			if (weight > bestweight)
				bestweight = weight;
		}
	}

	bestweight *= 0.5; //establish a threshold for spawns that are within a certain value of bestweight.

	// Build a list of player spawns
	for ( int i=0; i < vSpots->Count(); i++ )
	{
		CGEPlayerSpawn *pSpawn = (CGEPlayerSpawn*) vSpots->Element(i).Get();
		if ( GERules()->IsSpawnPointValid( pSpawn, this ) )
		{
			int weight = pSpawn->GetDesirability( this );
			if ( weight > bestweight )
			{
				//Shifts the weights down into the 1/8-1/2 range from the 1/2 - 1 range. 
				//Spawns at the bottom are 1 / 4th as likely to be picked as spawns at the top.
				weight -= bestweight * 0.75;

				vSpawners.AddToTail( pSpawn );
				vWeights.AddToTail( weight );

				if ( ge_debug_playerspawns.GetBool() )
					Msg( "%i: Found spawn point %s with desirability %i\n", vSpawners.Count(), pSpawn->GetClassname(), weight );
			}
			else
			{
				vLeftovers.AddToTail( pSpawn );
			}
		}
	}

	// If we didn't find any spots bail out early
	if ( vSpawners.Count() == 0 )
	{
		if ( vLeftovers.Count() > 0 )
		{
			int x = GERandom<int>( vLeftovers.Count() );
			pSpot = vLeftovers[x];
		}
		else
		{
			if ( ge_debug_playerspawns.GetBool() )
				Msg( "No valid spawn points were found :(\n" );

			SetSpawnState( SS_BLOCKED_NO_SPOT );
			m_flNextSpawnTry = gpGlobals->curtime + 1.0f;
			return NULL;
		}
	}
	else
	{
		// Use our desirability to pull a random weighted spawn point out
		pSpot = GERandomWeighted<CGEPlayerSpawn*, float>( vSpawners.Base(), vWeights.Base(), vSpawners.Count() );

		// Drop dead fix for when random weighted returns null (this should never happen!)
		Assert( pSpot );
		if ( !pSpot )
			pSpot = vSpawners[ GERandom<int>(vSpawners.Count()) ];

		if ( ge_debug_playerspawns.GetBool() )
			Msg( "Selected spawn point %i!\n", vSpawners.Find( pSpot ) );
	}

	pSpot->NotifyOnUse();
	return pSpot;
}

CBaseEntity *CGEMPPlayer::EntFindSpawnPoint( int iStart, int iType, int &iReturn, bool bAllowWrap /*= true*/, bool bRandomize /*= false*/ )
{
	const CUtlVector<EHANDLE> *vSpawns = GEMPRules()->GetSpawnersOfType( iType );
	if ( !vSpawns || vSpawns->Count() == 0 )
		return NULL;

	if ( iStart < -1 || iStart >= vSpawns->Count() )
		iStart = -1;

	int iRand = bRandomize ? (GERandom<int>(5) + 1) : 1;
	int iNext = iStart + iRand;

	if (iNext >= vSpawns->Count())
	{
		if ( bAllowWrap )
			iNext = MAX(0, iNext % vSpawns->Count());
		else
			iNext = -1;
	}
	iReturn = iNext;
	return iNext >= 0 ? (*vSpawns)[iNext].Get() : NULL;
}

bool CGEMPPlayer::CheckForValidSpawns( int iSpawnerType /*= -1*/ )
{
	if ( iSpawnerType == -1 )
		iSpawnerType = GEMPRules()->GetSpawnPointType( this );

	if ( iSpawnerType == SPAWN_PLAYER_SPECTATOR )
		return true;

	// Troll from the beginning of the list and go through all the spawns
	// to try and find the first valid one
	const CUtlVector<EHANDLE> *vSpots = GERules()->GetSpawnersOfType( iSpawnerType );
	
	// Convert handles to player spawns
	for ( int i=0; i < vSpots->Count(); i++ )
	{
		if ( GERules()->IsSpawnPointValid( vSpots->Element(i), this ) )
			return true;
	}

	return false;
}

CBaseEntity *CGEMPPlayer::GiveNamedItem( const char *classname, int iSubType /*=0*/ )
{
	// Don't give out tokens
	if ( GEMPRules()->GetTokenManager()->IsValidToken( classname ) )
		return NULL;

	CBaseEntity *pEnt = BaseClass::GiveNamedItem( classname, iSubType );
	if ( pEnt && !pEnt->GetOwnerEntity() )
	{
		// If we didn't actually take the item, remove it
		pEnt->Remove();
		return NULL;
	}

	return pEnt;
}

extern ConVar ge_startarmed;
// Start armed settings:
//  0 = Only Slappers
//  1 = Give a knife
//  2 = Give first weapon in loadout
void CGEMPPlayer::GiveDefaultItems()
{
	EquipSuit(false);

	CBaseEntity *pGivenWeapon;
	// Always give slappers
	pGivenWeapon = GiveNamedItem( "weapon_slappers" );

	const CGELoadout *loadout = GEMPRules()->GetLoadoutManager()->CurrLoadout();
	if ( loadout )
	{
		// Give the weakest weapon in our set if more conditions are met
		if ( ge_startarmed.GetInt() >= 1 )
		{
			int wID = loadout->GetFirstWeapon();
			int aID = GetAmmoDef()->Index( GetAmmoForWeapon(wID) );

			// Give the player a slice of ammo for the lowest ranked weapon in the set
			if (wID)
			{
				CBasePlayer::GiveAmmo(GetAmmoDef()->CrateAmount(aID), aID);
				pGivenWeapon = GiveNamedItem(WeaponIDToAlias(wID));
			}
		}
	} 

	// Switch to the best given weapon
	if (pGivenWeapon)
		Weapon_Switch((CBaseCombatWeapon*)pGivenWeapon);
}

void CGEMPPlayer::SetPlayerName( const char *name )
{
	Q_strncpy( m_szCleanName, name, 32 );
	GEUTIL_RemoveColorHints( m_szCleanName );

	if ( Q_strlen(m_szCleanName) == 0 )
	{
		// Make sure we don't have just color codes in our name
		char newname[32];
		Q_strncpy( newname, "(NO NAME)", 32 );
		Q_strncpy( m_szCleanName, "(NO NAME)", 32 );
		engine->ClientCommand( edict(), "setinfo name \"%s\"", newname );
		BaseClass::SetPlayerName( newname );
	}
	else
	{
		BaseClass::SetPlayerName( name );
	}

	// Store python safe versions of our name.
	GEUTIL_CleanupNameEnding(m_szCleanName, m_szSafeName);
	GEUTIL_CleanupNameEnding(name, m_szSafeCleanName);
}

bool CGEMPPlayer::ClientCommand( const CCommand &args )
{
	const char* cmd = args[0];

	if ( Q_stricmp( cmd, "joinclass" ) == 0 )
	{
		if ( args.ArgC() < 2 )
			Warning( "Player sent bad joinclass syntax\n" );

		if ( ShouldRunRateLimitedCommand(args) )
		{
			// Set the skin if specified
			if ( args.ArgC() == 3 )
				SetPlayerModel( args[1], atoi(args[2]) );
			else
				SetPlayerModel( args[1] );
		}

		return true;
	}
	else if ( Q_stricmp( cmd, "achievement_progress" ) == 0 )
	{
		if ( args.ArgC() < 3 )
			Warning( "Player sent bad achievement progress\n" );

		// let's check this came from the client .dll and not the console
		unsigned short mask = UTIL_GetAchievementEventMask();

		int iPercent = atoi( args[1] ) ^ mask;
		int code = ( GetUserID() ^ iPercent ) ^ mask;

		if ( code == atoi( args[2] ) )
		{
			char *ach = Q_strrchr( args.ArgS(), '|' );
			if ( ach )
			{
				// We use the trailing end of the command which contains
				// the achievements we earned in a bitwise fashion encoded
				// in unsigned char ;-)
				ach++;

				char tmp[8];
				int size = Q_strlen(ach);
				m_iAchCount = 0;
				tmp[0] = m_szAchList[0] = '\0';

				int r = 0, k=1;
				for ( int i=FIRST_GES_ACH; r < size && i < MAX_GES_ACH; r++ )
				{
					// We start at k=1 since the LSB is always 1 to prevent null termination
					for ( k=1; k < 8 && i < MAX_GES_ACH; k++, i++ )
					{
						if ( ach[r] & (0x1 << k) )
						{
							if ( m_iAchCount )
								Q_snprintf( tmp, 8, " %d", i );
							else
								Q_snprintf( tmp, 8, "%d", i );

							Q_strncat( m_szAchList, tmp, 1028 );
							m_iAchCount++;
						}
					}
				}

				m_bAchReportPending = true;
			}

			// We only show achievement progress for non-developers
			if ( m_iDevStatus == 0 )
			{
				if ( iPercent >= 95 )
					m_iDevStatus = GE_GOLDACH;
				else if ( iPercent >= 85 )
					m_iDevStatus = GE_SILVERACH;
			}
		}

		return true;
	}
	else if (Q_stricmp(cmd, "skin_unlock_code") == 0)
	{
		if (args.ArgC() < 1)
			Warning("Player sent bad unlock code!\n");

		DevMsg("Got skin unlock code of %s\n", args[1]);

		uint64 unlockcode = atoll( args[1] );
		uint64 sendbits = 0;

		// We now want to check with the server and make sure that the client is allowed to confirm these skins.  Let's face it any encryption method we use is going to get broken.
		// If someone wants to put in the effort I have no objection to them giving themselves the promotional skins, but we have to protect the rare ones so they're actually worth something.
		// Not a perfect filter, but should suit our purposes since if a client authed skin is level 3 there are probably level 1 and 2 skins as well.
		sendbits = unlockcode & iAllowedClientSkins;

		DevMsg("Filtered it to be %llu\n", sendbits);

		if (sendbits != unlockcode)
			Warning("%s requested skins they aren't allowed to have using code %llu!\n", GetPlayerName(), unlockcode);

		m_iClientSkinsCode = sendbits;

		return true;
	}
	else if ( Q_stricmp( cmd, "spec_mode" ) == 0 ) // new observer mode
	{
		int mode;

		// check for parameters.
		if ( args.ArgC() >= 2 )
		{
			mode = atoi( args[1] );

			if ( mode < OBS_MODE_FIXED || mode > LAST_PLAYER_OBSERVERMODE )
				mode = OBS_MODE_FIXED;
		}
		else
		{
			// switch to next spec mode if no parameter given
			mode = GetObserverMode() + 1;
			
			if ( mode > LAST_PLAYER_OBSERVERMODE )
			{
				mode = OBS_MODE_FIXED;
			}
			else if ( mode < OBS_MODE_FIXED )
			{
				mode = OBS_MODE_ROAMING;
			}

		}
	
		// don't allow input while player is in death cam
		if ( GetObserverMode() > OBS_MODE_DEATHCAM )
		{
			// set new spectator mode, don't allow OBS_MODE_NONE
			if ( !SetObserverMode( mode ) )
				ClientPrint( this, HUD_PRINTCONSOLE, "#Spectator_Mode_Unkown");
			else
				engine->ClientCommand( edict(), "cl_spec_mode %d", mode );

			if ( mode == OBS_MODE_FIXED )
			{
				// Find us a place to reside
				m_hObserverTarget.Set( 0 );
				SetViewOffset(vec3_origin);
				ClearFlags();
				GERules()->GetSpectatorSpawnSpot( this );
			}
		}
		else
		{
			// remember spectator mode for later use
			m_iObserverLastMode = mode;
			engine->ClientCommand( edict(), "cl_spec_mode %d", mode );
		}

		return true;
	}
	else if ( Q_stricmp( cmd, "spec_next" ) == 0 ) // chase next player
	{
		if ( GetObserverMode() > OBS_MODE_FIXED )
		{
			// set new spectator mode
			CBaseEntity * target = FindNextObserverTarget( false );
			if ( target )
			{
				SetObserverTarget( target );
			}
		}
		else if ( GetObserverMode() == OBS_MODE_FIXED )
		{
			// Find us a place to reside
			m_hObserverTarget.Set(0);
			SetViewOffset(vec3_origin);
			ClearFlags();
			GERules()->GetSpectatorSpawnSpot( this );
		}
		
		return true;
	}
	else if ( Q_stricmp( cmd, "spec_prev" ) == 0 ) // chase prevoius player
	{
		if ( GetObserverMode() > OBS_MODE_FIXED )
		{
			// set new spectator mode
			CBaseEntity * target = FindNextObserverTarget( true );
			if ( target )
			{
				SetObserverTarget( target );
			}
		}
		else if ( GetObserverMode() == OBS_MODE_FIXED )
		{
			// Find us a place to reside
			m_hObserverTarget.Set(0);
			SetViewOffset(vec3_origin);
			ClearFlags();
			GERules()->GetSpectatorSpawnSpot( this );
		}
		
		return true;
	}

	return BaseClass::ClientCommand( args );
}


void CGEMPPlayer::PreThink()
{
	// Keep checking this until we get a valid Steam ID return
	if ( m_iSteamIDHash == 0  && GERules()->HaveStatusListsUpdated() && !(GetFlags() & FL_FAKECLIENT) )
	{
		// Get this player's information and hash their steam id for later use
		// this is sent to the client as well
		const char *steamID = GetNetworkIDString();
		if ( !Q_strnicmp( steamID, "STEAM_", 6 ) )
		{
			if ( !Q_stristr(steamID, "PENDING") )
			{
				m_iSteamIDHash = RSHash( steamID+8 );
				if ( IsOnList( LIST_DEVELOPERS, m_iSteamIDHash ) )
				{
					DevMsg( "DEV STATUS: %s (%s)\n", GetPlayerName(), steamID );
					m_iDevStatus = GE_DEVELOPER;
				}
				else if ( IsOnList( LIST_TESTERS, m_iSteamIDHash ) )
				{
					DevMsg( "BT STATUS: %s (%s)\n", GetPlayerName(), steamID );
					m_iDevStatus = GE_BETATESTER;
				}
				else if (IsOnList(LIST_CONTRIBUTORS, m_iSteamIDHash))
				{
					DevMsg("CC STATUS: %s (%s)\n", GetPlayerName(), steamID);
					m_iDevStatus = GE_CONTRIBUTOR;
				}
				else if ( IsOnList( LIST_BANNED, m_iSteamIDHash) )
				{
					char command[255];
					Q_snprintf(command, 255, "kick %d\n", GetUserID());
					Msg("%s is GE:S Auth Server Banned [%s]\n", steamID, command);
					engine->ServerCommand(command);
				}

				if (IsOnList(LIST_SKINS, m_iSteamIDHash))
				{
					DevMsg("OWNS AUTHED SKIN: %s (%s)\n", GetPlayerName(), steamID);

					int skinIndex = vSkinsHash.Find(m_iSteamIDHash);
					m_iSkinsCode = vSkinsValues[skinIndex];
				}
			}
		}
	}
	else if ( m_iSteamIDHash != 0 && m_bAchReportPending )
	{
		m_bAchReportPending = false;

		// Send this information off to any plugins listening (NOT TO CLIENTS)
		IGameEvent *event = gameeventmanager->CreateEvent( "achievement_progressreport" );
		if( event )
		{
			event->SetInt( "userid", GetUserID() );
			event->SetInt( "count", m_iAchCount );
			event->SetString( "achieved", m_szAchList );
			gameeventmanager->FireEvent( event, true );
		}

		if (atoi(engine->GetClientConVarValue(entindex(), "cl_ge_hidetags")))
			m_iDevStatus = 0; // Maybe we don't want anyone to know we have all the acheivements.
	}

	// If we are waiting to spawn and have passed our wait time, try again!
	if ( (IsSpawnState( SS_BLOCKED_NO_SPOT ) || IsSpawnState( SS_BLOCKED_GAMEPLAY ) ) && gpGlobals->curtime >= m_flNextSpawnTry )
		Spawn();

	// Check our ownership of thrown objects and update the vector if necessary
	for ( int i=0; i < m_hThrownObjects.Count(); i++ )
	{
		if ( !m_hThrownObjects[i].Get() )
			m_hThrownObjects.Remove(i);
	}

	// Display our hint messages if they are queded
	if ( Hints() )
	{
		Hints()->Update();
	}

	//UpdateJumpPenalty();

	BaseClass::PreThink();
}

void CGEMPPlayer::PostThink()
{
	BaseClass::PostThink();
	
	// Check our camping status
	UpdateCampingTime();
}

void CGEMPPlayer::Event_Killed( const CTakeDamageInfo &info )
{
	m_bSpawnFromDeath = true;
	m_vLastDeath = GetAbsOrigin();

	NotifyOnDeath();

	BaseClass::Event_Killed( info );

	// Call into our anim handler and setup our death animations
//	DoAnimationEvent( PLAYERANIMEVENT_DIE, LastHitGroup() );

	// Look ahead, set this early so gameplays can use it right away
	if ( !GERules()->FPlayerCanRespawn(this) )
	{
		SetSpawnState( SS_BLOCKED_GAMEPLAY );
		// Arbitrary wait time, if they try to spawn earlier it will still catch them
		m_flNextSpawnTry = gpGlobals->curtime + 10.0f;
	}
}

void CGEMPPlayer::NotifyOnDeath()
{
	// Notify nearby spawn points of our death (DM Spawns Only)
	const CUtlVector<EHANDLE> *vSpawns = GERules()->GetSpawnersOfType( SPAWN_PLAYER );
	Vector myPos = GetAbsOrigin();
	for ( int i=(vSpawns->Count()-1); i >= 0; i-- )
	{
		CGEPlayerSpawn *pSpawn = (CGEPlayerSpawn*) vSpawns->Element(i).Get();
		float dist = UTIL_DistApprox( myPos, pSpawn->GetAbsOrigin() );
		pSpawn->NotifyOnDeath( dist );
	}
}

bool CGEMPPlayer::HandleCommand_JoinTeam( int team )
{
	if ( !GetGlobalTeam( team ) && team != MAX_GE_TEAMS )
	{
		Warning( "HandleCommand_JoinTeam( %d ) - invalid team index.\n", team );
		return false;
	}

	if ( team == TEAM_SPECTATOR )
	{
		// Prevent this is the cvar is set
		if ( !mp_allowspectators.GetInt() )
		{
			ClientPrint( this, HUD_PRINTTALK, "#Cannot_Be_Spectator" );
			return false;
		}

		ChangeTeam( TEAM_SPECTATOR );
		return true;
	}

	// Switch their actual team...
	ChangeTeam( team );

	return true;
}

void CGEMPPlayer::ObserverTransistion()
{
	char specmode[16];
	if (GEMPRules()->GetSpawnersOfType(SPAWN_PLAYER_SPECTATOR)->Count() || GEMPRules()->GetNumActivePlayers() < 1)
	{
		Q_snprintf(specmode, 16, "spec_mode %i", OBS_MODE_FIXED);
		engine->ClientCommand(edict(), specmode);
		SetObserverTarget(NULL);
		SetObserverMode(OBS_MODE_FIXED);
		GERules()->GetSpectatorSpawnSpot(this);
		SetViewOffset(vec3_origin);
		ClearFlags();
	}
	else // Otherwise follow someone around.
	{
		Q_snprintf(specmode, 16, "spec_mode %i", OBS_MODE_CHASE);
		engine->ClientCommand(edict(), specmode);
		CGEMPPlayer *pPlayer = ToGEMPPlayer(FindNextObserverTarget(false));
		if (IsValidObserverTarget(pPlayer))
		{
			SetObserverMode(OBS_MODE_CHASE);
			SetObserverTarget(pPlayer);
		}
	}
}

bool CGEMPPlayer::StartObserverMode( int mode )
{
	// Cache our current state
	bool was_observer = IsObserver();

	BaseClass::StartObserverMode( mode );

	// If we made it to observer let the gameplay know
	if ( !was_observer && IsObserver() )
		GEGameplay()->GetScenario()->OnPlayerObserver( this );

	return true;
}

bool CGEMPPlayer::SetObserverTarget(CBaseEntity *target)
{
	if ( !IsValidObserverTarget( target ) )
	{
		// Find us a valid target
		target = FindNextObserverTarget( false );
		// If we still couldn't find one bail out
		if ( !target )
			return false;
	}

	// set new target, if it's a bot set it to the AI
	CGEBotPlayer *pBot = dynamic_cast<CGEBotPlayer*>( target );
	if ( pBot )
		m_hObserverTarget.Set( pBot->GetNPC() );
	else
		m_hObserverTarget.Set( target ); 

	// reset fov to default
	SetFOV( this, 0 );	

	if ( m_iObserverMode == OBS_MODE_ROAMING )
	{
		Vector	dir, end;
		Vector	start = target->EyePosition();

		AngleVectors( target->EyeAngles(), &dir );
		VectorNormalize( dir );
		VectorMA( start, -64.0f, dir, end );

		Ray_t ray;
		ray.Init( start, end, VEC_DUCK_HULL_MIN	, VEC_DUCK_HULL_MAX );

		trace_t	tr;
		UTIL_TraceRay( ray, MASK_PLAYERSOLID, target, COLLISION_GROUP_PLAYER_MOVEMENT, &tr );

		JumptoPosition( tr.endpos, target->EyeAngles() );
	}

	return true;
}

bool CGEMPPlayer::IsValidObserverTarget(CBaseEntity * target)
{
	if ( !target )
		return false;

	if ( !target->IsPlayer() )
	{
		// Convert us to a bot player then try to the standard tests
		CGEBotPlayer *pBot = ToGEBotPlayer( target );
		if ( pBot && !pBot->IsInRound() )
			return false;

		return BaseClass::IsValidObserverTarget( pBot );
	}
	else
	{
		if ( !ToGEMPPlayer(target)->IsInRound() )
			return false;

		return BaseClass::IsValidObserverTarget( target );
	}
}

void CGEMPPlayer::CheckObserverSettings()
{
	// Observers are never frozen
	RemoveFlag( FL_FROZEN );

	BaseClass::CheckObserverSettings();

	// If we are observing an NPC do some special processing that is missed in the baseclass
	CGEBotPlayer *pBot = ToGEBotPlayer( m_hObserverTarget.Get() );
	if ( pBot )
	{
		CNPC_GEBase *pNPC = pBot->GetNPC();

		if ( m_iObserverMode == OBS_MODE_IN_EYE )
		{
			int flags = GetFlags();
			flags &= (pNPC->GetGroundEntity() != NULL)	? FL_ONGROUND : ~FL_ONGROUND;
			flags &= (pNPC->IsCrouching())				? FL_DUCKING  : ~FL_DUCKING;

			ClearFlags();
			AddFlag( flags );

			if ( pNPC->GetViewOffset() != GetViewOffset() )
				SetViewOffset( pNPC->GetViewOffset() );
		}

		// Update the fog.
		if ( pBot )
		{
			if ( pBot->m_Local.m_PlayerFog.m_hCtrl.Get() != m_Local.m_PlayerFog.m_hCtrl.Get() )
			{
				m_Local.m_PlayerFog.m_hCtrl.Set( pBot->m_Local.m_PlayerFog.m_hCtrl.Get() );
			}
		}
	}
}

int CGEMPPlayer::GetNextObserverSearchStartPoint( bool bReverse )
{
	int iDir = bReverse ? -1 : 1; 
	int startIndex;

	CGEBotPlayer *pBot = ToGEBotPlayer( m_hObserverTarget );
	if ( pBot )
	{
		// We are observing a bot, use him as the start point
		startIndex = pBot->entindex();

		startIndex += iDir;
		if (startIndex > gpGlobals->maxClients)
			startIndex = 1;
		else if (startIndex < 1)
			startIndex = gpGlobals->maxClients;

		return startIndex;
	}
	else
	{
		// Otherwise defer to standard behavior
		return BaseClass::GetNextObserverSearchStartPoint( bReverse );
	}
}

bool CGEMPPlayer::BumpWeapon( CBaseCombatWeapon *pWeapon )
{
	// Don't allow weapon pickup in between rounds
	if ( GEMPRules()->IsIntermission() )
		return false;

	CGEWeapon *pGEWeapon = ToGEWeapon(pWeapon);
	if ( !pGEWeapon->CanEquip(this) || !GEGameplay()->GetScenario()->CanPlayerHaveItem(this, pGEWeapon) )
		return false;

	bool forcepickup = GERules()->ShouldForcePickup(this, pWeapon);

	// We have to do this before we actually fire the weapon pickup code so the skin data is overwritten before it starts getting read.
	if (GetUsedWeaponSkin(pGEWeapon->GetWeaponID()) < pWeapon->m_nSkin && pGEWeapon->GetWeaponID() < WEAPON_RANDOM)
	{
		SetUsedWeaponSkin(pGEWeapon->GetWeaponID(), pWeapon->m_nSkin);
		forcepickup = true;  //If we got a new skin, pick up the weapon no matter what.
	}

	bool ret = BaseClass::BumpWeapon(pWeapon);
	if (!ret && (forcepickup || ((pGEWeapon->GetWeaponID() == WEAPON_MOONRAKER || pGEWeapon->GetWeaponID() == WEAPON_KNIFE) && IsAllowedToPickupWeapons())))
	{
		// If we didn't pick it up and the game rules say we should have anyway remove it from the world
		pWeapon->SetOwner( NULL );
		pWeapon->SetOwnerEntity( NULL );
		UTIL_Remove(pWeapon);
		EmitSound("BaseCombatCharacter.AmmoPickup");
	}
	else if ( ret )
	{
		// Tell plugins what we picked up
		NotifyPickup( pWeapon->GetClassname(), 0 );

		// Kill radar invisibility if this weapon is too powerful.
		if (GetStrengthOfWeapon(pGEWeapon->GetWeaponID()) > 4 && GEMPRules()->GetSpawnInvulnCanBreak())
			StopInvul();
	}
	
	return ret;
}

// Called when the player leaves the server to clean up anything they may have left behind
// this avoids null entity on owner error messages
void CGEMPPlayer::RemoveLeftObjects()
{
	CBaseEntity *pEnt = NULL;
	for ( int i=0; i < m_hThrownObjects.Count(); i++ )
	{
		pEnt = (CBaseEntity*)m_hThrownObjects[i].Get();
		if ( pEnt )
			UTIL_Remove( pEnt );
	}
}

void CGEMPPlayer::FreezePlayer( bool state /*=true*/ )
{
	if ( state )
		AddFlag( FL_FROZEN );
	else
		RemoveFlag( FL_FROZEN );
}

void CGEMPPlayer::QuickStartObserverMode()
{
	if ( m_iObserverLastMode <= OBS_MODE_DEATHCAM )
		m_iObserverLastMode = OBS_MODE_ROAMING;

	m_bForcedObserverMode = false;
	m_afPhysicsFlags |= PFLAG_OBSERVER;
	m_iObserverMode.Set( m_iObserverLastMode );

	// Holster weapon immediately, to allow it to cleanup
	if ( GetActiveWeapon() )
		GetActiveWeapon()->Holster();

	// clear out the suit message cache so we don't keep chattering
	SetSuitUpdate(NULL, FALSE, 0);
	SetGroundEntity( (CBaseEntity *)NULL );
	RemoveFlag( FL_DUCKING );
	AddSolidFlags( FSOLID_NOT_SOLID );

	ShowViewPortPanel( "specgui" , ModeWantsSpectatorGUI(m_iObserverLastMode) );

	// Setup flags
	m_Local.m_iHideHUD = HIDEHUD_HEALTH;
	m_takedamage = DAMAGE_NO;		

	// Become invisible
	AddEffects( EF_NODRAW );		

	m_iHealth = 1;
	m_lifeState = LIFE_DEAD; // Can't be dead, otherwise movement doesn't work right.
	m_flDeathAnimTime = gpGlobals->curtime;
	pl.deadflag = true;
}

void CGEMPPlayer::FinishClientPutInServer()
{
	BaseClass::FinishClientPutInServer();

	char sName[128];
	Q_strncpy( sName, GetPlayerName(), sizeof( sName ) );
	
	// First parse the name and remove any %'s
	for ( char *pApersand = sName; pApersand != NULL && *pApersand != 0; pApersand++ )
	{
		// Replace it with a space
		if ( *pApersand == '%' )
				*pApersand = ' ';
	}

	// Add ourselves to the radar resource (we will never be removed unless we disconnect)
	Color col(0,0,0,0);
	g_pRadarResource->AddRadarContact( this, RADAR_TYPE_PLAYER, false, "", col );
	
	// notify other clients of player joining the game
	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Game_connected", sName[0] != 0 ? sName : "<unconnected>" );

	// Show the player the scenario help
	ShowViewPortPanel( PANEL_SCENARIOHELP, true, new KeyValues("data","pane","-1") );
	ShowViewPortPanel( PANEL_TEAM );
}

void CGEMPPlayer::OnGameplayEvent( GPEvent event )
{
	if ( event == SCENARIO_INIT )
	{
		SetDamageMultiplier( 1.0f );
		SetSpeedMultiplier( 1.0f );
		SetMaxHealth( MAX_HEALTH );
		SetMaxArmor( MAX_ARMOR );
		SetInitialSpawn();  // Next time we spawn will be the "first" time
		HintMessage( "", 0 );  // Reset hint messages
	}
	else if ( event == MATCH_START )
	{
		// Show the scenario help (if applicable) on match start
		ShowScenarioHelp();
	}
	else if ( event == ROUND_START )
	{
		// Reset markers on round start
		SetScoreBoardColor( SB_COLOR_NORMAL );
	}
	else if ( event == ROUND_END || event == MATCH_END )
	{
		// Freeze us when the round or match ends
		FreezePlayer();
	}
}

void CGEMPPlayer::ShowScenarioHelp()
{
	ShowViewPortPanel( PANEL_SCENARIOHELP, true, new KeyValues("data","pane","-1") );
}

// Spawn punch debugging
#ifdef DEBUG
ConVar ge_punch_pitch( "ge_punch_pitch", "60" );
ConVar ge_punch_yaw( "ge_punch_yaw", "20" );
ConVar ge_punch_roll( "ge_punch_roll", "0" );
ConVar ge_punch_vel( "ge_punch_vel", "0" );

CON_COMMAND( ge_punch_doit, "" )
{
	CGEMPPlayer *pPlayer = ToGEMPPlayer( UTIL_PlayerByIndex( 1 ) );
	if ( !pPlayer )
		return;

	int yawdir = 1;
	if ( GERandom<int>(2) ) // [0,1]
		yawdir = -1;

	pPlayer->m_Local.m_vecPunchAngle.Set( PITCH, ge_punch_pitch.GetFloat() );
	pPlayer->m_Local.m_vecPunchAngle.Set( YAW, yawdir*ge_punch_yaw.GetFloat() );
	pPlayer->m_Local.m_vecPunchAngle.Set( ROLL, yawdir*ge_punch_roll.GetFloat() );

	pPlayer->m_Local.m_vecPunchAngleVel = ge_punch_vel.GetFloat() * QAngle( 1, yawdir, 0 ); 
}
#endif

#include "npc_gebase.h"

CGEMPPlayer* ToGEMPPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity )
		return NULL;

	if ( pEntity->IsNPC() )
	{
#ifdef _DEBUG
		return dynamic_cast<CGEMPPlayer*>( ((CNPC_GEBase*)pEntity)->GetBotPlayer() );
#else
		return static_cast<CGEMPPlayer*>( ((CNPC_GEBase*)pEntity)->GetBotPlayer() );
#endif
	}
	else if ( pEntity->IsPlayer() )
	{
#ifdef _DEBUG
		return dynamic_cast<CGEMPPlayer*>( pEntity );
#else
		return static_cast<CGEMPPlayer*>( pEntity );
#endif
	}

	return NULL;
}
