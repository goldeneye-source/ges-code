///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: weapon_goldengun.cpp
// Description:
//      Implimentation of the Golden Gun.
//
// Created On: 21 Dec 08
// Created By: Jonathan White <Killer Monkey>
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "ge_weaponpistol.h"

#ifdef CLIENT_DLL
#define CWeaponGG C_WeaponGG
#endif

//-----------------------------------------------------------------------------
// CWeaponGG
//-----------------------------------------------------------------------------

class CWeaponGG : public CGEWeaponPistol
{
public:
	DECLARE_CLASS( CWeaponGG, CGEWeaponPistol );

	CWeaponGG();
	virtual bool Reload( void );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual void Precache(void);
	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_GOLDENGUN; }
	
	DECLARE_ACTTABLE();

private:
	CWeaponGG( const CWeaponGG & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponGG, DT_WeaponGG )

BEGIN_NETWORK_TABLE( CWeaponGG, DT_WeaponGG )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponGG )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_golden_gun, CWeaponGG );
PRECACHE_WEAPON_REGISTER( weapon_golden_gun );

acttable_t CWeaponGG::m_acttable[] = 
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
IMPLEMENT_ACTTABLE( CWeaponGG );

CWeaponGG::CWeaponGG()
{
	m_fMaxRange1 = 3500;
}

void CWeaponGG::Precache(void)
{
	PrecacheModel("models/weapons/goldengun/v_goldengun.mdl");
	PrecacheModel("models/weapons/goldengun/w_goldengun.mdl");

	PrecacheMaterial("sprites/hud/weaponicons/gg");
	PrecacheMaterial("sprites/hud/ammoicons/ammo_goldengun");

	PrecacheScriptSound("Weapon_gg.Reload");
	PrecacheScriptSound("Weapon_gg.Single");
	PrecacheScriptSound("Weapon_gg.NPC_Single");
	PrecacheScriptSound("Weapon.Special1");
	PrecacheScriptSound("Weapon.Special2");

	BaseClass::Precache();
}

bool CWeaponGG::Reload( void )
{
	if ( GetOwner()->IsPlayer() )
	{
		if ( ToBasePlayer(GetOwner())->m_nButtons & IN_ATTACK && gpGlobals->curtime > m_flNextPrimaryAttack )
		{
			DryFire();
			return false;
		}
	}

	return BaseClass::Reload();
}
