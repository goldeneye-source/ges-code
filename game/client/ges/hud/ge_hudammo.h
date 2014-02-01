///////////// Copyright � 2008, Goldeneye Source. All rights reserved. /////////////
// 
// File: ge_hudammo.cpp
// Description:
//      Ammo indicators, Goldeneye style.
//
// Created On: 08/6/2008
// Created By: Killermonkey
/////////////////////////////////////////////////////////////////////////////

#include "hudelement.h"
#include "hud_numericdisplay.h"
#include "vgui_controls/Panel.h"
#include "ge_weapon.h"

using namespace vgui;

class CGEHudAmmo : public CHudNumericDisplay, public CHudElement
{
	DECLARE_CLASS_SIMPLE( CGEHudAmmo, CHudNumericDisplay );

public:
	CGEHudAmmo( const char *pElementName );
	virtual void Init( void );
	virtual void VidInit( void );
	virtual void Reset();
	virtual void Paint();

	virtual bool ShouldDraw( void );

	void SetAmmo1(int ammo, bool playAnimation);
	void SetAmmo2(int ammo, bool playAnimation);

protected:
	virtual void OnThink();

	void UpdateAmmoDisplays( void );
	
private:
	CHandle< C_BaseCombatWeapon >	m_hCurrentActiveWeapon;

	int		m_iAmmoClip;
	int		m_iAmmoTotal;

	// Store a persistent version of the numeric display's xpositions
	int m_iDigitXPos;

	CHudTexture *m_iconAmmo;
};

// TODO: Add HUD element for the left ammo display (dualies) once
//       the code is done to implement two view models