///////////// Copyright © 2006, Scott Loyd. All rights reserved. /////////////
// 
// File: weapon_gebasemelee.cpp
// Description:
//      All melee weapons will derive from this (such as knife/slappers).
//
// Created On: 3/9/2006 2:28:17 AM
// Created By: Scott Loyd <mailto:scottloyd@gmail.com> 
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ge_weaponmelee.h"
#include "ge_gamerules.h"
#include "ammodef.h"
#include "mathlib/mathlib.h"
#include "in_buttons.h"
#include "animation.h"
#include "takedamageinfo.h"

#if defined( CLIENT_DLL )
	#include "c_ge_player.h"
#else
	#include "ge_player.h"
	#include "npc_gebase.h"
	#include "ndebugoverlay.h"
	#include "te_effect_dispatch.h"
	#include "ilagcompensationmanager.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( GEWeaponMelee, DT_GEWeaponMelee )

BEGIN_NETWORK_TABLE( CGEWeaponMelee, DT_GEWeaponMelee )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CGEWeaponMelee )
END_PREDICTION_DATA()

#define BLUDGEON_HULL_DIM		28

static const Vector g_bludgeonMins( -BLUDGEON_HULL_DIM, -BLUDGEON_HULL_DIM, -BLUDGEON_HULL_DIM );
static const Vector g_bludgeonMaxs( BLUDGEON_HULL_DIM, BLUDGEON_HULL_DIM, BLUDGEON_HULL_DIM );

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CGEWeaponMelee::CGEWeaponMelee()
{
	m_bFiresUnderwater = true;
	// NPC Ranging
	m_fMinRange1	= 0;
	m_fMinRange2	= 0;
	m_fMaxRange1	= GetRange() * 0.9;
	m_fMaxRange2	= GetRange() * 0.9;
}

//-----------------------------------------------------------------------------
// Purpose: Spawn the weapon
//-----------------------------------------------------------------------------
void CGEWeaponMelee::Spawn( void )
{
	//Call base class first
	BaseClass::Spawn();
}

//------------------------------------------------------------------------------
// Purpose : Update weapon
//------------------------------------------------------------------------------
void CGEWeaponMelee::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	if ( (pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime) )
	{
		PrimaryAttack();
	} 
	else if ( (pOwner->m_nButtons & IN_ATTACK2) && (m_flNextSecondaryAttack <= gpGlobals->curtime) )
	{
		SecondaryAttack();
	}
	else 
	{
		WeaponIdle();
		return;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Add's a view kick, multiplied when you actually hit something!
//-----------------------------------------------------------------------------
void CGEWeaponMelee::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle	viewPunch;

	viewPunch.x = SharedRandomFloat( "gemeleepax", GetGEWpnData().Kick.x_min, GetGEWpnData().Kick.x_max );
	viewPunch.y = SharedRandomFloat( "gemeleepay", GetGEWpnData().Kick.y_min, GetGEWpnData().Kick.y_max );
	viewPunch.z = 0.0f;

	//Add it to the view punch
	pPlayer->ViewPunch( viewPunch );
}

#ifdef GAME_DLL
int CGEWeaponMelee::WeaponMeleeAttack1Condition( float flDot, float flDist )
{
	if ( flDist > GetRange() )
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if ( flDot < 0.7 )
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_MELEE_ATTACK1;
}
#endif

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CGEWeaponMelee::PrimaryAttack()
{
	//here to reg the bots :P
	if (gpGlobals->curtime < m_flNextPrimaryAttack)
		return;

#ifndef CLIENT_DLL
	CGEPlayer *pPlayer = ToGEPlayer( GetPlayerOwner() );
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( pPlayer, pPlayer->GetCurrentCommand() );
#else
	CUtlVector<C_GEPlayer*> ents;
	for ( int i=1; i < gpGlobals->maxClients; i++ )
	{
		C_GEPlayer *pPlayer = ToGEPlayer( UTIL_PlayerByIndex(i) );
		if ( !pPlayer )
			continue;

		pPlayer->AdjustCollisionBounds( true );
		ents.AddToTail( pPlayer );
	}
#endif

	Swing( false );

#ifndef CLIENT_DLL
	// Move other players back to history positions based on local player's lag
	lagcompensation->FinishLagCompensation( pPlayer );
#else
	for ( int i=0; i < ents.Count(); i++ )
		ents[i]->AdjustCollisionBounds( false );
#endif
}

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CGEWeaponMelee::SecondaryAttack()
{
	Swing( true );
}


//------------------------------------------------------------------------------
// Purpose: Implement impact function
//------------------------------------------------------------------------------
void CGEWeaponMelee::Hit( trace_t &traceHit, Activity nHitActivity )
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if ( !pOwner )
		return;
	
	//Do view kick
	AddViewKick();

	CBaseEntity	*pHitEntity = traceHit.m_pEnt;

	//Apply damage to a hit target
	if ( pHitEntity != NULL )
	{
		Vector hitDirection;
		AngleVectors( pOwner->EyeAngles(), &hitDirection, NULL, NULL );
		VectorNormalize( hitDirection );

		// Take into account bots...
		CBaseCombatCharacter *pOwner = GetOwner();

	#ifdef GAME_DLL
		if ( pOwner->IsNPC() )
		{
			CNPC_GEBase *pNPC = (CNPC_GEBase*) pOwner;
			if ( pNPC->GetBotPlayer() )
				pOwner = pNPC->GetBotPlayer();
		}
	#endif

		CTakeDamageInfo info( pOwner, pOwner, GetDamageForActivity( GetActivity() ), DMG_CLUB );
		info.SetWeapon(this);
		CalculateMeleeDamageForce( &info, hitDirection, traceHit.endpos, 0.02 );

		pHitEntity->DispatchTraceAttack( info, hitDirection, &traceHit );

	#ifndef CLIENT_DLL
		ApplyMultiDamage();
		// Now hit all triggers along the ray that... 
		TraceAttackToTriggers( info, traceHit.startpos, traceHit.endpos, hitDirection );
	#endif

		WeaponSound( MELEE_HIT );
	}

	// Apply an impact effect
	ImpactEffect( traceHit );
}

Activity CGEWeaponMelee::ChooseIntersectionPointAndActivity( trace_t &hitTrace, const Vector &mins, const Vector &maxs, CBaseEntity *pOwner )
{
	int			i, j, k;
	float		distance;
	const float	*minmaxs[2] = {mins.Base(), maxs.Base()};
	trace_t		tmpTrace;
	Vector		vecHullEnd = hitTrace.endpos;
	Vector		vecEnd;

	distance = 1e6f;
	Vector vecSrc = hitTrace.startpos;

	vecHullEnd = vecSrc + ((vecHullEnd - vecSrc)*2);
	UTIL_TraceLine( vecSrc, vecHullEnd, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &tmpTrace );
	if ( tmpTrace.fraction == 1.0 )
	{
		for ( i = 0; i < 2; i++ )
		{
			for ( j = 0; j < 2; j++ )
			{
				for ( k = 0; k < 2; k++ )
				{
					vecEnd.x = vecHullEnd.x + minmaxs[i][0];
					vecEnd.y = vecHullEnd.y + minmaxs[j][1];
					vecEnd.z = vecHullEnd.z + minmaxs[k][2];

					UTIL_TraceLine( vecSrc, vecEnd, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &tmpTrace );
					if ( tmpTrace.fraction < 1.0 )
					{
						float thisDistance = (tmpTrace.endpos - vecSrc).Length();
						if ( thisDistance < distance )
						{
							hitTrace = tmpTrace;
							distance = thisDistance;
						}
					}
				}
			}
		}
	}
	else
	{
		hitTrace = tmpTrace;
	}


	return ACT_VM_HITCENTER;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &traceHit - 
//-----------------------------------------------------------------------------
bool CGEWeaponMelee::ImpactWater( const Vector &start, const Vector &end )
{
	//FIXME: This doesn't handle the case of trying to splash while being underwater, but that's not going to look good
	//		 right now anyway...
	
	// We must start outside the water
	if ( UTIL_PointContents( start ) & (CONTENTS_WATER|CONTENTS_SLIME))
		return false;

	// We must end inside of water
	if ( !(UTIL_PointContents( end ) & (CONTENTS_WATER|CONTENTS_SLIME)))
		return false;

	trace_t	waterTrace;

	UTIL_TraceLine( start, end, (CONTENTS_WATER|CONTENTS_SLIME), GetOwner(), COLLISION_GROUP_NONE, &waterTrace );

	if ( waterTrace.fraction < 1.0f )
	{
#ifndef CLIENT_DLL
		CEffectData	data;

		data.m_fFlags  = 0;
		data.m_vOrigin = waterTrace.endpos;
		data.m_vNormal = waterTrace.plane.normal;
		data.m_flScale = 8.0f;

		// See if we hit slime
		if ( waterTrace.contents & CONTENTS_SLIME )
		{
			data.m_fFlags |= FX_WATER_IN_SLIME;
		}

		DispatchEffect( "watersplash", data );			
#endif
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGEWeaponMelee::ImpactEffect( trace_t &traceHit )
{
	// See if we hit water (we don't do the other impact effects in this case)
	if ( ImpactWater( traceHit.startpos, traceHit.endpos ) )
		return;

	if ( traceHit.m_pEnt && traceHit.m_pEnt->MyCombatCharacterPointer() )
		return;

	UTIL_ImpactTrace( &traceHit, DMG_CLUB );
}


//------------------------------------------------------------------------------
// Purpose : Starts the swing of the weapon and determines the animation
// Input   : bIsSecondary - is this a secondary attack?
//------------------------------------------------------------------------------
void CGEWeaponMelee::Swing( int bIsSecondary )
{
	trace_t traceHit;

	// Try a ray
	CBaseCombatCharacter *pOwner = GetOwner();
	if ( !pOwner )
		return;

#ifdef CLIENT_DLL
	Vector swingStart = pOwner->EyePosition();
#else
	Vector swingStart = pOwner->Weapon_ShootPosition();
#endif
	Vector forward;

	AngleVectors( pOwner->EyeAngles(), &forward, NULL, NULL );

	Vector swingEnd = swingStart + forward * GetRange();
	UTIL_TraceLine( swingStart, swingEnd, MASK_SHOT | CONTENTS_GRATE, pOwner, COLLISION_GROUP_NONE, &traceHit );
	Activity nHitActivity = ACT_VM_HITCENTER;

#ifndef CLIENT_DLL
	// Like bullets, bludgeon traces have to trace against triggers.
	CTakeDamageInfo triggerInfo( pOwner, pOwner, GetGEWpnData().m_iDamage, DMG_CLUB );
	TraceAttackToTriggers( triggerInfo, traceHit.startpos, traceHit.endpos, vec3_origin );
#endif

	if ( traceHit.fraction == 1.0 )
	{
		float bludgeonHullRadius = 1.732f * BLUDGEON_HULL_DIM;  // hull is +/- 16, so use cuberoot of 2 to determine how big the hull is from center to the corner point

		// Back off by hull "radius"
		swingEnd -= forward * bludgeonHullRadius;

		UTIL_TraceHull( swingStart, swingEnd, g_bludgeonMins, g_bludgeonMaxs, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &traceHit );
		if ( traceHit.fraction < 1.0 && traceHit.m_pEnt )
		{
			Vector vecToTarget = traceHit.m_pEnt->GetAbsOrigin() - swingStart;
			VectorNormalize( vecToTarget );

			float dot = vecToTarget.Dot( forward );

			// YWB:  Make sure they are sort of facing the guy at least...
			if ( dot < 0.70721f )
			{
				// Force amiss
				traceHit.fraction = 1.0f;
			}
			else
			{
				nHitActivity = ChooseIntersectionPointAndActivity( traceHit, g_bludgeonMins, g_bludgeonMaxs, pOwner );
			}
		}
	}

	WeaponSound( SINGLE );

	CBasePlayer *pPlayer = dynamic_cast<CBasePlayer*>( pOwner );
#ifdef GAME_DLL
	if ( pPlayer )
		gamestats->Event_WeaponFired( pPlayer, true, GetClassname() );
#endif

	// -------------------------
	//	Miss
	// -------------------------
	if ( traceHit.fraction == 1.0f )
	{
		nHitActivity = bIsSecondary ? ACT_VM_MISSCENTER2 : ACT_VM_MISSCENTER;

		// We want to test the first swing again
		Vector testEnd = swingStart + forward * GetRange();
		
		// See if we happened to hit water
		ImpactWater( swingStart, testEnd );
	}
	else
	{
#ifdef GAME_DLL
		if ( pPlayer )
			gamestats->Event_WeaponHit( pPlayer, true, GetClassname(), triggerInfo );
#endif
		Hit( traceHit, nHitActivity );
	}

	// Send the anim
	SendWeaponAnim( nHitActivity );

	if ( pPlayer )
		ToGEPlayer(pPlayer)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	//Setup our next attack times
	m_flNextPrimaryAttack = gpGlobals->curtime + max(GetFireRate(),SequenceDuration());
	m_flNextSecondaryAttack = gpGlobals->curtime + max(GetFireRate(),SequenceDuration());
}
