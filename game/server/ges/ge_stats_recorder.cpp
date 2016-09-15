/////////////  Copyright © 2008, Goldeneye: Source. All rights reserved.  /////////////
// 
// ge_stats_recorder.cpp
//
// Description:
//      This code captures all events that occur on the server and translates them into
//		award points. This is a passive function until called upon by GEGameplay to
//		give the results at the end of a round.
//
// Created On: 20 Dec 08
// Creator: Killermonkey
////////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ge_shareddefs.h"
#include "ge_stats_recorder.h"
#include "ge_playerresource.h"
#include "ge_player.h"
#include "gemp_player.h"
#include "ge_weapon.h"
#include "grenade_gebase.h"

#include "gemp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define GE_STATS_PLAYERRATIO 2.5f

// ----------------------------------------------------
// GEPlayerStats definitions
// ----------------------------------------------------
GEPlayerStats::GEPlayerStats( CBasePlayer *player )
{
	m_hPlayer = player;
	ResetRoundStats();
	ResetMatchStats();
}

void GEPlayerStats::AddStat( int idx, int amt )
{
	if ( idx >= 0 && idx < GE_AWARD_MAX )
		m_iRoundStats[idx] += amt;
}

void GEPlayerStats::SetStat( int idx, int amt )
{
	if ( idx >= 0 && idx < GE_AWARD_MAX )
		m_iRoundStats[idx] = amt;
}

int GEPlayerStats::GetRoundStat( int idx )
{
	if ( idx >= 0 && idx < GE_AWARD_MAX )
		return m_iRoundStats[idx];
	return 0;
}

int GEPlayerStats::GetMatchStat( int idx )
{
	if ( idx >= 0 && idx < GE_AWARD_MAX )
		return m_iMatchStats[idx];
	return 0;
}

void GEPlayerStats::ResetRoundStats( void )
{
	for ( int i=0; i < GE_AWARD_MAX; i++ )
		m_iRoundStats[i] = 0;

	for ( int i=0; i < WEAPON_MAX; i++ )
		m_iWeaponsHeldTime[i] = m_iWeaponsKills[i] = 0;

	m_iDeaths = 0;
}

void GEPlayerStats::AddRoundToMatchStats( void )
{
	for ( int i=0; i < GE_AWARD_MAX; i++ )
		m_iMatchStats[i] += m_iRoundStats[i];
}

void GEPlayerStats::ResetMatchStats( void )
{
	for ( int i=0; i < GE_AWARD_MAX; i++ )
		m_iMatchStats[i] = 0;
	for ( int i=0; i < WEAPON_MAX; i++ )
		m_iMatchWeapons[i] = 0;
	m_iDeaths = 0;
}

// ----------------------------------------------------
// CGEStats definitions
// ----------------------------------------------------

static CGEStats s_GEStats;
extern ConVar ge_rounddelay;

CGEStats::CGEStats( void )
{
	gamestats = &s_GEStats;
	m_iRoundCount = 0;
	m_flRoundStartTime = 0.0f;
}

CGEStats::~CGEStats( void )
{
	m_pPlayerStats.PurgeAndDeleteElements();
}

void CGEStats::ResetRoundStats( void )
{
	for ( int i=0; i < m_pPlayerStats.Count(); i++ )
	{
		m_pPlayerStats[i]->AddRoundToMatchStats();
		m_pPlayerStats[i]->ResetRoundStats();
	}

	m_flRoundStartTime = gpGlobals->curtime;
	m_iRoundCount++;
}

void CGEStats::ResetMatchStats( void )
{
	for ( int i=0; i < m_pPlayerStats.Count(); i++ )
	{
		m_pPlayerStats[i]->ResetRoundStats();
	}

	m_iRoundCount = 0;
}

int CGEStats::GetPlayerStat( int idx, CBasePlayer *player )
{
	int iPlayer = FindPlayer(player);
	if ( iPlayer == -1 || (idx < 0 || idx >= GE_AWARD_MAX) )
		return 0;

	return m_pPlayerStats[iPlayer]->GetRoundStat(idx);
}

GEPlayerStats *CGEStats::FindPlayerStats( CBasePlayer *player )
{
	for (int i=0; i < m_pPlayerStats.Count(); i++)
	{
		if ( m_pPlayerStats[i]->GetPlayer() == player )
			return m_pPlayerStats[i];
	}

	return NULL;
}

int CGEStats::FindPlayer( CBasePlayer *player )
{
	for (int i=0; i < m_pPlayerStats.Count(); i++)
	{
		if ( m_pPlayerStats[i]->GetPlayer() == player )
			return i;
	}

	return -1;
}

void CGEStats::SetAwardsInEvent( IGameEvent *pEvent )
{   
	// Ignore awards when only 1 person is playing
	if ( GEMPRules()->GetNumActivePlayers() < 2 )
		return;

	CUtlVector<GEStatSort*> vAwards;
	GEStatSort *award;
	int i;

	// Prevent divide by zero
	if ( m_iRoundCount == 0 )
		m_iRoundCount = 1;

	for ( i=0; i < GE_AWARD_GIVEMAX; i++ )
	{
		// Check for valid award
		if ( AwardIDToIdent(i) )
		{
			award = new GEStatSort;
			// See if we are going to give this one out, if so store it, if not erase the dummy var
			if ( GetAwardWinner(i, *award) )
				vAwards.AddToTail( award );
			else
				delete award;
		}
	}

	// Sort our ratios from High to Low
	vAwards.Sort( &CGEStats::StatSortHigh );

	char eventid[16], eventwinner[16];
	CBaseEntity *pPlayer;
	for ( i=0; i < vAwards.Count(); i++ )
	{
		// Never give out more than 6 awards
		if ( i == 6 )
			break;

		pPlayer = m_pPlayerStats[ vAwards[i]->idx ]->GetPlayer();
		if ( !pPlayer )
			continue;

		Q_snprintf( eventid, 16, "award%i_id", i+1 );
		Q_snprintf( eventwinner, 16, "award%i_winner", i+1 );

		pEvent->SetInt( eventid, vAwards[i]->m_iAward );
		pEvent->SetInt( eventwinner, pPlayer->entindex() );
	}

	vAwards.PurgeAndDeleteElements();
}

void CGEStats::SetFavoriteWeapons( void )
{
	GEWeaponSort wep;
	CGEPlayer *pPlayer;

	for ( int i=0; i < m_pPlayerStats.Count(); i++ )
	{
		if ( !m_pPlayerStats[i]->GetPlayer() )
			continue;

		wep.m_iWeaponID = WEAPON_NONE;
		wep.m_iPercentage = 0;

		if ( GetFavoriteWeapon( i, wep ) )
		{
			pPlayer = ToGEPlayer( m_pPlayerStats[i]->GetPlayer() );
			pPlayer->SetFavoriteWeapon( wep.m_iWeaponID );
			// Increment our preferred weapon for the match
			m_pPlayerStats[i]->m_iMatchWeapons[ wep.m_iWeaponID ]++;
		}
	}
}

void CGEStats::Event_PlayerConnected( CBasePlayer *pBasePlayer )
{
	GEPlayerStats *playerstats = new GEPlayerStats( pBasePlayer );
	m_pPlayerStats.AddToTail( playerstats );

	BaseClass::Event_PlayerConnected( pBasePlayer );
}

void CGEStats::Event_PlayerDisconnected( CBasePlayer *pBasePlayer )
{
	int iPlayer = FindPlayer(pBasePlayer);

	if ( iPlayer != -1 )
	{
		GEPlayerStats *stat = m_pPlayerStats[iPlayer];
		m_pPlayerStats.Remove(iPlayer);
		delete stat;
	}

	BaseClass::Event_PlayerDisconnected( pBasePlayer );
}

void CGEStats::Event_PlayerKilled( CBasePlayer *pPlayer, const CTakeDamageInfo &info )
{
	int iVictim = FindPlayer( pPlayer );
	if ( iVictim == -1 )
		return;
	CGEMPPlayer *gePlayer = ToGEMPPlayer( pPlayer );

	if (!gePlayer)
		return;

	if ( gePlayer->IsObserver() || gePlayer->GetTeamNumber() == TEAM_SPECTATOR )
		return;

	m_pPlayerStats[iVictim]->m_iDeaths++;
	float deaths = (float)m_pPlayerStats[iVictim]->m_iDeaths;
	float lifetime = gpGlobals->curtime - gePlayer->GetSpawnTime();

	m_pPlayerStats[iVictim]->AddStat( GE_INNING_VAL, lifetime );
	m_pPlayerStats[iVictim]->AddStat( GE_INNING_SQ, lifetime*lifetime );

	int inn;
	int inn_val = m_pPlayerStats[iVictim]->GetRoundStat( GE_INNING_VAL );
	int inn_sq = m_pPlayerStats[iVictim]->GetRoundStat( GE_INNING_SQ );

	// Calculate the running standard deviation of our innings
	float inn_sd = sqrt( (float)(inn_sq - (float)(inn_val*inn_val) / deaths) / deaths );
	
	inn = (int)((float)inn_val/(float)m_pPlayerStats[iVictim]->m_iDeaths - inn_sd);
	m_pPlayerStats[iVictim]->SetStat( GE_AWARD_LONGIN, inn );
	m_pPlayerStats[iVictim]->SetStat( GE_AWARD_SHORTIN, inn );

	// Fake a weapon switch so we can record their usage time of the weapon
	Event_WeaponSwitch( pPlayer, pPlayer->GetActiveWeapon(), NULL );

	//test if its a fall death
	if( !info.GetInflictor() )
	{
		m_pPlayerStats[iVictim]->AddStat( GE_AWARD_PROFESSIONAL, -5);
	}
}

void CGEStats::Event_PlayerKilledOther( CBasePlayer *pAttacker, CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	//Make sure there is a victim and an attacker
	if( !pVictim || !pAttacker )
		return;
	
	//Make sure the victim is a player
	if( !pVictim->IsPlayer() )
		return;

	CBasePlayer *pBaseVictim = static_cast<CBasePlayer*>(pVictim);

	//Return both Player ID's
	int iVictim = FindPlayer( pBaseVictim );
	int iAttacker = FindPlayer( pAttacker );
	
	if( iVictim == -1  || iAttacker == -1 )
		return;

	// Check for suicide
	if ( pVictim == pAttacker )
	{
		m_pPlayerStats[iVictim]->AddStat( GE_AWARD_PROFESSIONAL, -15 );
		m_pPlayerStats[iVictim]->AddStat( GE_AWARD_MOSTLYHARMLESS, 50 );
		m_pPlayerStats[iVictim]->AddStat( GE_AWARD_LEMMING, 3 );
	}
	else
	{
		m_pPlayerStats[iVictim]->AddStat( GE_AWARD_PROFESSIONAL , -5 );
		m_pPlayerStats[iVictim]->AddStat( GE_AWARD_MOSTLYHARMLESS, 30 );

		m_pPlayerStats[iAttacker]->AddStat( GE_AWARD_DEADLY , 50 );
		m_pPlayerStats[iAttacker]->AddStat( GE_AWARD_PROFESSIONAL , 5 );

		int weapid = WEAPON_NONE;
		if ( info.GetWeapon() )
			weapid = ToGEWeapon( (CBaseCombatWeapon*)info.GetWeapon() )->GetWeaponID();
		else if ( !info.GetInflictor()->IsNPC() && Q_stristr( info.GetInflictor()->GetClassname(), "npc_" ) )
			weapid = ToGEGrenade( info.GetInflictor() )->GetWeaponID();

		if ( weapid != WEAPON_NONE )
			// Increment our kills with this weapon
			m_pPlayerStats[iAttacker]->m_iWeaponsKills[weapid]++;
	}

	BaseClass::Event_PlayerKilledOther( pAttacker, pVictim, info );
}

void CGEStats::Event_WeaponFired( CBasePlayer *pShooter, bool bPrimary, char const *pchWeaponName )
{
	// Take off 1 point for every shot fired
	int iPlayer = FindPlayer( pShooter );
	m_pPlayerStats[iPlayer]->AddStat( GE_AWARD_MARKSMANSHIP, -1 );
	// Add 1 point to our frantic score
	m_pPlayerStats[iPlayer]->AddStat( GE_AWARD_FRANTIC, 1 );
}

void CGEStats::Event_WeaponHit( CBasePlayer *pShooter, bool bPrimary, char const *pchWeaponName, const CTakeDamageInfo &info )
{
	int iPlayer = FindPlayer( pShooter );
	m_pPlayerStats[iPlayer]->AddStat( GE_AWARD_MARKSMANSHIP, 1 );
	// Take away a frantic point since they actually hit something
	m_pPlayerStats[iPlayer]->AddStat( GE_AWARD_FRANTIC, -1 );
}

void CGEStats::Event_PlayerDamage( CBasePlayer *pBasePlayer, const CTakeDamageInfo &info )
{
	//Make sure there is a player taking the damage
	if( !pBasePlayer )
		return;
	int iVictim = FindPlayer( pBasePlayer );
	
	CBasePlayer* pAttacker = static_cast<CBasePlayer*>(info.GetAttacker());
	if( !pAttacker->IsPlayer() )
		return;
	int iAttacker = FindPlayer( pAttacker );

	// We hurt ourselves...
	if ( iAttacker == iVictim )
	{
		m_pPlayerStats[iVictim]->AddStat( GE_AWARD_PROFESSIONAL, -2);
		m_pPlayerStats[iVictim]->AddStat( GE_AWARD_LEMMING, 1 );
		return;
	}

	// Add the inflicted damage to our Most Deadly stat
	m_pPlayerStats[iAttacker]->AddStat( GE_AWARD_DEADLY, info.GetDamage() );
	// Add to the victim's mostly harmless award 1/2 the damage
	m_pPlayerStats[iVictim]->AddStat( GE_AWARD_MOSTLYHARMLESS, info.GetDamage() / 2 );

	// See where this attack hit and then apply appropriate marksmanship points
	int points = 0;
	switch ( pBasePlayer->LastHitGroup() )
	{
		case HITGROUP_HEAD:
			points = 5; break;

		case HITGROUP_CHEST:
		case HITGROUP_STOMACH: 
			points = 3; break;

		case HITGROUP_LEFTARM:
		case HITGROUP_RIGHTARM:
		case HITGROUP_LEFTLEG:
		case HITGROUP_RIGHTLEG:
			points = 1; break;
	}

	m_pPlayerStats[iAttacker]->AddStat( GE_AWARD_MARKSMANSHIP, points );

	// test if its fall damage
	if( !info.GetInflictor() )
	{
		m_pPlayerStats[iVictim]->AddStat( GE_AWARD_PROFESSIONAL, -2);
		return;
	}

	// Find out what weapons we are using
	CGEWeapon *pVicWeapon = static_cast<CGEWeapon*>(pBasePlayer->GetActiveWeapon());
	CGEWeapon *pWeapon = NULL;

	if ( info.GetInflictor() == pAttacker )
		pWeapon = static_cast<CGEWeapon*>(pAttacker->GetActiveWeapon());
	else
		pWeapon = dynamic_cast<CGEWeapon*>(info.GetInflictor());

	// If the victim reports no weapon, or our victim wasn't killed by a GEWeapon, then break
	if ( !pWeapon || !pVicWeapon )
		return;

	// Don't give honor to mine kills as it doesn't make a difference
	if ( pWeapon->GetWeaponID() < WEAPON_TIMEDMINE || pWeapon->GetWeaponID() > WEAPON_GRENADE )
	{
		// Define variables to find honor
		Vector vecVic, vecToAtt, right;

		// Get the victim's forward normal, their position, and the attacker's position
		pBasePlayer->EyeVectors( &vecVic, &right );
		vecVic.NormalizeInPlace();

		// Use vector subtration to find a vector to the attacker FROM the victim
		vecToAtt = pAttacker->EyePosition() - pBasePlayer->EyePosition();
		vecToAtt.NormalizeInPlace();
		
		// Find the angle between the two vectors
		float front = DotProduct(vecToAtt, vecVic);
		float side = DotProduct(vecToAtt, right);
		float xpos = 360.0f * -side;
		float ypos = 360.0f * -front;

		// Get the rotation (yaw)
		float ang = abs( atan2(xpos, ypos) );
		float scale;

		if ( ang > 2.10f ) {
			scale = RemapValClamped( ang, 2.10f, 3.14f, 0.2f, 1.0f );
			m_pPlayerStats[iAttacker]->AddStat( GE_AWARD_HONORABLE, 10*scale );
		} else if ( ang < 1.05f ) {
			scale = RemapValClamped( ang, 0.0f, 1.05f, 0.2f, 1.0f );
			m_pPlayerStats[iAttacker]->AddStat( GE_AWARD_DISHONORABLE, 10*scale );
		}
	}

	//Attacker using slappers, Victim not
	if( pWeapon->GetWeaponID() == WEAPON_SLAPPERS && pVicWeapon->GetWeaponID() != WEAPON_SLAPPERS )
	{
		m_pPlayerStats[iAttacker]->AddStat(GE_AWARD_DEADLY, 10);
		m_pPlayerStats[iAttacker]->AddStat(GE_AWARD_HONORABLE, 10);
	}
	
	//Victim using slappers, Attacker not
	if( pWeapon->GetWeaponID() != WEAPON_SLAPPERS && pVicWeapon->GetWeaponID() == WEAPON_SLAPPERS )
	{
		m_pPlayerStats[iAttacker]->AddStat(GE_AWARD_DISHONORABLE, 10);
		m_pPlayerStats[iAttacker]->AddStat(GE_AWARD_PROFESSIONAL, -5);
	}

	BaseClass::Event_PlayerDamage( pBasePlayer, info );
}

void CGEStats::Event_PlayerTraveled( CBasePlayer *pBasePlayer, float distanceInInches, bool bInVehicle, bool bSprinting )
{
	int iPlayer = FindPlayer( pBasePlayer );
	if ( iPlayer == -1 || pBasePlayer->IsObserver() )
		return;

	// Record the absolute number of feet traveled, the farther they go the more frantic they are
	m_pPlayerStats[iPlayer]->AddStat(GE_AWARD_FRANTIC, distanceInInches / 12);

	BaseClass::Event_PlayerTraveled( pBasePlayer, distanceInInches, bInVehicle, bSprinting );
}

void CGEStats::Event_WeaponSwitch( CBasePlayer *player, CBaseCombatWeapon *currWeapon, CBaseCombatWeapon *nextWeapon )
{
	int iPlayer = FindPlayer( player );
	if ( iPlayer == -1 || player->IsObserver() )
		return;

	CGEWeapon *pGEWeapon = ToGEWeapon( currWeapon );
	if ( !pGEWeapon || pGEWeapon->GetWeaponID() <= WEAPON_NONE || pGEWeapon->GetWeaponID() >= WEAPON_MAX )
		return;

	m_pPlayerStats[iPlayer]->m_iWeaponsHeldTime[pGEWeapon->GetWeaponID()] += pGEWeapon->GetHeldTime();
}

void CGEStats::Event_PickedArmor( CBasePlayer *pBasePlayer, int amt )
{
	CGEPlayer *pPlayer = ToGEPlayer( pBasePlayer );
	int iPlayer = FindPlayer( pBasePlayer );
	if ( iPlayer == -1 )
		return;

	// The less armor they actually pickup the higher their AC10 will be
	// thus the person with the highest AC10 is clearly picking up a lot of armor for little gain
	m_pPlayerStats[iPlayer]->AddStat(GE_AWARD_AC10, pPlayer->GetMaxArmor() - amt);

	// Every armor pick pulls them out by 2 points to seperate people who pick armor often and those that don't pick at all
	m_pPlayerStats[iPlayer]->AddStat(GE_AWARD_NOTAC10, 2);
}

void CGEStats::Event_PickedAmmo( CBasePlayer *pBasePlayer, int amt )
{
	int iPlayer = FindPlayer( pBasePlayer );
	if ( iPlayer == -1 )
		return;

	m_pPlayerStats[iPlayer]->AddStat(GE_AWARD_WTA, amt);
}

bool CGEStats::GetAwardWinner( int iAward, GEStatSort &winner )
{
	CUtlVector<GEStatSort*> stats;
	if ( GEMPRules()->GetNumActivePlayers() < 2 )
		return false;

	// Reset any previous inclinations
	winner.m_iAward = -1;
	winner.m_iStat = 0;

	CGEPlayer *pPlayer = NULL;
	// First parse out all the award to player information
	for ( int i=0; i < m_pPlayerStats.Count(); i++ )
	{
		// Don't try to give awards to invalid players
		pPlayer = m_pPlayerStats[i]->GetPlayer();
		if ( !pPlayer )
			continue;
		if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
			continue;

		GEStatSort *entry = new GEStatSort;
		entry->idx = i;

		// If this is the last report, then take the match stats, average them over the rounds, and use that as our stat
		if ( GEGameplay()->IsInFinalIntermission() )
			entry->m_iStat = m_pPlayerStats[i]->GetMatchStat( iAward ) / m_iRoundCount;
		else
			entry->m_iStat = m_pPlayerStats[i]->GetRoundStat( iAward );

		stats.AddToTail( entry );
	}

	// Make sure after our checks we have at least two players to do awards for
	if ( stats.Count() < 2 )
		return false;

	float playerrat = (float)(stats.Count() + GE_STATS_PLAYERRATIO) / (float)stats.Count();

	if ( GetAwardSort(iAward) == GE_SORT_HIGH )
		stats.Sort( &CGEStats::StatSortHigh );
	else
	{
		stats.Sort( &CGEStats::StatSortLow );
		// Since we are sorting low we have to invert our player ratio
		playerrat = 1 / playerrat;
	}


	// Prevent divide by zero and inflation
	if ( stats[1]->m_iStat == 0 )
		stats[1]->m_iStat = 1;

	float statrat = (float)stats[0]->m_iStat / (float)stats[1]->m_iStat;
	
	// Low sort should have a ratio less than the inverted playerrat
	if ( GetAwardSort(iAward) == GE_SORT_HIGH ? statrat > playerrat : statrat < playerrat )
	{
		// Prevent divide by zero
		if ( statrat == 0 )
			statrat = 1;

		winner.idx = stats[0]->idx;
		winner.m_iStat = (GetAwardSort(iAward) == GE_SORT_HIGH ? statrat : 1/statrat) * 1000;  // 3 decimal precision
		winner.m_iAward = iAward;

		stats.PurgeAndDeleteElements();
		return true;
	}

	stats.PurgeAndDeleteElements();
	return false;
}

bool CGEStats::GetFavoriteWeapon( int iPlayer, GEWeaponSort &fav )
{
	if ( iPlayer < 0 || iPlayer >= m_pPlayerStats.Count() )
		return false;

	GEPlayerStats *stats = m_pPlayerStats[iPlayer];
	CBasePlayer *pPlayer = ToBasePlayer( m_pPlayerStats[iPlayer]->GetPlayer() );

	if ( !pPlayer || !stats )
		return false;

	// We want raw kills, NOT score
	int playerFrags = pPlayer->FragCount();
	float roundtime = gpGlobals->curtime - m_flRoundStartTime;
	// Avoid divide by zero!
	if ( roundtime == 0.0f )
		roundtime = 0.5f;

	CUtlVector<GEWeaponSort*> weapons;
	float killPercent;

	for ( int i=WEAPON_NONE+1; i < WEAPON_RANDOM; i++ ) // Go up to WEAPON_RANDOM to exclude tokens.
	{
		GEWeaponSort *entry = new GEWeaponSort;
		if ( GEGameplay()->IsInFinalIntermission() )
		{
			entry->m_iPercentage = stats->m_iMatchWeapons[i];
		}
		else
		{
			// Avoid divide by zero!
			if ( playerFrags == 0 )
				killPercent = 0.0f;
			else
				killPercent = (float)stats->m_iWeaponsKills[i] / (float)playerFrags;

			if ( roundtime < 1.0f )
				entry->m_iPercentage = 0;
			else
				entry->m_iPercentage = 100 * ( ((float)stats->m_iWeaponsHeldTime[i] / roundtime) + killPercent );
		}
		
		entry->m_iWeaponID = i;
		weapons.AddToTail( entry );
	}

	weapons.Sort( &CGEStats::WeaponSort );

	// Only give them a fav weapon if they have a percentage
	fav.m_iPercentage = weapons[0]->m_iPercentage;
	if ( fav.m_iPercentage )
		fav.m_iWeaponID = weapons[0]->m_iWeaponID;
	else
		fav.m_iWeaponID = WEAPON_NONE;

#ifdef _DEBUG
	DevMsg( "%s's weapon stats:\n", pPlayer->GetPlayerName() );
	DevMsg( "Frags: %i, Roundtime: %0.2f\n", playerFrags, roundtime );
	for ( int i=0; i < WEAPON_MAX-2; i++ )
	{
		if ( weapons[i]->m_iPercentage )
			DevMsg( "%s : %i\n", WeaponIDToAlias( weapons[i]->m_iWeaponID ), weapons[i]->m_iPercentage );
	}
	DevMsg( "Favorite Weapon: %s\n", WeaponIDToAlias( fav.m_iWeaponID ) );
#endif

	weapons.PurgeAndDeleteElements();

	return true;
}


#ifdef _DEBUG

CON_COMMAND( ge_showstats, "USAGE: ge_showstats [PLAYERID]\n Shows PLAYERID's current statistics" )
{
	if ( args.ArgC() < 2 )
		return;

	CGEPlayer *player = ToGEPlayer(UTIL_PlayerByIndex( atoi(args.Arg(1)) ));
	if ( !player )
		return;

	DevMsg( "Stats for %s:\n", player->GetPlayerName() );

	const char* ident;
	for ( int i=0; i < GE_AWARD_GIVEMAX; i++ )
	{
		ident = AwardIDToIdent( i );
		if ( ident )
			DevMsg( "%s: %i\n", ident, GEStats()->GetPlayerStat(i, player) );
	}
}

#endif