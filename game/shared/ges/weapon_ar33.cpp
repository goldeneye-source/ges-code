///////////// Copyright © 2008, Goldeneye Source. All rights reserved. /////////////
// 
// File: weapon_AR33.cpp
// Description:
//      Implementation of weapon AR33.
//
// Created On: 7/24/2008
// Created By: Killermonkey
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "ge_weaponautomatic.h"

#ifdef CLIENT_DLL
	#define CWeaponAR33 C_WeaponAR33
#else
	#include "soundent.h"
#endif

//-----------------------------------------------------------------------------
// CWeaponAR33
//-----------------------------------------------------------------------------

class CWeaponAR33 : public CGEWeaponAutomatic
{
public:
	DECLARE_CLASS( CWeaponAR33, CGEWeaponAutomatic );

	CWeaponAR33(void);

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_AR33; }
	virtual void	ItemPostFrame( void );
	virtual bool	Deploy( void );
	virtual bool	Reload( void );
	virtual void	Precache( void );

	DECLARE_ACTTABLE();

private:
	CNetworkVar( int, m_iBurst );
	CNetworkVar( bool, m_bInBurst );

	CWeaponAR33( const CWeaponAR33 & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponAR33, DT_WeaponAR33 )

BEGIN_NETWORK_TABLE( CWeaponAR33, DT_WeaponAR33 )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iBurst ) ),
	RecvPropBool( RECVINFO( m_bInBurst ) ),
#else
	SendPropInt( SENDINFO( m_iBurst ) ),
	SendPropBool( SENDINFO( m_bInBurst ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponAR33 )
	DEFINE_PRED_FIELD( m_iBurst, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bInBurst, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_ar33, CWeaponAR33 );
PRECACHE_WEAPON_REGISTER( weapon_AR33 );

acttable_t CWeaponAR33::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_GES_IDLE_AR33,						false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_SMG1,				false },

	{ ACT_MP_RUN,						ACT_GES_RUN_AR33,						false },
	{ ACT_MP_WALK,						ACT_GES_WALK_AR33,						false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_SMG1,				false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_AR33,		false },
	{ ACT_GES_ATTACK_RUN_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_AR33,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_AR33,		false },

	{ ACT_GES_DRAW,						ACT_GES_GESTURE_DRAW_RIFLE,				false },

	{ ACT_MP_RELOAD_STAND,				ACT_GES_GESTURE_RELOAD_AR33,			false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_GES_GESTURE_RELOAD_AR33,			false },

	{ ACT_MP_JUMP,						ACT_GES_JUMP_AR33,						false },
	{ ACT_GES_CJUMP,					ACT_GES_CJUMP_AR33,						false },
};
IMPLEMENT_ACTTABLE( CWeaponAR33 );


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponAR33::CWeaponAR33( void )
{
	m_iBurst = 0;
	m_bInBurst = false;
}

void CWeaponAR33::Precache(void)
{
	PrecacheModel("models/weapons/ar33/v_ar33.mdl");
	PrecacheModel("models/weapons/ar33/w_ar33.mdl");

	PrecacheMaterial("sprites/hud/weaponicons/ar33");
	PrecacheMaterial("sprites/hud/ammoicons/ammo_rifle");

	PrecacheScriptSound("Weapon.Rifle_Reload");
	PrecacheScriptSound("Weapon_ar33.Single");
	PrecacheScriptSound("Weapon_ar33.NPC_Single");
	PrecacheScriptSound("Weapon_ar33.Burst");

	BaseClass::Precache();
}

void CWeaponAR33::ItemPostFrame( void )
{
	CGEPlayer *pOwner = ToGEPlayer ( GetOwner() );
	if ( !pOwner )
		return;

	if ( !m_bInBurst && m_nNumShotsFired > 0 )
		m_iBurst = (m_nNumShotsFired < 3) ? (3 - m_nNumShotsFired) : (m_nNumShotsFired % 3);

	if ( m_iBurst > 0 && ( pOwner->m_nButtons & IN_ATTACK ) == false && !pOwner->IsInAimMode() )
	{
		m_bInBurst = true;
		if ( gpGlobals->curtime > m_flNextPrimaryAttack )
		{
			PrimaryAttack();
			m_iBurst--;
		}
	}
	else
	{
		m_bInBurst = false;
		m_iBurst = 0;
	}

	BaseClass::ItemPostFrame();
}

bool CWeaponAR33::Deploy( void )
{
	bool ret = BaseClass::Deploy();

	if ( ret )
		m_iBurst = 0;

	return ret;
}

bool CWeaponAR33::Reload( void )
{
	bool ret = BaseClass::Reload();

	if ( ret )
		m_iBurst = 0;

	return ret;
}
