///////////// Copyright � 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_roundreport.h
// Description:
//      VGUI definition for the End of Round Report
//
// Created On: 13 Dec 08
// Created By: Jonathan White <killermonkey>
/////////////////////////////////////////////////////////////////////////////

#ifndef GE_ROUNDREPORT_H
#define GE_ROUNDREPORT_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/PanelListPanel.h>
#include <vgui_controls/SectionedListPanel.h>
#include <vgui_controls/TextEntry.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/ImageList.h>
#include "ge_awardpanel.h"
#include "ge_VertListPanel.h"
#include "c_ge_gameplayresource.h"

#include <game/client/iviewport.h>

#include <vgui/IScheme.h>
#include <vgui/KeyCode.h>
#include "utlvector.h"
#include "GameEventListener.h"

//-----------------------------------------------------------------------------
// Purpose: Displays the team menu
//-----------------------------------------------------------------------------
class CGERoundReport : public vgui::EditablePanel, public IViewPortPanel, public CGameEventListener, public C_GEGameplayResourceListener
{
private:
	DECLARE_CLASS_SIMPLE( CGERoundReport, vgui::EditablePanel );

public:
	CGERoundReport(IViewPort *pViewPort);
	virtual ~CGERoundReport();

	virtual const char *GetName( void ) { return PANEL_ROUNDREPORT; }
	virtual void SetData( KeyValues *data ) {};
	virtual void Reset();
	virtual void Update() {};
	virtual bool NeedsUpdate() { return false; };
	virtual void OnTick();
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );

	virtual void PlayWinLoseMusic( void );

	// Gameplay Information Listener
	virtual void OnGameplayDataUpdate();

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
  	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }

	virtual Panel* CreateControlByName(const char *controlName);

protected:
	virtual void FireGameEvent( IGameEvent *pEvent );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

private:	
	// VGUI2 overrides
	virtual void OnKeyCodePressed(vgui::KeyCode code);
	virtual void OnCommand( const char *command );

	virtual void AddSection( int type );

	// helper functions
	virtual void InitReportSections( void );
	virtual void PopulatePlayerList( void );
	virtual void PopulateAwards( void );
	virtual void UpdateTeamScores( void );
	virtual void SetWinnerText( void );
	virtual bool GetPlayerScoreInfo( int playerIndex, KeyValues *kv );

	virtual void SetupAwardPanel( const char* name, CBasePlayer *player, int iAward );

	virtual void FindOtherPanels( void );

private:
	enum 
	{ 
		GES_ICON_WIDTH = 16,
		GES_NAME_WIDTH = 155,
		GES_TEAM_WIDTH = 45,
		GES_CHAR_WIDTH = 45,
		GES_SCORE_WIDTH = 42,
		GES_DEATH_WIDTH = 42,
		GES_FAVWEAPON_WIDTH = 100,
	};

	IViewPort			*m_pViewPort;
	vgui::IScheme		*m_pScheme;
	vgui::ImagePanel	*m_pBackground;
	vgui::SectionedListPanel	*m_pPlayerList;
	vgui::ImageList		*m_pImageList;

	IViewPortPanel		*m_pScoreboard;
	IViewPortPanel		*m_pTeamMenu;
	IViewPortPanel		*m_pCharMenu;

	bool			m_bInTempHide;
	bool			m_bTakeScreenshot;
	ButtonCode_t	m_iScoreBoardKey;

	struct GERoundAward
	{
		int m_iPlayerID;
		int m_iAwardID;
	};

	struct GERoundData
	{
		bool is_locked;
		bool is_teamplay;
		bool is_match;
		int	 winner_id;
		int  round_count;
		wchar_t scenario_name[64];

		CUtlVector<GERoundAward*> awards;
	} m_RoundData;
};


#endif // GE_ROUNDREPORT_H