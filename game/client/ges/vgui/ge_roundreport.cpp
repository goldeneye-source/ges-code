///////////// Copyright � 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_roundreport.cpp
// Description:
//      VGUI definition for the End of Round Report
//
// Created On: 13 Dec 08
// Created By: Jonathan White <killermonkey>
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"

#include "hud.h"
#include <game/client/iviewport.h>
#include <vgui/IInput.h>
#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>

#include "ge_roundreport.h"
#include "ge_scoreboard.h"
#include "clientmode_ge.h"
#include "c_team.h"
#include "c_playerresource.h"
#include "c_ge_playerresource.h"
#include "c_gemp_player.h"
#include "ge_gamerules.h"
#include "gemp_gamerules.h"
#include "ge_utils.h"

#include <KeyValues.h>
#include "IGameUIFuncs.h" // for key bindings
#include "iclientmode.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define ROUNDREPORT_BG_FFA "roundreport/background_ffa"
#define ROUNDREPORT_BG_TEAM "roundreport/background_team"

ConVar cl_ge_disablecuemusic( "cl_ge_disablecuemusic", 0, FCVAR_ARCHIVE | FCVAR_CLIENTDLL, "Disables the special music that is played on round end..." );
ConVar cl_ge_hidereportguionspawn( "cl_ge_hidereportguionspawn", 0, FCVAR_ARCHIVE | FCVAR_CLIENTDLL, "This will make the End of Round Report hide as soon as you spawn in for the next round" );
// To take an auto-screenshot when this panel shows
extern ConVar hud_takesshots;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CGERoundReport::CGERoundReport(IViewPort *pViewPort) : EditablePanel(NULL, PANEL_ROUNDREPORT )
{
	m_pViewPort = pViewPort;
	m_bInTempHide = false;
	m_bTakeScreenshot = false;
	m_iScoreBoardKey = BUTTON_CODE_INVALID; // this is looked up in Activate()

	// load the new scheme early!!
	SetScheme(vgui::scheme()->LoadSchemeFromFile("resource/TeamMenuScheme.res", "TeamMenuScheme"));
	SetProportional( true );

	SetPaintBackgroundEnabled( false );
	SetPaintBorderEnabled( false );

	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( false );

	LoadControlSettings("resource/ui/RoundReport.res");

	InvalidateLayout();
	MakePopup();

	m_pBackground = dynamic_cast<vgui::ImagePanel*>( FindChildByName("Background") );
	m_pPlayerList = dynamic_cast<vgui::SectionedListPanel*>( FindChildByName("PlayerList") );

	ListenForGameEvent( "round_start" );
	ListenForGameEvent( "round_end" );
	ListenForGameEvent( "player_spawn" );

	SetVisible( false );

	m_RoundData.is_locked = false;
	m_RoundData.is_match = false;
	m_RoundData.is_teamplay = false;
	m_RoundData.winner_id = 0;
	m_RoundData.scenario_name[0] = '\0';

	m_pImageList = NULL;

	// Make sure we get ticked!
	vgui::ivgui()->AddTickSignal( GetVPanel() );

	m_pScheme = scheme()->GetIScheme( GetScheme() );
}

CGERoundReport::~CGERoundReport()
{
	// Remove our award data
	m_RoundData.awards.PurgeAndDeleteElements();
}

void CGERoundReport::FireGameEvent( IGameEvent *event )
{
	const char * name = event->GetName();

	if ( !Q_stricmp(name, "round_end") )
	{
		// Don't show the report if we asked not to
		if ( !event->GetBool( "showreport" ) )
			return;

		// Always play our music if we are scoring
		// So make sure our teamplay state is correct.
		m_RoundData.is_teamplay = GEMPRules()->IsTeamplay();

		// Do this here too, just in case.
		GEUTIL_GetGameplayName(m_RoundData.scenario_name, sizeof(m_RoundData.scenario_name));

		// Always play our music if we are scoring
		PlayWinLoseMusic();

		m_RoundData.round_count = event->GetInt( "roundcount" );

		// Record the results of the award winners
		if ( m_RoundData.is_teamplay )
			m_RoundData.winner_id = event->GetInt( "teamid" );
		else
			m_RoundData.winner_id = event->GetInt( "winnerid" );

		m_RoundData.is_match = event->GetBool( "isfinal" );

		// Clear out any prior awards
		m_RoundData.awards.PurgeAndDeleteElements();
		GERoundAward *award;
		char eventid[16], eventwinner[16];
		for ( int i=1; i <= 6; i++ )
		{
			award = new GERoundAward;
			Q_snprintf( eventid, 16, "award%i_id", i );
			Q_snprintf( eventwinner, 16, "award%i_winner", i );

			award->m_iAwardID = event->GetInt( eventid, -1 );
			award->m_iPlayerID = event->GetInt( eventwinner, -1 );

			m_RoundData.awards.AddToTail(award);
		}

		// Force hide us if we are showing the match report to ensure update
		if ( m_RoundData.is_match )
			ShowPanel( false );

		// Remove any latent queued reports and queue a new one
		GetClientModeGENormal()->ClearPanelQueue( PANEL_ROUNDREPORT );
		GetClientModeGENormal()->QueuePanel( PANEL_ROUNDREPORT, NULL );

		if ( hud_takesshots.GetBool() )
			m_bTakeScreenshot = true;

		// Unlock until we recieve the score data
		m_RoundData.is_locked = false;
	}
	else if ( !Q_stricmp(name, "round_start") )
	{
		// Store what our teamplay state is for use on end round
		m_RoundData.is_teamplay = event->GetBool( "teamplay" );

		// Store the gameplay name for the report
		GEUTIL_GetGameplayName( m_RoundData.scenario_name, sizeof(m_RoundData.scenario_name) );

		// Hide the panel if we want it gone
		if ( IsVisible() && cl_ge_hidereportguionspawn.GetBool() )
		{
			ShowPanel( false );
			m_bInTempHide = false;
		}
	}
	else if ( !Q_stricmp(name, "player_spawn") )
	{
		// Extra assurance that the panel will go away
		if ( IsVisible() && cl_ge_hidereportguionspawn.GetBool() )
		{
			ShowPanel( false );
			m_bInTempHide = false;
		}
	}
}

void CGERoundReport::OnGameplayDataUpdate()
{
	// HACK HACK
	// This is only really helpful when we first spawn into the map
	if ( m_RoundData.scenario_name[0] == '\0' )
		GEUTIL_GetGameplayName( m_RoundData.scenario_name, sizeof(m_RoundData.scenario_name) );
}

void CGERoundReport::OnCommand( const char *command )
{
	ShowPanel( false );
	BaseClass::OnCommand( command );
}

void CGERoundReport::Reset( void )
{
	// add all the sections
	InitReportSections();
	PopulatePlayerList();
	UpdateTeamScores();
	PopulateAwards();
	SetWinnerText();
}

void CGERoundReport::OnTick( void )
{
	// Always make sure we have control of the mouse!
	if ( IsVisible() )
		SetMouseInputEnabled( true );

	if ( !m_pScoreboard )
		FindOtherPanels();

	if ( IsVisible() )
	{
		if ( m_pCharMenu->IsVisible() || m_pTeamMenu->IsVisible() || m_pScoreboard->IsVisible() )
		{
			SetVisible( false );
			m_bInTempHide = true;
		}
	}
	else if ( m_bInTempHide )
	{
		if ( !( m_pCharMenu->IsVisible() || m_pTeamMenu->IsVisible() || m_pScoreboard->IsVisible() ) )
		{
			SetVisible( true );
			m_bInTempHide = false;
		}
	}
}

void CGERoundReport::ShowPanel( bool bShow )
{
	// If we are already showing we need to refresh the contents
	// so hide it, do our calcs, then show it again
	if ( IsVisible() == bShow && bShow )
		SetVisible( false );

	// This ensures we never overtake the scenario help
	if ( gViewPortInterface->FindPanelByName( PANEL_SCENARIOHELP )->IsVisible() )
	{
		SetVisible( false );
		return;
	}

	// Make sure we have valid handles
	FindOtherPanels();

	if ( bShow )
	{
		Reset();
		SetZPos( 1 );
		MoveToFront();
		m_bInTempHide = false;

		// Reset our background
		if ( m_pBackground )
		{
			if ( m_RoundData.is_teamplay )
				m_pBackground->SetImage( ROUNDREPORT_BG_TEAM );
			else
				m_pBackground->SetImage( ROUNDREPORT_BG_FFA );
		}

		SetMouseInputEnabled( true );
		m_pPlayerList->SetMouseInputEnabled( true );
		Panel *control = FindChildByName( "CancelButton" );
		if ( control )
			control->SetMouseInputEnabled( true );

		SetKeyBoardInputEnabled( false );
		vgui::surface()->CalculateMouseVisible();

		if ( m_iScoreBoardKey == BUTTON_CODE_INVALID ) 
			m_iScoreBoardKey = gameuifuncs->GetButtonCodeForBind( "showscores" );

		// Setup endgame screenshot if we want it
		if ( m_bTakeScreenshot )
		{
			gHUD.SetScreenShotTime( gpGlobals->curtime + 0.5f );
			m_bTakeScreenshot = false;
		}

		m_RoundData.is_locked = true;
	}
	else
	{
		SetMouseInputEnabled( false );
	}

	SetVisible( bShow );
}

void CGERoundReport::PlayWinLoseMusic( void )
{
	// Don't play the tunes if we disabled it
	if ( cl_ge_disablecuemusic.GetBool() )
		return;

	const char *soundfile = NULL;

	CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	CSingleUserRecipientFilter user( pPlayer );

	if ( m_RoundData.is_teamplay && m_RoundData.winner_id != TEAM_UNASSIGNED )
	{
		if ( m_RoundData.winner_id == TEAM_JANUS )
		{
			if ( pPlayer->GetTeamNumber() == TEAM_JANUS )
				soundfile = "GEGamePlay.JanusWin";
			else
				soundfile = "GEGamePlay.MI6Lose";
		}
		else
		{
			if ( pPlayer->GetTeamNumber() == TEAM_MI6 )
				soundfile = "GEGamePlay.MI6Win";
			else
				soundfile = "GEGamePlay.JanusLose";
		}
	}
	else
	{
		soundfile = "GEGamePlay.GameOver";
	}

	pPlayer->EmitSound( user, pPlayer->entindex(), soundfile );

	IGameEvent *event = gameeventmanager->CreateEvent( "specialmusic_setup" );
	if ( event )
	{
		event->SetInt( "playerid", pPlayer->GetUserID() );
		event->SetInt( "teamid", -1 );
		event->SetString( "soundfile", soundfile );
		gameeventmanager->FireEventClientSide( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets up base sections
//-----------------------------------------------------------------------------
void CGERoundReport::InitReportSections( void )
{
	if ( m_RoundData.is_locked )
		return;

	// Set the title
	Panel *control = FindChildByName( "ReportTitle" );
	if ( control )
	{
		if ( m_RoundData.is_match )
			PostMessage( control, new KeyValues( "SetText", "text", "#RoundReport_MatchTitle" ) );
		else
			PostMessage( control, new KeyValues( "SetText", "text", "#RoundReport_RoundTitle" ) );
	}

	// Just to make sure we are not showing this crap
	m_pPlayerList->SetBgColor( Color(0, 0, 0, 0) );
	m_pPlayerList->SetBorder(NULL);

	// Clear all the sections and items
	m_pPlayerList->DeleteAllItems();
	m_pPlayerList->RemoveAllSections();

	// Add the section depending on teamplay
	if ( m_RoundData.is_teamplay )
		AddSection( TYPE_TEAM );
	else
		AddSection( TYPE_PLAYERS );

	// Set the gameplay
	control = FindChildByName( "GamePlayName" );
	if ( control && GEGameplayRes() )
	{
		PostMessage( control, new KeyValues( "SetText", "text", m_RoundData.scenario_name ) );
		control->SetFgColor( m_pScheme->GetColor( "RoundReport_Default", Color(0,0,0,255) ) );
	}

	control = FindChildByName( "MI6Score_Label" );
	if ( control )
	{
		control->SetVisible( m_RoundData.is_teamplay );
		control->SetFgColor( m_pScheme->GetColor("RoundReport_MI6", Color(0,0,0,255)) );
	}
	control = FindChildByName( "MI6Score" );
	if ( control )
	{
		control->SetVisible( m_RoundData.is_teamplay );
		control->SetFgColor( m_pScheme->GetColor("RoundReport_MI6", Color(0,0,0,255)) );
	}
	control = FindChildByName( "JanusScore_Label" );
	if ( control )
	{
		control->SetVisible( m_RoundData.is_teamplay );
		control->SetFgColor( m_pScheme->GetColor("RoundReport_Janus", Color(0,0,0,255)) );
	}
	control = FindChildByName( "JanusScore" );
	if ( control )
	{
		control->SetVisible( m_RoundData.is_teamplay );
		control->SetFgColor( m_pScheme->GetColor("RoundReport_Janus", Color(0,0,0,255)) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new section to the report
//-----------------------------------------------------------------------------
void CGERoundReport::AddSection(int type )
{
	int sectionID = 0;

	m_pPlayerList->AddSection(sectionID, "", CGEScoreBoard::StaticPlayerSortFunc);
	// setup the columns
	m_pPlayerList->AddColumnToSection(sectionID, "icon", "", SectionedListPanel::COLUMN_CENTER | SectionedListPanel::COLUMN_IMAGE, 
	scheme()->GetProportionalScaledValueEx( GetScheme(), GES_ICON_WIDTH ) );

	m_pPlayerList->AddColumnToSection(sectionID, "name", "#GE_PlayerName", 0, scheme()->GetProportionalScaledValueEx( GetScheme(), GES_NAME_WIDTH ) );

	if ( type == TYPE_TEAM )
 		m_pPlayerList->AddColumnToSection(sectionID, "team", "#GE_Team" , SectionedListPanel::COLUMN_CENTER, scheme()->GetProportionalScaledValueEx( GetScheme(), GES_TEAM_WIDTH ) );
	else
		m_pPlayerList->AddColumnToSection(sectionID, "team", "" , SectionedListPanel::COLUMN_CENTER, scheme()->GetProportionalScaledValueEx( GetScheme(), GES_TEAM_WIDTH ) );

	m_pPlayerList->AddColumnToSection(sectionID, "char", "#GE_Charname" , SectionedListPanel::COLUMN_CENTER, scheme()->GetProportionalScaledValueEx( GetScheme(), GES_CHAR_WIDTH ) );
	m_pPlayerList->AddColumnToSection(sectionID, "score", "#GE_Score", SectionedListPanel::COLUMN_CENTER, scheme()->GetProportionalScaledValueEx( GetScheme(), GES_SCORE_WIDTH ) );
	m_pPlayerList->AddColumnToSection(sectionID, "deaths", "#GE_Deaths", SectionedListPanel::COLUMN_CENTER, scheme()->GetProportionalScaledValueEx( GetScheme(), GES_DEATH_WIDTH ) );
	m_pPlayerList->AddColumnToSection(sectionID, "favweapon", "#GE_FavWeapon", SectionedListPanel::COLUMN_CENTER, scheme()->GetProportionalScaledValueEx( GetScheme(), GES_FAVWEAPON_WIDTH ) );

	m_pPlayerList->SetImageList( m_pImageList, false );

	m_pPlayerList->SetSectionFgColor( sectionID, m_pScheme->GetColor( "RoundReport_Default", Color(0,0,0,255) ) );
	m_pPlayerList->SetSectionFont( sectionID, m_pScheme->GetFont("TypeWriter_Small", true) );
	m_pPlayerList->SetSectionDividerColor( sectionID, Color(0,0,0,0) );
	m_pPlayerList->SetSectionAlwaysVisible(sectionID);
	m_pPlayerList->SetVerticalScrollbar(true);

	if ( type == TYPE_TEAM )
	{
		m_pPlayerList->SetPos( scheme()->GetProportionalScaledValue(23), YRES(133) );
		m_pPlayerList->SetSize( scheme()->GetProportionalScaledValue(448), YRES(104) );
	}
	else
	{
		m_pPlayerList->SetPos( scheme()->GetProportionalScaledValue(23), YRES(101) );
		m_pPlayerList->SetSize( scheme()->GetProportionalScaledValue(448), YRES(137) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds players to the score board
//-----------------------------------------------------------------------------
void CGERoundReport::PopulatePlayerList( void )
{
	CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer || !GEPlayerRes() || m_RoundData.is_locked )
		return;

	// walk all the players and make sure they're in the scoreboard
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		if ( GEPlayerRes()->IsConnected(i) )
		{
			// Show spectators only if they scored in the round / match
			if ( !GEPlayerRes()->IsActive(i) && GEPlayerRes()->GetPlayerScore(i) == 0 )
				continue;

			// add the player to the list
			KeyValues *playerData = new KeyValues("data");
			GetPlayerScoreInfo( i, playerData );

  			int sectionID = 0;
			int itemID = m_pPlayerList->AddItem( sectionID, playerData );

			// Default color
			Color color = m_pScheme->GetColor( "RoundReport_Default", Color(0,0,0,255) );

			// The local player gets highlighted
			if ( i == pPlayer->entindex() )
				color = m_pScheme->GetColor( "RoundReport_MyPlayer", Color(186,79,0,255) );
			// Non-Active players are "grayed out"
			else if ( !GEPlayerRes()->IsActive(i) )
				color = m_pScheme->GetColor( "RoundReport_Spectator", Color(150,150,150,255) );
			// Team players are colored their team
			else if ( m_RoundData.is_teamplay && GEPlayerRes()->GetTeam(i) == TEAM_JANUS )
				color = m_pScheme->GetColor( "RoundReport_Janus", Color(142,37,37,255) );
			else if ( m_RoundData.is_teamplay && GEPlayerRes()->GetTeam(i) == TEAM_MI6 )
				color = m_pScheme->GetColor( "RoundReport_MI6", Color(37,50,142,255) );

			m_pPlayerList->SetItemFgColor( itemID, color );

			playerData->deleteThis();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new row to the scoreboard, from the playerinfo structure
//-----------------------------------------------------------------------------
bool CGERoundReport::GetPlayerScoreInfo( int playerIndex, KeyValues *kv )
{
	kv->SetInt("playerIndex", playerIndex);

	int iStatus = GEPlayerRes()->GetDevStatus(playerIndex);

	if ( GEPlayerRes()->IsFakePlayer( playerIndex ) )
		kv->SetInt( "icon", 5 );
	else if ( iStatus == GE_DEVELOPER )
		kv->SetInt("icon", 1);
	else if ( iStatus == GE_BETATESTER )
		kv->SetInt("icon", 2);
	else if ( iStatus == GE_CONTRIBUTOR )
		kv->SetInt("icon", 6);
	else if ( iStatus == GE_SILVERACH )
		kv->SetInt("icon", 3);
	else if ( iStatus == GE_GOLDACH )
		kv->SetInt("icon", 4);
	else
		kv->SetInt("icon", 0);

	char name[64];
	Q_strncpy( name, GEPlayerRes()->GetPlayerName(playerIndex), 64 );

	kv->SetString("name", GEUTIL_RemoveColorHints(name) );
	if ( m_RoundData.is_teamplay )
		kv->SetString("team", GEPlayerRes()->GetTeamName( GEPlayerRes()->GetTeam(playerIndex) ) );
	kv->SetInt("deaths", GEPlayerRes()->GetDeaths( playerIndex ));
	kv->SetString("favweapon", GetWeaponPrintName( GEPlayerRes()->GetFavoriteWeapon(playerIndex) ));
	kv->SetString( "char", GEPlayerRes()->GetCharName( playerIndex ) );

	if (!GEMPRules()->GetScoreboardMode())
		kv->SetInt("score", GEPlayerRes()->GetPlayerScore(playerIndex));
	else
	{
		int seconds = GEPlayerRes()->GetPlayerScore(playerIndex);
		int displayseconds = seconds % 60;
		int displayminutes = floor(seconds / 60);

		Q_snprintf(name, sizeof(name), "%d:%s%d", displayminutes, displayseconds < 10 ? "0" : "", displayseconds);
		kv->SetString("score", name);
	}

	return true;
}

void CGERoundReport::SetWinnerText( void )
{
	Panel *control = FindChildByName( "Winner_Label" );
	if ( !control || m_RoundData.is_locked )
		return;

	CBasePlayer *pPlayer = UTIL_PlayerByIndex( m_RoundData.winner_id );
	wchar_t name[32];
	wchar_t string[128];

	// Early out if there's a tie
	if ( !pPlayer || (m_RoundData.is_teamplay && m_RoundData.winner_id == TEAM_UNASSIGNED) )
	{
		if ( m_RoundData.is_match )
			g_pVGuiLocalize->ConstructString( string, sizeof(string), g_pVGuiLocalize->Find("#RoundReport_TieFinal"), 0 );
		else
			g_pVGuiLocalize->ConstructString( string, sizeof(string), g_pVGuiLocalize->Find("#RoundReport_TieRound"), 0 );
	}
	else
	{
		char player_name[32] = "";
		Q_strncpy( player_name, pPlayer->GetPlayerName(), 32 );

		if ( m_RoundData.is_teamplay )
			g_pVGuiLocalize->ConvertANSIToUnicode( GEPlayerRes()->GetTeamName( m_RoundData.winner_id ), name, sizeof(name) );
		else
			g_pVGuiLocalize->ConvertANSIToUnicode( GEUTIL_RemoveColorHints( player_name ), name, sizeof(name) );
		
		if ( m_RoundData.is_match )
			g_pVGuiLocalize->ConstructString( string, sizeof(string), g_pVGuiLocalize->Find("#RoundReport_WinnerFinal"), 1, name );
		else
			g_pVGuiLocalize->ConstructString( string, sizeof(string), g_pVGuiLocalize->Find("#RoundReport_WinnerRound"), 1, name );
	}

	PostMessage( control, new KeyValues( "SetText", "text", string ) );
	control->SetVisible( true );
}

void CGERoundReport::PopulateAwards( void )
{
	if ( m_RoundData.is_locked )
		return;

	CBasePlayer *pPlayer;
	char panelName[16];
	int i,j;

	for ( i=0,j=1; i < m_RoundData.awards.Count(); i++ )
	{
		Q_snprintf( panelName, 16, "Award%i", j );

		pPlayer = UTIL_PlayerByIndex( m_RoundData.awards[i]->m_iPlayerID );
		if ( !pPlayer )
			continue;
		if ( m_RoundData.awards[i]->m_iAwardID == -1 )
			continue;

		SetupAwardPanel( panelName, pPlayer, m_RoundData.awards[i]->m_iAwardID );
		Msg( "%i. %s: %s\n", j, pPlayer->GetPlayerName(), AwardIDToIdent( m_RoundData.awards[i]->m_iAwardID ) );
		j++;
	}

	// Fill in the rest of the award panels with NONE
	for ( ; j <= 6; j++ )
	{
		Q_snprintf( panelName, 16, "Award%i", j );
		SetupAwardPanel( panelName, NULL, -1 );
	}
}

void CGERoundReport::SetupAwardPanel( const char *name, CBasePlayer *player, int iAward )
{
	EditablePanel *control = (EditablePanel*)FindChildByName( name );
	ImagePanel *img = (ImagePanel*)control->FindChildByName( "AwardIcon" );
	if ( !control || !img )
		return;

	// Hide the check  mark by default
	control->SetControlVisible( "CheckIcon", false );

	// We don't have an award, so show the basics
	if ( !player || iAward == -1 )
	{
		control->SetControlString( "AwardName", "#GE_AWARD_NONE" );
		control->SetControlString( "PlayerName", "" );
		img->SetImage( "roundreport/award_none" );
	}
	else
	{
		char name[64];
		Q_strncpy( name, player->GetPlayerName(), 64 );

		control->SetControlString( "AwardName", GetAwardPrintName(iAward) );
		control->SetControlString( "PlayerName", GEUTIL_RemoveColorHints( name ) );
		// If the local player won this award, make it obvious by showing a check mark
		if ( player == CBasePlayer::GetLocalPlayer() )
			control->SetControlVisible( "CheckIcon", true );

		char icon[64];
		Q_snprintf( icon, 64, "roundreport/%s", GetIconForAward(iAward) );
		img->SetImage( icon );
	}
}		

void CGERoundReport::UpdateTeamScores( void )
{
	Panel *mi6 = FindChildByName( "MI6Score" );
	Panel *janus = FindChildByName( "JanusScore" );

	if ( (!mi6 || !janus) || !GERules() || !GEPlayerRes() || m_RoundData.is_locked )
		return;

	if ( m_RoundData.is_teamplay )
	{
		mi6->SetVisible(true);
		janus->SetVisible(true);

		char score_text[32];
		int score;
		
		// Janus Scores
		if ( m_RoundData.is_match )
			score = GetGlobalTeam(TEAM_JANUS)->GetMatchScore();
		else
			score = GetGlobalTeam(TEAM_JANUS)->GetRoundScore();
			
		Q_snprintf( score_text, sizeof(score_text), "%i", score );
		PostMessage( janus, new KeyValues( "SetText", "text", score_text ) );

		// MI6 Scores
		if ( m_RoundData.is_match )
			score = GetGlobalTeam(TEAM_MI6)->GetMatchScore();
		else
			score = GetGlobalTeam(TEAM_MI6)->GetRoundScore();
		
		Q_snprintf( score_text, sizeof(score_text), "%i", score );
		PostMessage( mi6, new KeyValues( "SetText", "text", score_text ) );
	}
	else
	{
		mi6->SetVisible(false);
		janus->SetVisible(false);
	}
}

Panel *CGERoundReport::CreateControlByName(const char *controlName)
{
    if ( Q_stricmp( controlName, "CGEAwardPanel" ) == 0 )
    {
        return new CGEAwardPanel( this, controlName );
    }

    return BaseClass::CreateControlByName( controlName );
}

void CGERoundReport::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	if ( m_pImageList )
		delete m_pImageList;

	int size = scheme()->GetProportionalScaledValue( GES_ICON_WIDTH );

	m_pImageList = new vgui::ImageList(true);
	m_pImageList->AddImage( scheme()->GetImage( "roundreport/ges_dev", true ) );
	m_pImageList->AddImage( scheme()->GetImage( "roundreport/ges_bt", true ) );
	m_pImageList->AddImage( scheme()->GetImage( "scoreboard/ges_silverach", true ) );
	m_pImageList->AddImage( scheme()->GetImage( "scoreboard/ges_goldach", true ) );
	m_pImageList->AddImage( scheme()->GetImage( "roundreport/ges_bot", true ) );
	m_pImageList->AddImage( scheme()->GetImage( "roundreport/ges_cc", true ));

	m_pImageList->GetImage(1)->SetSize( size, size );
	m_pImageList->GetImage(2)->SetSize( size, size );
	m_pImageList->GetImage(5)->SetSize( size, size );
	m_pImageList->GetImage(6)->SetSize( size, size );

	FindOtherPanels();
}

void CGERoundReport::FindOtherPanels( void )
{
	m_pScoreboard = gViewPortInterface->FindPanelByName( PANEL_SCOREBOARD );
	m_pTeamMenu = gViewPortInterface->FindPanelByName( PANEL_TEAM );
	m_pCharMenu = gViewPortInterface->FindPanelByName( PANEL_CHARSELECT );
}

void CGERoundReport::OnKeyCodePressed(KeyCode code)
{
	if ( m_iScoreBoardKey != BUTTON_CODE_INVALID && m_iScoreBoardKey == code )
	{
		gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, true );
		gViewPortInterface->PostMessageToPanel( PANEL_SCOREBOARD, new KeyValues( "PollHideCode", "code", code ) );
	}
	else if ( code == KEY_ESCAPE || code == KEY_ENTER )
	{
		ShowPanel(false);
	}
	else
	{
		BaseClass::OnKeyCodePressed(code);
	}
}

#ifdef DEBUG
CON_COMMAND( show_roundreport, "Show the round report" )
{
	gViewPortInterface->ShowPanel( PANEL_ROUNDREPORT, true );
}

CON_COMMAND( edit_roundreport, "" )
{
	CGERoundReport *panel = (CGERoundReport*) gViewPortInterface->FindPanelByName( PANEL_ROUNDREPORT );
	panel->ShowPanel( true );
	panel->ActivateBuildMode();
}
#endif
