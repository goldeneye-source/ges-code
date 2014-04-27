///////////// Copyright � 2011, Goldeneye: Source. All rights reserved. /////////////
// 
// ge_huddebugscreen.cpp
//
// Description:
//      Generic hud overlay to show debug information from client / server
//
// Created On: 28 Sep 11
// Created By: Jonathan White <killermonkey>
////////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "vgui/ISurface.h"
#include "vgui/ILocalize.h"
#include "vgui_controls/Panel.h"
#include "ge_shareddefs.h"
#include "gemp_gamerules.h"
#include "c_ge_playerresource.h"
#include "c_ge_radarresource.h"
#include "c_ge_player.h"
#include "c_team.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

class CGEDebugScreen : public CHudElement, public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE( CGEDebugScreen, vgui::Panel );

	CGEDebugScreen( const char *pElementName );
	
	virtual void Init() { }
	virtual void VidInit() { }
	virtual void Reset() { }

	virtual void ApplySchemeSettings( vgui::IScheme *scheme );
	virtual bool ShouldDraw();

	virtual void Paint();

protected:
	void DrawRadarDebug();
	void DrawObjectiveDebug();
	void DrawAimMode();

private:
	vgui::HFont		m_hDebugFont;
};

// Debugging convars (cheats!)
ConVar ge_debug_showdata( "ge_debug_showdata", "", FCVAR_CLIENTDLL | FCVAR_CHEAT, "Show debug data from various sources onscreen [radar, objectives]" );

DECLARE_HUDELEMENT( CGEDebugScreen );

CGEDebugScreen::CGEDebugScreen( const char *pElementName ) 
	: CHudElement( pElementName ), BaseClass( NULL, "GEDebugScreen" ) 
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( 0 );
}

void CGEDebugScreen::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );
	m_hDebugFont = scheme->GetFont( "Default", true );
}

bool CGEDebugScreen::ShouldDraw( void )
{
	if ( ge_debug_showdata.GetString()[0] != '\0' )
		return true;

	return false;
}

void CGEDebugScreen::Paint( void )
{
	// Draw debug information if requested
	if ( !Q_stricmp( ge_debug_showdata.GetString(), "radar" ) )
		DrawRadarDebug();
	else if ( !Q_stricmp( ge_debug_showdata.GetString(), "objectives" ) )
		DrawObjectiveDebug();
	else if ( !Q_stricmp( ge_debug_showdata.GetString(), "aimmode" ) )
		DrawAimMode();
}

void CGEDebugScreen::DrawRadarDebug( void )
{
	CBaseEntity *pEnt;

	int lineX = XRES(60);
	int lineY = YRES(60);
	int lineHeight = surface()->GetFontTall( m_hDebugFont ) + YRES(4);
	int lineMaxLen = XRES(100);
	char line[256];
	wchar_t wcLine[256];
	Color c;

	surface()->DrawSetTextFont( m_hDebugFont );

	for ( int i=0; i < MAX_NET_RADAR_ENTS; i++ )
	{
		// Check to break up the list
		if ( i == (MAX_NET_RADAR_ENTS/2)+1 )
		{
			lineX += lineMaxLen + XRES(10);
			lineY = YRES(60);
		}

		c.SetColor( 255, 255, 255, 255 );
		
		if ( g_RR->GetEntSerial(i) != INVALID_EHANDLE_INDEX )
		{
			const char *szClassname = "[Outside PVS]";
			Vector loc = g_RR->GetOrigin(i);
			const char *szAlwaysVis = g_RR->GetAlwaysVisible(i) ? "All Vis" : "-";
			
			// Set our color to what it would be on the radar
			if ( g_RR->GetColor(i).a() != 0 )
				c = g_RR->GetColor(i);
			else if ( g_RR->GetEntTeam(i) == TEAM_MI6 )
				c.SetColor( 134, 210, 255, 255 );
			else if ( g_RR->GetEntTeam(i) == TEAM_JANUS )
				c.SetColor( 202, 77, 77, 255 );
			else
				c.SetColor( 8, 186, 4, 255 );

			pEnt = g_RR->GetEnt(i);
			if ( pEnt )
				szClassname = pEnt->GetClassname();
			
			Q_snprintf( line, 256, "%i. %s, [%.0f,%.0f,%.0f], %s, %s", i+1, szClassname, loc.x, loc.y, loc.z, szAlwaysVis, g_RR->GetIcon(i) );
		}
		else
		{
			Q_snprintf( line, 256, "%i. Empty", i+1 );
		}

		// Convert to wide character
		g_pVGuiLocalize->ConvertANSIToUnicode( line, wcLine, sizeof(wcLine) );

		// Draw the text
		surface()->DrawSetTextColor( c );
		surface()->DrawSetTextPos( lineX, lineY );
		surface()->DrawPrintText( wcLine, wcslen(wcLine) );

		// Store our max width
		int tmpX, tmpY;
		surface()->GetTextSize( m_hDebugFont, wcLine, tmpX, tmpY );
		if ( tmpX > lineMaxLen )
			lineMaxLen = tmpX;

		// Go down a line
		lineY += lineHeight;
	}
}

void CGEDebugScreen::DrawObjectiveDebug( void )
{
	int lineX = XRES(60);
	int lineY = YRES(60);
	int lineHeight = surface()->GetFontTall( m_hDebugFont ) + YRES(4);
	int lineMaxLen = XRES(100);
	char line[256];
	wchar_t wcLine[256];

	surface()->DrawSetTextFont( m_hDebugFont );

	for( int i=0; i < MAX_NET_RADAR_ENTS; ++i )
	{
		if ( g_RR->IsObjective(i) )
		{
			const char *token = g_RR->GetObjectiveToken(i);
			const char *text  = g_RR->GetObjectiveText(i);
			Q_snprintf( line, 256, "%i. %s, %s, %s",
				i+1, 
				g_RR->GetObjectiveTeam(i) == TEAM_UNASSIGNED ? "TEAM_NONE" : (g_RR->GetObjectiveTeam(i) == TEAM_MI6 ? "TEAM_MI6" : "TEAM_JANUS"),
				token[0] ? token : "-",
				text[0]  ? text  : "-" );

			// Convert to wide character
			g_pVGuiLocalize->ConvertANSIToUnicode( line, wcLine, sizeof(wcLine) );

			// Draw the text
			surface()->DrawSetTextColor( g_RR->GetObjectiveColor(i) );
			surface()->DrawSetTextPos( lineX, lineY );
			surface()->DrawPrintText( wcLine, wcslen(wcLine) );

			// Store our max width
			int tmpX, tmpY;
			surface()->GetTextSize( m_hDebugFont, wcLine, tmpX, tmpY );
			if ( tmpX > lineMaxLen )
				lineMaxLen = tmpX;

			// Go down a line
			lineY += lineHeight;
		}
	}
}

void CGEDebugScreen::DrawAimMode()
{
	int lineX = XRES(60);
	int lineY = YRES(60);
	wchar_t wcLine[256] = L"";

	surface()->DrawSetTextFont( m_hDebugFont );
	surface()->DrawSetTextColor( Color( 255, 255, 255, 255 ) );

	C_GEPlayer *localPlayer = (C_GEPlayer*) C_GEPlayer::GetLocalPlayer();
	if ( localPlayer )
	{
		int mode = localPlayer->GetAimModeState();
		if ( mode == AIM_NONE )
			swprintf( wcLine, 256, L"AIM: None; FOV: %0.1f; Zoom: %0.1f", localPlayer->GetFOV(), localPlayer->GetZoom() );
		else if ( mode == AIM_ZOOM_IN )
			swprintf( wcLine, 256, L"AIM: Zooming In; FOV: %0.1f; Zoom: %0.1f", localPlayer->GetFOV(), localPlayer->GetZoom() );
		else if ( mode == AIM_ZOOM_OUT )
			swprintf( wcLine, 256, L"AIM: Zooming Out; FOV: %0.1f; Zoom: %0.1f", localPlayer->GetFOV(), localPlayer->GetZoom() );
		else if ( mode == AIM_ZOOMED )
			swprintf( wcLine, 256, L"AIM: Zoomed; FOV: %0.1f; Zoom: %0.1f", localPlayer->GetFOV(), localPlayer->GetZoom() );

		surface()->DrawSetTextPos( lineX, lineY );
		surface()->DrawPrintText( wcLine, wcslen(wcLine) );
	}
}