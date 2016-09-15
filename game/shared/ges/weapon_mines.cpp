///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: weapon_mines.cpp
// Description:
//      This file includes the weapon classes for mines, since a lot of the 
//		 methods will be the same, a base class was also used to help make
//		 things easier.
//
// Created On: 7/23/2008
// Created By: Jonathan White <killermonkey01@gmail.com> (Some code taken from Scott Lloyd)
/////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include "in_buttons.h"

#if defined( CLIENT_DLL )
	#include "c_ge_player.h"
#else
	#include "util.h"
	#include "eventqueue.h"
	#include "ge_player.h"
	#include "gemp_player.h"
	#include "npc_gebase.h"
	#include "grenade_mine.h"
#endif

#include "weapon_mines.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//------------------------------------------------------------------------------------------
//
// BASE MINE CLASS
//
IMPLEMENT_NETWORKCLASS_ALIASED( GEWeaponMine, DT_GEWeaponMine )

BEGIN_NETWORK_TABLE( CGEWeaponMine, DT_GEWeaponMine )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_iMineState ) ),
	RecvPropFloat( RECVINFO( m_flReleaseTime ) ),
	RecvPropBool( RECVINFO( m_bPreThrow ) ),
	RecvPropInt( RECVINFO(m_iCrateModelIndex) ),	
#else
	SendPropInt( SENDINFO( m_iMineState ) ),
	SendPropFloat( SENDINFO( m_flReleaseTime ) ),
	SendPropBool( SENDINFO( m_bPreThrow ) ),
	SendPropModelIndex( SENDINFO(m_iCrateModelIndex) ),
#endif
END_NETWORK_TABLE()

#if !defined( CLIENT_DLL )
BEGIN_DATADESC( CGEWeaponMine )
	DEFINE_FIELD( m_iMineState, FIELD_INTEGER ),
END_DATADESC()
#endif

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CGEWeaponMine )
	DEFINE_PRED_FIELD( m_bPreThrow, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iMineState, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flReleaseTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD_TOL( m_flReleaseTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),
	DEFINE_PRED_FIELD( m_iCrateModelIndex, FIELD_INTEGER, FTYPEDESC_INSENDTABLE | FTYPEDESC_MODELINDEX ),
END_PREDICTION_DATA()
#endif

CGEWeaponMine::CGEWeaponMine( void )
{
	m_bPreThrow = false;
	m_flReleaseTime = 0;
	m_iMineState = GE_MINE_THROW;
	// NPC Ranging
	m_fMaxRange1 = 700;
}

void CGEWeaponMine::Precache( void )
{
	PrecacheScriptSound( "weapon_mines.Throw" );

	BaseClass::Precache();
	m_iCrateModelIndex = PrecacheModel( BABYCRATE_MODEL );
	m_iWorldModelIndex = PrecacheModel( BaseClass::GetWorldModel() );
}

#ifdef CLIENT_DLL
int C_GEWeaponMine::GetWorldModelIndex( void )
{
	if ( GetOwner() )
		return m_iWorldModelIndex;
	else
		return m_iCrateModelIndex;
}
#endif

const char *CGEWeaponMine::GetWorldModel( void ) const
{
	if ( GetOwner() )
		return BaseClass::GetWorldModel();
	else
		return BABYCRATE_MODEL;
}

bool CGEWeaponMine::Deploy( void )
{
	// Reset our variables
	m_bPreThrow = false;
	m_flReleaseTime = 0;
	m_iMineState = GE_MINE_THROW;

	return BaseClass::Deploy();
}

void CGEWeaponMine::Drop( const Vector &vecVelocity )
{
	BaseClass::Drop( vecVelocity );

	SetAbsOrigin( GetAbsOrigin() + Vector(0,0,10) );

#ifdef GAME_DLL
	m_nSkin = AMMOCRATE_SKIN_MINES;
#endif
}

void CGEWeaponMine::PrimaryAttack( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
		return;

	if ( pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
		return;

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + GetFireRate();

	m_bPreThrow = true;
	m_flReleaseTime = gpGlobals->curtime + GetFireDelay();
	
	SendWeaponAnim( GetPrimaryAttackActivity() );
	pPlayer->SetAnimation( PLAYER_ATTACK1 );
	ToGEPlayer(pPlayer)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
}

#ifdef GAME_DLL
void CGEWeaponMine::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles )
{
	if ( pOperator->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
		return;

	ThrowMine();
}
#endif

void CGEWeaponMine::ItemPreFrame( void )
{
#ifdef GAME_DLL
	// If we are waiting for the mine to be spawned
	if ( m_bPreThrow )
	{
		// If we have reached the release time call the mine spawn
		if ( gpGlobals->curtime >= m_flReleaseTime )
		{
			ThrowMine();
			m_bPreThrow = false;
		}
	}
#endif

	BaseClass::ItemPreFrame();
}

void CGEWeaponMine::ItemPostFrame( void )
{
	// Insert code here to determine what state the mine is in and call the appropriate animation
	BaseClass::ItemPostFrame();
}

void CGEWeaponMine::ThrowMine( void )
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if ( !pOwner )
		return;

	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );

#ifdef GAME_DLL
	//Create a mine
	Vector	vecSrc = pOwner->Weapon_ShootPosition();

	Vector	vForward, vRight, vUp;
	AngleVectors( pOwner->EyeAngles(), &vForward, &vRight, &vUp );
	
	Vector vecThrow;
	pOwner->GetVelocity( &vecThrow, NULL );
	vecThrow += vForward * 600 + vUp * 80; //Pretty close to original throwing behavior.
	
	QAngle angAiming;
	VectorAngles( vecThrow, angAiming );

	// Convert us into a bot player :-D
	if ( pOwner->IsNPC() )
	{
		CNPC_GEBase *pNPC = (CNPC_GEBase*) pOwner;
		if ( pNPC->GetBotPlayer() )
			pOwner = pNPC->GetBotPlayer();
	}

	int iSeed = CBaseEntity::GetPredictionRandomSeed();
	int randarray[3];
	int rot1, rot2;

	for (int i = 0; i < 3; i++)
	{
		RandomSeed(iSeed + i * 7);
		randarray[i] = rand() % 4 + 4;
	}

	// clip and square randarray values to give more diversity to rotations.
	// Possible values are 16, 25, 36, 49.
	// randarray[2] is used to flip signs as just subtracting off the average value to make it the x intercept will not work in this case.
	rot1 = randarray[0] * randarray[0] * 10 * (randarray[2] > 3 ? 1 : -1);
	rot2 = randarray[1] * randarray[1] * 10 * (randarray[2] % 2 == 0 ? 1 : -1);

	CGEMine *pMine = (CGEMine *)CBaseEntity::Create( "npc_mine", vecSrc, angAiming, NULL );
	float timescale = phys_timescale.GetFloat();


	pMine->SetThrower( pOwner );
	pMine->SetOwnerEntity( pOwner );
	pMine->SetSourceWeapon( this );
	pMine->ApplyAbsVelocityImpulse( vecThrow * timescale );
	pMine->SetLocalAngularVelocity( QAngle(rot1, rot2, 0) * timescale );
	pMine->SetMineType( GetWeaponID() );
	pMine->SetDamage( GetGEWpnData().m_iDamage );
	pMine->SetDamageRadius( GetGEWpnData().m_flDamageRadius );

	if ( timescale != 1 )
		pMine->SetGravity( timescale*timescale );

	// Tell the owner what we threw to implement anti-spamming
	if ( pOwner->IsPlayer() )
		ToGEMPPlayer( pOwner )->AddThrownObject( pMine );
#endif
}

//------------------------------------------------------------------------------------------
//
// REMOTE MINE CLASS
//
LINK_ENTITY_TO_CLASS( weapon_remotemine, CWeaponRemoteMine );
PRECACHE_WEAPON_REGISTER( weapon_remotemine );

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponRemoteMine, DT_WeaponRemoteMine )

BEGIN_NETWORK_TABLE( CWeaponRemoteMine, DT_WeaponRemoteMine )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bWatchDeployed ) ),
//	RecvPropInt( RECVINFO(m_iWatchModelIndex) ),
#else
	SendPropBool( SENDINFO( m_bWatchDeployed ) ),
//	SendPropModelIndex( SENDINFO(m_iWatchModelIndex) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponRemoteMine )
	DEFINE_PRED_FIELD( m_bWatchDeployed, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
//	DEFINE_PRED_FIELD( m_iWatchModelIndex, FIELD_INTEGER, FTYPEDESC_INSENDTABLE | FTYPEDESC_MODELINDEX ),
END_PREDICTION_DATA()
#endif

#if !defined( CLIENT_DLL )
BEGIN_DATADESC( CWeaponRemoteMine )
	DEFINE_FIELD( m_bWatchDeployed, FIELD_BOOLEAN ),
END_DATADESC()
#endif

acttable_t CWeaponRemoteMine::m_acttable_mine[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_GES_IDLE_MINE,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_SLAM,			false },

	{ ACT_MP_RUN,						ACT_GES_RUN_MINE,					false },
	{ ACT_MP_WALK,						ACT_GES_WALK_MINE,					false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_SLAM,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_MINE,	true },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_MINE,	true },

	{ ACT_GES_DRAW,						ACT_GES_GESTURE_DRAW_MINE,			false },

	{ ACT_MP_JUMP,						ACT_GES_JUMP_MINE,					false },
	{ ACT_GES_CJUMP,					ACT_GES_CJUMP_MINE,					false },
};

acttable_t CWeaponRemoteMine::m_acttable_watch[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_GES_IDLE_WATCH,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_SLAM,			false },

	{ ACT_MP_RUN,						ACT_GES_RUN_WATCH,					false },
	{ ACT_MP_WALK,						ACT_GES_WALK_WATCH,					false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_SLAM,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_WATCH,	true },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_WATCH,	true },

	{ ACT_MP_JUMP,						ACT_GES_JUMP_WATCH,					false },
	{ ACT_GES_CJUMP,					ACT_GES_CJUMP_WATCH,				false },
};

// Explicitly define these so we can switch our activity table based on the state of the weapon
acttable_t *CWeaponRemoteMine::ActivityList( void )
{ 
	if ( m_bWatchDeployed )
		return m_acttable_watch;
	else
		return m_acttable_mine;
}
int CWeaponRemoteMine::ActivityListCount( void )
{
	if ( m_bWatchDeployed )
		return ARRAYSIZE(m_acttable_watch);
	else
		return ARRAYSIZE(m_acttable_mine);
}

void CWeaponRemoteMine::Precache( void )
{
	PrecacheModel( "models/weapons/mines/v_remotemine.mdl" );
	PrecacheModel( "models/weapons/mines/w_mine.mdl" );

	PrecacheMaterial( "sprites/hud/weaponicons/mine_remote" );
	PrecacheMaterial( "sprites/hud/ammoicons/ammo_remotemine" );

	PrecacheScriptSound( "weapon_mines.WatchBeep" );

//	m_iWatchModelIndex = CBaseEntity::PrecacheModel( "models/weapons/mines/w_watch.mdl" );
	BaseClass::Precache();
}

#ifdef CLIENT_DLL
int C_WeaponRemoteMine::GetWorldModelIndex( void )
{
	if ( GetOwner() && GetOwner()->IsPlayer() )
	{
		if ( m_bWatchDeployed )
			return NULL;
		else
			return m_iWorldModelIndex;
	}
	else
	{
		return BaseClass::GetWorldModelIndex();
	}
}
#endif

void CWeaponRemoteMine::Spawn( void )
{
	BaseClass::Spawn();
	m_nSkin = FAMILY_GROUP_REMOTE;
	m_bWatchDeployed = false;
	m_bFiresUnderwater = true;
#ifdef GAME_DLL
	m_bSwitchOnBlow = false;
#endif
}

void CWeaponRemoteMine::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip( pOwner );

	m_nSkin = FAMILY_GROUP_REMOTE;
	m_bWatchDeployed = false;
#ifdef GAME_DLL
	m_bSwitchOnBlow = false;
#endif
}

void CWeaponRemoteMine::Drop( const Vector &vecVelocity )
{
	BaseClass::Drop( vecVelocity );
	m_bWatchDeployed = false;
}

void CWeaponRemoteMine::PrimaryAttack( void )
{
	if ( m_bWatchDeployed )
	{
		SendWeaponAnim( ACT_VM_WATCH_DETONATE );
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();

		// Emit the watch beep sound
		WeaponSound( SPECIAL1 );

#ifdef GAME_DLL
		int minesblown = 0;
		CGEMine *pMine = static_cast<CGEMine*>(gEntList.FindEntityByClassname( NULL, "npc_mine_remote" ));
		while ( pMine != NULL)
		{
			if ( pMine->GetThrower() == GetOwner() )
			{
				g_EventQueue.AddEvent( pMine, "Explode", 0.30, GetOwner(), pMine );
				minesblown++;
			}
			pMine = static_cast<CGEMine*>(gEntList.FindEntityByClassname( pMine, "npc_mine_remote" ));
		}

		// if we actually blew something up send us to our mines again so we can start throwing!
		if ( minesblown )
			m_bSwitchOnBlow = true;
#endif
	} else {
		BaseClass::PrimaryAttack();
	}
}

void CWeaponRemoteMine::SecondaryAttack( void )
{
	// If we are showing the watch and have no mines left don't switch back!
	CBaseCombatCharacter *pOwner = GetOwner();
	if ( !pOwner )
		return;

	// Make sure if we run out of ammo we go straight to the watch
	if ( !pOwner->GetAmmoCount( GetPrimaryAmmoType() ) )
	{
		if ( m_bWatchDeployed )
			return;

		SendWeaponAnim( ACT_VM_WATCH_DRAW );
		m_bWatchDeployed = !m_bWatchDeployed;
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();
		return;
	}

	ToggleWatch();
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();
}

void CWeaponRemoteMine::ToggleWatch( void )
{
	if ( m_bWatchDeployed ) {
		m_bFiresUnderwater = false;
		SendWeaponAnim( ACT_VM_WATCH_TO_MINE );
	} else {
		// Make sure we can detonate already thrown mines underwater
		m_bFiresUnderwater = true;
		SendWeaponAnim( ACT_VM_MINE_TO_WATCH );
	}

	m_bWatchDeployed = !m_bWatchDeployed;
}

void CWeaponRemoteMine::WeaponIdle( void )
{
	if ( HasWeaponIdleTimeElapsed() )
	{
		if ( m_bWatchDeployed )
			SendWeaponAnim( ACT_VM_WATCH_IDLE );
		else
			SendWeaponAnim( ACT_VM_IDLE );
	}
}

void CWeaponRemoteMine::ItemPreFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner == NULL )
		return;

	if ( (pOwner->m_nButtons & IN_ATTACK) && (pOwner->m_nButtons & IN_ATTACK2) )
	{
		// This makes sure we don't enter watch mode or throw another mine
		m_flNextPrimaryAttack = m_flNextSecondaryAttack = gpGlobals->curtime + 0.30f;

#ifdef GAME_DLL
		int detonated = 0;
		CGEMine *pMine = (CGEMine*) gEntList.FindEntityByClassname( NULL, "npc_mine_remote" );
		while ( pMine )
		{
			if ( !pMine->m_bPreExplode && pMine->GetThrower() == GetOwner() )
			{
				g_EventQueue.AddEvent( pMine, "Explode", 0.30, GetOwner(), GetOwner() );
				pMine->m_bPreExplode = true;
				detonated++;
			}

			pMine = (CGEMine*) gEntList.FindEntityByClassname( pMine, "npc_mine_remote" );
		}

		
		if ( detonated > 0 )
		{
			CBasePlayer *pPred = GetPredictionPlayer();
			SetPredictionPlayer( NULL );
			WeaponSound( SPECIAL1 );
			SetPredictionPlayer( pPred );
		}
#endif
	}

	// If we have run out of mines switch to the watch
	if ( !pOwner->GetAmmoCount( GetPrimaryAmmoType() ) && !m_bWatchDeployed && m_flNextSecondaryAttack <= gpGlobals->curtime )
		SecondaryAttack();

#ifdef GAME_DLL
	// Successful blow? Switch back to mines after the alloted time!
	if ( m_bSwitchOnBlow && m_flNextSecondaryAttack <= gpGlobals->curtime )
	{
		m_bSwitchOnBlow = false;
		SecondaryAttack();
	}
#endif

	BaseClass::ItemPreFrame();
}

void CWeaponRemoteMine::ItemPostFrame( void )
{
	// Insert code to check our states then call the baseclass
	// If we are in watch state make sure our watch is deployed
	// otherwise make sure our mine is deployed
	BaseClass::ItemPostFrame();
}

void CWeaponRemoteMine::HandleFireOnEmpty( void )
{
	// If we are showing the watch we don't want to hold up the show!
	if ( m_bWatchDeployed )
		PrimaryAttack();
	else
		BaseClass::HandleFireOnEmpty();
}

Activity CWeaponRemoteMine::GetDrawActivity( void )
{
	if ( m_bWatchDeployed )
		return ACT_VM_WATCH_DRAW;
	else
		return BaseClass::GetDrawActivity();
}

//------------------------------------------------------------------------------------------
//
// PROXIMITY MINE CLASS
//
LINK_ENTITY_TO_CLASS( weapon_proximitymine, CWeaponProximityMine );
PRECACHE_WEAPON_REGISTER( weapon_proximitymine );

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponProximityMine, DT_WeaponProximityMine )

BEGIN_NETWORK_TABLE( CWeaponProximityMine, DT_WeaponProximityMine )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponProximityMine )
END_PREDICTION_DATA()
#endif

#if !defined( CLIENT_DLL )
BEGIN_DATADESC( CWeaponProximityMine )
END_DATADESC()
#endif

acttable_t CWeaponProximityMine::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_GES_IDLE_MINE,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_SLAM,			false },

	{ ACT_MP_RUN,						ACT_GES_RUN_MINE,					false },
	{ ACT_MP_WALK,						ACT_GES_WALK_MINE,					false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_SLAM,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_MINE,	true },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_MINE,	true },

	{ ACT_GES_DRAW,						ACT_GES_GESTURE_DRAW_MINE,			false },

	{ ACT_MP_JUMP,						ACT_GES_JUMP_MINE,					false },
	{ ACT_GES_CJUMP,					ACT_GES_CJUMP_MINE,					false },
};
IMPLEMENT_ACTTABLE( CWeaponProximityMine );

void CWeaponProximityMine::Spawn( void )
{
	BaseClass::Spawn();
	m_nSkin = FAMILY_GROUP_PROXIMITY;
}

void CWeaponProximityMine::Precache(void)
{
	PrecacheModel("models/weapons/mines/v_proximitymine.mdl");
	PrecacheModel("models/weapons/mines/w_mine.mdl");

	PrecacheMaterial("sprites/hud/weaponicons/mine_proximity");
	PrecacheMaterial("sprites/hud/ammoicons/ammo_proximitymine");

	PrecacheScriptSound("weapon_mines.Throw");

	BaseClass::Precache();
}

void CWeaponProximityMine::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip( pOwner );
	m_nSkin = FAMILY_GROUP_PROXIMITY;
}

//------------------------------------------------------------------------------------------
//
// TIMED MINE CLASS
//
LINK_ENTITY_TO_CLASS( weapon_timedmine, CWeaponTimedMine );
PRECACHE_WEAPON_REGISTER( weapon_timedmine );

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponTimedMine, DT_WeaponTimedMine )

BEGIN_NETWORK_TABLE( CWeaponTimedMine, DT_WeaponTimedMine )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponTimedMine )
END_PREDICTION_DATA()
#endif

#if !defined( CLIENT_DLL )
BEGIN_DATADESC( CWeaponTimedMine )
END_DATADESC()
#endif

acttable_t CWeaponTimedMine::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_GES_IDLE_MINE,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_SLAM,			false },

	{ ACT_MP_RUN,						ACT_GES_RUN_MINE,					false },
	{ ACT_MP_WALK,						ACT_GES_WALK_MINE,					false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_SLAM,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_MINE,	true },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_MINE,	true },

	{ ACT_GES_DRAW,						ACT_GES_GESTURE_DRAW_MINE,			false },

	{ ACT_MP_JUMP,						ACT_GES_JUMP_MINE,					false },
	{ ACT_GES_CJUMP,					ACT_GES_CJUMP_MINE,					false },
};
IMPLEMENT_ACTTABLE( CWeaponTimedMine );

void CWeaponTimedMine::Spawn( void )
{
	BaseClass::Spawn();
	m_nSkin = FAMILY_GROUP_TIMED;
}

void CWeaponTimedMine::Precache(void)
{
	PrecacheModel("models/weapons/mines/v_timedmine.mdl");
	PrecacheModel("models/weapons/mines/w_mine.mdl");

	PrecacheMaterial("sprites/hud/weaponicons/mine_timed");
	PrecacheMaterial("sprites/hud/ammoicons/ammo_timedmine");

	PrecacheScriptSound("weapon_mines.Throw");

	BaseClass::Precache();
}

void CWeaponTimedMine::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip( pOwner );
	m_nSkin = FAMILY_GROUP_TIMED;
}
