///////////// Copyright � 2011, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_charselectgui.h
// Description:
//      VGUI definition for the character select screen
//
// Created On: 04 Nov 2011
// Created By: Jonathan White <killermonkey>
/////////////////////////////////////////////////////////////////////////////

#ifndef GE_CHARSELECTGUI_H
#define GE_CHARSELECTGUI_H

#ifdef _WIN32
#pragma once
#endif

#include <game/client/iviewport.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include "ge_VertListPanel.h"
#include "basemodelpanel.h"
#include "vgui_grid.h"
#include <vgui/KeyCode.h>
#include "ge_CreditsText.h"
#include "GameEventListener.h"
#include "ge_character_data.h"

namespace vgui {

//-----------------------------------------------------------------------------
// Purpose: Displays the Character Selection Menu
//-----------------------------------------------------------------------------
class CGECharSelect : public EditablePanel, public IViewPortPanel, public CGameEventListener
{
private:
	DECLARE_CLASS_SIMPLE( CGECharSelect, EditablePanel );

public:
	CGECharSelect( IViewPort *pViewPort );
	virtual ~CGECharSelect();

	virtual void ShowPanel( bool bShow );
	virtual void OnCommand( const char *command );

	virtual const char *GetName( void )		{ return PANEL_CHARSELECT; }
	virtual void SetData(KeyValues *data)	{ };
	virtual void Reset( void )				{ };
	virtual void Update( void )				{ };
	virtual bool NeedsUpdate( void )		{ return false; }
	virtual bool HasInputElements( void )	{ return true; }

	// IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void )			{ return BaseClass::GetVPanel(); }
	virtual bool IsVisible()				{ return BaseClass::IsVisible(); }
	virtual void SetParent( VPANEL parent )	{ BaseClass::SetParent( parent ); }

	// IGameEventListener interface:
	virtual void FireGameEvent( IGameEvent *event );

	MESSAGE_FUNC_PARAMS( CharacterClicked,		 "CharClicked",			data );
	MESSAGE_FUNC_PARAMS( CharacterDoubleClicked, "CharDoubleClicked",	data );
	MESSAGE_FUNC_PARAMS( SkinClicked,			 "SkinClicked",			data );

protected:
	// VGUI2 overrides
	virtual void OnKeyCodePressed( KeyCode code );

	// Update our known list of characters, triggered on show and team change
	void UpdateCharacterList();
	void SelectDefaultCharacter();

	void SetSelectedCharacter( const char *ident, int skin=0 );

	void ResetCharacterGrid();
	void PopulateCharacterGrid( int page = 0 );

	int  FindCharacter( const char *ident );

private:
	KeyCode			m_iScoreBoardKey;

	Label			*m_pCharName;
	GECreditsText	*m_pCharBio;
	CGrid			*m_pCharGrid;
	CModelPanel		*m_pCharModel;
	GEVertListPanel *m_pCharSkins;

	// List of characters currently presented
	CUtlVector<const CGECharData*> m_vCharacters;

	int		m_iCurrPage;
	char	m_szCurrChar[MAX_CHAR_IDENT];
	int		m_iCurrSkin;
};

} // vgui namespace

#endif
