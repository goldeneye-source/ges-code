///////////// Copyright © 2008, Goldeneye Source. All rights reserved. /////////////
// 
// File: weapon_klobb.cpp
// Description:
//      Implementation of weapon Klobb.
//
// Created On: 7/24/2008
// Created By: Killermonkey
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "ge_weaponautomatic.h"

#ifdef CLIENT_DLL
#define CWeaponKlobb C_WeaponKlobb
#include "c_ge_playerresource.h"
#include "c_gemp_player.h"
#else

#include "gemp_player.h"

#endif

//-----------------------------------------------------------------------------
// CWeaponKlobb
//-----------------------------------------------------------------------------

class CWeaponKlobb : public CGEWeaponAutomatic
{
public:
	DECLARE_CLASS( CWeaponKlobb, CGEWeaponAutomatic );

	CWeaponKlobb(void);

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual void	Precache(void);
	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_KLOBB; }
	
	DECLARE_ACTTABLE();

private:
	CWeaponKlobb( const CWeaponKlobb & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponKlobb, DT_WeaponKlobb )

BEGIN_NETWORK_TABLE( CWeaponKlobb, DT_WeaponKlobb )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponKlobb )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_klobb, CWeaponKlobb );
PRECACHE_WEAPON_REGISTER( weapon_klobb );

acttable_t CWeaponKlobb::m_acttable[] = 
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
IMPLEMENT_ACTTABLE( CWeaponKlobb );

CWeaponKlobb::CWeaponKlobb()
{
	// NPC Ranging
	m_fMaxRange1 = 1024;
}

void CWeaponKlobb::Precache(void)
{
	PrecacheModel("models/weapons/klobb/v_klobb.mdl");
	PrecacheModel("models/weapons/klobb/w_klobb.mdl");

	PrecacheMaterial("sprites/hud/weaponicons/kf7");
	PrecacheMaterial("sprites/hud/ammoicons/ammo_rifle");

	PrecacheScriptSound("Weapon.Reload");
	PrecacheScriptSound("Weapon_klobb.Single");
	PrecacheScriptSound("Weapon_klobb.NPC_Single");
	PrecacheScriptSound("Weapon.Special1");
	PrecacheScriptSound("Weapon.Special2");

	BaseClass::Precache();
}