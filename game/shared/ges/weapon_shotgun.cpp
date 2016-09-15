///////////// Copyright © 2008, Goldeneye Source. All rights reserved. /////////////
// 
// File: weapon_shotgun.cpp
// Description:
//      Implementation of weapon Shotgun.
//
// Created On: 11/8/2008
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
#define CWeaponShotgun C_WeaponShotgun
#endif

//-----------------------------------------------------------------------------
// CWeaponShotgun
//-----------------------------------------------------------------------------

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponShotgun, DT_WeaponShotgun )

BEGIN_NETWORK_TABLE( CWeaponShotgun, DT_WeaponShotgun )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponShotgun )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_shotgun, CWeaponShotgun );
PRECACHE_WEAPON_REGISTER( weapon_shotgun );

acttable_t CWeaponShotgun::m_acttable[] = 
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
IMPLEMENT_ACTTABLE( CWeaponShotgun );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponShotgun::CWeaponShotgun( void )
{
	// NPC Ranging
	m_fMaxRange1 = 1024;
	m_iShellBodyGroup[0] = -1;
}

void CWeaponShotgun::Precache(void)
{
	PrecacheModel("models/Weapons/shotgun/v_shotgun.mdl");
	PrecacheModel("models/weapons/shotgun/w_shotgun.mdl");

	PrecacheMaterial("sprites/hud/weaponicons/shotgun");
	PrecacheMaterial("sprites/hud/ammoicons/ammo_buckshot");

	PrecacheScriptSound("Weapon.Shotgun_Reload");
	PrecacheScriptSound("Weapon_pshotgun.Single");
	PrecacheScriptSound("Weapon_pshotgun.NPC_Single");
	PrecacheScriptSound("Weapon.Special1");
	PrecacheScriptSound("Weapon.Special2");

	BaseClass::Precache();
}

void CWeaponShotgun::PrimaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
		return;

	// MUST call sound before removing a round from the clip of a CMachineGun
	WeaponSound(SINGLE);

	pPlayer->DoMuzzleFlash();

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	ToGEPlayer(pPlayer)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	// Don't fire again until our ROF expires
	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	m_flSoonestPrimaryAttack = gpGlobals->curtime + GetClickFireRate();
	m_iClip1 -= 1;

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector	vecSrc		= pPlayer->Weapon_ShootPosition( );
	Vector	vecAiming	= pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );	

//	FireBulletsInfo_t info( 5, vecSrc, vecAiming, pGEPlayer->GetAttackSpread(this), MAX_TRACE_LENGTH, m_iPrimaryAmmoType );
//	info.m_pAttacker = pPlayer;

	// Knock the player's view around
	AddViewKick();

	RecordShotFired();

	// Fire the bullets, and force the first shot to be perfectly accuracy
	PrepareFireBullets(5, pPlayer, vecSrc, vecAiming, true);

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0); 
	}
}

#ifdef GAME_DLL
void CWeaponShotgun::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles )
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

	PrepareFireBullets(5, pOperator, vecShootOrigin, vecShootDir, false);
}
#endif

void CWeaponShotgun::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle	viewPunch;

	viewPunch.x = SharedRandomFloat( "Shotgunpax", GetGEWpnData().Kick.x_min, GetGEWpnData().Kick.x_max );
	viewPunch.y = SharedRandomFloat( "Shotgunpay", GetGEWpnData().Kick.y_min, GetGEWpnData().Kick.y_max );
	viewPunch.z = 0.0f;

	//Add it to the view punch
	pPlayer->ViewPunch( viewPunch );
}


void CWeaponShotgun::OnReloadOffscreen(void)
{
	BaseClass::OnReloadOffscreen();

	CBaseCombatCharacter *pOwner = GetOwner();
	if (!pOwner)
		return;

	if (pOwner->GetActiveWeapon() == this)
		CalculateShellVis(true); // Check shell count when we reload, but adjust for the amount of ammo we're about to take out of the clip.
}


bool CWeaponShotgun::Deploy(void)
{
	bool success = BaseClass::Deploy();

	// We have to do this here because "GetBodygroupFromName" relies on the weapon having a viewmodel.
	// It's debatable if this is really how we should go about it, instead of just a table of integers for the bodygroups.
	// but this does protect us from someone recompiling the model with a different order of bodygroups.

	if (m_iShellBodyGroup[0] == -1) // We only need to check shell1 because we assign all of them at once.
	{
		m_iShellBodyGroup[0] = GetBodygroupFromName("shell1");
		m_iShellBodyGroup[1] = GetBodygroupFromName("shell2");
		m_iShellBodyGroup[2] = GetBodygroupFromName("shell3");
		m_iShellBodyGroup[3] = GetBodygroupFromName("shell4");
		m_iShellBodyGroup[4] = GetBodygroupFromName("shell5");
	}

	CalculateShellVis(false); // Also check shell count when we draw the weapon.

	return success;
}


void CWeaponShotgun::CalculateShellVis(bool fillclip)
{
	// Shells on the shotgun viewmodel will be removed if the reserve ammo is less than 5.  
	// Each time we draw and reload, we check to see if this is the case, and if it is we change bodygroups accordingly.

	CBaseCombatCharacter *pOwner = GetOwner();

	if (!pOwner)
		return;

	int reserveammo = pOwner->GetAmmoCount(m_iPrimaryAmmoType);

	if (fillclip) //If we need to fill the clip, just subtract the difference between the max capacity and the current value.
		reserveammo -= GetMaxClip1() - m_iClip1; //This can be negative, it won't change the comparision results.

	for (int i = 4; i >= 0; i--)
	{
		if (reserveammo > i)
			SwitchBodygroup(m_iShellBodyGroup[i], 0);
		else
			SwitchBodygroup(m_iShellBodyGroup[i], 1);
	}
}