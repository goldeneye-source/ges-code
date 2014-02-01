/////////////  Copyright � 2008, Goldeneye: Source. All rights reserved.  /////////////
// 
// ge_stats_recorder.h
//
// Description:
//      This code captures all events that occur on the server and translates them into
//		award points. This is a passive function until called upon by GELuaGameplay to
//		give the results at the end of a round.
//
// Created On: 20 Dec 08
// Creator: KillerMonkey (started by David)
////////////////////////////////////////////////////////////////////////////////

#ifndef GE_STATS_H
#define GE_STATS_H

#include "ge_gamerules.h"
#include "ge_gameplay.h"
#include "ge_player.h"
#include "ge_shareddefs.h"
#include "gamestats.h"

extern CGEPlayer *ToGEPlayer( CBaseEntity *pEntity );

class GEPlayerStats
{
public:
	GEPlayerStats( CBasePlayer *player );

	void SetPlayer( CBasePlayer *player ) { m_hPlayer = player; };
	CGEPlayer *GetPlayer( void ) { return ToGEPlayer( m_hPlayer.Get() ); };

	void AddStat( int idx, int amt );
	void SetStat( int idx, int amt );

	int GetRoundStat( int idx );
	int GetMatchStat( int idx );

	void ResetRoundStats( void );
	void AddRoundToMatchStats( void );
	void ResetMatchStats( void );

	int m_iDeaths; // Used to keep track of ACTUAL deaths

	// Keep track of weapon usage
	int m_iWeaponsHeldTime[ WEAPON_MAX ];
	int m_iWeaponsKills[ WEAPON_MAX ];
	// Stores cumulative results for preferred weapon over rounds
	int m_iMatchWeapons[ WEAPON_MAX ];

private:
	int m_iRoundStats[ GE_AWARD_MAX ];
	int m_iMatchStats[ GE_AWARD_MAX ];

	EHANDLE m_hPlayer;
};

// This is used to build a new vector for sorting out the winner of a
// certain stat (for speedier sorts)
struct GEStatSort
{
	int idx;
	int m_iStat;
	int m_iAward;
};

struct GEWeaponSort
{
	int m_iPercentage;
	int m_iWeaponID;
};


class CGEStats : public CBaseGameStats
{
public:
	typedef CBaseGameStats BaseClass;

	CGEStats();
	~CGEStats();

	void ResetRoundStats( void );
	void ResetMatchStats( void );
	void ResetPlayerStats( CBasePlayer *player );
	int GetPlayerStat( int idx, CBasePlayer *player );

	void SetAwardsInEvent( IGameEvent* pEvent );
	void SetFavoriteWeapons( void );

	virtual void Event_PlayerConnected( CBasePlayer *pBasePlayer );
	virtual void Event_PlayerDisconnected( CBasePlayer *pBasePlayer );

	virtual void Event_PlayerKilled( CBasePlayer *pPlayer, const CTakeDamageInfo &info );
	virtual void Event_PlayerKilledOther( CBasePlayer *pAttacker, CBaseEntity *pVictim, const CTakeDamageInfo &info );

	virtual void Event_WeaponFired( CBasePlayer *pShooter, bool bPrimary, char const *pchWeaponName );
	virtual void Event_WeaponHit( CBasePlayer *pShooter, bool bPrimary, char const *pchWeaponName, const CTakeDamageInfo &info );

	virtual void Event_PlayerDamage( CBasePlayer *pBasePlayer, const CTakeDamageInfo &info );
	virtual void Event_PlayerTraveled( CBasePlayer *pBasePlayer, float distanceInInches, bool bInVehicle, bool bSprinting );
	virtual void Event_WeaponSwitch( CBasePlayer *player, CBaseCombatWeapon *currWeapon, CBaseCombatWeapon *nextWeapon );
	virtual void Event_PickedArmor( CBasePlayer *pBasePlayer, int amt );
	virtual void Event_PickedAmmo( CBasePlayer *pBasePlayer, int amt );

	static int StatSortHigh( GEStatSort* const *a, GEStatSort* const *b ) { return ( (*b)->m_iStat - (*a)->m_iStat ); };
	static int StatSortLow( GEStatSort* const *a, GEStatSort* const *b ) { return ( (*a)->m_iStat - (*b)->m_iStat ); };
	static int WeaponSort( GEWeaponSort* const *a, GEWeaponSort* const *b ) { return ( (*b)->m_iPercentage - (*a)->m_iPercentage ); };

private:
	CUtlVector<GEPlayerStats*> m_pPlayerStats;
	int m_iRoundCount;
	float m_flRoundStartTime;

	bool GetAwardWinner( int iAward, GEStatSort &winner );
	bool GetFavoriteWeapon( int iPlayer, GEWeaponSort &fav );

	GEPlayerStats *FindPlayerStats( CBasePlayer *player );
	int FindPlayer( CBasePlayer *player );
};

inline CGEStats *GEStats()
{
	return (CGEStats*)gamestats;
}

#endif // GE_STATS_H
