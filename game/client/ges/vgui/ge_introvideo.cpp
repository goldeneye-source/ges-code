///////////// Copyright � 2010 LodleNet. All rights reserved. /////////////
//
//   Project     : Client
//   File        : ge_introvideo.cpp
//   Description :
//      [TODO: Write the purpose of ge_introvideo.cpp.]
//
//   Created On: 2/27/2010 4:03:57 PM
//   Created By: Mark Chandler <mailto:mark@moddb.com>
////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "cdll_int.h"
#include "ienginevgui.h"
#include "iclientmode.h"
#include "vgui/IVGui.h"
#include "vgui/IInput.h"
#include "vgui/ISurface.h"
#include "vgui/Cursor.h"
#include "vgui_controls/Controls.h"
#include "vgui_video.h"

#include "ge_gamerules.h"
#include "ge_introvideo.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar ge_skipintro( "cl_ge_skipintro", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );
ConVar ge_skipintrobg( "cl_ge_skipintrobg", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );

#define BACKGROUNDVID_TIMEOUT 90.0f

static GameUI<CGEIntroVideoPanel> g_GEIntroVideo;

CGEIntroVideoPanel::CGEIntroVideoPanel( vgui::VPANEL parent ) : BaseClass( NULL, "GESIntroVideo" )
{
	SetParent( parent );

	// load the new scheme early!!
	SetScheme(vgui::scheme()->LoadSchemeFromFile("resource/SourceScheme.res", "SourceScheme"));
	SetProportional( true );

	LoadControlSettings("resource/GESIntroVideo.res");
	SetSize( ScreenWidth(), ScreenHeight() );

	InvalidateLayout();

	DisableFadeEffect();
	SetSizeable( false );
	SetMoveable( false );
	SetCloseButtonVisible(false);
	SetTitleBarVisible(false);
	SetPaintBackgroundEnabled(true);
	SetPaintBorderEnabled(false);

	SetMouseInputEnabled(true);
	SetKeyBoardInputEnabled(true);

	m_fLastMouseMove = -10.0f;
	m_iMouseX = m_iMouseY = 0;
	SetCursor( vgui::dc_blank );
	m_bIsCursorShown = false;
	m_bInBackground = false;
	
	m_pMaterial = NULL;

	DisableBackgroundPlaytime();
	
	m_pButStop = (vgui::Button*)FindChildByName("StopButton");
	m_pButSkip = (vgui::Button*)FindChildByName("SkipButton");
	m_pButReplay = (vgui::Button*)FindChildByName("ReplayButton");
	m_pAlwaysSkip = (vgui::CheckButton*)FindChildByName("Opt_SkipIntro");
	m_pBGSkip = (vgui::CheckButton*)FindChildByName("Opt_SkipBackground");

	// Add tick signal to allow for background movies to play
	vgui::ivgui()->AddTickSignal( GetVPanel(), 1000 );

	m_bIsPostInit = false;
}

CGEIntroVideoPanel::~CGEIntroVideoPanel()
{
	stopPlayback();
}

void CGEIntroVideoPanel::PostInit( void )
{
	m_bIsPostInit = true;

	// Show us if we want to
	if ( !ge_skipintro.GetBool() && !IsInGame() )
		engine->ClientCmd( "showintro" );
	else
		StartMenuMusic();

	ResetBackgroundPlaytime();
}

void CGEIntroVideoPanel::PaintBackground()
{
	Color c(0,0,0,55);

	int wide, tall;
	GetSize( wide, tall );
	vgui::surface()->DrawSetColor(c);
	vgui::surface()->DrawFilledRect(0, 0, wide, tall);
}

void CGEIntroVideoPanel::SetVisible( bool bShow )
{
	// If we are already showing ignore this
	if ( (IsVisible() == bShow && bShow) || !m_bIsPostInit )
		return;

	if ( bShow )
	{
		m_pAlwaysSkip->SetSelected( ge_skipintro.GetBool() );
		m_pBGSkip->SetSelected( ge_skipintrobg.GetBool() );

		StopMenuMusic();
		LoadMovieList();
		beginPlayback();
		SetZPos(1);
		MoveToFront();
		SetKeyBoardInputEnabled( true );
		RequestFocus();

		DisableBackgroundPlaytime();
	}
	else
	{
		stopPlayback();
		ResetBackgroundPlaytime();
		StartMenuMusic();
		SetKeyBoardInputEnabled( false );

		ge_skipintro.SetValue( m_pAlwaysSkip->IsSelected() );
		ge_skipintrobg.SetValue( m_pBGSkip->IsSelected() );
	}

	BaseClass::SetVisible( bShow );
}

bool CGEIntroVideoPanel::beginPlayback( const char *pFilename /*= NULL*/ )
{
	stopPlayback();

	const char *pFn = pFilename;
	if ( !pFn )
	{
		if ( ++m_iCurrMovieIdx >= m_vMovies.Count() )
			return false;

		pFn = m_vMovies[m_iCurrMovieIdx];
	}

	// TODO: Use VideoPanel
	return true;
}

void CGEIntroVideoPanel::stopPlayback( void )
{
	// TODO: Use VideoPanel
}

void CGEIntroVideoPanel::LoadMovieList( void )
{
	m_vMovies.PurgeAndDeleteElements();
	m_iCurrMovieIdx = -1;

	KeyValues *pKV;
	if ( m_bInBackground )
		pKV = ReadEncryptedKVFile( filesystem, "media/BackgroundVids", NULL );
	else
		pKV = ReadEncryptedKVFile( filesystem, "media/IntroVids", NULL );

	// No definition, no movies!
	if ( !pKV )
		return;

	char *filename;
	KeyValues *pKey = pKV->GetFirstSubKey();
	while( pKey )
	{
		if ( !Q_strcasecmp( pKey->GetName(), "file" ) )
		{
			filename = new char[128];
			Q_strncpy( filename, pKey->GetString(), 128 );
			Q_FixSlashes( filename );
			m_vMovies.AddToTail( filename );
		}

		pKey = pKey->GetNextKey();
	}
}

void CGEIntroVideoPanel::Activate( void )
{
	MoveToFront();
	RequestFocus();
	SetVisible( true );
	SetEnabled( true );

	vgui::surface()->SetMinimized( GetVPanel(), false );
}

void CGEIntroVideoPanel::OnCursorMoved(int x,int y)
{
	m_fLastMouseMove = gpGlobals->curtime;

	if ( !m_bIsCursorShown )
	{
		SetCursor( vgui::dc_arrow );
		m_bIsCursorShown = true;

		m_pButStop->SetVisible(true);
		m_pButSkip->SetVisible(true);
		m_pButReplay->SetVisible(true);
		m_pAlwaysSkip->SetVisible(true);
		m_pBGSkip->SetVisible(true);
	}

	BaseClass::OnCursorMoved(x,y);
}

void CGEIntroVideoPanel::OnKeyCodePressed(vgui::KeyCode code )
{
	// These keys cause the panel to shutdown
	switch ( code )
	{
	case KEY_ESCAPE:
	case KEY_BACKQUOTE:
	case KEY_SPACE:
	case KEY_ENTER:
		OnClose();
		break;

	default:
		BaseClass::OnKeyCodePressed( code );
	}
}

void CGEIntroVideoPanel::OnClose( void )
{
	StartMenuMusic();

	if ( vgui::input()->GetAppModalSurface() == GetVPanel() )
		vgui::input()->ReleaseAppModalSurface();

	vgui::surface()->RestrictPaintToSinglePanel( NULL );

	BaseClass::OnClose();
}

void CGEIntroVideoPanel::OnCommand(const char *command)
{
	if (stricmp(command, "skip") == 0)
	{
		if ( !beginPlayback() )
			OnClose();
	}
	else if (stricmp(command, "replay") == 0)
	{
		m_iCurrMovieIdx = clamp( m_iCurrMovieIdx, 0, m_vMovies.Count() );
		beginPlayback( m_vMovies[m_iCurrMovieIdx] );
	}
	else if (stricmp(command, "stop") == 0)
	{
		OnClose();
	}
	else
	{
		BaseClass::OnCommand(command);
	}
}

void CGEIntroVideoPanel::ResetBackgroundPlaytime( void )
{
	m_fNextBackgroundPlaytime = gpGlobals->realtime + BACKGROUNDVID_TIMEOUT;
}

void CGEIntroVideoPanel::OnTick( void )
{
	int x,y;
	vgui::surface()->SurfaceGetCursorPos( x, y );
	if ( abs(x - m_iMouseX) > 3 || abs(y - m_iMouseY) > 3 )
		ResetBackgroundPlaytime();

	m_iMouseX = x;
	m_iMouseY = y;

	// We started showing the video mid-load. Make sure it's stopped!
	if ( IsInGame() && IsVisible() && (m_bInBackground || !m_bUserRequested) )
	{
		OnClose();
		return;
	}
	else if ( IsInGame() )
	{
		// Never show the BG Videos while playing the game, but also make sure when we exit the game we don't show them instantly
		ResetBackgroundPlaytime();
		return;
	}
	
	if ( ge_skipintrobg.GetBool() || m_fNextBackgroundPlaytime == -1.0f || gpGlobals->realtime < m_fNextBackgroundPlaytime )
		return;

	// If we made it here we want to show the Background Videos so show them
	SetPlayBackgroundVids( true );
	SetVisible( true );
}

void CGEIntroVideoPanel::Paint( void )
{
	BaseClass::Paint();
	
	float time = gpGlobals->curtime - m_fLastMouseMove;

	if (time >= 0.0f && time <= 3.0f)
	{
		int alpha = 25;
		
		if (time > 1)
			alpha *= (3.0f-time)/2.0;

		m_pButStop->SetAlpha(alpha+20);
		m_pButSkip->SetAlpha(alpha+20);
		m_pButReplay->SetAlpha(alpha+20);
		m_pAlwaysSkip->SetAlpha(alpha+100);
		m_pBGSkip->SetAlpha(alpha+100);

		Color c(75,75,75,alpha);

		int wide, tall;
		GetSize( wide, tall );
		vgui::surface()->DrawSetColor(c);
		vgui::surface()->DrawFilledRect(0, tall-YRES(26), wide, tall);
	}
	else
	{
		if (m_bIsCursorShown)
		{
			m_pButStop->SetVisible(false);
			m_pButSkip->SetVisible(false);
			m_pButReplay->SetVisible(false);
			m_pAlwaysSkip->SetVisible(false);
			m_pBGSkip->SetVisible(false);

			SetCursor( vgui::dc_blank );
			m_bIsCursorShown = false;
		}
	}
	
	// Black out the background (we could omit drawing under the video surface, but this is straight-forward)
	vgui::surface()->DrawSetColor(  0, 0, 0, 255 );
	vgui::surface()->DrawFilledRect( 0, 0, GetWide(), GetTall() );

	if ( !m_pMaterial )
		return;

	// Draw the polys to draw this out
	CMatRenderContextPtr pRenderContext( materials );
	
	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PushMatrix();
	pRenderContext->LoadIdentity();

	pRenderContext->Bind( m_pMaterial, NULL );

	CMeshBuilder meshBuilder;
	IMesh* pMesh = pRenderContext->GetDynamicMesh( true );
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	float flLeftX = (GetWide() - m_nPlaybackWidth) /2;
	float flRightX = flLeftX + m_nPlaybackWidth;

	float flTopY = (GetTall()-m_nPlaybackHeight)/2;
	float flBottomY = flTopY + m_nPlaybackHeight;

	// Map our UVs to cut out just the portion of the video we're interested in
	float flLeftU = 0.0f;
	float flTopV = 0.0f;

	// We need to subtract off a pixel to make sure we don't bleed
	float flRightU = m_flU - ( 1.0f / (float) m_nPlaybackWidth );
	float flBottomV = m_flV - ( 1.0f / (float) m_nPlaybackHeight );

	// Get the current viewport size
	int vx, vy, vw, vh;
	pRenderContext->GetViewport( vx, vy, vw, vh );

	// map from screen pixel coords to -1..1
	flRightX = FLerp( -1, 1, 0, vw, flRightX );
	flLeftX = FLerp( -1, 1, 0, vw, flLeftX );
	flTopY = FLerp( 1, -1, 0, vh ,flTopY );
	flBottomY = FLerp( 1, -1, 0, vh, flBottomY );

	float alpha = ((float)GetFgColor()[3]/255.0f);

	for ( int corner=0; corner<4; corner++ )
	{
		bool bLeft = (corner==0) || (corner==3);
		meshBuilder.Position3f( (bLeft) ? flLeftX : flRightX, (corner & 2) ? flBottomY : flTopY, 0.0f );
		meshBuilder.Normal3f( 0.0f, 0.0f, 1.0f );
		meshBuilder.TexCoord2f( 0, (bLeft) ? flLeftU : flRightU, (corner & 2) ? flBottomV : flTopV );
		meshBuilder.TangentS3f( 0.0f, 1.0f, 0.0f );
		meshBuilder.TangentT3f( 1.0f, 0.0f, 0.0f );
		meshBuilder.Color4f( 1.0f, 1.0f, 1.0f, alpha );
		meshBuilder.AdvanceVertex();
	}
	
	meshBuilder.End();
	pMesh->Draw();

	pRenderContext->MatrixMode( MATERIAL_VIEW );
	pRenderContext->PopMatrix();

	pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	pRenderContext->PopMatrix();
}

CON_COMMAND(showintro, "Brings up the intro video for GE:S")
{
	if ( args.ArgC() > 1 ) {
		if ( !Q_strcmp(args.Arg(1), "0") ) {
			g_GEIntroVideo->SetUserRequested( true );
			g_GEIntroVideo->SetPlayBackgroundVids( false );
		} else {
			g_GEIntroVideo->SetUserRequested( false );
			g_GEIntroVideo->SetPlayBackgroundVids( true );
		}
	} else {
		g_GEIntroVideo->SetUserRequested( false );
	}

	g_GEIntroVideo->SetVisible( !g_GEIntroVideo->IsVisible() );
}
