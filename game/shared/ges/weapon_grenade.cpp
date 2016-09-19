///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: weapon_grenade.cpp
// Description:
//      Implements the Grenade (bounce type) with throw scaling based on time primed
//
// Created On: 9/29/2008
// Created By: Jonathan White <killermonkey01@gmail.com> (Mostly copied from HL2)
/////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include "in_buttons.h"
#include "ge_weapon.h"
#include "ge_shareddefs.h"

#if defined( CLIENT_DLL )
	#include "c_ge_player.h"
	#include "c_gemp_player.h"
#else
	#include "util.h"
	#include "gemp_player.h"
	#include "npc_gebase.h"
	#include "grenade_ge.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if defined( CLIENT_DLL )
	#define CGEWeaponGrenade	C_GEWeaponGrenade
#endif

#define GE_GRENADE_THROW_FORCE	750.0f
#define GE_GRENADE_FUSE_TIME	4.0f
#define GE_GRENADE_SPAWN_DELAY	0.1f;
#define GE_GRENADE_PIN_DELAY	0.1f;
#define GE_GRENADE_IDLE_DELAY	0.8f; // Time to wait after throwing a grenade to going idle

//------------------------------------------------------------------------------------------
//
// GE Grenade weapon class
//
class CGEWeaponGrenade : public CGEWeapon
{
public:
	DECLARE_CLASS( CGEWeaponGrenade, CGEWeapon );

#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif
	
	DECLARE_ACTTABLE();

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	
	CGEWeaponGrenade( void );

	virtual void Precache( void );

	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_GRENADE; }

	virtual bool IsExplosiveWeapon() { return true; }
	virtual bool IsThrownWeapon()	 { return true; }

	virtual void PrimaryAttack( void );
	virtual void ItemPostFrame( void );

	virtual void Equip( CBaseCombatCharacter *pOwner );
	virtual bool Deploy( void );
	virtual void Drop( const Vector &vecVelocity );
	virtual void PreOwnerDeath(); //Fired when owner dies but before anything else.

	virtual const char	*GetWorldModel( void ) const;

	virtual void WeaponIdle( void );

	bool	IsPrimed( void ) { return m_bPreThrow;	}

#ifdef CLIENT_DLL
	virtual int  GetWorldModelIndex( void );
#else
	void FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles );
#endif

protected:
	CNetworkVar( int, m_iCrateModelIndex );

private:
	// check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
	void	CheckThrowPosition( const Vector &vecEye, Vector &vecSrc );
	void	ThrowGrenade( float throwforce );
	void	ExplodeInHand( void );
	
	CNetworkVar( bool, m_bDrawNext );
	CNetworkVar( bool, m_bPreThrow );
	CNetworkVar( bool, m_bSpawnWait );
	CNetworkVar( float, m_flNextReleaseTime );
	CNetworkVar( float, m_flPrimedTime );
	CNetworkVar( float, m_flGrenadeSpawnTime );

	CGEWeaponGrenade( const CGEWeaponGrenade & );
};

acttable_t	CGEWeaponGrenade::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_GES_IDLE_GRENADE,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_GRENADE,			false },

	{ ACT_MP_RUN,						ACT_GES_RUN_GRENADE,					false },
	{ ACT_MP_WALK,						ACT_GES_WALK_GRENADE,					false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_GRENADE,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_GRENADE,	false },
	{ ACT_GES_ATTACK_RUN_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_GRENADE,	false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_GRENADE,	false },

	{ ACT_MP_RELOAD_STAND,				ACT_HL2MP_GESTURE_RELOAD_GRENADE,		false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_GRENADE,		false },

	{ ACT_MP_JUMP,						ACT_GES_JUMP_GRENADE,					false },
	{ ACT_GES_CJUMP,					ACT_GES_CJUMP_GRENADE,					false },
};
IMPLEMENT_ACTTABLE(CGEWeaponGrenade);

IMPLEMENT_NETWORKCLASS_ALIASED( GEWeaponGrenade, DT_GEWeaponGrenade )

BEGIN_NETWORK_TABLE( CGEWeaponGrenade, DT_GEWeaponGrenade )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bDrawNext ) ),
	RecvPropBool( RECVINFO( m_bPreThrow ) ),
	RecvPropBool( RECVINFO( m_bSpawnWait ) ),
	RecvPropFloat( RECVINFO( m_flNextReleaseTime ) ),
	RecvPropFloat( RECVINFO( m_flPrimedTime ) ),
	RecvPropFloat( RECVINFO( m_flGrenadeSpawnTime ) ),
	RecvPropInt( RECVINFO(m_iCrateModelIndex) ),
#else
	SendPropBool( SENDINFO( m_bDrawNext ) ),
	SendPropBool( SENDINFO( m_bPreThrow ) ),
	SendPropBool( SENDINFO( m_bSpawnWait ) ),
	SendPropFloat( SENDINFO( m_flNextReleaseTime ) ),
	SendPropFloat( SENDINFO( m_flPrimedTime ) ),
	SendPropFloat( SENDINFO( m_flGrenadeSpawnTime ) ),
	SendPropModelIndex( SENDINFO(m_iCrateModelIndex) ),
#endif
END_NETWORK_TABLE()

#if !defined( CLIENT_DLL )
BEGIN_DATADESC( CGEWeaponGrenade )
	DEFINE_FIELD( m_bDrawNext, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bPreThrow, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bSpawnWait, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flNextReleaseTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_flPrimedTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_flGrenadeSpawnTime, FIELD_FLOAT ),
END_DATADESC()
#endif

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CGEWeaponGrenade )
	DEFINE_PRED_FIELD( m_bDrawNext, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bPreThrow, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bSpawnWait, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flNextReleaseTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flPrimedTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flGrenadeSpawnTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_iCrateModelIndex, FIELD_INTEGER, FTYPEDESC_INSENDTABLE | FTYPEDESC_MODELINDEX ),

	DEFINE_PRED_FIELD_TOL( m_flNextReleaseTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),
	DEFINE_PRED_FIELD_TOL( m_flPrimedTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),
	DEFINE_PRED_FIELD_TOL( m_flGrenadeSpawnTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_grenade, CGEWeaponGrenade );
PRECACHE_WEAPON_REGISTER(weapon_grenade);

CGEWeaponGrenade::CGEWeaponGrenade( void )
{
	// NPC Ranging
	m_fMinRange1 = 200;
	m_fMaxRange1 = 1000;
}

void CGEWeaponGrenade::Precache( void )
{
	PrecacheModel("models/weapons/grenade/v_grenade.mdl");
	PrecacheModel("models/weapons/grenade/w_grenade.mdl");

	PrecacheMaterial("sprites/hud/ammoicons/ammo_grenade");

	PrecacheScriptSound("Weapon_mines.Throw");

	BaseClass::Precache();
	m_iCrateModelIndex = PrecacheModel( BABYCRATE_MODEL );
	m_iWorldModelIndex = PrecacheModel( BaseClass::GetWorldModel() );
}

#ifdef CLIENT_DLL
int C_GEWeaponGrenade::GetWorldModelIndex( void )
{
	if ( GetOwner() )
		return m_iWorldModelIndex;
	else
		return m_iCrateModelIndex;
}
#endif

void CGEWeaponGrenade::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip( pOwner );
	m_nSkin = 0;
}

bool CGEWeaponGrenade::Deploy( void )
{
	m_bPreThrow = false;
	m_bSpawnWait = false;
	m_bDrawNext = false;
	m_flGrenadeSpawnTime = -1;
	m_flNextReleaseTime = -1;
	m_flPrimedTime = -1;

	return BaseClass::Deploy();
}

void CGEWeaponGrenade::PreOwnerDeath()
{
	// Throw the grenade if the player was about to before they died.
	if (m_bSpawnWait || m_bPreThrow)
		ThrowGrenade(GE_GRENADE_THROW_FORCE/5);

	BaseClass::PreOwnerDeath();
}

void CGEWeaponGrenade::Drop( const Vector &vecVelocity )
{
	BaseClass::Drop( vecVelocity );

	SetAbsOrigin( GetAbsOrigin() + Vector(0,0,10) );

#ifdef GAME_DLL
	m_nSkin = AMMOCRATE_SKIN_GRENADES;
#endif
}

const char *CGEWeaponGrenade::GetWorldModel( void ) const
{
	if ( GetOwner() )
		return BaseClass::GetWorldModel();
	else
		return BABYCRATE_MODEL;
}

//-----------------------------------------------------------------------------
// Purpose: Prepare the grenade for throwing
//-----------------------------------------------------------------------------
void CGEWeaponGrenade::PrimaryAttack( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

	// Note that this is a primary attack and prepare the grenade attack to pause.
	m_bPreThrow = true;
	SendWeaponAnim( ACT_VM_GRENADE_PULLPIN );

	m_flNextReleaseTime = gpGlobals->curtime + SequenceDuration();
	m_flPrimedTime = gpGlobals->curtime + GE_GRENADE_PIN_DELAY;
	
	// Put both of these off indefinitely. We do not know how long
	// the player will hold the grenade.
	m_flTimeWeaponIdle = FLT_MAX;
	m_flNextPrimaryAttack = FLT_MAX;

	// If I'm now out of ammo, switch away
	if ( !HasPrimaryAmmo() )
		pPlayer->SwitchToNextBestWeapon( this );
}

#ifdef GAME_DLL
void CGEWeaponGrenade::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles )
{
	m_flPrimedTime = gpGlobals->curtime;
	ThrowGrenade(GE_GRENADE_THROW_FORCE);
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Checks to see two things...
//			1. Did the player release the throw button (if so throw a grenade!)
//			2. Did the grenade's fuse run out? (if so explode!)
//-----------------------------------------------------------------------------
void CGEWeaponGrenade::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if( m_bPreThrow )
	{
		// Wait until we are fully drawn back before releasing
		if ( pOwner && m_flNextReleaseTime < gpGlobals->curtime )
		{
			// The player released the attack button, initiate the throwing
			if( !(pOwner->m_nButtons & IN_ATTACK) )
			{
				// Since we are going to be showing the grenade being thrown we 
				// have to wait for the anim to play to that point
				SendWeaponAnim( ACT_VM_GRENADE_THROW1 );
				m_flGrenadeSpawnTime = gpGlobals->curtime + GE_GRENADE_SPAWN_DELAY;
				m_bSpawnWait = true;
				m_bPreThrow = false;
			}
			else if ( gpGlobals->curtime > m_flPrimedTime + GE_GRENADE_FUSE_TIME )
			{
				ExplodeInHand();
				m_bSpawnWait = false;
				m_bPreThrow = false;
			}
		}
	}
	else if ( m_bSpawnWait )
	{
		if ( m_flGrenadeSpawnTime < gpGlobals->curtime )
		{
			ThrowGrenade(GE_GRENADE_THROW_FORCE);

			// player "shoot" animation
			pOwner->SetAnimation( PLAYER_ATTACK1 );
			ToGEPlayer(pOwner)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

			m_bSpawnWait = false;
		}
	}

	BaseClass::ItemPostFrame();
}

// Make sure we aren't going to throw the grenade INTO something!
void CGEWeaponGrenade::CheckThrowPosition( const Vector &vecEye, Vector &vecSrc )
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if ( !pOwner )
		return;

	trace_t tr;
	UTIL_TraceHull( vecEye, vecSrc, -Vector(6,6,6), Vector(6,6,6), pOwner->PhysicsSolidMaskForEntity(), pOwner, pOwner->GetCollisionGroup(), &tr );
	
	if ( tr.DidHit() )
		vecSrc = tr.endpos;
}

//-----------------------------------------------------------------------------
// Purpose: Throw a primed grenade with timeleft of (gpGlobals->curtime - m_flPrimedTime)
//-----------------------------------------------------------------------------
void CGEWeaponGrenade::ThrowGrenade( float throwforce )
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if ( !pOwner )
		return;

	// Remove the grenade from our ammo pool
	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );

#ifndef CLIENT_DLL
	Vector	vecEye = pOwner->EyePosition();

	Vector	vForward, vRight;
	AngleVectors( pOwner->EyeAngles(), &vForward, &vRight, NULL );

	Vector vecSrc = vecEye + vForward * 18.0f + vRight * 8.0f;
	CheckThrowPosition( vecEye, vecSrc );
	vForward[2] += 0.1f;

	Vector vecThrow;
	pOwner->GetVelocity( &vecThrow, NULL );
	vecThrow += vForward * throwforce;

	// Convert us into a bot player :-D
	if ( pOwner->IsNPC() )
	{
		CNPC_GEBase *pNPC = (CNPC_GEBase*) pOwner;
		if ( pNPC->GetBotPlayer() )
			pOwner = pNPC->GetBotPlayer();
	}

	CGEGrenade *pGrenade = (CGEGrenade *)CBaseEntity::Create( "npc_grenade", vecSrc, vec3_angle, NULL );

	if ( pGrenade )
	{
		pGrenade->SetThrower( pOwner );
		pGrenade->SetOwnerEntity( pOwner );
		pGrenade->SetVelocity( vecThrow, AngularImpulse(600,random->RandomInt(-1000,1000),0) );

		pGrenade->SetDamage( GetGEWpnData().m_iDamage );
		pGrenade->SetDamageRadius( GetGEWpnData().m_flDamageRadius );
		pGrenade->SetSourceWeapon(this);
		
		if (throwforce == GE_GRENADE_THROW_FORCE / 5) // For acheivement tracking.
			pGrenade->m_bDroppedOnDeath = true;

		// The timer is whatever is left over from the primed time + our fuse minus our 
		// current time to give us an absolute time in seconds
		pGrenade->SetTimer( (m_flPrimedTime + GE_GRENADE_FUSE_TIME) - gpGlobals->curtime );

		// Tell the owner what we threw to implement anti-spamming
		if ( pOwner->IsPlayer() )
			ToGEMPPlayer(pOwner)->AddThrownObject( pGrenade );
	}
#endif

	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	m_flTimeWeaponIdle = gpGlobals->curtime + GE_GRENADE_IDLE_DELAY;
	m_bDrawNext = true;

	WeaponSound( SINGLE );
}

//-----------------------------------------------------------------------------
// Purpose: Spawn a grenade with no velocity and a timer of 0
//-----------------------------------------------------------------------------
void CGEWeaponGrenade::ExplodeInHand( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if (!pPlayer)
		return;

#ifndef CLIENT_DLL
	Vector	vecEye = pPlayer->EyePosition();

	CGEGrenade *pGrenade = (CGEGrenade *)CBaseEntity::Create( "npc_grenade", vecEye, vec3_angle, NULL );

	if ( pGrenade )
	{
		pGrenade->SetThrower( GetOwner() );
		pGrenade->SetOwnerEntity( GetOwner() );
		pGrenade->SetSourceWeapon(this);
		pGrenade->SetVelocity( vec3_origin, AngularImpulse(0,0,0) );
		pGrenade->m_bHitSomething = true; //I'm not gonna just give it to you!

		pGrenade->SetDamage( GetGEWpnData().m_iDamage );
		pGrenade->SetDamageRadius( GetGEWpnData().m_flDamageRadius );

		pGrenade->SetTimer( 0 );
	}
#endif

	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();

	// Remove the grenade from our ammo pool
	pPlayer->RemoveAmmo( 1, m_iPrimaryAmmoType );
}

void CGEWeaponGrenade::WeaponIdle( void )
{
	if ( HasWeaponIdleTimeElapsed() )
	{
		if ( m_bDrawNext )
		{
			m_bDrawNext = false;
			SendWeaponAnim( ACT_VM_DRAW );
			m_flTimeWeaponIdle = gpGlobals->curtime + SequenceDuration();
		}
		else
			SendWeaponAnim( ACT_VM_IDLE );
	}
}
