///////////  Copyright © 2007, Mark Chandler. All rights reserved.  ///////////
//
// Description:
//
////////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ge_charPanel.h"
#include <vgui/ISurface.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

GECharPanel::GECharPanel( Panel *parent, const char *name ) : BaseClass( parent, name )
{
	SetProportional( true );
	SetShouldScaleImage( true );
	SetMouseInputEnabled( true );
	SetKeyBoardInputEnabled( false );

// 	m_pCharName	 = new Label( this, "CharName", "" );
// 	m_pCharName->SetMouseInputEnabled( true );
// 	m_pCharName->SetReflectMouse( true );

	m_szCharIdent[0] = '\0';
}

void GECharPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

// 	m_pCharName->SetContentAlignment( Label::a_east );
// 	m_pCharName->SetTextInset( XRES(1), YRES(4) );
// 	m_pCharName->SetZPos( 1 );
// 	m_pCharName->SetFgColor( pScheme->GetColor("white", Color(255,255,255,255)) );
// 	m_pCharName->SetFont( pScheme->GetFont("CharSelectName", true) );
}

void GECharPanel::SetCharacter( const CGECharData *pChar )
{
	if ( pChar )
	{
		Q_strncpy( m_szCharIdent, pChar->szIdentifier, MAX_CHAR_IDENT );

		Q_snprintf( m_szOnImage,  128, "charselect/%s_portrait_S", pChar->szIdentifier );
		Q_snprintf( m_szOffImage, 128, "charselect/%s_portrait_N", pChar->szIdentifier );

		SetImage( m_szOffImage );
//		m_pCharName->SetText( pChar->szPrintName );
	}
	else
	{
		m_szCharIdent[0] = m_szOffImage[0] = m_szOnImage[0] = '\0';
		SetImage( (IImage*) NULL );
//		m_pCharName->SetText( "" );
	}
}

void GECharPanel::PerformLayout( void )
{
	BaseClass::PerformLayout();

	int wide, tall;
	GetSize( wide, tall );

// 	m_pCharName->SizeToContents();
// 	m_pCharName->SetPos( wide - m_pCharName->GetWide(), tall - m_pCharName->GetTall() - YRES(2) );
}

void GECharPanel::SetSelected( bool state )
{
	if ( state )
		SetImage( m_szOnImage );
	else
		SetImage( m_szOffImage );
}

void GECharPanel::OnMousePressed(MouseCode code)
{
	if ( code == MOUSE_LEFT )
	{
		KeyValues *kv = new KeyValues( "CharClicked" );
		kv->SetString( "ident", m_szCharIdent );
		PostActionSignal( kv );
	}
}

void GECharPanel::OnMouseDoublePressed(MouseCode code)
{
	if ( code == MOUSE_LEFT )
	{
		KeyValues *kv = new KeyValues( "CharDoubleClicked" );
		kv->SetString( "ident", m_szCharIdent );
		PostActionSignal( kv );
	}
}