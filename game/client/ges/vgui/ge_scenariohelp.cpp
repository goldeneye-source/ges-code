///////////// Copyright � 2011, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_scenariohelp.cpp
// Description:
//      [See Header]
//
// Created On: 09 Feb 2012
// Created By: Jonathan White <killermonkey>
/////////////////////////////////////////////////////////////////////////////
#include "cbase.h"

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include "ge_CreditsText.h"
#include "ge_scenariohelp.h"
#include "ge_loadingscreen.h"
#include "ge_utils.h"
#include "c_ge_gameplayresource.h"
#include "clientmode_ge.h"
#include "hud_macros.h"
#include "filesystem.h"
#include "networkstringtable_clientdll.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define GE_HELP_SETTINGS_FILE "cfg/ges_helpsettings.txt"

void __MsgFunc_CGEScenarioHelp_ScenarioHelp( bf_read &msg );
void __MsgFunc_CGEScenarioHelp_ScenarioHelpPane( bf_read &msg );
void __MsgFunc_CGEScenarioHelp_ScenarioHelpShow( bf_read &msg );

CGEScenarioHelp::CGEScenarioHelp(IViewPort *pViewPort) : EditablePanel(NULL, PANEL_SCENARIOHELP )
{
	// load the new scheme early!!
	SetScheme( "ClientScheme" );
	SetProportional( true );

	LoadControlSettings("resource/UI/ScenarioHelp.res");

	InvalidateLayout();
	MakePopup();

	m_pScenarioName		= (Label*) FindChildByName( "ScenarioName" );
	m_pScenarioTagLine	= (Label*) FindChildByName( "ScenarioTagLine" );
	m_pHelpContainer	= (GEVertListPanel*) FindChildByName( "HelpContainer" );

	m_pBackground = (ImagePanel*) FindChildByName( "background" );
	m_pBackground->SetImage( GetLoadingScreen()->GetDefaultImage() );

	ivgui()->AddTickSignal( GetVPanel(), 200 );

	HOOK_HUD_MESSAGE( CGEScenarioHelp, ScenarioHelp );
	HOOK_HUD_MESSAGE( CGEScenarioHelp, ScenarioHelpPane );
	HOOK_HUD_MESSAGE( CGEScenarioHelp, ScenarioHelpShow );

	m_pHelpSettings = new KeyValues( "HelpSettings" );
	m_pHelpSettings->LoadFromFile( filesystem, GE_HELP_SETTINGS_FILE, "MOD" );

	m_szTagLine[0] = m_szWebsite[0] = '\0';
	m_szNextPanel[0] = '\0';
	m_szScenarioIdent[0] = '\0';
	m_iCurrentPane = m_iDefaultPane = 1;
	m_bWaitForResolve = false;
	m_bPlayerRequestShow = false;
	m_flSoundVolume = -1;
}

CGEScenarioHelp::~CGEScenarioHelp()
{
	ClearHelp();
	
	m_pHelpSettings->SaveToFile( filesystem, GE_HELP_SETTINGS_FILE, "MOD" );
	m_pHelpSettings->deleteThis();
}

void CGEScenarioHelp::ApplySchemeSettings( IScheme *pScheme )
{
	SetBounds( 0, 0, ScreenWidth(), ScreenHeight() );
	m_pBackground->SetBounds( 0, 0, ScreenWidth(), ScreenHeight() );

	BaseClass::ApplySchemeSettings( pScheme );
}

void CGEScenarioHelp::SetData( KeyValues *data )
{
	m_iCurrentPane = data->GetInt( "pane", -1 );
	Q_strncpy( m_szNextPanel, data->GetString( "nextpanel" ), 32 );
}

void CGEScenarioHelp::ShowPanel( bool bShow )
{
	// Dim sound effects when visible
	ConVar *pVol = g_pCVar->FindVar( "volume" );

	if ( bShow )
	{
		if ( m_iCurrentPane == -1 )
			m_iCurrentPane = m_iDefaultPane;
		
		if ( ShouldShowPane( m_iCurrentPane ) )
		{
			SetSize( ScreenWidth(), ScreenHeight() );
			m_pBackground->SetBounds( 0, 0, GetWide(), GetTall() );
			m_pBackground->SetImage( GetLoadingScreen()->GetCurrentImage() );

			InvalidateLayout( true );

			SetVisible( true );
			SetMouseInputEnabled( true );

			m_flSoundVolume = pVol->GetFloat();
			pVol->SetValue( "0.1" );

			// Set us up to show information when we get it
			m_bWaitForResolve = true;

			SetPlayerRequestShow( false );
			return;
		}
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );

		if ( m_flSoundVolume >= 0 )
			pVol->SetValue( m_flSoundVolume );
	}

	// Catch all if we don't end up showing the panel
	if ( m_szNextPanel[0] )
	{
		gViewPortInterface->ShowPanel( m_szNextPanel, true );
		m_szNextPanel[0] = '\0';
	}
}

void CGEScenarioHelp::SetupPane( int id )
{
	sHelpPane *pPane = FindPane( id );
	if ( !pPane )
		return;

	// Are we resolved yet?
	if ( !pPane->str_resolved )
		return;

	m_pScenarioName->SetText( GEGameplayRes()->GetGameplayName() );
	m_pScenarioTagLine->SetText( m_szTagLine );

	CheckButton *cb = (CheckButton*) FindChildByName( "DontShowButton" );
	if ( cb )
		cb->SetSelected( !ShouldShowPane(id) );

	vgui::HFont hFont = scheme()->GetIScheme(GetScheme())->GetFont("TypeWriter", true);
	int max_w = m_pHelpContainer->GetWide() - XRES(10);
	int max_h = 300;

	m_pHelpContainer->ClearItems();
	for ( int i=0; i < pPane->help.Count(); i++ )
	{
		int w = max_w, h = max_h;

		IImage *img = pPane->help[i]->image;
		if ( img )
		{
			img->GetContentSize( w, h );
			float ratio = w / (float) h;
			w = min( XRES(w), max_w );
			h = w / ratio;

			if ( h > max_h )
			{
				w = max_h * ratio;
				h = max_h;
			}

			ImagePanel *ip = new ImagePanel( NULL, "image" );
			ip->SetImage( pPane->help[i]->image );
			ip->SetShouldScaleImage( true );
			ip->SetSize( w, h );
			m_pHelpContainer->AddItem( ip, GEVertListPanel::LF_CENTERED );
		}
		
		if ( pPane->help[i]->desc[0] != '\0' )
		{
			GECreditsText *text = new GECreditsText( NULL, NULL );
			text->SetVerticalScrollbar( false );
			text->SetWide( min( w * 1.5f, max_w ) );
			text->SetFont( hFont );
			text->SetFgColor( Color(0,0,0,255) );
			text->InsertCenterText( true );
			text->InsertFontChange( hFont );
			text->InsertString( pPane->help[i]->desc );
			text->SetToFullHeight();
			m_pHelpContainer->AddItem( text, GEVertListPanel::LF_CENTERED );
		}

		Panel *spacer = new Panel();
		spacer->SetBounds( 0, 0, m_pHelpContainer->GetWide() - 20, YRES(5) );
		m_pHelpContainer->AddItem( spacer, GEVertListPanel::LF_CENTERED );
	}

	// Mark us done
	m_bWaitForResolve = false;
}

bool CGEScenarioHelp::ShouldShowPane( int id )
{
	sHelpPane *pPane = FindPane(id);
	if ( !pPane )
		return false;

	if ( m_bPlayerRequestShow )
		return true;

	KeyValues *kv = m_pHelpSettings->FindKey( m_szScenarioIdent );
	if ( kv )
		return kv->GetInt( pPane->id, 1 ) != 0;

	return true;
}

void CGEScenarioHelp::OnTick( void )
{
	for ( int i=0; i < m_vHelpPanes.Count(); i++ )
		ResolveHelpStrings( m_vHelpPanes[i] );

	if ( m_bWaitForResolve && m_iCurrentPane != -1 )
		SetupPane( m_iCurrentPane );	
}

void CGEScenarioHelp::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "ok" ) )
	{
		sHelpPane *pPane = FindPane( m_iCurrentPane );
		if ( pPane )
		{
			CheckButton *cb = (CheckButton*) FindChildByName( "DontShowButton" );
			KeyValues *kv = m_pHelpSettings->FindKey( m_szScenarioIdent, true );
			if ( cb )
				kv->SetInt( pPane->id, cb->IsSelected() ? 0 : 1 );
		}

		ShowPanel( false );
	}
	else
	{
		BaseClass::OnCommand( command );
	}
}

void CGEScenarioHelp::OnKeyCodePressed(KeyCode code)
{
	// TODO: Do we want to capture escape?
	BaseClass::OnKeyCodePressed( code );
}

void CGEScenarioHelp::MsgFunc_ScenarioHelp( bf_read &msg )
{
	// Always start from a blank slate when we get this message
	ShowPanel( false );
	ClearHelp();

	msg.ReadString( m_szScenarioIdent, 32 );
	msg.ReadString( m_szTagLine, 64 );
	msg.ReadString( m_szWebsite, 128 );
	m_iDefaultPane = msg.ReadByte();
	m_iCurrentPane = -1;
}

void CGEScenarioHelp::MsgFunc_ScenarioHelpPane( bf_read &msg )
{
	int pane_key = msg.ReadByte();
	if ( FindPane( pane_key ) )
	{
		Warning( "[ScenarioHelp] Duplicate pane keys received!\n" );
		return;
	}

	// Create our pane and add it to the list
	sHelpPane* pPane = new sHelpPane;
	pPane->key = pane_key;
	msg.ReadString( pPane->id, 32 );
	pPane->str_idx = msg.ReadShort();
	pPane->str_resolved = false;
	m_vHelpPanes.AddToTail( pPane );
}

void CGEScenarioHelp::MsgFunc_ScenarioHelpShow( bf_read &msg )
{
	GetClientModeGENormal()->QueuePanel( PANEL_SCENARIOHELP, new KeyValues("data", "pane", msg.ReadByte()) );
}

void CGEScenarioHelp::ClearHelp( void )
{
	ShowPanel( false );

	// Remove all elements from our vertical list
	if ( m_pHelpContainer )
		m_pHelpContainer->ClearItems();

	// Clear out the stored help information
	m_vHelpPanes.PurgeAndDeleteElements();

	// Reset our variables
	m_iCurrentPane = m_iDefaultPane = -1;
	m_szTagLine[0] = m_szWebsite[0] = '\0';
	m_bWaitForResolve = false;
}

CGEScenarioHelp::sHelpPane *CGEScenarioHelp::FindPane( int pane_key )
{
	for ( int i=m_vHelpPanes.Count()-1; i >= 0; --i )
	{
		if ( m_vHelpPanes[i]->key == pane_key )
			return m_vHelpPanes[i];
	}

	return NULL;
}

void CGEScenarioHelp::ResolveHelpStrings( sHelpPane *pPane )
{
	if ( !pPane || pPane->str_resolved )
		return;

	// Try to find our string
	const char *help_string = g_pStringTableGameplay->GetString( pPane->str_idx );
	if ( !help_string )
		return;

	// Mark that we found our string
	pPane->str_resolved = true;

	// Pull out the entries
	CUtlVector<char*> entries;
	Q_SplitString( help_string, ";;;", entries );

	// Make sure we are empty
	pPane->help.PurgeAndDeleteElements();

	CUtlVector<char*> help_data;
	for ( int i=0; i < entries.Count(); i++ )
	{
		Q_SplitString( entries[i], "::", help_data );

		if ( help_data.Count() >= 2 )
		{
			// Create a help item and copy the description
			sHelpItem *help_item = new sHelpItem;
			Q_strncpy( help_item->desc, help_data[0], 128 );

			// Resolve the image if valid
			char image_file[128];
			Q_snprintf( image_file, 128, "vgui/hud/gameplayhelp/%s", help_data[1] );

			if ( !IsErrorMaterial( materials->FindMaterial( image_file, TEXTURE_GROUP_VGUI, false) ) )
				help_item->image = scheme()->GetImage( image_file+5, true );
			else
				help_item->image = NULL;
			
			// Add to our pane
			pPane->help.AddToTail( help_item );
		}

		// Cleanup strings
		ClearStringVector( help_data );
	}

	// Final cleanup
	ClearStringVector( entries );
}

// MESSAGE FUNC DEFS
//
void __MsgFunc_CGEScenarioHelp_ScenarioHelp( bf_read &msg )
{
	CGEScenarioHelp *pPanel = (CGEScenarioHelp*) gViewPortInterface->FindPanelByName( PANEL_SCENARIOHELP );
	if ( pPanel )
		pPanel->MsgFunc_ScenarioHelp( msg );
}

void __MsgFunc_CGEScenarioHelp_ScenarioHelpPane( bf_read &msg )
{
	CGEScenarioHelp *pPanel = (CGEScenarioHelp*) gViewPortInterface->FindPanelByName( PANEL_SCENARIOHELP );
	if ( pPanel )
		pPanel->MsgFunc_ScenarioHelpPane( msg );
}

void __MsgFunc_CGEScenarioHelp_ScenarioHelpShow( bf_read &msg )
{
	CGEScenarioHelp *pPanel = (CGEScenarioHelp*) gViewPortInterface->FindPanelByName( PANEL_SCENARIOHELP );
	if ( pPanel )
		pPanel->MsgFunc_ScenarioHelpShow( msg );
}
//
// END

CON_COMMAND( recallhelp, "Show the full-screen scenario help" )
{
	CGEScenarioHelp *panel = (CGEScenarioHelp*) gViewPortInterface->FindPanelByName( PANEL_SCENARIOHELP );
	panel->SetPlayerRequestShow( true );
	panel->ShowPanel( true );
}

#ifdef _DEBUG
CON_COMMAND(edit_scenariohelp, "Edit the panel")
{
	CGEScenarioHelp *panel = (CGEScenarioHelp*) gViewPortInterface->FindPanelByName( PANEL_SCENARIOHELP );
	panel->ShowPanel( true );
	panel->ActivateBuildMode();
}
#endif
