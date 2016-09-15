///////////// Copyright � 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_creditsvgui.cpp
// Description:
//      VGUI definition for the main menu credits
//
// Created On: 13 Dec 08
// Created By: Jonathan White <killermonkey>
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "hud.h"
#include "iclientmode.h"
#include "filesystem.h"
#include "engine/IEngineSound.h"
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/IVGui.h>

#include "ge_creditsvgui.h"
#include "ge_gamerules.h"
#include "ge_musicmanager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;
extern ISoundEmitterSystemBase *soundemitterbase;

#define CREDITS_FILE "scripts/credits.txt"

static GameUI<CGECreditsPanel> g_CGECreditsPanel;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CGECreditsPanel::CGECreditsPanel( vgui::VPANEL parent ) : Frame(NULL, PANEL_CREDITS )
{
	SetParent( parent );

	// load the new scheme early!!
	SetScheme(vgui::scheme()->LoadSchemeFromFile("resource/SourceScheme.res", "SourceScheme"));
	SetProportional( true );
	SetSizeable( false );

	SetPaintBackgroundEnabled( true );
	SetPaintBorderEnabled( true );

	SetKeyBoardInputEnabled( true );
	SetMouseInputEnabled( true );

	LoadControlSettings("resource/GESCredits.res");

	InvalidateLayout();
	MakePopup();

	m_pScheme = scheme()->GetIScheme( GetScheme() );

	m_pCreditsText = static_cast<GECreditsText*>(FindChildByName("CreditsText"));
	m_pCreditsText->SetPanelInteractive( false );
	
	LoadCredits();
}

CGECreditsPanel::~CGECreditsPanel()
{
}

void CGECreditsPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_pCreditsText->SetDrawOffsets( XRES(2), YRES(-2) );
//	m_pCreditsText->SetBgColor( Color(0,0,0,220) );
}

void CGECreditsPanel::SetVisible( bool bShow )
{
	// If we are already showing ignore this
	if ( IsVisible() == bShow && bShow )
		return;

	if ( bShow )
	{
		m_pCreditsText->GotoTextStart();
		m_pCreditsText->SetBgColor( Color(0,0,0,220) );
		
		SetZPos( 1 );
		MoveToFront();
	}

	// Toggle Credits/Intro music on Open/Close
	PlayMusic( bShow );

	BaseClass::SetVisible( bShow );
}

void CGECreditsPanel::OnCommand( const char *command )
{
	g_CGECreditsPanel->SetVisible( !g_CGECreditsPanel->IsVisible() );
}


void CGECreditsPanel::PlayMusic( bool showing /*= false*/ )
{
	static int guid = 0;

	if ( showing )
	{
		enginesound->NotifyBeginMoviePlayback();
		GEMusicManager()->PauseMusic();
		enginesound->EmitAmbientSound( "ui/goldenpie.mp3", 1.0f );
		guid = enginesound->GetGuidForLastSoundEmitted();
	}
	else
	{
		enginesound->StopSoundByGuid( guid );
		GEMusicManager()->ResumeMusic();
		enginesound->NotifyEndMoviePlayback();
	}
}

void CGECreditsPanel::ClearCredits( void )
{
	m_vCredits.PurgeAndDeleteElements();

	if ( m_pCreditsText )
		m_pCreditsText->SetText("");
}

void CGECreditsPanel::LoadCredits( void )
{
	ClearCredits();
	m_pCreditsText->SetText("");
	m_pCreditsText->SetBgColor( Color(0,0,0,220) );

	KeyValues *pKV = new KeyValues( "CreditsFile" );
	if ( !pKV->LoadFromFile( filesystem, CREDITS_FILE, "MOD" ) )
	{
		pKV->deleteThis();
		return;
	}

	KeyValues *pKVSubkey = pKV->GetFirstSubKey();
	GECreditsEntry *entry;
	CUtlVector<char*, CUtlMemory<char*> > data;

	while ( pKVSubkey )
	{
		entry = new GECreditsEntry;
		Q_strncpy( entry->szName, pKVSubkey->GetName(), MAX_PLAYER_NAME_LENGTH );

		Q_SplitString( pKVSubkey->GetString(), " ", data );
		entry->textFont = m_pScheme->GetFont( data[0], true );
		entry->textColor = m_pScheme->GetColor( data[1], Color( 255, 255, 255, 255 ) );
		m_vCredits.AddToTail( entry );

		if ( entry->textFont != INVALID_FONT )
			m_pCreditsText->InsertFontChange( entry->textFont );
//		if ( entry->textFont != INVALID_FONT )
		m_pCreditsText->InsertCenterText( true );
		m_pCreditsText->InsertColorChange( entry->textColor );
		m_pCreditsText->InsertString( entry->szName );
		m_pCreditsText->InsertChar( L'\n' );

		pKVSubkey = pKVSubkey->GetNextKey();
	}

	pKV->deleteThis();
}

CON_COMMAND(showcredits, "Brings up the credits for GES")
{
	g_CGECreditsPanel->SetVisible( !g_CGECreditsPanel->IsVisible() );
}
