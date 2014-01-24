///////////// Copyright © 2012 GoldenEye: Source All rights reserved. /////////////
//
//   Project     : Server (GES)
//   File        : npc_gebase_aiming.cpp
//   Description :
//      Defines aiming specific functions for the GE NPC's
//
//   Created On: 05/26/2012
//   Created By: Jonathan White <KillerMonkey>
////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ai_memory.h"
#include "npc_gebase.h"
#include "npc_bullseye.h"
#include "shot_manipulator.h"
#include "ge_shareddefs.h"
#include "ge_weapon.h"

extern ConVar ai_lead_time;
extern ConVar ai_spread_defocused_cone_multiplier;
extern ConVar ai_debug_aim_positions;
extern ConVar ai_shot_stats;

//
// GetActualShootPosition: Defines where we really WANT to hit
// 
Vector CNPC_GEBase::GetActualShootPosition( const Vector &shootOrigin )
{
	if ( !GetEnemy() )
		return shootOrigin;

	Vector shootTarget;
	if ( GERandom<float>(1.0) <= m_fHeadShotProb )
		shootTarget = GetEnemy()->HeadTarget( shootOrigin );
	else
		shootTarget = GetEnemy()->BodyTarget( shootOrigin );

	// Project the target's location into the future.
	Vector vecTargetPosition = (shootTarget - GetEnemy()->GetAbsOrigin()) + GetEnemyLKP();

	// lead for some fraction of a second.
	return (vecTargetPosition + ( GetEnemy()->GetSmoothedVelocity() * ai_lead_time.GetFloat() ));
}

//
// GetActualShootTrajectory: Modifies where we want to hit with randomness / spread
// 
Vector CNPC_GEBase::GetActualShootTrajectory( const Vector &shootOrigin )
{
	if( !GetEnemy() )
		return GetShootEnemyDir( shootOrigin );

	Vector vecProjectedPosition = GetActualShootPosition( shootOrigin );

	// Draw where we WANT to shoot
	if ( ai_debug_aim_positions.GetBool() )
		NDebugOverlay::Line( shootOrigin, vecProjectedPosition, 0, 255, 0, false, 1.0 );

	Vector shotDir = vecProjectedPosition - shootOrigin;
	VectorNormalize( shotDir );

	// Apply appropriate accuracy.
	bool bUsePerfectAccuracy = false;

	// NOW we have a shoot direction. Where a 100% accurate bullet should go.
	// Modify it by weapon proficiency.
	// construct a manipulator 
	CShotManipulator manipulator( shotDir );

	if ( GetEnemy() && GetEnemy()->Classify() == CLASS_BULLSEYE )
	{
		CNPC_Bullseye *pBullseye = dynamic_cast<CNPC_Bullseye*>(GetEnemy()); 
		if ( pBullseye && pBullseye->UsePerfectAccuracy() )
		{
			bUsePerfectAccuracy = true;
		}
	}

	if ( !bUsePerfectAccuracy )
	{
		manipulator.ApplySpread( GetAttackSpread( GetActiveWeapon(), GetEnemy() ), GetSpreadBias( GetActiveWeapon(), GetEnemy() ) );
		shotDir = manipulator.GetResult();
	}

/*
	// Look for an opportunity to make misses hit interesting things.
	CBaseCombatCharacter *pEnemy;

	pEnemy = GetEnemy()->MyCombatCharacterPointer();

	if ( pEnemy && pEnemy->ShouldShootMissTarget( this ) )
	{
		Vector vecEnd = shootOrigin + shotDir * 8192;
		trace_t tr;

		AI_TraceLine(shootOrigin, vecEnd, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

		if ( tr.fraction != 1.0 && tr.m_pEnt && tr.m_pEnt->m_takedamage != DAMAGE_NO )
		{
			// Hit something we can harm. Just shoot it.
			return manipulator.GetResult();
		}

		// Find something interesting around the enemy to shoot instead of just missing.
		CBaseEntity *pMissTarget = pEnemy->FindMissTarget();

		if ( pMissTarget )
		{
			shotDir = pMissTarget->WorldSpaceCenter() - shootOrigin;
			VectorNormalize( shotDir );
		}
	}
*/

	// Draw where we intend to shoot (before GE Spread)
	if ( ai_debug_aim_positions.GetBool() )
	{
		Vector endPoint;
		VectorMA( shootOrigin, (vecProjectedPosition - shootOrigin).Length(), shotDir, endPoint );
		NDebugOverlay::Line( shootOrigin, endPoint, 0, 0, 255, false, 1.0 );
	}

	if ( ai_shot_stats.GetBool() )
	{
		// TODO?
	}

	return shotDir;
}

//
// GetAttackSpread: Get our shot spread (separate from the gun's spread!)
//
Vector CNPC_GEBase::GetAttackSpread( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget )
{
	// NOTE: GES Using m_fFocusTime to give per-npc focusing
	Vector baseResult = VECTOR_CONE_15DEGREES;
	if ( pWeapon )
		pWeapon->GetBulletSpread( GetCurrentWeaponProficiency() );

	// Base spread mod
	baseResult *= m_fSpreadMod;

	// Dynamic spread mod based on acquisition timing
	AI_EnemyInfo_t *pEnemyInfo = m_pEnemies->Find( pTarget );
	if ( pEnemyInfo )
	{
		if ( m_fFocusTime > 0.0 )
		{
			float timeSinceValidEnemy = gpGlobals->curtime - pEnemyInfo->timeValidEnemy;
			if ( timeSinceValidEnemy < 0 )
				timeSinceValidEnemy = 0;

			if ( timeSinceValidEnemy < m_fFocusTime )
			{
				float coneMultiplier = ai_spread_defocused_cone_multiplier.GetFloat();
				if ( coneMultiplier > 1.0 )
				{
					float scale = 1.0 - timeSinceValidEnemy / m_fFocusTime;
					Assert( scale >= 0.0 && scale <= 1.0 );
					float multiplier = ( (coneMultiplier - 1.0) * scale ) + 1.0;
					baseResult *= multiplier;
				}
			}
		}
	}
	return baseResult;
}

//
// GetSpreadBias: Get the spread's bias (difference from gaussian)
//
float CNPC_GEBase::GetSpreadBias( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget )
{
	// NOTE: GES Using m_fShotBias to give per-npc biasing
	float bias = m_fShotBias;
	if ( pWeapon && bias == 1.0f )
		bias = pWeapon->GetSpreadBias( GetCurrentWeaponProficiency() );

	AI_EnemyInfo_t *pEnemyInfo = m_pEnemies->Find( pTarget );
	if ( pEnemyInfo )
	{
		if ( m_fFocusTime > 0.0 )
		{
			float timeSinceValidEnemy = gpGlobals->curtime - pEnemyInfo->timeValidEnemy;
			if ( timeSinceValidEnemy < 0.0f )
			{
				timeSinceValidEnemy = 0.0f;
			}
			float timeSinceReacquire = gpGlobals->curtime - pEnemyInfo->timeLastReacquired;
			if ( timeSinceValidEnemy < m_fFocusTime )
			{
				float scale = timeSinceValidEnemy / m_fFocusTime;
				Assert( scale >= 0.0 && scale <= 1.0 );
				bias *= scale;
			}
			else if ( timeSinceReacquire < m_fFocusTime ) // handled seperately as might be tuning seperately
			{
				float scale = timeSinceReacquire / m_fFocusTime;
				Assert( scale >= 0.0 && scale <= 1.0 );
				bias *= scale;
			}

		}
	}
	return bias;
}

float CNPC_GEBase::GetReactionDelay( CBaseEntity *pEnemy )
{
	return m_fReactionDelay;
}
