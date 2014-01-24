///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: b3_achievements.cpp
// Description:
//      Goldeneye Achievements for Beta 3.0
//
// Created On: 04 Sep 08
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include "achievementmgr.h"
#include "util_shared.h"
#include "ge_achievement.h"

// Define our client's manager so that they can recieve updates when they achieve things
#ifdef CLIENT_DLL
		
#include "c_ge_player.h"
#include "c_gemp_player.h"
#include "c_ge_playerresource.h"
#include "ge_weapon.h"
#include "gemp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// This is the declaration of your achievement mgr, it gets registered
// with the engine so the variable name doesn't matter
CAchievementMgr		g_AchievementMgrGES;

// RCP 4 DEATH
CREATE_ACH_WEPLEVEL( AchRCP90, WEAPON_RCP90, 1, 100 );
CREATE_ACH_WEPLEVEL( AchRCP90, WEAPON_RCP90, 2, 250 );
CREATE_ACH_WEPLEVEL( AchRCP90, WEAPON_RCP90, 3, 500 );

DECLARE_GE_ACHIEVEMENT( CAchRCP90Level1, ACHIEVEMENT_GES_RCP90_LEVEL1, "GES_RCP90_LEVEL1", 10, GE_ACH_UNLOCKED );
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchRCP90Level2, ACHIEVEMENT_GES_RCP90_LEVEL2, "GES_RCP90_LEVEL2", 15, ACHIEVEMENT_GES_RCP90_LEVEL1 );
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchRCP90Level3, ACHIEVEMENT_GES_RCP90_LEVEL3, "GES_RCP90_LEVEL3", 20, ACHIEVEMENT_GES_RCP90_LEVEL2 );

// ARU33?
CREATE_ACH_WEPLEVEL( AchAR33, WEAPON_AR33, 1, 133 );
CREATE_ACH_WEPLEVEL( AchAR33, WEAPON_AR33, 2, 433 );
CREATE_ACH_WEPLEVEL( AchAR33, WEAPON_AR33, 3, 733 );

DECLARE_GE_ACHIEVEMENT( CAchAR33Level1, ACHIEVEMENT_GES_AR33_LEVEL1, "GES_AR33_LEVEL1", 10, GE_ACH_UNLOCKED );
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchAR33Level2, ACHIEVEMENT_GES_AR33_LEVEL2, "GES_AR33_LEVEL2", 15, ACHIEVEMENT_GES_AR33_LEVEL1 );
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchAR33Level3, ACHIEVEMENT_GES_AR33_LEVEL3, "GES_AR33_LEVEL3", 20, ACHIEVEMENT_GES_AR33_LEVEL2 );

// PKing Soup
CREATE_ACH_WEPLEVEL( AchPP7, WEAPON_PP7 || event->GetInt("weaponid") == WEAPON_PP7_SILENCED, 1, 100 );
CREATE_ACH_WEPLEVEL( AchPP7, WEAPON_PP7 || event->GetInt("weaponid") == WEAPON_PP7_SILENCED, 2, 250 );
CREATE_ACH_WEPLEVEL( AchPP7, WEAPON_PP7 || event->GetInt("weaponid") == WEAPON_PP7_SILENCED, 3, 500 );

DECLARE_GE_ACHIEVEMENT( CAchPP7Level1, ACHIEVEMENT_GES_PP7_LEVEL1, "GES_PP7_LEVEL1", 10, GE_ACH_UNLOCKED );
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchPP7Level2, ACHIEVEMENT_GES_PP7_LEVEL2, "GES_PP7_LEVEL2", 15, ACHIEVEMENT_GES_PP7_LEVEL1 );
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchPP7Level3, ACHIEVEMENT_GES_PP7_LEVEL3, "GES_PP7_LEVEL3", 20, ACHIEVEMENT_GES_PP7_LEVEL2 );

// A View To A Kill
CREATE_ACH_WEPLEVEL( AchSniperRifle, WEAPON_SNIPER_RIFLE, 1, 50 );
CREATE_ACH_WEPLEVEL( AchSniperRifle, WEAPON_SNIPER_RIFLE, 2, 150 );
CREATE_ACH_WEPLEVEL( AchSniperRifle, WEAPON_SNIPER_RIFLE, 3, 250 );

DECLARE_GE_ACHIEVEMENT( CAchSniperRifleLevel1, ACHIEVEMENT_GES_SNIPERRIFLE_LEVEL1, "GES_SNIPERRIFLE_LEVEL1", 10, GE_ACH_UNLOCKED );
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchSniperRifleLevel2, ACHIEVEMENT_GES_SNIPERRIFLE_LEVEL2, "GES_SNIPERRIFLE_LEVEL2", 15, ACHIEVEMENT_GES_SNIPERRIFLE_LEVEL1 );
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchSniperRifleLevel3, ACHIEVEMENT_GES_SNIPERRIFLE_LEVEL3, "GES_SNIPERRIFLE_LEVEL3", 20, ACHIEVEMENT_GES_SNIPERRIFLE_LEVEL2 );

// Phantastic Phantom
CREATE_ACH_WEPLEVEL( AchPhantom, WEAPON_PHANTOM, 1, 100 );
CREATE_ACH_WEPLEVEL( AchPhantom, WEAPON_PHANTOM, 2, 500 );
CREATE_ACH_WEPLEVEL( AchPhantom, WEAPON_PHANTOM, 3, 750 );

DECLARE_GE_ACHIEVEMENT( CAchPhantomLevel1, ACHIEVEMENT_GES_PHANTOM_LEVEL1, "GES_PHANTOM_LEVEL1", 10, GE_ACH_UNLOCKED );
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchPhantomLevel2, ACHIEVEMENT_GES_PHANTOM_LEVEL2, "GES_PHANTOM_LEVEL2", 15, ACHIEVEMENT_GES_PHANTOM_LEVEL1 );
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchPhantomLevel3, ACHIEVEMENT_GES_PHANTOM_LEVEL3, "GES_PHANTOM_LEVEL3", 20, ACHIEVEMENT_GES_PHANTOM_LEVEL2 );

// Klobbering Time
CREATE_ACH_WEPLEVEL( AchKlobb, WEAPON_KLOBB, 1, 25 );
CREATE_ACH_WEPLEVEL( AchKlobb, WEAPON_KLOBB, 2, 75 );
CREATE_ACH_WEPLEVEL( AchKlobb, WEAPON_KLOBB, 3, 125 );

DECLARE_GE_ACHIEVEMENT( CAchKlobbLevel1, ACHIEVEMENT_GES_KLOBB_LEVEL1, "GES_KLOBB_LEVEL1", 10, GE_ACH_UNLOCKED );
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchKlobbLevel2, ACHIEVEMENT_GES_KLOBB_LEVEL2, "GES_KLOBB_LEVEL2", 15, ACHIEVEMENT_GES_KLOBB_LEVEL1 );
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchKlobbLevel3, ACHIEVEMENT_GES_KLOBB_LEVEL3, "GES_KLOBB_LEVEL3", 20, ACHIEVEMENT_GES_KLOBB_LEVEL2 );

// Remotely Remove
CREATE_ACH_WEPLEVEL( AchRemoteMine, WEAPON_REMOTEMINE, 1, 50 );
CREATE_ACH_WEPLEVEL( AchRemoteMine, WEAPON_REMOTEMINE, 2, 150 );
CREATE_ACH_WEPLEVEL( AchRemoteMine, WEAPON_REMOTEMINE, 3, 250 );

DECLARE_GE_ACHIEVEMENT( CAchRemoteMineLevel1, ACHIEVEMENT_GES_REMOTEMINE_LEVEL1, "GES_REMOTEMINE_LEVEL1", 10, GE_ACH_UNLOCKED );
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchRemoteMineLevel2, ACHIEVEMENT_GES_REMOTEMINE_LEVEL2, "GES_REMOTEMINE_LEVEL2", 15, ACHIEVEMENT_GES_REMOTEMINE_LEVEL1 );
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchRemoteMineLevel3, ACHIEVEMENT_GES_REMOTEMINE_LEVEL3, "GES_REMOTEMINE_LEVEL3", 20, ACHIEVEMENT_GES_REMOTEMINE_LEVEL2 );

// Proximity Pulvarisor
CREATE_ACH_WEPLEVEL( AchProximityMine, WEAPON_PROXIMITYMINE, 1, 50 );
CREATE_ACH_WEPLEVEL( AchProximityMine, WEAPON_PROXIMITYMINE, 2, 150 );
CREATE_ACH_WEPLEVEL( AchProximityMine, WEAPON_PROXIMITYMINE, 3, 250 );

DECLARE_GE_ACHIEVEMENT( CAchProximityMineLevel1, ACHIEVEMENT_GES_PROXIMITYMINE_LEVEL1, "GES_PROXIMITYMINE_LEVEL1", 10, GE_ACH_UNLOCKED );
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchProximityMineLevel2, ACHIEVEMENT_GES_PROXIMITYMINE_LEVEL2, "GES_PROXIMITYMINE_LEVEL2", 15, ACHIEVEMENT_GES_PROXIMITYMINE_LEVEL1 );
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchProximityMineLevel3, ACHIEVEMENT_GES_PROXIMITYMINE_LEVEL3, "GES_PROXIMITYMINE_LEVEL3", 20, ACHIEVEMENT_GES_PROXIMITYMINE_LEVEL2 );

// Timed Termination
CREATE_ACH_WEPLEVEL( AchTimedMine, WEAPON_TIMEDMINE, 1, 50 );
CREATE_ACH_WEPLEVEL( AchTimedMine, WEAPON_TIMEDMINE, 2, 100 );
CREATE_ACH_WEPLEVEL( AchTimedMine, WEAPON_TIMEDMINE, 3, 150 );

DECLARE_GE_ACHIEVEMENT( CAchTimedMineLevel1, ACHIEVEMENT_GES_TIMEDMINE_LEVEL1, "GES_TIMEDMINE_LEVEL1", 10, GE_ACH_UNLOCKED );
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchTimedMineLevel2, ACHIEVEMENT_GES_TIMEDMINE_LEVEL2, "GES_TIMEDMINE_LEVEL2", 15, ACHIEVEMENT_GES_TIMEDMINE_LEVEL1 );
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchTimedMineLevel3, ACHIEVEMENT_GES_TIMEDMINE_LEVEL3, "GES_TIMEDMINE_LEVEL3", 20, ACHIEVEMENT_GES_TIMEDMINE_LEVEL2 );

// License To Kill
class CAchLicenseToKill : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetAttackerFilter( "player" );
		SetVictimFilter( "player" );
		SetGoal( 500 );
		VarInit();
	}

	virtual void VarInit()
	{
		m_iKillInterp = 0;
	}

	virtual void ListenForEvents()
	{
		VarInit();
	}
	
	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		m_iKillInterp++;
		if ( IsScenario( "ltk", false ) && m_iKillInterp % 3 )
			return;

		IncrementCount();
	}

private:
	int m_iKillInterp;
};
DECLARE_GE_ACHIEVEMENT( CAchLicenseToKill, ACHIEVEMENT_GES_LICENSE_TO_KILL, "GES_LICENSE_TO_KILL", 10, GE_ACH_UNLOCKED );

// SharpSh00tah (hidden)
class CAchSharpShootah : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetAttackerFilter( "player" );
		SetVictimFilter( "player" );
		SetGoal( 1 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		if ( event->GetInt("weaponid") == WEAPON_SNIPER_RIFLE )
		{
			// We must be > 170 ft away
			if ( pVictim->GetAbsOrigin().DistTo( pAttacker->GetAbsOrigin() ) > 2040.0f )
				IncrementCount();
		}
	}
};
DECLARE_GE_ACHIEVEMENT( CAchSharpShootah, ACHIEVEMENT_GES_SHARPSHOOTAH, "GES_SHARPSHOOTAH", 15, GE_ACH_HIDDEN );

// UFO'd (hidden)
class CAchUFOd : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetAttackerFilter( "player" );
		SetVictimFilter( "player" );
		SetGoal( 25 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		// Ignore suicides
		if ( pVictim == pAttacker )
			return;

		// Only give them this if the mine hasn't attached yet, custom stores the value for "m_bInAir"
		if ( event->GetInt("weaponid") == WEAPON_REMOTEMINE && event->GetBool("custom") )
				IncrementCount();
	}
};
DECLARE_GE_ACHIEVEMENT( CAchUFOd, ACHIEVEMENT_GES_UFOD, "GES_UFOD", 30, GE_ACH_HIDDEN );

// Octopussy (hidden)
#define ACH_OCTOPUSSY_POINTSREQ 10
class CAchOctopussy : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );

		m_iCampingPoints = 0;
	}
	virtual void ListenForEvents()
	{
		ListenForGameEvent( "round_start" );
		ListenForGameEvent( "round_end" );
		ListenForGameEvent( "player_death" );
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		C_GEMPPlayer *pLocalPlayer = ToGEMPPlayer(C_BasePlayer::GetLocalPlayer());
		if ( !pLocalPlayer )
			return;

		if ( Q_strcmp( event->GetName(), "round_start" ) == 0 )
		{
			m_iCampingPoints = 0;
		}
		else if ( Q_strcmp( event->GetName(), "round_end" ) == 0 )
		{
			if ( !event->GetBool("showreport") )
				return;

			m_iCampingPoints += CalcCampingPoints( GEPlayerRes()->GetCampingPercent(pLocalPlayer->entindex()) );

			// If we have more than 4 players AND the person earned enough camping points, he is an octopussy
			if ( CalcPlayerCount() >= 4 && m_iCampingPoints > ACH_OCTOPUSSY_POINTSREQ && event->GetInt("roundlength") > 120 )
				IncrementCount();
		}
		else if ( Q_stricmp( event->GetName(), "player_death" ) == 0 )
		{
			int localid = pLocalPlayer->GetUserID();

			// Check if we were camping during our kills / death
			if ( event->GetInt("attacker") != localid && event->GetInt("userid") != localid )
				return;
	
			// Rate our camping level based on the percentage of bad camping
			m_iCampingPoints += CalcCampingPoints( GEPlayerRes()->GetCampingPercent(pLocalPlayer->entindex()) );
		}
	}

private:
	int CalcCampingPoints( int percent )
	{
		if ( percent > 75 )
			return 3;
		else if ( percent > 25 )
			return 2;
		else if ( percent > 0 )
			return 1;
		else
			return 0;
	}

	float m_iCampingPoints;
};
DECLARE_GE_ACHIEVEMENT( CAchOctopussy, ACHIEVEMENT_GES_OCTOPUSSY, "GES_OCTOPUSSY", 20, true );

// Silent Assassin
class CAchSilentAssassin : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetAttackerFilter( "player" );
		SetVictimFilter( "player" );
		SetGoal( 1 );

		m_bMeleeOnly = true;
	}
	virtual void ListenForEvents()
	{
		ListenForGameEvent( "round_start" );
		ListenForGameEvent( "round_end" );
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( Q_strcmp( event->GetName(), "round_start" ) == 0 )
		{
			m_bMeleeOnly = true;
		}
		else if ( Q_strcmp( event->GetName(), "round_end" ) == 0 )
		{
			CBasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
			if (!pLocalPlayer || !event->GetBool("showreport")) return;
			int localid = pLocalPlayer->entindex();
			
			// If we made it the whole round without killing with a weapon AND we won the round they get it!
			if ( localid == event->GetInt("winnerid") && m_bMeleeOnly && CalcPlayerCount() >= 4 
				&& event->GetInt("roundlength") > 30 && GEPlayerRes()->GetFrags( localid ) > 0 )
				IncrementCount();
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		// Ignore suicide
		if ( pVictim == pAttacker )
			return;

		// If the player used anything but slappers to kill someone they lose this achievement
		if ( event->GetInt("weaponid") != WEAPON_SLAPPERS && event->GetInt("weaponid") != WEAPON_KNIFE )
				m_bMeleeOnly = false;
	}
private:
	bool m_bMeleeOnly;
};
DECLARE_GE_ACHIEVEMENT( CAchSilentAssassin, ACHIEVEMENT_GES_SILENT_ASSASSIN, "GES_SILENT_ASSASSIN", 30, GE_ACH_UNLOCKED );

// #1 With The Bullet
class CAchOneWithBullet : public CGEAchBaseAwardType
{
protected:
	virtual void Init()
	{
		CGEAchBaseAwardType::Init();

		AddAwardType( GE_AWARD_MARKSMANSHIP );
		AddAwardType( GE_AWARD_DEADLY );
	}
};
DECLARE_GE_ACHIEVEMENT( CAchOneWithBullet, ACHIEVEMENT_GES_ONE_WITH_BULLET, "GES_ONE_WITH_BULLET", 40, GE_ACH_UNLOCKED );

// Invincible (as Boris ONLY)
class CAchInvincible : public CGEAchBaseAwardType
{
protected:
	virtual void Init()
	{
		CGEAchBaseAwardType::Init();

		AddAwardType( GE_AWARD_DEADLY );
		AddAwardType( GE_AWARD_LONGIN );

		SetCharReq( "boris" );
	}
};
DECLARE_GE_ACHIEVEMENT( CAchInvincible, ACHIEVEMENT_GES_INVINCIBLE, "GES_INVINCIBLE", 30, GE_ACH_UNLOCKED );

// For England, James (hidden)
class CAchForEngland : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_LISTEN_KILL_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 10 );
		VarInit();
	}
	virtual void VarInit() {
		m_flLastGrenadeKill = 0;
		m_flLastGrenadeSuicide = 0;
	}
	virtual void ListenForEvents()
	{
		VarInit();
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		// Grenades ONLY!
		// Only worry about local player suicide or if they are the attacker
		if ( event->GetInt("weaponid") == WEAPON_GRENADE && ((pVictim == pAttacker && pVictim == CBasePlayer::GetLocalPlayer()) || pAttacker == CBasePlayer::GetLocalPlayer()) )
		{
			// Ok, so we had a suicide AND we had a 
			float kDiff = gpGlobals->curtime - m_flLastGrenadeKill;
			float sDiff = gpGlobals->curtime - m_flLastGrenadeSuicide;

			if ( kDiff >= 0.0f && kDiff < 0.5f )
			{
				// We have a very recent (0.5 seconds) KILL with a grenade!
				// This report better be my suicide report to give me credit!
				if ( pVictim == pAttacker ) {
					IncrementCount();
					return;
				}
			}

			if ( sDiff >= 0.0f && sDiff < 0.5f )
			{
				// We have a very recent SUICIDE with a grenade!
				// this has to be the other person's death report
				IncrementCount();
				return;
			}

			// If Suicide is reported record the time
			if ( pVictim == pAttacker ) {
				m_flLastGrenadeSuicide = gpGlobals->curtime;
				return;
			}

			// Otherwise the other person kill
			m_flLastGrenadeKill = gpGlobals->curtime;
		}
	}

private:
	float m_flLastGrenadeKill;
	float m_flLastGrenadeSuicide;
};
DECLARE_GE_ACHIEVEMENT( CAchForEngland, ACHIEVEMENT_GES_FORENGLAND, "GES_FORENGLAND", 30, GE_ACH_HIDDEN );

// Dutch Rudder
class CAchDutchRudder : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_LISTEN_KILL_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		VarInit();
	}
	virtual void VarInit() {
		m_flLastSlapperDeath = 0;
		m_pLastKilledBy = NULL;
	}
	virtual void ListenForEvents()
	{
		VarInit();
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		CBasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( !pLocalPlayer || !( pLocalPlayer == pVictim || pLocalPlayer == pAttacker ) || pVictim == pAttacker ) 
			return;

		// Slappers ONLY!
		// We only get credit when we revenge kill!
		if ( event->GetInt("weaponid") == WEAPON_SLAPPERS )
		{
			// Find the time differences in our last killed WITH slappers
			float dDiff = gpGlobals->curtime - m_flLastSlapperDeath;

			if ( dDiff >= 0.0f && dDiff < 20.0f && m_pLastKilledBy == pVictim )
			{
				// We have a recent death by slappers and we revenged our kill!
				IncrementCount();
				return;
			}

			// If death is reported, record the time
			if ( pVictim == pLocalPlayer )
			{
				m_pLastKilledBy = pAttacker;
				m_flLastSlapperDeath = gpGlobals->curtime;
				return;
			}
		}
	}

private:
	float m_flLastSlapperDeath;
	CBaseEntity *m_pLastKilledBy;
};
DECLARE_GE_ACHIEVEMENT( CAchDutchRudder, ACHIEVEMENT_GES_DUTCHRUDDER, "GES_DUTCHRUDDER", 30, GE_ACH_UNLOCKED );

// Gold Fingered
class CAchGoldFingered : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		CBasePlayer *pVicPlayer = ToBasePlayer( pVictim );
		if ( !pVicPlayer || pVictim == pAttacker )
			return;

		// Slappers ONLY!
		if ( event->GetInt("weaponid") == WEAPON_SLAPPERS )
		{
			// Find out if the victim had the golden gun out
			CGEWeapon *pVicWeapon = (CGEWeapon*)pVicPlayer->GetActiveWeapon();
			if ( !pVicWeapon )
				return;

			if ( pVicWeapon->GetWeaponID() == WEAPON_GOLDENGUN )
				IncrementCount();
		}
	}
};
DECLARE_GE_ACHIEVEMENT( CAchGoldFingered, ACHIEVEMENT_GES_GOLDFINGERED, "GES_GOLDFINGERED", 30, GE_ACH_UNLOCKED );

#endif