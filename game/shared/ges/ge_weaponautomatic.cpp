///////////// Copyright © 2008, Goldeneye Source. All rights reserved. /////////////
// 
// File: ge_weaponautomatic.cpp
// Description:
//      Implementation of ge_weaponautomatic.h base class.
//
// Created On: 3/14/2008 13:45
// Created By: Mario Cacciatore
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_ge_player.h"
#else
	#include "ai_basenpc.h"
	#include "ge_player.h"
#endif

#include "ge_weaponautomatic.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( GEWeaponAutomatic, DT_GEWeaponAutomatic )

BEGIN_NETWORK_TABLE( CGEWeaponAutomatic, DT_GEWeaponAutomatic )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_nNumShotsFired ) ),
#else
	SendPropInt( SENDINFO( m_nNumShotsFired ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CGEWeaponAutomatic )
	DEFINE_PRED_FIELD( m_nNumShotsFired, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

BEGIN_DATADESC( CGEWeaponAutomatic )
	DEFINE_FIELD( m_nNumShotsFired, FIELD_INTEGER ),
END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CGEWeaponAutomatic::CGEWeaponAutomatic( void )
{
	m_fMinRange1		= 24;
	m_fMaxRange1		= 3000;
	m_fMinRange2		= 24;
	m_fMaxRange2		= 3000;

	m_bFiresUnderwater	= false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CGEWeaponAutomatic::PrimaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;
	
	// Abort here to handle burst and auto fire modes
	if ( (UsesClipsForAmmo1() && m_iClip1 == 0) || ( !UsesClipsForAmmo1() && !pPlayer->GetAmmoCount(m_iPrimaryAmmoType) ) )
		return;

	pPlayer->DoMuzzleFlash();

	// To make the firing framerate independent, we may have to fire more than one bullet here on low-framerate systems, 
	// especially if the weapon we're firing has a really fast rate of fire.
	int iBulletsToFire = 0;
	float fireRate = GetFireRate();

	while ( m_flNextPrimaryAttack <= gpGlobals->curtime )
	{
		// MUST call sound before removing a round from the clip of a CHLMachineGun
		WeaponSound(SINGLE, m_flNextPrimaryAttack);
		m_flNextPrimaryAttack = m_flNextPrimaryAttack + fireRate;
		iBulletsToFire++;
		m_nNumShotsFired++;
	}

	// Add an accuracy penalty which can move past our maximum penalty time if we're really spastic
	m_flAccuracyPenalty += GetFireRate() * iBulletsToFire;

	// Make sure we don't fire more than the amount in the clip, if this weapon uses clips
	if ( UsesClipsForAmmo1() )
	{
		if ( iBulletsToFire > m_iClip1 )
			iBulletsToFire = m_iClip1;
		m_iClip1 -= iBulletsToFire;
	}
		// Fire the bullets
	FireBulletsInfo_t info;
	info.m_iShots = iBulletsToFire;
	info.m_vecSrc = pPlayer->Weapon_ShootPosition( );
	info.m_vecDirShooting = pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	info.m_vecSpread = GetBulletSpread();
	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType;
	info.m_iTracerFreq = 2;
	pPlayer->FireBullets( info );

	// Record our shots
	RecordShotFired( iBulletsToFire );

	//Factor in the view kick
	AddViewKick();
	
	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0); 
	}

	SendWeaponAnim( GetPrimaryAttackActivity() );
	pPlayer->SetAnimation( PLAYER_ATTACK1 );
	ToGEPlayer(pPlayer)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
}

void CGEWeaponAutomatic::DoMachineGunKick( CBasePlayer *pPlayer, float dampEasy, float maxVerticleKickAngle, float fireDurationTime, float slideLimitTime )
{
	#define	KICK_MIN_X			0.2f	//Degrees
	#define	KICK_MIN_Y			0.2f	//Degrees
	#define	KICK_MIN_Z			0.1f	//Degrees

	QAngle vecScratch;
	int iSeed = CBaseEntity::GetPredictionRandomSeed() & 255;
	
	//Find how far into our accuracy degradation we are
	float duration	= ( fireDurationTime > slideLimitTime ) ? slideLimitTime : fireDurationTime;
	float kickPerc = duration / slideLimitTime;

	// do this to get a hard discontinuity, clear out anything under 10 degrees punch
	pPlayer->ViewPunchReset( 10 );

	//Apply this to the view angles as well
	vecScratch.x = -( KICK_MIN_X + ( maxVerticleKickAngle * kickPerc ) );
	vecScratch.y = -( KICK_MIN_Y + ( maxVerticleKickAngle * kickPerc ) ) / 3;
	vecScratch.z = KICK_MIN_Z + ( maxVerticleKickAngle * kickPerc ) / 8;

	RandomSeed( iSeed );

	//Wibble left and right
	if ( RandomInt( -1, 1 ) >= 0 )
		vecScratch.y *= -1;

	iSeed++;

	//Wobble up and down
	if ( RandomInt( -1, 1 ) >= 0 )
		vecScratch.z *= -1;

	//Clip this to our desired min/max
	UTIL_ClipPunchAngleOffset( vecScratch, pPlayer->m_Local.m_vecPunchAngle, QAngle( 24.0f, 3.0f, 1.0f ) );

	//Add it to the view punch
	// NOTE: 0.5 is just tuned to match the old effect before the punch became simulated
	pPlayer->ViewPunch( vecScratch * 0.5 );
}

bool CGEWeaponAutomatic::Deploy( void )
{
	m_nNumShotsFired = 0;

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: Make enough sound events to fill the estimated think interval
// returns: number of shots needed
//-----------------------------------------------------------------------------
int CGEWeaponAutomatic::WeaponSoundRealtime( WeaponSound_t shoot_type )
{
	int numBullets = 0;

	// ran out of time, clamp to current
	if (m_flNextSoundTime < gpGlobals->curtime)
	{
		m_flNextSoundTime = gpGlobals->curtime;
	}

	// make enough sound events to fill up the next estimated think interval
	float dt = clamp( m_flAnimTime - m_flPrevAnimTime, 0, 0.2 );
	if (m_flNextSoundTime < gpGlobals->curtime + dt)
	{
		WeaponSound( SINGLE_NPC, m_flNextSoundTime );
		m_flNextSoundTime += GetFireRate();
		numBullets++;
	}
	if (m_flNextSoundTime < gpGlobals->curtime + dt)
	{
		WeaponSound( SINGLE_NPC, m_flNextSoundTime );
		m_flNextSoundTime += GetFireRate();
		numBullets++;
	}

	return numBullets;
}

//-----------------------------------------------------------------------------
// Purpose: Allows firing as fast as button is pressed
//-----------------------------------------------------------------------------
void CGEWeaponAutomatic::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	// Debounce the recoiling counter
	if ( ( pOwner->m_nButtons & IN_ATTACK ) == false )
	{
		m_nNumShotsFired = 0;
	}

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CGEWeaponAutomatic::Reload( void )
{
	bool bRet = BaseClass::Reload();

	if ( bRet )
	{
		m_nNumShotsFired = 0;
		m_flAccuracyPenalty = 0.0f;
	}

	return bRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGEWeaponAutomatic::AddViewKick( void )
{
	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
		return;

	QAngle	viewPunch;

	viewPunch.x = SharedRandomFloat( "geautopax", GetGEWpnData().Kick.x_min, GetGEWpnData().Kick.x_max );
	viewPunch.y = SharedRandomFloat( "geautopay", GetGEWpnData().Kick.y_min, GetGEWpnData().Kick.y_max );
	viewPunch.z = 0.0f;

	DoMachineGunKick( pPlayer, 0.5f, viewPunch.y, m_fFireDuration, viewPunch.x );
}