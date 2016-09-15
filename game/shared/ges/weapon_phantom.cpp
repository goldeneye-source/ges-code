///////////// Copyright © 2008, Goldeneye Source. All rights reserved. /////////////
// 
// File: weapon_phantom.cpp
// Description:
//      Implementation of weapon Phantom.
//
// Created On: 11/3/2008
// Created By: Killermonkey
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "ge_weaponautomatic.h"

#ifdef CLIENT_DLL
#define CWeaponPhantom C_WeaponPhantom
#endif

//-----------------------------------------------------------------------------
// CWeaponPhantom
//-----------------------------------------------------------------------------

class CWeaponPhantom : public CGEWeaponAutomatic
{
public:
	DECLARE_CLASS( CWeaponPhantom, CGEWeaponAutomatic );

	CWeaponPhantom(void);

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual void Precache(void);
	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_PHANTOM; }
	
	DECLARE_ACTTABLE();

private:
	CWeaponPhantom( const CWeaponPhantom & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponPhantom, DT_WeaponPhantom )

BEGIN_NETWORK_TABLE( CWeaponPhantom, DT_WeaponPhantom )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponPhantom )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_phantom, CWeaponPhantom );
PRECACHE_WEAPON_REGISTER( weapon_phantom );

acttable_t CWeaponPhantom::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_GES_IDLE_PHANTOM,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_AR2,				false },

	{ ACT_MP_RUN,						ACT_GES_RUN_PHANTOM,					false },
	{ ACT_MP_WALK,						ACT_GES_WALK_PHANTOM,					false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_AR2,				false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_PHANTOM,	false },
	{ ACT_GES_ATTACK_RUN_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_PHANTOM,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_PHANTOM,	false },

	{ ACT_GES_DRAW,						ACT_GES_GESTURE_DRAW_RIFLE,				false },

	{ ACT_MP_RELOAD_STAND,				ACT_GES_GESTURE_RELOAD_PHANTOM,			false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_GES_GESTURE_RELOAD_PHANTOM,			false },

	{ ACT_MP_JUMP,						ACT_GES_JUMP_PHANTOM,					false },
	{ ACT_GES_CJUMP,					ACT_GES_CJUMP_PHANTOM,					false },
};
IMPLEMENT_ACTTABLE( CWeaponPhantom );


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponPhantom::CWeaponPhantom( void )
{

}

void CWeaponPhantom::Precache(void)
{
	PrecacheModel("models/weapons/phantom/v_phantom.mdl");
	PrecacheModel("models/weapons/phantom/w_phantom.mdl");

	PrecacheMaterial("sprites/hud/weaponicons/phantom");
	PrecacheMaterial("sprites/hud/ammoicons/ammo_9mm");

	PrecacheScriptSound("Weapon.SMG_Reload");
	PrecacheScriptSound("Weapon_phantom.Single");
	PrecacheScriptSound("Weapon_phantom.NPC_Single");
	PrecacheScriptSound("Weapon.Special1");
	PrecacheScriptSound("Weapon.Special2");

	BaseClass::Precache();
}