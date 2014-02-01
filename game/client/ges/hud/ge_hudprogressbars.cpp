///////////// Copyright � 2009, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_hudgameplayinfo.cpp
// Description:
//      See Header
//
// Created On: 06 Jan 10
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"
#include "vgui_controls/AnimationController.h"
#include "c_ge_player.h"
#include "gemp_gamerules.h"
#include "ge_hudprogressbars.h"
#include "ge_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

DECLARE_HUDELEMENT( CHudProgressBars );
DECLARE_HUD_MESSAGE( CHudProgressBars, AddProgressBar );
DECLARE_HUD_MESSAGE( CHudProgressBars, RemoveProgressBar );
DECLARE_HUD_MESSAGE( CHudProgressBars, UpdateProgressBar );
DECLARE_HUD_MESSAGE( CHudProgressBars, ConfigProgressBar );

char *CleanupFloatText( char *str )
{
	if ( !str ) return NULL;

	char *out = str;
	for ( char *in = str; *in != 0; ++in )
	{
		if (*in == '.')
		{
			if (*(in+1) == '0' && *(in+2) == '0') // Make ".00" => ""
				in += 3;
		}

		*out = *in;
		++out;
	}
	*out = 0;

	return str;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudProgressBars::CHudProgressBars( const char *pElementName ) : 
	BaseClass(NULL, "GEHUDProgressBars"), CHudElement( pElementName )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_pHudAnims = g_pClientMode->GetViewportAnimationController();

	// Initialize the progress bars
	char barname[8];
	for ( int i=0; i < GE_HUD_NUMPROGRESSBARS; i++ )
	{
		m_vProgressBars[i] = new sProgressBar;

		Q_snprintf( barname, 8, "label%i", i );
		m_vProgressBars[i]->pTitle = new vgui::Label(this, barname, "");
		m_vProgressBars[i]->pTitle->SetVisible( false );

		Q_snprintf( barname, 8, "value%i", i );
		m_vProgressBars[i]->pValue = new vgui::Label(this, barname, "");
		m_vProgressBars[i]->pValue->SetVisible( false );

		Q_snprintf( barname, 8, "bar%i", i );
		m_vProgressBars[i]->pBar = new vgui::ContinuousProgressBar( this, barname );
		m_vProgressBars[i]->pBar->SetVisible( false );

		ResetProgressBar(i);
	}

	ListenForGameEvent( "game_newmap" );
	ListenForGameEvent( "round_end" );

	HOOK_HUD_MESSAGE( CHudProgressBars, AddProgressBar );
	HOOK_HUD_MESSAGE( CHudProgressBars, RemoveProgressBar );
	HOOK_HUD_MESSAGE( CHudProgressBars, UpdateProgressBar );
	HOOK_HUD_MESSAGE( CHudProgressBars, ConfigProgressBar );
}

CHudProgressBars::~CHudProgressBars()
{
	for ( int i=0; i < GE_HUD_NUMPROGRESSBARS; i++ )
	{
		m_vProgressBars[i]->pBar->DeletePanel();
		m_vProgressBars[i]->pTitle->DeletePanel();
		m_vProgressBars[i]->pValue->DeletePanel();

		delete m_vProgressBars[i];
		m_vProgressBars[i] = NULL;
	}
}

void CHudProgressBars::VidInit()
{
	SetSize( ScreenWidth(), ScreenHeight() );
	InvalidateLayout();
}

bool CHudProgressBars::ShouldDraw( void )
{
	// Never show during intermission
	if ( GEMPRules() && GEMPRules()->IsIntermission() )
		return false;

	CHudElement *blood_screen = gHUD.FindElement( "CHudBloodScreen" );
	if ( blood_screen->ShouldDraw() )
		return false;

	return CHudElement::ShouldDraw();
}

void CHudProgressBars::FireGameEvent( IGameEvent * event )
{
	// Reset all the progress bars on round end and map change
	for ( int i=0; i < GE_HUD_NUMPROGRESSBARS; i++ )
		ResetProgressBar(i);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudProgressBars::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	SetFgColor( Color(0,0,0,0) );
	SetBgColor( Color(0,0,0,0) );
	SetAlpha( 255 );
	SetPaintBackgroundType( 0 );

	for ( int i=0; i < GE_HUD_NUMPROGRESSBARS; i++ )
	{
		m_vProgressBars[i]->pTitle->SetFont( m_hFont );
		m_vProgressBars[i]->pValue->SetFont( m_hFont );
	}
}

void CHudProgressBars::MsgFunc_AddProgressBar( bf_read &msg )
{
	int id = msg.ReadByte();
	if ( id < 0 )
		return;

	id %= GE_HUD_NUMPROGRESSBARS;

	sProgressBar *bar = m_vProgressBars[id];
	ResetProgressBar( id );

	// Read the display flags and the max value
	msg.ReadBits( &(bar->flags), 3 );
	bar->maxValue = msg.ReadFloat();
	bar->currValue = msg.ReadFloat();

	// X and Y come in as 0 -> 100 to save bandwidth then we convert them to floats 0 -> 1.0
	bar->x = msg.ReadChar() / 100.0f;
	bar->y = msg.ReadChar() / 100.0f;

	// Width and Height come in as 0 -> 640 and 0 -> 480 respectively
	bar->w = msg.ReadShort();
	bar->h = msg.ReadShort();

	int iColor;
	msg.ReadBits( &iColor, 32 );

	Color fgColor, bgColor;
	fgColor.SetRawColor( iColor );
	bgColor.SetColor( fgColor[0], fgColor[1], fgColor[2], 50 );

	// Setup the title text
	char title[64];
	msg.ReadString( title, 64 );

	wchar_t wszTitle[64];
	GEUTIL_ParseLocalization( wszTitle, 64, title );
	bar->pTitle->SetText( wszTitle );
	bar->pTitle->SetFgColor( fgColor );
	bar->pTitle->SetVisible( true );

	// Setup the progress bar
	bar->pBar->SetFgColor( fgColor );
	bar->pBar->SetBgColor( bgColor );

	// Setup the value text
	Q_snprintf( title, 64, " %0.2f / %0.2f", bar->currValue, bar->maxValue );
	CleanupFloatText( title );
	bar->pValue->SetText( title );
	bar->pValue->SetFgColor( fgColor );

	bar->visible = true;

	LayoutProgressBar( id );
}

void CHudProgressBars::MsgFunc_RemoveProgressBar( bf_read &msg )
{
	int id = msg.ReadByte();
	
	// Any number < 0 will result in all bars being reset
	if ( id < 0 )
	{
		for ( int i=0; i < GE_HUD_NUMPROGRESSBARS; i++ )
			ResetProgressBar(i);
	}
	else
	{
		id %= GE_HUD_NUMPROGRESSBARS;
		ResetProgressBar( id );
	}
}

void CHudProgressBars::MsgFunc_UpdateProgressBar( bf_read &msg )
{
	int id = msg.ReadByte();
	if ( id < 0 )
		return;

	id %= GE_HUD_NUMPROGRESSBARS;

	sProgressBar *bar = m_vProgressBars[id];
	bar->currValue = msg.ReadFloat();

	// Set the progress indicator
	if ( bar->flags & 0x1 )
		bar->pBar->SetProgress( bar->currValue / bar->maxValue );

	if ( bar->flags & 0x2 )
	{
		char value[64];
		Q_snprintf( value, 64, " %0.2f / %0.2f", bar->currValue, bar->maxValue );
		CleanupFloatText( value );

		bar->pValue->SetText( value );
		bar->pValue->SizeToContents();
	}
}

void CHudProgressBars::MsgFunc_ConfigProgressBar( bf_read &msg )
{
	int id = msg.ReadByte();
	if ( id < 0 )
		return;
	id %= GE_HUD_NUMPROGRESSBARS;

	sProgressBar *bar = m_vProgressBars[id];
	int orig_width = bar->pTitle->GetWide();

	// Setup the title text
	char title[64];
	msg.ReadString( title, 64 );

	// Check string update, '\r' indicates do not update
	if ( title[0] != '\r' )
	{
		wchar_t wszTitle[64];
		GEUTIL_ParseLocalization( wszTitle, 64, title );
		bar->pTitle->SetText( wszTitle );
		bar->pTitle->SizeToContents();

		if ( abs( orig_width - bar->pTitle->GetWide() ) > 5 )
			InvalidateLayout();
	}

	// Check if we sent a color update
	int iColor;
	msg.ReadBits( &iColor, 32 );

	if ( iColor != 0 )
	{
		Color fgColor, bgColor;
		fgColor.SetRawColor( iColor );
		bgColor.SetColor( fgColor[0], fgColor[1], fgColor[2], 50 );

		// Set Title color
		bar->pTitle->SetFgColor( fgColor );

		// Set Bar color
		bar->pBar->SetFgColor( fgColor );
		bar->pBar->SetBgColor( bgColor );

		// Setup Value color
		bar->pValue->SetFgColor( fgColor );
	}
}

void CHudProgressBars::PerformLayout( void )
{
	sProgressBar *pProBar = NULL;
	for ( int i=0; i < GE_HUD_NUMPROGRESSBARS; i++ )
	{
		pProBar = m_vProgressBars[i];
		if ( !pProBar )
			continue;

		if ( !pProBar->visible )
		{
			ResetProgressBar( i );
			continue;
		}

		LayoutProgressBar(i);
	}
}

void CHudProgressBars::LayoutProgressBar( int id )
{
	if ( id < 0 || id >= GE_HUD_NUMPROGRESSBARS )
		return;
	
	sProgressBar *pProBar = m_vProgressBars[id];
	int w, h;
	float x, y;

	w = XRES( pProBar->w );
	h = YRES( pProBar->h );

	x = pProBar->x;
	y = pProBar->y;

	// Make sure our labels are the correct size
	pProBar->pTitle->SizeToContents();
	pProBar->pValue->SizeToContents();
	
	// Declare offsets to be used later
	int barXOffset = 0,
		barYOffset = 0,
		valueXOffset = 0,
		valueYOffset = 0;

	// Set our direction
	if ( pProBar->flags & 0x4 )
	{
		// We are vertical, layout TITLE over BAR over TEXT
		pProBar->pBar->SetProgressDirection( vgui::ProgressBar::PROGRESS_NORTH );

		int totH = pProBar->pTitle->GetTall();
		if ( pProBar->flags & 0x1 )
			totH += h;
		if ( pProBar->flags & 0x2 )
			totH += pProBar->pValue->GetTall();

		int midW = pProBar->pTitle->GetWide() / 2.0f;

		// Figure out our base position
		if ( x < 0 )
			x = XRES( 320 ) - midW;
		else
			x = XRES( 640 * x );

		if ( y < 0 )
			y = YRES( 240 ) - (totH / 2.0f);
		else
			y = YRES( 480 * y );

		int buffer = YRES(3);

		// This exclusion prevents the bar offsetting the value if it is hidden
		barXOffset = midW - w/2;
		barYOffset = pProBar->pTitle->GetTall();

		valueXOffset = midW - pProBar->pValue->GetWide()/2;

		// This prevents the bar offsetting the value if it is hidden
		if ( pProBar->flags & 0x1 )
			valueYOffset = barYOffset + h + buffer;
		else
			valueYOffset = barYOffset + buffer;
	}
	else
	{
		pProBar->pBar->SetProgressDirection( vgui::ProgressBar::PROGRESS_EAST );

		int totW = pProBar->pTitle->GetWide();
		if ( pProBar->flags & 0x1 )
			totW += w;
		if ( pProBar->flags & 0x2 )
			totW += pProBar->pValue->GetWide();

		float maxH = max( pProBar->pTitle->GetTall(), h );

		// Figure out our base position
		if ( x < 0 )
			x = XRES( 320 ) - (totW / 2.0f);
		else
			x = XRES( 640 * x );

		if ( y < 0 )
			y = YRES( 240 ) - (maxH / 2.0f);
		else
			y = YRES( 480 * y );

		int buffer = XRES(3);

		barXOffset = pProBar->pTitle->GetWide() + buffer;
		barYOffset = abs(h - pProBar->pTitle->GetTall()) / 2.0f;

		// Set the value position
		if ( pProBar->flags & 0x1 )
			valueXOffset = barXOffset + w + buffer;
		else
			valueXOffset = barXOffset + buffer;
	}

	// Position the title
	pProBar->pTitle->SetPos( x, y );

	// Show the bar if we want to and position it
	if ( pProBar->flags & 0x1 )
	{
		vgui::ContinuousProgressBar *pBar = pProBar->pBar;
		pBar->SetSize( w, h );
		pBar->SetPos( x + barXOffset, y + barYOffset );
		pBar->SetVisible( true );
	}

	// Show the text value if we want to and position it
	if ( pProBar->flags & 0x2 )
	{
		pProBar->pValue->SetPos( x + valueXOffset, y + valueYOffset );
		pProBar->pValue->SetVisible( true );
	}
}

void CHudProgressBars::ResetProgressBar( int id )
{
	if ( id < 0 || id >= GE_HUD_NUMPROGRESSBARS )
		return;

	m_vProgressBars[id]->visible = false;
	m_vProgressBars[id]->maxValue = 0;
	m_vProgressBars[id]->flags = 0;
	m_vProgressBars[id]->pTitle->SetText("");
	m_vProgressBars[id]->pTitle->SetVisible( false );
	m_vProgressBars[id]->pValue->SetText("");
	m_vProgressBars[id]->pValue->SetVisible( false );
	m_vProgressBars[id]->pBar->SetProgress( 0 );
	m_vProgressBars[id]->pBar->SetVisible( false );
}
