///////////// Copyright © 2008, Goldeneye Source. All rights reserved. /////////////
// 
// File: weapon_autoshotgun.cpp
// Description:
//      Implementation of weapon Auto Shotgun.
//
// Created On: 11/1/2008
// Created By: Killermonkey
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "weapon_shotgun.h"

#ifdef CLIENT_DLL
	#include "c_ge_player.h"
#else
	#include "ge_player.h"
#endif

#include "ge_weaponpistol.h"

#ifdef CLIENT_DLL
#define CWeaponAutoShotgun C_WeaponAutoShotgun
#endif

//-----------------------------------------------------------------------------
// CWeaponAutoShotgun
//-----------------------------------------------------------------------------

class CWeaponAutoShotgun : public CWeaponShotgun //Just inherit from shotgun since they behave identically.
{
public:
	DECLARE_CLASS( CWeaponAutoShotgun, CWeaponShotgun );

	CWeaponAutoShotgun(void);

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	// Override pistol behavior of recoil fire animation
	virtual Activity GetPrimaryAttackActivity( void ) { return ACT_VM_PRIMARYATTACK; };

	virtual void	Precache(void);
	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_AUTO_SHOTGUN; }
	virtual bool IsShotgun() { return true; };
	
	DECLARE_ACTTABLE();

private:
	CWeaponAutoShotgun( const CWeaponAutoShotgun & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponAutoShotgun, DT_WeaponAutoShotgun )

BEGIN_NETWORK_TABLE( CWeaponAutoShotgun, DT_WeaponAutoShotgun )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponAutoShotgun )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_auto_shotgun, CWeaponAutoShotgun );
PRECACHE_WEAPON_REGISTER( weapon_auto_shotgun );

acttable_t CWeaponAutoShotgun::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_GES_IDLE_AUTOSHOTGUN,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_SHOTGUN,				false },

	{ ACT_MP_RUN,						ACT_GES_RUN_AUTOSHOTGUN,					false },
	{ ACT_MP_WALK,						ACT_GES_WALK_AUTOSHOTGUN,					false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_SHOTGUN,				false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_AUTOSHOTGUN,	false },
	{ ACT_GES_ATTACK_RUN_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_AUTOSHOTGUN,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_AUTOSHOTGUN,	false },

	{ ACT_GES_DRAW,						ACT_GES_GESTURE_DRAW_RIFLE,					false },

	{ ACT_MP_RELOAD_STAND,				ACT_GES_GESTURE_RELOAD_AUTOSHOTGUN,			false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_GES_GESTURE_RELOAD_AUTOSHOTGUN,			false },

	{ ACT_MP_JUMP,						ACT_GES_JUMP_AUTOSHOTGUN,					false },
	{ ACT_GES_CJUMP,					ACT_GES_CJUMP_AUTOSHOTGUN,					false },
};
IMPLEMENT_ACTTABLE( CWeaponAutoShotgun );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponAutoShotgun::CWeaponAutoShotgun( void )
{
	// NPC Ranging
	m_fMaxRange1 = 1024;
}

void CWeaponAutoShotgun::Precache(void)
{
	PrecacheModel("models/Weapons/autosg/v_autosg.mdl");
	PrecacheModel("models/weapons/autosg/w_autosg.mdl");

	PrecacheMaterial("sprites/hud/weaponicons/auto_shotgun");
	PrecacheMaterial("sprites/hud/ammoicons/ammo_buckshot");

	PrecacheScriptSound("Weapon.Shotgun_Reload");
	PrecacheScriptSound("Weapon_autoshotgun.Single");
	PrecacheScriptSound("Weapon_autoshotgun.NPC_Single");
	PrecacheScriptSound("Weapon.Special1");
	PrecacheScriptSound("Weapon.Special2");

	BaseClass::Precache();
}