///////////// Copyright � 2009, Goldeneye: Source. All rights reserved. /////////////
// 
// File: clientmode_ge.cpp
// Description:
//      See header
//
// Created On: 13 Jul 09
// Created By: Jonathan White <killermonkey> 
////////////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include "clientmode_ge.h"
#include "vgui_int.h"
#include "hud.h"
#include <vgui/IInput.h>
#include <vgui/IPanel.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/AnimationController.h>
#include "vgui/ILocalize.h"

#include "iinput.h"
#include "ienginevgui.h"
#include "GameUI/IGameUI.h"

#include "ivrenderview.h"
#include "model_types.h"
#include "view_shared.h"
#include "view.h"
#include "dlight.h"
#include "iefx.h"

#include "ge_utils.h"
#include "gemp_gamerules.h"
#include "ge_screeneffects.h"
#include "ge_vieweffects.h"

#include "vgui/ge_spectatorgui.h"
#include "vgui/ge_charselectgui.h"
#include "vgui/ge_teammenu.h"
#include "vgui/ge_scoreboard.h"
#include "vgui/ge_roundreport.h"
#include "vgui/ge_loadingscreen.h"
#include "vgui/ge_scenariohelp.h"

#include "ge_hudgameplay.h"
#include "ge_hudtargetid.h"

// An internal MP3 player for our levels
#include "ge_panelhelper.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Add all these convars because Valve forces us, they don't do anything!
ConVar hap_HasDevice("hap_HasDevice", "0");
ConVar topcolor("topcolor","0");
ConVar bottomcolor("bottomcolor","0");
ConVar cl_himodels("cl_himodels","1");
ConVar cl_crosshairusealpha("cl_crosshairusealpha","0");
ConVar cl_crosshaircolor("cl_crosshaircolor","0");
ConVar cl_crosshair_red("cl_crosshair_red","0");
ConVar cl_crosshair_green("cl_crosshair_green","0");
ConVar cl_crosshair_blue("cl_crosshair_blue","0");
ConVar cl_crosshairscale("cl_crosshairscale","0");
ConVar cl_crosshair_scale("cl_crosshair_scale","0");
ConVar cl_crosshair_file("cl_crosshair_file","0");

// Hack support for 3D HUD elements
ConVar cl_ge_draw3dhud( "cl_ge_draw3dhud", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Draw select hud elements in 3D. Use with NVIDIA 3D Vision" );

extern CDllDemandLoader g_GEGameUI;

// Instance the singleton and expose the interface to it.
IClientMode *GetClientModeNormal()
{
	static ClientModeGENormal g_ClientModeNormal;
	return &g_ClientModeNormal;
}


ClientModeGENormal* GetClientModeGENormal()
{
	return (ClientModeGENormal*) GetClientModeNormal();
}

//-----------------------------------------------------------------------------
// Purpose: Send special command to python scenarios
//-----------------------------------------------------------------------------
CON_COMMAND( cl_ge_voodoo, "Send a special command to python scenarios!" )
{
	engine->ClientCmd( "say !voodoo" );
}

CON_COMMAND( cl_ge_voodoo2, "Send a special command to python scenarios!" )
{
	engine->ClientCmd( "say !gesrocks" );
}

//-----------------------------------------------------------------------------
// Purpose: Custom commands for VGUI interaction
//-----------------------------------------------------------------------------
CON_COMMAND( ge_debug_reloadvgui, "Reloads all VGUI panel elements (.res files)" )
{
	// Reload the schemes
	vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme" );
	vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/SourceScheme.res", "SourceScheme" );
	vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/TeamMenuScheme.res", "TeamMenuScheme" );

	// Fake a screen resize to reload the panels
	CBaseViewport *viewport = (CBaseViewport*) GetClientModeGENormal()->GetViewport();
	viewport->PostMessage( viewport, new KeyValues("OnScreenSizeChanged", "oldwide", ScreenWidth(), "oldtall", ScreenHeight()), NULL );

	// Handle our game ui panels
	destroyGEPanels();
	VPANEL uiParent = enginevgui->GetPanel( PANEL_GAMEUIDLL );
	createGEPanels( uiParent );
}

CON_COMMAND( __OVR_map, "Load the specified map." )
{
	const char *map = args.Arg(1);
	if ( map )
	{
		GetLoadingScreen()->SetMapImage( map );
	}

	// Show progress dialog
	CreateInterfaceFn gameUIFactory = g_GEGameUI.GetFactory();
	if ( gameUIFactory )
	{
		IGameUI *gameUI = (IGameUI *) gameUIFactory(GAMEUI_INTERFACE_VERSION, NULL );
		if ( gameUI )
		{
			// insert custom loading panel for the loading dialog
			gameUI->OnLevelLoadingStarted( true );
		}
	}

	engine->ClientCmd_Unrestricted( VarArgs("__real_map %s", args.ArgS()) );
}

void RenderGEPlayerSprites()
{
	// Prevents an assert
	AllowCurrentViewAccess( true );

	CGETargetID *target = GET_HUDELEMENT( CGETargetID );
	if ( target )
		target->DrawOverheadIcons();

	GEViewEffects()->DrawCrosshair();

	AllowCurrentViewAccess( false );
}

//-----------------------------------------------------------------------------
// Purpose: this is the viewport that contains all the hud elements
//-----------------------------------------------------------------------------
class CHudViewport : public CBaseViewport
{
public:
	DECLARE_CLASS_SIMPLE( CHudViewport, CBaseViewport );

protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
	{
		BaseClass::ApplySchemeSettings( pScheme );

		gHUD.InitColors( pScheme );

		SetPaintBackgroundEnabled( false );
	}

	virtual IViewPortPanel *CreatePanelByName( const char *szPanelName );
	virtual void CreateDefaultPanels( void );
};

IViewPortPanel* CHudViewport::CreatePanelByName( const char *szPanelName )
{
	IViewPortPanel* newpanel = NULL;

	if ( Q_strcmp( PANEL_SCOREBOARD, szPanelName) == 0 )
		newpanel = new CGEScoreBoard( this );

	else if ( Q_strcmp(PANEL_SPECGUI, szPanelName) == 0 )
		newpanel = new CGESpectatorGUI( this );	

	else if ( Q_strcmp(PANEL_CHARSELECT, szPanelName) == 0 )
		newpanel = new CGECharSelect( this );

	else if ( Q_strcmp(PANEL_ROUNDREPORT, szPanelName) == 0 )
		newpanel = new CGERoundReport( this );

	else if ( Q_strcmp(PANEL_TEAM, szPanelName) == 0 )
		newpanel = new CGETeamMenu( this );

	else if ( Q_strcmp(PANEL_SCENARIOHELP, szPanelName) == 0 )
		newpanel = new CGEScenarioHelp( this );

	return (newpanel != NULL) ? newpanel : BaseClass::CreatePanelByName( szPanelName ); 
}

void CHudViewport::CreateDefaultPanels( void )
{
	BaseClass::CreateDefaultPanels();
	AddNewPanel( CreatePanelByName( PANEL_CHARSELECT ), "PANEL_CHARSELECT" );
	AddNewPanel( CreatePanelByName( PANEL_TEAM ), "PANEL_TEAM" );
	AddNewPanel( CreatePanelByName( PANEL_ROUNDREPORT ), "PANEL_ROUNDREPORT" );
	AddNewPanel( CreatePanelByName( PANEL_SCENARIOHELP ), "PANEL_SCENARIOHELP" );
}

//-----------------------------------------------------------------------------
// ClientModeHLNormal implementation
//-----------------------------------------------------------------------------
ClientModeGENormal::ClientModeGENormal()
{
	m_pViewport = new CHudViewport();
	m_pViewport->Start( gameuifuncs, gameeventmanager );

	LoadGameplayTips();
}

ClientModeGENormal::~ClientModeGENormal()
{
	UnloadGameplayTips();
}

void ClientModeGENormal::Init()
{
	BaseClass::Init();

	// Swap the map command with our own
	GEUTIL_OverrideCommand( "map", "__real_map", "__OVR_map", "Load the specified map." );
}

void ClientModeGENormal::LevelInit( const char* newmap )
{
	g_pScreenSpaceEffects->EnableScreenSpaceEffect( "ge_entglow" );
	BaseClass::LevelInit( newmap );
}

void ClientModeGENormal::LevelShutdown( void )
{
	g_pScreenSpaceEffects->DisableScreenSpaceEffect( "ge_entglow" );
	BaseClass::LevelShutdown();
}

void ClientModeGENormal::OverrideMouseInput( float *x, float *y )
{
	BaseClass::OverrideMouseInput( x, y );
	GEViewEffects()->BreathMouseMove( x, y );
}

int ClientModeGENormal::GetDeathMessageStartHeight( void )
{
	return m_pViewport->GetDeathMessageStartHeight();
}

void ClientModeGENormal::LoadGameplayTips( void )
{
	UnloadGameplayTips();

	KeyValues *pKV = new KeyValues( "Tips" );
	if ( pKV->LoadFromFile( filesystem, "scripts/ges_gameplay_tips.txt", "MOD" ) )
	{
		// parse out all of the top level sections which are the tips
		KeyValues *pKeys = pKV;
		while ( pKeys )
		{
			GameplayTip *tip = new GameplayTip;
			GEUTIL_ParseLocalization( tip->name, 64, pKeys->GetName() );
			Q_strncpy( tip->line1, pKeys->GetString("1"), 64 );
			Q_strncpy( tip->line2, pKeys->GetString("2"), 64 );
			Q_strncpy( tip->line3, pKeys->GetString("3"), 64 );
			m_vGameplayTips.AddToTail( tip );

			pKeys = pKeys->GetNextKey();
		}
	}

	pKV->deleteThis();
}

void ClientModeGENormal::UnloadGameplayTips( void )
{
	// Simple!
	m_iLastTip = -1;
	m_vGameplayTips.PurgeAndDeleteElements();
}

const GameplayTip *ClientModeGENormal::GetRandomTip( void )
{
	// Return a random (new) tip if we have more than 1 available
	if ( m_vGameplayTips.Count() > 1 )
	{
		int tip_num;
		do {
			tip_num = GERandom<int>( m_vGameplayTips.Count() );
		} while ( tip_num == m_iLastTip );

		m_iLastTip = tip_num;
		return m_vGameplayTips[tip_num];
	}
	else if ( m_vGameplayTips.Count() > 0 )
	{
		return m_vGameplayTips[0];
	}
	
	// No tips...
	return NULL;
}

const char *ClientModeGENormal::GetRandomTipLine( const GameplayTip *tip )
{
	if ( tip )
	{
		int line_count = 0;
		if ( tip->line1[0] != L'\0' )	line_count++;
		if ( tip->line2[0] != L'\0' )	line_count++;
		if ( tip->line3[0] != L'\0' )	line_count++;

		switch ( GERandom<int>( line_count ) )
		{
		case 1:
			return tip->line1;
		case 2:
			return tip->line2;
		case 3:
			return tip->line3;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: We've received a keypress from the engine. Return 1 if the engine is allowed to handle it.
//-----------------------------------------------------------------------------
int	ClientModeGENormal::KeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( engine->Con_IsVisible() )
		return 1;
	
	// Should we start typing a message?
	if ( pszCurrentBinding &&
		( Q_strcmp( pszCurrentBinding, "messagemode" ) == 0 ||
		  Q_strcmp( pszCurrentBinding, "say" ) == 0 ) )
	{
		if ( down )
		{
			StartMessageMode( MM_SAY );
		}
		return 0;
	}
	else if ( pszCurrentBinding &&
				( Q_strcmp( pszCurrentBinding, "messagemode2" ) == 0 ||
				  Q_strcmp( pszCurrentBinding, "say_team" ) == 0 ) )
	{
		if ( down )
		{
			// No teamplay messages if we don't have teams!
			if ( GEMPRules() && !GEMPRules()->IsTeamplay() )
				StartMessageMode( MM_SAY );
			else
				StartMessageMode( MM_SAY_TEAM );
		}
		return 0;
	}
	else if ( pszCurrentBinding && Q_strcmp( pszCurrentBinding, "cl_ge_gameplay_help" ) == 0 )
	{
		if ( down )
		{
			CHudGameplay *elem = GET_HUDELEMENT( CHudGameplay );
			if ( elem )
				elem->KeyboardInput( CHudGameplay::GAMEPLAY_HELP );
		}
		return 0;
	}
	else if ( pszCurrentBinding && Q_strcmp( pszCurrentBinding, "cl_ge_weaponset_list" ) == 0 )
	{
		if ( down )
		{
			CHudGameplay *elem = GET_HUDELEMENT( CHudGameplay );
			if ( elem )
				elem->KeyboardInput( CHudGameplay::GAMEPLAY_WEAPONSET );
		}
		return 0;
	}
	else if (pszCurrentBinding && Q_strcmp(pszCurrentBinding, "cl_ge_weapon_stats") == 0)
	{
		if (down)
		{
			CHudGameplay *elem = GET_HUDELEMENT( CHudGameplay );
			if (elem)
				elem->KeyboardInput(CHudGameplay::GAMEPLAY_WEAPONSTATS);
		}
		return 0;
	}
	else if ( pszCurrentBinding && Q_strcmp( pszCurrentBinding, "cl_ge_gameplay_stats" ) == 0 )
	{
		if ( down )
		{
			CHudGameplay *elem = GET_HUDELEMENT( CHudGameplay );
			if ( elem )
				elem->KeyboardInput( CHudGameplay::GAMEPLAY_STATS );
		}
		return 0;
	}

	return BaseClass::KeyInput( down, keynum, pszCurrentBinding );
}

bool ClientModeGENormal::QueuePanel( const char *panelName, KeyValues *data /*=NULL*/ )
{
	if ( !Q_stricmp( panelName, PANEL_SCENARIOHELP ) )
	{
		AddPanelToQueue( panelName, data, 0 );
		return true;
	}
	else if ( 
		!Q_stricmp( panelName, PANEL_TEAM )			||
		!Q_stricmp( panelName, PANEL_CHARSELECT )	|| 
		!Q_stricmp( panelName, PANEL_ROUNDREPORT ) 
		)
	{
		AddPanelToQueue( panelName, data );
		return true;
	}

	return false;
}
