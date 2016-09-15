///////////// Copyright © 2011, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_loadingscreen.cpp
// Description:
//      VGUI definition for the map specific loading screen
//
// Created On: 16 Oct 2011
// Created By: Jonathan White <killermonkey>
/////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include <vgui_controls/Controls.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/ILocalize.h>
#include <vgui/IVGui.h>
#include "GameUI/IGameUI.h"
#include "materialsystem/itexture.h"
#include "filesystem.h"
#include "clientmode_ge.h"
#include "ge_loadingscreen.h"
#include "ge_panelhelper.h"
#include "ge_musicmanager.h"
#include "ge_shareddefs.h"
#include "ge_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static GameUI<CGELevelLoadingPanel> g_CGELevelLoadingPanel;

#define LOADING_TIP_DURATION	20.0f

CGELevelLoadingPanel *GetLoadingScreen()
{
	return g_CGELevelLoadingPanel.GetNativePanel();
}

CDllDemandLoader g_GEGameUI( "gameui" );

CGELevelLoadingPanel::CGELevelLoadingPanel( vgui::VPANEL parent ) : BaseClass( NULL, "GESLevelLoad" )
{
	SetParent( parent );

	// load the new scheme early!!
	SetScheme( vgui::scheme()->LoadSchemeFromFile("resource/ClientScheme.res", "ClientScheme") );
	SetProportional( true );

	LoadControlSettings("resource/GESLevelLoading.res");
	SetSize( ScreenWidth(), ScreenHeight() );

	InvalidateLayout();

	SetPaintBackgroundEnabled(false);
	SetPaintBorderEnabled(false);
	SetVisible(false);

	m_pImagePanel = (vgui::ImagePanel*) FindChildByName("LevelImage");
	m_pImagePanel->SetSize( ScreenWidth(), ScreenHeight() );

	m_pTipContainer = (vgui::GECreditsText*) FindChildByName("Tips");

	ListenForGameEvent( "server_mapannounce" );

	Reset();

	// Hook our loading screen panel
	CreateInterfaceFn gameUIFactory = g_GEGameUI.GetFactory();
	if ( gameUIFactory )
	{
		IGameUI *gameUI = (IGameUI *) gameUIFactory(GAMEUI_INTERFACE_VERSION, NULL );
		if ( gameUI )
		{
			// insert custom loading panel for the loading dialog
			gameUI->SetLoadingBackgroundDialog( GetVPanel() );
		}
	}
}

CGELevelLoadingPanel::~CGELevelLoadingPanel()
{
}

void CGELevelLoadingPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	m_pTipContainer->SetFgColor( Color(0,0,0,255) );
}

// Called from ge_panelhelper.cpp in StartMenuMusic
void CGELevelLoadingPanel::Reset()
{
	m_flNextTipTime = 0;
	m_bMapImageSet = false;
	SetMapImage( "_default" );
}

void CGELevelLoadingPanel::OnThink()
{
	if ( Plat_FloatTime() > m_flNextTipTime )
		DisplayTip();
}

void CGELevelLoadingPanel::FireGameEvent( IGameEvent *event )
{
	// Fail safe - triggered by server_mapannounce
	SetMapImage( event->GetString("levelname", "_default") );
}

const char *CGELevelLoadingPanel::GetDefaultImage()
{
	return "loadingscreens/_default_widescreen";
}

void CGELevelLoadingPanel::SetMapImage( const char *levelname )
{
	// Make sure we are sized properly
	SetBounds( 0, 0, ScreenWidth(), ScreenHeight() );

	// Load a tip on the next think
	m_flNextTipTime = Plat_FloatTime();

	// Clear our queue
	GetClientModeGENormal()->ClearPanelQueue();

	// Don't change anything if we are already there
	if ( !Q_stricmp(levelname, m_szCurrentMap) )
		return;

	Q_strncpy( m_szCurrentMap, levelname, 64 );

	char imgFile[256];

	Q_snprintf( imgFile, 256, "vgui/loadingscreens/%s_widescreen", levelname );

	// Check the existance of our material
	if ( IsErrorMaterial( materials->FindMaterial( imgFile, TEXTURE_GROUP_VGUI, false) ) )
	{
		// Invalid background image, use the default
		m_pImagePanel->SetImage( GetDefaultImage() );
	}
	else
	{
		// Set the image, note that we chop off the "vgui/" since this is appended internally
		m_pImagePanel->SetImage( imgFile+5 );
		m_bMapImageSet = true;
	}

	if (!IsWidescreen())
	{
		m_pImagePanel->SetSize(ScreenHeight()*1.778, ScreenHeight());
		m_pImagePanel->SetPos((ScreenWidth() - ScreenHeight()*1.778)*0.5, 0);
	}

	// Setup our music manager
	if ( GEMusicManager() && Q_strnicmp(levelname, "_default", 8) )
		GEMusicManager()->LoadPlayList( levelname );
}

const char *CGELevelLoadingPanel::GetCurrentImage()
{
	if ( m_pImagePanel )
		return m_pImagePanel->GetImageName();

	return GetDefaultImage();
}

void CGELevelLoadingPanel::DisplayTip( void )
{
	const GameplayTip *tip = GetClientModeGENormal()->GetRandomTip();

	if (!Q_strcmp(GetCurrentImage(), GetDefaultImage()))
	{
		m_pTipContainer->SetText(""); // No tips on the default loading screen.
	}
	else if ( tip )
	{
		wchar_t tmp[128];
		g_pVGuiLocalize->ConstructString( tmp, sizeof(tmp), g_pVGuiLocalize->Find("#GE_TIP_TITLE"), 1, tip->name );

		m_pTipContainer->SetFgColor( Color(0,0,0,255) );
		m_pTipContainer->SetText( "" );
		m_pTipContainer->InsertCenterText( false );
		m_pTipContainer->InsertString( tmp );
		
		GEUTIL_ParseLocalization( tmp, 128, tip->line1 );
		m_pTipContainer->InsertChar( L'\n' );
		m_pTipContainer->InsertString( tmp );

		if ( tip->line2[0] != '\0' )
		{
			GEUTIL_ParseLocalization( tmp, 128, tip->line2 );
			m_pTipContainer->InsertChar( L'\n' );
			m_pTipContainer->InsertString( tmp );
		}

		if ( tip->line3[0] != '\0' )
		{
			GEUTIL_ParseLocalization( tmp, 128, tip->line3 );
			m_pTipContainer->InsertChar( L'\n' );
			m_pTipContainer->InsertString( tmp );
		}

		m_pTipContainer->ForceRecalculateLineBreaks();
	}

	// Setup our next tip time
	m_flNextTipTime = Plat_FloatTime() + LOADING_TIP_DURATION;
}

#ifdef _DEBUG
CON_COMMAND( ge_debug_levelload, "Show the level load panel" )
{
	if ( args.ArgC() > 1 )
	{
		GetLoadingScreen()->SetVisible( true );
		GetLoadingScreen()->SetZPos( 3 );
	}
	else
	{
		GetLoadingScreen()->SetVisible( false );
		GetLoadingScreen()->SetZPos( -1 );
	}
}

CON_COMMAND( ge_debug_levelloadmap, "Set the map for level load" )
{
	if ( args.Arg(1) )
		GetLoadingScreen()->SetMapImage( args.Arg(1) );
}
#endif
