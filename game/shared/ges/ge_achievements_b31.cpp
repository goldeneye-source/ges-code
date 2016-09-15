///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: b314_achievements.cpp
// Description:
//      Goldeneye Achievements for Beta 3.1.4
//
// Created On: 01 Mar 09
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

// Who Throws a Show? (Oddjob Only)
class CAchWhoThrowsShoe : public CGEAchBaseAwardType
{
protected:
	virtual void Init()
	{
		CGEAchBaseAwardType::Init();

		SetCharReq( "oddjob" );
		AddAwardType( GE_AWARD_MOSTLYHARMLESS );
	}
};
DECLARE_GE_ACHIEVEMENT( CAchWhoThrowsShoe, ACHIEVEMENT_GES_WHO_THROWS_SHOE, "GES_WHO_THROWS_SHOE", 30, GE_ACH_UNLOCKED );

// Bond, James Bond (Bond ONLY)
class CAchJamesBond : public CGEAchBaseAwardType
{
protected:
	virtual void Init()
	{
		CGEAchBaseAwardType::Init();
		
		SetCharReq( "bond" );
		AddAwardType( GE_AWARD_PROFESSIONAL );
		AddAwardType( GE_AWARD_MARKSMANSHIP );
	}
};
DECLARE_GE_ACHIEVEMENT( CAchJamesBond, ACHIEVEMENT_GES_JAMESBOND, "GES_JAMESBOND", 30, GE_ACH_UNLOCKED );

// Hayday Mayday (as MayDay ONLY)
class CAchHaydayMayday : public CGEAchBaseAwardType
{
protected:
	virtual void Init()
	{
		CGEAchBaseAwardType::Init();

		SetCharReq( "mayday" );
		AddAwardType( GE_AWARD_FRANTIC );
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		CBasePlayer *pLocalPlayer = CBasePlayer::GetLocalPlayer();
		if ( !pLocalPlayer ) return;
		int localid = pLocalPlayer->entindex();

		if ( Q_strcmp( event->GetName(), "round_end" ) == 0 && event->GetBool("showreport") )
		{
			// We must also win the round, so filter out round loss events
			if ( !(event->GetInt("winnerid") == localid) )
			{
				// Make sure we can achieve this next round
				if ( IsChar() ) SetCharForRound( true );
				return;
			}
		}
		CGEAchBaseAwardType::FireGameEvent_Internal( event );
	}
};
DECLARE_GE_ACHIEVEMENT( CAchHaydayMayday, ACHIEVEMENT_GES_HAYDAYMAYDAY, "GES_HAYDAYMAYDAY", 30, GE_ACH_UNLOCKED );

// Mischeivious Mishkin (as Mishkin ONLY)
class CAchMischMishkin : public CGEAchBaseAwardType
{
protected:
	virtual void Init()
	{
		CGEAchBaseAwardType::Init();

		SetCharReq( "mishkin" );
		AddAwardType( GE_AWARD_DISHONORABLE );
	}
};
DECLARE_GE_ACHIEVEMENT( CAchMischMishkin, ACHIEVEMENT_GES_MISCHMISHKIN, "GES_MISCHMISHKIN", 30, GE_ACH_UNLOCKED );

// Two-Faced
class CAchTwoFaced : public CGEAchBaseAwardType
{
protected:
	virtual void Init()
	{
		CGEAchBaseAwardType::Init();
		SetGoal( 5 );

		AddAwardType( GE_AWARD_HONORABLE );
		AddAwardType( GE_AWARD_DISHONORABLE );
	}
};
DECLARE_GE_ACHIEVEMENT( CAchTwoFaced, ACHIEVEMENT_GES_TWO_FACED, "GES_TWO_FACED", 25, GE_ACH_UNLOCKED );

// Why Won't You Die?
class CAchAC10 : public CGEAchBaseAwardType
{
protected:
	virtual void Init()
	{
		CGEAchBaseAwardType::Init();
		SetGoal( 25 );

		AddAwardType( GE_AWARD_AC10 );
	}
};
DECLARE_GE_ACHIEVEMENT( CAchAC10, ACHIEVEMENT_GES_WHY_WONT_YOU_DIE, "GES_WHY_WONT_YOU_DIE", 20, GE_ACH_UNLOCKED );

// Viashinowned
class CAchViashinowned : public CGEAchievement
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
		if ( Q_strcmp( event->GetName(), "round_end" ) == 0 && event->GetBool("showreport") )
		{
			CBasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
			if (!pLocalPlayer) return;
			int localid = pLocalPlayer->entindex();

			// If we won 4 or more awards we win this achievement
			if ( GetAwardsForPlayer(event, localid) >= 4 && FindAwardForPlayer(event, GE_AWARD_DEADLY, localid)
				&& FindAwardForPlayer(event, GE_AWARD_PROFESSIONAL, localid) && FindAwardForPlayer(event, GE_AWARD_MARKSMANSHIP, localid)
				&& event->GetInt("winnerid") == localid && CalcPlayerCount() >= 4 )
				IncrementCount();
		}
	}
};
DECLARE_GE_ACHIEVEMENT( CAchViashinowned, ACHIEVEMENT_GES_VIASHINOWNED, "GES_VIASHINOWNED", 25, GE_ACH_UNLOCKED );

// The World is Not Enough
class CAchWorldIsNotEnough : public CGEAchievement
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
		if ( Q_strcmp( event->GetName(), "round_end" ) == 0 && event->GetBool("showreport") )
		{
			CBasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
			if (!pLocalPlayer) return;
			int localid = pLocalPlayer->entindex();

			// Win all 6 awards in 1 round to win this achievement
			if ( GetAwardsForPlayer(event, localid) == 6 && event->GetInt("winnerid") == localid && CalcPlayerCount() >= 4 )
				IncrementCount();
		}
	}
};
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchWorldIsNotEnough, ACHIEVEMENT_GES_WORLD_IS_NOT_ENOUGH, "GES_WORLD_IS_NOT_ENOUGH", 25, ACHIEVEMENT_GES_VIASHINOWNED );

// Secret Agent, Man
class CAchSecretAgentMan : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetAttackerFilter( "player" );
		SetVictimFilter( "player" );
		SetGoal( 1500 );
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
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchSecretAgentMan, ACHIEVEMENT_GES_SECRET_AGENT_MAN, "GES_SECRET_AGENT_MAN", 25, ACHIEVEMENT_GES_LICENSE_TO_KILL );

// GoldenEye Master
class CAchGoldeneyeMaster : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetAttackerFilter( "player" );
		SetVictimFilter( "player" );
		SetGoal( 3000 );
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
DECLARE_GE_DEPENDENT_ACHIEVEMENT( CAchGoldeneyeMaster, ACHIEVEMENT_GES_GOLDENEYE_MASTER, "GES_GOLDENEYE_MASTER", 50, ACHIEVEMENT_GES_SECRET_AGENT_MAN );

// You Should Be Honored
class CAchBeHonored : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(50);
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("player_death");
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
		if (!pPlayer)
			return;

		// If this isn't us doing the killing, we don't care
		if (event->GetInt("attacker") != pPlayer->GetUserID())
			return;

		// If we killed them with a skinned weapon, we did it!
		if (event->GetInt("weaponid") < WEAPON_SPAWNMAX && event->GetInt("weaponskin") > 0)
			IncrementCount();
	}
};
DECLARE_GE_ACHIEVEMENT( CAchBeHonored, ACHIEVEMENT_GES_BE_HONORED, "GES_BE_HONORED", 40, GE_ACH_UNLOCKED );

// You Should Be Grateful
class CAchBeGrateful : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 50 );
	}
	
	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		if ( !pVictim )
			return;

		if ( pAttacker == C_BasePlayer::GetLocalPlayer() && GEPlayerRes()->GetDevStatus( pVictim->entindex() ) )
			IncrementCount();
	}
};
DECLARE_GE_ACHIEVEMENT( CAchBeGrateful, ACHIEVEMENT_GES_BE_GRATEFUL, "GES_BE_GRATEFUL", 40, GE_ACH_UNLOCKED );

// Die Another Day
class CAchDieAnotherDay : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );
		SetGoal( 35 );
	}
	
	virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )
	{
		if ( !pAttacker ) return;
		if ( pAttacker->GetHealth() <= 32  )
			IncrementCount();
	}
};
DECLARE_GE_ACHIEVEMENT( CAchDieAnotherDay, ACHIEVEMENT_GES_DIE_ANOTHER_DAY, "GES_DIE_ANOTHER_DAY", 80, GE_ACH_UNLOCKED );

#endif