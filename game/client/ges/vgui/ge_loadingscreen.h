///////////// Copyright � 2011, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_loadingscreen.cpp
// Description:
//      VGUI definition for the map specific loading screen
//
// Created On: 16 Oct 2011
// Created By: Jonathan White <killermonkey>
/////////////////////////////////////////////////////////////////////////////
#ifndef GE_LOADINGSCREEN_H
#define GE_LOADINGSCREEN_H
#pragma once

#include <vgui/IScheme.h>
#include "vgui_controls/Frame.h"
#include "vgui/ge_CreditsText.h"
#include "GameEventListener.h"

class CGELevelLoadingPanel : public vgui::Frame, public CGameEventListener
{
public:
	DECLARE_CLASS_SIMPLE( CGELevelLoadingPanel, vgui::Frame );

	CGELevelLoadingPanel( vgui::VPANEL parent );
	~CGELevelLoadingPanel();
	
	void ApplySchemeSettings( vgui::IScheme *pScheme );

	void OnThink();

	void SetMapImage( const char *levelname );
	const char *GetCurrentImage();
	const char *GetDefaultImage();

	void Reset();

protected:
	void FireGameEvent( IGameEvent *event );
	void DisplayTip();

private:
	float m_flNextTipTime;

	vgui::GECreditsText *m_pTipContainer;
	vgui::ImagePanel	*m_pImagePanel;

	bool m_bMapImageSet;
	char m_szCurrentMap[64];
};

extern CGELevelLoadingPanel *GetLoadingScreen();

#endif
