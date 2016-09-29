///////////// Copyright © 2008, Anthony Iacono. All rights reserved. /////////////
// 
// File: ge_hudammo.cpp
// Description:
//      Ammo indicators, Goldeneye style.
//
// Created On: 03/15/2008
// Created By: Anthony Iacono
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "hud.h"
#include "ge_hudammo.h"
#include "iclientmode.h"
#include "view.h"
#include "vgui/ISurface.h"
#include "ivrenderview.h"
#include "in_buttons.h"
#include "vgui_controls/AnimationController.h"
#include "vgui/ILocalize.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

DECLARE_HUDELEMENT( CGEHudAmmo );

ConVar cl_ge_show_ammocount("cl_ge_show_ammocount", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Show the match time when rounds are disabled instead of --:--");

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CGEHudAmmo::CGEHudAmmo( const char *pElementName ) : BaseClass(NULL, "GEAmmo"), CHudElement( pElementName )
{
	SetHiddenBits( HIDEHUD_PLAYERDEAD | HIDEHUD_WEAPONSELECTION );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGEHudAmmo::Init( void )
{
	m_iAmmoClip		= 0;
	m_iAmmoTotal	= 0;

	m_iDigitXPos = 0;
	
	m_iconAmmo = NULL;
}

void CGEHudAmmo::VidInit()
{
	m_iDigitXPos = digit_xpos;
}

//-----------------------------------------------------------------------------
// Purpose: Resets hud after save/restore
//-----------------------------------------------------------------------------
void CGEHudAmmo::Reset()
{
	BaseClass::Reset();

	m_hCurrentActiveWeapon = NULL;
	m_iAmmoClip = 0;
	m_iAmmoTotal = 0;

	UpdateAmmoDisplays();
}

//-----------------------------------------------------------------------------
// Purpose: called every frame to get ammo info from the weapon
//-----------------------------------------------------------------------------
void CGEHudAmmo::UpdateAmmoDisplays( void )
{
	C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
	if (!player)
		return;

	C_GEWeapon *wpn = ToGEWeapon( GetActiveWeapon() );

	if ( !wpn || !player || !wpn->UsesPrimaryAmmo() )
	{
		SetPaintEnabled(false);
		SetPaintBackgroundEnabled(false);
		return;
	}

	SetPaintEnabled(true);
	SetPaintBackgroundEnabled(false);

	// Get our icons for the ammo types
	m_iconAmmo = gWR.GetAmmoIconFromWeapon( wpn->GetPrimaryAmmoType() );

	// Early out for moonraker only
	if ( wpn->GetWeaponID() == WEAPON_MOONRAKER )
	{
		SetShouldDisplayValue(false);
		SetShouldDisplaySecondaryValue(false);
		m_hCurrentActiveWeapon = wpn;
		return;
	}
	else
	{
		SetShouldDisplayValue(true);
	}

	// get the ammo in our clip
	int clip = wpn->Clip1();
	int total;
	if (clip < 0)
	{
		// we don't use clip ammo, just use the total ammo count
		clip = player->GetAmmoCount(wpn->GetPrimaryAmmoType());
		total = 0;
	}
	else
	{
		// we use clip ammo, so the second ammo is the total ammo
		total = player->GetAmmoCount(wpn->GetPrimaryAmmoType());
	}

	if (wpn == m_hCurrentActiveWeapon)
	{
		// same weapon, just update counts
		SetAmmo1(clip, true);
		SetAmmo2(total, true);
	}
	else
	{
		// diferent weapon, change without triggering
		SetAmmo1(clip, false);
		SetAmmo2(total, false);

		// update whether or not we show the total ammo display
		if (wpn->UsesClipsForAmmo1())
		{
			SetShouldDisplaySecondaryValue(true);
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("WeaponUsesClips");
		}
		else
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("WeaponDoesNotUseClips");
			SetShouldDisplaySecondaryValue(false);
		}

		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("WeaponChanged");
		m_hCurrentActiveWeapon = wpn;
	}
}

bool CGEHudAmmo::ShouldDraw( void )
{
	bool ret = CHudElement::ShouldDraw();

	if ( !cl_ge_show_ammocount.GetBool() )
		return false;

	CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return false;

	if ( pPlayer->IsObserver() )
		return false;

	return ret;
}

//-----------------------------------------------------------------------------
// Purpose: called every frame to get ammo info from the weapon
//-----------------------------------------------------------------------------
void CGEHudAmmo::OnThink()
{
	UpdateAmmoDisplays();
}

//-----------------------------------------------------------------------------
// Purpose: Updates ammo display
//-----------------------------------------------------------------------------
void CGEHudAmmo::SetAmmo1(int ammo, bool playAnimation)
{
	if (ammo != m_iAmmoClip)
	{
		if (ammo == 0)
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("AmmoEmpty");
		}
		else if (ammo < m_iAmmoClip)
		{
			// ammo has decreased
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("AmmoDecreased");
		}
		else
		{
			// ammunition has increased
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("AmmoIncreased");
		}

		m_iAmmoClip = ammo;
	}

	SetDisplayValue(ammo);
}

//-----------------------------------------------------------------------------
// Purpose: Updates 2nd ammo display
//-----------------------------------------------------------------------------
void CGEHudAmmo::SetAmmo2(int ammo2, bool playAnimation)
{
	if (ammo2 != m_iAmmoTotal)
	{
		if (ammo2 == 0)
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("Ammo2Empty");
		}
		else if (ammo2 < m_iAmmoTotal)
		{
			// ammo has decreased
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("Ammo2Decreased");
		}
		else
		{
			// ammunition has increased
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("Ammo2Increased");
		}

		m_iAmmoTotal = ammo2;
	}

	SetSecondaryValue(ammo2);
}

void CGEHudAmmo::Paint( void )
{
	int clipX, totalX;

	// Get the size of one digit
	int charWidth = surface()->GetCharacterWidth(m_hNumberFont, '0');
	int charHeight = surface()->GetFontTall(m_hNumberFont);

	int iconX = 0, iconY = 0;
	int iconW = 0;  // If we don't have an icon we won't worry about it's position later

	// Apply indents to our digits, the numeric display indent algorithm is very lacking
	// so we have to do it ourselfs to stay around the bullet
	if ( m_iAmmoClip < 10 )
		clipX = digit_xpos + charWidth;
	else
		clipX = digit_xpos;

	if ( m_iconAmmo )
	{
		iconW = m_iconAmmo->Width();

		// Figure out where we're going to put this
		if ( m_iAmmoClip < 10 )
			iconX = clipX + charWidth + 3;
		else
			iconX = clipX + charWidth + charWidth + 3;

		iconY = (digit_ypos + charHeight) - m_iconAmmo->Height() - YRES(2);
		
		m_iconAmmo->DrawSelf( iconX, iconY, GetFgColor() );
	}

	// Now figure out where to put the total ammo based on the icon position
	totalX = iconX + iconW + 3;

	// draw our numbers
	if (m_bDisplayValue)
	{
		surface()->DrawSetTextColor( GetFgColor() );
		PaintNumbers(m_hNumberFont, clipX, digit_ypos, m_iValue);
	}

	if (m_bDisplaySecondaryValue)
	{
		surface()->DrawSetTextColor( GetFgColor() );
		PaintNumbers(m_hSmallNumberFont, totalX, digit_ypos, m_iSecondaryValue);
	}
}
