//////////  Copyright © 2011, Goldeneye Source. All rights reserved. ///////////
// 
// ge_playerspawn.cpp
//
// Description:
//     See Header
//
// Created On: 6/11/2011
// Created By: Killer Monkey
////////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ge_playerspawn.h"
#include "gemp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Efficiency of our desirability calcs
#define SPAWNER_CALC_EFFICIENCY		1.0f
// Rate decrease of nearby death memory (in points / sec)
#define SPAWNER_DEATHFORGET_RATE	0.2f
// Last use limit in secs
#define SPAWNER_LASTUSE_LIMIT		10.0f
// Default weight for calcs, raising this will decrease the effect of negative weight
#define SPAWNER_DEFAULT_WEIGHT		500

#define SPAWNER_MAX_ENEMY_WEIGHT	600
#define SPAWNER_MAX_DEATH_WEIGHT	100
#define SPAWNER_MAX_USE_WEIGHT		300

#define SPAWNER_MAX_ENEMY_DIST		1500.0f

// Spawnflag to disallow bots to use this point
#define SF_NO_BOTS 0x1

LINK_ENTITY_TO_CLASS( info_player_deathmatch, CGEPlayerSpawn );
LINK_ENTITY_TO_CLASS( info_player_spectator, CGEPlayerSpawn );
LINK_ENTITY_TO_CLASS( info_player_janus, CGEPlayerSpawn );
LINK_ENTITY_TO_CLASS( info_player_mi6, CGEPlayerSpawn );

BEGIN_DATADESC( CGEPlayerSpawn )
	// Think
	DEFINE_THINKFUNC( Think ),
END_DATADESC()

ConVar ge_debug_playerspawns( "ge_debug_playerspawns", "0", FCVAR_CHEAT | FCVAR_GAMEDLL, "Debug spawn point locations and desirability, 1=DM, 2=MI6, 3=Janus" );

CGEPlayerSpawn::CGEPlayerSpawn( void ) 
	: m_iTeam( TEAM_INVALID )
{ 
	memset( m_iPVS, 0, sizeof(m_iPVS) );
	m_bFoundPVS = false;
}

void CGEPlayerSpawn::Spawn( void )
{
	//
	// NOTE: We specifically do not call down into our 
	//		 base spawning methods because they will mess us up!
	//
	SetSolid( SOLID_NONE );
	ResetTrackers();

	// Classify us with a team
	if ( !Q_stricmp(GetClassname(), "info_player_deathmatch") )
		m_iTeam = TEAM_UNASSIGNED;
	else if ( !Q_stricmp(GetClassname(), "info_player_spectator") )
		m_iTeam = TEAM_SPECTATOR;
	else if ( !Q_stricmp(GetClassname(), "info_player_janus") )
		m_iTeam = TEAM_JANUS;
	else if ( !Q_stricmp(GetClassname(), "info_player_mi6") )
		m_iTeam = TEAM_MI6;

	SetThink( &CGEPlayerSpawn::Think );
	SetNextThink( gpGlobals->curtime );
}

void CGEPlayerSpawn::OnInit( void )
{
	ResetTrackers();
}

void CGEPlayerSpawn::Think( void )
{
	SetNextThink( gpGlobals->curtime + SPAWNER_CALC_EFFICIENCY );

	// If we are debugging spawners show it now
	if ( ge_debug_playerspawns.GetBool() )
		DEBUG_ShowOverlay( SPAWNER_CALC_EFFICIENCY );
}

int CGEPlayerSpawn::GetDesirability( CGEPlayer *pRequestor )
{
	// We should never be picked if we are not enabled
	if ( !IsEnabled() )
		return 0;

	int myTeam = m_iTeam;

	// Spectators have no loyalty or cares in the world!
	if ( myTeam == TEAM_SPECTATOR )
		return SPAWNER_DEFAULT_WEIGHT;

	// Adjust for team spawn swaps
	if ( GEMPRules()->IsTeamplay() && GEMPRules()->IsTeamSpawnSwapped() && m_iTeam >= FIRST_GAME_TEAM )
		myTeam = (myTeam == TEAM_JANUS) ? TEAM_MI6 : TEAM_JANUS;

	m_iLastEnemyWeight = m_iLastUseWeight = m_iLastDeathWeight = 0;

	// Knock us down for nearby enemies
	FOR_EACH_PLAYER( pPlayer )
	{
		if ( pPlayer->IsObserver() || pPlayer->IsDead() )
			continue;

		if ( !CheckInPVS( pPlayer ) )
			continue;

		// Check team affiliation
		bool onMyTeam = false;
		if ( GEMPRules()->IsTeamplay() && (pPlayer->GetTeamNumber() == myTeam ||
			(myTeam == TEAM_UNASSIGNED && pPlayer->GetTeamNumber() == pRequestor->GetTeamNumber()) ) )
			onMyTeam = true;

		Vector diff = pPlayer->GetAbsOrigin() - GetAbsOrigin();
		float dist = diff.Length2D();

		// Check if this player is "near" me
		if ( abs(diff.z) <= 100.0f && dist <= SPAWNER_MAX_ENEMY_DIST )
		{
			int weight = min( SPAWNER_MAX_ENEMY_WEIGHT, 
							  (int)( SPAWNER_MAX_ENEMY_DIST * (1.0f - Bias( dist / SPAWNER_MAX_ENEMY_DIST, 0.8 )) ) );

			// If I am a DM spawn in teamplay, add a boost for player's near me on my team
			if ( onMyTeam )
				m_iLastEnemyWeight -= weight;
			else
				m_iLastEnemyWeight += weight;
		}
	}
	END_OF_PLAYER_LOOP()

	// Clean up nearby deaths and scale our metric
	float timeDiff = gpGlobals->curtime - m_fLastDeathCalc;
	m_fLastDeathCalc = gpGlobals->curtime;
	m_fNearbyDeathMetric = max( 0, m_fNearbyDeathMetric - (timeDiff * SPAWNER_DEATHFORGET_RATE) );

	m_iLastDeathWeight = min( SPAWNER_MAX_DEATH_WEIGHT, SPAWNER_MAX_DEATH_WEIGHT*m_fNearbyDeathMetric );

	// Spit out percentage wait from last use
	timeDiff = gpGlobals->curtime - m_fLastUseTime;
	m_iLastUseWeight = RemapValClamped( timeDiff, 0, SPAWNER_LASTUSE_LIMIT, SPAWNER_MAX_USE_WEIGHT, 0 );

	// Return a sum of our weight factors against the default
	return max( SPAWNER_DEFAULT_WEIGHT - m_iLastEnemyWeight - m_iLastDeathWeight - m_iLastUseWeight, 0 );
}

bool CGEPlayerSpawn::IsOccupied( void )
{
	if ( !IsEnabled() )
		return false;

	// Quick check if we have been used very recently
	if ( gpGlobals->curtime < (m_fLastUseTime + 0.5f) )
		return true;

	CBaseEntity *pList[32];
	int count = UTIL_EntitiesInBox( pList, 32, GetAbsOrigin() + VEC_HULL_MIN, GetAbsOrigin() + VEC_HULL_MAX, 0 );
	for ( int i = 0; i < count; i++ )
	{
		if ( (pList[i]->IsPlayer() || pList[i]->IsNPC()) && pList[i]->IsAlive() && !(pList[i]->GetEFlags() & EF_NODRAW) )
			return true;
	}

	return false;
}

bool CGEPlayerSpawn::IsBotFriendly( void )
{
	return !HasSpawnFlags( SF_NO_BOTS );
}

void CGEPlayerSpawn::NotifyOnDeath( float dist )
{
	if ( dist < SPAWNER_MAX_ENEMY_DIST )
	{
		// Give a boost for rapid deaths
		float timeScale = RemapValClamped( gpGlobals->curtime - m_fLastDeathNotice, 0, 5.0f, 3.0f, 1.0f );
		m_fNearbyDeathMetric += Bias( RemapValClamped( dist, SPAWNER_MAX_ENEMY_DIST, 0, 0, 1.0f ), 0.8 ) * timeScale;
		m_fLastDeathNotice = m_fLastDeathCalc = gpGlobals->curtime;
	}
}

void CGEPlayerSpawn::NotifyOnUse( void )
{
	m_fLastUseTime = gpGlobals->curtime;
}

void CGEPlayerSpawn::OnEnabled( void )
{
	ResetTrackers();
}

void CGEPlayerSpawn::OnDisabled( void )
{
	ResetTrackers();
}

void CGEPlayerSpawn::ResetTrackers( void )
{
	// Reset static counters
	m_fLastDeathNotice	 = 0;
	m_fLastDeathCalc	 = 0;
	m_fNearbyDeathMetric = 0;
	m_fLastUseTime		 = 0;

	// Reset Weights
	m_iLastDeathWeight = m_iLastUseWeight = m_iLastEnemyWeight = 0;
}

bool CGEPlayerSpawn::CheckInPVS( CGEPlayer *player )
{
	if ( !m_bFoundPVS )
	{
		int clusterIndex = engine->GetClusterForOrigin( EyePosition() );
		engine->GetPVSForCluster( clusterIndex, sizeof(m_iPVS), m_iPVS );
		m_bFoundPVS = true;
	}

	if ( player->NetworkProp()->IsInPVS( edict(), m_iPVS, sizeof(m_iPVS) ) )
		return true;

	return false;
}

void CGEPlayerSpawn::DEBUG_ShowOverlay( float duration )
{
	// Only show this on local host servers
	if ( engine->IsDedicatedServer() )
		return;

	CGEPlayer *pPlayer = (CGEPlayer*) UTIL_PlayerByIndex( 1 );
	if ( !pPlayer )
		return;

	int myTeam = m_iTeam;

	if ( GEMPRules()->IsTeamplay() || ge_debug_playerspawns.GetInt() > 1 )
	{
		// Filter for "team spawning"
		if ( (GEMPRules()->IsTeamSpawn() || ge_debug_playerspawns.GetInt() > 1) && m_iTeam <= LAST_SHARED_TEAM )
			return;
		else if ( !GEMPRules()->IsTeamSpawn() && m_iTeam > TEAM_UNASSIGNED )
			return;

		// Adjust team for swapped spawns
		if ( myTeam >= FIRST_GAME_TEAM && GEMPRules()->IsTeamSpawnSwapped() )
			myTeam = (myTeam == TEAM_JANUS) ? TEAM_MI6 : TEAM_JANUS;
	}
	else if ( m_iTeam > TEAM_UNASSIGNED )
	{
		// Non-teamplay disregard spectator and team spawns
		return;
	}

	int line = 0;
	char tempstr[64];

	// Calculate our desirability
	int d = 0;
	if ( !IsOccupied() )
		d = GetDesirability( pPlayer );

	int r = (int) RemapValClamped( d, 0, 1.0f, 255.0f, 0 );
	int g = (int) RemapValClamped( d, 0, 1.0f, 0, 255.0f );

	Color c( r, g, 0, 200 );
	debugoverlay->AddBoxOverlay2( GetAbsOrigin(), VEC_HULL_MIN, VEC_HULL_MAX, GetAbsAngles(), c, c, duration );

	if ( ge_debug_playerspawns.GetInt() > 1 )
		Q_snprintf( tempstr, 64, "%s", GetClassname() );
	else
		Q_snprintf( tempstr, 64, "%s", (myTeam == TEAM_UNASSIGNED) ? "Deathmatch Spawn" : ((myTeam == TEAM_JANUS) ? "Janus Spawn" : "MI6 Spawn") );
	
	// Show spawner name
	EntityText( ++line, tempstr, duration );

	if ( IsDisabled() )
	{
		// We are disabled
		Q_snprintf( tempstr, 64, "DISABLED", d );
		EntityText( ++line, tempstr, duration );
	}
	else
	{
		// We are enabled, show our desirability and stats
		Q_snprintf( tempstr, 64, "Desirability: %i", d );
		EntityText( ++line, tempstr, duration );

		if ( IsOccupied() )
		{
			EntityText( ++line, "OCCUPIED!", duration );
		}
		else
		{
			if ( m_iLastEnemyWeight > 0 )
			{
				Q_snprintf( tempstr, 64, "Enemy Weight: -%i", m_iLastEnemyWeight );
				EntityText( ++line, tempstr, duration );
			}
			if ( m_iLastDeathWeight > 0 )
			{
				Q_snprintf( tempstr, 64, "Death Weight: -%i", m_iLastDeathWeight );
				EntityText( ++line, tempstr, duration );
			}
			if ( m_iLastUseWeight > 0 )
			{
				Q_snprintf( tempstr, 64, "Use Weight: -%i", m_iLastUseWeight );
				EntityText( ++line, tempstr, duration );
			}
		}
	}

	if ( !IsBotFriendly() )
		EntityText( ++line, "NO BOTS", duration );
}

#include "gemp_player.h"
CON_COMMAND_F( ge_debug_checkplayerspawns, "Check player spawns against several criteria including getting stuck.", FCVAR_CHEAT )
{
	int playeridx = UTIL_GetCommandClientIndex();
	CGEMPPlayer *pPlayer = ToGEMPPlayer( UTIL_PlayerByIndex( playeridx ) );

	if ( !UTIL_IsCommandIssuedByServerAdmin() || !pPlayer )
	{
		Warning( "You must be a localhost admin to use this command!\n" );
		return;
	}

	if ( !pPlayer->IsActive() )
	{
		Warning( "You must be spawned into the game in order to use this command!\n" );
		return;
	}

	CUtlVector<EHANDLE> spawns;
	spawns.AddVectorToTail( *GERules()->GetSpawnersOfType( SPAWN_PLAYER ) );
	spawns.AddVectorToTail( *GERules()->GetSpawnersOfType( SPAWN_PLAYER_MI6 ) );
	spawns.AddVectorToTail( *GERules()->GetSpawnersOfType( SPAWN_PLAYER_JANUS ) );

	for ( int i=0; i < spawns.Count(); i++ )
	{
		CGEPlayerSpawn *spawn = (CGEPlayerSpawn*) spawns[i].Get();
		if ( !spawn )
			continue;

		trace_t trace;

		// Move the player to this spawn point
		Vector origin = spawn->GetAbsOrigin();
		pPlayer->SetLocalOrigin( origin + Vector(0,0,1) );
		pPlayer->SetAbsVelocity( vec3_origin );
		pPlayer->SetLocalAngles( spawn->GetLocalAngles() );
		pPlayer->m_Local.m_vecPunchAngle = vec3_angle;
		pPlayer->m_Local.m_vecPunchAngleVel = vec3_angle;
		pPlayer->SnapEyeAngles( spawn->GetLocalAngles() );

		// Check if the player will get stuck
		UTIL_TraceEntity( pPlayer, pPlayer->GetAbsOrigin(), pPlayer->GetAbsOrigin(), MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
		if ( trace.allsolid || trace.startsolid )
			Warning( "[%0.2f, %0.2f, %0.2f] Spawn %s will get the player stuck!\n", origin.x, origin.y, origin.z, spawn->GetClassname() );

		// Check for height above the ground
		UTIL_TraceLine( origin + Vector(0,0,1), origin + Vector(0,0,-100), MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &trace );
		if ( trace.fraction > 0.1f )
			Warning( "[%0.2f, %0.2f, %0.2f] Spawn %s is too high off the ground (%0.1f units)!\n", origin.x, origin.y, origin.z, spawn->GetClassname(), trace.fraction*100.0f );

		// Check for overlapping spawn points
		CBaseEntity *ents[128];
		Vector mins, maxs;

		spawn->CollisionProp()->WorldSpaceAABB( &mins, &maxs );
		int count = UTIL_EntitiesInBox( ents, ARRAYSIZE(ents), mins, maxs, 0 );
		
		for ( int k = 0; i < count; i++ )
		{
			// Check if we collide with a spawn of the same type (ignore spectator spawns)
			if ( Q_strcmp( ents[k]->GetClassname(), "info_player_spectator" ) && !Q_strcmp( ents[k]->GetClassname(), spawn->GetClassname() ) )
			{
				Warning( "[%0.2f, %0.2f, %0.2f] Spawn %s collides with another spawn of the same type!\n", origin.x, origin.y, origin.z, spawn->GetClassname() );
				break;
			}
		}
	}

	Msg( "Completed testing all player spawns...\n" );
}
