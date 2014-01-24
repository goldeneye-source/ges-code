///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_baseachievement.cpp
// Description:
//      Goldeneye Base Achievement
//
// Created On: 12 Sep 08
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "achievementmgr.h"
#include "ge_achievement.h"

#ifdef CLIENT_DLL
#include "c_ge_gameplayresource.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC( CGEAchievement )
DEFINE_FIELD( m_iDependentID,	FIELD_INTEGER ),
END_DATADESC()

CGEAchievement::CGEAchievement( void )
{
	m_iDependentID = -1;
}

void CGEAchievement::SetDependent( int id )
{
	m_iDependentID = id;
}

bool CGEAchievement::IsActive( void )
{
	// If we are dependent on another achievement being achieved don't do anything with this
	// if it hasn't been achieved yet!
	if ( IsDependent() )
	{
		if ( !(m_pAchievementMgr->GetAchievementByID(m_iDependentID)->IsAchieved()) )
			return false;
	}
	
	return BaseClass::IsActive();
}

bool CGEAchievement::ShouldHideUntilAchieved( void )
{
	// If we are dependent on another achievement being achieved don't show us until it has been achieved
	if ( IsDependent() )
	{
		CBaseAchievement *ach = m_pAchievementMgr->GetAchievementByID(m_iDependentID);
		if ( !ach )
			return false;

		if ( !ach->IsAchieved() )
			return true;
	}
	else if ( m_bHideUntilAchieved )
	{
		if ( GetCount() == 0 )
			return true;
	}
	
	return false;
}

void CGEAchievement::HandleProgressUpdate()
{
	// We are a hidden achievement and this is our first increment
	// show the unlocked message
	if ( m_bHideUntilAchieved && GetCount() == 1 )
		ShowUnlockedNotification();

	BaseClass::HandleProgressUpdate();
}

//-----------------------------------------------------------------------------
// Purpose: displays achievement progress notification in the HUD
//-----------------------------------------------------------------------------
void CGEAchievement::ShowUnlockedNotification()
{
	if ( !ShouldShowProgressNotification() )
		return;

	IGameEvent *event = gameeventmanager->CreateEvent( "achievement_unlocked" );
	if ( event )
	{
		event->SetString( "achievement_name", GetName() );
#ifdef GAME_DLL
		gameeventmanager->FireEvent( event );
#else
		gameeventmanager->FireEventClientSide( event );
#endif
	}	
}

bool FindAwardForPlayer( IGameEvent *pEvent, int iAward, int iPlayer )
{
	if ( !pEvent || iAward < 0 || iPlayer < 0 )
		return false;

	char eventid[16], eventwinner[16];
	for ( int i=1; i <= 6; i++ )
	{
		Q_snprintf( eventid, 16, "award%i_id", i );
		Q_snprintf( eventwinner, 16, "award%i_winner", i );

		// We found the award (remember only 1 can happen)
		if ( pEvent->GetInt( eventid, -1 ) == iAward )
		{
			if ( pEvent->GetInt( eventwinner, -1 ) == iPlayer )
				return true;
			else
				return false;
		}
	}

	return false;
}

// Returns the number of awards this player got
int GetAwardsForPlayer( IGameEvent *pEvent, int iPlayer )
{
	if ( !pEvent || iPlayer < 0 )
		return 0;

	char eventwinner[16];
	int count = 0;

	for ( int i=1; i <= 6; i++ )
	{
		Q_snprintf( eventwinner, 16, "award%i_winner", i );

		// We found an award the player one, increment!
		if ( pEvent->GetInt( eventwinner, -1 ) == iPlayer )
			count++;
	}

	return count;
}

#ifdef CLIENT_DLL
extern ConVar cc_achievement_debug;
bool IsScenario( const char *ident, bool official /*=true*/ )
{
	if ( !GEGameplayRes() )
		return false;

	// Check the ident
	if ( !Q_stricmp( GEGameplayRes()->GetGameplayIdent(), ident ) )
	{
		// Return true if we don't specify we want official, or if we are actually official
		if ( !official || GEGameplayRes()->GetGameplayOfficial() || cc_achievement_debug.GetInt() > 0 )
			return true;
	}

	return false;
}
#endif
