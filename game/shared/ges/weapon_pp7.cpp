///////////// Copyright © 2008, Anthony Iacono. All rights reserved. /////////////
// 
// File: weapon_pp7.cpp
// Description:
//      Implimentation of the PP7 weapon.
//
// Created On: 2/20/2008 1800
// Created By: Anthony Iacono <pimpinjuice>
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
#define CWeaponPP7 C_WeaponPP7
#endif

//-----------------------------------------------------------------------------
// CWeaponPP7
//-----------------------------------------------------------------------------

class CWeaponPP7 : public CGEWeaponPistol
{
public:
	DECLARE_CLASS( CWeaponPP7, CGEWeaponPistol );

	CWeaponPP7(void) { };

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual void	Precache(void);
	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_PP7; }
	
	DECLARE_ACTTABLE();

private:
	CWeaponPP7( const CWeaponPP7 & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponPP7, DT_WeaponPP7 )

BEGIN_NETWORK_TABLE( CWeaponPP7, DT_WeaponPP7 )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponPP7 )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_pp7, CWeaponPP7 );
PRECACHE_WEAPON_REGISTER( weapon_pp7 );

acttable_t CWeaponPP7::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_GES_IDLE_PISTOL,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_PISTOL,			false },

	{ ACT_MP_RUN,						ACT_GES_RUN_PISTOL,						false },
	{ ACT_MP_WALK,						ACT_GES_WALK_PISTOL,					false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_PISTOL,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_PISTOL,	false },
	{ ACT_GES_ATTACK_RUN_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_PISTOL,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_PISTOL,	false },

	{ ACT_GES_DRAW,						ACT_GES_GESTURE_DRAW_PISTOL,			false },
	{ ACT_GES_DRAW_RUN,					ACT_GES_GESTURE_DRAW_PISTOL_RUN,		false },

	{ ACT_MP_RELOAD_STAND,				ACT_GES_GESTURE_RELOAD_PISTOL,			false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_GES_GESTURE_RELOAD_PISTOL,			false },

	{ ACT_MP_JUMP,						ACT_GES_JUMP_PISTOL,					false },
	{ ACT_GES_CJUMP,					ACT_GES_CJUMP_PISTOL,					false },
};
IMPLEMENT_ACTTABLE( CWeaponPP7 );

void CWeaponPP7 :: Precache(void)
{
	PrecacheModel("models/weapons/pp7/v_pp7.mdl");
	PrecacheModel("models/weapons/pp7/w_pp7.mdl");

	PrecacheMaterial("sprites/hud/weaponicons/pp7");
	PrecacheMaterial("sprites/hud/ammoicons/ammo_9mm");

	PrecacheScriptSound("Weapon_pp7.Single");
	PrecacheScriptSound("Weapon_pp7.AltSingle");
	PrecacheScriptSound("Weapon.Silenced");
	PrecacheScriptSound("Weapon_pp7.NPC_Single");

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// CWeaponPP7Silver
//-----------------------------------------------------------------------------

#ifdef CLIENT_DLL
#define CWeaponPP7Silver C_WeaponPP7Silver
#endif

class CWeaponPP7Silver : public CGEWeaponPistol
{
public:
	DECLARE_CLASS( CWeaponPP7Silver, CGEWeaponPistol );

	CWeaponPP7Silver(void) { };

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual void	Precache(void);
	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_SILVERPP7; }

	DECLARE_ACTTABLE();

private:
	CWeaponPP7Silver( const CWeaponPP7Silver & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponPP7Silver, DT_WeaponPP7Silver )

BEGIN_NETWORK_TABLE( CWeaponPP7Silver, DT_WeaponPP7Silver )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponPP7Silver )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_silver_pp7, CWeaponPP7Silver );
PRECACHE_WEAPON_REGISTER( weapon_silver_pp7 );

acttable_t CWeaponPP7Silver::m_acttable[] = 
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
IMPLEMENT_ACTTABLE( CWeaponPP7Silver );

void CWeaponPP7Silver::Precache(void)
{
	PrecacheModel("models/weapons/pp7/v_pp7_silver.mdl");
	PrecacheModel("models/weapons/pp7/w_pp7_silver.mdl");

	PrecacheMaterial("sprites/hud/weaponicons/pp7_silver");
	PrecacheMaterial("sprites/hud/ammoicons/ammo_9mm");

	PrecacheScriptSound("Weapon_pp7.Single");
	PrecacheScriptSound("Weapon_pp7.AltSingle");
	PrecacheScriptSound("Weapon.Silenced");
	PrecacheScriptSound("Weapon_pp7.NPC_Single");

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// CWeaponPP7Gold
//-----------------------------------------------------------------------------

#ifdef CLIENT_DLL
#define CWeaponPP7Gold C_WeaponPP7Gold
#endif

class CWeaponPP7Gold : public CGEWeaponPistol
{
public:
	DECLARE_CLASS( CWeaponPP7Gold, CGEWeaponPistol );

	CWeaponPP7Gold(void) { };

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual void	Precache(void);
	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_GOLDENPP7; }
	
	DECLARE_ACTTABLE();

private:
	CWeaponPP7Gold( const CWeaponPP7Gold & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponPP7Gold, DT_WeaponPP7Gold )

BEGIN_NETWORK_TABLE( CWeaponPP7Gold, DT_WeaponPP7Gold )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponPP7Gold )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_golden_pp7, CWeaponPP7Gold );
PRECACHE_WEAPON_REGISTER( weapon_golden_pp7 );

acttable_t CWeaponPP7Gold::m_acttable[] = 
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

	{ ACT_MP_JUMP,						ACT_HL2MP_JUMP_PISTOL,						false },
	{ ACT_GES_CJUMP,					ACT_GES_CJUMP_PISTOL,						false },
};
IMPLEMENT_ACTTABLE( CWeaponPP7Gold );

void CWeaponPP7Gold::Precache(void)
{
	PrecacheModel("models/weapons/pp7/v_pp7_gold.mdl");
	PrecacheModel("models/weapons/pp7/w_pp7_gold.mdl");

	PrecacheMaterial("sprites/hud/weaponicons/pp7_gold");
	PrecacheMaterial("sprites/hud/ammoicons/ammo_9mm");

	PrecacheScriptSound("Weapon_pp7.Single");
	PrecacheScriptSound("Weapon_pp7.AltSingle");
	PrecacheScriptSound("Weapon.Silenced");
	PrecacheScriptSound("Weapon_pp7.NPC_Single");

	BaseClass::Precache();
}