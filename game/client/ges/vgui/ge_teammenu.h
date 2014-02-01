///////////// Copyright � 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_teammenu.h
// Description:
//      VGUI definition for the team selection menu
//
// Created On: Oct 08
// Created By: Jonathan White <killermonkey>
/////////////////////////////////////////////////////////////////////////////

#ifndef GE_TEAMMENU_H
#define GE_TEAMMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/PanelListPanel.h>
#include <vgui_controls/SectionedListPanel.h>
#include <vgui_controls/HTML.h>
#include <vgui_controls/TextEntry.h>

#include <game/client/iviewport.h>

#include <vgui/KeyCode.h>
#include "utlvector.h"
#include "GameEventListener.h"
#include "c_ge_gameplayresource.h"

//-----------------------------------------------------------------------------
// Purpose: Displays the team menu
//-----------------------------------------------------------------------------
class CGETeamMenu : public vgui::Frame, public IViewPortPanel, public CGameEventListener, public C_GEGameplayResourceListener
{
private:
	DECLARE_CLASS_SIMPLE( CGETeamMenu, vgui::Frame );

public:
	CGETeamMenu(IViewPort *pViewPort);

	virtual const char *GetName() { return PANEL_TEAM; }
	virtual void SetData( KeyValues *data ) {};
	virtual void Reset() {};
	virtual void Update();
	virtual bool NeedsUpdate();
	virtual bool HasInputElements() { return true; }
	virtual void ShowPanel( bool bShow );

	virtual void OnGameplayDataUpdate();

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel() { return BaseClass::GetVPanel(); }
  	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }
	
protected:	
	// VGUI2 overrides
	virtual void OnKeyCodePressed(vgui::KeyCode code);
	virtual void OnCommand( const char *command );

	virtual void FireGameEvent( IGameEvent *pEvent );

	// helper functions
	virtual void MakeTeamButtons();
	virtual vgui::Button *AddButton( const wchar_t *label, int iTeam );
	virtual vgui::Button *AddButton( const char* label, int iTeam );
	virtual void ServerOptAdder( const char* name, void *value, int type );
	virtual void GetMapTimeLeft( char* szTime );
	virtual void SetMapName( const char* name );

	// MOTD functions
	virtual void ShowMOTD();
	virtual void ShowFile( const char *filename);
	virtual void ShowURL( const char *URL);

	virtual void CreateServerOptText();

	IViewPort					*m_pViewPort;
	vgui::HTML					*m_pHTMLMessage;
	vgui::PanelListPanel		*m_pTeamButtonList;
	vgui::SectionedListPanel	*m_pServerOptList;

	bool		m_bTeamplay;
	float		m_flNextUpdateTime;

	char		m_szGameMode[64];
	char		m_szWeaponSet[32];

	ButtonCode_t	m_iJumpKey;
	ButtonCode_t	m_iScoreBoardKey;

	char m_szMapName[ MAX_PATH ];
};


#endif // GE_TEAMMENU_H
