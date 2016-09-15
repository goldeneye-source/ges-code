//////////  Copyright © 2011, Goldeneye Source. All rights reserved. ///////////
// 
// ge_playerspawn.h
//
// Description:
//      A complete description of a player spawn type entity
//
// Created On: 6/11/2011
// Created By: Killer Monkey
////////////////////////////////////////////////////////////////////////////////
#ifndef GE_PLAYERSPAWN_H
#define GE_PLAYERSPAWN_H

#include "ge_spawner.h"
#include "ge_player.h"
#include "ge_shareddefs.h"

class CGEPlayerSpawn : public CGESpawner
{
	DECLARE_CLASS( CGEPlayerSpawn, CGESpawner );

public:
	CGEPlayerSpawn();

	DECLARE_DATADESC();

	virtual void Spawn( void );
	virtual void Think( void );

	bool IsBotFriendly();
	bool IsOccupied();

	void NotifyOnDeath( float dist );
	void NotifyOnUse();
	void SetUseFadeTime(float usetime);

	// Returns desirability weighting (high weight, high desirability)
	int GetDesirability( CGEPlayer *pRequestor );

	// Debugging
	void DEBUG_ShowOverlay( float duration );

protected:
	void OnInit();

	void OnEnabled();
	void OnDisabled();

	void ResetTrackers();

	bool CheckInPVS( CGEPlayer *player );

private:
	int m_iBaseDesirability;

	// Tracking variables
	float m_fLastUseTime;
	float m_fUseFadeTime;
	float m_fDeathFadeTime;
	float m_fMaxSpawnDist;

	int m_iLastEnemyWeight;
	int m_iLastDeathWeight;
	int m_iLastUseWeight;
	int m_iUniLastDeathWeight;
	int m_iUniLastUseWeight;

	// PVS Checks
	byte m_iPVS[ MAX_MAP_CLUSTERS/8 ];
	bool m_bFoundPVS;

	// KeyValues
	int   m_iTeam;
	float m_flDesirabilityMultiplier;
};

#endif
