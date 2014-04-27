///////////// Copyright � 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_scoreboard.h
// Description:
//      VGUI definition for the scoreboard
//
// Created On: 13 Sep 08
// Created By: Jonathan White <killermonkey>
/////////////////////////////////////////////////////////////////////////////

#ifndef GE_SCOREBOARD_H
#define GE_SCOREBOARD_H

#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/ImagePanel.h"
#include "clientscoreboarddialog.h"
#include "c_ge_gameplayresource.h"

namespace vgui {
//-----------------------------------------------------------------------------
// Purpose: Displays the Scoreboard
//-----------------------------------------------------------------------------
class CGEScoreBoard : public CClientScoreBoardDialog, public C_GEGameplayResourceListener
{
private:
	DECLARE_CLASS_SIMPLE( CGEScoreBoard, CClientScoreBoardDialog);

public:
	CGEScoreBoard(IViewPort *pViewPort);
	~CGEScoreBoard();

	virtual void ShowPanel( bool bShow );
	virtual void Update( void );
	virtual void OnTick( void );

	// scoreboard overrides
	virtual void InitScoreboardSections( void );
	virtual void PopulateScoreBoard( void );

	virtual void UpdateTeamInfo( void );
	virtual void UpdatePlayerInfo( void );

	// Listeners
	virtual void OnGameplayDataUpdate();
	virtual void FireGameEvent( IGameEvent *event);

	virtual bool GetPlayerScoreInfo( int playerIndex, KeyValues *kv );
	// sorts players within a section
	static bool StaticPlayerSortFunc(vgui::SectionedListPanel *list, int itemID1, int itemID2);

private:
	virtual void AddHeader( void ) { }; // nulled out to prevent a header from ever showing
	virtual void AddSection(int teamType, int teamNumber); // add a new section header for a team
	virtual int GetSectionFromTeamNumber( int teamNumber );

	virtual void UpdateSpectatorList( void );
	virtual void UpdateTeamScores( void );
	virtual void UpdateMapTimer( void );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	enum 
	{ 
		GES_DEADICON_WIDTH = 16,
		GES_NAME_WIDTH = 143,
		GES_CHAR_WIDTH = 41,
		GES_SCORE_WIDTH = 39,
		GES_DEATH_WIDTH = 39,
		GES_PING_WIDTH = 28,
	};

	vgui::ImageList		*m_pImageList;

	vgui::ImagePanel	*m_pBackground;
	bool				m_bTeamPlay;
	bool				m_bVisibleOnToggle;
};

}

#endif