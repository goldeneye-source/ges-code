//////////  Copyright � 2011, Goldeneye Source. All rights reserved. ///////////
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
#include "gemp_player.h"
#include "ai_network.h"
#include "ge_ai.h"
#include "ai_node.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Efficiency of our desirability calcs
#define SPAWNER_CALC_EFFICIENCY		1.0f
// Rate decrease of nearby death memory (in points / sec)
#define SPAWNER_USEFADE_LIMIT		40.0f
// Last use limit in secs
#define SPAWNER_DEATHFADE_LIMIT		60.0f
// Default weight for calcs, raising this will decrease the effect of negative weight
#define SPAWNER_DEATHBOOST_BASE		20.0f

#define SPAWNER_DEFAULT_WEIGHT		1000

#define SPAWNER_MAX_ENEMY_WEIGHT	1200
#define SPAWNER_MAX_DEATH_WEIGHT	800
#define SPAWNER_MAX_USE_WEIGHT		800
#define SPAWNER_MAX_ENEMY_DIST		1600.0f
#define SPAWNER_MAX_DEATH_DIST		800.0f

// Spawnflag to disallow bots to use this point
#define SF_NO_BOTS 0x1

LINK_ENTITY_TO_CLASS( info_player_deathmatch, CGEPlayerSpawn );
LINK_ENTITY_TO_CLASS( info_player_spectator, CGEPlayerSpawn );
LINK_ENTITY_TO_CLASS( info_player_janus, CGEPlayerSpawn );
LINK_ENTITY_TO_CLASS( info_player_mi6, CGEPlayerSpawn );

BEGIN_DATADESC( CGEPlayerSpawn )
	DEFINE_KEYFIELD( m_flDesirabilityMultiplier, FIELD_FLOAT, "desirability" ),
	// Think
	DEFINE_THINKFUNC( Think ),
END_DATADESC()

ConVar ge_debug_playerspawns( "ge_debug_playerspawns", "0", FCVAR_CHEAT | FCVAR_GAMEDLL, "Debug spawn point locations and desirability, 1=DM, 2=MI6, 3=Janus" );

CGEPlayerSpawn::CGEPlayerSpawn( void ) 
	: m_iTeam( TEAM_INVALID )
{ 
	memset( m_iPVS, 0, sizeof(m_iPVS) );
	m_bFoundPVS = false;
	m_fLastUseTime = 0;
	m_fUseFadeTime = 0;
	m_fDeathFadeTime = 0;
	m_fMaxSpawnDist = 0;
	m_flDesirabilityMultiplier = 1;
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

	// Multiply the default weight by the hammer modifer
	m_iBaseDesirability = SPAWNER_DEFAULT_WEIGHT * m_flDesirabilityMultiplier;

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


int CGEPlayerSpawn::GetDesirability(CGEPlayer *pRequestor)
{
	// We should never be picked if we are not enabled
	//Proximity is based on the most threatening player in range.  This is calculated mostly on proximity but has many modifiers on top of it.
	//Use is just based on the last usetime of the spawn and the spawns near it.  Should help destribute spawns more evenly.
	//Deaths is a slowly ticking modifier that's meant to add up over time and prevent spawns that are the site of frequent deaths.
	//The death modifier still has a lot of kinks to work out.

	// We should never be picked if we are not enabled
	if (!IsEnabled())
		return 0;

	int myTeam = m_iTeam;

	// Spectators have no loyalty or cares in the world!
	if (myTeam == TEAM_SPECTATOR)
		return m_iBaseDesirability;

	// Adjust for team spawn swaps
	if (GEMPRules()->IsTeamplay() && GEMPRules()->IsTeamSpawnSwapped() && m_iTeam >= FIRST_GAME_TEAM)
		myTeam = (myTeam == TEAM_JANUS) ? TEAM_MI6 : TEAM_JANUS;

	m_iLastEnemyWeight = m_iLastUseWeight = m_iLastDeathWeight = 0;

	// Knock us down for nearby enemies
	FOR_EACH_PLAYER(pPlayer)
	{
		if (pPlayer->IsObserver() || pPlayer->IsDead())
			continue;

		// Check team affiliation
		bool onMyTeam = false;
		if (GEMPRules()->IsTeamplay() && (pPlayer->GetTeamNumber() == myTeam ||
			(myTeam == TEAM_UNASSIGNED && pPlayer->GetTeamNumber() == pRequestor->GetTeamNumber())))
			onMyTeam = true;

		Vector diff = pPlayer->GetAbsOrigin() - GetAbsOrigin();
		float dist = diff.Length2D();
		float life = pPlayer->ArmorValue() + pPlayer->GetHealth();

		// Check if this player is "near" me
		// avoid having to do the fancy math on people across the map.
		if (dist > SPAWNER_MAX_ENEMY_DIST)
			continue;

		// Now check for players on lower levels and pay them slightly less mind since
		// even if it is possible dropping down is often a hassle and not always worth 
		// it for a spawn kill.  If they are higher consider them further than that because going up
		// is even harder.  Hopefully this doesn't cause too many problems around stairs.
		float floorheight = GEMPRules()->GetMapFloorHeight();

		if (diff.z > floorheight)
			dist += 200;
		else if (diff.z < floorheight * -1)
			dist += 300;

		// If player is not in the PVS, check to see how we should treat them.
		if (!CheckInPVS(pPlayer))
		{
			// If I am a DM spawn in teamplay, add a boost for player's near me on my team
			if (abs(diff.z) > floorheight)
				continue;
			// If they are then still apply massive penalties for being outside PVS.
			dist *= 2.00;
			dist += 300;
		}
		else
		{
			trace_t		tr;
			UTIL_TraceLine(GetAbsOrigin() + Vector(0, 0, 32), pPlayer->EyePosition(), MASK_OPAQUE, pPlayer, COLLISION_GROUP_NONE, &tr);

			if (tr.fraction == 1.0f)
			{
				if (dist > SPAWNER_MAX_ENEMY_DIST / 2)
					dist -= SPAWNER_MAX_ENEMY_DIST / 2;
				else
					dist = 0;
			}
		}

		// Scale threat by health level of player.  Players with very low health are
		// treated as just as dangerous as players with high health so new players
		// players don't spawn right on top of nearly dead players and ruin their day

		int adjlife = 160 - life;

		dist /= MIN(2.0, adjlife*adjlife / 51200.00f + 1); // 51200 is 160*160*2, to get 1.5 as the typical max value

		// Check if this player is still close enough after modifiers.
		if (dist > SPAWNER_MAX_ENEMY_DIST)
			continue;

		//Adjusted square root curve so far away players aren't considered as much.
		int threat = (1 - sqrt(dist / SPAWNER_MAX_ENEMY_DIST + 0.05) + 0.025) * 125;


		// If I am a DM spawn in teamplay, make spawn more desirable for teammates

		if (onMyTeam)
			threat *= -0.4;


		// Finally, compare the calculated threat level to the highest one on record and
		// replace it if higher.
		if (abs(threat) > abs(m_iLastEnemyWeight))
			m_iLastEnemyWeight = MIN(100, threat);

	}
	END_OF_PLAYER_LOOP()

	// Clean up nearby deaths and scale our metric
	m_iLastEnemyWeight = RemapValClamped(m_iLastEnemyWeight, 0, 100, 0, SPAWNER_MAX_ENEMY_WEIGHT);

	// Spit out percentage of death time
	float timeDiff = m_fDeathFadeTime - gpGlobals->curtime;
	m_iLastDeathWeight = RemapValClamped(timeDiff, 0, SPAWNER_DEATHFADE_LIMIT, 0, SPAWNER_MAX_DEATH_WEIGHT);

	// Spit out percentage wait from last use
	timeDiff = m_fUseFadeTime - gpGlobals->curtime;
	m_iLastUseWeight = RemapValClamped(timeDiff, 0, SPAWNER_USEFADE_LIMIT, 0, SPAWNER_MAX_USE_WEIGHT);

	// Return a sum of our weight factors against the default
	// PERSONAL MODIFIERS
	//-------------------


	CGEMPPlayer *pUniquePlayer = ToGEMPPlayer(pRequestor);

	// Zero out these penalties because they might not get calculated again.
	m_iUniLastDeathWeight = m_iUniLastUseWeight = 0;

	// They haven't had their first death yet, so don't bother with this nonsense.
	if (pUniquePlayer->GetLastDeath() == Vector(-1, -1, -1))
		return clamp(m_iBaseDesirability - m_iLastEnemyWeight - m_iLastUseWeight - m_iLastDeathWeight, 0, SPAWNER_DEFAULT_WEIGHT);
	
	// Calculate penalty based on where requesting player last died.
	float dist = (pUniquePlayer->GetLastDeath() - GetAbsOrigin()).Length2D();

	if (dist < SPAWNER_MAX_ENEMY_DIST)
		m_iUniLastDeathWeight = (1 - dist / SPAWNER_MAX_ENEMY_DIST) * 0.75 * SPAWNER_MAX_DEATH_WEIGHT;

	// If player spawned within 40 seconds, also calculate penalty based on where they last spawned.  If not double death multiplier.
	if (pUniquePlayer->GetLastSpawnTime() > gpGlobals->curtime - 40)
	{
		dist = (pUniquePlayer->GetLastSpawn() - GetAbsOrigin()).Length2D();

		if (dist < SPAWNER_MAX_ENEMY_DIST)
			m_iUniLastUseWeight = (1 - dist / SPAWNER_MAX_ENEMY_DIST) * 0.75 * SPAWNER_MAX_USE_WEIGHT;
	}
	else
		m_iUniLastDeathWeight *= 2;

	// Return a sum of our weight factors
	return clamp(m_iBaseDesirability - m_iLastEnemyWeight - m_iLastUseWeight - m_iLastDeathWeight - m_iUniLastDeathWeight - m_iUniLastUseWeight, 0, SPAWNER_DEFAULT_WEIGHT);
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
	if (dist > SPAWNER_MAX_DEATH_DIST)
		return;

	float boost = (1 - dist / SPAWNER_MAX_DEATH_DIST) * SPAWNER_DEATHBOOST_BASE;

	if (gpGlobals->curtime > m_fDeathFadeTime)
		m_fDeathFadeTime = gpGlobals->curtime + boost;
	else
		m_fDeathFadeTime = MIN(m_fDeathFadeTime + boost, gpGlobals->curtime + SPAWNER_DEATHFADE_LIMIT);
}

void CGEPlayerSpawn::NotifyOnUse( void )
{
	const CUtlVector<EHANDLE> *vSpawns = GERules()->GetSpawnersOfType(SPAWN_PLAYER);

	m_fLastUseTime = gpGlobals->curtime;

	// Find the spawn furthest from this one to get an idea of how far to spread the word when it gets used.
	if (m_fMaxSpawnDist == 0)
	{
		float maxdist = 0.00;
		for (int i = (vSpawns->Count() - 1); i >= 0; i--)
		{
			CGEPlayerSpawn *pSpawn = (CGEPlayerSpawn*)vSpawns->Element(i).Get();
			Vector diff = pSpawn->GetAbsOrigin() - GetAbsOrigin();
			float dist = diff.Length2DSqr();

			if (dist > maxdist)
				maxdist = dist;
		}

		m_fMaxSpawnDist = sqrt(maxdist)*0.6; //rooting maxdist to correct Length2DSqr and multiplying by 3/5 to limit range.
	}

	Vector myPos = GetAbsOrigin();

	for (int i = (vSpawns->Count() - 1); i >= 0; i--)
	{
		CGEPlayerSpawn *pSpawn = (CGEPlayerSpawn*)vSpawns->Element(i).Get();
		Vector diff = pSpawn->GetAbsOrigin() - GetAbsOrigin();
		float dist = diff.Length2D();
		if (abs(diff.z) < 128)
			pSpawn->SetUseFadeTime((1 - dist / m_fMaxSpawnDist) * 15);
	}
}

void CGEPlayerSpawn::SetUseFadeTime(float usetime)
{
	if (gpGlobals->curtime > m_fUseFadeTime)
		m_fUseFadeTime = gpGlobals->curtime + usetime;
	else
		m_fUseFadeTime = MIN(m_fUseFadeTime + usetime, gpGlobals->curtime + SPAWNER_USEFADE_LIMIT);
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
	m_fLastUseTime		 = 0;
	m_fUseFadeTime		 = 0;
	m_fDeathFadeTime	 = 0;

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
		Q_snprintf( tempstr, 64, "DISABLED" );
		EntityText( ++line, tempstr, duration );
	}
	else
	{
		// We are enabled, show our desirability and stats
		Q_snprintf( tempstr, 64, "Desirability: %i", d );
		EntityText( ++line, tempstr, duration );

		Q_snprintf(tempstr, 64, "Base Desirability: %i", m_iBaseDesirability);
		EntityText(++line, tempstr, duration);

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
			if (m_iUniLastDeathWeight > 0)
			{
				Q_snprintf(tempstr, 64, "Unique Death Weight: -%i", m_iUniLastDeathWeight);
				EntityText(++line, tempstr, duration);
			}
			if (m_iUniLastUseWeight > 0)
			{
				Q_snprintf(tempstr, 64, "Unique Use Weight: -%i", m_iUniLastUseWeight);
				EntityText(++line, tempstr, duration);
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
		if (trace.fraction > 0.1f)
		{
			if (trace.fraction < 0.24f)
				Msg("[%0.2f, %0.2f, %0.2f] Spawn %s is somewhat high off the ground (%0.1f units)!\n", origin.x, origin.y, origin.z, spawn->GetClassname(), trace.fraction*100.0f);
			else
				Warning("[%0.2f, %0.2f, %0.2f] Spawn %s is too high off the ground (%0.1f units)!\n", origin.x, origin.y, origin.z, spawn->GetClassname(), trace.fraction*100.0f);
		}
		// Check for overlapping spawn points
		CBaseEntity *ents[128];
		Vector mins, maxs;

		spawn->CollisionProp()->WorldSpaceAABB( &mins, &maxs );
		int count = UTIL_EntitiesInBox( ents, ARRAYSIZE(ents), mins, maxs, 0 );
		
		for ( int k = 0; k < count; k++ )
		{
			// Check if we collide with a spawn of the same type (ignore spectator spawns)
			if ( Q_strcmp(ents[k]->GetClassname(), "info_player_spectator") && !Q_strcmp(ents[k]->GetClassname(), spawn->GetClassname()) && ents[k] != spawn )
			{
				Msg( "[%0.2f, %0.2f, %0.2f] Spawn %s collides with another spawn of the same type!\n", origin.x, origin.y, origin.z, spawn->GetClassname() );
				break;
			}
		}


		// Now check and see if bots can find their way to the node network from here.
		if (g_pBigAINet->NumNodes() >= 10 && !(spawn->GetSpawnFlags() & 1)) // We don't have enough nodes, or the spawn doesn't allow bots, so don't bug the mapper with this.
		{
			count = g_pBigAINet->NumNodes();
			bool hitsomething = false;

			for (int k = 0; k < count; k++)
			{
				CAI_Node *testnode = g_pBigAINet->GetNode(k);

				if (!testnode)
					continue;

				UTIL_TraceEntity(pPlayer, pPlayer->GetAbsOrigin(), testnode->GetOrigin() + Vector(0, 0, 1), MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &trace);

				// Check if we can reach the node, and if we did then we found at least one node from here and can get to the network.
				if (trace.endpos == testnode->GetOrigin() + Vector(0, 0, 1))
				{
					hitsomething = true;
					break;
				}
			}

			if (!hitsomething)
				Warning("[%0.2f, %0.2f, %0.2f] Spawn %s might trap bots!  Disable bot spawns or add a node it can see.\n", origin.x, origin.y, origin.z, spawn->GetClassname());
		}
	}

	Msg( "Completed testing all player spawns...\n" );
}
