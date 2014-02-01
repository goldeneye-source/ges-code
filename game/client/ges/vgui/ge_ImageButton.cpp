//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Basic button control
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include <stdio.h>

#include <vgui/IBorder.h>
#include <vgui/IInput.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui/IVGui.h>
#include <vgui/MouseCode.h>
#include <vgui/KeyCode.h>
#include <KeyValues.h>

#include "ge_ImageButton.h"

#include <vgui_controls/FocusNavGroup.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

DECLARE_BUILD_FACTORY_DEFAULT_TEXT( ImageButton, ImageButton );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
ImageButton::ImageButton(Panel *parent, const char *panelName, const char *text, Panel *pActionSignalTarget, const char *pCmd )
	: Button(parent, panelName, text, pActionSignalTarget, pCmd)
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
ImageButton::ImageButton(Panel *parent, const char *panelName, const wchar_t *wszText, Panel *pActionSignalTarget, const char *pCmd )
	: Button(parent, panelName, wszText, pActionSignalTarget, pCmd)
{
	Init();
}

ImageButton::~ImageButton( void )
{
	delete m_pImgDefault, m_pImgArmed, m_pImgDepressed;
}

void ImageButton::Init( void )
{
	m_pImgDefault = new ImagePanel( this, "Default" );
	m_pImgDefault->SetMouseInputEnabled( true );
	m_pImgDefault->SetShouldScaleImage( true );
	m_pImgDefault->SetZPos( 0 );
	m_pImgDefault->SetVisible( true );

	m_pImgArmed = new ImagePanel( this, "Armed" );
	m_pImgArmed->SetMouseInputEnabled( true );
	m_pImgArmed->SetShouldScaleImage( true );
	m_pImgArmed->SetZPos( 1 );
	m_pImgArmed->SetVisible( false );

	m_pImgDepressed = new ImagePanel( this, "Depressed" );
	m_pImgDepressed->SetMouseInputEnabled( true );
	m_pImgDepressed->SetShouldScaleImage( true );
	m_pImgDepressed->SetZPos( 2 );
	m_pImgDepressed->SetVisible( false );

	SetPaintBackgroundEnabled( false );
	SetPaintBorderEnabled( false );

	SetText("");

	m_flNextThink = gpGlobals->curtime;
}

void ImageButton::OnSizeChanged( int wide, int tall )
{
	m_pImgDefault->SetBounds( 0, 0, wide, tall );
	m_pImgArmed->SetBounds( 0, 0, wide, tall );
	m_pImgDepressed->SetBounds( 0, 0, wide, tall );
}

void ImageButton::OnThink( void )
{
	if (m_flNextThink > gpGlobals->curtime)
		return;

	VPANEL pMouseOver = input()->GetMouseOver();
	// If we are enabled and the mouse is over one of our images make sure we are showing armed
	if ( IsEnabled() && ( m_pImgDepressed->GetVPanel() == pMouseOver 
			|| m_pImgArmed->GetVPanel() == pMouseOver
			|| m_pImgDefault->GetVPanel() == pMouseOver ) && !m_pImgArmed->IsVisible() )
	{
		OnCursorEntered();
	}
	else if ( IsEnabled() && !( m_pImgDepressed->GetVPanel() == pMouseOver 
			|| m_pImgArmed->GetVPanel() == pMouseOver
			|| m_pImgDefault->GetVPanel() == pMouseOver ) && m_pImgArmed->IsVisible() )
	{
		OnCursorExited();
	}

	m_flNextThink = gpGlobals->curtime + 0.3;
}

void ImageButton::SetImages( const char *szDefault, const char *szArmed /* = NULL */, const char *szDepressed /* = NULL */ )
{
	if ( !szArmed )
	{
		szArmed = szDefault;
	}

	if ( !szDepressed )
	{
		if ( !szArmed )
			szDepressed = szDefault;
		else
			szDepressed = szArmed;
	}

	m_pImgDefault->SetImage( szDefault );
	m_pImgArmed->SetImage( szArmed );
	m_pImgDepressed->SetImage( szDepressed );
}

void ImageButton::SetButtonState( ImageState state )
{
	m_pImgDefault->SetVisible( state == STATE_DEFAULT );
	m_pImgArmed->SetVisible( state == STATE_ARMED );
	m_pImgDepressed->SetVisible( state == STATE_DEPRESSED );
}

void ImageButton::SetSelectedPanel( int state )
{
	if ( state == 1 )
		SetButtonState( STATE_DEPRESSED );
	else
		SetButtonState( STATE_DEFAULT );
}

// TODO: Setup a state system and a checker to make sure we are displaying the right image
//       instead of relying on the cursor input...
void ImageButton::OnCursorEntered( void )
{
//	SetButtonState( STATE_ARMED );
	BaseClass::OnCursorEntered();
}

void ImageButton::OnCursorExited( void )
{
//	SetButtonState( STATE_DEFAULT );
	BaseClass::OnCursorExited();
}

void ImageButton::OnMousePressed( MouseCode code )
{
	SetButtonState( STATE_DEPRESSED );
	BaseClass::OnMousePressed(code);
}

void ImageButton::OnMouseReleased( MouseCode code )
{
	SetButtonState( STATE_ARMED );

	VPANEL pMouseOver = input()->GetMouseOver();

	// it has to be both enabled and (mouse over the button) to fire
	if ( IsEnabled() && ( m_pImgDepressed->GetVPanel() == pMouseOver 
			|| m_pImgArmed->GetVPanel() == pMouseOver
			|| m_pImgDefault->GetVPanel() == pMouseOver ) )
	{
		DoClick();
	}

	BaseClass::OnMouseReleased(code);
}

void ImageButton::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	const char *defaultimg = inResourceData->GetString( "defaultimg", "" );
	const char *armedimg = inResourceData->GetString( "armedimg", NULL );
	const char *depressedimg = inResourceData->GetString( "depressedimg", NULL );
	
	SetImages( defaultimg, armedimg, depressedimg );
}
