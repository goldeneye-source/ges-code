///////////// Copyright © 2010 LodleNet. All rights reserved. /////////////
//
//   Project     : Client
//   File        : ge_panelhelper.cpp
//   Description :
//      [TODO: Write the purpose of ge_panelhelper.cpp.]
//
//   Created On: 2/27/2010 4:07:20 PM
//   Created By: Mark Chandler <mailto:mark@moddb.com>
////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ge_panelhelper.h"
#include "ge_gamerules.h"
#include "clientmode_ge.h"
#include "ge_musicmanager.h"
#include "ge_loadingscreen.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Helper functions to start and stop the menu music
void StopMenuMusic( void )
{
	if ( GEMusicManager() )
		GEMusicManager()->PauseMusic();
}

void StartMenuMusic( void )
{
	if ( GEMusicManager() )
	{
		// Reload our play list if we changed it (and are NOT in game)
		if ( !IsInGame() && Q_stricmp( GE_MUSIC_MENU, GEMusicManager()->CurrentPlaylist() ) )
		{
			GEMusicManager()->LoadPlayList( GE_MUSIC_MENU );
		}
		else
		{
			GEMusicManager()->ResumeMusic();
		}
	}

	if ( GetLoadingScreen() )
		GetLoadingScreen()->Reset();
}

bool IsInGame( void )
{
	// Since detecting g_pGameRules is unreliable we look to the world entity
	return g_pEntityList->LookupEntityByNetworkIndex(0) != NULL;
}

CUtlVector<IGEGameUI*> *g_pvPanelList = NULL;

// Called from our children who want to be part of the PANEL_GAMEUIDLL family
void registerGEPanel( IGEGameUI* panel )
{
	if ( !g_pvPanelList )
		g_pvPanelList = new CUtlVector<IGEGameUI*>();

	g_pvPanelList->AddToTail(panel);
}

// Called in vgui_int.cpp, VGui_CreateGlobalPanels(), the parent is PANEL_GAMEUIDLL
void createGEPanels( vgui::VPANEL parent )
{
	if ( !g_pvPanelList )
		return;

	for ( int x=0; x < g_pvPanelList->Count(); x++ )
	{
		g_pvPanelList->Element(x)->Create(parent);
	}
}

// Called in vgui_int.cpp, VGui_Shutdown()
void destroyGEPanels()
{
	if ( !g_pvPanelList )
		return;

	for ( int x=0; x < g_pvPanelList->Count(); x++ )
	{
		if ( g_pvPanelList->Element(x)->GetPanel() )
		{
			g_pvPanelList->Element(x)->Destroy();
		}
	}
}