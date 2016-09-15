///////////// Copyright © 2008, Goldeneye Source. All rights reserved. /////////////
// 
// File: weapon_d5k.cpp
// Description:
//      Implementation of weapon D5K.
//
// Created On: 7/24/2008
// Created By: Killermonkey
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "ge_weaponautomatic.h"

#ifdef CLIENT_DLL
#define CWeaponD5K C_WeaponD5K
#define CWeaponD5KSilenced C_WeaponD5KSilenced
#endif

//-----------------------------------------------------------------------------
// CWeaponD5K
//-----------------------------------------------------------------------------

class CWeaponD5K : public CGEWeaponAutomatic
{
public:
	DECLARE_CLASS( CWeaponD5K, CGEWeaponAutomatic );

	CWeaponD5K( void ) { };

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_D5K; }
	virtual void Precache( void );

	DECLARE_ACTTABLE();

private:
	CWeaponD5K( const CWeaponD5K & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponD5K, DT_WeaponD5K )

BEGIN_NETWORK_TABLE( CWeaponD5K, DT_WeaponD5K )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponD5K )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_d5k, CWeaponD5K );
PRECACHE_WEAPON_REGISTER( weapon_d5k );

acttable_t CWeaponD5K::m_acttable[] = 
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
IMPLEMENT_ACTTABLE( CWeaponD5K );


void CWeaponD5K::Precache(void)
{
	PrecacheModel("models/weapons/d5k/v_d5k.mdl");
	PrecacheModel("models/weapons/d5k/w_d5k.mdl");

	PrecacheMaterial("sprites/hud/weaponicons/d5k");
	PrecacheMaterial("sprites/hud/ammoicons/ammo_9mm");

	PrecacheScriptSound("Weapon.SMG_Reload");
	PrecacheScriptSound("Weapon.Empty");
	PrecacheScriptSound("Weapon_d5k.Single");
	PrecacheScriptSound("Weapon_d5k.NPC_Single");

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// CWeaponD5KSilenced
//-----------------------------------------------------------------------------

class CWeaponD5KSilenced : public CGEWeaponAutomatic
{
public:
	DECLARE_CLASS( CWeaponD5KSilenced, CGEWeaponAutomatic );

	CWeaponD5KSilenced( void );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual GEWeaponID	GetWeaponID( void ) const { return WEAPON_D5K_SILENCED; }
	virtual bool		CanBeSilenced( void ) { return true; }
	virtual void		Precache( void );

	DECLARE_ACTTABLE();

private:
	CWeaponD5KSilenced( const CWeaponD5KSilenced & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponD5KSilenced, DT_WeaponD5KSilenced )

BEGIN_NETWORK_TABLE( CWeaponD5KSilenced, DT_WeaponD5KSilenced )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponD5KSilenced )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_d5k_silenced, CWeaponD5KSilenced );
PRECACHE_WEAPON_REGISTER( weapon_d5k_silenced );

acttable_t CWeaponD5KSilenced::m_acttable[] = 
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
IMPLEMENT_ACTTABLE( CWeaponD5KSilenced );

CWeaponD5KSilenced::CWeaponD5KSilenced( void )
{
	ToggleSilencer( false );
}

void CWeaponD5KSilenced::Precache(void)
{
	PrecacheModel("models/weapons/d5k/v_d5k_silenced.mdl");
	PrecacheModel("models/weapons/d5k/w_d5k_silenced.mdl");

	PrecacheMaterial("sprites/hud/weaponicons/d5k");
	PrecacheMaterial("sprites/hud/ammoicons/ammo_9mm");

	PrecacheScriptSound("Weapon.SMG_Reload");
	PrecacheScriptSound("Weapon_d5k.AltSingle");
	PrecacheScriptSound("Weapon_d5k.NPC_AltSingle");

	BaseClass::Precache();
}