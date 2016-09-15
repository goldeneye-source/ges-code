///////////// Copyright © 2008, Goldeneye Source. All rights reserved. /////////////
// 
// File: weapon_zmg.cpp
// Description:
//      Implementation of weapon ZMG.
//
// Created On: 7/24/2008
// Created By: Killermonkey
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "ge_weaponautomatic.h"

#ifdef CLIENT_DLL
#define CWeaponZMG C_WeaponZMG
#endif

//-----------------------------------------------------------------------------
// CWeaponZMG
//-----------------------------------------------------------------------------

class CWeaponZMG : public CGEWeaponAutomatic
{
public:
	DECLARE_CLASS( CWeaponZMG, CGEWeaponAutomatic );

	CWeaponZMG(void);

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual void	Precache(void);
	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_ZMG; }

	DECLARE_ACTTABLE();

private:
	CWeaponZMG( const CWeaponZMG & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponZMG, DT_WeaponZMG )

BEGIN_NETWORK_TABLE( CWeaponZMG, DT_WeaponZMG )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponZMG )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_zmg, CWeaponZMG );
PRECACHE_WEAPON_REGISTER( weapon_zmg );

acttable_t CWeaponZMG::m_acttable[] = 
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
IMPLEMENT_ACTTABLE( CWeaponZMG );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponZMG::CWeaponZMG( void )
{

}

void CWeaponZMG::Precache(void)
{
	PrecacheModel("models/weapons/zmg/v_zmg.mdl");
	PrecacheModel("models/weapons/zmg/w_zmg.mdl");

	PrecacheMaterial("sprites/hud/weaponicons/phantom");
	PrecacheMaterial("sprites/hud/ammoicons/ammo_9mm");

	PrecacheScriptSound("Weapon.SMG_Reload");
	PrecacheScriptSound("Weapon_zmg.Single");
	PrecacheScriptSound("Weapon_zmg.NPC_Single");

	BaseClass::Precache();
}