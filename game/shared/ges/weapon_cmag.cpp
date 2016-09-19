///////////// Copyright © 2008, Anthony Iacono. All rights reserved. /////////////
// 
// File: weapon_cmag.cpp
// Description:
//      Implimentation of the Cougar Magnum
//
// Created On: 09/16/2008
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
#define CWeaponCMag C_WeaponCMag
#endif

//-----------------------------------------------------------------------------
// CWeaponPP7
//-----------------------------------------------------------------------------

class CWeaponCMag : public CGEWeaponPistol
{
public:
	DECLARE_CLASS( CWeaponCMag, CGEWeaponPistol );

	CWeaponCMag(void);

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_COUGAR_MAGNUM; }

	virtual void PrimaryAttack( void );
	// This function is purely for the CMag and encorporates primary attack functionality
	// but does not play the animation again!
	virtual void FireWeapon( void );
	virtual void HandleFireOnEmpty(void);

	virtual void Precache( void );
	virtual bool Deploy( void );

	virtual void ItemPreFrame( void );
	virtual void ItemPostFrame( void );
	
	DECLARE_ACTTABLE();

private:
	CNetworkVar( bool, m_bPreShoot );
	CNetworkVar( float, m_flShootTime );

	CWeaponCMag( const CWeaponCMag & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponCMag, DT_WeaponCMag )

BEGIN_NETWORK_TABLE( CWeaponCMag, DT_WeaponCMag )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO(m_bPreShoot) ),
	RecvPropFloat( RECVINFO(m_flShootTime) ),
#else
	SendPropBool( SENDINFO(m_bPreShoot) ),
	SendPropFloat( SENDINFO(m_flShootTime) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponCMag )
	DEFINE_PRED_FIELD( m_bPreShoot, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flShootTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD_TOL( m_flShootTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_cmag, CWeaponCMag );
PRECACHE_WEAPON_REGISTER( weapon_cmag );

acttable_t CWeaponCMag::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_GES_IDLE_CMAG,						false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_PISTOL,			false },

	{ ACT_MP_RUN,						ACT_GES_RUN_CMAG,						false },
	{ ACT_MP_WALK,						ACT_GES_WALK_CMAG,						false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_PISTOL,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_CMAG,		false },
	{ ACT_GES_ATTACK_RUN_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_CMAG,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_CMAG,		false },

	{ ACT_GES_DRAW,						ACT_GES_GESTURE_DRAW_PISTOL,			false },
	{ ACT_GES_DRAW_RUN,					ACT_GES_GESTURE_DRAW_PISTOL_RUN,		false },

	{ ACT_MP_RELOAD_STAND,				ACT_GES_GESTURE_RELOAD_CMAG,			false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_GES_GESTURE_RELOAD_CMAG,			false },

	{ ACT_MP_JUMP,						ACT_GES_JUMP_CMAG,						false },
	{ ACT_GES_CJUMP,					ACT_GES_CJUMP_CMAG,						false },
};
IMPLEMENT_ACTTABLE( CWeaponCMag );


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponCMag::CWeaponCMag( void )
{
	m_bPreShoot = false;
	m_flShootTime = 0.0f;

	m_fMaxRange1 = 3000;
}

void CWeaponCMag::Precache(void)
{
	PrecacheModel("models/weapons/cmag/v_cmag.mdl");
	PrecacheModel("models/weapons/cmag/w_cmag.mdl");

	PrecacheMaterial("sprites/hud/weaponicons/cmag");
	PrecacheMaterial("sprites/hud/ammoicons/ammo_magnum");

	PrecacheScriptSound("Weapon_cmag.Single");
	PrecacheScriptSound("Weapon_cmag.NPC_Single");
	PrecacheScriptSound("Weapon.Special1");
	PrecacheScriptSound("Weapon.Special2");

	BaseClass::Precache();
}

bool CWeaponCMag::Deploy( void )
{
	m_bPreShoot = false;
	m_flShootTime = 0.0f;

	return BaseClass::Deploy();
}

void CWeaponCMag::PrimaryAttack( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

	// If my clip is empty (and I use clips) start reload
	if ( UsesClipsForAmmo1() && !m_iClip1 ) 
	{
		Reload();
		return;
	}

	// Bring us back to center view
	pPlayer->ViewPunchReset();

	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();

	m_bPreShoot = true;
	m_flShootTime = gpGlobals->curtime + GetFireDelay();
	
	WeaponSound( SPECIAL1 );
	SendWeaponAnim( GetPrimaryAttackActivity() );
	pPlayer->SetAnimation( PLAYER_ATTACK1 );
	ToGEPlayer(pPlayer)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
}

void CWeaponCMag::FireWeapon( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

	// If my clip is empty dry fire instead.
	if (!m_iClip1)
	{
		WeaponSound( EMPTY );
		return;
	}

	pPlayer->DoMuzzleFlash();

	FireBulletsInfo_t info;
	info.m_vecSrc	 = pPlayer->Weapon_ShootPosition( );
	info.m_vecDirShooting = pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );
	info.m_iShots = 1;

	WeaponSound(SINGLE);

	// Make sure we don't fire more than the amount in the clip
	if ( UsesClipsForAmmo1() )
	{
		info.m_iShots = MIN( info.m_iShots, m_iClip1 );
		m_iClip1 -= info.m_iShots;
	}
	else
	{
		info.m_iShots = MIN( info.m_iShots, pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) );
		pPlayer->RemoveAmmo( info.m_iShots, m_iPrimaryAmmoType );
	}

	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType;
	info.m_iTracerFreq = 1;

#if !defined( CLIENT_DLL )
	// Fire the bullets
	info.m_vecSpread = GetBulletSpread();
#else
	//!!!HACKHACK - what does the client want this function for? 
	info.m_vecSpread = GetActiveWeapon()->GetBulletSpread();
#endif // CLIENT_DLL

	pPlayer->FireBullets( info );

	RecordShotFired();

	//Add our view kick in
	AddViewKick();
}

void CWeaponCMag::HandleFireOnEmpty()
{
	m_bFireOnEmpty = true;

	if ( m_flNextEmptySoundTime <= gpGlobals->curtime )
	{
		SendWeaponAnim( ACT_VM_DRYFIRE );
		WeaponSound( SPECIAL1 );

		m_bPreShoot = true;
		m_flShootTime = gpGlobals->curtime + GetFireDelay();
		m_flNextEmptySoundTime = gpGlobals->curtime + GetFireRate();
		m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	}
}

void CWeaponCMag::ItemPreFrame( void )
{
	// If we are waiting for the hammer to pull back
	if (m_bPreShoot) {
		// If we have reached the shoot time, fire the weapon
		if (gpGlobals->curtime >= m_flShootTime) {
			FireWeapon();
			// We are no longer in pre-shoot mode!
			m_bPreShoot = false;
		}
	}

	BaseClass::ItemPreFrame();
}

void CWeaponCMag::ItemPostFrame( void )
{
	// We don't want to be able to fire this weapon as fast as we can so skip over
	// CGEWeaponPistol's definition
	CGEWeapon::ItemPostFrame();
}
