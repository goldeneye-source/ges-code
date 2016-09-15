///////////// Copyright © 2012, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_hudpopupmsg.cpp
// Description:
//      Python controlled popup message for gameplay information
//
// Created On: 08 Feb 2012
// Created By: Killermonkey
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "vgui/ILocalize.h"
#include "vgui/IVGui.h"
#include "vgui/ISurface.h"
#include "vgui_controls/AnimationController.h"
#include "vgui_controls/ImagePanel.h"
#include "vgui_controls/Label.h"
#include "ge_CreditsText.h"
#include "networkstringtable_clientdll.h"
#include "c_ge_player.h"
#include "c_ge_playerresource.h"
#include "gemp_gamerules.h"
#include "ge_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

class CHudPopupMessage : public Panel, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudPopupMessage, Panel );

public:
	CHudPopupMessage( const char *pElementName );
	~CHudPopupMessage();

	virtual void Reset();
	virtual void VidInit();
	virtual bool ShouldDraw();
	virtual void FireGameEvent( IGameEvent *event );

	virtual void PerformLayout();
	virtual bool RequestInfo( KeyValues *outputData );
	virtual bool SetInfo( KeyValues *inputData );

	void ResetHelp( void );

	bool IsShowing( void ) { return m_flWidthPercent >= 0.95f && m_iCurrMovement == MOVEMENT_NONE; };
	bool IsHidden( void ) { return m_flWidthPercent <= 0.05f && m_iCurrMovement == MOVEMENT_NONE; };

	// Receives message from server
	void MsgFunc_PopupMessage( bf_read &msg );

	enum {
		MOVEMENT_SHOWING,
		MOVEMENT_HIDING,
		MOVEMENT_NONE,
	};

protected:
	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void OnTick();

	bool ResolveMessage();

	void ShowNextMessage();
	void HideMessage( bool instant = false );

private:
	struct sMessage
	{
		sMessage( void ) {
			title[0] = '\0';
			img[0] = '\0';
			msg = ::INVALID_STRING_INDEX;
		};

		char	title[32];
		char	img[128];
		int		msg;
	};

	float m_flNextThink;
	float m_fCurrMsgHideTime;
	int m_iCurrMovement;

	int m_iOrigX;
	int m_iOrigY;

	// Storage of animation controller
	AnimationController *m_pHudAnims;

	ImagePanel	*m_pBackground;
	Label			*m_pTitle;
	GECreditsText	*m_pMessage;
	ImagePanel	*m_pImage;

	// Quick and dirty access
	sMessage	*m_pCurrHelp;

	// Message Queue
	CUtlVector<sMessage*> m_vMessages;

	// Configurable Parameters
	CPanelAnimationVarAliasType( int, m_iTextX, "text_xpad", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTextY, "text_ypad", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iImageSize, "image_size", "140", "proportional_int" );

	CPanelAnimationVar( HFont, m_hTitleFont, "title_font", "Default" );
	CPanelAnimationVar( HFont, m_hMessageFont, "msg_font", "Default" );
	CPanelAnimationVar( Color, m_TextColor, "fgcolor", "0 0 0 255" );
	CPanelAnimationVar( Color, m_BgColor, "bgcolor", "0 0 0 100" );

	// Define these to control specific parts of the HUD via a scripted animation
	CPanelAnimationVar( float, m_flWidthPercent, "WidthPercent", "0" );
};

DECLARE_HUDELEMENT( CHudPopupMessage );
DECLARE_HUD_MESSAGE( CHudPopupMessage, PopupMessage );

#define GE_MSG_HOLDTIME 5.0f
#define GPH_BG_LARGE "hud/GameplayHelpBg"
#define GPH_BG_SMALL "hud/GameplayHelpBg_Small"

// TODO: this needs to be renamed?
ConVar cl_ge_disablepopuphelp( "cl_ge_disablepopuphelp", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL );

CHudPopupMessage::CHudPopupMessage( const char *pElementName ) 
	: BaseClass(NULL, "GEHUDPopupHelp"), CHudElement( pElementName )
{
	SetParent( g_pClientMode->GetViewport() );
	
	SetScheme( "ClientScheme" );
	SetProportional( true );

	m_pHudAnims = g_pClientMode->GetViewportAnimationController();

	m_iCurrMovement = MOVEMENT_NONE;
	m_flWidthPercent = 0;
	m_flNextThink = 0;
	m_fCurrMsgHideTime = 0;
	m_pCurrHelp = NULL;

	m_pBackground = new ImagePanel( this, "Background" );
	m_pBackground->SetShouldScaleImage( true );
	m_pBackground->SetPaintBorderEnabled( false );

	m_pTitle = new Label( m_pBackground, "Title", "" );

	m_pMessage = new GECreditsText( m_pBackground, "Message" );
	m_pMessage->SetVerticalScrollbar( false );
	m_pMessage->SetText("");

	m_pImage = new ImagePanel( m_pBackground, "Image" );
	m_pImage->SetImage( "" );
	m_pImage->SetShouldScaleImage( true );

	HOOK_HUD_MESSAGE( CHudPopupMessage, PopupMessage );

	ListenForGameEvent( "game_newmap" );
	ListenForGameEvent( "round_end" );

	ivgui()->AddTickSignal( GetVPanel(), 30 );
}

CHudPopupMessage::~CHudPopupMessage()
{
	m_vMessages.PurgeAndDeleteElements();
}

//-----------------------------------------------------------------------------
// Purpose: Reset the HUD element on map change
//-----------------------------------------------------------------------------
void CHudPopupMessage::Reset()
{
	m_flNextThink = 0;

	if ( m_iCurrMovement != MOVEMENT_NONE )
	{
		if ( m_iCurrMovement == MOVEMENT_SHOWING )
			m_flWidthPercent = 1.0f;
		else
			m_flWidthPercent = 0;
	}
}

void CHudPopupMessage::VidInit()
{
	InvalidateLayout();
	Reset();
}

bool CHudPopupMessage::ShouldDraw()
{
	if ( m_flWidthPercent < 0.025f )
		return false;

	return CHudElement::ShouldDraw();
}

void CHudPopupMessage::FireGameEvent( IGameEvent *event )
{
	// Reset help when we change maps, end round, or change scenario
	ResetHelp();
}

bool CHudPopupMessage::RequestInfo( KeyValues *outputData )
{
	if ( !Q_stricmp(outputData->GetName(), "WidthPercent") )
	{
		outputData->SetFloat("WidthPercent", m_flWidthPercent);
		return true;
	}
	
	return BaseClass::RequestInfo(outputData);
}

bool CHudPopupMessage::SetInfo( KeyValues *inputData )
{
	if ( !Q_stricmp(inputData->GetName(), "WidthPercent") )
	{
		m_flWidthPercent = inputData->GetFloat("WidthPercent");
		return true;
	}

	return BaseClass::SetInfo(inputData);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudPopupMessage::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetFgColor( Color(0,0,0,0) );
	SetBgColor( Color(0,0,0,0) );

	m_pBackground->SetImage( "hud/GameplayHelpBg" );
	m_pBackground->SetBgColor( m_BgColor );
	m_pBackground->SetFgColor( GetFgColor() );
	
	m_pTitle->SetFgColor( m_TextColor );
	m_pTitle->SetFont( m_hTitleFont );
	m_pTitle->SetContentAlignment( Label::a_center );

	m_pMessage->SetFgColor( m_TextColor );
	m_pMessage->SetFont( m_hMessageFont );
	m_pMessage->SetWide( GetWide() );

	GetPos( m_iOrigX, m_iOrigY );

	SetZPos( 3 );
	SetAlpha( 255 );
	SetPaintBackgroundType( 0 );
}

//-----------------------------------------------------------------------------
// Purpose: Resizes the label
//-----------------------------------------------------------------------------
void CHudPopupMessage::PerformLayout()
{
	BaseClass::PerformLayout();

	int wide = GetWide(), 
		x = m_iTextX, 
		y = m_iTextY;

	m_pBackground->SetPos( 0, 0 );
	m_pBackground->SetWide( wide );

	// Inner width accounting for borders
	int innerW = wide - 2*x;

	// Position the title
	m_pTitle->SetPos( x, y );
	m_pTitle->SizeToContents();
	m_pTitle->SetWide( innerW );

	y += m_pTitle->GetTall() + m_iTextY;

	// Position the image
	if ( Q_stricmp(m_pImage->GetImageName(), "") )
	{
		int imgW, imgH;
		m_pImage->GetImage()->GetContentSize( imgW, imgH );
		float imgRat = (float)imgH / (float)imgW;

		imgW = min( m_iImageSize, innerW );
		imgH = imgRat * imgW;

		int iOffset = (innerW / 2) - (imgW / 2);

		m_pImage->SetVisible( true );
		m_pImage->SetPos( x + iOffset, y );
		m_pImage->SetSize( imgW, imgH );

		y += imgH + m_iTextY;
	}
	else
	{
		m_pImage->SetVisible( false );
	}

	// Position the message
	m_pMessage->SetPos( x - m_iTextX, y );
	m_pMessage->SetWide( innerW );
	m_pMessage->SetToFullHeight();
	m_pMessage->SetTall( m_pMessage->GetTall() + m_iTextY*2 );
	m_pMessage->GotoTextStart();

	int totalHeight = y + m_pMessage->GetTall() + m_iTextY + m_iTextY*2;

	m_pBackground->SetTall( totalHeight );
	SetTall( totalHeight );

	if ( totalHeight > 128 )
		m_pBackground->SetImage( GPH_BG_LARGE );
	else
		m_pBackground->SetImage( GPH_BG_SMALL );
}

bool CHudPopupMessage::ResolveMessage( void )
{
	if ( !m_pCurrHelp || !g_pStringTableGameplay )
		return false;

	m_pMessage->SetText("");
	m_pMessage->InsertCenterText( true );
	m_pMessage->InsertFontChange( m_hMessageFont );

	if ( m_pCurrHelp->msg == ::INVALID_STRING_INDEX )
		return true;

	const char *pMessage = g_pStringTableGameplay->GetString( m_pCurrHelp->msg );
	if ( pMessage )
	{
		wchar_t wszMsg[512];
		GEUTIL_ParseLocalization( wszMsg, 512, pMessage );

		m_pMessage->InsertString( wszMsg );
		m_pMessage->ForceRecalculateLineBreaks();
		return true;
	}

	return false;
}

void CHudPopupMessage::OnTick( void )
{
	// Check if the current message is done
	if ( IsShowing() && gpGlobals->curtime > m_fCurrMsgHideTime )
		HideMessage();

	// Show the next message (if we have one)
	if ( IsHidden() )
		ShowNextMessage();

	// Set our position based on our animation variable
	m_flWidthPercent = clamp( m_flWidthPercent, 0, 1.0f );
	int offset = GetWide() * (1.0f - m_flWidthPercent);
	SetPos( m_iOrigX - offset, m_iOrigY );

	// Hide us and ensure our state is set properly
	if ( m_flWidthPercent < 0.01f && m_iCurrMovement == MOVEMENT_HIDING )
	{
		m_iCurrMovement = MOVEMENT_NONE;
	}
	else if ( m_flWidthPercent == 1.0f && m_iCurrMovement == MOVEMENT_SHOWING )
	{
		m_iCurrMovement = MOVEMENT_NONE;
	}
}

void CHudPopupMessage::ShowNextMessage( void )
{
	// Get rid of our current one
	if ( m_pCurrHelp )
	{
		delete m_pCurrHelp;
		m_pCurrHelp = NULL;
	}
	
	// Nothing to do if we don't have any messages
	if ( m_vMessages.Count() == 0 )
		return;

	// Extract the message from our queue
	m_pCurrHelp = m_vMessages[0];
	m_vMessages.Remove(0);

	// TODO: this might be an issue in a lagged environment
	// should we give a delay between messages anyway??
	ResolveMessage();

	// Notify the player with a sound
	if ( C_BasePlayer::GetLocalPlayer() )
		C_BasePlayer::GetLocalPlayer()->EmitSound( "GEPlayer.HudNotify" );

	m_pTitle->SetText( m_pCurrHelp->title );
	m_pImage->SetImage( m_pCurrHelp->img );

	InvalidateLayout( true );
	SetVisible( true );

	m_fCurrMsgHideTime = gpGlobals->curtime + GE_MSG_HOLDTIME + m_pHudAnims->GetAnimationSequenceLength( "PopupHelpShow" );
	
	m_pHudAnims->StartAnimationSequence( "PopupHelpShow" );
	m_iCurrMovement = MOVEMENT_SHOWING;
}

void CHudPopupMessage::HideMessage( bool instant /*= false*/ )
{
	if ( instant )
		m_flWidthPercent = 0;
	else
		m_pHudAnims->StartAnimationSequence( "PopupHelpHide" );

	m_iCurrMovement = MOVEMENT_HIDING;
}

void CHudPopupMessage::ResetHelp( void )
{
	HideMessage(true);

	m_vMessages.PurgeAndDeleteElements();
	m_pCurrHelp = NULL;
}

void CHudPopupMessage::MsgFunc_PopupMessage( bf_read &msg )
{
	// Create a new item in the queue
	sMessage *help = new sMessage;

	// Read in our data
	char tempImg[64];
	msg.ReadString( help->title, 32 );
	msg.ReadString( tempImg, 64 );
	help->msg = msg.ReadShort();

	if ( tempImg[0] )
		Q_snprintf( help->img, 128, "hud/gameplayhelp/%s", tempImg );
	else
		help->img[0] = '\0';

	// Add us to the end of our queue
	m_vMessages.AddToTail( help );
}
