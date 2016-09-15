///////////// Copyright © 2006, Scott Loyd. All rights reserved. /////////////
// 
// File: ge_hudbloodscreen.cpp
// Description:
//      Shows the dieing effect :D
//
// Created On: 3/12/2007 
// Created By: Mark Chandler (modifications by Killer Monkey)
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "filesystem.h"
#include "shareddefs.h"
#include "ge_utils.h"
#include "clientmode_ge.h"
#include "c_ge_player.h"

#include "materialsystem/imaterial.h"
#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"
#include "vgui/IScheme.h"
#include "vgui_controls/AnimationController.h"
#include "vgui_controls/Panel.h"
#include "ge_CreditsText.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define DEATHMUSIC_SOUND "GEPlayer.Death"

class CHudBloodScreen : public vgui::Panel, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CHudBloodScreen, vgui::Panel );

public:
	CHudBloodScreen( const char *pElementName );
	~CHudBloodScreen();

	virtual void Init();
	virtual void Reset();
	virtual void VidInit();
	virtual void LevelInit( void );

	virtual void PlayDeathMusic();
	virtual void FireGameEvent( IGameEvent *event );

protected:
	virtual void ApplySchemeSettings(vgui::IScheme *scheme);
	virtual bool ShouldDraw( void );
	virtual void OnThink( void );
	virtual void Paint( void );

	void DrawFullScreenColor( Color col );
	void DrawRespawnText( void );

private:
	CPanelAnimationVarAliasType( int, m_iTipPadding, "TipPadding", "5", "proportional_int" );
	CPanelAnimationVar( Color,	m_fullScreenColor, "FullScreenDeathColor", "0 0 0 0" );
	CPanelAnimationVar( Color,	m_textColor,  "RespawnTextColor", "0 0 0 0" );
	CPanelAnimationVar( Color,	m_tipBgColor, "TipBgColor",	"200 200 200 255" );
	CPanelAnimationVar( Color,	m_tipFgColor, "TipFgColor",	"255 255 255 255" );
	CPanelAnimationVar( int,	m_iTipAlpha,  "TipAlpha",	"0" );
	CPanelAnimationVar( vgui::HFont, m_hTipFont, "TipFont", "TypeWriter" );
	CPanelAnimationVar( vgui::HFont, m_hFont, "Font", "GERespawnText" );

	vgui::GECreditsText *m_pTipContainer;

	CMaterialReference m_WhiteMaterial;

	bool	m_bShow;
	bool	m_bResetOccured;
	bool	m_bDoFadeOut;
	float	m_flHideTime;

	float	m_flNextThink;
	int		screenWide;
	int		screenTall;
};


DECLARE_HUDELEMENT( CHudBloodScreen );

#define GE_RESPAWN_TEXT				"#GES_ClickToRespawn"

ConVar cl_ge_disablebloodscreen( "cl_ge_disablebloodscreen", "0", FCVAR_ARCHIVE | FCVAR_USERINFO, "Disables the blood screen from showing up when you die" );
ConVar cl_ge_disablehints( "cl_ge_disablehints", "0", FCVAR_CLIENTDLL, "Disables death screen hints" );
extern ConVar cl_ge_disablecuemusic;

using namespace vgui;

CHudBloodScreen::CHudBloodScreen( const char *pElementName ) : CHudElement(pElementName), BaseClass(NULL, "HudBloodScreen")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	ListenForGameEvent( "player_death" );
	ListenForGameEvent( "player_spawn" );

	// Create the tip container
	m_pTipContainer = new vgui::GECreditsText( this, "tip_text" );
}

CHudBloodScreen::~CHudBloodScreen()
{

}

void CHudBloodScreen::Init()
{
	m_bShow			= false;
	m_bResetOccured = false;
	m_bDoFadeOut	= false;
	m_flHideTime	= 0;
	m_flNextThink	= 0;
}

void CHudBloodScreen::Reset()
{
	m_bResetOccured = true;
}

void CHudBloodScreen::VidInit()
{
	SetZPos( -1 );

	surface()->GetScreenSize( screenWide, screenTall );
	SetBounds(0, 0, screenWide, screenTall);

	// Load in the color material
	m_WhiteMaterial.Init( "vgui/white", TEXTURE_GROUP_VGUI ); 
}

void CHudBloodScreen::LevelInit( void )
{
	m_bShow			= false;
	m_flHideTime	= 0;
	m_flNextThink	= 0;

	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "GESDeathFadeOut" );
}

void CHudBloodScreen::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	surface()->GetScreenSize( screenWide, screenTall );
	SetBounds(0, 0, screenWide, screenTall);

	SetPaintBackgroundEnabled( false );
	SetPaintBorderEnabled( false );

	// Setup the tip container
	m_pTipContainer->SetPaintBorderEnabled( true );
	m_pTipContainer->SetPaintBackgroundType( 2 );
	m_pTipContainer->SetVerticalScrollbar( false );
	m_pTipContainer->SetBgColor( m_tipBgColor );
	m_pTipContainer->SetFgColor( m_tipFgColor );
	m_pTipContainer->SetDrawOffsets( m_iTipPadding, m_iTipPadding );
	m_pTipContainer->SetVisible( false );
}

bool CHudBloodScreen::ShouldDraw( void )
{
	return m_bShow || m_flHideTime > gpGlobals->curtime;
}

void CHudBloodScreen::PlayDeathMusic( void )
{
	// Don't play the tunes if we disabled it
	if ( cl_ge_disablecuemusic.GetBool() )
		return;

	CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
	CSingleUserRecipientFilter user( pPlayer );

	pPlayer->EmitSound( user, pPlayer->entindex(), DEATHMUSIC_SOUND );

	IGameEvent *event = gameeventmanager->CreateEvent( "specialmusic_setup" );
	if ( event )
	{
		event->SetInt( "playerid", pPlayer->GetUserID() );
		event->SetInt( "teamid", -1 );
		event->SetString( "soundfile", DEATHMUSIC_SOUND );
		gameeventmanager->FireEventClientSide( event );
	}
}

void CHudBloodScreen::OnThink( void )
{
	if ( gpGlobals->curtime < m_flNextThink )
		return;

	if ( m_bShow )
	{
		C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();
		if( !pPlayer )
			return;

		if ( m_bDoFadeOut && m_bResetOccured )
		{
			m_bShow = false;
			m_bDoFadeOut = false;
			m_flHideTime = gpGlobals->curtime + g_pClientMode->GetViewportAnimationController()->GetAnimationSequenceLength( "GESDeathFadeOut" );
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "GESDeathFadeOut" );
		}

		// Very early out if we are observing someone and they died and haven't respawned yet
		if ( pPlayer->IsObserver() && pPlayer->GetObserverMode() == OBS_MODE_IN_EYE && pPlayer->GetObserverTarget() )
		{
			// Double check always fade out if the person we are observing is alive
			if ( m_bShow && pPlayer->GetObserverTarget()->IsAlive() )
				m_bResetOccured = m_bDoFadeOut = true;

			m_flNextThink = gpGlobals->curtime + 0.1f;
			return;
		}

		// Early out if we went to observer mode
		if( pPlayer->IsObserver() )
		{
			m_bShow = false;
			m_flHideTime = gpGlobals->curtime;
			return;
		}
	}

	m_flNextThink = gpGlobals->curtime + 0.03f;
}

void CHudBloodScreen::Paint( void )
{
	DrawFullScreenColor( m_fullScreenColor );
	DrawRespawnText();
}

void CHudBloodScreen::DrawFullScreenColor( Color col )
{
	CMatRenderContextPtr pRenderContext( materials );

	pRenderContext->Bind( m_WhiteMaterial );
	IMesh *pMesh = pRenderContext->GetDynamicMesh();
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Color4ub( col.r(), col.g(), col.b(), col.a() );
	meshBuilder.TexCoord2f( 0,0,0 );
	meshBuilder.Position3f( 0, 0, 0 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( col.r(), col.g(), col.b(), col.a() );
	meshBuilder.TexCoord2f( 0,1,0 );
	meshBuilder.Position3f( GetWide(), 0, 0 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( col.r(), col.g(), col.b(), col.a() );
	meshBuilder.TexCoord2f( 0,1,1 );
	meshBuilder.Position3f( GetWide(), GetTall(), 0 );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( col.r(), col.g(), col.b(), col.a() );
	meshBuilder.TexCoord2f( 0,0,1 );
	meshBuilder.Position3f( 0, GetTall(), 0 );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

void CHudBloodScreen::DrawRespawnText( void )
{
	wchar_t wszText[256];
	_snwprintf( wszText, 256, g_pVGuiLocalize->Find(GE_RESPAWN_TEXT) );

	int wide, tall;
	vgui::surface()->GetTextSize( m_hFont, wszText, wide, tall );

	int xpos, ypos;
	xpos = (GetWide() - wide) / 2;
	ypos = (GetTall() - tall) / 2;

	vgui::surface()->DrawSetTextFont( m_hFont );
	vgui::surface()->DrawSetTextPos( xpos, ypos );
	vgui::surface()->DrawSetTextColor( m_textColor );
	vgui::surface()->DrawPrintText( wszText, wcslen(wszText) );

	if ( m_pTipContainer->IsVisible() )
		m_pTipContainer->SetAlpha( m_iTipAlpha );
}

void CHudBloodScreen::FireGameEvent( IGameEvent *event )
{
	C_GEPlayer *pLocal = ToGEPlayer( C_BasePlayer::GetLocalPlayer() );
	if ( !pLocal )
		return;

	if ( event->GetInt("userid") != pLocal->GetUserID() )
	{
		// Check if the person we are observing died
		if ( pLocal->IsObserver() && pLocal->GetObserverMode() == OBS_MODE_IN_EYE && pLocal->GetObserverTarget() && pLocal->GetObserverTarget()->IsPlayer() )
		{
			if ( ToBasePlayer(pLocal->GetObserverTarget())->GetUserID() != event->GetInt("userid") )
				return;
			
			// Fake a reset since we don't actually reset if we spectate a respawning player
			m_bResetOccured = true;
		}
		else
			return;
	}

	if ( !Q_stricmp( event->GetName(), "player_spawn" ) )
	{
		m_bDoFadeOut = true;
	}
	else // player_death
	{
		PlayDeathMusic();

		if ( cl_ge_disablebloodscreen.GetBool() )
			return;

		// Make sure we move out of aim mode
		pLocal->ResetAimMode( true );

		m_bShow = true;
		m_bResetOccured = false;

		// Choose to show a tip with 40% chance
		if ( !cl_ge_disablehints.GetBool() && GERandom<float>(1.0f) <= 0.4f )
		{
			const GameplayTip *tip = GetClientModeGENormal()->GetRandomTip();
			const char *line = GetClientModeGENormal()->GetRandomTipLine( tip );
			if ( tip && line )
			{
				wchar_t tmp[128];
				GEUTIL_ParseLocalization( tmp, 128, line );

				m_pTipContainer->SetText( "" );
				m_pTipContainer->InsertFontChange( m_hFont );
				m_pTipContainer->InsertCenterText( true );
				m_pTipContainer->InsertString( tip->name );
				m_pTipContainer->InsertChar( L'\n' );

				m_pTipContainer->InsertFontChange( m_hTipFont );
				m_pTipContainer->InsertCenterText( false );
				m_pTipContainer->InsertString( tmp );

				int wide, tall;
				GEUTIL_GetTextSize( line, m_hTipFont, wide, tall );
				m_pTipContainer->SetWide( wide + XRES(10) + m_iTipPadding );
				m_pTipContainer->SetToFullHeight();
				m_pTipContainer->SetTall( m_pTipContainer->GetTall() + tall + m_iTipPadding );
				m_pTipContainer->SetPos( (ScreenWidth() / 2) - (wide / 2), ScreenHeight() / 2 + 50 );

				m_pTipContainer->SetVisible( true );
				m_pTipContainer->SetAlpha( 0 );
				m_iTipAlpha = 0;
			}
		}
		else
		{
			m_pTipContainer->SetText( "" );
			m_pTipContainer->SetVisible( false );
		}

		// Ensure our screen is not faded out
		m_fullScreenColor.SetColor( 0, 0, 0, 0 );
		m_textColor.SetColor( 0, 0, 0, 0 );

		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "GESDeathFadeIn" );
		m_flNextThink = gpGlobals->curtime + 0.5f;
	}
}
