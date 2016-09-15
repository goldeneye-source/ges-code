///////////// Copyright © 2008, Goldeneye Source. All rights reserved. /////////////
// 
// File: weapon_kf7.cpp
// Description:
//      Implementation of weapon kf7.
//
// Created On: 7/24/2008
// Created By: Killermonkey
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "ge_weaponautomatic.h"

#ifdef CLIENT_DLL
#define CWeaponKF7 C_WeaponKF7
#include "c_ge_playerresource.h"
#include "c_gemp_player.h"
#else
#include "gemp_player.h"

#endif

//-----------------------------------------------------------------------------
// CWeaponKF7
//-----------------------------------------------------------------------------

class CWeaponKF7 : public CGEWeaponAutomatic
{
public:
	DECLARE_CLASS( CWeaponKF7, CGEWeaponAutomatic );

	CWeaponKF7(void);

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual void	Precache(void);
	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_KF7; }
	virtual void	ItemPostFrame( void );
	virtual bool	Deploy( void );
	virtual bool	Reload( void );

	// NPC Functions
	virtual int		GetMinBurst() { return 3; }
	virtual int		GetMaxBurst() { return 5; }

	DECLARE_ACTTABLE();

private:
	CNetworkVar( int, m_iBurst );
	CNetworkVar( bool, m_bInBurst );

	CWeaponKF7( const CWeaponKF7 & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponKF7, DT_WeaponKF7 )

BEGIN_NETWORK_TABLE( CWeaponKF7, DT_WeaponKF7 )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iBurst ) ),
	RecvPropBool( RECVINFO( m_bInBurst ) ),
#else
	SendPropInt( SENDINFO( m_iBurst ) ),
	SendPropBool( SENDINFO( m_bInBurst ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponKF7 )
	DEFINE_PRED_FIELD( m_iBurst, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bInBurst, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_kf7, CWeaponKF7 );
PRECACHE_WEAPON_REGISTER( weapon_kf7 );

acttable_t CWeaponKF7::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_GES_IDLE_AR33,						false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_AR2,				false },

	{ ACT_MP_RUN,						ACT_GES_RUN_AR33,						false },
	{ ACT_MP_WALK,						ACT_GES_WALK_AR33,						false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_AR2,				false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_AR33,		false },
	{ ACT_GES_ATTACK_RUN_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_AR33,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_AR33,		false },

	{ ACT_GES_DRAW,						ACT_GES_GESTURE_DRAW_RIFLE,				false },

	{ ACT_MP_RELOAD_STAND,				ACT_GES_GESTURE_RELOAD_AR33,			false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_GES_GESTURE_RELOAD_AR33,			false },

	{ ACT_MP_JUMP,						ACT_GES_JUMP_AR33,						false },
	{ ACT_GES_CJUMP,					ACT_GES_CJUMP_AR33,						false },
};
IMPLEMENT_ACTTABLE( CWeaponKF7 );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponKF7::CWeaponKF7( void )
{
	m_iBurst = 0;
	m_bInBurst = false;
}

void CWeaponKF7::Precache(void)
{
	PrecacheModel("models/weapons/kf7/v_kf7.mdl");
	PrecacheModel("models/weapons/kf7/w_kf7.mdl");

	PrecacheMaterial("sprites/hud/weaponicons/kf7");
	PrecacheMaterial("sprites/hud/ammoicons/ammo_rifle");

	PrecacheScriptSound("Weapon.Rifle_Reload");
	PrecacheScriptSound("Weapon_kf7.Single");
	PrecacheScriptSound("Weapon.Special1");
	PrecacheScriptSound("Weapon.Special2");

	BaseClass::Precache();
}

void CWeaponKF7::ItemPostFrame( void )
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

bool CWeaponKF7::Deploy( void )
{
	bool ret = BaseClass::Deploy();

	if ( ret)
		m_iBurst = 0;

	return ret;
}

bool CWeaponKF7::Reload( void )
{
	bool ret = BaseClass::Reload();

	if ( ret)
		m_iBurst = 0;

	return ret;
}
