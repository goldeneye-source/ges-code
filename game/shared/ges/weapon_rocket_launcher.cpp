///////////// Copyright © 2009, Goldeneye: Source. All rights reserved. /////////////
// 
// File: weapon_rocket_launcher.cpp
// Description:
//      Implements the Rocket Launcher weapon
//
// Created On: 12/5/2009
// Created By: Jonathan White <killermonkey01@gmail.com>
/////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include "in_buttons.h"
#include "ge_weapon.h"
#include "ge_shareddefs.h"

#if defined( CLIENT_DLL )
	#include "c_ge_player.h"
	#include "c_gemp_player.h"
	#include "c_te_legacytempents.h"
#else
	#include "util.h"
	#include "gemp_player.h"
	#include "grenade_rocket.h"
	#include "npc_gebase.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if defined( CLIENT_DLL )
	#define CGEWeaponRocketLauncher	C_GEWeaponRocketLauncher
#endif

//------------------------------------------------------------------------------------------
//
// GE Rocket Launcher weapon class
//
class CGEWeaponRocketLauncher : public CGEWeapon
{
public:
	DECLARE_CLASS( CGEWeaponRocketLauncher, CGEWeapon );

#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif

	DECLARE_ACTTABLE();
	
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	
	CGEWeaponRocketLauncher( void );

	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_ROCKET_LAUNCHER; }

	virtual bool IsExplosiveWeapon() { return true; }

	virtual void PrimaryAttack( void );
	virtual void ItemPostFrame( void );
	virtual void OnReloadOffscreen( void );

	virtual void AddViewKick( void );

	virtual bool Deploy( void );
	virtual void Precache( void );

#ifdef GAME_DLL
	virtual int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	// NPC Functions
	virtual void	FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles );
#endif

private:
	// check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
	void	CheckLaunchPosition( const Vector &vecEye, Vector &vecSrc );
	void	LaunchRocket( void );
	
	int		m_iRocketGroup;

	CNetworkVar( bool, m_bPreLaunch );
	CNetworkVar( float, m_flRocketSpawnTime );

private:
	CGEWeaponRocketLauncher( const CGEWeaponRocketLauncher & );
};

acttable_t	CGEWeaponRocketLauncher::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_GES_IDLE_RL,						false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_SHOTGUN,			false },

	{ ACT_MP_RUN,						ACT_GES_RUN_RL,						false },
	{ ACT_MP_WALK,						ACT_GES_WALK_RL,						false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_RPG,				false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_RL,		false },
	{ ACT_GES_ATTACK_RUN_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_RL,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_RL,		false },

	{ ACT_GES_DRAW,						ACT_GES_GESTURE_DRAW_RIFLE,				false },

	{ ACT_MP_RELOAD_STAND,				ACT_GES_GESTURE_RELOAD_PHANTOM,			false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_GES_GESTURE_RELOAD_PHANTOM,			false },

	{ ACT_MP_JUMP,						ACT_GES_JUMP_RL,						false },
	{ ACT_GES_CJUMP,					ACT_GES_CJUMP_RL,						false },
};
IMPLEMENT_ACTTABLE(CGEWeaponRocketLauncher);

IMPLEMENT_NETWORKCLASS_ALIASED( GEWeaponRocketLauncher, DT_GEWeaponRocketLauncher )

BEGIN_NETWORK_TABLE( CGEWeaponRocketLauncher, DT_GEWeaponRocketLauncher )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bPreLaunch ) ),
	RecvPropFloat( RECVINFO( m_flRocketSpawnTime ) ),
#else
	SendPropBool( SENDINFO( m_bPreLaunch ) ),
	SendPropFloat( SENDINFO( m_flRocketSpawnTime ) ),
#endif
END_NETWORK_TABLE()

#if !defined( CLIENT_DLL )
BEGIN_DATADESC( CGEWeaponRocketLauncher )
	DEFINE_FIELD( m_bPreLaunch, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flRocketSpawnTime, FIELD_FLOAT ),
END_DATADESC()
#endif

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CGEWeaponRocketLauncher )
	DEFINE_PRED_FIELD( m_bPreLaunch, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flRocketSpawnTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),

	DEFINE_PRED_FIELD_TOL( m_flRocketSpawnTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TD_MSECTOLERANCE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_rocket_launcher, CGEWeaponRocketLauncher );
PRECACHE_WEAPON_REGISTER(weapon_rocket_launcher);

CGEWeaponRocketLauncher::CGEWeaponRocketLauncher( void )
{
	// NPC Ranging
	m_fMinRange1 = 300;
	m_fMaxRange1 = 5000;

	m_iRocketGroup = 0;
}

bool CGEWeaponRocketLauncher::Deploy( void )
{
	m_bPreLaunch = false;
	m_flRocketSpawnTime = -1;

	bool success = BaseClass::Deploy();

	if (m_iClip1 > 0)
		SwitchBodygroup(1, 0);
	else
		SwitchBodygroup(1, 1);

	return success;
}

void CGEWeaponRocketLauncher::Precache( void )
{
	PrecacheModel("models/weapons/rocket_launcher/v_rocket_launcher.mdl");
	PrecacheModel("models/weapons/rocket_launcher/w_rocket_launcher.mdl");

	PrecacheModel("models/weapons/rocket_launcher/w_rocket.mdl");
	PrecacheMaterial("models/weapons/w_models/w_gl/grenadeprojectile");

	PrecacheMaterial("sprites/hud/weaponicons/rocket_launcher");
	PrecacheMaterial("sprites/hud/ammoicons/ammo_rocket");

	PrecacheScriptSound("Weapon_RocketLauncher.Single");
	PrecacheScriptSound("Weapon_RocketLauncher.Ignite");

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Put the rocket back on the end of the rocket launcher after we reload it.
//-----------------------------------------------------------------------------
void CGEWeaponRocketLauncher::OnReloadOffscreen(void)
{
	BaseClass::OnReloadOffscreen();

	CBaseCombatCharacter *pOwner = GetOwner();
	if (!pOwner)
		return;

	if (pOwner->GetActiveWeapon() == this)
		SwitchBodygroup(1, 0);
}

//-----------------------------------------------------------------------------
// Purpose: Prepare the launcher to fire
//-----------------------------------------------------------------------------
void CGEWeaponRocketLauncher::PrimaryAttack( void )
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

	// Note that this is a primary attack and prepare the grenade attack to pause.
	m_bPreLaunch = true;
	SendWeaponAnim( GetPrimaryAttackActivity() );
	ToGEPlayer(pPlayer)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	m_flRocketSpawnTime = gpGlobals->curtime + GetFireDelay();
	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
}

void CGEWeaponRocketLauncher::ItemPostFrame( void )
{
	if( m_bPreLaunch && m_flRocketSpawnTime < gpGlobals->curtime )
	{
		LaunchRocket();
		m_bPreLaunch = false;
	}
	
	BaseClass::ItemPostFrame();
}

// Make sure we aren't going to throw the grenade INTO something!
void CGEWeaponRocketLauncher::CheckLaunchPosition( const Vector &vecEye, Vector &vecSrc )
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if ( !pOwner )
		return;

	trace_t tr;
	UTIL_TraceHull( vecEye, vecSrc, -Vector(6,6,6), Vector(6,6,6), pOwner->PhysicsSolidMaskForEntity(), pOwner, pOwner->GetCollisionGroup(), &tr );
	
	if ( tr.DidHit() )
		vecSrc = tr.endpos;
}

#ifdef GAME_DLL
void CGEWeaponRocketLauncher::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles )
{
	LaunchRocket();
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Launch a bounce/impact grenade
//-----------------------------------------------------------------------------
void CGEWeaponRocketLauncher::LaunchRocket( void )
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if ( !pOwner )
		return;

#ifndef CLIENT_DLL
	Vector	vForward, vRight, vUp;
	AngleVectors( pOwner->EyeAngles(), &vForward, &vRight, &vUp );

	// Manipulate the shoot position such that it is in front of our gun muzzle
	Vector vecSrc = pOwner->Weapon_ShootPosition();

	if ( pOwner->IsPlayer() )
	{
		VectorMA( vecSrc, 4.5f, vRight, vecSrc ); // 3.0, 5.0
		VectorMA( vecSrc, 16.5f, vForward, vecSrc ); // 20, 19
		VectorMA( vecSrc, -5.25f, vUp, vecSrc ); // -2, -6, -5, 5.25
	}
	else
	{
		VectorMA( vecSrc, 20.0, vForward, vecSrc );
	}

	CheckLaunchPosition( pOwner->EyePosition(), vecSrc );

	if ( pOwner->MyNPCPointer() )
		vForward = pOwner->MyNPCPointer()->GetActualShootTrajectory( vecSrc );

	QAngle angAiming;
	VectorAngles( vForward, angAiming );

	// Convert us into a bot player :-D
	if ( pOwner->IsNPC() )
	{
		CNPC_GEBase *pNPC = (CNPC_GEBase*) pOwner;
		if ( pNPC->GetBotPlayer() )
			pOwner = pNPC->GetBotPlayer();
	}
	
	CGERocket *pRocket = (CGERocket*)CBaseEntity::Create( "npc_rocket", vecSrc, angAiming, NULL );

	if ( pRocket )
	{
		pRocket->SetThrower( pOwner );
		pRocket->SetOwnerEntity( pOwner );
		pRocket->SetSourceWeapon(this);

		pRocket->SetDamage( GetGEWpnData().m_iDamage );
		pRocket->SetDamageRadius( GetGEWpnData().m_flDamageRadius );

		if (pOwner->GetTeamNumber() == TEAM_JANUS)
			pRocket->SetCollisionGroup( COLLISION_GROUP_GRENADE_JANUS );
		else if (pOwner->GetTeamNumber() == TEAM_MI6)
			pRocket->SetCollisionGroup( COLLISION_GROUP_GRENADE_MI6 );

		// Tell the owner what we threw to implement anti-spamming
		if ( pOwner->IsPlayer() )
			ToGEMPPlayer( pOwner )->AddThrownObject( pRocket );
	}
#endif

	// Remove the rocket from our ammo pool
	m_iClip1 -= 1;
	WeaponSound( SINGLE );

	SwitchBodygroup(1, 1);

	//Add our view kick in
	AddViewKick();
}

void CGEWeaponRocketLauncher::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle	viewPunch;

	viewPunch.x = SharedRandomFloat( "gerocketpax", GetGEWpnData().Kick.x_min, GetGEWpnData().Kick.x_max );
	viewPunch.y = SharedRandomFloat( "gerocketpay", GetGEWpnData().Kick.y_min, GetGEWpnData().Kick.y_max );
	viewPunch.z = 0.0f;

	//Add it to the view punch
	pPlayer->ViewPunch( viewPunch );
}
