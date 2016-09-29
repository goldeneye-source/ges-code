///////////// Copyright © 2016, Goldeneye: Source. All rights reserved. /////////////
// 
// File: v5_achievements.cpp
// Description:
//      Goldeneye Achievements for Version 5.0
//
// Created On: 01 Oct 09
// Check github for contributors.
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

// Double Down:  Kill 2 players with one shotgun blast.
class CAchDoubleDown : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(5);
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

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
		if (!pPlayer)
			return;

		// Only care about our kills
		if ( event->GetInt("attacker") == pPlayer->GetUserID() && event->GetInt("weaponid") == WEAPON_SHOTGUN )
		{
			if (gpGlobals->curtime <= m_flLastKillTime + 0.2f)
			{
				// If it's not LTK just give it to them instantly.
				if (!IsScenario("ltk"))
				{
					SetCount(GetGoal() - 1);
					IncrementCount();
				}
				else
					IncrementCount();
			}

			m_flLastKillTime = gpGlobals->curtime;
		}
	}

private:
	float m_flLastKillTime;
};
DECLARE_GE_ACHIEVEMENT(CAchDoubleDown, ACHIEVEMENT_GES_DOUBLE_DOWN, "GES_DOUBLE_DOWN", 100, GE_ACH_UNLOCKED);

// There's The Armor:  Take 8 gauges of damage in a single life
class CAchTheresTheArmor : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		VarInit();
	}
	virtual void VarInit()
	{
		m_iDmgTaken = 0;
	}
	virtual void ListenForEvents()
	{
		ListenForGameEvent("player_hurt");
		ListenForGameEvent("player_spawn");
		VarInit();
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
		if (!pPlayer)
			return;

		// If this isn't us getting hurt we don't care
		if (event->GetInt("userid") != pPlayer->GetUserID())
			return;

		// Reset conditions on spawn because only losers die and this acheivement is for winners.
		if (Q_stricmp(event->GetName(), "player_spawn") == 0)
		{
			m_iDmgTaken = 0;
			return;
		}

		// If we made it this far then the event is just the damage event, so we should add the damage we took to our total.
		// Provided that we're still alive, anyway.
		if (event->GetInt("health") > 0)
		{
			m_iDmgTaken += event->GetInt("damage");

			if (m_iDmgTaken >= 1280) //160 * 8 = 1280
				IncrementCount();
		}
		else
			m_iDmgTaken = 0;

	}
private:
	int m_iDmgTaken;
};
DECLARE_GE_ACHIEVEMENT(CAchTheresTheArmor, ACHIEVEMENT_GES_THERES_THE_ARMOR, "GES_THERES_THE_ARMOR", 100, GE_ACH_UNLOCKED);

// Remote Delivery:  Kill a player that you can't see with remote mines
class CAchRemoteDelivery : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(25);
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

		// If this isn't us doing the killing we don't care
		if (event->GetInt("attacker") != pPlayer->GetUserID())
			return;

		// If we're using the remote mines and the victim isn't within our PVS, we did it!
		if (event->GetInt("weaponid") == WEAPON_REMOTEMINE && !event->GetBool("InPVS"))
			IncrementCount();
	}
};
DECLARE_GE_ACHIEVEMENT(CAchRemoteDelivery, ACHIEVEMENT_GES_REMOTE_DELIVERY, "GES_REMOTE_DELIVERY", 100, GE_ACH_UNLOCKED);

// You're Fired:  Kill a player with a direct rocket launcher hit
class CAchYoureFired : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(100);
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

		// If this isn't us doing the killing we don't care
		if (event->GetInt("attacker") != pPlayer->GetUserID())
			return;

		// If we're using the rocket launcher and the victim registered a direct hit, we did it!
		if (event->GetInt("weaponid") == WEAPON_ROCKET_LAUNCHER && event->GetBool("headshot"))
			IncrementCount();
	}
};
DECLARE_GE_ACHIEVEMENT(CAchYoureFired, ACHIEVEMENT_GES_YOURE_FIRED, "GES_YOURE_FIRED", 100, GE_ACH_UNLOCKED);


// Shell Shocked:  Kill a player with a direct grenade launcher hit
class CAchShellShocked : public CGEAchievement
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

		// If this isn't us doing the killing we don't care
		if (event->GetInt("attacker") != pPlayer->GetUserID())
			return;

		// If we're using the grenade launcher, the victim isn't within our PVS, and the game registered a headshot, we did it!
		if (event->GetInt("weaponid") == WEAPON_GRENADE_LAUNCHER && event->GetBool("headshot"))
			IncrementCount();
	}
};
DECLARE_GE_ACHIEVEMENT(CAchShellShocked, ACHIEVEMENT_GES_SHELL_SHOCKED, "GES_SHELL_SHOCKED", 100, GE_ACH_UNLOCKED);


// Dart Board:  Kill a player from over 60 feet away with a throwing knife
class CAchDartBoard : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(5);
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

		// If this isn't us doing the killing we don't care
		if (event->GetInt("attacker") != pPlayer->GetUserID())
			return;

		// If we're using the throwing knives, and the victim is over 60 feet(720 inches) away, we did it!
		if (event->GetInt("weaponid") == WEAPON_KNIFE_THROWING && event->GetInt("dist") > 720)
			IncrementCount();
	}
};
DECLARE_GE_ACHIEVEMENT(CAchDartBoard, ACHIEVEMENT_GES_DART_BOARD, "GES_DART_BOARD", 100, GE_ACH_UNLOCKED);


// Precision Bombing:  Kill a player from over 100 feet away with a grenade launcher direct hit
class CAchPrecisionBombing : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(3);
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

		// If this isn't us doing the killing we don't care
		if (event->GetInt("attacker") != pPlayer->GetUserID())
			return;

		// If we're using the grenade launcher, got a direct hit, and the victim is over 100 feet(1200 inches) away, we did it!
		if (event->GetInt("weaponid") == WEAPON_GRENADE_LAUNCHER && event->GetBool("headshot") && event->GetInt("dist") > 1200)
			IncrementCount();
	}
};
DECLARE_GE_ACHIEVEMENT(CAchPrecisionBombing, ACHIEVEMENT_GES_PRECISION_BOMBING, "GES_PRECISION_BOMBING", 100, GE_ACH_UNLOCKED);


// Indirect Fire:  Kill a player you cannot see with a direct grenade launcher hit
class CAchIndirectFire : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(3);
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

		// If this isn't us doing the killing we don't care
		if (event->GetInt("attacker") != pPlayer->GetUserID())
			return;

		// If we're using the grenade launcher, the victim isn't within our PVS, and the game registered a headshot, we did it!
		if (event->GetInt("weaponid") == WEAPON_GRENADE_LAUNCHER && event->GetBool("headshot") && !event->GetBool("InPVS"))
			IncrementCount();
	}
};
DECLARE_GE_ACHIEVEMENT(CAchIndirectFire, ACHIEVEMENT_GES_INDIRECT_FIRE, "GES_INDIRECT_FIRE", 100, GE_ACH_UNLOCKED);


// Hunting Party:  Get point blank shotgun kills
class CAchHuntingParty : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(100);
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

		// If this isn't us doing the killing we don't care
		if (event->GetInt("attacker") != pPlayer->GetUserID())
			return;

		// If we're using the shotgun, and the victim is less than 8 feet(96 inches) away, we did it!
		if (event->GetInt("weaponid") == WEAPON_SHOTGUN && event->GetInt("dist") < 60)
			IncrementCount();
	}
};
DECLARE_GE_ACHIEVEMENT(CAchHuntingParty, ACHIEVEMENT_GES_HUNTING_PARTY, "GES_HUNTING_PARTY", 100, GE_ACH_UNLOCKED);


// Plastique Surgery:  Get envionmental explosive kills
class CAchPlastiqueSurgery : public CGEAchievement
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

		// If this isn't us doing the killing or we killed ourselves, we don't care
		if (event->GetInt("attacker") != pPlayer->GetUserID() || event->GetInt("attacker") == event->GetInt("userid"))
			return;

		// If we killed them with an envionmental explosion, we did it!
		if (event->GetInt("weaponid") == WEAPON_EXPLOSION)
			IncrementCount();
	}
};
DECLARE_GE_ACHIEVEMENT(CAchPlastiqueSurgery, ACHIEVEMENT_GES_PLASTIQUE_SURGERY, "GES_PLASTIQUE_SURGERY", 100, GE_ACH_UNLOCKED);


// Devide and Conquer:  Kill a developer with the developer klobb
class CAchDevideAndConquer : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL);
		SetGoal(1);
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("player_death");
	}

	virtual void Event_EntityKilled(CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event)
	{
		if (!pVictim)
			return;

		// If the attacker is us, we're using the developer klobb, and the victim is a developer, we did it!
		if (pAttacker == C_BasePlayer::GetLocalPlayer() && event->GetInt("weaponid") == WEAPON_KLOBB && event->GetInt("weaponskin") == 3 && GEPlayerRes()->GetDevStatus(pVictim->entindex()) == 1)
			IncrementCount();
	}
};
DECLARE_GE_ACHIEVEMENT(CAchDevideAndConquer, ACHIEVEMENT_GES_DEVIDE_AND_CONQUER, "GES_DEVIDE_AND_CONQUER", 100, GE_ACH_UNLOCKED);


// Devidends:  Get 20 kills with the developer or BT klobb
class CAchDevidends : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(20);
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

		// If we killed them with the klobb, and the klobb has one of the dev skins, we did it!
		if (event->GetInt("weaponid") == WEAPON_KLOBB && event->GetInt("weaponskin") > 0)
			IncrementCount();
	}
};
DECLARE_GE_ACHIEVEMENT(CAchDevidends, ACHIEVEMENT_GES_DEVIDENDS, "GES_DEVIDENDS", 100, GE_ACH_UNLOCKED);

// Proxima Centauri:  Get proximity mine kills from 100 feet away
class CAchProximaCentauri : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(10);
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

		// If this isn't us doing the killing we don't care
		if (event->GetInt("attacker") != pPlayer->GetUserID())
			return;

		// If we're using the proximity mines, and the victim is less than 100 feet(1200 inches) away, we did it!
		if (event->GetInt("weaponid") == WEAPON_PROXIMITYMINE && event->GetInt("dist") > 1200)
			IncrementCount();
	}
};
DECLARE_GE_ACHIEVEMENT(CAchProximaCentauri, ACHIEVEMENT_GES_PROXIMA_CENTAURI, "GES_PROXIMA_CENTAURI", 100, GE_ACH_UNLOCKED);


// Time To Target:  Get 2 grenade launcher direct hits within a tenth of a second
class CAchTimeToTarget : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
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

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
		if (!pPlayer)
			return;

		// If this isn't us doing the killing we don't care
		if (event->GetInt("attacker") != pPlayer->GetUserID())
			return;

		// If we're using the grenade launcher, and the game registered a headshot, we can consider this a kill.
		if (event->GetInt("weaponid") == WEAPON_GRENADE_LAUNCHER && event->GetBool("headshot"))
		{
			if (gpGlobals->curtime - m_flLastKillTime < 0.1 && CalcPlayerCount() >= 4)
				IncrementCount();
			else
				m_flLastKillTime = gpGlobals->curtime;
		}
	}
private:
	float m_flLastKillTime;
};
DECLARE_GE_ACHIEVEMENT(CAchTimeToTarget, ACHIEVEMENT_GES_TIMETOTARGET, "GES_TIMETOTARGET", 100, GE_ACH_UNLOCKED);


// Pitfall: Knock 100 people into pits
class CAchPitfall : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(100);
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

		// If this isn't us doing the killing we don't care
		if (event->GetInt("attacker") != pPlayer->GetUserID())
			return;

		// If the victim died to a trap, we did it.
		if ( event->GetInt("weaponid") == WEAPON_TRAP )
			IncrementCount();
	}
};
DECLARE_GE_ACHIEVEMENT(CAchPitfall, ACHIEVEMENT_GES_PITFALL, "GES_PITFALL", 100, GE_ACH_UNLOCKED);


// Backtracking: Steal 100 levels in Arsenal
class CAchBacktracker : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(100);
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("gameplay_event");
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		if (!Q_stricmp(event->GetString("name"), "ar_levelsteal"))
		{
			CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
			if (!pPlayer)
				return;

			// If we're the one that triggered the event, we did it!
			if (Q_atoi(event->GetString("value1")) == pPlayer->GetUserID() && IsScenario("arsenal"))
				IncrementCount();
		}
	}
};
DECLARE_GE_ACHIEVEMENT(CAchBacktracker, ACHIEVEMENT_GES_BACKTRACKER, "GES_BACKTRACKER", 100, GE_ACH_UNLOCKED);


// One Man Arsenal: Win a round of Arsenal with only the slappers
class CAchOneManArsenal : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		VarInit();
	}

	virtual void VarInit()
	{
		m_bOnlySlappers = true;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("player_death");
		ListenForGameEvent("gameplay_event");
		ListenForGameEvent("round_start");
		VarInit();
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
		if (!pPlayer)
			return;

		if (!Q_stricmp(event->GetName(), "player_death"))
		{
			// If this isn't us doing the killing we don't care
			if (event->GetInt("attacker") != pPlayer->GetUserID())
				return;

			// If the victim died to a trap, we did it.
			if (event->GetInt("weaponid") != WEAPON_SLAPPERS)
				m_bOnlySlappers = false;
		}

		else if (!Q_stricmp(event->GetName(), "gameplay_event"))
		{
			if (!Q_stricmp(event->GetString("name"), "ar_completedarsenal"))
			{
				// If we completed the arsenal (no force round end victories), there are more than 3 players, and we only used slappers, we did it!
				if (Q_atoi(event->GetString("value1")) == pPlayer->GetUserID() && m_bOnlySlappers && CalcPlayerCount() > 3 && IsScenario("arsenal"))
					IncrementCount();
			}
		}
		else if (!Q_stricmp(event->GetName(), "round_start"))
			m_bOnlySlappers = true; //Reset our status on round start.
	}
private:
	bool m_bOnlySlappers;
};
DECLARE_GE_ACHIEVEMENT(CAchOneManArsenal, ACHIEVEMENT_GES_ONEMANARSENAL, "GES_ONEMANARSENAL", 100, GE_ACH_UNLOCKED);


// Pineapple Express: Kill 10 people with grenades you dropped on death
class CAchPineappleExpress : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(10);
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

		// If this isn't us doing the killing we don't care
		if (event->GetInt("attacker") != pPlayer->GetUserID())
			return;

		// If the kill was with a grenade, and it has the droppedondeath bitflag set, we did it!
		if (event->GetInt("weaponid") == WEAPON_GRENADE && event->GetInt("custom") & 2)
			IncrementCount();
	}
};
DECLARE_GE_ACHIEVEMENT(CAchPineappleExpress, ACHIEVEMENT_GES_PINEAPPLE_EXPRESS, "GES_PINEAPPLE_EXPRESS", 100, GE_ACH_HIDDEN);


// Pineapple Express: Kill 10 people with grenades you dropped on death
class CAchPinpointPrecision : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(10);
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

		// If this isn't us doing the killing or we killed ourselves we don't care
		if (event->GetInt("attacker") != pPlayer->GetUserID() || event->GetInt("attacker") == event->GetInt("userid"))
			return;

		// If the kill was with a grenade, and it has the inair bitflag set, we did it!
		if (event->GetInt("weaponid") == WEAPON_GRENADE && event->GetInt("custom") & 1)
			IncrementCount();
	}
};
DECLARE_GE_ACHIEVEMENT(CAchPinpointPrecision, ACHIEVEMENT_GES_PINPOINT_PRECISION, "GES_PINPOINT_PRECISION", 100, GE_ACH_UNLOCKED);


// Chopping Block: Eliminate 20 people with slappers in YOLT
class CAchChoppingBlock : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(20);
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("gameplay_event");
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		if (!Q_stricmp(event->GetString("name"), "yolt_eliminated"))
		{
			CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
			if (!pPlayer)
				return;

			// If we're the one that triggered the event, and the weapon is slappers, we did it!
			if (Q_atoi(event->GetString("value2")) == pPlayer->GetUserID() && !Q_stricmp(event->GetString("value3"), "weapon_slappers") 
				&& IsScenario("yolt") && CalcPlayerCount() >= 4)
				IncrementCount();
		}
	}
};
DECLARE_GE_ACHIEVEMENT(CAchChoppingBlock, ACHIEVEMENT_GES_CHOPPINGBLOCK, "GES_CHOPPINGBLOCK", 100, GE_ACH_UNLOCKED);


// Last man hiding: Be the last man standing in YOLT with only one kill
class CAchLastManHiding : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		VarInit();
	}

	virtual void VarInit()
	{
		m_iKillCount = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("player_death");
		ListenForGameEvent("gameplay_event");
		ListenForGameEvent("round_start");
		VarInit();
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
		if (!pPlayer)
			return;

		if (!Q_stricmp(event->GetName(), "player_death"))
		{
			// If this isn't us doing the killing we don't care
			if (event->GetInt("attacker") != pPlayer->GetUserID())
				return;

			// But if it is...
			m_iKillCount++;
		}

		else if (!Q_stricmp(event->GetName(), "gameplay_event"))
		{
			if (!Q_stricmp(event->GetString("name"), "yolt_lastmanstanding"))
			{
				// If we were the last man standing, we had less than 2 kills, and there were more than 3 players in the round, we did it!
				if (Q_atoi(event->GetString("value1")) == pPlayer->GetUserID() && m_iKillCount < 2 && Q_atoi(event->GetString("value2")) > 3 && IsScenario("yolt"))
					IncrementCount();
			}
		}

		else if (!Q_stricmp(event->GetName(), "round_start"))
			m_iKillCount = 0; //Reset our killcount.
	}
private:
	int m_iKillCount;
};
DECLARE_GE_ACHIEVEMENT(CAchLastManHiding, ACHIEVEMENT_GES_LASTMANHIDING, "GES_LASTMANHIDING", 100, GE_ACH_UNLOCKED);


// Frags to Riches: Trade the grenade for the golden gun in Investment Losses
class CAchFragsToRiches : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("gameplay_event");
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		if (!Q_stricmp(event->GetString("name"), "gt_weaponswap"))
		{
			CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
			if (!pPlayer)
				return;

			// If we're the one that triggered the event, and the grenade was traded for the golden gun, we did it!
			if (Q_atoi(event->GetString("value1")) == pPlayer->GetUserID() && !Q_stricmp(event->GetString("value3"), "weapon_grenade") && (!Q_strnicmp(event->GetString("value4"), "weapon_gold", 11)) && IsScenario("guntrade"))
				IncrementCount();
		}
	}
};
DECLARE_GE_ACHIEVEMENT(CAchFragsToRiches, ACHIEVEMENT_GES_FRAGSTORICHES, "GES_FRAGSTORICHES", 100, GE_ACH_UNLOCKED);


// Upgrade Path: Trade the pp7 or its silenced variant for the silver PP7, and then trade that for the gold PP7 without making any trades in between.
class CAchUpgradePath : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		VarInit();
	}

	virtual void VarInit()
	{
		m_bState = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("gameplay_event");
		VarInit();
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		if (!Q_stricmp(event->GetString("name"), "gt_weaponswap"))
		{
			CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
			if (!pPlayer)
				return;

			// If it's not us we don't care
			if (Q_atoi(event->GetString("value1")) == pPlayer->GetUserID())
			{
				if (!m_bState) // We're on PP7
				{
					if (!Q_strnicmp(event->GetString("value3"), "weapon_pp7", 10) && (!Q_stricmp(event->GetString("value4"), "weapon_silver_pp7")))
						m_bState = true;

				}
				else if (m_bState) // We're on Silver PP7
				{
					if (!Q_stricmp(event->GetString("value3"), "weapon_silver_pp7") && (!Q_stricmp(event->GetString("value4"), "weapon_golden_pp7")) && IsScenario("guntrade"))
						IncrementCount(); // We got to gold PP7!
					else
						m_bState = false; // Killed with wrong weapon or started with wrong weapon somehow.
				}
			}
			// But we're also allowed to trade by dying
			else if (Q_atoi(event->GetString("value2")) == pPlayer->GetUserID())
			{
				if (!m_bState) // We're on PP7
				{
					if (!Q_strnicmp(event->GetString("value4"), "weapon_pp7", 10) && (!Q_stricmp(event->GetString("value3"), "weapon_silver_pp7")))
						m_bState = true;
				}
				else if (m_bState) // We're on Silver PP7
				{
					if (!Q_stricmp(event->GetString("value4"), "weapon_silver_pp7") && (!Q_stricmp(event->GetString("value3"), "weapon_golden_pp7")) && IsScenario("guntrade"))
						IncrementCount(); // We got to gold PP7!
					else
						m_bState = false; // Killed with wrong weapon or started with wrong weapon somehow.
				}
			}
		}
	}

private:
	bool m_bState;
};
DECLARE_GE_ACHIEVEMENT(CAchUpgradePath, ACHIEVEMENT_GES_UPGRADEPATH, "GES_UPGRADEPATH", 100, GE_ACH_UNLOCKED);


// Return on Investment: Return to a player the weapon they just killed you with
class CAchReturnOnInvestment : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		VarInit();
	}

	virtual void VarInit()
	{
		m_iKillerID = -1;
		Q_strncpy(m_sWeapon, "weapon_none", 32);
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("gameplay_event");
		VarInit();
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		if (!Q_stricmp(event->GetString("name"), "gt_weaponswap"))
		{
			CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
			if (!pPlayer)
				return;

			// Can't get it if we're dead to prevent double kills from counting.
			if (!pPlayer->IsAlive())
				return;

			if (Q_atoi(event->GetString("value2")) == pPlayer->GetUserID())
			{
				m_iKillerID = atoi(event->GetString("value1"));
				Q_strncpy(m_sWeapon, event->GetString("value3"), 32);
			}
			else if (Q_atoi(event->GetString("value1")) == pPlayer->GetUserID())
			{
				// If our weapon was the weapon we were killed with, and the victim was our previous killer, we did it!
				if (m_iKillerID == atoi(event->GetString("value2")) && !Q_stricmp(event->GetString("value3"), m_sWeapon) && IsScenario("guntrade"))
					IncrementCount();
			}
		}
	}

private:
	int m_iKillerID;
	char m_sWeapon[32];
};
DECLARE_GE_ACHIEVEMENT(CAchReturnOnInvestment, ACHIEVEMENT_GES_RETURNONINVESTMENT, "GES_RETURNONINVESTMENT", 100, GE_ACH_UNLOCKED);


// Fort Knox: Pick up the golden gun and get 10 kills without using it.
class CAchFortKnox : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		VarInit();
	}

	virtual void VarInit()
	{
		m_bHasGG = false;
		m_iKillCount = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("player_death");
		ListenForGameEvent("gameplay_event");
		ListenForGameEvent("round_start");
		VarInit();
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
		if (!pPlayer)
			return;

		if (!Q_stricmp(event->GetName(), "gameplay_event"))
		{
			if (!Q_stricmp(event->GetString("name"), "mwgg_ggpickup") && Q_atoi(event->GetString("value1")) == pPlayer->GetUserID())
				m_bHasGG = true;
		}
		else if (!Q_stricmp(event->GetName(), "player_death"))
		{
			// If we died we need to reset our golden gun status and kill count.
			if (event->GetInt("userid") == pPlayer->GetUserID())
			{
				m_iKillCount = 0;
				m_bHasGG = false;
			}
			else if ( m_bHasGG && event->GetInt("attacker") == pPlayer->GetUserID() )
			{
				if (event->GetInt("weaponid") != WEAPON_GOLDENGUN)
				{
					m_iKillCount++;

					// If we managed to get 10 kills without using the golden gun, we did it!
					if ( m_iKillCount >= 10 && IsScenario("mwgg") && CalcPlayerCount() >= 4 )
						IncrementCount();
				}
				else
				{
					m_iKillCount = 0;
					m_bHasGG = false; // We still have the golden gun but we'll pretend we don't to make this life invalid, as we can't "pick it up" again without dying.
				}
			}
		}
		else if (!Q_stricmp(event->GetName(), "round_start"))
		{
			m_iKillCount = 0; //Reset our killcount.
			m_bHasGG = false;
		}
	}

private:
	bool m_bHasGG;
	int m_iKillCount;
};
DECLARE_GE_ACHIEVEMENT(CAchFortKnox, ACHIEVEMENT_GES_FORTKNOX, "GES_FORTKNOX", 100, GE_ACH_UNLOCKED);


// Gold Standard: In MWGG win the round without using any weapon other than the golden gun.
class CAchGoldStandard : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		VarInit();
	}

	virtual void VarInit()
	{
		m_bOnlyUsedGG = true;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("player_death");
		ListenForGameEvent("round_start");
		ListenForGameEvent("round_end");
		VarInit();
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
		if (!pPlayer)
			return;

		if ( !Q_stricmp(event->GetName(), "round_start") )
			m_bOnlyUsedGG = true;
		else if (!m_bOnlyUsedGG)
			return;
		else if (!IsScenario("mwgg"))
			m_bOnlyUsedGG = false;
		else if (!Q_stricmp(event->GetName(), "player_death"))
		{
			if (event->GetInt("attacker") == pPlayer->GetUserID())
			{
				if (event->GetInt("weaponid") != WEAPON_GOLDENGUN)
					m_bOnlyUsedGG = false;
			}
		}
		else if (!Q_stricmp(event->GetName(), "round_end"))
		{
			if (pPlayer->entindex() == event->GetInt("winnerid") && IsScenario("mwgg") 
				&& event->GetInt("roundlength") > 120 && CalcPlayerCount() >= 4)
				IncrementCount();
		}
	}

private:
	bool m_bOnlyUsedGG;
};
DECLARE_GE_ACHIEVEMENT(CAchGoldStandard, ACHIEVEMENT_GES_GOLDSTANDARD, "GES_GOLDSTANDARD", 100, GE_ACH_UNLOCKED);


// Going for the Gold: Pick up the golden gun 100 times
class CAchGoingForTheGold : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(100);
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("gameplay_event");
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		if (!Q_stricmp(event->GetString("name"), "mwgg_ggpickup"))
		{
			CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
			if (!pPlayer)
				return;

			if (Q_atoi(event->GetString("value1")) == pPlayer->GetUserID() && IsScenario("mwgg"))
				IncrementCount();
		}
	}
};
DECLARE_GE_ACHIEVEMENT(CAchGoingForTheGold, ACHIEVEMENT_GES_GOINGFORTHEGOLD, "GES_GOINGFORTHEGOLD", 100, GE_ACH_UNLOCKED);


// Crouching Tiger: Kill 7 players without moving or dying in LTK.
class CAchCrouchingTiger : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		VarInit();
	}

	virtual void VarInit()
	{
		m_vecLastKillPos = vec3_origin;
		m_iKillCount = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("player_death");
		ListenForGameEvent("round_start");
		VarInit();
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
		if (!pPlayer)
			return;

		if ( !Q_stricmp(event->GetName(), "round_start") )
		{
			m_vecLastKillPos = vec3_origin;
			m_iKillCount = 0;
		}
		else if (!Q_stricmp(event->GetName(), "player_death"))
		{
			if (event->GetInt("attacker") == pPlayer->GetUserID())
			{
				if ( !IsScenario("ltk") )
					return;

				if (pPlayer->GetAbsOrigin() == m_vecLastKillPos)
				{
					m_iKillCount++;

					if (m_iKillCount >= 7)
						IncrementCount();
				}
				else
				{
					m_vecLastKillPos = pPlayer->GetAbsOrigin();
					m_iKillCount = 1;
				}
			}
		}
	}

private:
	Vector m_vecLastKillPos;
	int m_iKillCount;
};
DECLARE_GE_ACHIEVEMENT( CAchCrouchingTiger, ACHIEVEMENT_GES_CROUCHINGTIGER, "GES_CROUCHINGTIGER", 100, GE_ACH_UNLOCKED );


// Licence To Spree Kill: Kill 4 enemies within 7 seconds in LTK
class CAchLicenceToSpreeKill : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		VarInit();
	}

	virtual void VarInit()
	{
		m_flKillTime = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("player_death");
		ListenForGameEvent("round_start");
		VarInit();
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
		if (!pPlayer)
			return;

		if (!Q_stricmp(event->GetName(), "round_start"))
		{
			m_flKillTime = 0;
			m_iKillCount = 0;
		}
		else if (!Q_stricmp(event->GetName(), "player_death"))
		{
			if (event->GetInt("attacker") == pPlayer->GetUserID())
			{
				if (!IsScenario("ltk"))
					return;

				if (m_flKillTime >= gpGlobals->curtime - 7)
				{
					m_iKillCount++;

					if (m_iKillCount >= 4)
						IncrementCount();
				}
				else
				{
					m_flKillTime = gpGlobals->curtime;
					m_iKillCount = 1;
				}
			}
		}
	}

private:
	float m_flKillTime;
	int m_iKillCount;
};
DECLARE_GE_ACHIEVEMENT(CAchLicenceToSpreeKill, ACHIEVEMENT_GES_LICENCETOSPREEKILL, "GES_LICENCETOSPREEKILL", 100, GE_ACH_UNLOCKED);


// View to no kills: Win VTAK without killing anyone
class CAchViewToNoKills : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		VarInit();
	}

	virtual void VarInit()
	{
		m_bKilledSomeone = false;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("player_death");
		ListenForGameEvent("round_end");
		ListenForGameEvent("round_start");
		VarInit();
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
		if (!pPlayer)
			return;

		if (!Q_stricmp(event->GetName(), "player_death"))
		{
			// If this isn't us doing the killing we don't care
			if (event->GetInt("attacker") != pPlayer->GetUserID())
				return;

			// But if it is...
			m_bKilledSomeone = true;
		}
		else if (!Q_stricmp(event->GetName(), "round_start"))
			m_bKilledSomeone = false; //Reset our kill flag.
		else if (!Q_stricmp(event->GetName(), "round_end"))
		{
			if (!m_bKilledSomeone && pPlayer->entindex() == event->GetInt("winnerid") && IsScenario("viewtoakill")
				&& event->GetInt("roundlength") > 120 && CalcPlayerCount() >= 4)
				IncrementCount();
		}
	}
private:
	bool m_bKilledSomeone;
};
DECLARE_GE_ACHIEVEMENT(CAchViewToNoKills, ACHIEVEMENT_GES_VIEWTONOKILLS, "GES_VIEWTONOKILLS", 100, GE_ACH_UNLOCKED);


// We've got all the time in the world: Achieve certain victory in VTAK
class CAchAllTheTime : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("gameplay_event");
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		if (!Q_stricmp(event->GetString("name"), "vtak_certainvictory"))
		{
			CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
			if (!pPlayer)
				return;

			if (Q_atoi(event->GetString("value1")) == pPlayer->GetUserID() && IsScenario("viewtoakill") && CalcPlayerCount() >= 4)
				IncrementCount();
		}
	}
};
DECLARE_GE_ACHIEVEMENT(CAchAllTheTime, ACHIEVEMENT_GES_ALLTHETIME, "GES_ALLTHETIME", 100, GE_ACH_UNLOCKED);


// His Finest Hour: Steal a total of 60 minutes of time in VTAK
class CAchFinestHour : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(60);
		VarInit();
	}

	virtual void VarInit()
	{
		m_iStolenSeconds = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("gameplay_event");
		VarInit();
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		if (!Q_stricmp(event->GetString("name"), "vtak_stealtime"))
		{
			CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
			if (!pPlayer)
				return;

			if ( Q_atoi(event->GetString("value1")) != pPlayer->GetUserID() )
				return;

			m_iStolenSeconds += MAX( Q_atoi(event->GetString("value3")), 0 );

			while ( m_iStolenSeconds >= 60 && IsScenario("viewtoakill") )
			{
				m_iStolenSeconds -= 60;
				IncrementCount();
			}
		}
	}
private:
	int m_iStolenSeconds;
};
DECLARE_GE_ACHIEVEMENT(CAchFinestHour, ACHIEVEMENT_GES_FINESTHOUR, "GES_FINESTHOUR", 100, GE_ACH_UNLOCKED);



// Flag Tag: Steal 100 points in Living Daylights with the flag
class CAchFlagTag : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(100);
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("gameplay_event");
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		if (!Q_stricmp(event->GetString("name"), "ld_flagpoint"))
		{
			CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
			if (!pPlayer)
				return;

			if (Q_atoi(event->GetString("value1")) == pPlayer->GetUserID() && !Q_stricmp(event->GetString("value3"), "flaghit") && IsScenario("livingdaylights"))
				IncrementCount();
		}
	}
};
DECLARE_GE_ACHIEVEMENT(CAchFlagTag, ACHIEVEMENT_GES_FLAGTAG, "GES_FLAGTAG", 100, GE_ACH_UNLOCKED);

// King of the Flag: Win living daylights with less than 10 points.
class CAchKingOfTheFlag : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("round_end");
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
		if (!pPlayer)
			return;

		if ( !Q_stricmp(event->GetName(), "round_end") )
		{
			if (pPlayer->entindex() == event->GetInt("winnerid") && event->GetInt("winnerscore") <= 10 && IsScenario("livingdaylights")
				&& event->GetInt("roundlength") > 120 && CalcPlayerCount() >= 4)
				IncrementCount();
		}
	}
};
DECLARE_GE_ACHIEVEMENT( CAchKingOfTheFlag, ACHIEVEMENT_GES_KINGOFTHEFLAG, "GES_KINGOFTHEFLAG", 100, GE_ACH_UNLOCKED );


// Flag Saver: Capture the enemy flag 10 times in Capture the Flag
class CAchFlagSaver : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(10);
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("gameplay_event");
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		if ( !Q_stricmp(event->GetString("name"), "ctf_tokencapture") )
		{
			CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
			if (!pPlayer)
				return;

			if (Q_atoi(event->GetString("value1")) == pPlayer->GetUserID() && IsScenario("capturetheflag") && CalcPlayerCount() >= 4)
				IncrementCount();
		}
	}
};
DECLARE_GE_ACHIEVEMENT(CAchFlagSaver, ACHIEVEMENT_GES_FLAGSAVER, "GES_FLAGSAVER", 100, GE_ACH_UNLOCKED);

// Flagged Down: Kill an enemy who has your flag with their flag in Capture the Flag
class CAchFlaggedDown : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("gameplay_event");
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		if (!Q_stricmp(event->GetString("name"), "ctf_tokendefended"))
		{
			CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
			if (!pPlayer)
				return;

			// If we defended our flag using a flag, we killed their token holder while we were a token holder!
			if (Q_atoi(event->GetString("value1")) == pPlayer->GetUserID() && !Q_strnicmp(event->GetString("value4"), "token_", 6) && IsScenario("capturetheflag"))
				IncrementCount();
		}
	}
};
DECLARE_GE_ACHIEVEMENT(CAchFlaggedDown, ACHIEVEMENT_GES_FLAGGEDDOWN, "GES_FLAGGEDDOWN", 100, GE_ACH_UNLOCKED);


// Cycle of Death: Land 2 CMAG headshots on the same target within 1 second.
class CAchCycleOfDeath : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		VarInit();
	}

	virtual void VarInit()
	{
		m_iVictimID = 0;
		m_flLastHitTime = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("player_hurt");
		VarInit();
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
		if (!pPlayer)
			return;

		// Not us, don't care
		if (event->GetInt("attacker") != pPlayer->GetUserID())
			return;

		if ( event->GetInt("weaponid") == WEAPON_COUGAR_MAGNUM && event->GetInt("hitgroup") == HITGROUP_HEAD )
		{
			if (event->GetInt("userid") == m_iVictimID && gpGlobals->curtime <= m_flLastHitTime + 1.0f)
				IncrementCount();
			else
			{
				m_flLastHitTime = gpGlobals->curtime;
				m_iVictimID = event->GetInt("userid");
			}
		}
	}
private:
	int m_iVictimID;
	float m_flLastHitTime;
};
DECLARE_GE_ACHIEVEMENT(CAchCycleOfDeath, ACHIEVEMENT_GES_CYCLEOFDEATH, "GES_CYCLEOFDEATH", 100, GE_ACH_UNLOCKED);


// Highly Trained Operative: Get a kill with 9 different weapons in one life.
class CAchHighlyTrained : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		VarInit();
	}

	virtual void VarInit()
	{
		m_iWeaponFlags = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("player_death");
		ListenForGameEvent("player_spawn");
		VarInit();
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
		if (!pPlayer)
			return;

		// We died or respawned, reset our progress
		if (event->GetInt("userid") == pPlayer->GetUserID())
		{
			m_iWeaponFlags = 0;
			return;
		}

		// If it was the player spawn event there's no need to go further.
		if (Q_stricmp(event->GetName(), "player_spawn") == 0)
			return;

		// Not us doing the killing, don't care
		if (event->GetInt("attacker") != pPlayer->GetUserID())
			return;

		int weapid = event->GetInt("weaponid");

		if (weapid == WEAPON_TOKEN)
			weapid = WEAPON_SPAWNMAX; // Assign tokens to an unused bit.
		else if (weapid > WEAPON_RANDOM)
			weapid = WEAPON_RANDOM_MAX; // Assign explosions and traps to an unused bit.

		m_iWeaponFlags |= 1 << weapid;

		int uniquecount = 0;

		for ( int i = 0; i < WEAPON_RANDOM; i++ )
		{
			if ( 1 << i & m_iWeaponFlags )
				uniquecount++;
		}

		if ( uniquecount > 8)
			IncrementCount();
	}
private:
	int m_iWeaponFlags;
};
DECLARE_GE_ACHIEVEMENT(CAchHighlyTrained, ACHIEVEMENT_GES_HIGHLYTRAINED, "GES_HIGHLYTRAINED", 100, GE_ACH_UNLOCKED);


// Silent Opposition: kill 5 people with the silenced pp7 without being damaged in deathmatch.
class CAchSilentOpp : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal(1);
		VarInit();
	}

	virtual void VarInit()
	{
		m_iKillcount = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("player_death");
		ListenForGameEvent("player_hurt");
		VarInit();
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
		if (!pPlayer)
			return;

		// We got hurt or died, reset the count.
		if ( event->GetInt("userid") == pPlayer->GetUserID() )
		{
			m_iKillcount = 0;
			return;
		}

		// Not us attacking
		if ( event->GetInt("attacker") != pPlayer->GetUserID() )
			return;

		if (!Q_stricmp(event->GetName(), "player_death"))
		{
			if ( event->GetInt("weaponid") == WEAPON_PP7_SILENCED )
			{
				m_iKillcount++;

				if ( m_iKillcount > 4 && IsScenario("deathmatch") && CalcPlayerCount() >= 4 )
					IncrementCount();
			}
		}
	}
private:
	int m_iKillcount;
};
DECLARE_GE_ACHIEVEMENT(CAchSilentOpp, ACHIEVEMENT_GES_SILENTOPP, "GES_SILENTOPP", 100, GE_ACH_UNLOCKED);


//The Perfect Agent : Get 8128 kills over your GoldenEye career.
class CAchPerfectAgent : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags(ACH_SAVE_GLOBAL);
		SetGoal( 8128 );
		VarInit();
	}

	virtual void VarInit()
	{
		m_iKillInterp = 0;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent("player_death");
		VarInit();
	}

	virtual void FireGameEvent_Internal(IGameEvent *event)
	{
		CGEPlayer *pPlayer = ToGEPlayer(CBasePlayer::GetLocalPlayer());
		if (!pPlayer)
			return;

		// Not us attacking
		if ( event->GetInt("attacker") != pPlayer->GetUserID() )
			return;

		m_iKillInterp++;
		if ( IsScenario( "ltk" ) && m_iKillInterp % 3 )
			return;

		IncrementCount();
	}
private:
	int m_iKillInterp;
};
DECLARE_GE_DEPENDENT_ACHIEVEMENT(CAchPerfectAgent, ACHIEVEMENT_GES_PERFECTAGENT, "GES_PERFECTAGENT", 28, ACHIEVEMENT_GES_GOLDENEYE_MASTER);

#endif

