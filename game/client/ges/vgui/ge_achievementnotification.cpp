//========= Copyright � 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ImagePanel.h>
#include "ge_achievementnotification.h"
#include "steam/steam_api.h"
#include "iachievementmgr.h"
#include "fmtstr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define ACHIEVEMENT_NOTIFICATION_DURATION 9.0f

extern ConVar cl_ge_disablecuemusic;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

DECLARE_HUDELEMENT_DEPTH( CAchievementNotificationPanel, 100 );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAchievementNotificationPanel::CAchievementNotificationPanel( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "AchievementNotificationPanel" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	LoadControlSettings( "resource/UI/AchievementNotification.res" );

	m_flHideTime = 0;
	m_pPanelBackground = (EditablePanel*) FindChildByName( "Notification_Background" );
	m_pIcon = (ImagePanel*) FindChildByName( "Notification_Icon" );
	m_pLabelHeading = (Label*) FindChildByName( "HeadingLabel" );
	m_pLabelTitle = (Label*) FindChildByName( "TitleLabel" );

	m_pIcon->SetShouldScaleImage( true );

	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAchievementNotificationPanel::Init()
{
	ListenForGameEvent( "achievement_event" );
	ListenForGameEvent( "achievement_unlocked" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAchievementNotificationPanel::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
//	LoadControlSettings( "resource/UI/AchievementNotification.res" );
	
	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAchievementNotificationPanel::PerformLayout( void )
{
	BaseClass::PerformLayout();

	IScheme *pScheme = scheme()->GetIScheme( GetScheme() );

	// Set background color of various elements.  Need to do this in code, if we do it in res file it gets slammed by the
	// scheme.  (Incl. label background: some products don't have label background colors set in their scheme and helpfully slam it to white.)
	SetBgColor( Color( 0, 0, 0, 0 ) );
	m_pLabelHeading->SetBgColor( Color( 0, 0, 0, 0 ) );
	m_pLabelTitle->SetBgColor( Color( 0, 0, 0, 0 ) );
	m_pPanelBackground->SetBgColor( pScheme->GetColor( "FolderTan_Trans", Color(62,70,55, 200) ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAchievementNotificationPanel::FireGameEvent( IGameEvent * event )
{
	const char *name = event->GetName();
	if ( 0 == Q_strcmp( name, "achievement_event" ) )
	{
		const char *pchName = event->GetString( "achievement_name" );
		int iCur = event->GetInt( "cur_val" );
		int iMax = event->GetInt( "max_val" );
		wchar_t szLocalizedName[256]=L"";

		const wchar_t *pchLocalizedName = ACHIEVEMENT_LOCALIZED_NAME_FROM_STR( pchName );
		Assert( pchLocalizedName );
		if ( !pchLocalizedName || !pchLocalizedName[0] )
			return;
		Q_wcsncpy( szLocalizedName, pchLocalizedName, sizeof( szLocalizedName ) );

		// this is achievement progress, compose the message of form: "<name> (<#>/<max>)"
		wchar_t szFmt[128]=L"";
		wchar_t szText[512]=L"";
		wchar_t szNumFound[16]=L"";
		wchar_t szNumTotal[16]=L"";
		_snwprintf( szNumFound, ARRAYSIZE( szNumFound ), L"%i", iCur );
		_snwprintf( szNumTotal, ARRAYSIZE( szNumTotal ), L"%i", iMax );

		const wchar_t *pchFmt = g_pVGuiLocalize->Find( "#GameUI_Achievement_Progress_Fmt" );
		if ( !pchFmt || !pchFmt[0] )
			return;
		Q_wcsncpy( szFmt, pchFmt, sizeof( szFmt ) );

		g_pVGuiLocalize->ConstructString( szText, sizeof( szText ), szFmt, 3, szLocalizedName, szNumFound, szNumTotal );

		if ( iCur == iMax )
		{
			AddNotification( pchName, g_pVGuiLocalize->Find( "#GameUI_Achievement_Awarded" ), szText );
			PlayQueMusic( GE_ACHIEVEMENT_EARNED );
		}
		else
		{
			AddNotification( pchName, g_pVGuiLocalize->Find( "#GameUI_Achievement_Progress" ), szText );
			PlayQueMusic( GE_ACHIEVEMENT_UNLOCKED );
		}
	}
	else if ( Q_strcmp( name, "achievement_unlocked" ) == 0 )
	{
		const char *pchName = event->GetString( "achievement_name" );

		wchar_t szLocalizedName[256]=L"";
		const wchar_t *pchLocalizedName = ACHIEVEMENT_LOCALIZED_NAME_FROM_STR( pchName );
		Assert( pchLocalizedName );
		if ( !pchLocalizedName || !pchLocalizedName[0] )
			return;
		Q_wcsncpy( szLocalizedName, pchLocalizedName, sizeof( szLocalizedName ) );

		AddNotification( pchName, g_pVGuiLocalize->Find( "#GameUI_Achievement_Unlocked" ), szLocalizedName );
		PlayQueMusic( GE_ACHIEVEMENT_UNLOCKED );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called on each tick
//-----------------------------------------------------------------------------
void CAchievementNotificationPanel::OnTick( void )
{
	if ( ( m_flHideTime > 0 ) && ( m_flHideTime < gpGlobals->curtime ) )
	{
		m_flHideTime = 0;
		ShowNextNotification();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CAchievementNotificationPanel::ShouldDraw( void )
{
	return ( ( m_flHideTime > 0 ) && ( m_flHideTime > gpGlobals->curtime ) && CHudElement::ShouldDraw() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAchievementNotificationPanel::AddNotification( const char *szIconBaseName, const wchar_t *pHeading, const wchar_t *pTitle )
{
	// put this notification in our queue
	int iQueueItem = m_queueNotification.AddToTail();
	Notification_t &notification = m_queueNotification[iQueueItem];
	Q_strncpy( notification.szIconBaseName, szIconBaseName, ARRAYSIZE( notification.szIconBaseName ) );
	Q_wcsncpy( notification.szHeading, pHeading, sizeof( notification.szHeading ) );
	Q_wcsncpy( notification.szTitle, pTitle, sizeof( notification.szTitle ) );

	// if we are not currently displaying a notification, go ahead and show this one
	if ( 0 == m_flHideTime )
	{
		ShowNextNotification();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Shows next notification in queue if there is one
//-----------------------------------------------------------------------------
void CAchievementNotificationPanel::ShowNextNotification()
{
	// see if we have anything to do
	if ( 0 == m_queueNotification.Count() )
	{
		m_flHideTime = 0;
		return;
	}

	Notification_t &notification = m_queueNotification[ m_queueNotification.Head() ];

	m_flHideTime = gpGlobals->curtime + ACHIEVEMENT_NOTIFICATION_DURATION;

	// set the text and icon in the dialog
	SetDialogVariable( "heading", notification.szHeading );
	SetDialogVariable( "title", notification.szTitle );
	const char *pchIconBaseName = notification.szIconBaseName;
	if ( pchIconBaseName && pchIconBaseName[0] )
	{
		m_pIcon->SetImage( CFmtStr( "achievements/%s.vmt", pchIconBaseName ) );
	}

	// resize the panel so it always looks good

	// get fonts
	HFont hFontHeading = m_pLabelHeading->GetFont();
	HFont hFontTitle = m_pLabelTitle->GetFont();
	// determine how wide the text strings are
	int iHeadingWidth = UTIL_ComputeStringWidth( hFontHeading, notification.szHeading );
	int iTitleWidth = UTIL_ComputeStringWidth( hFontTitle, notification.szTitle );
	// use the widest string
	int iTextWidth = max( iHeadingWidth, iTitleWidth );
	// don't let it be insanely wide
	iTextWidth = min( iTextWidth, XRES( 300 ) );
	int iIconWidth = m_pIcon->GetWide();
	int iSpacing = XRES( 5 );
	int iPanelWidth = iSpacing + iIconWidth + iSpacing + iTextWidth + iSpacing;
	int iPanelX, iPanelY;
	m_pPanelBackground->GetPos( iPanelX, iPanelY );
	int iIconX = iPanelX + iSpacing;
	int iTextX = iIconX + iIconWidth + iSpacing;
	// resize all the elements
	SetXAndWide( m_pPanelBackground, iPanelX, iPanelWidth );
	SetXAndWide( m_pIcon, iIconX, iIconWidth );
	SetXAndWide( m_pLabelHeading, iTextX, iTextWidth );
	SetXAndWide( m_pLabelTitle, iTextX, iTextWidth );

	m_queueNotification.Remove( m_queueNotification.Head() );
}

void CAchievementNotificationPanel::PlayQueMusic( int type )
{
	// Don't play the tunes if we disabled it
	if ( cl_ge_disablecuemusic.GetBool() )
		return;

	const char *soundfile = NULL;

	CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
	CSingleUserRecipientFilter user( pPlayer );

	if ( type == GE_ACHIEVEMENT_UNLOCKED )
	{
		soundfile = "GEAchievement.Unlocked";
		pPlayer->EmitSound( user, pPlayer->entindex(), soundfile );
	}
	else if ( type == GE_ACHIEVEMENT_EARNED )
	{
		soundfile = "GEAchievement.Earned";
		pPlayer->EmitSound( user, pPlayer->entindex(), soundfile );
	}
	else
		return;

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
// Purpose: 
//-----------------------------------------------------------------------------
void CAchievementNotificationPanel::SetXAndWide( Panel *pPanel, int x, int wide )
{
	int xCur, yCur;
	pPanel->GetPos( xCur, yCur );
	pPanel->SetPos( x, yCur );
	pPanel->SetWide( wide );
}

#ifdef _DEBUG
CON_COMMAND_F( achievement_notification_test, "Test the hud notification UI", 0 )
{
	static int iCount=0;

	IGameEvent *event;
	event = gameeventmanager->CreateEvent( "achievement_event" );
	if ( event )
	{
		event->SetString( "achievement_name", "GES_SHAKEN_LEVEL2" );
		event->SetInt( "cur_val", 1 );
		event->SetInt( "max_val", 1 );
		gameeventmanager->FireEventClientSide( event );
	}

	event = gameeventmanager->CreateEvent( "achievement_event" );
	if ( event )
	{
		event->SetString( "achievement_name", "GES_THE_SPECIALIST" );
		event->SetInt( "cur_val", 10 );
		event->SetInt( "max_val", 100 );
		gameeventmanager->FireEventClientSide( event );
	}

	event = gameeventmanager->CreateEvent( "achievement_unlocked" );
	if ( event )
	{
		event->SetString( "achievement_name", "GES_SHARPSHOOTAH" );
		gameeventmanager->FireEventClientSide( event );
	}

	iCount++;
}
#endif
