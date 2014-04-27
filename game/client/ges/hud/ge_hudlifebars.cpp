///////////// Copyright � 2008, Anthony Iacono. All rights reserved. /////////////
// 
// File: ge_hudlifebars.cpp
// Description:
//      Goldeneye style health and armor bar.
//
// Created On: 03/07/2008
// Created By: Anthony Iacono
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "hud.h"
#include "c_ge_player.h"
#include "vgui_controls/AnimationController.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "view.h"
#include "hudelement.h"
#include "vgui/ISurface.h"
#include "game/client/iviewport.h"
#include "ivrenderview.h"
#include "in_buttons.h"
#include "ge_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

class CGEHudLifebars : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CGEHudLifebars, vgui::Panel );

public:
	CGEHudLifebars( const char *pElementName );
	void Init( void );
	void OnThink( void );
	void MsgFunc_Battery( bf_read &msg );
	void MsgFunc_Damage( bf_read &msg );
	void MsgFunc_HideLifeBars( bf_read &msg );
	void LoadBars( void );
	void DrawBars( void );

protected:
	virtual float	CalcDialPoints( float curr, float max );
	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	Paint();

private:
	CHudTexture		*m_pHealthbarFull;
	CHudTexture		*m_pHealthbarEmpty;
	CHudTexture		*m_pArmorbarFull;
	CHudTexture		*m_pArmorbarEmpty;
	CHudTexture		*m_pArmorbarDeny;

	bool		m_bInFlash;
	float		m_flFlashResetTime;

	bool		m_bBarsFlashing;
	float		m_flHideBarsTime;

	vgui::AnimationController	*m_pAnimController;

	int m_Health;
	int m_Armor;
	float m_HealthPercent;
	float m_ArmorPercent;
};

DECLARE_HUDELEMENT( CGEHudLifebars );
DECLARE_HUD_MESSAGE( CGEHudLifebars, Battery );
DECLARE_HUD_MESSAGE( CGEHudLifebars, Damage );
DECLARE_HUD_MESSAGE( CGEHudLifebars, HideLifeBars );

#define HUD_HW 115
#define HUD_HH 350
#define HUD_HDIS 130
#define HUD_HRADIUS 97 + HUD_HW

CGEHudLifebars::CGEHudLifebars( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudLifebars" )
{
	SetParent( g_pClientMode->GetViewport() );
	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_NEEDSUIT );
}

void CGEHudLifebars::Init()
{
	m_Health = 0;
	m_HealthPercent = 0.0f;
	m_Armor = 0;
	m_ArmorPercent = 0.0f;

	m_bInFlash = false;
	m_flFlashResetTime = 0.0f;
	m_bBarsFlashing = false;
	m_flHideBarsTime = 0.0f;

	m_pAnimController = g_pClientMode->GetViewportAnimationController();

	HOOK_HUD_MESSAGE( CGEHudLifebars, Battery );
	HOOK_HUD_MESSAGE( CGEHudLifebars, Damage );
	HOOK_HUD_MESSAGE( CGEHudLifebars, HideLifeBars );
}

// Input when the player's Armor is changed
void CGEHudLifebars::MsgFunc_Battery( bf_read &msg )
{
	CGEPlayer *pPlayer = ToGEPlayer( CBasePlayer::GetLocalPlayer() );
	if ( !pPlayer )
		return;

	int maxArmor = pPlayer->GetMaxArmor();
	int newArmor = msg.ReadShort();

	if(newArmor < 0) newArmor = 0;
	// This is so we actually show something in gameplays that set max armor = 0 after pickup
	if(maxArmor == 0) maxArmor = MAX_ARMOR;
	if(newArmor > maxArmor) newArmor = maxArmor;

	if(newArmor != m_Armor)
	{
		m_Armor = newArmor;

		float DialAP = CalcDialPoints( m_Armor, maxArmor );
		m_ArmorPercent = DialAP; // / (float)maxArmor;

		m_pAnimController->StartAnimationSequence("StatusBarFlash");
		m_bBarsFlashing = true;
		m_flHideBarsTime = gpGlobals->curtime + m_pAnimController->StartAnimationSequence("StatusBarFlash");
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called whenever damage is taken to do the screen flash
//-----------------------------------------------------------------------------
void CGEHudLifebars::MsgFunc_Damage( bf_read &msg )
{
	int armor = msg.ReadByte();	// armor
	int damageTaken = msg.ReadByte();	// health

	// Don't start another flash if we are still working one to prevent a screen whiteout
	if ( !m_bInFlash && (damageTaken > 0 || armor > 0) )
	{
		// Actually took damage?
		if ( damageTaken < 80 )
		{
			m_pAnimController->StartAnimationSequence("DamageSmall");
			m_flFlashResetTime = gpGlobals->curtime + m_pAnimController->GetAnimationSequenceLength("DamageSmall") + 0.05f;
		}
		else if ( damageTaken < 150 )
		{
			m_pAnimController->StartAnimationSequence("DamageMedium");
			m_flFlashResetTime = gpGlobals->curtime + m_pAnimController->GetAnimationSequenceLength("DamageMedium") + 0.05f;
		}
		else
		{
			m_pAnimController->StartAnimationSequence("DamageLarge");
			m_flFlashResetTime = gpGlobals->curtime + m_pAnimController->GetAnimationSequenceLength("DamageLarge") + 0.05f;
		}

		// Emit gasp sound if we are still alive
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pPlayer && pPlayer->GetHealth() > 0 )
			pPlayer->EmitSound( "GEPlayer.Gasp" );

		m_bInFlash = true;
	}
}

void CGEHudLifebars::MsgFunc_HideLifeBars(bf_read &msg )
{
	m_pAnimController->StartAnimationSequence("HideStatusBar");
}

// Since we don't have a usermessage setup when the player takes damage, we have to keep looking for it
void CGEHudLifebars::OnThink()
{
	CGEPlayer* pPlayer = ToGEPlayer( C_BasePlayer::GetLocalPlayer() );
	if ( !pPlayer )
		return;

	if ( m_bInFlash && m_flFlashResetTime < gpGlobals->curtime )
		m_bInFlash = false;

	// This forces the bars to hide at their prescribed time. Shouldn't do much unless they are stuck on
	if ( m_bBarsFlashing && (m_flHideBarsTime + 0.2) < gpGlobals->curtime
		&& !gViewPortInterface->FindPanelByName( PANEL_SCOREBOARD )->IsVisible() )
	{
		m_bBarsFlashing = false;
		m_pAnimController->StartAnimationSequence("HideStatusBar");
	}

	int newHealth = pPlayer->GetHealth();
	if (newHealth < 0) newHealth = 0;
	if (newHealth > pPlayer->GetMaxHealth()) newHealth = pPlayer->GetMaxHealth();

	if(m_Health != newHealth)
	{
		m_Health = newHealth;
		
		float DialHP = CalcDialPoints( m_Health, pPlayer->GetMaxHealth() );
		m_HealthPercent = DialHP; /// (float)pPlayer->GetMaxHealth();
		
		m_pAnimController->StartAnimationSequence("StatusBarFlash");
		m_bBarsFlashing = true;
		m_flHideBarsTime = gpGlobals->curtime + m_pAnimController->StartAnimationSequence("StatusBarFlash");
	}
}

float CGEHudLifebars::CalcDialPoints( float curr, float max )
{
	static const float smallbarscaling = 8.0 / 11.0;
	static const float largebarscaling = 16.0 / 11.0;
	static const float largebaroffset = 5.0 / 11.0;

	if ( max == 0 )
		return 0;

	float prop, percent;
	prop = curr / max;
	percent = ( prop < 0.625f )? ( prop * smallbarscaling ) : ( prop * largebarscaling - largebaroffset );

	return percent;
}

void CGEHudLifebars::LoadBars()
{
	m_pHealthbarFull = gHUD.GetIcon("health_full");
	m_pHealthbarEmpty = gHUD.GetIcon("health_empty");
	m_pArmorbarFull = gHUD.GetIcon("armor_full");
	m_pArmorbarEmpty = gHUD.GetIcon("armor_empty");
	m_pArmorbarDeny = gHUD.GetIcon("armor_deny");
}

void CGEHudLifebars::ApplySchemeSettings(vgui::IScheme *scheme)
{
	BaseClass::ApplySchemeSettings( scheme );
	SetPaintBackgroundEnabled( false );
    SetSize( ScreenWidth(), ScreenHeight() );
}

void CGEHudLifebars::Paint()
{
	if(ShouldDraw())
		DrawBars();
}

void CGEHudLifebars::DrawBars()
{
	if( !m_pHealthbarFull )
		LoadBars();

	CGEPlayer *pPlayer = ToGEPlayer( CBasePlayer::GetLocalPlayer() );
	if ( !pPlayer )
		return;

	int scaledW, scaledH, offset, scaledR;
	offset = scheme()->GetProportionalScaledValue(130);
	scaledW = scheme()->GetProportionalScaledValue(HUD_HW);
	scaledH = scheme()->GetProportionalScaledValue(HUD_HH);
	scaledR = scheme()->GetProportionalScaledValue(HUD_HRADIUS);

	if ( pPlayer->GetMaxArmor() == 0 )
		m_pArmorbarFull->DrawScew_Right( m_pArmorbarDeny->textureId, scaledW, scaledH, m_ArmorPercent, offset, scaledR);
	else
		m_pArmorbarFull->DrawScew_Right( m_pArmorbarEmpty->textureId, scaledW, scaledH, m_ArmorPercent, offset, scaledR);

	m_pHealthbarFull->DrawScew_Left( m_pHealthbarEmpty->textureId, scaledW, scaledH, m_HealthPercent, offset, scaledR);
}

