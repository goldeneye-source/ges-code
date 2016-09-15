///////////// Copyright � 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_hudroundtimer.cpp
// Description:
//      Shows the current round time (placer until the Gameplay Info Panel is up and running)
//
// Created On: 11/14/2008
// Created By: Killer Monkey
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "hud.h"
#include "iclientmode.h"
#include "view.h"
#include "hud_numericdisplay.h"
#include "vgui_controls/AnimationController.h"
#include "vgui/ISurface.h"
#include "in_buttons.h"
#include "hudelement.h"

#include "ge_gamerules.h"
#include "gemp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

ConVar cl_ge_show_timer( "cl_ge_show_timer", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Show the round time at the bottom of the screen." );

class CGEHudRoundTimer : public CHudNumericDisplay, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CGEHudRoundTimer, CHudNumericDisplay );

public:
	CGEHudRoundTimer( const char *pElementName );

	virtual void Reset();
	virtual bool ShouldDraw();
	virtual void Think();
	
	void ResetColor( bool paused=false );

	CPanelAnimationVar( Color, m_NormColor, "fontcolor", "255 255 255 255" );
	CPanelAnimationVar( Color, m_SpecColor, "speccolor", "0 0 0 235" );
	CPanelAnimationVar( Color, m_PausedColor, "pausecolor", "120 120 120 255" );

private:
	float m_flNextAnim;
	float m_flNextThink;
	float m_iPrevValue;
};

DECLARE_HUDELEMENT( CGEHudRoundTimer );

CGEHudRoundTimer::CGEHudRoundTimer( const char *pElementName ) : 
	BaseClass( NULL, "GERoundTimer" ), CHudElement( pElementName )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetIsTime(true);
	m_flNextAnim = 0;
	m_flNextThink = 0;
	m_iPrevValue = 0;
}

void CGEHudRoundTimer::Reset()
{
	m_flNextThink = 0;
}

bool CGEHudRoundTimer::ShouldDraw()
{
	// Don't show if we are in intermission time!
	if ( !cl_ge_show_timer.GetBool() )
		return false;

	// Don't show if we are in intermission time!
	if ( GEMPRules() && GEMPRules()->IsIntermission() )
	{
		m_flNextAnim = 0;
		return false;
	}

	// Protect against timer flashes due to hiding it for more than 5 seconds.  Think is not called in the escape menu for some reason.
	if (GEMPRules() && gpGlobals->curtime >= m_flNextThink + 3)
	{
		float timeleft = 0;

		if (GEMPRules()->IsRoundTimeRunning())
			timeleft = GEMPRules()->GetRoundTimeRemaining();
		else if (GEMPRules()->IsMatchTimeRunning())
			timeleft = GEMPRules()->GetMatchTimeRemaining();

		m_iPrevValue = timeleft;
		m_flNextThink = gpGlobals->curtime + 0.1f;
	}

	return CHudElement::ShouldDraw();
}

void CGEHudRoundTimer::Think()
{
	if ( !GEMPRules() || gpGlobals->curtime < m_flNextThink )
		return;

	// Default is to show "--:--"
	float timeleft = 0;

	// Extract a time to use, clamp to the minimum between the match and round timers
	if (GEMPRules()->IsRoundTimeRunning())
		timeleft = GEMPRules()->GetRoundTimeRemaining(); //Rounds always finish even when match time runs out.
	else if (GEMPRules()->IsMatchTimeRunning() && GEMPRules()->IsRoundTimeEnabled()) // If round time isn't enabled then we're controling rounds some other way.
		timeleft = GEMPRules()->GetMatchTimeRemaining();

	// Check if we have time to display
	if ( timeleft <= 0 )
	{
		// Set the timer display to show "--:--" and reset the coloring
		SetDisplayValue( -1 );
		ResetColor();
		
		m_flNextThink = gpGlobals->curtime + 1.0f;
	}
	else if ( GEMPRules()->IsRoundTimePaused() )
	{
		// Set our timer display to show the time left
		SetDisplayValue( timeleft );
		// Color us as paused
		ResetColor( true );
	}
	else
	{
		// Set our timer display to show the time left
		SetDisplayValue( timeleft );

		// Check if we need to add some special coloring / animations

		if ( timeleft <= 10.0f && m_flNextAnim < gpGlobals->curtime )
		{
			// If we only have 10 seconds left start going beserk! (every second)
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("RoundTimeCritical");
			m_flNextAnim = gpGlobals->curtime + g_pClientMode->GetViewportAnimationController()->GetAnimationSequenceLength("RoundTimeCritical");
		}
		else if ( timeleft <= 30.0f && m_flNextAnim < gpGlobals->curtime )
		{
			// If we only have 30 seconds left start notifying the player! (every 5 seconds)
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("RoundTimeExpiring");
			m_flNextAnim = gpGlobals->curtime + 5.0f;
		}
		else if ( timeleft - m_iPrevValue >= 5.0f )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("RoundTimeIncreased");
			m_flNextAnim = gpGlobals->curtime + g_pClientMode->GetViewportAnimationController()->GetAnimationSequenceLength("RoundTimeIncreased");
		}
		else if ( timeleft - m_iPrevValue <= -5.0f )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("RoundTimeDecreased");
			m_flNextAnim = gpGlobals->curtime + g_pClientMode->GetViewportAnimationController()->GetAnimationSequenceLength("RoundTimeDecreased");
		}
		else if (m_flNextAnim < gpGlobals->curtime)
		{	
			// Reset the color since we have no animation time
			ResetColor();
		}

		m_iPrevValue = timeleft;
		m_flNextThink = gpGlobals->curtime + 0.1f;
	}

	
}

void CGEHudRoundTimer::ResetColor( bool paused /*=false*/ )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	// Reset the color immediately, spectators only see one color regardless of paused
	if ( pPlayer && pPlayer->IsObserver() )
		SetFgColor( m_SpecColor );
	else if ( paused )
		SetFgColor( m_PausedColor );
	else
		SetFgColor( m_NormColor );

	// Reset the blur
	m_flBlur = 0;
}
