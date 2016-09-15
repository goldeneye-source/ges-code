///////////// Copyright � 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_scoreboard.cpp
// Description:
//      VGUI definition for the scoreboard
//
// Created On: 13 Sep 08
// Created By: Jonathan White <killermonkey>
////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "hud.h"
#include "ge_scoreboard.h"
#include "c_team.h"
#include "c_ge_playerresource.h"
#include "c_ge_player.h"
#include "ge_gamerules.h"
#include "gemp_gamerules.h"
#include "ge_utils.h"

#include <KeyValues.h>

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/SectionedListPanel.h>
#include <vgui_controls/ImageList.h>
// For Life Bars
#include <vgui_controls/AnimationController.h>
#include "iclientmode.h"

#include "voice_status.h"

using namespace vgui;

#define SCOREBOARD_BG_FFA "scoreboard/background_ffa"
#define SCOREBOARD_BG_TEAM "scoreboard/background_team"

// id's of sections used in the scoreboard
enum GEScoreboardSections
{
	SECTION_MI6 = 0,
	SECTION_JANUS,
	SECTION_FFA = 0,
	SECTION_SPEC = 2,
};

ConVar togglescores( "togglescores", "0", FCVAR_CLIENTDLL );

//-----------------------------------------------------------------------------
// Purpose: Constructor / Destructor
//-----------------------------------------------------------------------------
CGEScoreBoard::CGEScoreBoard(IViewPort *pViewPort):CClientScoreBoardDialog(pViewPort)
{
	m_pBackground = static_cast<vgui::ImagePanel*>(FindChildByName("Background"));

	m_bTeamPlay = false;
	m_pBackground->SetImage( SCOREBOARD_BG_FFA );

	// Know when we change game modes so we can show it
	ListenForGameEvent( "hostname_change" );
	ListenForGameEvent( "game_newmap" );

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	m_pImageList = NULL;
	m_bVisibleOnToggle = false;
}

CGEScoreBoard::~CGEScoreBoard()
{
}

void CGEScoreBoard::OnGameplayDataUpdate( void )
{
	// We'll post the message ourselves instead of using SetControlString()
	// so we don't try to translate the hostname.
	wchar_t gameplay[64];
	GEUTIL_GetGameplayName( gameplay, sizeof(gameplay) );
	
	Panel *control = FindChildByName( "GamePlayName" );
	if ( control )
		PostMessage( control, new KeyValues( "SetText", "text", gameplay ) );
}

void CGEScoreBoard::FireGameEvent( IGameEvent *event )
{
	const char *type = event->GetName();
	if ( Q_strcmp(type, "game_newmap") == 0 )
	{
		const char *mapname = event->GetString( "mapname" );
		Panel *control = FindChildByName( "MapName" );
		if ( control )
			PostMessage( control, new KeyValues( "SetText", "text", mapname ) );
	}
	else if ( Q_strcmp(type, "hostname_change") == 0 )
	{
		const char *hostname = event->GetString( "hostname" );
		Panel *control = FindChildByName( "ServerName" );
		if ( control )
		{
			PostMessage( control, new KeyValues( "SetText", "text", hostname ) );
			control->MoveToFront();
		}
	}
	else
		BaseClass::FireGameEvent(event);
}

void CGEScoreBoard::ShowPanel( bool bShow )
{
	BaseClass::ShowPanel(bShow);

	if ( bShow )
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("ShowStatusBar");
	else
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HideStatusBar");
}

void CGEScoreBoard::Update( void )
{
	m_pPlayerList->DeleteAllItems();
	
	PopulateScoreBoard();

	MoveToCenterOfScreen();

	UpdateMapTimer();

	// update every second
	m_fNextUpdateTime = gpGlobals->curtime + 1.0f; 
}

void CGEScoreBoard::OnTick( void )
{
	// If we are forcing this up, show it!
	if ( togglescores.GetBool() )
	{
		m_bVisibleOnToggle = true;
		ShowPanel( true );
	}
	else if ( m_bVisibleOnToggle )
	{
		m_bVisibleOnToggle = false;
		ShowPanel( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: sets up base sections
//-----------------------------------------------------------------------------
void CGEScoreBoard::InitScoreboardSections( void )
{
	// Just to make sure we are not showing this crap
	m_pPlayerList->SetBgColor( Color(0, 0, 0, 0) );
	m_pPlayerList->SetBorder(NULL);

	if ( GEMPRules()->IsTeamplay() )
	{
		// add the team sections
		AddSection( TYPE_TEAM, TEAM_MI6 );
		AddSection( TYPE_TEAM, TEAM_JANUS );

		m_pPlayerList->SetSectionMinimumHeight( GetSectionFromTeamNumber(TEAM_MI6), 113 );
		
		// This check is to make sure we aren't reloading the background everytime we open the scoreboard
		if ( !m_bTeamPlay && m_pBackground )
		{
			m_bTeamPlay = true;
			m_pBackground->SetImage( SCOREBOARD_BG_TEAM );
		}		
	}
	else
	{
		AddSection( TYPE_TEAM, TEAM_UNASSIGNED );

		// same comment as above
		if ( m_bTeamPlay && m_pBackground )
		{
			m_bTeamPlay = false;
			m_pBackground->SetImage( SCOREBOARD_BG_FFA );
		}	
	}
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new section to the scoreboard (i.e the team header)
//-----------------------------------------------------------------------------
void CGEScoreBoard::AddSection(int teamType, int teamNumber)
{
	// Used to load the fonts
	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );

	HFont hFallbackFont = pScheme->GetFont("Scoreboard", true);

	int sectionID = GetSectionFromTeamNumber( teamNumber );
	if ( teamType == TYPE_TEAM )
	{
 		m_pPlayerList->AddSection(sectionID, "", StaticPlayerSortFunc);

		// setup the columns
		m_pPlayerList->AddColumnToSection(sectionID, "deadicon", "", SectionedListPanel::COLUMN_CENTER | SectionedListPanel::COLUMN_IMAGE, 
			scheme()->GetProportionalScaledValue( GES_DEADICON_WIDTH ) );
		m_pPlayerList->AddColumnToSection(sectionID, "name", "", 0, scheme()->GetProportionalScaledValue( GES_NAME_WIDTH ), hFallbackFont );
		m_pPlayerList->AddColumnToSection(sectionID, "char", "#GE_Charname" , SectionedListPanel::COLUMN_CENTER, scheme()->GetProportionalScaledValue( GES_CHAR_WIDTH ) );
		m_pPlayerList->AddColumnToSection(sectionID, "score", "#GE_Score", SectionedListPanel::COLUMN_CENTER, scheme()->GetProportionalScaledValue( GES_SCORE_WIDTH ) );
		m_pPlayerList->AddColumnToSection(sectionID, "deaths", "#GE_Deaths", SectionedListPanel::COLUMN_CENTER, scheme()->GetProportionalScaledValue( GES_DEATH_WIDTH ) );
		m_pPlayerList->AddColumnToSection(sectionID, "ping", "#GE_Ping", SectionedListPanel::COLUMN_RIGHT, scheme()->GetProportionalScaledValue( GES_PING_WIDTH ) );

		// Add the death icon to the list panel
		m_pPlayerList->SetImageList( m_pImageList, false );

		// set the section to have the team color
		if ( teamNumber )
		{
			if ( GEPlayerRes() )
				m_pPlayerList->SetSectionFgColor(sectionID,  GEPlayerRes()->GetTeamColor(teamNumber));
		}
		
		m_pPlayerList->SetSectionFont( sectionID, pScheme->GetFont("Scoreboard", true) );
		m_pPlayerList->SetSectionDividerColor( sectionID, Color(0,0,0,0) );
		m_pPlayerList->SetSectionAlwaysVisible(sectionID);
	}
	else if ( teamType == TYPE_SPECTATORS )
	{
		m_pPlayerList->AddSection(sectionID, "");
		m_pPlayerList->AddColumnToSection(sectionID, "name", "#Spectators", 0, scheme()->GetProportionalScaledValueEx( GetScheme(), GES_NAME_WIDTH ), hFallbackFont );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Puts all our information onto the scoreboard
//-----------------------------------------------------------------------------
void CGEScoreBoard::PopulateScoreBoard( void )
{
	UpdateTeamInfo();
	UpdateTeamScores();
	UpdatePlayerInfo();
}

//-----------------------------------------------------------------------------
// Purpose: resets the scoreboard team info
//-----------------------------------------------------------------------------
void CGEScoreBoard::UpdateTeamInfo( void )
{
	if ( GEPlayerRes() == NULL )
		return;

	// update the team sections in the scoreboard
	C_Team *team = NULL;
	int sectionID = 0;

	for ( int i = TEAM_UNASSIGNED; i < MAX_GE_TEAMS; i++ )
	{
		team = GetGlobalTeam(i);
		if ( team )
		{
			sectionID = GetSectionFromTeamNumber( i );
	
			// update team name
			wchar_t string1[1024];
			wchar_t wNumPlayers[6];
			_snwprintf(wNumPlayers, 6, L"%i", team->Get_Number_Players() );

			if ( GEMPRules()->IsTeamplay() == false )									
			{
				g_pVGuiLocalize->ConstructString( string1, sizeof(string1), g_pVGuiLocalize->Find("#ScoreBoard_FFA"), 1, wNumPlayers );
			}
			else
			{
				wchar_t name[64];
				g_pVGuiLocalize->ConvertANSIToUnicode(team->Get_Name(), name, sizeof(name));
				g_pVGuiLocalize->ConstructString( string1, sizeof(string1), g_pVGuiLocalize->Find("#ScoreBoard_TeamPlay"), 2, name, wNumPlayers );
			}
		
			m_pPlayerList->ModifyColumn(sectionID, "name", string1);
		}
	}

	// Update our spectator listing
	UpdateSpectatorList();
}

void CGEScoreBoard::UpdateSpectatorList( void )
{
	C_Team *specTeam = GetGlobalTeam( TEAM_SPECTATOR );
	Panel *spectators = FindChildByName( "Spectators" );
	if ( !specTeam || !spectators )
		return;

	wchar_t wszSpecNames[1024];
	int iPlayers = specTeam->Get_Number_Players();
	wszSpecNames[0] = 0;

	if ( iPlayers > 0 )
	{
		char name[64];
		wchar_t wszNames[1024];
		wchar_t wszPlayer[MAX_PLAYER_NAME_LENGTH];
		wszNames[0] = 0;

		// Parse out the spectators' names into a list
		for ( int i=0; i < iPlayers; i++ )
		{
			if ( i > 0 )
				wcsncat( wszNames, L", ", sizeof(wszNames) );

			Q_strncpy( name, UTIL_SafeName(GEPlayerRes()->GetPlayerName( specTeam->m_aPlayers[i] )), 64 );
			g_pVGuiLocalize->ConvertANSIToUnicode( GEUTIL_RemoveColorHints(name), wszPlayer, sizeof(wszPlayer) );
			wcsncat( wszNames, wszPlayer, sizeof(wszNames) );
		}
		
		// Smash the spectator name list into our localization
		g_pVGuiLocalize->ConstructString(wszSpecNames, sizeof(wszSpecNames), g_pVGuiLocalize->Find("#ScoreBoard_Spectators"), 1, wszNames);
	}

	PostMessage(spectators, new KeyValues("SetText", "text", wszSpecNames));

	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
	spectators->SetFgColor( pScheme->GetColor( "Scoreboard_Spectators", Color(100,100,100,200) ) );
}

int CGEScoreBoard::GetSectionFromTeamNumber( int teamNumber )
{
	if ( GEMPRules()->IsTeamplay() )
	{
		if ( teamNumber == TEAM_JANUS )
			return SECTION_JANUS;
		else if ( teamNumber == TEAM_MI6 )
			return SECTION_MI6;
	}
	else if ( teamNumber == TEAM_UNASSIGNED )
	{
		return SECTION_FFA;
	}

	return -1;
}
//-----------------------------------------------------------------------------
// Purpose: Adds a new row to the scoreboard, from the playerinfo structure
//-----------------------------------------------------------------------------
bool CGEScoreBoard::GetPlayerScoreInfo( int playerIndex, KeyValues *kv )
{
	char name[64];
	Q_strncpy( name, GEPlayerRes()->GetPlayerName(playerIndex), 64 );
		
	kv->SetInt("playerIndex", playerIndex);
	kv->SetInt("team", GEPlayerRes()->GetTeam( playerIndex ) );
	kv->SetString("name", GEUTIL_RemoveColorHints(name) );
	kv->SetInt("deaths", GEPlayerRes()->GetDeaths( playerIndex ));
	kv->SetString("char", GEPlayerRes()->GetCharName( playerIndex ));

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
	// Get our developer status
	int iStatus = GEPlayerRes()->GetDevStatus(playerIndex);

	// If we are a developer we get a special icon
	if ( iStatus == GE_DEVELOPER )
	{
		if ( !GEPlayerRes()->IsAlive( playerIndex ) )
			kv->SetInt("deadicon", 4);
		else
			kv->SetInt("deadicon", 2);
	}
	else if ( iStatus == GE_BETATESTER )
	{
		if ( !GEPlayerRes()->IsAlive( playerIndex ) )
			kv->SetInt("deadicon", 4);
		else
			kv->SetInt("deadicon", 3);
	}
	else if (iStatus == GE_CONTRIBUTOR)
	{
		if (!GEPlayerRes()->IsAlive(playerIndex))
			kv->SetInt("deadicon", 1);
		else
			kv->SetInt("deadicon", 7);
	}
	else if ( iStatus == GE_SILVERACH )
	{
		if ( !GEPlayerRes()->IsAlive( playerIndex ) )
			kv->SetInt("deadicon", 1);
		else
			kv->SetInt("deadicon", 5);
	}
	else if ( iStatus == GE_GOLDACH )
	{
		if ( !GEPlayerRes()->IsAlive( playerIndex ) )
			kv->SetInt("deadicon", 1);
		else
			kv->SetInt("deadicon", 6);
	}
	else
	{
		if ( !GEPlayerRes()->IsAlive( playerIndex ) )
			kv->SetInt("deadicon", 1);
		else
			kv->SetInt("deadicon", 0);
	}
	
	if ( GEPlayerRes()->GetPing( playerIndex ) < 1 )
	{
		if ( GEPlayerRes()->IsFakePlayer( playerIndex ) )
		{
			kv->SetString("ping", "BOT");
		}
		else
		{
			kv->SetString("ping", "");
		}
	}
	else
	{
		kv->SetInt("ping", GEPlayerRes()->GetPing( playerIndex ));
	}
	
	return true;
}

enum {
	MAX_PLAYERS_PER_TEAM = 16,
	MAX_SCOREBOARD_PLAYERS = 32
};

//-----------------------------------------------------------------------------
// Purpose: Used for sorting players
//-----------------------------------------------------------------------------
bool CGEScoreBoard::StaticPlayerSortFunc(vgui::SectionedListPanel *list, int itemID1, int itemID2)
{
	KeyValues *it1 = list->GetItemData(itemID1);
	KeyValues *it2 = list->GetItemData(itemID2);
	Assert(it1 && it2);

	int v1, v2;

	// first compare frags
	if (!GEMPRules()->GetScoreboardMode())
	{
		v1 = it1->GetInt("score");
		v2 = it2->GetInt("score");

		if (v1 > v2)
			return true;
		else if (v1 < v2)
			return false;
	}
	else // Having to do this instead of just sorting by score and then translating score into the time string really bugs me but I'm not sure if it can be done.
	{
		const char* score1 = it1->GetString("score");
		const char* score2 = it2->GetString("score");

		int length1 = Q_strlen(score1);
		int length2 = Q_strlen(score2);

		// The bigger string will be the bigger number.
		if (length1 > length2)
			return true;
		else if (length2 > length1)
			return false;

		//However, they will usually end up the same length, so we'll need to compare them.
		int compval = Q_strcmp(score1, score2);

		if (compval < 0)
			return false;
		else if (compval > 0)
			return true;

		//If we made it this far they're equal.
	}

	// next compare deaths
	v1 = it1->GetInt("deaths");
	v2 = it2->GetInt("deaths");
	if (v1 > v2)
		return false;
	else if (v1 < v2)
		return true;

	// the same, so compare itemID's (as a sentinel value to get deterministic sorts)
	return itemID1 < itemID2;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGEScoreBoard::UpdatePlayerInfo()
{
	CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer || !GEPlayerRes() )
		return;

	KeyValues *playerData = new KeyValues("data");

	// walk all the players and make sure they're in the scoreboard
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		bool shouldShow = GEPlayerRes()->IsConnected( i );
		if ( shouldShow )
		{
			// add the player to the list
			GetPlayerScoreInfo( i, playerData );
			int itemID = FindItemIDForPlayerIndex( i );
  			int sectionID = GetSectionFromTeamNumber( GEPlayerRes()->GetTeam( i ) );
						
			if (itemID == -1)
				// add a new row
				itemID = m_pPlayerList->AddItem( sectionID, playerData );
			else
				// modify the current row
				m_pPlayerList->ModifyItem( itemID, sectionID, playerData );


			if ( i == pPlayer->entindex() && pPlayer->GetTeamNumber() != TEAM_SPECTATOR )
				// this is the local player, hilight this row
				m_pPlayerList->SetSelectedItem(itemID);

			// Obey Python's color scheme for this player, default is the team color
			int col = GEPlayerRes()->GetScoreBoardColor(i);
			IScheme *pScheme = scheme()->GetIScheme( GetScheme() );
			switch ( col )
			{
			case SB_COLOR_GOLD:
				m_pPlayerList->SetItemFgColor( itemID, pScheme->GetColor("SB_Gold", Color(232,180,2,255)) );
				break;
			case SB_COLOR_WHITE:
				m_pPlayerList->SetItemFgColor( itemID, pScheme->GetColor("SB_White", Color(255,255,255,255)) );
				break;
			case SB_COLOR_ELIMINATED:
				if ( GEMPRules()->IsTeamplay() )
					m_pPlayerList->SetItemFgColor( itemID, pScheme->GetColor("SB_Elim_Team", Color(0,0,0,255)) );
				else
					m_pPlayerList->SetItemFgColor( itemID, pScheme->GetColor("SB_Elim_FFA", Color(255,0,0,255)) );
				break;
			default:
				m_pPlayerList->SetItemFgColor( itemID, GEPlayerRes()->GetTeamColor( GEPlayerRes()->GetTeam(i) ) );
				break;
			}
		}
		else
		{
			// remove the player
			int itemID = FindItemIDForPlayerIndex( i );
			if (itemID != -1)
				m_pPlayerList->RemoveItem(itemID);
		}
	}

	playerData->deleteThis();
}

void CGEScoreBoard::UpdateTeamScores( void )
{
	Panel *mi6 = FindChildByName( "MI6Score" );
	Panel *janus = FindChildByName( "JanusScore" );

	if ( (!mi6 || !janus) || !GEMPRules() || !GEPlayerRes() )
		return;

	if ( GEMPRules()->IsTeamplay() )
	{
		wchar_t *scoreFmt = g_pVGuiLocalize->Find( "#ScoreBoard_TeamScore" );
		if ( !scoreFmt )
			scoreFmt = L"Score: %s1";

		mi6->SetVisible(true);
		janus->SetVisible(true);

		char rounds[8];
		wchar_t score[32], wszRounds[8];

		// Score for Janus
		Q_snprintf( rounds, 8, "%i", GetGlobalTeam(TEAM_JANUS)->GetRoundScore() );
		g_pVGuiLocalize->ConvertANSIToUnicode( rounds, wszRounds, sizeof(wszRounds) );

		g_pVGuiLocalize->ConstructString( score, sizeof(score), scoreFmt, 1, wszRounds );
		PostMessage( janus, new KeyValues( "SetText", "text", score ) );
		janus->SetFgColor( GEPlayerRes()->GetTeamColor(TEAM_JANUS) );

		// Score for MI6
		Q_snprintf( rounds, 8, "%i", GetGlobalTeam(TEAM_MI6)->GetRoundScore() );
		g_pVGuiLocalize->ConvertANSIToUnicode( rounds, wszRounds, sizeof(wszRounds) );

		g_pVGuiLocalize->ConstructString( score, sizeof(score), scoreFmt, 1, wszRounds );
		PostMessage( mi6, new KeyValues( "SetText", "text", score ) );
		mi6->SetFgColor( GEPlayerRes()->GetTeamColor(TEAM_MI6) );
	}
	else
	{
		mi6->SetVisible(false);
		janus->SetVisible(false);
	}
}

void CGEScoreBoard::UpdateMapTimer( void )
{
	if ( !GEMPRules() )
		return;

	Panel *control = FindChildByName( "TimeLeft" );
	if ( control )
	{
		float timeleft = GEMPRules()->GetMatchTimeRemaining();
		if ( timeleft > 0 )
		{
			int mins = timeleft / 60;
			int secs = timeleft - (mins*60);

			char time[6];
			Q_snprintf(time, 6, "%d:%.2d", mins, secs);
			PostMessage( control, new KeyValues( "SetText", "text", time ) );
		}
		else
			PostMessage( control, new KeyValues( "SetText", "text", "--:--" ) );		
	}
}

void CGEScoreBoard::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	if ( m_pImageList )
		delete m_pImageList;

	int size = scheme()->GetProportionalScaledValue( GES_DEADICON_WIDTH );

	m_pImageList = new vgui::ImageList(true);
	m_pImageList->AddImage( scheme()->GetImage( "scoreboard/player_dead", true ) );
	m_pImageList->AddImage( scheme()->GetImage( "scoreboard/ges_dev", true ) );
	m_pImageList->AddImage( scheme()->GetImage( "scoreboard/ges_bt", true ) );
	m_pImageList->AddImage( scheme()->GetImage( "scoreboard/ges_dev_dead", true ) );
	m_pImageList->AddImage( scheme()->GetImage( "scoreboard/ges_silverach", true ) );
	m_pImageList->AddImage( scheme()->GetImage( "scoreboard/ges_goldach", true ) );

	m_pImageList->GetImage(1)->SetSize( size, size );
	m_pImageList->GetImage(2)->SetSize( size, size );
	m_pImageList->GetImage(3)->SetSize( size, size );
	m_pImageList->GetImage(4)->SetSize( size, size );
}
