///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: weapon_pp7_silenced.cpp
// Description:
//      Implimentation of the PP7 weapon (silenced).
//
// Created On: 11/14/2008
// Created By: Killermonkey
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_ge_player.h"
#else
	#include "ge_player.h"
#endif

#include "ge_weaponpistol.h"

#ifdef CLIENT_DLL
#define CWeaponPP7Silenced C_WeaponPP7Silenced
#endif

//-----------------------------------------------------------------------------
// CWeaponPP7Silenced
//-----------------------------------------------------------------------------

class CWeaponPP7Silenced : public CGEWeaponPistol
{
public:
	DECLARE_CLASS( CWeaponPP7Silenced, CGEWeaponPistol );

	CWeaponPP7Silenced(void);

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual void	Precache(void);
	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_PP7_SILENCED; }

	// So that we can explicitly make this weapon silenced upon creation
	virtual bool	CanBeSilenced( void ) { return true; }
	
	DECLARE_ACTTABLE();

private:
	CWeaponPP7Silenced( const CWeaponPP7Silenced & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponPP7Silenced, DT_WeaponPP7Silenced )

BEGIN_NETWORK_TABLE( CWeaponPP7Silenced, DT_WeaponPP7Silenced )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponPP7Silenced )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_pp7_silenced, CWeaponPP7Silenced );
PRECACHE_WEAPON_REGISTER( weapon_pp7_silenced );

acttable_t CWeaponPP7Silenced::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_GES_IDLE_PISTOL,						false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_PISTOL,				false },

	{ ACT_MP_RUN,						ACT_GES_RUN_PISTOL,							false },
	{ ACT_MP_WALK,						ACT_GES_WALK_PISTOL,						false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_PISTOL,				false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_PISTOL,		false },
	{ ACT_GES_ATTACK_RUN_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_PISTOL,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_PISTOL,		false },

	{ ACT_GES_DRAW,						ACT_GES_GESTURE_DRAW_PISTOL,				false },
	{ ACT_GES_DRAW_RUN,					ACT_GES_GESTURE_DRAW_PISTOL_RUN,			false },

	{ ACT_MP_RELOAD_STAND,				ACT_GES_GESTURE_RELOAD_PISTOL,				false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_GES_GESTURE_RELOAD_PISTOL,				false },

	{ ACT_MP_JUMP,						ACT_GES_JUMP_PISTOL,						false },
	{ ACT_GES_CJUMP,					ACT_GES_CJUMP_PISTOL,						false },
};
IMPLEMENT_ACTTABLE( CWeaponPP7Silenced );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponPP7Silenced::CWeaponPP7Silenced( void )
{
	ToggleSilencer( false );
}

void CWeaponPP7Silenced::Precache(void)
{
	PrecacheModel("models/weapons/pp7/v_pp7_s.mdl");
	PrecacheModel("models/weapons/pp7/w_pp7_s.mdl");

	PrecacheMaterial("sprites/hud/weaponicons/pp7_silenced");
	PrecacheMaterial("sprites/hud/ammoicons/ammo_9mm");

	PrecacheScriptSound("Weapon.Silenced");

	BaseClass::Precache();
}