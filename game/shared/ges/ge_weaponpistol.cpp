///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_weaponpistol.cpp
// Description:
//      All Goldeneye PISTOLS derive from this class
//
// Created On: 3/5/2008 1800
// Created By: Jonathan White <killermonkey>
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

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( GEWeaponPistol, DT_GEWeaponPistol )

BEGIN_NETWORK_TABLE( CGEWeaponPistol, DT_GEWeaponPistol )
#ifdef CLIENT_DLL
	RecvPropTime( RECVINFO( m_flSoonestPrimaryAttack ) ),
	RecvPropTime( RECVINFO( m_flLastAttackTime ) ),
	RecvPropInt( RECVINFO( m_nNumShotsFired ) ),
#else
	SendPropTime( SENDINFO( m_flSoonestPrimaryAttack ) ),
	SendPropTime( SENDINFO( m_flLastAttackTime ) ),
	SendPropInt( SENDINFO( m_nNumShotsFired ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CGEWeaponPistol )
	DEFINE_PRED_FIELD( m_flSoonestPrimaryAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flLastAttackTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nNumShotsFired, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

#if !defined( CLIENT_DLL )

#include "globalstate.h"
//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CGEWeaponPistol )

	DEFINE_FIELD( m_flSoonestPrimaryAttack, FIELD_TIME ),
	DEFINE_FIELD( m_flLastAttackTime, FIELD_TIME ),
	DEFINE_FIELD( m_nNumShotsFired, FIELD_INTEGER ),

END_DATADESC()
#endif

//LINK_ENTITY_TO_CLASS( geweapon_pistol, CGEWeaponPistol );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CGEWeaponPistol::CGEWeaponPistol( void )
{
	m_flSoonestPrimaryAttack = gpGlobals->curtime;
	m_flAccuracyPenalty = 0.0f;

	m_fMinRange1		= 24;
	m_fMaxRange1		= 2000;

	m_bFiresUnderwater	= true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CGEWeaponPistol::PrimaryAttack( void )
{
	// We can't fire yet!
	if ( m_flSoonestPrimaryAttack > gpGlobals->curtime )
		return;

	// Used to determine the animation to play
	if ( ( gpGlobals->curtime - m_flLastAttackTime ) > 0.5f )
		m_nNumShotsFired = 0;
	else
		m_nNumShotsFired++;

	m_flLastAttackTime = gpGlobals->curtime;
	m_flSoonestPrimaryAttack = gpGlobals->curtime + GetClickFireRate();

	// Don't fire again until our ROF expires
	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	m_iClip1 -= 1;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if( pOwner )
	{
		// Each time the player fires the pistol, reset the view punch. This prevents
		// the aim from 'drifting off' when the player fires very quickly. This may
		// not be the ideal way to achieve this, but it's cheap and it works, which is
		// great for a feature we're evaluating. (sjb)
		pOwner->ViewPunchReset();
	}

	BaseClass::PrimaryAttack();
}

//-----------------------------------------------------------------------------
// Purpose: Allows firing as fast as button is pressed
//-----------------------------------------------------------------------------
void CGEWeaponPistol::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	if ( m_bInReload )
		return;
	
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL || pOwner->m_nButtons & IN_RELOAD )
		return;

	//Allow a refire as fast as the player can click
	if ( ( ( pOwner->m_nButtons & IN_ATTACK ) == false ) && ( m_flSoonestPrimaryAttack < gpGlobals->curtime ) )
	{
		m_flNextPrimaryAttack = gpGlobals->curtime - 0.1f;
	}
	else if ( ( pOwner->m_nButtons & IN_ATTACK ) && ( m_flNextPrimaryAttack < gpGlobals->curtime ) && ( m_iClip1 <= 0 ) )
	{
		m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
Activity CGEWeaponPistol::GetPrimaryAttackActivity( void )
{
	return ACT_VM_PRIMARYATTACK;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CGEWeaponPistol::Reload( void )
{
	bool bRet = BaseClass::Reload();

	if ( bRet )
		m_flAccuracyPenalty = 0.0f;

	return bRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGEWeaponPistol::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle	viewPunch;

	viewPunch.x = SharedRandomFloat( "gepistolpax", GetGEWpnData().Kick.x_min, GetGEWpnData().Kick.x_max );
	viewPunch.y = SharedRandomFloat( "gepistolpay", GetGEWpnData().Kick.y_min, GetGEWpnData().Kick.y_max );
	viewPunch.z = 0.0f;


	if (viewPunch.x == 0.0f && viewPunch.y == 0.0f)
		return;

	CGEPlayer *pGEOwner = ToGEPlayer(GetOwner());

	if (pGEOwner)
	{
		if (pGEOwner->IsInAimMode())
		{
			viewPunch.x *= 0.25;
			viewPunch.y *= 0.25;
		}

		viewPunch.x *= GetAccPenalty() * 3 / GetAccShots() + 1;
		viewPunch.y *= GetAccPenalty() * 3 / GetAccShots() + 1;
	}

	//Add it to the view punch
	pPlayer->ViewPunch( viewPunch );
}