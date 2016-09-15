///////////// Copyright © 2006, Scott Loyd. All rights reserved. /////////////
// 
// File: weapon_knife.cpp
// Description:
//      Melee weapon that deals a decent amount of damage.
//
// Created On: 3/9/2006 2:28:35 AM
// Created By: Scott Loyd <mailto:scottloyd@gmail.com> 
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ge_weaponmelee.h"
#include "ge_gamerules.h"
#include "ammodef.h"
#include "mathlib/mathlib.h"
#include "in_buttons.h"
#include "vstdlib/random.h"
#include "npcevent.h"

#if defined( CLIENT_DLL )
	#include "c_ge_player.h"
#else
	#include "ge_player.h"
	#include "ai_basenpc.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	KNIFE_RANGE		75.0f

#ifdef CLIENT_DLL
#define CWeaponKnife C_WeaponKnife
#endif

//-----------------------------------------------------------------------------
// CWeaponKnife
//-----------------------------------------------------------------------------

class CWeaponKnife : public CGEWeaponMelee
{
public:
	DECLARE_CLASS( CWeaponKnife, CGEWeaponMelee );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	DECLARE_ACTTABLE();

	CWeaponKnife();

	float		GetRange( void )		{ return KNIFE_RANGE; }

	void		AddViewKick( void );

	virtual		void Precache(void);

//	void		Drop( const Vector &vecVelocity );

	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_KNIFE; }

	// Animation event
#ifndef CLIENT_DLL
	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	void HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator );

	void SwitchToThrowing();
#endif

protected:
	void ImpactEffect( trace_t &trace );

private:
	CWeaponKnife( const CWeaponKnife & );
};

//-----------------------------------------------------------------------------
// CWeaponKnife
//-----------------------------------------------------------------------------

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponKnife, DT_WeaponKnife )

BEGIN_NETWORK_TABLE( CWeaponKnife, DT_WeaponKnife )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponKnife )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_knife, CWeaponKnife );
PRECACHE_WEAPON_REGISTER( weapon_knife );

acttable_t	CWeaponKnife::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_GES_IDLE_KNIFE,						false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_MELEE,			false },

	{ ACT_MP_RUN,						ACT_GES_RUN_KNIFE,						false },
	{ ACT_MP_WALK,						ACT_GES_WALK_KNIFE,						false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_MELEE,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_KNIFE,		true },
	{ ACT_GES_ATTACK_RUN_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_KNIFE,		true },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_KNIFE,		true },

	{ ACT_GES_DRAW,						ACT_GES_GESTURE_DRAW_KNIFE,				false },

	{ ACT_MP_RELOAD_STAND,				ACT_HL2MP_GESTURE_RELOAD_MELEE,			false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_HL2MP_GESTURE_RELOAD_MELEE,			false },

	{ ACT_MP_JUMP,						ACT_GES_JUMP_KNIFE,						false },
	{ ACT_GES_CJUMP,					ACT_GES_CJUMP_KNIFE,					false },
};
IMPLEMENT_ACTTABLE(CWeaponKnife);


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponKnife::CWeaponKnife( void )
{
}


void CWeaponKnife::Precache(void)
{
	PrecacheModel("models/weapons/knife/v_knife.mdl");
	PrecacheModel("models/weapons/knife/w_knife.mdl");

	PrecacheMaterial("sprites/hud/weaponicons/knife");

	PrecacheScriptSound("Weapon_Knife.Single");
	PrecacheScriptSound("Weapon_Crowbar.Melee_Hit");

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: Add in a view kick for this weapon
//-----------------------------------------------------------------------------
void CWeaponKnife::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle punchAng;

	punchAng.x = random->RandomFloat( -1.0f, 1.0f );
	punchAng.y = random->RandomFloat( 0.5f, 1.25f );
	punchAng.z = 0.0f;
	
	pPlayer->ViewPunch( punchAng ); 
}


#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Animation event handlers
//-----------------------------------------------------------------------------
void CWeaponKnife::HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	// Trace up or down based on where the enemy is...
	// But only if we're basically facing that direction
	Vector vecDirection;
	AngleVectors( GetAbsAngles(), &vecDirection );

	Vector vecEnd;
//	VectorMA( pOperator->Weapon_ShootPosition(), 50, vecDirection, vecEnd );
//	CBaseEntity *pHurt = pOperator->CheckTraceHullAttack( pOperator->Weapon_ShootPosition(), vecEnd, 
//		Vector(-16,-16,-16), Vector(36,36,36), GetDamageForActivity( GetActivity() ), DMG_CLUB, 0.75 );

	VectorMA(pOperator->Weapon_ShootPosition(), KNIFE_RANGE, vecDirection, vecEnd);
	CBaseEntity *pHurt = pOperator->CheckTraceHullAttack(pOperator->Weapon_ShootPosition(), vecEnd, Vector(-16, -16, -16), Vector(36, 36, 36), GetGEWpnData().m_iDamage, DMG_CLUB);
	
	// did I hit someone?
	if ( pHurt )
	{
		// play sound
		WeaponSound( MELEE_HIT );

		// Fake a trace impact, so the effects work out like a player's crowbaw
		trace_t traceHit;
		UTIL_TraceLine( pOperator->Weapon_ShootPosition(), pHurt->GetAbsOrigin(), MASK_SHOT_HULL, pOperator, COLLISION_GROUP_NONE, &traceHit );
		ImpactEffect( traceHit );
	}
	else
	{
		WeaponSound( MELEE_MISS );
	}
}


//-----------------------------------------------------------------------------
// Animation event
//-----------------------------------------------------------------------------
void CWeaponKnife::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
	case EVENT_WEAPON_AR2:
		Swing( false );
		break;

	case EVENT_WEAPON_MELEE_HIT:
		HandleAnimEventMeleeHit( pEvent, pOperator );
		break;

	default:
		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
	}
}
#endif

#ifndef CLIENT_DLL
void CWeaponKnife::SwitchToThrowing()
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	CBaseCombatWeapon *pKnifeThrowing = NULL;
	pKnifeThrowing = pOwner->Weapon_OwnsThisType("weapon_knife_throwing");

	if(pKnifeThrowing)
	{
		if(pOwner->GetAmmoCount(pKnifeThrowing->m_iPrimaryAmmoType) > 0)
			pOwner->Weapon_Switch( pKnifeThrowing );
	}
}
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
//void CWeaponKnife::Drop( const Vector &vecVelocity )
//{
//#ifndef CLIENT_DLL
//	UTIL_Remove( this );
//#endif
//}

void CWeaponKnife::ImpactEffect( trace_t &traceHit )
{
	return; // Avoid doing melee impact effects for 5.0

	// See if we hit water (we don't do the other impact effects in this case)
	if ( ImpactWater( traceHit.startpos, traceHit.endpos ) )
		return;

	if ( traceHit.m_pEnt && traceHit.m_pEnt->MyCombatCharacterPointer() )
		return;

	UTIL_ImpactTrace( &traceHit, DMG_SLASH );
}
