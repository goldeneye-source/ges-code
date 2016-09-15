///////////// Copyright © 2008, Goldeneye Source. All rights reserved. /////////////
// 
// File: weapon_rcp90.cpp
// Description:
//      Implementation of weapon rcp90.
//
// Created On: 7/24/2008
// Created By: Killermonkey
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "ge_weaponautomatic.h"

#ifdef CLIENT_DLL
#define CWeaponRCP90 C_WeaponRCP90
#endif

//-----------------------------------------------------------------------------
// CWeaponRCP90
//-----------------------------------------------------------------------------

class CWeaponRCP90 : public CGEWeaponAutomatic
{
public:
	DECLARE_CLASS( CWeaponRCP90, CGEWeaponAutomatic );

	CWeaponRCP90(void);

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual void	Precache(void);
	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_RCP90; }

	DECLARE_ACTTABLE();

private:
	CWeaponRCP90( const CWeaponRCP90 & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponRCP90, DT_WeaponRCP90 )

BEGIN_NETWORK_TABLE( CWeaponRCP90, DT_WeaponRCP90 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponRCP90 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_rcp90, CWeaponRCP90 );
PRECACHE_WEAPON_REGISTER( weapon_rcp90 );

acttable_t CWeaponRCP90::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_GES_IDLE_RCP90,						false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_PISTOL,			false },

	{ ACT_MP_RUN,						ACT_GES_RUN_RCP90,						false },
	{ ACT_MP_WALK,						ACT_GES_WALK_RCP90,						false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_PISTOL,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_RCP90,		false },
	{ ACT_GES_ATTACK_RUN_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_RCP90,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_RCP90,		false },

	{ ACT_GES_DRAW,						ACT_GES_GESTURE_DRAW_RIFLE,				false },

	{ ACT_MP_RELOAD_STAND,				ACT_HL2MP_GESTURE_RELOAD_AR2,			false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_AR2,			false },

	{ ACT_MP_JUMP,						ACT_GES_JUMP_RCP90,						false },
	{ ACT_GES_CJUMP,					ACT_GES_CJUMP_RCP90,					false },
};
IMPLEMENT_ACTTABLE( CWeaponRCP90 );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponRCP90::CWeaponRCP90( void )
{

}

void CWeaponRCP90::Precache(void)
{
	PrecacheModel("models/weapons/rcp90/v_rcp90.mdl");
	PrecacheModel("models/weapons/rcp90/w_rcp90.mdl");

	PrecacheMaterial("sprites/hud/weaponicons/phantom");
	PrecacheMaterial("sprites/hud/ammoicons/ammo_9mm");

	PrecacheScriptSound("Weapon.SMG_Reload");
	PrecacheScriptSound("Weapon_rcp90.Single");
	PrecacheScriptSound("Weapon_rcp90.NPC_Single");

	BaseClass::Precache();
}