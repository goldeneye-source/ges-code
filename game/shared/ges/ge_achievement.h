///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_baseachievement.h
// Description:
//      Goldeneye Base Achievement
//
// Created On: 12 Sep 08
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////

#ifndef GEACHIEVEMENT_H
#define GEACHIEVEMENT_H
#ifdef _WIN32
#pragma once
#endif

#include "baseachievement.h"
#include "ge_achievement_defs.h"

class CGEAchievement : public CBaseAchievement
{
	DECLARE_CLASS( CGEAchievement, CBaseAchievement );
public:
	CGEAchievement();

	DECLARE_DATADESC();

	// If this achievement depends on another to be active return true
	virtual bool IsDependent( void ) { return m_iDependentID != -1; };
	// Returns the ID of the achievement this depends on (if dependent)
	virtual int GetDependent( void ) { return m_iDependentID; };
	// Set this achievement to be dependent on another being achieved
	virtual void SetDependent( int id );

	// Set to false if we are dependent and that has not been achieved yet
	virtual bool IsActive( void );

	// This will hide the achievement until our dependent has been achieved
	virtual bool ShouldHideUntilAchieved( void );

protected:
	virtual void HandleProgressUpdate();
	virtual void ShowUnlockedNotification();

private:
	int		m_iDependentID;
};

#ifdef CLIENT_DLL

#define DECLARE_GE_ACHIEVEMENT_( className, achievementID, achievementName, gameDirFilter, iPointValue, bHidden, dependentID  ) \
static CBaseAchievement *Create_##className( void )										\
{																\
	CGEAchievement *pAchievement = new className( );									\
	pAchievement->SetAchievementID( achievementID );									\
	pAchievement->SetName( achievementName );										\
	pAchievement->SetPointValue( iPointValue );										\
	pAchievement->SetHideUntilAchieved( bHidden );										\
	pAchievement->SetDependent( dependentID );										\
	if ( gameDirFilter ) pAchievement->SetGameDirFilter( gameDirFilter ); 							\
	return pAchievement;													\
};																\
static CBaseAchievementHelper g_##className##_Helper( Create_##className );

// HELPER FUNCTIONS TO MAKE FINDING THINGS OUT EASIER!
bool FindAwardForPlayer( IGameEvent *pEvent, int iAward, int iPlayer );
int GetAwardsForPlayer( IGameEvent *pEvent, int iPlayer );
#ifdef CLIENT_DLL
bool IsScenario( const char *ident, bool official=true );
#endif

#define GE_ACH_HIDDEN		true
#define GE_ACH_UNLOCKED		false

#define DECLARE_GE_DEPENDENT_ACHIEVEMENT( className, achievementID, achievementName, iPointValue, dependentID ) \
	DECLARE_GE_ACHIEVEMENT_( className, achievementID, achievementName, NULL, iPointValue, false, dependentID )

#define DECLARE_GE_ACHIEVEMENT( className, achievementID, achievementName, iPointValue, bHidden ) \
	DECLARE_GE_ACHIEVEMENT_( className, achievementID, achievementName, NULL, iPointValue, bHidden, -1 )

#define CREATE_ACH_WEPLEVEL( className, weapid, level, goal )	\
	class C##className##Level##level : public CGEAchievement	\
	{															\
	protected:													\
		virtual void Init()	{									\
			SetFlags( ACH_LISTEN_PLAYER_KILL_ENEMY_EVENTS | ACH_SAVE_GLOBAL );	\
			SetGoal( goal );									\
			m_bIsLTK = false;									\
			m_iKillInterp = 0;									\
		}														\
		virtual void Event_EntityKilled( CBaseEntity *pVictim, CBaseEntity *pAttacker, CBaseEntity *pInflictor, IGameEvent *event )	{ \
			m_iKillInterp++;									\
			if ( IsScenario( "ltk", false ) && m_iKillInterp % 3 ) return;		\
			if ( event->GetInt("weaponid") == weapid ) IncrementCount(); \
		}														\
	private:													\
		int m_iKillInterp;										\
		bool m_bIsLTK;											\
	};

// Base Award Type Achievement with support for specific character
class CGEAchBaseAwardType : public CGEAchievement
{
protected:
	virtual void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );

		m_vAwardsReq.RemoveAll();
		m_szCharReq = NULL;

		m_iCharChangeCnt = 0;
		m_bWasCharForRound = false;
		m_bIsChar = false;
	}

	virtual void ListenForEvents()
	{
		ListenForGameEvent( "round_end" );
		ListenForGameEvent( "player_changeident" );
	}

	virtual void FireGameEvent_Internal( IGameEvent *event )
	{
		CBasePlayer *pLocalPlayer = CBasePlayer::GetLocalPlayer();
		if (!pLocalPlayer) return;
		int localid = pLocalPlayer->entindex();

		if ( Q_strcmp( event->GetName(), "round_end" ) == 0 && event->GetBool("showreport"))
		{
			// See if we met all of our requirements
			bool bAchieved = false;
			if ( ((m_szCharReq && m_bWasCharForRound) || !m_szCharReq) && CalcPlayerCount() >= 4 )
			{
				bAchieved = true;

				for ( int i=0; i < m_vAwardsReq.Count(); i++ )
				{
					if ( !FindAwardForPlayer(event, m_vAwardsReq[i], localid) )
						bAchieved = false;
				}
			}

			// If we made it through the gauntlet of requirements give em the achievement
			if ( bAchieved )
				IncrementCount();

			// Make sure we start the next round as being able to achieve this if we changed to boris mid round last round
			if ( m_bIsChar )
				m_bWasCharForRound = true;
		}
		else if ( Q_strcmp( event->GetName(), "player_changeident" ) == 0 )
		{
			// This is not us, ignore
			if ( !m_szCharReq || localid != event->GetInt("playerid") )
				return;

			// See if we changed to our required character
			if ( !Q_stricmp( event->GetString("ident"), m_szCharReq ) )
			{
				// If this is our first character change for the round then we set our flag
				if ( !m_iCharChangeCnt )
					m_bWasCharForRound = true;

				m_bIsChar = true;
			}
			else
			{
				m_bWasCharForRound = false;
				m_bIsChar = false;
			}

			m_iCharChangeCnt++;
		}
	}
protected:
	void SetCharReq( char *name ) { m_szCharReq = name; }
	void AddAwardType( int award ) { m_vAwardsReq.AddToTail( award ); }
	bool IsChar( void ) { return m_bIsChar; }
	bool WasCharForRound( void ) { return m_bWasCharForRound; }
	void SetCharForRound( bool state ) { m_bWasCharForRound = state; }

	char *m_szCharReq;
	CUtlVector<int> m_vAwardsReq;

private:
	bool m_bIsChar;
	bool m_bWasCharForRound;
	int m_iCharChangeCnt;
};

#endif // CLIENT_DLL
#endif // HEADER
