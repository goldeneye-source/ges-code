///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_spectatorgui.cpp
// Description:
//      VGUI definition for the spectator
//
// Created On: 12 Dec 08
// Created By: Jonathan White <killermonkey>
/////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include <cdll_client_int.h>
#include <globalvars_base.h>
#include <cdll_util.h>
#include <KeyValues.h>

#include "ge_spectatorgui.h"
#include "ge_utils.h"

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IPanel.h>
#include <vgui_controls/ImageList.h>
#include <vgui_controls/MenuItem.h>
#include <vgui_controls/TextImage.h>

#include <stdio.h> // _snprintf define

#include <game/client/iviewport.h>
#include "commandmenu.h"
#include "hltvcamera.h"

#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Menu.h>
#include "IGameUIFuncs.h" // for key bindings
#include <imapoverview.h>
#include <shareddefs.h>
#include "ge_gamerules.h"
#include "gemp_gamerules.h"
#include "c_ge_playerresource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar cl_spec_mode;

CGESpectatorGUI::CGESpectatorGUI(IViewPort *pViewPort) : CSpectatorGUI(pViewPort)
{
	m_pScheme = NULL;
}


bool CGESpectatorGUI::NeedsUpdate( void )
{
	if ( !C_BasePlayer::GetLocalPlayer() )
		return false;

	if ( m_nLastSpecMode != C_BasePlayer::GetLocalPlayer()->GetObserverMode() )
		return true;

	// If we are currently observering SOMEONE we need to update all the time (health/armor)
	if ( C_BasePlayer::GetLocalPlayer()->GetObserverTarget() )
		return true;

	return BaseClass::NeedsUpdate();
}

void CGESpectatorGUI::Update()
{
	C_GEPlayer *pLocalPlayer = ToGEPlayer( C_BasePlayer::GetLocalPlayer() );

	if( !pLocalPlayer )
		return;

	m_nLastSpecMode = pLocalPlayer->GetObserverMode();
	m_hLastSpecTarget.Set( pLocalPlayer->GetObserverTarget() );

	int wide, tall;
	int bx, by, bwide, btall;

	GetHudSize(wide, tall);
	m_pTopBar->GetBounds( bx, by, bwide, btall );

	bool show_player = ShouldShowPlayerLabel( GetSpectatorMode() );
	m_pPlayerLabel->SetVisible( show_player );

	// Update player name field
	if ( show_player && GEPlayerRes() )
	{
		C_GEPlayer *pObserved = ToGEPlayer( m_hLastSpecTarget.Get() );
		if ( pObserved )
		{
			int playernum = pObserved->entindex();

			// Set the player's name in their team color
			int team = GEPlayerRes()->GetTeam( playernum );
			if ( !team )
				m_pPlayerLabel->SetFgColor( m_pScheme->GetColor( "SpecGUI_Default", Color(0,0,0,255) ) );
			else if ( team == TEAM_JANUS )
				m_pPlayerLabel->SetFgColor( m_pScheme->GetColor( "SpecGUI_Janus", Color(0,0,0,255) ) );
			else
				m_pPlayerLabel->SetFgColor( m_pScheme->GetColor( "SpecGUI_MI6", Color(0,0,0,255) ) );

			// Populate the player's name
			char player_name[64];
			Q_strncpy( player_name, UTIL_SafeName( GEPlayerRes()->GetPlayerName( playernum ) ), 64 );
			GEUTIL_RemoveColorHints( player_name );

			// Calculate their health and armor for display
			int health = 0, armor = 0;

			if ( pObserved->GetMaxHealth() > 0 )
				health = (int) ((float) max(pObserved->GetHealth(), 0) * 100.0f / pObserved->GetMaxHealth() );

			if ( pObserved->GetMaxArmor() > 0 )
				armor = (int) ((float) max(pObserved->GetArmor(), 0) * 100.0f / pObserved->GetMaxArmor() );

			// Build the localization string (we use %%%% due to key binding syntax...)
			char label_text[128] = "";
			Q_snprintf( label_text, 128, "#Spec_Playername\r%s\r%i%%%%\r%i%%%%", player_name, health, armor );
			
			// Localize the above strings into the standard display and populate our label
			wchar_t wcs_label_text[128];
			GEUTIL_ParseLocalization( wcs_label_text, sizeof( wcs_label_text ), label_text );

			m_pPlayerLabel->SetText( wcs_label_text );
		}
	}
	else
	{
		m_pPlayerLabel->SetText( L"" );
	}

	// update extra info field
	wchar_t szEtxraInfo[1024];
	char tempstr[128];

	// Show map name
	Q_FileBase( engine->GetLevelName(), tempstr, sizeof(tempstr) );

	wchar_t wMapName[64];
	g_pVGuiLocalize->ConvertANSIToUnicode(tempstr,wMapName,sizeof(wMapName));
	g_pVGuiLocalize->ConstructString( szEtxraInfo,sizeof( szEtxraInfo ), g_pVGuiLocalize->Find("#Spec_Map" ),1, wMapName );

	SetLabelText("extrainfo", szEtxraInfo );
}

void CGESpectatorGUI::OnThink( void )
{
	BaseClass::OnThink();

	if ( !m_pScoreboard )
		FindOtherPanels();

	if ( !GEMPRules() )
		return;

	if ( GEMPRules()->IsIntermission() )
		SetAlpha( 90 );
	else if ( m_pCharMenu->IsVisible() || m_pTeamMenu->IsVisible() || m_pScoreboard->IsVisible() )
		SetAlpha( 120 );
	else
		SetAlpha( 255 );

	// If we are chasing or viewing in eye and we want to choose a character, force the mode to be roaming so that the
	// character preview doesn't bounce around
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pLocalPlayer && m_pCharMenu->IsVisible() && ( pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE || pLocalPlayer->GetObserverMode() == OBS_MODE_CHASE ) )
	{
		char specmode[16];
		Q_snprintf( specmode, 16, "spec_mode %i", OBS_MODE_ROAMING );
		engine->ClientCmd( specmode );
	}
}

void CGESpectatorGUI::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_pScheme = pScheme;

	// Update our pointers
	FindOtherPanels();

	int w,h;
	GetHudSize( w, h );

	// Fix us up for wide screen users
	vgui::Panel *control = FindChildByName( "bottombarimg" );
	if ( control )
		control->SetWide(w);

	control = FindChildByName( "topbarimg" );
	if ( control )
		control->SetWide(w);

	// Get rid of these stupid bars
	m_pBottomBarBlank->SetVisible( false );
	m_pTopBar->SetVisible( false );
}

void CGESpectatorGUI::FindOtherPanels( void )
{
	m_pScoreboard = gViewPortInterface->FindPanelByName( PANEL_SCOREBOARD );
	m_pTeamMenu = gViewPortInterface->FindPanelByName( PANEL_TEAM );
	m_pCharMenu = gViewPortInterface->FindPanelByName( PANEL_CHARSELECT );
	m_pRoundReport = gViewPortInterface->FindPanelByName( PANEL_ROUNDREPORT );
}
