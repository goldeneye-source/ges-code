///////////// Copyright � 2009, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_hudteamscores.cpp
// Description:
//      See Header
//
// Created On: 10 Dec 09
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"
#include "vgui_controls/AnimationController.h"

#include "c_ge_player.h"
#include "c_ge_playerresource.h"
#include "gemp_gamerules.h"
#include "c_team.h"

#include "ge_hudteamscores.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT( CHudTeamScores );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudTeamScores::CHudTeamScores( const char *pElementName ) : BaseClass(NULL, "GEHUDTeamScores"), CHudElement( pElementName )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pHudAnims = g_pClientMode->GetViewportAnimationController();

	m_pCurrTeamIcon = NULL;
	m_iTeamBackground = -1;

	// Define our own colors to mesh with the folder background better
	m_TeamColors[TEAM_JANUS] = COLOR_JANUS;
	m_TeamColors[TEAM_MI6] = Color( 87, 135, 247, 255 );

	SetHiddenBits( HIDEHUD_WEAPONSELECTION );
}

//-----------------------------------------------------------------------------
// Purpose: Reset the HUD element on map change
//-----------------------------------------------------------------------------
void CHudTeamScores::Reset()
{
	// Reset the current leader
	m_iTeamLeader = TEAM_UNASSIGNED;
}

void CHudTeamScores::VidInit()
{
	m_pCurrTeamIcon = gHUD.GetIcon( "ge_currteam_icon" );
}

bool CHudTeamScores::ShouldDraw()
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer || pPlayer->IsObserver() )
		return false;
	else if ( !GEMPRules() || !GEMPRules()->IsTeamplay() )
		return false;

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTeamScores::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetFgColor( Color(0,0,0,0) );

	SetAlpha( 255 );
	SetBgColor( Color( 0, 0, 0, 0 ) );
	SetPaintBackgroundType( 0 );

	m_iTeamBackground = vgui::surface()->DrawGetTextureId( "VGUI/hud/GameplayHUDBg_Small" );
	if ( m_iTeamBackground == -1 )
		m_iTeamBackground = vgui::surface()->CreateNewTextureID();
	vgui::surface()->DrawSetTextureFile( m_iTeamBackground, "VGUI/hud/GameplayHUDBg_Small", true, true );
}

void CHudTeamScores::Paint( void )
{
	int teamFontTall = vgui::surface()->GetFontTall( m_hTeamFont );
	int boxTall = teamFontTall * (MAX_GE_TEAMS - FIRST_GAME_TEAM) + m_iTextY*2;

	SetTall( boxTall );

	// Draw the background first
	vgui::surface()->DrawSetTexture( m_iTeamBackground );
	vgui::surface()->DrawSetColor( 255, 255, 255, 255 );
	vgui::surface()->DrawTexturedRect( 0, GetTall()-boxTall, GetWide(), GetTall() );

	// This is where our team scores are going
	int leftalign = m_iTextX, 
		rightalign = GetWide() - m_iTextX, 
		y = GetTall() - m_iTextY - teamFontTall;

	wchar_t		TeamName[32], TeamScore[8];
	C_Team		*pTeam		= NULL;
	int			iLeader = TEAM_UNASSIGNED, iHighScore = 0, iScore = 0;;

	for ( int i=FIRST_GAME_TEAM; i < MAX_GE_TEAMS; ++i )
	{
		pTeam = GetGlobalTeam( i );
		if ( !pTeam )
			continue;

		// When we are in intermission time the round score is already applied to the match score
		iScore = GetGlobalTeam(i)->GetRoundScore();
	
		// Convert the team name into unicode
		g_pVGuiLocalize->ConvertANSIToUnicode( pTeam->Get_Name(), TeamName, sizeof(TeamName) );

		// Add the current round score to the match score and display the total score
		_snwprintf(TeamScore, 8, L"%i", iScore );
		int w, h;
		vgui::surface()->GetTextSize( m_hTeamFont, TeamScore, w, h );

		PaintString( &TeamName[0], m_TeamColors[i], m_hTeamFont, leftalign, y );
		PaintString( &TeamScore[0], m_TeamColors[i], m_hTeamFont, rightalign - w, y );

		if ( i == C_BasePlayer::GetLocalPlayer()->GetTeamNumber() && m_pCurrTeamIcon )
		{
			// Draw our current team indicator
			int size = m_iTextX * 0.85;
			int pos = y + (teamFontTall - size)/2;
			m_pCurrTeamIcon->DrawSelf( 2, pos, size, size, Color(255,255,255,255) );
		}

		// If we are taking the lead draw the blur (if required)
		if ( i == m_iTeamLeader )
		{
			for (float fl = m_flTeamBlur; fl > 0.0f; fl -= 1.0f)
			{
				if (fl >= 1.0f)
				{
					PaintString( &TeamName[0], m_TeamColors[i], m_hTeamFontPulsing, leftalign, y );
					PaintString( &TeamScore[0], m_TeamColors[i], m_hTeamFontPulsing, rightalign - w, y );
				}
				else
				{
					// draw a percentage of the last one
					Color col = m_TeamColors[i];
					col[3] *= fl;
					PaintString( &TeamName[0], col, m_hTeamFontPulsing, leftalign, y );
					PaintString( &TeamScore[0], col, m_hTeamFontPulsing, rightalign - w, y );
				}
			}
		}

		y -= teamFontTall;

		// Determine a new leader if possible
		if ( iHighScore < iScore )
		{
			iHighScore = iScore;
			iLeader = i;
		}
		else if ( iHighScore == iScore )
		{
			// We have a tie in a high score, null out any supposed "leader"
			iLeader = TEAM_UNASSIGNED;
		}
	}

	// If we found a new leader start the pulsing effect again
	if ( iLeader != m_iTeamLeader )
	{
		m_iTeamLeader = iLeader;
		m_pHudAnims->StartAnimationSequence( "TeamplayScoreLeader" );
	}
}

void CHudTeamScores::PaintString( const wchar_t *text, Color col, vgui::HFont& font, int x, int y )
{
	vgui::surface()->DrawSetTextColor( col );
	vgui::surface()->DrawSetTextFont( font );
	vgui::surface()->DrawSetTextPos( x, y );
	vgui::surface()->DrawUnicodeString( text );
}
