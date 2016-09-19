///////////// Copyright © 2013, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_round_timer.cpp
// Description:
//      Round timer for GE:S
//
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include "ge_game_timer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( ge_game_timer, CGEGameTimer );

IMPLEMENT_NETWORKCLASS_ALIASED( GEGameTimer, DT_GEGameTimer )

BEGIN_NETWORK_TABLE_NOBASE( CGEGameTimer, DT_GEGameTimer )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bEnabled ) ),
	RecvPropBool( RECVINFO( m_bStarted ) ),
	RecvPropBool( RECVINFO( m_bPaused ) ),
	RecvPropTime( RECVINFO( m_flPauseTimeRemaining ) ),
	RecvPropTime( RECVINFO( m_flLength ) ),
	RecvPropTime( RECVINFO( m_flEndTime ) ),
#else
	SendPropBool( SENDINFO( m_bEnabled ) ),
	SendPropBool( SENDINFO( m_bStarted ) ),
	SendPropBool( SENDINFO( m_bPaused ) ),
	SendPropTime( SENDINFO( m_flPauseTimeRemaining ) ),
	SendPropTime( SENDINFO( m_flLength ) ),
	SendPropTime( SENDINFO( m_flEndTime ) ),
#endif
END_NETWORK_TABLE()

//-----------------------------------------------------------------------------
// Purpose: constructor
//-----------------------------------------------------------------------------
CGEGameTimer::CGEGameTimer()
{
#ifdef GAME_DLL
	// Reset the timer to the default state (enabled and stopped)
	Enable();
	Stop();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Gets the seconds left on the timer, paused or not.
//-----------------------------------------------------------------------------
float CGEGameTimer::GetTimeRemaining()
{
	// If we are not started return 0
	if (!IsStarted())
		return 0;

	float seconds_remaining;
	
	if ( IsPaused() )
		seconds_remaining = m_flPauseTimeRemaining;
	else
		seconds_remaining = m_flEndTime - gpGlobals->curtime;

	return MAX( seconds_remaining, 0 );
}

#ifdef GAME_DLL
// Enable the timer, you have to call Start() yourself
void CGEGameTimer::Enable()
{
	m_bEnabled = true;
}

// Disable the timer, this also stops the timer
void CGEGameTimer::Disable()
{
	m_bEnabled = false;
	Stop();
}

// Starts the timer if length_seconds is greater than 0, otherwise stops the timer
// If the timer is already started, this will adjust the end time accordingly
void CGEGameTimer::Start( float length_seconds )
{
	// Only start if we are enabled
	if ( IsEnabled() )
	{
		if ( length_seconds <= 0 ) {
			// Stop the timer if called with no length
			Stop();
		} else {
			// Start the timer
			m_bStarted = true;
			m_flLength = length_seconds;
			m_flEndTime = gpGlobals->curtime + m_flLength;

			// Reset any pause state
			m_bPaused = false;
			m_flPauseTimeRemaining = 0;
		}
	}
}

void CGEGameTimer::Stop()
{
	// Stop the timer
	m_bStarted = false;
	m_flLength = 0;
	m_flEndTime = 0;

	// Reset any pause state
	m_bPaused = false;
	m_flPauseTimeRemaining = 0;
}

void CGEGameTimer::ChangeLength( float length_seconds )
{
	// Don't do anything we are are not enabled
	if ( !IsEnabled() )
		return;

	// Stop the timer if we change to no length
	if ( length_seconds <= 0 ) {
		Stop();
		return;
	}

	// Simply start the timer if not running already
	if ( !IsStarted() ) {
		Start( length_seconds );
		return;
	}

	// If paused, resume the time to capture our real end time
	bool was_paused = IsPaused();
	if ( was_paused )
		Resume();

	// Find time diff and modify the end time
	int time_diff = length_seconds - m_flLength;

	DevMsg("%d is time difference \n", time_diff);
	DevMsg("%f is endtime \n", (float)m_flEndTime);
	DevMsg("%f is timer length \n", (float)m_flLength);
	DevMsg("%f is entered length \n", length_seconds);
	DevMsg("%f is computed offset \n", m_flEndTime - gpGlobals->curtime + (float)time_diff);

	// If changing the time would result in negative time, just set the roundtime to the time it was changed to.
	if (m_flEndTime - gpGlobals->curtime + time_diff > 0) // If the time to be adjusted to is greater than 0, go ahead and offset
	{
		m_flEndTime += time_diff;
		m_flLength += time_diff;
	}
	else // Otherwise just directly set the time to the time entered to prevent nonsense.
	{
		m_flEndTime = gpGlobals->curtime + length_seconds;
		m_flLength = length_seconds;
	}

	// If we were paused, put as back in that state
	if ( was_paused )
		Pause();
}

void CGEGameTimer::SetCurrentLength(float length_seconds)
{
	// Don't do anything we are are not enabled
	if (!IsEnabled())
		return;

	// Stop the timer if we change to no length
	if (length_seconds <= 0) {
		Stop();
		return;
	}

	// Simply start the timer if not running already
	if (!IsStarted()) {
		Start(length_seconds);
		return;
	}

	// If paused, resume the time to capture our real end time
	bool was_paused = IsPaused();
	if (was_paused)
		Resume();

	// We always directly set to this time.
	m_flEndTime = gpGlobals->curtime + length_seconds;
	m_flLength = length_seconds;

	// If we were paused, put as back in that state
	if (was_paused)
		Pause();
}

void CGEGameTimer::AddToLength(float length_seconds)
{
	// Don't do anything we are are not enabled
	if (!IsEnabled())
		return;

	// If paused, resume the time to capture our real end time
	bool was_paused = IsPaused();
	if (was_paused)
		Resume();

	// If changing the time would result in negative time, end the round.
	if (m_flEndTime - gpGlobals->curtime + length_seconds > 0) // If the time to be adjusted to is greater than 0, go ahead and offset
	{
		m_flEndTime += length_seconds;
		m_flLength += length_seconds;
	}
	else // Otherwise we took off all the remaining round time and ended it.
	{
		m_flEndTime = gpGlobals->curtime;
		m_flLength = 1;
	}

	// If we were paused, put as back in that state
	if (was_paused)
		Pause();
}

//-----------------------------------------------------------------------------
// Purpose: Timer is paused at round end, stops the countdown
//-----------------------------------------------------------------------------
void CGEGameTimer::Pause()
{
	if ( IsStarted() && !IsPaused() )
	{
		// Store our pause time remaining to our currently remaining time
		m_flPauseTimeRemaining = GetTimeRemaining();
		m_bPaused = true;
	}
}

//-----------------------------------------------------------------------------
// Purpose: To start or re-start the timer after a pause
//-----------------------------------------------------------------------------
void CGEGameTimer::Resume()
{
	if ( IsStarted() && IsPaused() )
	{
		// Set our new end time to our Resume Time plus our Remaining Time
		m_flEndTime = gpGlobals->curtime + GetTimeRemaining();
		m_flPauseTimeRemaining = 0;
		m_bPaused = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: The timer is always transmitted to clients
//-----------------------------------------------------------------------------
int CGEGameTimer::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}

#endif
