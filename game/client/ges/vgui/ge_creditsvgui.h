///////////// Copyright � 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_roundreport.h
// Description:
//      VGUI definition for the End of Round Report
//
// Created On: 13 Dec 08
// Created By: Jonathan White <killermonkey>
/////////////////////////////////////////////////////////////////////////////

#ifndef GE_CREDITSVGUI_H
#define GE_CREDITSVGUI_H

#include <vgui/IScheme.h>
#include <game/client/iviewport.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>

#include "ge_CreditsText.h"
#include "ge_panelhelper.h"

using namespace vgui;

struct GECreditsEntry
{
	char		szName[MAX_PLAYER_NAME_LENGTH];
	Color		textColor;
	HFont	textFont;
};

//-----------------------------------------------------------------------------
// Purpose: Displays the credits on the main menu
//-----------------------------------------------------------------------------
class CGECreditsPanel : public vgui::Frame
{
private:
	DECLARE_CLASS_SIMPLE( CGECreditsPanel, vgui::Frame );

public:
	CGECreditsPanel( vgui::VPANEL parent );
	~CGECreditsPanel();

	virtual const char *GetName( void ) { return PANEL_CREDITS; }
	virtual void SetData( KeyValues *data ) {};
	virtual void Reset() { };
	virtual void Update() { };
	virtual bool NeedsUpdate() { return false; };
	virtual bool HasInputElements( void ) { return true; }
	virtual void SetVisible( bool bShow );
	virtual void ApplySchemeSettings( IScheme *pScheme );

	virtual void PlayMusic( bool closing = false );

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
  	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }
	virtual void SetParent( Panel *newParent ) { /*BaseClass::SetParent( newParent );*/ }
	
protected:
	virtual void ClearCredits( void );
	virtual void LoadCredits( void );

private:	
	virtual void OnCommand( const char *command );

	IViewPort	*m_pViewPort;
	IScheme		*m_pScheme;

	GECreditsText	*m_pCreditsText;
	
	CUtlVector<GECreditsEntry*> m_vCredits;
};

#endif
