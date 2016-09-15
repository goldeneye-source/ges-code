///////////// Copyright © 2008, Killer Monkey. All rights reserved. /////////////
// 
// File: weapon_dd44.cpp
// Description:
//      Implimentation of the DD44 Pistol
//
// Created On: 9/20/08
// Created By: Killer Monkey
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
#define CWeaponDD44 C_WeaponDD44
#endif

//-----------------------------------------------------------------------------
// CWeaponDD44
//-----------------------------------------------------------------------------

class CWeaponDD44 : public CGEWeaponPistol
{
public:
	DECLARE_CLASS( CWeaponDD44, CGEWeaponPistol );

	CWeaponDD44(void);

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual void Precache( void );
	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_DD44; }

	DECLARE_ACTTABLE();

private:
	CWeaponDD44( const CWeaponDD44& );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponDD44, DT_WeaponDD44 )

BEGIN_NETWORK_TABLE( CWeaponDD44, DT_WeaponDD44 )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponDD44 )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_dd44, CWeaponDD44 );
PRECACHE_WEAPON_REGISTER( weapon_dd44 );

acttable_t CWeaponDD44::m_acttable[] = 
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
IMPLEMENT_ACTTABLE( CWeaponDD44 );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponDD44::CWeaponDD44( void )
{
}

void CWeaponDD44::Precache(void)
{
	PrecacheModel("models/weapons/dd44/v_dd44.mdl");
	PrecacheModel("models/weapons/dd44/w_dd44.mdl");

	PrecacheMaterial("sprites/hud/weaponicons/dd44");
	PrecacheMaterial("sprites/hud/ammoicons/ammo_9mm");

	PrecacheScriptSound("Weapon_dd44.Single");
	PrecacheScriptSound("Weapon_dd44.NPC_Single");
	PrecacheScriptSound("Weapon.Special1");
	PrecacheScriptSound("Weapon.Special2");

	BaseClass::Precache();
}