///////////// Copyright � 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_spectatorgui.h
// Description:
//      VGUI definition for the spectator
//
// Created On: 12 Dec 08
// Created By: Jonathan White <killermonkey>
/////////////////////////////////////////////////////////////////////////////

#ifndef GE_SPECTATORGUI_H
#define GE_SPECTATORGUI_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/SectionedListPanel.h>

#include "mouseoverpanelbutton.h"
#include "c_ge_player.h"
#include "GameEventListener.h"

#include "spectatorgui.h"

#include <game/client/iviewport.h>

#include <vgui/KeyCode.h>
#include "utlvector.h"

class CGESpectatorGUI : public CSpectatorGUI
{
private:
	DECLARE_CLASS_SIMPLE( CGESpectatorGUI, CSpectatorGUI );

public:
	CGESpectatorGUI( IViewPort *pViewPort );

	virtual void Update( void );
	virtual void OnThink( void );
	virtual bool NeedsUpdate( void );

	virtual Color GetBlackBarColor( void ) { return Color(172,160,138,185); } // A folder color semi-transparent

protected:
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void FindOtherPanels( void );

	int		m_nLastSpecMode;
	EHANDLE m_hLastSpecTarget;

	vgui::IScheme		*m_pScheme;

	IViewPortPanel		*m_pScoreboard;
	IViewPortPanel		*m_pTeamMenu;
	IViewPortPanel		*m_pCharMenu;
	IViewPortPanel		*m_pRoundReport;
};

#endif
