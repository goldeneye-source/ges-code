///////////// Copyright © 2008, Goldeneye: Source All rights reserved. /////////////
// 
// File: npc_tknife.cpp
// Description:
//      Weapon Throwing Knife Projectile definition.  *shing shing* 
//
// Created On: 11/23/2008
// Created By: Killermonkey 
/////////////////////////////////////////////////////////////////////////////
#include "cbase.h"

#include "physics_collisionevent.h"
#include "particle_parse.h"
#include "engine/IEngineSound.h"
#include "te_effect_dispatch.h"
#include "iservervehicle.h"
#include "soundent.h"
#include "IEffects.h"
#include "ammodef.h"

#include "npc_gebase.h"
#include "ge_player.h"
#include "ge_gamerules.h"

#include "npc_tknife.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------
//    Knife Projectile
LINK_ENTITY_TO_CLASS( npc_tknife, CGETKnife );

BEGIN_DATADESC( CGETKnife )
	// Function Pointers
	DEFINE_ENTITYFUNC( DamageTouch ),
	DEFINE_ENTITYFUNC( PickupTouch ),

	DEFINE_THINKFUNC( SoundThink ),
	DEFINE_THINKFUNC( RemoveThink ),
END_DATADESC()

#define TKNIFE_FORCE_SCALE 0.015f

CGETKnife::CGETKnife( void )
{
	m_bInAir = false;
	m_flDamage = 0.0f;
	m_flStopFlyingTime = 0.0f;
}

void CGETKnife::Precache( void )
{
	PrecacheModel( "models/weapons/knife/w_tknife.mdl" );
//	PrecacheParticleSystem( "tracer_tknife" );
}

void CGETKnife::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	// Call the baseclass first so we don't interfere with the normal running of things
	BaseClass::VPhysicsCollision( index, pEvent );

	// Grab what we hit
	CBaseEntity *pOther = pEvent->pEntities[!index];

	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS) )
		return;

	if ( !PassServerEntityFilter( this, pOther) )
		return;

	if ( !g_pGameRules->ShouldCollide( GetCollisionGroup(), pOther->GetCollisionGroup() ) )
		return;

	trace_t tr;
	CollisionEventToTrace( index, pEvent, tr );

	Vector vecAiming = pEvent->preVelocity[index];
	VectorNormalize( vecAiming );

	if ( pOther->m_takedamage != DAMAGE_NO && (pOther->IsPlayer() || pOther->IsNPC()) )
	{
		ClearMultiDamage();

		CTakeDamageInfo	dmgInfo( this, GetOwnerEntity(), GetDamage(), DMG_SLASH | DMG_NEVERGIB );
		CalculateMeleeDamageForce( &dmgInfo, vecAiming, tr.endpos, TKNIFE_FORCE_SCALE );
		dmgInfo.SetDamagePosition( tr.endpos );

		if ( this->GetOwnerEntity() && this->GetOwnerEntity()->IsPlayer() )
		{
			CBasePlayer *pPlayer = ToBasePlayer( this->GetOwnerEntity() );
			dmgInfo.SetWeapon( pPlayer->Weapon_OwnsThisType( "weapon_throwing_knife" ) );
		}

		pOther->DispatchTraceAttack( dmgInfo, vecAiming, &tr );
		
		ApplyMultiDamage();

		PhysCallbackSetVelocity( pEvent->pObjects[index], vec3_origin );

		SetTouch( NULL );
		SetThink( NULL );

		PhysCallbackRemove( this->NetworkProp() );
	}
	else
	{
		if ( pOther->IsWorld() )
		{
			// We hit the world, we have to check if this is sky
			trace_t tr2;
			Vector origin;
			pEvent->pInternalData->GetContactPoint( origin );
			UTIL_TraceLine( origin, origin + (vecAiming * 4), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr2 );
			if ( tr2.surface.flags & SURF_SKY )
			{
				// We hit sky, remove us NOW
				SetTouch( NULL );
				SetThink( NULL );

				PhysCallbackRemove( this->NetworkProp() );
				return;
			}
		}

		m_bInAir = false;
		
		CollisionProp()->UseTriggerBounds( true, 24 );

		g_PostSimulationQueue.QueueCall( this, &CBaseEntity::SetOwnerEntity, (CBaseEntity*)NULL );

//		const CBaseEntity *host = te->GetSuppressHost();
//		te->SetSuppressHost( NULL );
//		StopParticleEffects( this );
//		te->SetSuppressHost( (CBaseEntity*)host );

		SetTouch( &CGETKnife::PickupTouch );
		SetThink( &CGETKnife::RemoveThink );

		g_PostSimulationQueue.QueueCall(this, &CBaseEntity::SetCollisionGroup, COLLISION_GROUP_DROPPEDWEAPON);

		SetNextThink( gpGlobals->curtime + 10.0f );
	}
}

void CGETKnife::DamageTouch( CBaseEntity *pOther )
{
	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS) )
		return;

	if ( !PassServerEntityFilter( this, pOther) )
		return;

	if ( !g_pGameRules->ShouldCollide( GetCollisionGroup(), pOther->GetCollisionGroup() ) )
		return;

	if (!pOther->IsPlayer() && !pOther->IsNPC())
		return;

	Vector vecAiming;
	VPhysicsGetObject()->GetVelocity( &vecAiming, NULL );
	VectorNormalize( vecAiming );

	trace_t tr;
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + (vecAiming * 24), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

	// We didn't hit the player again with our damage cast.  Do a cast to the player's center to get a proper hit.
	if (tr.m_pEnt != pOther)
	{
		Vector TargetVec = pOther->GetAbsOrigin();
		TargetVec.z = MIN(GetAbsOrigin().z, pOther->EyePosition().z); //Make sure we don't cast over their head.
		UTIL_TraceLine(GetAbsOrigin(), pOther->GetAbsOrigin(), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);
	}

// TEMPORARY DEBUGGING PURPOSES
//	DebugDrawLine( tr.startpos, tr.endpos, 0, 255, 0, true, 5.0f );
//	debugoverlay->AddSweptBoxOverlay( tr.startpos, tr.endpos, CollisionProp()->OBBMins(), CollisionProp()->OBBMaxs(), GetAbsAngles(), 0, 0, 255, 100, 5.0f );
// END TEMPORARY

	// If our target can take damage and the trace actually hit our target
	if ( pOther->m_takedamage != DAMAGE_NO && tr.fraction < 1.0f && tr.m_pEnt == pOther )
	{
		ClearMultiDamage();

		CTakeDamageInfo	dmgInfo( this, GetOwnerEntity(), GetDamage(), DMG_SLASH | DMG_NEVERGIB );
		CalculateMeleeDamageForce( &dmgInfo, vecAiming, tr.endpos, TKNIFE_FORCE_SCALE );
		dmgInfo.SetDamagePosition( tr.endpos );

		if ( this->GetOwnerEntity() && this->GetOwnerEntity()->IsPlayer() )
		{
			CBasePlayer *pPlayer = ToBasePlayer( this->GetOwnerEntity() );
			dmgInfo.SetWeapon( pPlayer->Weapon_OwnsThisType( "weapon_knife_throwing" ) );
		}

		pOther->DispatchTraceAttack( dmgInfo, vecAiming, &tr );
		
		ApplyMultiDamage();

		SetAbsVelocity( vec3_origin );

		SetTouch( NULL );
		SetThink( NULL );

		PhysCallbackRemove( this->NetworkProp() );
	}
}

bool UTIL_ItemCanBeTouchedByPlayer( CBaseEntity *pItem, CBasePlayer *pPlayer );
int ITEM_GiveAmmo( CBasePlayer *pPlayer, float flCount, const char *pszAmmoName, bool bSuppressSound = false );

void CGETKnife::PickupTouch( CBaseEntity *pOther )
{
	// Vehicles can touch items + pick them up
	if ( pOther->GetServerVehicle() )
	{
		pOther = pOther->GetServerVehicle()->GetPassenger();
		if ( !pOther )
			return;
	}

	CBaseCombatCharacter *pPicker = pOther->MyCombatCharacterPointer();
	if ( !pPicker )
		return;

	// Must be a valid pickup scenario (no blocking). Though this is a more expensive
	// check than some that follow, this has to be first because it's the only one
	// that inhibits firing the output OnCacheInteraction.
	if ( UTIL_ItemCanBeTouchedByPlayer( this, ToGEPlayer(pPicker) ) == false )
		return;

	// Can I even pick stuff up?
	if ( !pPicker->IsAllowedToPickupWeapons() )
		return;

	if ( MyTouch( pPicker ) )
	{
		SetTouch( NULL );
		SetThink( NULL );

		UTIL_Remove( this );
	}
}

bool CGETKnife::MyTouch( CBaseCombatCharacter *pToucher )
{
	// Try to give them throwing knifes first, if not try giving them ammo
	if ( pToucher->IsPlayer() )
	{
		if ( !ToGEPlayer(pToucher)->GiveNamedItem( "weapon_knife_throwing" ) )
		{
			if ( pToucher->GiveAmmo( 1, GetAmmoDef()->Index(AMMO_TKNIFE) ) )
				return true;
		}
		else
			return true;
	}
	else if ( pToucher->MyNPCPointer() )
	{
		if ( !((CNPC_GEBase*)pToucher)->GiveNamedItem( "weapon_knife_throwing" ) )
		{
			if ( pToucher->GiveAmmo( 1, GetAmmoDef()->Index(AMMO_TKNIFE) ) )
				return true;
		}
		else
			return true;
	}
	
	
	return false;
}

void CGETKnife::SoundThink( void )
{
	if ( gpGlobals->curtime < m_flStopFlyingTime )
	{
		EmitSound( "ThrowingKnife2" );
		SetNextThink( gpGlobals->curtime + 0.5f );
	}
	else
	{
		// This should never happen, but JUST IN CASE we get stuck flying turn
		// us into a physics prop thats able to be picked up
		m_bInAir = false;
		CollisionProp()->UseTriggerBounds( true, 24 );
		SetOwnerEntity( NULL );

		SetTouch( &CGETKnife::PickupTouch );
		SetThink( &CGETKnife::RemoveThink );
		SetNextThink( gpGlobals->curtime + 10.0f );
	}
}

void CGETKnife::RemoveThink( void )
{
	SetTouch( NULL );
	SetThink( NULL );

	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGETKnife::Spawn( void )
{
	Precache();
	BaseClass::Spawn();

	SetModel( "models/weapons/knife/w_tknife.mdl" );
	SetSize( -Vector(8,8,8), Vector(8,8,8) );

	// Init our physics definition
	VPhysicsInitNormal( SOLID_VPHYSICS, GetSolidFlags() | FSOLID_TRIGGER, false );
	SetMoveType( MOVETYPE_VPHYSICS );
	SetCollisionGroup( COLLISION_GROUP_GRENADE );

	// No Air-Drag, lower gravity
	VPhysicsGetObject()->EnableDrag( false );
	SetGravity( UTIL_ScaleForGravity( 540.0f ) );

	m_bInAir = true;
	m_takedamage = DAMAGE_NO;
	AddSolidFlags( FSOLID_NOT_STANDABLE );

	m_flStopFlyingTime = gpGlobals->curtime + 4.5f;

//	const CBaseEntity *host = te->GetSuppressHost();
//	te->SetSuppressHost( NULL );
//	DispatchParticleEffect( "tracer_tknife", PATTACH_POINT_FOLLOW, this, "tip" );
//	te->SetSuppressHost( (CBaseEntity*)host );

	SetTouch( &CGETKnife::DamageTouch );

	SetThink( &CGETKnife::SoundThink );
	SetNextThink( gpGlobals->curtime );
}

void CGETKnife::SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		pPhysicsObject->SetVelocityInstantaneous( &velocity, &angVelocity );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Converts a VPhysics Collision event to a standard trace
//-----------------------------------------------------------------------------
void CGETKnife::CollisionEventToTrace( int index, gamevcollisionevent_t *pEvent, trace_t &tr )
{
	UTIL_ClearTrace( tr );
	pEvent->pInternalData->GetSurfaceNormal( tr.plane.normal );
	pEvent->pInternalData->GetContactPoint( tr.endpos );
	tr.plane.dist = DotProduct( tr.plane.normal, tr.endpos );
	VectorMA( tr.endpos, -1.0f, pEvent->preVelocity[index], tr.startpos );
	tr.m_pEnt = pEvent->pEntities[!index];
	tr.fraction = 0.01f;	// spoof!
}
