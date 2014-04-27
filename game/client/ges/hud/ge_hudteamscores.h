///////////// Copyright � 2009, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_hudteamscores.cpp
// Description:
//      Shows your current team and the Round team scores
//
// Created On: 10 Dec 09
// Created By: Killermonkey
/////////////////////////////////////////////////////////////////////////////
#ifndef GE_HUDTEAMSCORES
#define GE_HUDTEAMSCORES

#include "hud.h"
#include "hudelement.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/RichText.h"

class CHudTeamScores : public vgui::Panel, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudTeamScores, vgui::Panel );

public:
	CHudTeamScores( const char *pElementName );
	virtual void Reset();
	virtual void VidInit();
	virtual bool ShouldDraw();

protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Paint();

private:
	void PaintString( const wchar_t *text, Color col, vgui::HFont& font, int x, int y );

	vgui::AnimationController *m_pHudAnims;

	// Keep track of our last known leader
	int	m_iTeamLeader;
	Color m_TeamColors[MAX_GE_TEAMS];
	int	m_iTeamBackground;	// Team Background

	CHudTexture *m_pCurrTeamIcon;

	CPanelAnimationVarAliasType( int, m_iTextX, "text_xpad", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTextY, "text_ypad", "8", "proportional_int" );
	
	CPanelAnimationVar( vgui::HFont, m_hTeamFont, "team_font", "Default" );
	CPanelAnimationVar( vgui::HFont, m_hTeamFontPulsing, "team_font_blur", "Default" );
	
	// Define these to control specific parts of the HUD via a scripted animation
	CPanelAnimationVar( float, m_flTeamBlur, "TeamBlur", "0" );
};

#endif
