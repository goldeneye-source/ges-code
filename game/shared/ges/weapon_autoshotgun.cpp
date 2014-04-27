///////////// Copyright � 2008, Goldeneye Source. All rights reserved. /////////////
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

class CWeaponAutoShotgun : public CGEWeaponPistol
{
public:
	DECLARE_CLASS( CWeaponAutoShotgun, CGEWeaponPistol );

	CWeaponAutoShotgun(void);

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual void PrimaryAttack( void );
	virtual void AddViewKick( void );

	// Override pistol behavior of recoil fire animation
	virtual Activity GetPrimaryAttackActivity( void ) { return ACT_VM_PRIMARYATTACK; };

	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_AUTO_SHOTGUN; }

#ifdef GAME_DLL
	void FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles );
#endif
	
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

void CWeaponAutoShotgun::PrimaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
		return;
	
	CGEPlayer *pGEPlayer = ToGEPlayer( pPlayer );

	// MUST call sound before removing a round from the clip of a CMachineGun
	WeaponSound(SINGLE);

	pPlayer->DoMuzzleFlash();

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	// Don't fire again until our ROF expires
	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	m_flSoonestPrimaryAttack = gpGlobals->curtime + GetClickFireRate();
	m_iClip1 -= 1;

	// Add an accuracy penalty which can move past our maximum penalty time if we're really spastic
	m_flAccuracyPenalty += GetClickFireRate();

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );
	//ToGEPlayer(pPlayer)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	Vector	vecSrc		= pPlayer->Weapon_ShootPosition( );
	Vector	vecAiming	= pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );	

	FireBulletsInfo_t info( 5, vecSrc, vecAiming, pGEPlayer->GetAttackSpread(this), MAX_TRACE_LENGTH, m_iPrimaryAmmoType );
	info.m_pAttacker = pPlayer;

	RecordShotFired();

	// Knock the player's view around
	AddViewKick();

	// Fire the bullets, and force the first shot to be perfectly accuracy
	pPlayer->FireBullets( info );

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0); 
	}
}

#ifdef GAME_DLL
void CWeaponAutoShotgun::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles )
{
	Vector vecShootOrigin, vecShootDir;
	CAI_BaseNPC *npc = pOperator->MyNPCPointer();
	ASSERT( npc != NULL );

	WeaponSound( SINGLE_NPC );
	pOperator->DoMuzzleFlash();
	m_iClip1 -= 1;

	if ( bUseWeaponAngles )
	{
		QAngle	angShootDir;
		GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
		AngleVectors( angShootDir, &vecShootDir );
	}
	else 
	{
		vecShootOrigin = pOperator->Weapon_ShootPosition();
		vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );
	}

	pOperator->FireBullets( 5, vecShootOrigin, vecShootDir, GetBulletSpread(), MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 0 );
}
#endif

void CWeaponAutoShotgun::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle	viewPunch;

	viewPunch.x = SharedRandomFloat( "geshotgunpax", GetGEWpnData().Kick.x_min, GetGEWpnData().Kick.x_max );
	viewPunch.y = SharedRandomFloat( "geshotgunpay", GetGEWpnData().Kick.y_min, GetGEWpnData().Kick.y_max );
	viewPunch.z = 0.0f;

	//Add it to the view punch
	pPlayer->ViewPunch( viewPunch );
}