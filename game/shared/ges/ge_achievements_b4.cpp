///////////// Copyright © 2009, Goldeneye: Source. All rights reserved. /////////////
// 
// File: b4_achievements.cpp
// Description:
//      Goldeneye Achievements for Beta 4.0
//
// Created On: 01 Oct 09
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include "achievementmgr.h"
#include "util_shared.h"
#include "ge_achievement.h"

// Define our client's manager so that they can recieve updates when they achieve things
#ifdef CLIENT_DLL

#include "clientmode_shared.h"
#include "c_ge_player.h"
#include "c_gemp_player.h"
#include "c_ge_playerresource.h"
#include "c_ge_gameplayresource.h"
#include "ge_weapon.h"
#include "gemp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CREATE_ACH_WEPLEVEL( AchLTG, WEAPON_ROCKET_LAUNCHER, 1, 100 );
CREATE_ACH_WEPLEVEL( AchLTG, WEAPON_ROCKET_LAUNCHER, 2, 250 );
CREATE_ACH_WEPLEVEL( AchLTG, WEAPON_ROCKET_LAUNCHER, 3, 500 );

DECLARE_GE_ACHIEVEMENT( CAchLTGLevel1, ACHIEVEMENT_GES_LTG_LEVEL1, "GES_LTG_LEVEL1", 10, GE_ACH_UNLOCKED );
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchLTGLevel2, ACHIEVEMENT_GES_LTG_LEVEL2, "GES_LTG_LEVEL2", 15, ACHIEVEMENT_GES_LTG_LEVEL1 );
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchLTGLevel3, ACHIEVEMENT_GES_LTG_LEVEL3, "GES_LTG_LEVEL3", 20, ACHIEVEMENT_GES_LTG_LEVEL2 );

CREATE_ACH_WEPLEVEL( AchMoonraker, WEAPON_MOONRAKER, 1, 100 );
CREATE_ACH_WEPLEVEL( AchMoonraker, WEAPON_MOONRAKER, 2, 250 );
CREATE_ACH_WEPLEVEL( AchMoonraker, WEAPON_MOONRAKER, 3, 500 );

DECLARE_GE_ACHIEVEMENT( CAchMoonrakerLevel1, ACHIEVEMENT_GES_MOONRAKER_LEVEL1, "GES_MOONRAKER_LEVEL1", 10, GE_ACH_UNLOCKED );
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchMoonrakerLevel2, ACHIEVEMENT_GES_MOONRAKER_LEVEL2, "GES_MOONRAKER_LEVEL2", 15, ACHIEVEMENT_GES_MOONRAKER_LEVEL1 );
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchMoonrakerLevel3, ACHIEVEMENT_GES_MOONRAKER_LEVEL3, "GES_MOONRAKER_LEVEL3", 20, ACHIEVEMENT_GES_MOONRAKER_LEVEL2 );

CREATE_ACH_WEPLEVEL( AchKF7, WEAPON_KF7, 1, 100 );
CREATE_ACH_WEPLEVEL( AchKF7, WEAPON_KF7, 2, 500 );
CREATE_ACH_WEPLEVEL( AchKF7, WEAPON_KF7, 3, 750 );

DECLARE_GE_ACHIEVEMENT( CAchKF7Level1, ACHIEVEMENT_GES_KF7_LEVEL1, "GES_KF7_LEVEL1", 10, GE_ACH_UNLOCKED );
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchKF7Level2, ACHIEVEMENT_GES_KF7_LEVEL2, "GES_KF7_LEVEL2", 15, ACHIEVEMENT_GES_KF7_LEVEL1 );
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchKF7Level3, ACHIEVEMENT_GES_KF7_LEVEL3, "GES_KF7_LEVEL3", 20, ACHIEVEMENT_GES_KF7_LEVEL2 );

// BOOM, HEADSHOT!!!
class CAchBoomHeadshot : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 500 );
	}
	
	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		if ( event->GetBool("headshot")  )
			IncrementCount();
	}
};
DECLARE_GE_ACHIEVEMENT( CAchBoomHeadshot, ACHIEVEMENT_GES_BOOM_HEADSHOT, "GES_BOOM_HEADSHOT", 100, GE_ACH_UNLOCKED );

// Late Attendance
class CAchLateAttendance : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 5 );
	}
	
	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		if ( GEMPRules()->IsIntermission() )
			IncrementCount();
	}
};
DECLARE_GE_ACHIEVEMENT( CAchLateAttendance, ACHIEVEMENT_GES_LATE_ATTENDANCE, "GES_LATE_ATTENDANCE", 50, GE_ACH_UNLOCKED );

// Shaken, but Not Disturbed (LEVEL 1)
class CAchShakenLevel1 : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		VarInit();
	}
	virtual void VarInit()
	{
		m_iKillCount = 0;
		m_iKillsRqd = 5;
	}
	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_hurt" );
		ListenForGameEvent( "player_death" );
		ListenForGameEvent( "round_start" );
		VarInit();
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( !CBasePlayer::GetLocalPlayer() )
			return;

		if ( Q_strstr(event->GetName(), "player_") )
		{
			if ( event->GetInt("userid") == CBasePlayer::GetLocalPlayer()->GetUserID() )
				m_iKillCount = 0;
		}
		else if ( !Q_stricmp(event->GetName(), "round_start") )
		{
			m_iKillCount = 0;
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		if ( CalcPlayerCount() < 4 )
		{
			m_iKillCount = 0;
			return;
		}

		m_iKillCount++;

		if ( m_iKillCount >= m_iKillsRqd )
			IncrementCount();
	}

protected:
	int m_iKillsRqd;

private:
	int m_iKillCount;
};
DECLARE_GE_ACHIEVEMENT( CAchShakenLevel1, ACHIEVEMENT_GES_SHAKEN_LEVEL1, "GES_SHAKEN_LEVEL1", 20, GE_ACH_UNLOCKED );

// Shaken, but Not Disturbed (LEVEL 2)
class CAchShakenLevel2 : public CAchShakenLevel1
{
protected:
	virtual void VarInit()
	{
		CAchShakenLevel1::VarInit();
		m_iKillsRqd = 10;
	}
};
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchShakenLevel2, ACHIEVEMENT_GES_SHAKEN_LEVEL2, "GES_SHAKEN_LEVEL2", 40, ACHIEVEMENT_GES_SHAKEN_LEVEL1 );

// Shaken, but Not Disturbed (LEVEL 3)
class CAchShakenLevel3 : public CAchShakenLevel1
{
protected:
	virtual void VarInit()
	{
		CAchShakenLevel1::VarInit();
		m_iKillsRqd = 15;
	}
};
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchShakenLevel3, ACHIEVEMENT_GES_SHAKEN_LEVEL3, "GES_SHAKEN_LEVEL3", 80, ACHIEVEMENT_GES_SHAKEN_LEVEL2 );

// Scaramanga's Pride
class CAchScaramangasPride : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		VarInit();
	}
	virtual void VarInit()
	{
		m_iKillCount = 0;
		m_iKillsRqd = 20;
	}
	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_death" );
		ListenForGameEvent( "round_start" );
		VarInit();
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( !CBasePlayer::GetLocalPlayer() )
			return;

		if ( !Q_stricmp(event->GetName(), "player_death") )
		{
			if ( event->GetInt("userid") == CBasePlayer::GetLocalPlayer()->GetUserID() )
				m_iKillCount = 0;
		}
		else if ( !Q_stricmp(event->GetName(), "round_start") )
		{
			m_iKillCount = 0;
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		if ( CalcPlayerCount() < 4 )
		{
			m_iKillCount = 0;
			return;
		}

		if ( event->GetInt("weaponid") == WEAPON_GOLDENGUN )
		{
			if ( ++m_iKillCount >= m_iKillsRqd )
				IncrementCount();
		}
	}

private:
	int m_iKillsRqd;
	int m_iKillCount;
};
DECLARE_GE_ACHIEVEMENT( CAchScaramangasPride, ACHIEVEMENT_GES_SCARAMANGAS_PRIDE, "GES_SCARAMANGAS_PRIDE", 130, GE_ACH_UNLOCKED );

// The Man Who Cannot Die
class CAchManCantDie : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 10 );
		VarInit();
	}
	virtual void VarInit()
	{
		m_bHasDied = false;
	}
	virtual void ListenForEvents()
	{
		ListenForGameEvent( "player_death" );
		ListenForGameEvent( "player_team" );
		ListenForGameEvent( "round_start" );
		ListenForGameEvent( "round_end" );
		VarInit();
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
		if ( !pPlayer )
			return;

		if ( !Q_stricmp( event->GetName(), "player_death" ) )
		{
			if ( event->GetInt("userid") == pPlayer->GetUserID() )
				m_bHasDied = true;
		}
		else if ( !Q_stricmp(event->GetName(), "player_team") )
		{
			if ( event->GetInt("userid") == pPlayer->GetUserID() && event->GetInt("team") == TEAM_SPECTATOR )
				m_bHasDied = true;
		}
		else if ( !Q_stricmp(event->GetName(), "round_start") )
		{
			if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
				m_bHasDied = true;
			else
				m_bHasDied = false;
		}
		else if ( !Q_stricmp( event->GetName(), "round_end" ) && event->GetBool("showreport") )
		{
			// If we haven't died at all and the round was more than 4 minutes long and we scored at least 10 kills
			if ( !m_bHasDied && event->GetInt("roundlength") > 240 && g_PR->GetFrags(pPlayer->entindex()) >= 10 )
				IncrementCount();
		}
	}
private:
	bool m_bHasDied;
};
DECLARE_GE_ACHIEVEMENT( CAchManCantDie, ACHIEVEMENT_GES_MAN_WHO_CANT_DIE, "GES_MAN_WHO_CANT_DIE", 100, GE_ACH_UNLOCKED );

// Russian Roulette
class CAchRussianRoulette : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		VarInit();
	}
	virtual void VarInit()
	{
		m_iKillCount = 0;
	}
	virtual void ListenForEvents()
	{
		ListenForGameEvent( "weapon_event" );
		ListenForGameEvent( "player_spawn" );
		VarInit();
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( !CBasePlayer::GetLocalPlayer() )
			return;

		if ( event->GetInt("userid") != CBasePlayer::GetLocalPlayer()->GetUserID() )
			return;

		// Always reset the count when we spawn
		if ( !Q_stricmp(event->GetName(), "player_spawn") )
		{
			m_iKillCount = 0;
			return;
		}

		// Reset the kill count when we reload the magnum only (so we don't reset on weapon pickup)
		if ( event->GetInt("weaponid") == WEAPON_COUGAR_MAGNUM && event->GetInt("eventid") == WEAPON_EVENT_RELOAD )	
			m_iKillCount = 0;
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		if ( event->GetInt("weaponid") == WEAPON_COUGAR_MAGNUM )
		{
			m_iKillCount += 1;
			if ( m_iKillCount >= 6 )
				IncrementCount();
		}
	}

private:
	int m_iKillCount;
};
DECLARE_GE_ACHIEVEMENT( CAchRussianRoulette, ACHIEVEMENT_GES_RUSSIAN_ROULETTE, "GES_RUSSIAN_ROULETTE", 100, GE_ACH_UNLOCKED );

// The Specialist
class CAchTheSpecialist : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 1 );

		m_bExplosivesOnly = true;
	}
	virtual void ListenForEvents()
	{
		ListenForGameEvent( "round_start" );
		ListenForGameEvent( "round_end" );
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( !Q_stricmp(event->GetName(), "round_start") )
		{
			m_bExplosivesOnly = true;
		}
		else if ( !Q_stricmp(event->GetName(), "round_end") )
		{
			CBasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
			if (!pLocalPlayer || !event->GetBool("showreport"))
				return;

			int localid = pLocalPlayer->entindex();

			// If we made it the whole round only using explosives AND we won the round they get it!
			if ( localid == event->GetInt("winnerid") && m_bExplosivesOnly && CalcPlayerCount() >= 4 
				&& event->GetInt("roundlength") > 30 && GEPlayerRes()->GetFrags( localid ) >= 15 )
				IncrementCount();
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		// If the player used anything but explosives to kill someone they lose this achievement
		int weapid = event->GetInt("weaponid");
		if ( !(weapid == WEAPON_GRENADE || weapid == WEAPON_TIMEDMINE || weapid == WEAPON_REMOTEMINE || weapid == WEAPON_PROXIMITYMINE
			|| weapid == WEAPON_GRENADE_LAUNCHER || weapid == WEAPON_ROCKET_LAUNCHER) )
		{
			m_bExplosivesOnly = false;
		}
	}
private:
	bool m_bExplosivesOnly;
};
DECLARE_GE_ACHIEVEMENT( CAchTheSpecialist, ACHIEVEMENT_GES_THE_SPECIALIST, "GES_THE_SPECIALIST", 80, GE_ACH_UNLOCKED );

// 004
class CAch004 : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_LISTEN_KILL_EVENTS | ACH_SAVE_GLOBAL );
		SetAttackerFilter("player");
		SetVictimFilter("player");
		SetGoal( 25 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		// Only interested in suicides and not alone
		if ( pVictim != pAttacker || CalcPlayerCount() < 2 || pVictim != C_BasePlayer::GetLocalPlayer() )
			return;

		// If the player used anything but mines to kill themselves they lose this achievement
		if ( event->GetInt("weaponid") == WEAPON_TIMEDMINE || event->GetInt("weaponid") == WEAPON_REMOTEMINE || event->GetInt("weaponid") == WEAPON_PROXIMITYMINE )
			IncrementCount();
	}
};
DECLARE_GE_ACHIEVEMENT( CAch004, ACHIEVEMENT_GES_004, "GES_004", 20, GE_ACH_HIDDEN );

// For England, Alec
class CAchForEnglandAlec : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 20 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		// Only interested in mine kills
		int weapid = event->GetInt("weaponid");
		if ( weapid == WEAPON_TIMEDMINE || weapid == WEAPON_REMOTEMINE || weapid == WEAPON_PROXIMITYMINE )
		{
			// We must be BOND and they must be 006
			if ( !Q_stricmp(GEPlayerRes()->GetCharIdent(pVictim->entindex()),"006_mi6") && !Q_stricmp(GEPlayerRes()->GetCharIdent(pAttacker->entindex()),"bond") )
				IncrementCount();
		}
	}
};
DECLARE_GE_ACHIEVEMENT( CAchForEnglandAlec, ACHIEVEMENT_GES_FOR_ENGLAND_ALEC, "GES_FOR_ENGLAND_ALEC", 60, GE_ACH_UNLOCKED );

// Through the Fire and Flames
#define ACHFIREFLAMES_DMG_TIMEOUT 3.0f
class CAchThroughFireFlames : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 500 );
		VarInit();
	}
	virtual void VarInit()
	{
		m_flDmgExpTime = 0;
		m_iDmgTaken = 0;
	}
	virtual void ListenForEvents()
	{
		ListenForGameEvent("player_hurt");
		ListenForGameEvent("player_death");
		VarInit();
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
		if ( !pPlayer )
			return;

		// Reset conditions on death
		if ( Q_stricmp( event->GetName(), "player_death" ) == 0 )
		{
			if ( event->GetInt("userid") == pPlayer->GetUserID() )
			{
				m_flDmgExpTime = 0;
				m_iDmgTaken = 0;
			}

			return;
		}

		if ( event->GetInt("userid") == pPlayer->GetUserID() && event->GetInt("dmgtype") == DMG_BLAST )
		{
			// If we received damage past our expiration, clear our accumulator and set a new dmg expiration
			if ( gpGlobals->curtime > m_flDmgExpTime )
			{
				m_flDmgExpTime = gpGlobals->curtime + ACHFIREFLAMES_DMG_TIMEOUT;
				m_iDmgTaken = 0;
			}

			// Accumulate our damage amount
			m_iDmgTaken += event->GetInt("damage");
			
			// If our accumulated damage is more than a third our max health and we are still alive
			// give us some creds
			int healthThreshold = pPlayer->GetMaxHealth()*0.333f;
			int currHealth = event->GetInt("health");
			if ( m_iDmgTaken >= healthThreshold && currHealth > 0 )
			{
				IncrementCount();
				m_iDmgTaken = 0;
			}
		}
	}
private:
	float m_flDmgExpTime;
	int m_iDmgTaken;
};
DECLARE_GE_ACHIEVEMENT( CAchThroughFireFlames, ACHIEVEMENT_GES_THROUGH_FIRE_FLAMES, "GES_THROUGH_FIRE_FLAMES", 80, GE_ACH_UNLOCKED );

// Domino Effect
class CAchDominoEffect : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 5 );
		VarInit();
	}
	virtual void VarInit()
	{
		m_flLastKillTime = 0;
	}
	virtual void ListenForEvents()
	{
		ListenForGameEvent("player_death");
		VarInit();
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
		if ( !pPlayer )
			return;

		// Only care about our kills
		if ( event->GetInt("attacker") == pPlayer->GetUserID() )
		{
			if ( event->GetBool("penetrated") && gpGlobals->curtime <= m_flLastKillTime + 0.2f )
				IncrementCount();

			m_flLastKillTime = gpGlobals->curtime;
		}
	}

private:
	float m_flLastKillTime;
};
DECLARE_GE_ACHIEVEMENT( CAchDominoEffect, ACHIEVEMENT_GES_DOMINO_EFFECT, "GES_DOMINO_EFFECT", 180, GE_ACH_UNLOCKED );

// Last Agent Standing
class CAchLastAgentStanding : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 50 );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "round_end" );
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( !Q_stricmp(event->GetName(), "round_end") )
		{
			if ( !CBasePlayer::GetLocalPlayer() || !event->GetBool("showreport") || !IsScenario( "yolt" ) || CalcPlayerCount() < 4 )
				return;

			// If we won we get cred
			if ( event->GetInt("winnerid") == CBasePlayer::GetLocalPlayer()->entindex() )
				IncrementCount();
		}
	}
};
DECLARE_GE_ACHIEVEMENT( CAchLastAgentStanding, ACHIEVEMENT_GES_LAST_AGENT_STANDING, "GES_LAST_AGENT_STANDING", 100, GE_ACH_UNLOCKED );

// You Can't Win.
class CAchYouCantWin : public CGEAchBaseAwardType
{
protected:
	virtual void Init()
	{
		CGEAchBaseAwardType::Init();

		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );

		SetCharReq( "ourumov" );
		AddAwardType( GE_AWARD_GIVEMAX ); // Prevent us from getting this from the baseclass logic.

		m_bDD44Only = true;
	}

	virtual void ListenForEvents()
	{
		m_bDD44Only = true;

		ListenForGameEvent( "player_hurt" );
		ListenForGameEvent( "round_start" );
		CGEAchBaseAwardType::ListenForEvents();
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if ( !pPlayer )
			return;

		if ( !Q_stricmp(event->GetName(), "player_hurt") )
		{
			if ( pPlayer->GetUserID() == event->GetInt("attacker") && event->GetInt("userid") != event->GetInt("attacker") )
			{
				if ( event->GetInt("weaponid") != WEAPON_DD44 )
					m_bDD44Only = false;
			}
		}
		else if ( !Q_stricmp(event->GetName(), "round_end") )
		{
			if ( !event->GetBool("showreport") || !IsScenario( "yolt" ) )
				return;

			// If we won with DD44 only and with 4+ kills we get cred
			if ( event->GetInt("winnerid") == pPlayer->entindex() && WasCharForRound() 
					&& m_bDD44Only && g_PR->GetFrags(pPlayer->entindex()) >= 4 )
				IncrementCount();
		}
		else if (!Q_stricmp(event->GetName(), "round_start"))
			m_bDD44Only = true; //Reset our status on round start.

		CGEAchBaseAwardType::FireGameEvent_Internal( event );
	}

private:
	bool m_bDD44Only;
};
DECLARE_GE_ACHIEVEMENT( CAchYouCantWin, ACHIEVEMENT_GES_YOU_CANT_WIN, "GES_YOU_CANT_WIN", 100, GE_ACH_UNLOCKED );

// Just A Flesh Wound
#define ACH_FLESHWOUND_TIMEOUT 10.0f
class CAchJustAFleshWound : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 15 );
		VarInit();
	}
	virtual void VarInit()
	{
		m_flLastKnifeTime = 0;
		m_nLastVictim = 0;
		m_bInDuel = false;
	}
	virtual void ListenForEvents()
	{
		ListenForGameEvent("player_hurt");
		VarInit();
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
		if ( !pPlayer )
			return;

		// Only care about our attacks
		if ( event->GetInt("attacker") == pPlayer->GetUserID() )
		{
			// Reset our duel parameters if we passed our duel limit
			if ( gpGlobals->curtime > m_flLastKnifeTime + ACH_FLESHWOUND_TIMEOUT )
			{
				m_bInDuel = false;
				m_nLastVictim = 0;
			}

			if ( event->GetInt("weaponid") == WEAPON_KNIFE )
			{
				m_flLastKnifeTime = gpGlobals->curtime;
				m_nLastVictim = event->GetInt("userid");

				// Check to see if our victim also has a knife. If they do we are dueling!
				CBasePlayer *pVictim = USERID2PLAYER( event->GetInt("userid") );
				if ( pVictim && pVictim->GetActiveWeapon() )
				{
					if ( ToGEWeapon(pVictim->GetActiveWeapon())->GetWeaponID() == WEAPON_KNIFE )
						m_bInDuel = true;
					else
						m_bInDuel = false;
				}
			}
		}
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		// Only interested in knife kills
		if ( event->GetInt("weaponid") != WEAPON_KNIFE || !pVictim->IsPlayer() )
			return;

		if ( gpGlobals->curtime > m_flLastKnifeTime + ACH_FLESHWOUND_TIMEOUT )
			return;

		// We must be in a knife duel AND the victim must be our duelee
		if ( m_bInDuel && ToBasePlayer(pVictim)->GetUserID() == m_nLastVictim )
			IncrementCount();
	}

private:
	float m_flLastKnifeTime;
	int m_nLastVictim;
	bool m_bInDuel;
};
DECLARE_GE_ACHIEVEMENT( CAchJustAFleshWound, ACHIEVEMENT_GES_FLESH_WOUND, "GES_FLESH_WOUND", 180, GE_ACH_UNLOCKED );

// Make My Mayday
class CAchMakeMyMayday : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 50 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		// We have to be Mayday when we make the kill
		if ( Q_stricmp(GEPlayerRes()->GetCharIdent( pAttacker->entindex() ), "mayday") != 0 )
			return;

		// Only interested in magnum headshots
		if ( event->GetInt("weaponid") == WEAPON_COUGAR_MAGNUM && event->GetBool("headshot") )
			IncrementCount();
	}
};
DECLARE_GE_ACHIEVEMENT( CAchMakeMyMayday, ACHIEVEMENT_GES_MAKE_MY_MAYDAY, "GES_MAKE_MY_MAYDAY", 80, GE_ACH_UNLOCKED );

// That's My Little Octopussy
class CAchThatsMyOctopussy : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 25 );
	}

	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		if ( !pVictim->IsPlayer() )
			return;

		// Only interested in campers no matter how bad they are)
		if ( GEPlayerRes()->GetCampingPercent(pVictim->entindex()) > 0 )
			IncrementCount();
	}
};
DECLARE_GE_ACHIEVEMENT( CAchThatsMyOctopussy, ACHIEVEMENT_GES_THATS_MY_OCTOPUSSY, "GES_THATS_MY_OCTOPUSSY", 80, GE_ACH_UNLOCKED );

// Two Klobbs Don't Make It Right
#define ACH_TWOKLOBBS_TIMEOUT 5.0f
#define INVALID_ID -1
class CAchTwoKlobbs : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 20 );
		VarInit();
	}
	virtual void VarInit()
	{
		m_flLastKlobbTime = 0;
		m_nVictim = INVALID_ID;
		m_nHelper = INVALID_ID;
		m_bInAssist = false;
	}
	virtual void ListenForEvents()
	{
		ListenForGameEvent("player_hurt");
		ListenForGameEvent("player_death");
		VarInit();
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
		if ( !pPlayer )
			return;

		// This message must be about a Klobb and we have to have an active weapon
		if ( !pPlayer->GetActiveWeapon() || event->GetInt("weaponid") != WEAPON_KLOBB )
			return;

		int localid = pPlayer->GetUserID();

		// We must be holding the Klobb
		if ( ToGEWeapon(pPlayer->GetActiveWeapon())->GetWeaponID() == WEAPON_KLOBB )
		{
			if ( !Q_stricmp(event->GetName(), "player_hurt") )
			{
				// Reset our parameters if we timed out
				if ( gpGlobals->curtime > (m_flLastKlobbTime + ACH_TWOKLOBBS_TIMEOUT) )
				{
					m_nVictim = m_nHelper = INVALID_ID;
					m_bInAssist = false;
				}

				// We have filtered out all non-klobb attacks and we must be holding the klobb
				// get the information from the event
				int attacker = event->GetInt("attacker");
				int victim = event->GetInt("userid");

				// Set our current victim
				if ( attacker == localid )
				{
					m_nVictim = victim;
					m_flLastKlobbTime = gpGlobals->curtime;
				}
				else if ( m_nHelper == INVALID_ID && m_nVictim == victim )
				{
					// If another person is attacker our current victim they must be helping us
					m_nHelper = attacker;
				}

				// If this is an attack on our victim and we have a helper and either myself or they are attacking we 
				// are in an assist scenario
				if ( m_nHelper != INVALID_ID && (attacker == localid || attacker == m_nHelper) && m_nVictim == victim )
					m_bInAssist = true;
				else if ( attacker == localid || attacker == m_nHelper )
					m_bInAssist = false;
			}
			else if ( !Q_stricmp(event->GetName(), "player_death") )
			{
				if ( gpGlobals->curtime > (m_flLastKlobbTime + ACH_TWOKLOBBS_TIMEOUT) )
					return;

				// We must be in a assisting a fellow klobber and be attacking the same victim
				if ( m_bInAssist && event->GetInt("userid") == m_nVictim )
				{
					m_flLastKlobbTime = 0;
					m_nHelper = m_nVictim = INVALID_ID;
					m_bInAssist = false;

					IncrementCount();
				}
			}
		}
		else
		{
			m_flLastKlobbTime = 0;
			m_nHelper = m_nVictim = INVALID_ID;
			m_bInAssist = false;
		}
	}

private:
	float m_flLastKlobbTime;
	int m_nVictim;
	int m_nHelper;
	bool m_bInAssist;
};
DECLARE_GE_ACHIEVEMENT( CAchTwoKlobbs, ACHIEVEMENT_GES_TWO_KLOBBS, "GES_TWO_KLOBBS", 80, GE_ACH_UNLOCKED );

// Break a Leg
#define ACH_BREAKLEG_TIMEOUT 5.0f
#define ACH_BREAKLEG_RATIO 0.5f
class CAchBreakALeg : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 50 );
		VarInit();
	}
	virtual void VarInit()
	{
		m_iLegHits = 0;
		m_iTotHits = 0;
		m_nVictim = INVALID_ID;
		m_flTimeout = 0;
	}
	virtual void ListenForEvents()
	{
		ListenForGameEvent("player_hurt");
		VarInit();
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
		if ( !pPlayer )
			return;

		// Ignore suicide
		if ( event->GetInt("attacker") == event->GetInt("userid") )
			return;

		// Only care about our attacks (ignore blast damage)
		if ( event->GetInt("attacker") == pPlayer->GetUserID() && event->GetInt("dmgtype") != DMG_BLAST )
		{
			// Check if we have a new victim or we timed out
			if ( gpGlobals->curtime > m_flTimeout || m_nVictim != event->GetInt("userid") )
			{
				// This is a NEW victim
				m_nVictim = event->GetInt("userid");
				m_iTotHits = m_iLegHits = 0;
			}

			m_flTimeout = gpGlobals->curtime + ACH_BREAKLEG_TIMEOUT;

			// Check our hit group
			m_iTotHits++;
			if ( event->GetInt("hitgroup") == HITGROUP_LEFTLEG || event->GetInt("hitgroup") == HITGROUP_RIGHTLEG )
				m_iLegHits++;

			// Check for a death and award if we kept with all leg hits
			if ( event->GetInt("health") <= 0 )
			{
				float ratio = (float)m_iLegHits / (float)m_iTotHits;
				if ( m_iTotHits > 1 && ratio > ACH_BREAKLEG_RATIO )
					IncrementCount();

				m_nVictim = INVALID_ID;
				m_iTotHits = m_iLegHits = 0;
			}
		}
	}
private:
	int m_iLegHits;
	int m_iTotHits;
	int m_nVictim;
	float m_flTimeout;
};
DECLARE_GE_ACHIEVEMENT( CAchBreakALeg, ACHIEVEMENT_GES_BREAK_A_LEG, "GES_BREAK_A_LEG", 100, GE_ACH_UNLOCKED );

// Target Practice
class CAchTargetPractice : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "round_end" );
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( !Q_stricmp(event->GetName(), "round_end") )
		{
			CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
			if ( !pPlayer || !event->GetBool("showreport") || !IsScenario( "yolt" ) || CalcPlayerCount() < 4 )
				return;

			// If we won, got 4+ kills, and earned marksmanship get the cred
			if ( event->GetInt("winnerid") == pPlayer->entindex() && g_PR->GetFrags( pPlayer->entindex() ) >= 4 
					&& FindAwardForPlayer( event, GE_AWARD_MARKSMANSHIP, pPlayer->entindex() ) )
				IncrementCount();
		}
	}
};
DECLARE_GE_ACHIEVEMENT( CAchTargetPractice, ACHIEVEMENT_GES_TARGET_PRACTICE, "GES_TARGET_PRACTICE", 100, GE_ACH_UNLOCKED );

#endif // CLIENT_DLL
