///////////// Copyright � 2011, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_charselectgui.cpp
// Description:
//      VGUI definition for the character select screen
//
// Created On: 04 Nov 2011
// Created By: Jonathan White <killermonkey>
/////////////////////////////////////////////////////////////////////////////
#include "cbase.h"

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include "ge_ImageButton.h"
#include "ge_charPanel.h"

#include "ge_charselectgui.h"
#include "clientmode_ge.h"
#include "ge_character_data.h"
#include "gemp_gamerules.h"
#include "ge_panelhelper.h"
#include "c_ge_playerresource.h"
#include "c_ge_gameplayresource.h"

#include "filesystem.h"
#include "IGameUIFuncs.h" // for key bindings
#include "cdll_client_int.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifndef _XBOX
extern IGameUIFuncs *gameuifuncs; // for key binding details
#endif

#define CHARSELECT_RANDOM_MDL	"models/players/random/randomcharacter.mdl"

using namespace vgui;

ConVar ge_favchar		( "ge_favchar",		 "", FCVAR_ARCHIVE, "Your last used character" );
ConVar ge_favcharMI6	( "ge_favcharMI6",	 "", FCVAR_ARCHIVE, "Your last used MI6 character" );
ConVar ge_favcharJanus	( "ge_favcharJanus", "", FCVAR_ARCHIVE, "Your last used Janus character" );

CGECharSelect::CGECharSelect(IViewPort *pViewPort) : EditablePanel(NULL, PANEL_CHARSELECT )
{
	m_iScoreBoardKey = KEY_NONE;	// this is looked up in Activate()

	// load the new scheme early!!
	SetScheme( "ClientScheme" );
	SetProportional( true );
	MakePopup();

	LoadControlSettings("resource/UI/charselect.res");

	ListenForGameEvent( "player_team" );

	m_pCharName = (Label*) FindChildByName( "CharName" );
	m_pCharName->SetFgColor( Color(0,0,0,255) );

	m_pCharBio = (GECreditsText*) FindChildByName( "CharBio" );
	m_pCharBio->SetKeyBoardInputEnabled(false);
	m_pCharBio->SetPanelInteractive(false);

	m_pCharGrid	 = (CGrid*) FindChildByName( "CharGrid" );
	m_pCharModel = (CModelPanel*) FindChildByName( "CharModel" );
	m_pCharSkins = (GEVertListPanel*) FindChildByName( "CharSkins" );

	ResetCharacterGrid();

	SetVisible( false );

	m_szCurrChar[0] = '\0';
}

CGECharSelect::~CGECharSelect()
{
}

//-----------------------------------------------------------------------------
// Purpose: shows the character selection menu
//-----------------------------------------------------------------------------
void CGECharSelect::ShowPanel(bool bShow)
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer || BaseClass::IsVisible() == bShow )
		return;

	int team = pPlayer->GetTeamNumber();
	if ( bShow && team != TEAM_SPECTATOR )
	{
		// Update our list and select the default
		UpdateCharacterList();

		m_pCharGrid->SetPaintBackgroundType( 1 );
		m_pCharGrid->SetPaintBackgroundEnabled( true );

		if ( team == TEAM_JANUS )
			m_pCharGrid->SetTexture( 1, "vgui/charselect/ui/film_strip_janus" );
		else if ( team == TEAM_MI6 )
			m_pCharGrid->SetTexture( 1, "vgui/charselect/ui/film_strip_mi6" );
		else
			m_pCharGrid->SetTexture( 1, "vgui/charselect/ui/film_strip_dm" );

		SetMouseInputEnabled( true );
		SetVisible( true );

		if ( m_iScoreBoardKey < 0 )
			m_iScoreBoardKey = gameuifuncs->GetButtonCodeForBind( "showscores" );

		// HACK!!! for clipping on 4:3
		if ( (ScreenWidth() / (float) ScreenHeight()) < 1.5 )
			m_pCharModel->m_pModelInfo->m_vecOriginOffset.x = 115;
		else
			m_pCharModel->m_pModelInfo->m_vecOriginOffset.x = 85;
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
	}
}

void CGECharSelect::OnCommand( const char *command )
{
	if ( !Q_stricmp( command, "ok" ) )
	{
		int idx = FindCharacter( m_szCurrChar );
		if ( idx == -1 )
			idx = 0;

		const CGECharData *pChar = m_vCharacters[idx];

		char strargs[36];
		char strcmd[100];
		Q_snprintf(strargs, 36, "%s %i", pChar->szIdentifier, m_iCurrSkin );
		Q_snprintf(strcmd, 100, "joinclass %s", strargs );

		engine->ClientCmd( strcmd );

		// Save the players last character so they don't have to search for it again
		// Figure out the proper team CVar to choose from for favorite character
		if ( GEMPRules()->IsTeamplay() )
		{
			if ( CBasePlayer::GetLocalPlayer()->GetTeamNumber() == TEAM_MI6 )
				ge_favcharMI6.SetValue( strargs );
			else
				ge_favcharJanus.SetValue( strargs );
		}
		else
		{
			ge_favchar.SetValue( strargs );
		}
	}
	
	SetVisible( false );
	gViewPortInterface->ShowBackGround( false );
	BaseClass::OnCommand(command);
}

void CGECharSelect::OnKeyCodePressed(KeyCode code)
{
	if ( m_iScoreBoardKey >= 0 && m_iScoreBoardKey == code )
	{
		gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, true );
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}

void CGECharSelect::UpdateCharacterList( void )
{
	// Get the list of characters
	GECharacters()->GetTeamMembers( C_BasePlayer::GetLocalPlayer()->GetTeamNumber(), m_vCharacters );

	// Remove exclusions
	CUtlVector<char *> exclusions;
	GEGameplayRes()->GetCharacterExclusion( exclusions );
	for ( int i=0; i < m_vCharacters.Count(); i++ )
	{
		for ( int k=0; k < exclusions.Count(); k++ )
		{
			if ( !Q_stricmp( exclusions[k], m_vCharacters[i]->szIdentifier ) )
			{
				m_vCharacters.FastRemove( i-- );
				break;
			}
		}
	}

	// Add random char to the front
	m_vCharacters.AddToHead( GECharacters()->GetRandomChar() );

	// Populate the first page of characters
	PopulateCharacterGrid();

	// Select the "default" character (may switch pages)
	SelectDefaultCharacter();
}

void CGECharSelect::SetSelectedCharacter( const char *ident, int skin /*=0*/ ) 
{
	int idx = FindCharacter( ident );
	if ( idx == -1 )
		idx = 0; // Default to first char (random)

	const CGECharData *pChar = m_vCharacters[idx];
	Q_strncpy( m_szCurrChar, pChar->szIdentifier, MAX_CHAR_IDENT );

	// Set the character's name
	m_pCharName->SetText( pChar->szPrintName );

	// Set the character's bio
	m_pCharBio->SetText( "" );
	m_pCharBio->InsertCenterText( false );
	m_pCharBio->InsertString( pChar->szBio );

	// Make sure we are on this character's page
	int cols, rows;
	m_pCharGrid->GetDimensions( cols, rows );

	int per_page = rows * cols;
	int offset = m_iCurrPage * per_page;
	
	if ( per_page > 0 && (idx < offset || idx > (offset + per_page)) )
	{
		// Calculate what page we are on and set it
		int tmp = 0, page = 0;
		while ( tmp < idx ) { tmp += per_page; page++; }
		PopulateCharacterGrid( page );
	}
	
	// Set the char panel to be selected
	for ( int y=0; y < rows; y++ )
	{
		for ( int x=0; x < cols; x++ )
		{
			GECharPanel *panel = (GECharPanel*) m_pCharGrid->GetEntry( x, y );
			if ( (offset + x) == idx )
				panel->SetSelected( true );
			else
				panel->SetSelected( false );
		}

		offset += cols;
	}

	// Set the characters skins (non-random && > 1 skin)
	m_pCharSkins->ClearItems();
	m_iCurrSkin = skin;
	
	// Set the characters model
	if ( !Q_stricmp(ident, CHAR_RANDOM_IDENT) )
	{
		// Show the random selection model
		m_pCharModel->SwapModel( CHARSELECT_RANDOM_MDL );
	}
	else
	{
		char N_image[128];
		char S_image[128];

		for ( int i=0; i < pChar->m_pSkins.Count(); i++ )
		{
			// Don't show the skin if we don't have a preview (should always have one)
			if ( !pChar->m_pSkins[i]->szPreviewImg[0] )
				continue;

			Q_snprintf( N_image, 128, "charselect/skins/%s_N", pChar->m_pSkins[i]->szPreviewImg );
			Q_snprintf( S_image, 128, "charselect/skins/%s_S", pChar->m_pSkins[i]->szPreviewImg );

			ImageButton *skinPanel = new ImageButton( m_pCharSkins, "SkinBtn", "", this );
			skinPanel->SetCommand( new KeyValues("SkinClicked", "id", i) );
			skinPanel->AddActionSignalTarget( this );
			skinPanel->SetImages( N_image, N_image, S_image );
			skinPanel->SetSize( m_pCharSkins->GetWide(), m_pCharSkins->GetWide() );
			skinPanel->SetTabPosition( i ); // HACK
			m_pCharSkins->AddItem( skinPanel, GEVertListPanel::LF_AUTOSIZE );
		}

		// Officially select the skin
		KeyValues *kv = new KeyValues( "SkinClicked", "id", m_iCurrSkin );
		SkinClicked( kv );
		kv->deleteThis();

		m_pCharSkins->InvalidateLayout( true );
	}
}

void CGECharSelect::SelectDefaultCharacter( void )
{
	if ( m_szCurrChar[0] )
	{
		// If we already had one selected, setup the panel with him again
		SetSelectedCharacter( m_szCurrChar, m_iCurrSkin );
	}
	else
	{
		const char *favchar = NULL;
		if ( GEMPRules()->IsTeamplay() )
		{
			if ( CBasePlayer::GetLocalPlayer()->GetTeamNumber() == TEAM_MI6 )
				favchar = ge_favcharMI6.GetString();
			else
				favchar = ge_favcharJanus.GetString();
		}
		else
		{
			favchar = ge_favchar.GetString();
		}

		CUtlVector<char *> args;
		Q_SplitString( favchar, " ", args );

		if ( args.Count() && args[0][0] != '\0' )
		{
			int skin = 0;
			if ( args.Count() > 1 )
				skin = atoi( args[1] );
			
			SetSelectedCharacter( args[0], skin );
		}
		else
			SetSelectedCharacter( CHAR_RANDOM_IDENT );

		args.PurgeAndDeleteElements();
	}
}

void CGECharSelect::ResetCharacterGrid( void )
{
	int rows, cols;
	m_pCharGrid->GetDimensions( cols, rows );

	for ( int y=0; y < rows; y++ )
	{
		for ( int x=0; x < cols; x++ )
		{
			Panel *panel = new GECharPanel(m_pCharGrid, "charimage");
			panel->AddActionSignalTarget( this );
			m_pCharGrid->SetEntry( x, y, panel );
		}
	}
}

void CGECharSelect::PopulateCharacterGrid( int page /* = 0 */ )
{
	int rows, cols;
	m_pCharGrid->GetDimensions( cols, rows );
	m_pCharGrid->AutoSizeCells();

	// Cache our page
	m_iCurrPage = page;

	// This is the offset from the characters array for our pages
	int offset = rows*cols*page;

	for ( int y=0; y < rows; y++ )
	{
		for ( int x=0; x < cols; x++ )
		{
			GECharPanel *panel = (GECharPanel*) m_pCharGrid->GetEntry( x, y );
			if ( (offset+x) >= m_vCharacters.Count() )
				panel->SetCharacter( NULL );
			else
				panel->SetCharacter( m_vCharacters[offset+x] );
		}
		// Move down a row in offset
		offset += cols;
	}
}

int CGECharSelect::FindCharacter( const char *ident )
{
	for ( int i=0; i < m_vCharacters.Count(); i++ )
	{
		if ( !Q_stricmp(m_vCharacters[i]->szIdentifier, ident) )
			return i;
	}
	return -1;
}

void CGECharSelect::CharacterClicked( KeyValues *data )
{
	const char *selected = data->GetString("ident", CHAR_RANDOM_IDENT);
	if ( Q_stricmp( m_szCurrChar, selected ) )
		SetSelectedCharacter( selected );
}

void CGECharSelect::CharacterDoubleClicked( KeyValues *data )
{
	CharacterClicked( data );
	OnCommand( "Ok" );
}

void CGECharSelect::SkinClicked( KeyValues *data )
{ 
	int idx = FindCharacter( m_szCurrChar );
	if ( idx >= 0 )
	{
		const CGECharData *pChar = m_vCharacters[idx];
		m_iCurrSkin = clamp( data->GetInt("id"), 0, pChar->m_pSkins.Count()-1 );

		m_pCharModel->SwapModel( pChar->m_pSkins[m_iCurrSkin]->szModel, "models/weapons/pp7/w_pp7.mdl" );
		m_pCharModel->m_pModelInfo->m_nSkin = pChar->m_pSkins[m_iCurrSkin]->iWorldSkin;

		// Set this skin as selected
		for ( int i=0; i < m_pCharSkins->Count(); i++ )
		{
			// Uses HACK from above (tab position)
			if ( m_pCharSkins->GetPanel(i)->GetTabPosition() == m_iCurrSkin )
				m_pCharSkins->SetSelectedPanel( m_iCurrSkin );
		}
	}
}

void CGECharSelect::FireGameEvent( IGameEvent *event )
{
	// Event: player_team
	// If the player changes teams an event is fired, to make sure we have the
	// most up-to-date team characters we need to read this message and grab
	// what team they joined
	C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	if ( pPlayer->GetUserID() == event->GetInt( "userid", 0 ) )
	{
		// Reset our chosen character
		m_szCurrChar[0] = '\0';
		
		// Hide the panel if we joined spectators
		int team = event->GetInt( "team", 0 );
		if ( team == TEAM_SPECTATOR )
			ShowPanel( false );
		else
			GetClientModeGENormal()->QueuePanel( PANEL_CHARSELECT, NULL );
	}
}

CON_COMMAND(charselect, "Brings up the character selection menu.")
{
	gViewPortInterface->ShowPanel( PANEL_CHARSELECT, true );
}

#ifdef DEBUG
CON_COMMAND( charselect_edit, "" )
{
	CGECharSelect *panel = (CGECharSelect*) gViewPortInterface->FindPanelByName( PANEL_CHARSELECT );
	panel->ShowPanel( true );
	panel->ActivateBuildMode();
}
#endif
