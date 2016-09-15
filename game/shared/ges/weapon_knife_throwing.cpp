///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: weapon_knife_throwing.cpp
// Description:
//      Throws a knife projectile that hurts
//
// Created On: 1/16/09
// Created By: Jonathan White <killermonkey01> (Original By Scott Lloyd)
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "in_buttons.h"

#ifdef GAME_DLL
	#include "npc_tknife.h"
	#include "gemp_player.h"
	#include "npc_gebase.h"
#endif


#include "ge_weaponmelee.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponKnifeThrowing C_WeaponKnifeThrowing
#endif

#define TKNIFE_AIR_VELOCITY		820
#define TKNIFE_WATER_VELOCITY	400

//-----------------------------------------------------------------------------
// CWeaponKnife
//-----------------------------------------------------------------------------

class CWeaponKnifeThrowing : public CGEWeapon
{
public:
	DECLARE_CLASS( CWeaponKnifeThrowing, CGEWeapon );

	DECLARE_DATADESC();
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	DECLARE_ACTTABLE();

	CWeaponKnifeThrowing();

	void		AddViewKick( void );

	virtual void PrimaryAttack();

	virtual void ThrowKnife();

	virtual void Precache( void );
	virtual void ItemPreFrame( void );
	virtual void ItemPostFrame( void );

	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_KNIFE_THROWING; }
	virtual bool IsThrownWeapon()				 { return true; }

#ifndef CLIENT_DLL
	void FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles );
	CGETKnife *CreateKnife( const Vector &vecOrigin, const QAngle &angAngles, CBaseCombatCharacter *pentOwner );
#endif

private:
	CNetworkVar( bool, m_bPreThrow );
	CNetworkVar( float, m_flReleaseTime );

	CWeaponKnifeThrowing( const CWeaponKnifeThrowing & );
};


//-----------------------------------------------------------------------------
// CWeaponKnifeThrowing
//-----------------------------------------------------------------------------
IMPLEMENT_NETWORKCLASS_ALIASED( WeaponKnifeThrowing, DT_WeaponKnifeThrowing )

BEGIN_NETWORK_TABLE( CWeaponKnifeThrowing, DT_WeaponKnifeThrowing )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bPreThrow ) ),
	RecvPropFloat( RECVINFO( m_flReleaseTime ) ),
#else
	SendPropBool( SENDINFO( m_bPreThrow ) ),
	SendPropFloat( SENDINFO( m_flReleaseTime ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponKnifeThrowing )
	DEFINE_PRED_FIELD( m_bPreThrow, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flReleaseTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

BEGIN_DATADESC( CWeaponKnifeThrowing )
	DEFINE_FIELD( m_bPreThrow, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flReleaseTime, FIELD_FLOAT ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( weapon_knife_throwing, CWeaponKnifeThrowing );
PRECACHE_WEAPON_REGISTER( weapon_knife_throwing );

acttable_t	CWeaponKnifeThrowing::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_GES_IDLE_TKNIFE,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_MELEE,			false },

	{ ACT_MP_RUN,						ACT_GES_RUN_TKNIFE,						false },
	{ ACT_MP_WALK,						ACT_GES_WALK_TKNIFE,					false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_MELEE,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_TKNIFE,	true },
	{ ACT_GES_ATTACK_RUN_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_TKNIFE,	true },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_TKNIFE,	true },

	{ ACT_MP_RELOAD_STAND,				ACT_HL2MP_GESTURE_RELOAD_MELEE,			false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_MELEE,			false },

	{ ACT_MP_JUMP,						ACT_GES_JUMP_TKNIFE,					false },
	{ ACT_GES_CJUMP,					ACT_GES_CJUMP_TKNIFE,					false },
};
IMPLEMENT_ACTTABLE(CWeaponKnifeThrowing);

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponKnifeThrowing::CWeaponKnifeThrowing( void )
{
	m_bPreThrow = false;
	m_flReleaseTime = 0.0f;

	m_bReloadsSingly = true;

	m_fMinRange1 = 24;
	m_fMaxRange1 = 500;
}

void CWeaponKnifeThrowing::Precache(void)
{
	PrecacheModel("models/weapons/knife/v_tknife.mdl");
	PrecacheModel("models/weapons/knife/w_tknife.mdl");

	PrecacheMaterial("sprites/hud/ammoicons/ammo_tknife");

	PrecacheScriptSound("weapon_knife_throwing.Single");
	PrecacheScriptSound("Weapon_Crowbar.Melee_Hit");

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Add in a view kick for this weapon
//-----------------------------------------------------------------------------
void CWeaponKnifeThrowing::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle	viewPunch;

	viewPunch.x = SharedRandomFloat( "getknifepax", GetGEWpnData().Kick.x_min, GetGEWpnData().Kick.x_max );
	viewPunch.y = SharedRandomFloat( "getknifepay", GetGEWpnData().Kick.y_min, GetGEWpnData().Kick.y_max );
	viewPunch.z = 0.0f;

	//Add it to the view punch
	pPlayer->ViewPunch( viewPunch );
}

void CWeaponKnifeThrowing::ItemPreFrame( void )
{
	if ( m_bPreThrow && gpGlobals->curtime >= m_flReleaseTime )
	{
		ThrowKnife();
		m_bPreThrow = false;
	}

	BaseClass::ItemPreFrame();
}

void CWeaponKnifeThrowing::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	BaseClass::ItemPostFrame();
}

void CWeaponKnifeThrowing::PrimaryAttack()
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
		return;

	m_bPreThrow = true;
	m_flReleaseTime = gpGlobals->curtime + GetFireDelay();

	m_flNextSecondaryAttack	= gpGlobals->curtime + GetFireRate();
	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();

	SendWeaponAnim( ACT_VM_MISSCENTER );
	pOwner->SetAnimation( PLAYER_ATTACK1 );
	ToGEPlayer(pOwner)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );
}

#ifdef GAME_DLL
void CWeaponKnifeThrowing::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles )
{
	ThrowKnife();
}

CGETKnife *CWeaponKnifeThrowing::CreateKnife( const Vector &vecOrigin, const QAngle &angAngles, CBaseCombatCharacter *pOwner )
{
	CGETKnife *pKnife = (CGETKnife *)CBaseEntity::Create( "npc_tknife", vecOrigin, angAngles, NULL );

	pKnife->SetDamage( GetGEWpnData().m_iDamage );
	pKnife->SetOwnerEntity( pOwner );
	pKnife->SetSourceWeapon(this);

	if (pOwner->GetTeamNumber() == TEAM_JANUS)
		pKnife->SetCollisionGroup(COLLISION_GROUP_GRENADE_JANUS);
	else if (pOwner->GetTeamNumber() == TEAM_MI6)
		pKnife->SetCollisionGroup(COLLISION_GROUP_GRENADE_MI6);

	// Tell the owner what we threw to implement anti-spamming
	if ( pOwner->IsPlayer() )
		ToGEMPPlayer( pOwner )->AddThrownObject( pKnife );

	return pKnife;
}
#endif

void CWeaponKnifeThrowing::ThrowKnife()
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if ( !pOwner || pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
		return;

	pOwner->RemoveAmmo(1, m_iPrimaryAmmoType);

#ifndef CLIENT_DLL
	Vector vecSrc = pOwner->Weapon_ShootPosition();

	Vector	vForward, vRight;
	AngleVectors( pOwner->EyeAngles(), &vForward, &vRight, NULL );
	vForward[2] += 0.1f;
	
	if (pOwner->IsPlayer())
	{
		VectorMA(vecSrc, 3.0f, vRight, vecSrc); // 3.0, 5.0
		VectorMA(vecSrc, 2.0f, vForward, vecSrc); // 20, 19
	}
	else
	{
		VectorMA(vecSrc, 20.0, vForward, vecSrc);
	}

	Vector vecThrow;
	pOwner->GetVelocity( &vecThrow, NULL );
	if ( pOwner->GetWaterLevel() == 3 )
		vecThrow += vForward * TKNIFE_WATER_VELOCITY;
	else
		vecThrow += vForward * TKNIFE_AIR_VELOCITY;

	QAngle angAiming;
	VectorAngles( vForward, angAiming );

	// Convert the NPC to a bot if necessary
	if ( pOwner->IsNPC() )
	{
		CNPC_GEBase *pNPC = (CNPC_GEBase*) pOwner;
		if ( pNPC->GetBotPlayer() )
			pOwner = pNPC->GetBotPlayer();
	}

	CGETKnife *pKnife = CreateKnife( vecSrc, angAiming, pOwner );

	pKnife->SetVelocity( vecThrow, AngularImpulse(0,800,0) );
#endif
}
