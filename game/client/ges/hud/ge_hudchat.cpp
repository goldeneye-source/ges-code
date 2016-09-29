///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_hudchat.cpp
// Description:
//      HUD Chat
//
// Created On: 08 Nov 08
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ge_hudchat.h"
#include "hud_macros.h"
#include "text_message.h"
#include "vguicenterprint.h"
#include "vgui/ILocalize.h"
#include "c_team.h"
#include "c_playerresource.h"
#include "c_ge_playerresource.h"
#include "c_ge_player.h"
#include "ge_gamerules.h"
#include "gemp_gamerules.h"
#include "ihudlcd.h"
#include <game/client/iviewport.h>
#include "viewport_panel_names.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT( CHudChat );

DECLARE_HUD_MESSAGE( CHudChat, SayText );
DECLARE_HUD_MESSAGE( CHudChat, SayText2 );
DECLARE_HUD_MESSAGE( CHudChat, TextMsg );

//=====================
//CHudChat
//=====================

CHudChat::CHudChat( const char *pElementName ) : BaseClass( pElementName )
{
	m_IScheme = NULL;
	m_pHudGameplay = NULL;
	m_bIntermissionSet = false;
}

bool CHudChat::ShouldDraw(void)
{
	return true;
}

void CHudChat::CreateChatInputLine( void )
{
	m_pChatInput = new CHudChatInputLine( this, "ChatInputLine" );
	m_pChatInput->SetVisible( false );
}

void CHudChat::CreateChatLines( void )
{
	m_ChatLine = new CHudChatLine( this, "ChatLine1" );
	m_ChatLine->SetVisible( false );	
}

void CHudChat::OnTick( void )
{
	if ( GEMPRules() && m_IScheme && !m_nMessageMode )
	{
		if ( GEMPRules()->IsIntermission() && !m_bIntermissionSet )
		{
			m_bIntermissionSet = true;
			ApplySchemeSettings( m_IScheme );
		}
		else if ( !GEMPRules()->IsIntermission() && m_bIntermissionSet )
		{
			m_bIntermissionSet = false;
			// Reset our view after intermission is over
			ApplySchemeSettings( m_IScheme );
		}
	}

	BaseClass::OnTick();
}

void CHudChat::Init( void )
{
	BaseClass::Init();

	SetZPos( 2 );

	m_pHudGameplay = GET_HUDELEMENT( CHudGameplay );

	HOOK_HUD_MESSAGE( CHudChat, SayText );
	HOOK_HUD_MESSAGE( CHudChat, SayText2 );
	HOOK_HUD_MESSAGE( CHudChat, TextMsg );
}

//-----------------------------------------------------------------------------
// Purpose: Overrides base reset to not cancel chat at round restart
//-----------------------------------------------------------------------------
void CHudChat::Reset( void )
{
}

int CHudChat::GetChatInputOffset( void )
{
	if ( m_pChatInput->IsVisible() )
	{
		return m_iFontHeight;
	}
	else
		return 0;
}

void CHudChat::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	if ( m_bIntermissionSet )
		LoadControlSettings( "resource/UI/BaseChat_Intermission.res" );
	else
		LoadControlSettings( "resource/UI/BaseChat.res" );

	m_IScheme = pScheme;

	// Skip over the BaseChat so we don't reload non-intermission settings
	EditablePanel::ApplySchemeSettings( pScheme );

	SetPaintBackgroundType( 2 );
	SetPaintBorderEnabled( true );
	SetPaintBackgroundEnabled( true );

	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( false );
	m_nVisibleHeight = 0;

	Color cColor = pScheme->GetColor( "DullWhite", GetBgColor() );
	SetBgColor( Color ( cColor.r(), cColor.g(), cColor.b(), CHAT_HISTORY_ALPHA ) );

	GetChatHistory()->SetVerticalScrollbar( false );
}

Color CHudChat::GetClientColor( int clientIndex )
{
	if ( clientIndex == 0 ) // console msg
	{
		return g_ColorYellow;
	}
	else if( GEPlayerRes() )
	{
		vgui::IScheme *pClientScheme = vgui::scheme()->GetIScheme( vgui::scheme()->GetScheme( "ClientScheme" ) );

		switch ( GEPlayerRes()->GetTeam( clientIndex ) )
		{
			case TEAM_JANUS:
				return pClientScheme->GetColor( "GEJanusChatColor", GEPlayerRes()->GetTeamColor(TEAM_JANUS) );

			case TEAM_MI6:
				return pClientScheme->GetColor( "GEMI6ChatColor", GEPlayerRes()->GetTeamColor(TEAM_MI6) );

			default:
				return pClientScheme->GetColor( "GENoTeamChatColor", GEPlayerRes()->GetTeamColor(TEAM_UNASSIGNED) );
		}
	}

	return g_ColorYellow;
}
