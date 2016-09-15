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
#include "hud_basechat.h"
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

ConVar cl_ge_killfeed_readskins("cl_ge_killfeed_readskins", "1", FCVAR_ARCHIVE | FCVAR_USERINFO, "Use unique names for special weapon skins.");
ConVar cl_ge_drawkillfeed("cl_ge_drawkillfeed", "1", FCVAR_ARCHIVE | FCVAR_USERINFO, "Draw the killfeed.");

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
	virtual bool ShouldDraw();
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
bool CGEHudDeathNotice::ShouldDraw(void)
{
	if (!cl_ge_drawkillfeed.GetBool())
		return false;

	return true;
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
	int killedwithid = event->GetInt("weaponid");
	int damagetype = event->GetInt("dmgtype");
	const char *killedwith = event->GetString("weapon");
	int killedwithskinID = event->GetInt("weaponskin");
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
	char killedwithskin[32];


	// If our weapon has a skin try and find a unique name for it if we want one.
	if (killedwithskinID && cl_ge_killfeed_readskins.GetBool())
		Q_snprintf(killedwithskin, sizeof(killedwithskin), "%s%d", killedwith, killedwithskinID);
	else
		Q_strncpy(killedwithskin, killedwith, MAX_PLAYER_NAME_LENGTH);

	

	wchar_t *wszKilledWith = g_pVGuiLocalize->Find(killedwithskin);

	if ( wszKilledWith )
		g_pVGuiLocalize->ConvertUnicodeToANSI( wszKilledWith, m_DeathMsgInfo.killedWith, sizeof( m_DeathMsgInfo.killedWith ) );
	else
		Q_strncpy( m_DeathMsgInfo.killedWith, killedwith, MAX_PLAYER_NAME_LENGTH );

	char szConMsg[512];
	// Record the death notice in the console and record what localization string we should use
	if ( m_DeathMsgInfo.bSuicide )
	{
		if (Q_strncmp(killedwith, "-TD-", 4) == 0) //Check custom death messages first
		{
			char szTriggerMessage[64];

			// Cut off the -TD- flag.  This is seriously the best and only way I found that works.
			Q_StrRight(killedwith, -4, szTriggerMessage, 64); // Copies over the rightmost part of the string, starting 4 places from the leftmost part of it.

			if (Q_strncmp(szTriggerMessage, "#", 1) == 0) //If the string starts with a pound sign it's probably localized.
			{
				if (!strcmp(szTriggerMessage, "#GES_Pit_Death")) //Just a quick addition for easy generic death pit messages.
					Q_snprintf( szTriggerMessage, sizeof(szTriggerMessage), "%s%d", szTriggerMessage, GERandom<int>(6) );

				m_DeathMsgInfo.wszFormat = g_pVGuiLocalize->Find(szTriggerMessage); //So look for it
				Q_snprintf(szConMsg, sizeof(szConMsg), "%s died to a trap.", m_DeathMsgInfo.Victim.szName);
			}
			else //The string should be displayed as-is.  If only this were easy.
			{
				//First print the console message because that part is straightforward at least.
				Q_snprintf(szConMsg, sizeof(szConMsg), "%s %s", m_DeathMsgInfo.Victim.szName, szTriggerMessage);
				
				char szDeathMsgUnf[128]; //ANSI version of the string
				char szStrPlc[4] = "%s"; //Placeholder string so we can replace the second %s without replacing the first.
				wchar_t szDeathMessageW[128]; //Temporary place to store the result so we can = to it instead of directly overwriting m_DeathMsgInfo.wszFormat because this crashes the game for some reason.

				Q_snprintf(szDeathMsgUnf, sizeof(szDeathMsgUnf), "^6%s1 %s", szStrPlc, szTriggerMessage); //Format the death message so the system knows how to parse it.
				g_pVGuiLocalize->ConvertANSIToUnicode(szDeathMsgUnf, szDeathMessageW, sizeof(szDeathMessageW)); //Store it in the placeholder string.

				m_DeathMsgInfo.wszFormat = szDeathMessageW; //Transfer it over to the actual death message.
				// Seriously, if someone knows a better way to do this please tell me.  I can't find any documentation on this stuff at all.
			}
		}
		else if (!strcmp(m_DeathMsgInfo.killedWith, "self")) // killbinds and a few other things can cause this and should be checked for first since the damagetype is often blast.
		{
			Q_snprintf(szConMsg, sizeof(szConMsg), "%s committed suicide.", m_DeathMsgInfo.Victim.szName);
			m_DeathMsgInfo.wszFormat = g_pVGuiLocalize->Find("#GES_Death_Suicide");
		}
		else if (damagetype & DMG_BLAST) // Try to get specific damagetype death messages, starting with explosives
		{
			Q_snprintf(szConMsg, sizeof(szConMsg), "%s blew up.", m_DeathMsgInfo.Victim.szName);
			m_DeathMsgInfo.wszFormat = g_pVGuiLocalize->Find("#GES_Death_Suicide_Explosive");
		}
		else if (damagetype & DMG_FALL) // Fall damage related deaths are also common
		{
			Q_snprintf(szConMsg, sizeof(szConMsg), "%s fell to their death.", m_DeathMsgInfo.Victim.szName);
			m_DeathMsgInfo.wszFormat = g_pVGuiLocalize->Find("#GES_Death_Suicide_Fall");
		}
		else // Otherwise use a generic message
		{
			Q_snprintf(szConMsg, sizeof(szConMsg), "%s died.", m_DeathMsgInfo.Victim.szName);
			m_DeathMsgInfo.wszFormat = g_pVGuiLocalize->Find("#GES_Death_Suicide_World");
		}
	}
	else if (killedwithid == WEAPON_NONE) // There is a killer, but they are only getting credit for a suicide.
	{
		if (!strcmp(m_DeathMsgInfo.killedWith, "self")) // First check for killbinds because they can use a few different damage types.
		{
			Q_snprintf(szConMsg, sizeof(szConMsg), "%s drove %s to suicide", m_DeathMsgInfo.Killer.szName, m_DeathMsgInfo.Victim.szName);
			m_DeathMsgInfo.wszFormat = g_pVGuiLocalize->Find("#GES_Death_Killed_Suicide");
		}
		else if(damagetype & DMG_BLAST) // Then try to get specific damagetype death messages first, starting with explosives since they are the most common.
		{
			Q_snprintf(szConMsg, sizeof(szConMsg), "%s got %s to blow themselves up", m_DeathMsgInfo.Killer.szName, m_DeathMsgInfo.Victim.szName);
			m_DeathMsgInfo.wszFormat = g_pVGuiLocalize->Find("#GES_Death_Killed_Explosive");
		}
		else if (damagetype & DMG_FALL) // Fall damage related deaths are also common
		{
			Q_snprintf(szConMsg, sizeof(szConMsg), "%s pushed %s to their death", m_DeathMsgInfo.Killer.szName, m_DeathMsgInfo.Victim.szName);
			m_DeathMsgInfo.wszFormat = g_pVGuiLocalize->Find("#GES_Death_Killed_Fall");
		}
		else // Everything else
		{
			Q_snprintf(szConMsg, sizeof(szConMsg), "%s helped %s find death.", m_DeathMsgInfo.Killer.szName, m_DeathMsgInfo.Victim.szName);
			m_DeathMsgInfo.wszFormat = g_pVGuiLocalize->Find("#GES_Death_Killed_World");
		}
	}
	else
	{
		if (killedwithid != WEAPON_TRAP) // Traps don't get articles on standard kill messages.
			GetEnglishArticle( m_DeathMsgInfo.killedWith, m_DeathMsgInfo.article );
		else
			Q_strncpy(m_DeathMsgInfo.article, "", MAX_PLAYER_NAME_LENGTH);

		// Check for custom trap kill message first.  These can also display when someone pushes someone else into a pit.
		if (Q_strncmp(killedwith, "-TD-", 4) == 0)
		{
			char szTriggerMessage[64];

			// Cut off the -TD- flag.  This is seriously the best and only way I found that works.
			Q_StrRight(killedwith, -4, szTriggerMessage, 64); // Copies over the rightmost part of the string, starting 4 places from the leftmost part of it.
			
			if (Q_strncmp(szTriggerMessage, "#", 1) == 0) //If the string starts with a pound sign it's probably localized.
			{
				if (!strcmp(szTriggerMessage, "#GES_Pit_Kill")) //Just a quick addition for easy generic death pit messages.
					Q_snprintf(szTriggerMessage, sizeof(szTriggerMessage), "%s%d", szTriggerMessage, GERandom<int>(6));
				
				m_DeathMsgInfo.wszFormat = g_pVGuiLocalize->Find(szTriggerMessage); //So look for it
			}
			else //The string should be displayed as-is.  If only this were easy.
			{
				char szDeathMsgUnf[128]; //ANSI version of the string
				char szTriggerMessageFix[64]; //Place to hold the substituted string.
				char szStrPlc[4] = "%s"; //Placeholder string so we can replace the second %s without replacing the first.
				wchar_t szDeathMessageW[128]; //Temporary place to store the result so we can = to it instead of directly overwriting m_DeathMsgInfo.wszFormat because this crashes the game for some reason.
		
				Q_StrSubst(szTriggerMessage, "player2", "^8%s2^1", szTriggerMessageFix, sizeof(szTriggerMessageFix), true); //Replace player2 with %s so we can actually add in their name.

				Q_snprintf(szDeathMsgUnf, sizeof(szDeathMsgUnf), "^7%s1^1 %s", szStrPlc, szTriggerMessageFix); //Format the death message so the system knows how to parse it.
				g_pVGuiLocalize->ConvertANSIToUnicode(szDeathMsgUnf, szDeathMessageW, sizeof(szDeathMessageW)); //Store it in the placeholder string.

				m_DeathMsgInfo.wszFormat = szDeathMessageW; //Transfer it over to the actual death message.
				// Seriously, if someone knows a better way to do this please tell me.  I can't find any documentation on this stuff at all.
			}
			Q_snprintf(szConMsg, sizeof(szConMsg), "%s killed %s with a trap.", m_DeathMsgInfo.Killer.szName, m_DeathMsgInfo.Victim.szName);
		}
		else if ( headshot )
		{
			if (killedwithid == WEAPON_ROCKET_LAUNCHER || killedwithid == WEAPON_GRENADE_LAUNCHER)
			{
				Q_snprintf(szConMsg, sizeof(szConMsg), "%s killed %s with %s%s Direct Hit.", m_DeathMsgInfo.Killer.szName, m_DeathMsgInfo.Victim.szName, m_DeathMsgInfo.article, m_DeathMsgInfo.killedWith);
				m_DeathMsgInfo.wszFormat = g_pVGuiLocalize->Find("#GES_Death_Direct");
			}
			else
			{
				Q_snprintf(szConMsg, sizeof(szConMsg), "%s killed %s with %s%s Headshot.", m_DeathMsgInfo.Killer.szName, m_DeathMsgInfo.Victim.szName, m_DeathMsgInfo.article, m_DeathMsgInfo.killedWith);
				m_DeathMsgInfo.wszFormat = g_pVGuiLocalize->Find("#GES_Death_Headshot");
			}
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
