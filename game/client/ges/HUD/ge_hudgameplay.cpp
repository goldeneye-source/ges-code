///////////// Copyright © 2009, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_hudgameplay.cpp
// Description:
//      See Header
//
// Created On: 01 Jul 09
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_ge_player.h"
#include "c_ge_playerresource.h"
#include "iclientmode.h"
#include "vgui/ILocalize.h"
#include "gemp_gamerules.h"
#include "c_team.h"
#include <vgui/ISurface.h>
#include "vgui_controls/controls.h"
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/TextImage.h>
#include "ge_hudgameplay.h"
#include "networkstringtable_clientdll.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT_DEPTH( CHudGameplay, 100 );

#define HUDGP_SETICONDIR "hud/weaponseticons/"

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudGameplay::CHudGameplay( const char *pElementName ) : BaseClass(NULL, "GEHUDGameplay"), CHudElement( pElementName )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pHudAnims = g_pClientMode->GetViewportAnimationController();

	m_iCurrState = GAMEPLAY_HIDDEN;
	m_iCurrMovement = MOVEMENT_NONE;
	m_flHeightPercent = 0;

	m_pBackground = new vgui::ImagePanel( this, "Background" );
	m_pBackground->SetImage( "hud/GameplayHUDBg" );
	m_pBackground->SetShouldScaleImage( true );
	m_pBackground->SetPaintBackgroundType( 1 );
	m_pBackground->SetPaintBorderEnabled( false );

	m_pWeaponSet = new vgui::Panel( this, "WeaponSet" );
	m_pWeaponSet->SetPaintBackgroundEnabled( false );
	m_pWeaponSet->SetPaintBorderEnabled( false );

	m_pWeaponSetName = new vgui::Label( m_pWeaponSet, "weaponsetname", "NULL" );
	m_pWeaponSetName->SetPaintBackgroundEnabled( false );
	m_pWeaponSetName->SetPaintBorderEnabled( false );

	char name[8];
	for ( int i=0; i < MAX_WEAPON_SPAWN_SLOTS; i++ )
	{
		Q_snprintf( name, 8, "slot%i", i );
		m_vWeaponLabels.AddToTail( new sWeaponLabel );
		m_vWeaponLabels.Tail()->pImage = new vgui::ImagePanel( m_pWeaponSet, name );
		m_vWeaponLabels.Tail()->pImage->SetShouldScaleImage( true );
		m_vWeaponLabels.Tail()->pLabel = new vgui::Label( m_pWeaponSet, name, "" );
	}

	m_pGameplayHelp = new vgui::GECreditsText( this, "GameplayHelp" );
	m_pGameplayHelp->SetPaintBackgroundEnabled( false );
	m_pGameplayHelp->SetPaintBorderEnabled( false );
	m_pGameplayHelp->SetVerticalScrollbar( false );
}

CHudGameplay::~CHudGameplay()
{
	m_vWeaponLabels.PurgeAndDeleteElements();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the HUD element on map change
//-----------------------------------------------------------------------------
void CHudGameplay::Reset()
{
	if ( m_iCurrMovement != MOVEMENT_NONE )
	{
		if ( m_iCurrMovement == MOVEMENT_SHOWING )
			m_flHeightPercent = 1.0f;
		else
			m_flHeightPercent = 0;
	}
}

void CHudGameplay::VidInit()
{
	// Make sure we hide our goods
	m_iCurrState = GAMEPLAY_HIDDEN;

	// Explicitly reset our ypos to 0% height
	m_flHeightPercent = 0;

	SetZPos( 3 );
	
	InvalidateLayout();
	Reset();
}

bool CHudGameplay::RequestInfo(KeyValues *outputData)
{
	if (!stricmp(outputData->GetName(), "HeightPercent"))
	{
		outputData->SetFloat("HeightPercent", m_flHeightPercent);
		return true;
	}
	
	return BaseClass::RequestInfo(outputData);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudGameplay::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetFgColor( Color(0,0,0,0) );

	m_pBackground->SetBgColor( m_BgColor );
	m_pBackground->SetFgColor( GetFgColor() );
	
	m_pWeaponSetName->SetFgColor( m_TextColor );
	m_pWeaponSetName->SetFont( m_hWeaponFont );

	m_pGameplayHelp->SetFgColor( m_TextColor );
	m_pGameplayHelp->SetBgColor( m_BgColor );
	m_pGameplayHelp->SetFont( m_hWeaponFont );
	m_pGameplayHelp->SetWide( GetWide() );

	GetPos( m_iOrigX, m_iOrigY );

	SetZPos( 500 );
	SetAlpha( 255 );
	SetBgColor( Color( 0, 0, 0, 0 ) );
	SetPaintBackgroundType( 0 );
}

void CHudGameplay::OnGameplayDataUpdate( void )
{
	char szClassName[32], szTexture[256];
	wchar_t szPrintName[32], *szName;

	// Set the weaponset name
	szName = g_pVGuiLocalize->Find( GEGameplayRes()->GetLoadoutName() );
	if ( szName )
		m_pWeaponSetName->SetText( szName );
	else
		m_pWeaponSetName->SetText( GEGameplayRes()->GetLoadoutName() );
	m_pWeaponSetName->SetFont( m_hHeaderFont );
	m_pWeaponSetName->SizeToContents();

	// Set the weapons
	CUtlVector<int> weapons;
	GEGameplayRes()->GetLoadoutWeaponList( weapons );

	for( int i=0; i < weapons.Count(); i++ )
	{
		szName = g_pVGuiLocalize->Find( GetWeaponPrintName(weapons[i]) );
		if ( szName )
			_snwprintf( szPrintName, 32, L"%i. %s", i+1, szName );
		else
			_snwprintf( szPrintName, 32, L"%i. Unknown Weapon", i+1 );
			
		m_vWeaponLabels[i]->pLabel->SetText( szPrintName );
		m_vWeaponLabels[i]->pLabel->SetFgColor( m_TextColor );
		m_vWeaponLabels[i]->pLabel->SetFont( m_hWeaponFont );
		m_vWeaponLabels[i]->pLabel->SizeToContents();

		Q_strncpy( szClassName, WeaponIDToAlias(weapons[i]), 32 );
		Q_StrRight( szClassName, strlen(szClassName)-7, szClassName, 32 );
		Q_snprintf( szTexture, 256, "%s%s", HUDGP_SETICONDIR, szClassName );
		m_vWeaponLabels[i]->pImage->SetImage( szTexture );
		m_vWeaponLabels[i]->pImage->SetSize( XRES(64), YRES(16) );
	}

	ResolveGameplayHelp();
	InvalidateLayout();
}

void CHudGameplay::ResolveGameplayHelp( void )
{
	if ( !g_pStringTableGameplay )
		return;

	m_pGameplayHelp->SetText( "" );
	m_pGameplayHelp->InsertCenterText( true );
	m_pGameplayHelp->InsertFontChange( m_hHeaderFont );

	m_pGameplayHelp->InsertString( GEGameplayRes()->GetGameplayName() );

	m_pGameplayHelp->InsertCenterText( false );
	m_pGameplayHelp->InsertFontChange( m_hHelpFont );
	m_pGameplayHelp->InsertString( "\n\n" );

	const char *pHelpString = g_pStringTableGameplay->GetString( GEGameplayRes()->GetGameplayHelp() );
	if ( pHelpString )
	{
		m_pGameplayHelp->InsertString( pHelpString );
	}
	else
	{
		wchar_t *szParsedHelp = g_pVGuiLocalize->Find( "#GES_GP_NOHELP" );
		if ( szParsedHelp )
			m_pGameplayHelp->InsertString( szParsedHelp );
		else
			m_pGameplayHelp->InsertString( "No help available for this gameplay" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Resizes the label
//-----------------------------------------------------------------------------
void CHudGameplay::PerformLayout()
{
	BaseClass::PerformLayout();

	int wide, tall;
	GetSize( wide, tall );

	int infoFontTall = vgui::surface()->GetFontTall( m_hWeaponFont );

	m_pBackground->SetPos( 0, 0 );
	m_pBackground->SetSize( wide, tall );

	m_pWeaponSet->SetVisible( false );
	m_pWeaponSet->SetPos( 0, 0 );
	m_pWeaponSet->SetSize( wide - m_iTabWidth, tall );

	m_pGameplayHelp->SetVisible( false );
	m_pGameplayHelp->SetPos( m_iTextX, m_iTextY );
	m_pGameplayHelp->SetSize( wide - m_iTextX - m_iTabWidth, tall - m_iTextY );

	switch( m_iCurrState )
	{
	case GAMEPLAY_HELP:
		// Set the gameplay help visible
		m_pGameplayHelp->SetVisible( true );
		break;

	case GAMEPLAY_WEAPONSET:
		m_pWeaponSet->SetVisible( true );
		m_pWeaponSetName->SetPos( m_pWeaponSet->GetWide()/2 - m_pWeaponSetName->GetWide()/2, m_iTextY );

		int x_in = m_iTextX, y_in = m_iTextY*4 + infoFontTall;

		// We list weapons from lowest spawn slot to highest, top to bottom
		for ( int i=0; i < MAX_WEAPON_SPAWN_SLOTS; ++i )
		{
			m_vWeaponLabels[i]->SetPos( x_in, y_in );
			y_in += m_vWeaponLabels[i]->GetTall() + m_iTextY;
		}	
		break;
	}
}

void CHudGameplay::OnThink( void )
{
	// Set our position based on our animation variable
	m_flHeightPercent = clamp( m_flHeightPercent, 0, 1.0f );
	int offset = GetTall() * (1.0f - m_flHeightPercent);
	SetPos( m_iOrigX, m_iOrigY + offset );

	// Hide us and ensure our state is set properly
	if ( m_flHeightPercent == 0 )
	{
		m_iCurrState = GAMEPLAY_HIDDEN;
		m_iCurrMovement = MOVEMENT_NONE;
		SetVisible(false);
	}
	else if ( m_flHeightPercent == 1.0f )
	{
		m_iCurrMovement = MOVEMENT_NONE;
	}
}

void CHudGameplay::PaintString( const wchar_t *text, Color col, vgui::HFont& font, int x, int y )
{
	vgui::surface()->DrawSetTextColor( col );
	vgui::surface()->DrawSetTextFont( font );
	vgui::surface()->DrawSetTextPos( x, y );
	vgui::surface()->DrawUnicodeString( text );
}

void CHudGameplay::KeyboardInput( int action )
{
	if ( m_iCurrState == action )
	{
		// Hitting the same key again will hide the panel
		m_pHudAnims->StartAnimationSequence( "GameplayHudHide" );
		m_iCurrState = GAMEPLAY_HIDDEN;
		m_iCurrMovement = MOVEMENT_HIDING;
		return;
	}
	else if ( m_iCurrState == GAMEPLAY_HIDDEN )
	{
		// If we were hidden, we need to pop up again
		SetVisible( true );
		m_pHudAnims->StartAnimationSequence( "GameplayHudShow" );
		m_iCurrMovement = MOVEMENT_SHOWING;
	}

	// Set this so we show the correct data
	m_iCurrState = action;
	InvalidateLayout();
}

