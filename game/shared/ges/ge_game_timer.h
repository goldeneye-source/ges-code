///////////// Copyright © 2013, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_round_timer.h
// Description:
//      Round timer for GE:S
//
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#ifndef GE_GAMEPLAY_TIMER_H
#define GE_GAMEPLAY_TIMER_H

#include "ge_shareddefs.h"

#ifdef CLIENT_DLL
#define CGEGameTimer C_GEGameTimer
#endif

class CGEGameTimer : public CBaseEntity
{
public:
	DECLARE_CLASS( CGEGameTimer, CBaseEntity );
	DECLARE_NETWORKCLASS();

	CGEGameTimer();

	// Get the remaining time on this timer
	float GetTimeRemaining();
	// Get the length of the timer
	float GetLength()	{ return m_flLength; }

	// Check if this timer is enabled for use
	bool IsEnabled()	{ return m_bEnabled; }

	// Check the state of the timer
	bool IsStarted()	{ return m_bStarted; }
	bool IsPaused()		{ return m_bPaused; }
	
#ifdef GAME_DLL
	// Enable/Disable this timer
	void Enable();
	void Disable();

	// Start and Stop the timer
	void Start( float length_seconds );
	void Stop();

	// Change the length of an already running timer, will account for time already elapsed.
	void ChangeLength( float length_seconds );
	
	// Set the current value of the timer, will ignore time already elapsed.
	void SetCurrentLength( float length_seconds );
	// Add time to the timer
	void AddToLength( float length_seconds );

	// Pause and resume control
	void Pause();
	void Resume();

	// Ensures we send network updates to all players
	virtual int UpdateTransmitState();
#endif

private:
	CNetworkVar( bool, m_bEnabled );
	CNetworkVar( bool, m_bStarted );
	CNetworkVar( bool, m_bPaused );
	CNetworkVar( float, m_flPauseTimeRemaining );
	CNetworkVar( float, m_flEndTime );
	CNetworkVar( float, m_flLength );
};

#endif	//GE_ROUND_TIMER_H