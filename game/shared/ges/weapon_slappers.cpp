///////////// Copyright © 2006, Scott Loyd. All rights reserved. /////////////
// 
// File: weapon_slappers.cpp
// Description:
//      Where would Bond be be without his hands?  This should act just like
//		 any other basic melee(such as knife), but not draw a weapon model, 
//		 just a handrig (since it needs animations).
//
// Created On: 3/9/2006 2:28:22 AM
// Created By: Scott Loyd <mailto:scottloyd@gmail.com> 
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ge_weaponmelee.h"
#include "gamerules.h"
#include "ammodef.h"
#include "mathlib/mathlib.h"
#include "in_buttons.h"
#include "vstdlib/random.h"
#include "npcevent.h"

#ifdef GAME_DLL
	#include "ai_basenpc.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	SLAPPER_RANGE		65.0f

#ifdef CLIENT_DLL
#define CWeaponSlappers C_WeaponSlappers
#endif

//-----------------------------------------------------------------------------
// CWeaponSlappers
//-----------------------------------------------------------------------------

class CWeaponSlappers : public CGEWeaponMelee
{
public:
	DECLARE_CLASS( CWeaponSlappers, CGEWeaponMelee );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	DECLARE_ACTTABLE();

	CWeaponSlappers();

	virtual void Spawn( void );

	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_SLAPPERS; }

	float		GetRange( void )		{	return	SLAPPER_RANGE;	}
	bool		DamageWorld()			{	return	false;			}

	virtual void	Precache(void);
	void		AddViewKick( void );
	void		SecondaryAttack( void )	{	return;	}

	virtual void Equip( CBaseCombatCharacter *pOwner );
	virtual void Drop( const Vector &vecVelocity );

	// Animation event
#ifndef CLIENT_DLL
	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	void HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
#endif

protected:
	virtual	void	ImpactEffect( trace_t &trace );

private:
	CWeaponSlappers( const CWeaponSlappers & );		
};

//-----------------------------------------------------------------------------
// CWeaponSlappers
//-----------------------------------------------------------------------------

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponSlappers, DT_WeaponSlappers )

BEGIN_NETWORK_TABLE( CWeaponSlappers, DT_WeaponSlappers )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponSlappers )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_slappers, CWeaponSlappers );
PRECACHE_WEAPON_REGISTER( weapon_slappers );

acttable_t	CWeaponSlappers::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_GES_IDLE_SLAPPERS,					false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_MELEE,			false },

	{ ACT_MP_RUN,						ACT_GES_RUN_SLAPPERS,					false },
	{ ACT_MP_WALK,						ACT_GES_WALK_SLAPPERS,					false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_MELEE,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_SLAPPERS,	true },
	{ ACT_GES_ATTACK_RUN_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_SLAPPERS,	true },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_SLAPPERS,	true },

	{ ACT_GES_DRAW,						ACT_GES_GESTURE_DRAW_KNIFE,				false },

	{ ACT_MP_JUMP,						ACT_GES_JUMP_SLAPPERS,					false },
	{ ACT_GES_CJUMP,					ACT_GES_CJUMP_SLAPPERS,					false },
};
IMPLEMENT_ACTTABLE(CWeaponSlappers);

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CWeaponSlappers::CWeaponSlappers( void )
{

}

void CWeaponSlappers::Precache(void)
{
	PrecacheModel("models/weapons/slappers/v_slappers.mdl");
	PrecacheModel("models/weapons/slappers/w_slappers.mdl");

	PrecacheScriptSound("Weapon_slappers.Swing");
	PrecacheScriptSound("Weapon_slappers.Smack");

	BaseClass::Precache();
}

void CWeaponSlappers::Spawn( void )
{
	BaseClass::Spawn();
	CollisionProp()->SetCollisionBounds( -Vector(16,16,16), Vector(16,16,16) );
}

//-----------------------------------------------------------------------------
// Purpose: Add in a view kick for this weapon
//-----------------------------------------------------------------------------
void CWeaponSlappers::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle punchAng;

	punchAng.x = random->RandomFloat(-0.75f, 0.75f);
	punchAng.y = random->RandomFloat(0.5f, 1.00f);
	punchAng.z = 0.0f;
	
	pPlayer->ViewPunch( punchAng ); 
}


#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Animation event handlers
//-----------------------------------------------------------------------------
void CWeaponSlappers::HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	// Trace up or down based on where the enemy is...
	// But only if we're basically facing that direction
	Vector vecDirection;
	AngleVectors( GetAbsAngles(), &vecDirection );

	Vector vecEnd;
	VectorMA( pOperator->Weapon_ShootPosition(), SLAPPER_RANGE, vecDirection, vecEnd );
	CBaseEntity *pHurt = pOperator->CheckTraceHullAttack( pOperator->Weapon_ShootPosition(), vecEnd, Vector(-16,-16,-16), Vector(36,36,36), GetGEWpnData().m_iDamage, DMG_CLUB );
	
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
void CWeaponSlappers::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
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


void CWeaponSlappers::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip( pOwner );
	CollisionProp()->SetCollisionBounds( -Vector(16,16,16), Vector(16,16,16) );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponSlappers::Drop( const Vector &vecVelocity )
{
#ifndef CLIENT_DLL
	UTIL_Remove( this );
#endif
}

void CWeaponSlappers::ImpactEffect( trace_t &traceHit )
{
	// Slappers don't make impact marks
}
