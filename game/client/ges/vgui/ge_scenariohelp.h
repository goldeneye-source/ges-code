///////////// Copyright © 2011, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_scenariohelp.h
// Description:
//      VGUI definition for the scenario help screen
//
// Created On: 09 Feb 2012
// Created By: Jonathan White <killermonkey>
/////////////////////////////////////////////////////////////////////////////

#ifndef GE_SCENARIOHELPGUI_H
#define GE_SCENARIOHELPGUI_H
#ifdef _WIN32
#pragma once
#endif

#include <game/client/iviewport.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/CheckButton.h>
#include "vgui/IImage.h"
#include "ge_VertListPanel.h"
#include "GameEventListener.h"

namespace vgui {

//-----------------------------------------------------------------------------
// Purpose: Displays the Character Selection Menu
//-----------------------------------------------------------------------------
class CGEScenarioHelp : public EditablePanel, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE( CGEScenarioHelp, EditablePanel );

public:
	CGEScenarioHelp( IViewPort *pViewPort );
	~CGEScenarioHelp();

	virtual void ShowPanel( bool bShow );
	virtual void OnCommand( const char *command );
	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void SetData( KeyValues *data );
	virtual void OnTick();

	virtual void Reset()					{ };
	virtual void Update()					{ };
	virtual bool NeedsUpdate()				{ return false; }
	virtual bool HasInputElements()			{ return true; }
	virtual const char *GetName()			{ return PANEL_SCENARIOHELP; }

	// IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void )			{ return BaseClass::GetVPanel(); }
	virtual bool IsVisible()				{ return BaseClass::IsVisible(); }
	virtual void SetParent( VPANEL parent )	{ BaseClass::SetParent( parent ); }

	void SetPlayerRequestShow( bool state )	{ m_bPlayerRequestShow = state; }

	void MsgFunc_ScenarioHelp( bf_read &msg );
	void MsgFunc_ScenarioHelpPane( bf_read &msg );
	void MsgFunc_ScenarioHelpShow( bf_read &msg );

protected:
	// VGUI2 overrides
	virtual void OnKeyCodePressed( KeyCode code );

	void SetupPane( int id );
	bool ShouldShowPane( int id );

	void ClearHelp();

private:
	Label			*m_pScenarioName;
	Label			*m_pScenarioTagLine;
	ImagePanel		*m_pBackground;
	GEVertListPanel *m_pHelpContainer;

	KeyValues		*m_pHelpSettings;

	// Help Specific information
	bool	m_bWaitForResolve;
	char	m_szNextPanel[32];

	char	m_szScenarioIdent[32];
	char	m_szTagLine[64];
	char	m_szWebsite[128];

	int		m_iCurrentPane;
	int		m_iDefaultPane;
	bool	m_bPlayerRequestShow;

	// For dimming sound effects while help is showing
	float	m_flSoundVolume;

	struct sHelpItem
	{
		char	desc[128];
		IImage*	image;
	};

	struct sHelpPane
	{
		~sHelpPane() { help.PurgeAndDeleteElements(); }

		int		key;
		char	id[32];

		int		str_idx;
		bool	str_resolved;
		CUtlVector<sHelpItem*> help;
	};
	
	CUtlVector<sHelpPane*> m_vHelpPanes;

	// Utility functions
	sHelpPane*	FindPane( int pane_key );
	void		ResolveHelpStrings( sHelpPane *pPane );
};

} // vgui namespace

#endif
