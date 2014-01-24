///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// ge_hudinfomsg.cpp
//
// Description:
//      Displays a ChatLike info area for kills.
//
// Created On: 11/9/2008
// Created By: Jonathan White <killermonkey>
////////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include <hud_basechat.h>
#include "hud_macros.h"
#include "ihudlcd.h"
#include "vgui/ILocalize.h"
#include "c_playerresource.h"
#include "c_ge_playerresource.h"
#include "ge_gamerules.h"
#include "gemp_gamerules.h"
#include "ge_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

enum InfoColor
{
	//COLOR_NORMAL = 1,
	COLOR_SUICIDE = 6,
	COLOR_KILLER = 7,
	COLOR_VICTIM = 8,
	COLOR_WEAPON = 9,
	//COLOR_MAX
};

// Player entries in a death notice
struct DeathNoticePlayer
{
	char		szName[MAX_PLAYER_NAME_LENGTH];
	int			iEntIndex;
};

// Contents of each entry in our list of death notices
struct DeathNoticeItem 
{
	DeathNoticePlayer	Killer;
	DeathNoticePlayer   Victim;
	bool		bSuicide;
	float		flDisplayTime;
	char		killedWith[MAX_PLAYER_NAME_LENGTH];
	char		article[4];
	wchar_t		*wszFormat;
};

class CGEHudDeathNoticeLine : public CBaseHudChatLine
{
	DECLARE_CLASS_SIMPLE( CGEHudDeathNoticeLine, CBaseHudChatLine );

public:
	CGEHudDeathNoticeLine( vgui::Panel *parent, const char *panelName ) : CBaseHudChatLine( parent, panelName )
	{
	}

	virtual void ApplySchemeSettings(vgui::IScheme *pScheme)
	{
		BaseClass::ApplySchemeSettings( pScheme );
		
		m_hFont = pScheme->GetFont( "InfoTextFont" );
		SetBorder( NULL );
		SetBgColor( Color( 0, 0, 0, 0 ) );
		SetFgColor( Color( 0, 0, 0, 0 ) );

		SetFont( m_hFont );
	}

	void MsgFunc_InfoText(bf_read &msg)
	{
	}


private:
	CGEHudDeathNoticeLine( const CGEHudDeathNoticeLine & ); // not defined, not accessible
};

class CGEHudDeathNotice : public CBaseHudChat
{
	DECLARE_CLASS_SIMPLE( CGEHudDeathNotice, CBaseHudChat );

public:
	CGEHudDeathNotice( const char *pElementName );
	virtual void CreateChatInputLine( void );
	virtual void CreateChatLines( void );
	virtual void Init( void );
	virtual void Reset( void );
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);

	// Handle all events based on filter given in class init
	virtual void FireGameEvent( IGameEvent * event );

	void DeathPrintf( void );

	virtual Color GetDefaultTextColor( void );
	virtual Color GetTextColorForClient( wchar_t colorNum, int clientIndex );

	int	GetChatInputOffset( void );

private:
	void GetEnglishArticle( const char *word, char *article );

	DeathNoticeItem		m_DeathMsgInfo;
};


DECLARE_HUDELEMENT( CGEHudDeathNotice );

CGEHudDeathNotice::CGEHudDeathNotice( const char *pElementName )
	: BaseClass( "GEDeathNotice" )
{
	SetScheme( "ClientScheme" );

	ListenForGameEvent( "player_death" );
	ListenForGameEvent( "death_message" );
}
//This pointer is used in base class, so it must be created even though it isn't used.
void CGEHudDeathNotice::CreateChatInputLine( void )
{
	m_pChatInput = new CBaseHudChatInputLine( this, "InfoNotUsedInputLine" );
	m_pChatInput->SetVisible( false );
}
void CGEHudDeathNotice::CreateChatLines( void )
{
	m_ChatLine = new CGEHudDeathNoticeLine( this, "InfoLine1" );
	m_ChatLine->SetVisible( false );
}
void CGEHudDeathNotice::Init( void )
{
	BaseClass::Init();
	SetBgColor( Color( 0, 0, 0, 0 ) );
}
void CGEHudDeathNotice::Reset( void )
{
	SetBgColor( Color( 0, 0, 0, 0 ) );
	BaseClass::Reset();
}
void CGEHudDeathNotice::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	LoadControlSettings( "resource/UI/DeathMessage.res" );

	// Skip over the Base Chat definition, it's all in here with OUR .res file
	vgui::EditablePanel::ApplySchemeSettings( pScheme );

	SetBgColor( Color( 0, 0, 0, 0 ) );

	SetPaintBorderEnabled( false );
	SetPaintBackgroundEnabled( false );
	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( false );
	GetChatHistory()->SetVerticalScrollbar( false );
}
void CGEHudDeathNotice::FireGameEvent( IGameEvent * event )
{	
	// Ignore this event
	if ( !Q_stricmp( event->GetName(), "hltv_chat" ) )
		return;

	if ( !Q_stricmp( event->GetName(), "death_message" ) )
	{
		// Show a simple message from the scenario
		char message[128];
		wchar_t wszMessage[128];
		GEUTIL_ParseLocalization( wszMessage, 128, event->GetString("message") );
		g_pVGuiLocalize->ConvertUnicodeToANSI( wszMessage, message, 128 );

		// Print the death notice in the HUD
		ChatPrintf( 0, CHAT_FILTER_NONE, message );
		return;
	}

	if ( !GEPlayerRes() )
		return;

	// the event should be "player_death"
	int killer = engine->GetPlayerForUserID( event->GetInt("attacker") );
	int victim = engine->GetPlayerForUserID( event->GetInt("userid") );
	const char *killedwith = event->GetString("weapon");
	bool headshot = event->GetBool("headshot");

	// Get the names of the players
	const char *killer_name = GEPlayerRes()->GetPlayerName( killer );
	const char *victim_name = GEPlayerRes()->GetPlayerName( victim );

	if ( !killer_name )
		killer_name = "";
	if ( !victim_name )
		victim_name = "";

	// The m_DeathMsgInfo variable stores all the info we need to carry through
	// this process of displaying the notice. Including team information and 
	// whether the player is local for special colorization
	m_DeathMsgInfo.Killer.iEntIndex = killer;
	m_DeathMsgInfo.Victim.iEntIndex = victim;
	Q_strncpy( m_DeathMsgInfo.Killer.szName, killer_name, MAX_PLAYER_NAME_LENGTH );
	Q_strncpy( m_DeathMsgInfo.Victim.szName, victim_name, MAX_PLAYER_NAME_LENGTH );
	m_DeathMsgInfo.bSuicide = ( !killer || killer == victim );

	// Localize the weapon name!
	wchar_t *wszKilledWith = g_pVGuiLocalize->Find( killedwith );
	if ( wszKilledWith )
		g_pVGuiLocalize->ConvertUnicodeToANSI( wszKilledWith, m_DeathMsgInfo.killedWith, sizeof( m_DeathMsgInfo.killedWith ) );
	else
		Q_strncpy( m_DeathMsgInfo.killedWith, killedwith, MAX_PLAYER_NAME_LENGTH );

	char szConMsg[512];
	// Record the death notice in the console and record what localization string we should use
	if ( m_DeathMsgInfo.bSuicide )
	{
		if ( !strcmp( killedwith, "world" ) )
		{
			Q_snprintf( szConMsg, sizeof(szConMsg), "%s died.", m_DeathMsgInfo.Victim.szName );
			m_DeathMsgInfo.wszFormat = g_pVGuiLocalize->Find( "#GES_Death_Suicide_World" );
		}
		else
		{
			Q_snprintf( szConMsg, sizeof(szConMsg), "%s committed suicide.", m_DeathMsgInfo.Victim.szName );
			m_DeathMsgInfo.wszFormat = g_pVGuiLocalize->Find( "#GES_Death_Suicide" );
		}
	}
	else
	{
		GetEnglishArticle( m_DeathMsgInfo.killedWith, m_DeathMsgInfo.article );

		if ( headshot )
		{
			Q_snprintf( szConMsg, sizeof(szConMsg), "%s killed %s with %s%s Headshot.", m_DeathMsgInfo.Killer.szName, m_DeathMsgInfo.Victim.szName, m_DeathMsgInfo.article, m_DeathMsgInfo.killedWith );
			m_DeathMsgInfo.wszFormat = g_pVGuiLocalize->Find( "#GES_Death_Headshot" );
		}
		else if ( strcmp(m_DeathMsgInfo.killedWith, "world") == 0)
		{
			Q_snprintf( szConMsg, sizeof(szConMsg), "%s killed %s.", m_DeathMsgInfo.Killer.szName, m_DeathMsgInfo.Victim.szName );
			m_DeathMsgInfo.wszFormat = g_pVGuiLocalize->Find( "#GES_Death_Killed_World" );
		}
		else
		{
			Q_snprintf( szConMsg, sizeof(szConMsg), "%s killed %s with %s%s.", m_DeathMsgInfo.Killer.szName, m_DeathMsgInfo.Victim.szName, m_DeathMsgInfo.article, m_DeathMsgInfo.killedWith );
			m_DeathMsgInfo.wszFormat = g_pVGuiLocalize->Find( "#GES_Death_Killed" );
		}
	}

	// Print the death notice in the console
	GEUTIL_RemoveColorHints( szConMsg );
	Msg( "%s\n", szConMsg );

	// Print the death notice
	DeathPrintf();
}

void CGEHudDeathNotice::GetEnglishArticle( const char *word, char *article )
{
	wchar_t *wszPrep;
	// Make a temp copy to work with
	char szPrep[MAX_PLAYER_NAME_LENGTH];
	Q_strncpy( szPrep, word, MAX_PLAYER_NAME_LENGTH );
	// Cancel out any previous entries
	article[0] = '\0';

	// No preposition if we end in an s
	if ( szPrep[Q_strlen(szPrep)-1] == 's' )
		return;

	// Check for vowel
	char v = Q_strlower(szPrep)[0];
	if ( v == 'a' || v == 'e' || v == 'i' || v == 'u' || v == 'o' )
		wszPrep = g_pVGuiLocalize->Find( "#English_Article_An" );
	else
		wszPrep = g_pVGuiLocalize->Find( "#English_Article_A" );

	if ( wszPrep )
		g_pVGuiLocalize->ConvertUnicodeToANSI( wszPrep, article, sizeof(article) );
}

void CGEHudDeathNotice::DeathPrintf( void )
{
	wchar_t szBuf[256];

	if ( m_DeathMsgInfo.bSuicide )
	{
		wchar_t victim[MAX_PLAYER_NAME_LENGTH];
		g_pVGuiLocalize->ConvertANSIToUnicode( m_DeathMsgInfo.Victim.szName, victim, sizeof( victim ) );
		g_pVGuiLocalize->ConstructString( szBuf, sizeof( szBuf ), m_DeathMsgInfo.wszFormat, 1, victim );
	}
	else
	{
		wchar_t killer[MAX_PLAYER_NAME_LENGTH], victim[MAX_PLAYER_NAME_LENGTH], killedwith[MAX_PLAYER_NAME_LENGTH], article[4];
		g_pVGuiLocalize->ConvertANSIToUnicode( m_DeathMsgInfo.Killer.szName, killer, sizeof( killer ) );
		g_pVGuiLocalize->ConvertANSIToUnicode( m_DeathMsgInfo.Victim.szName, victim, sizeof( victim ) );
		g_pVGuiLocalize->ConvertANSIToUnicode( m_DeathMsgInfo.killedWith, killedwith, sizeof( killedwith ) );
		g_pVGuiLocalize->ConvertANSIToUnicode( m_DeathMsgInfo.article, article, sizeof( article ) );
		g_pVGuiLocalize->ConstructString( szBuf, sizeof( szBuf ), m_DeathMsgInfo.wszFormat, 4, killer, victim, article, killedwith );
	}

	char ansiString[512];
	g_pVGuiLocalize->ConvertUnicodeToANSI( ConvertCRtoNL( szBuf ), ansiString, sizeof( ansiString ) );

	// Print the death notice in the HUD
	ChatPrintf( 0, CHAT_FILTER_NONE, ansiString );
}


Color CGEHudDeathNotice::GetDefaultTextColor( void )
{
	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( vgui::scheme()->GetScheme( "ClientScheme" ) );
	CBasePlayer *pLocalPlayer = CBasePlayer::GetLocalPlayer();

	if ( !pScheme || !pLocalPlayer )
		return Color( 255, 255, 255, 255 );

	if ( m_DeathMsgInfo.bSuicide && m_DeathMsgInfo.Victim.iEntIndex == pLocalPlayer->entindex() )
		return pScheme->GetColor( "GEDeathLocal", Color( 255, 240, 158, 255 ) );
	else if ( m_DeathMsgInfo.bSuicide )
		return pScheme->GetColor( "GEDeathSuicide", Color( 255, 100, 100, 255 ) );
	else
		return pScheme->GetColor( "GEChatColor", Color( 255, 255, 255, 255 ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Color CGEHudDeathNotice::GetTextColorForClient( wchar_t colorNum, int clientIndex )
{
	if (colorNum == COLOR_SUICIDE)
		colorNum = L'6';

	if (colorNum == COLOR_KILLER)
		colorNum = L'7';

	if (colorNum == COLOR_VICTIM)
		colorNum = L'8';

	if (colorNum == COLOR_WEAPON)
		colorNum = L'9';


	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( vgui::scheme()->GetScheme( "ClientScheme" ) ); 

	if ( pScheme )
	{
		CBasePlayer *pLPlayer = CBasePlayer::GetLocalPlayer();

		if (colorNum == L'7')
		{
			if ( pLPlayer && pLPlayer->entindex() == m_DeathMsgInfo.Killer.iEntIndex )
				return pScheme->GetColor( "GEDeathLocal", Color( 255, 240, 158, 255 ) );

			else if ( GEMPRules()->IsTeamplay() )
				return GEPlayerRes()->GetTeamColor( GEPlayerRes()->GetTeam(m_DeathMsgInfo.Killer.iEntIndex) );

			else
				return pScheme->GetColor( "GEDeathKiller", GetDefaultTextColor() );
		}

		if (colorNum == L'8')
		{
			if ( pLPlayer && pLPlayer->entindex() == m_DeathMsgInfo.Victim.iEntIndex )
				return pScheme->GetColor( "GEDeathLocal", Color( 255, 240, 158, 255 ) );

			else if ( GEMPRules()->IsTeamplay() )
				return GEPlayerRes()->GetTeamColor( GEPlayerRes()->GetTeam(m_DeathMsgInfo.Victim.iEntIndex) );

			else
				return pScheme->GetColor( "GEDeathVictim", GetDefaultTextColor() );
		}
	}

	return BaseClass::GetTextColorForClient(colorNum, clientIndex);
}

int	CGEHudDeathNotice::GetChatInputOffset( void )
{
	return 0;
}
