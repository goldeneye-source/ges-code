///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_player.h
// Description:
//      see ge_player.cpp
//
// Created On: 23 Feb 08, 16:35
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#include "cbase.h"

#ifdef CLIENT_DLL
	#include "c_ge_player.h"
	#include "prediction.h"
	#include "c_te_effect_dispatch.h"
	#define CRecipientFilter C_RecipientFilter
#else
	#include "ge_player.h"
	#include "ilagcompensationmanager.h"
	#include "ge_stats_recorder.h"
	#include "te_effect_dispatch.h"
	#include "iservervehicle.h"
	#include "soundent.h"
	#include "func_break.h"
	#include "ge_playerresource.h"
#endif

#include "decals.h"
#include "shot_manipulator.h"
#include "takedamageinfo.h"
#include "ai_debug_shared.h"
#include "in_buttons.h"
#include "ge_gamerules.h"
#include "ammodef.h"
#include "ge_weapon.h"
#include "rumble_shared.h"
#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "particle_parse.h"
#include "gemp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define BULLETPROOF_DECALS		"BulletProofGlass"
#define MIN_PENETRATION_DEPTH	1.8f

void GECharSelect_Precache( void *pUser )
{
	CBaseEntity::PrecacheModel( "models/players/random/randomcharacter.mdl" );
}
PRECACHE_REGISTER_FN( GECharSelect_Precache );

#if defined( GAME_DLL )
class CBulletsTraceFilter : public CTraceFilterSimpleList
{
public:
	CBulletsTraceFilter( int collisionGroup ) : CTraceFilterSimpleList( collisionGroup ) {}

	bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle(pHandleEntity);

		if ( m_PassEntities.Count() )
		{
			CBaseEntity *pPassEntity = EntityFromEntityHandle( m_PassEntities[0] );
			if (pEntity && pPassEntity && pEntity->GetOwnerEntity() == pPassEntity &&
				pPassEntity->IsSolidFlagSet(FSOLID_NOT_SOLID) && pPassEntity->IsSolidFlagSet(FSOLID_CUSTOMBOXTEST) &&
				pPassEntity->IsSolidFlagSet(FSOLID_CUSTOMRAYTEST))
			{
				// It's a bone follower of the entity to ignore (toml 8/3/2007)
				return false;
			}
		}

		if (pEntity && (Q_strncmp(pEntity->GetClassname(), "npc_mine_", 9) == 0))
			return true; // Can always shoot mines.

		return CTraceFilterSimpleList::ShouldHitEntity( pHandleEntity, contentsMask );
	}

};
#else
typedef CTraceFilterSimpleList CBulletsTraceFilter;
#endif

extern ConVar ai_debug_shoot_positions;
extern ConVar ai_debug_aim_positions;

//-----------------------------------------------------------------------------
// Consider the weapon's built-in accuracy, this character's proficiency with
// the weapon, and the status of the target. Use this information to determine
// how accurately to shoot at the target.
//-----------------------------------------------------------------------------
Vector CGEPlayer::GetAttackSpread( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget )
{
	if ( pWeapon )
		return pWeapon->GetBulletSpread( WEAPON_PROFICIENCY_PERFECT );
	
	return VECTOR_CONE_15DEGREES;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : step - 
//			fvol - 
//			force - force sound to play
//-----------------------------------------------------------------------------
void CGEPlayer::PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force )
{
#ifdef GAME_DLL
	if ( !GERules()->PlayFootstepSounds( this ) )
		return;
#endif

	CBasePlayer::PlayStepSound( vecOrigin, psurface, fvol, force );
}

//-----------------------------------------------------------------------------
// Purpose: Sets the view model's location, set it closer when ducked
// Input  : Eye position and angle
//-----------------------------------------------------------------------------
void CGEPlayer::CalcViewModelView( const Vector& eyeOrigin, const QAngle& eyeAngles)
{
	static float maxViewPullBack = -3.5f, maxViewPullRight = 1.5f;

	float flViewPullBack = 0, flViewPullRight = 0;
	Vector newEyeOrigin = eyeOrigin;
	Vector	forward, right;
	AngleVectors( eyeAngles, &forward, &right, NULL );

	if ( IsObserver() ) // Dinky fix for now, spectator needs a complete rework.
	{
		flViewPullBack = 0;
		flViewPullRight = 0;
	}
	else if ( IsDucking() )
	{
		// We are in transition, figure out our extent of pull based on our current height
		float fraction = RemapValClamped( GetViewOffset().z, VEC_DUCK_VIEW.z, VEC_VIEW.z, 1.0f, 0 );
		flViewPullBack = maxViewPullBack * fraction;
		flViewPullRight = maxViewPullRight * fraction;
	}
	else if ( IsDucked() )
	{
		flViewPullBack = maxViewPullBack;
		flViewPullRight = maxViewPullRight;
	}

	// Apply the view pull to our eye origin
	if ( IsDucking() || IsDucked() )
	{
		VectorMA( newEyeOrigin, flViewPullBack, forward, newEyeOrigin );
		VectorMA( newEyeOrigin, flViewPullRight, right, newEyeOrigin );
	}

	BaseClass::CalcViewModelView( newEyeOrigin, eyeAngles );
}

float CGEPlayer::GetSequenceGroundSpeed( CStudioHdr *pStudioHdr, int iSequence )
{
	// Aiming causes us to move at roughly half our speed so make sure our animations are consistent
	if ( IsInAimMode() )
		return ( BaseClass::GetSequenceGroundSpeed( pStudioHdr, iSequence ) * 0.5 );
	else
		return BaseClass::GetSequenceGroundSpeed( pStudioHdr, iSequence );
}

void CGEPlayer::RemoveAmmo(int iCount, int iAmmoIndex)
{
	if ( GEMPRules()->InfAmmoEnabled() )
		return;

	BaseClass::RemoveAmmo(iCount, iAmmoIndex);
}

void CGEPlayer::FireBullets( const FireBulletsInfo_t &info )
{
	FireBulletsInfo_t modinfo = info;

#ifndef GAME_DLL
	CUtlVector<C_GEPlayer*> ents;
#endif

	if ( !( info.m_nFlags & FIRE_BULLETS_PENETRATED_SHOT ) )
	{
#ifdef GAME_DLL
		// Move other players back to history positions based on local player's lag
		lagcompensation->StartLagCompensation( this, this->GetCurrentCommand() );
#else
		for ( int i=1; i < gpGlobals->maxClients; i++ )
		{
			C_GEPlayer *pPlayer = ToGEPlayer( UTIL_PlayerByIndex(i) );
			if ( !pPlayer )
				continue;
			pPlayer->AdjustCollisionBounds( true );
			ents.AddToTail( pPlayer );
		}
#endif
	}

	CGEWeapon *pWeapon = dynamic_cast<CGEWeapon *>( GetActiveWeapon() );
	if ( pWeapon )
	{
		// Record the initial shot only
		if ( !(modinfo.m_nFlags & FIRE_BULLETS_PENETRATED_SHOT) )
		{
#ifdef GAME_DLL
			if ( modinfo.m_iShots > 1 ) {
				char num[2];
				Q_snprintf( num, 2, "%i", modinfo.m_iShots );
				gamestats->Event_WeaponFired( this, false, num );
			} else {
				gamestats->Event_WeaponFired( this, true, pWeapon->GetClassname() );
			}

			// This is a local message for stat tracking plugins
			IGameEvent *event = gameeventmanager->CreateEvent( "player_shoot" );
			if( event )
			{
				event->SetInt( "userid", GetUserID() );
				event->SetInt( "weaponid", pWeapon->GetWeaponID() );
				event->SetInt( "mode", IsInAimMode() ? 1 : 0 );
				gameeventmanager->FireEvent( event, true );
			}

			int rumbleEffect = pWeapon->GetRumbleEffect();
			if( rumbleEffect != RUMBLE_INVALID )
				RumbleEffect( rumbleEffect, 0, RUMBLE_FLAG_RESTART );
#endif
			// Set our initial penetration depth
			modinfo.m_flPenetrateDepth = MAX( MIN_PENETRATION_DEPTH, pWeapon->GetMaxPenetrationDepth() );
		}
	}

#ifdef GAME_DLL
	// If we were invulnerable on spawn, well its cancelled now
	// also, this shot counts for 0 damage!
	if (m_bInSpawnInvul && !IsObserver() && GEMPRules()->GetSpawnInvulnCanBreak())
	{
		StopInvul();
	}
#endif

	BaseClass::FireBullets( modinfo );

	if ( !(modinfo.m_nFlags & FIRE_BULLETS_PENETRATED_SHOT) )
	{
#if GAME_DLL
		NoteWeaponFired();
		// Move other players back to history positions based on local player's lag
		lagcompensation->FinishLagCompensation( this );
#else
		for ( int i=0; i < ents.Count(); i++ )
			ents[i]->AdjustCollisionBounds( false );
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fire's bullets
// Input  : bullet information
//-----------------------------------------------------------------------------
void CBaseEntity::FireBullets( const FireBulletsInfo_t &info )
{
	FireBulletsInfo_t modinfo = info;
	CGEWeapon *pWeapon = NULL;
	bool isWepShotgun = false;

	// Player and NPC specific actions
	if ( IsNPC() || IsPlayer() )
	{
		pWeapon = ToGEWeapon( ((CBaseCombatCharacter*)this)->GetActiveWeapon() );

		if ( pWeapon )
		{
			if ( !(modinfo.m_nFlags & FIRE_BULLETS_PENETRATED_SHOT) )
				// Only take the weapon's damage if this is the first shot
				modinfo.m_flDamage = pWeapon->GetGEWpnData().m_iDamage;

			isWepShotgun = pWeapon->IsShotgun(); // We can't use the amount of shots fired for this because automatics can fire multiple bullets per frame.
		}
	}

	// Always replicate all the damage to the player
	modinfo.m_iPlayerDamage = modinfo.m_flDamage;

	// Now we handle the entire sequence so that we can implement bullet penetration properly
	static int	tracerCount;
	trace_t		tr;
	CAmmoDef*	pAmmoDef	= GetAmmoDef();
	int			nDamageType	= pAmmoDef->DamageType(modinfo.m_iAmmoType);

	// the default attacker is ourselves
	CBaseEntity *pAttacker = modinfo.m_pAttacker ? modinfo.m_pAttacker : this;

	// Make sure we don't have a dangling damage target from a recursive call
	if ( g_MultiDamage.GetTarget() != NULL )
	{
		ApplyMultiDamage();
	}
	  
	ClearMultiDamage();
	g_MultiDamage.SetDamageType( nDamageType | DMG_NEVERGIB );
	g_MultiDamage.SetDamageStats( modinfo.m_nFlags );

	Vector vecDir;
	Vector vecEnd;
	
	// Skip multiple entities when tracing
	CBulletsTraceFilter traceFilter( COLLISION_GROUP_NONE );
	traceFilter.SetPassEntity( this ); // Standard pass entity for THIS so that it can be easily removed from the list after passing through a portal
	traceFilter.AddEntityToIgnore( modinfo.m_pAdditionalIgnoreEnt );

	bool bUnderwaterBullets = ShouldDrawUnderwaterBulletBubbles();
	bool bStartedInWater = false;
	if ( bUnderwaterBullets )
	{
		bStartedInWater = ( enginetrace->GetPointContents( modinfo.m_vecSrc ) & (CONTENTS_WATER|CONTENTS_SLIME) ) != 0;
	}

#ifdef GAME_DLL
	// Prediction seed
	int iSeed;

	// If we're worried we can desynch the server and client seed.
	if ( iAlertCode & 1 )
		iSeed = GERandom<int>(INT_MAX);
	else
		iSeed = CBaseEntity::GetPredictionRandomSeed();
#else
	// Prediction seed
	int iSeed = CBaseEntity::GetPredictionRandomSeed();
#endif

	//-----------------------------------------------------
	// Set up our shot manipulator.
	//-----------------------------------------------------
	CShotManipulator Manipulator( modinfo.m_vecDirShooting );
	float flCumulativeDamage = 0.0f;
	Vector shotoffsetarray[4][5] = {
		{ Vector(0, 0, 0), Vector(1, 0, 0), Vector(0, 1, 0), Vector(-1, 0, 0), Vector(0, -1, 0) },//0 degrees
		{ Vector(0, 0, 0), Vector(0.92, 0.38, 0), Vector(-0.38, 0.92, 0), Vector(-0.92, -0.38, 0), Vector(0.38, -0.92, 0) }, //67.5 degrees
		{ Vector(0, 0, 0), Vector(0.7, 0.7, 0), Vector(-0.7, 0.7, 0), Vector(-0.7, -0.7, 0), Vector(0.7, -0.7, 0) }, //45 degrees
		{ Vector(0, 0, 0), Vector(0.38, 0.92, 0), Vector(-0.92, 0.38, 0), Vector(-0.38, -0.92, 0), Vector(0.92, -0.38, 0) }, //22.5 degrees
	}; //4 quadrantal points rotated 67.5 degrees.

	RandomSeed(iSeed);
	int ishotmatrixID = rand() % 4;

	// Now we actually fire the shot(s)
	for (int iShot = 0; iShot < modinfo.m_iShots; iShot++)
	{
		bool bHitWater = false;

		// Prediction seed
		RandomSeed( iSeed );

		// If we're firing multiple shots, and the first shot has to be bang on target, ignore spread
		if ( iShot == 0 && modinfo.m_iShots > 1 && (modinfo.m_nFlags & FIRE_BULLETS_FIRST_SHOT_ACCURATE) )
		{
			vecDir = Manipulator.GetShotDirection();
		}
		else
		{
			Vector adjShotDir = modinfo.m_vecDirShooting;
			Vector adjSpread = modinfo.m_vecSpread;

			// For shotguns we want to take more control of the spread to give the weapon a more consistent performance.
			// This code projects one shot down the center that is fairly accurate, and then 4 more shots at each corner
			// that are substancially less so.  Overlap between the shot spreads is minimal, meaning across the map instant
			// kills and point blank misses are much less likely.
			if (isWepShotgun)
			{
				Vector vecRight, vecUp;

				adjSpread *= 0.64; //Reduce radius to compensate for offsets.
				VectorVectors(adjShotDir, vecRight, vecUp);

				adjShotDir += shotoffsetarray[ishotmatrixID][iShot].x * vecRight * adjSpread.x + shotoffsetarray[ishotmatrixID][iShot].y * vecUp * adjSpread.y;

				if (iShot > 0)
					adjSpread *= 0.5;
				else // Center shot is more accurate
					adjSpread *= 0.35;
			}

#ifdef GAME_DLL
			// If we're worried we can desynch the server and client.
			if ( iAlertCode & 1 )
				vecDir = ApplySpreadGauss(adjSpread, adjShotDir, modinfo.m_iGaussFactor, GERandom<int>(INT_MAX) + iShot);
			else
				vecDir = ApplySpreadGauss(adjSpread, adjShotDir, modinfo.m_iGaussFactor, CBaseEntity::GetPredictionRandomSeed() + iShot);
#else
			vecDir = ApplySpreadGauss(adjSpread, adjShotDir, modinfo.m_iGaussFactor, CBaseEntity::GetPredictionRandomSeed() + iShot);
#endif
		}

		vecEnd = modinfo.m_vecSrc + vecDir * modinfo.m_flDistance;
		AI_TraceLine(modinfo.m_vecSrc, vecEnd, MASK_SHOT, &traceFilter, &tr);

		// Tracker 70354/63250:  ywb 8/2/07
		// Fixes bug where trace from turret with attachment point outside of Vcollide
		//  starts solid so doesn't hit anything else in the world and the final coord 
		//  is outside of the MAX_COORD_FLOAT range.  This cause trying to send the end pos
		//  of the tracer down to the client with an origin which is out-of-range for networking
		if ( tr.startsolid )
		{
			tr.endpos = tr.startpos;
			tr.fraction = 0.0f;
		}

	#ifdef GAME_DLL
		if ( ai_debug_shoot_positions.GetBool() || ai_debug_aim_positions.GetInt() > 1 )
			NDebugOverlay::Line(modinfo.m_vecSrc, vecEnd, 255, 255, 255, false, 1.0f );
	#endif

		if ( bStartedInWater )
		{
		#ifdef GAME_DLL
			Vector vBubbleStart = modinfo.m_vecSrc;
			Vector vBubbleEnd = tr.endpos;
			CreateBubbleTrailTracer( vBubbleStart, vBubbleEnd, vecDir );
		#endif
			bHitWater = true;
		}

		// Now hit all triggers along the ray that respond to shots...
		// Clip the ray to the first collided solid returned from traceline
		CTakeDamageInfo triggerInfo( pAttacker, pAttacker, modinfo.m_flDamage, nDamageType );
		CalculateBulletDamageForce( &triggerInfo, modinfo.m_iAmmoType, vecDir, tr.endpos );
		triggerInfo.ScaleDamageForce( modinfo.m_flDamageForceScale );
		triggerInfo.SetAmmoType( modinfo.m_iAmmoType );
	#ifdef GAME_DLL
		TraceAttackToTriggers( triggerInfo, tr.startpos, tr.endpos, vecDir );
	#endif

		// Make sure given a valid bullet type
		if (modinfo.m_iAmmoType == -1)
		{
			DevMsg("ERROR: Undefined ammo type!\n");
			return;
		}

		Vector vecTracerDest = tr.endpos;

		// do damage, paint decals
		if (tr.fraction != 1.0)
		{
		#ifdef GAME_DLL
			UpdateShotStatistics( tr );

			// For shots that don't need persistance
			int soundEntChannel = ( modinfo.m_nFlags&FIRE_BULLETS_TEMPORARY_DANGER_SOUND ) ? SOUNDENT_CHANNEL_BULLET_IMPACT : SOUNDENT_CHANNEL_UNSPECIFIED;
			CSoundEnt::InsertSound( SOUND_BULLET_IMPACT, tr.endpos, 200, 0.5, this, soundEntChannel );
		#endif

			// See if the bullet ended up underwater + started out of the water
			if ( !bHitWater && ( enginetrace->GetPointContents( tr.endpos ) & (CONTENTS_WATER|CONTENTS_SLIME) ) )
			{
				bHitWater = HandleShotImpactingWater( modinfo, vecEnd, &traceFilter, &vecTracerDest );
			}

			float flActualDamage = modinfo.m_flDamage;
			
			if ( tr.m_pEnt && tr.m_pEnt->IsPlayer() )
			{
				// If we hit a player set them as ignored for any possible next round of
				// bullet firing so that they do not get "double penetrated" through multiple
				// hitboxes
				modinfo.m_pAdditionalIgnoreEnt = tr.m_pEnt;
			}

			int nActualDamageType = nDamageType;
			if ( flActualDamage == 0.0 )
			{
				flActualDamage = g_pGameRules->GetAmmoDamage( pAttacker, tr.m_pEnt, modinfo.m_iAmmoType );
			}
			else
			{
				nActualDamageType = nDamageType | ((flActualDamage > 16) ? DMG_ALWAYSGIB : DMG_NEVERGIB );
			}

			if ( tr.m_pEnt && (!bHitWater || ((modinfo.m_nFlags & FIRE_BULLETS_DONT_HIT_UNDERWATER) == 0)) )
			{
				// Damage specified by function parameter
				CTakeDamageInfo dmgInfo( this, pAttacker, flActualDamage, nActualDamageType );
				CalculateBulletDamageForce( &dmgInfo, modinfo.m_iAmmoType, vecDir, tr.endpos );
				dmgInfo.ScaleDamageForce( modinfo.m_flDamageForceScale );
				dmgInfo.SetAmmoType( modinfo.m_iAmmoType );
				dmgInfo.SetWeapon( pWeapon );
				dmgInfo.SetDamageStats( modinfo.m_nFlags );
				tr.m_pEnt->DispatchTraceAttack( dmgInfo, vecDir, &tr );
			
				if ( ToBaseCombatCharacter( tr.m_pEnt ) )
				{
					flCumulativeDamage += dmgInfo.GetDamage();
				}

				// Do our impact effect
				if ( bStartedInWater || !bHitWater || (modinfo.m_nFlags & FIRE_BULLETS_ALLOW_WATER_SURFACE_IMPACTS) )
				{
					surfacedata_t *psurf = physprops->GetSurfaceData( tr.surface.surfaceProps );
					if ( psurf && psurf->game.material == CHAR_TEX_GLASS )
					{
						// We'll handle the impact decal in HandleBulletPenetration(...)
						// to determine if we show bullet proof or penetrated decals on glass
					}
					else
					{
						DoImpactEffect( tr, nDamageType );

						if ( psurf && (psurf->game.material == CHAR_TEX_WOOD || psurf->game.material == CHAR_TEX_TILE || psurf->game.material == CHAR_TEX_CONCRETE ||
									   psurf->game.material == CHAR_TEX_COMPUTER || psurf->game.material == CHAR_TEX_PLASTIC) )
						{
							DispatchParticleEffect( "ge_impact_add", tr.endpos + tr.plane.normal, vec3_angle );
						}
					}
				}
				else
				{
					// We may not impact, but we DO need to affect ragdolls on the client
					CEffectData data;
					data.m_vStart = tr.startpos;
					data.m_vOrigin = tr.endpos;
					data.m_nDamageType = nDamageType;
					
					DispatchEffect( "RagdollImpact", data );
				}
			}
		}

		// Create a tracer, penetrated shots only get a tracer if they started with one
		if ( modinfo.m_nFlags & FIRE_BULLETS_PENETRATED_SHOT )
		{
			if ( modinfo.m_nFlags & FIRE_BULLETS_FORCE_TRACER )
			{
				trace_t Tracer;
				Tracer = tr;
				Tracer.endpos = vecTracerDest;

				MakeTracer( modinfo.m_vecSrc, Tracer, pAmmoDef->TracerType(modinfo.m_iAmmoType) );
			}
		}
		else if ( ( modinfo.m_iTracerFreq != 0 ) && ( tracerCount++ % modinfo.m_iTracerFreq ) == 0 )
		{
			Vector vecTracerSrc = vec3_origin;
			ComputeTracerStartPosition( modinfo.m_vecSrc, &vecTracerSrc );

			trace_t Tracer;
			Tracer = tr;
			Tracer.endpos = vecTracerDest;
			// Make sure any penetrated shots get a tracer
			modinfo.m_nFlags = modinfo.m_nFlags | FIRE_BULLETS_FORCE_TRACER;

			MakeTracer( vecTracerSrc, Tracer, pAmmoDef->TracerType(modinfo.m_iAmmoType) );
		}

		// Do bullet penetration if applicable
		HandleBulletPenetration( pWeapon, modinfo, tr, vecDir, &traceFilter );

	#ifdef GAME_DLL
		// Per bullet damage!
		ApplyMultiDamage();

		if ( IsPlayer() && flCumulativeDamage > 0.0f )
		{
			CBasePlayer *pPlayer = static_cast< CBasePlayer * >( this );
			if ( pWeapon )
			{
				CTakeDamageInfo dmgInfo( this, pAttacker, flCumulativeDamage, nDamageType );
				dmgInfo.SetWeapon( pWeapon );
				gamestats->Event_WeaponHit( pPlayer, true, pWeapon->GetClassname(), dmgInfo );
			}

			flCumulativeDamage = 0.0f;
		}
	#endif

		iSeed++;
	} // end fire bullets loop
}

inline const Vector CBaseEntity::ApplySpreadGauss(const Vector &vecSpread, const Vector &vecShotDir, int gfactor, int pseed)
{
	Vector vecRight, vecUp;

	VectorVectors(vecShotDir, vecRight, vecUp);

	// establish polar coordinates
	float x, y, radius, angle, sin, cos;

	//Get random angle from 0 to 360 degrees
	angle = random->RandomFloat(0, 6.2831853); //approximation of 2 pi.


	// Calculate a gaussian integer with a given gaussfactor
	// The higher the gaussfactor is, the closer the range becomes to a true normal destribution.

	float gausrand = 0.000;

	if (gfactor < 1) //A GFactor of 0 or less should mean equal destribution across the cone of fire's area
	{
		RandomSeed(pseed - 1); //Avoid having the same seed as the random angle.
		gausrand = rand() % 1000;
		radius = sqrtf(gausrand / 1000); //Radius is rooted here to make the spread a function of circle area, not radius.
	}
	else //A GFactor of 1 should mean equal destribution across the cone of fire's radius
	{
		for (int i = 0; i < gfactor; i++)
		{
			RandomSeed(pseed + i);
			gausrand += rand() % 1000;
		}

		// Adjust gauss range to 0-1 where 0 is the middle of the normal curve.
		// This is a function of radius, not area, meaning shots will tend towards the center even at gfactor = 1!
		radius = abs(gausrand / (gfactor * 500) - 1);
	}

	SinCos(angle, &sin, &cos);

	x = radius * cos;
	y = radius * sin;

	return (vecShotDir + x * vecSpread.x * vecRight + y * vecSpread.y * vecUp);
}

//-----------------------------------------------------------------------------
// Purpose: Handle bullet penetrations
//-----------------------------------------------------------------------------
#ifdef GAME_DLL
ConVar ge_debug_penetration( "ge_debug_penetration", "0", FCVAR_GAMEDLL | FCVAR_CHEAT );
#endif
void CBaseEntity::HandleBulletPenetration( CBaseCombatWeapon *pWeapon, const FireBulletsInfo_t &info, trace_t &tr, const Vector &vecDir, ITraceFilter *pTraceFilter )
{
	// Store the index of bullet proof glass for future use
	static int sBPGlassSurfaceIdx = physprops->GetSurfaceIndex( "bulletproof_glass" );
	static int recurse_index = 0;

	if ( !(info.m_nFlags & FIRE_BULLETS_PENETRATED_SHOT) )
		recurse_index = 0;

	// Protect against infinite recursion!
	if ( ++recurse_index >= 8 )
		return;

	FireBulletsInfo_t refireInfo;
	surfacedata_t *psurf = physprops->GetSurfaceData( tr.surface.surfaceProps );

#ifdef GAME_DLL
	if ( ge_debug_penetration.GetBool() )
	{
		// Draw the shot line from start to finish in green
		debugoverlay->AddLineOverlay( tr.startpos, tr.endpos, 0, 255, 0, true, 3.5f );
	}
#endif

	// If we are bullet proof and we don't have enough penetration, disable any further checks
	// TODO: We should differentiate btwn bulletproof glass and BLOCK BULLETS
	if ( info.m_flPenetrateDepth <= MIN_PENETRATION_DEPTH && tr.m_pEnt && tr.m_pEnt->IsBulletProof() )
	{
		if ( psurf && psurf->game.material == CHAR_TEX_GLASS )
		{
			tr.surface.surfaceProps = sBPGlassSurfaceIdx;
			DoImpactEffect( tr, DMG_BULLET );
		}

		return;
	}

	// Check if we have hit glass so we can do proper effects
	// we don't care about the thickness of glass for penetration purposes
	// unless it is bullet-proof (handled above)
	if ( psurf && psurf->game.material == CHAR_TEX_GLASS )
	{
		refireInfo = info;
		refireInfo.m_nFlags |= FIRE_BULLETS_PENETRATED_SHOT;
		HandleShotImpactingGlass( refireInfo, tr, vecDir, pTraceFilter );
		return;
	}

	// We are done if we can't penetrate further than 1 unit
	if ( info.m_flPenetrateDepth < 1.0f )
		return;

	// Also give up if trace is from outside world.
#ifdef GAME_DLL
	if (engine->GetClusterForOrigin(tr.endpos) == -1)
		return;
#endif

	//Entities get penetrated twice as far.
	float depthmult = 1.0;
	if (tr.DidHitNonWorldEntity() && info.m_flPenetrateDepth >= 2)
		depthmult = 2.0;

	// Move through up to our max penetration
	Vector	testPos = tr.endpos + ( vecDir * info.m_flPenetrateDepth * depthmult);

	trace_t	passTrace;
	// Re-trace as if the bullet had passed right through
	UTIL_TraceLine( testPos, tr.endpos, MASK_SHOT, pTraceFilter, &passTrace );

	float depth = info.m_flPenetrateDepth * (1.0 - passTrace.fraction);

	// If we didn't make it through, we will do a very short range bullet refire to make sure we hit whatever we ended up in.
	// Setting the penetrate depth to 0 here will kill the next refire attempt.
	if (passTrace.startsolid || passTrace.fraction == 1.0f)
	{
		if (passTrace.DidHitNonWorldEntity() && passTrace.m_pEnt != tr.m_pEnt)
			refireInfo.m_flPenetrateDepth = 0;
		else // If we ended up inside the world or the entity we hit with the first trace there's no point in hitting it again.
			return;
	}
	else
		refireInfo.m_flPenetrateDepth = info.m_flPenetrateDepth - depth;


	// If surface is thick enough, impact the other side (will look like an exit effect)
	if (depth/depthmult > 4)
		DoImpactEffect( passTrace, GetAmmoDef()->DamageType(info.m_iAmmoType) );

	if ( tr.m_pEnt && (tr.m_pEnt->IsPlayer() || tr.m_pEnt->IsNPC()) )
	{
		// Don't let this bullet hit us again
		refireInfo.m_pAdditionalIgnoreEnt = tr.m_pEnt;
	}
	else
	{
		refireInfo.m_pAdditionalIgnoreEnt = info.m_pAdditionalIgnoreEnt;
	}

#ifdef GAME_DLL
	if ( ge_debug_penetration.GetBool() )
	{
		// Spit out penetration data and draw penetration line in blue
		debugoverlay->AddTextOverlay( tr.endpos, 1, 3.5f, "Depth: %0.1f", depth );
		debugoverlay->AddTextOverlay( tr.endpos, 2, 3.5f, "Material: %s", physprops->GetPropName(tr.surface.surfaceProps) );
		debugoverlay->AddTextOverlay( tr.endpos, 3, 3.5f, "Penetration Left: %0.1f", info.m_flPenetrateDepth - depth );
		if ( tr.m_pEnt && (tr.m_pEnt->IsPlayer() || tr.m_pEnt->IsNPC()) )
			debugoverlay->AddTextOverlay( tr.endpos, 4, 3.5f, "(Half depth -> Player hit)" );

		debugoverlay->AddLineOverlay( tr.endpos, passTrace.endpos, 0, 0, 255, true, 3.5f );
	}
#endif

	// Refire the round, as if starting from behind the glass
	refireInfo.m_iShots			= 1;
	refireInfo.m_vecSrc			= passTrace.endpos;
	refireInfo.m_vecDirShooting = vecDir;
	refireInfo.m_flDistance		= info.m_flDistance*(1.0f - tr.fraction);
	refireInfo.m_vecSpread		= vec3_origin;
	refireInfo.m_iAmmoType		= info.m_iAmmoType;
	refireInfo.m_iTracerFreq	= info.m_iTracerFreq;
	refireInfo.m_flDamage		= info.m_flDamage;
	refireInfo.m_pAttacker		= info.m_pAttacker ? info.m_pAttacker : this;
	refireInfo.m_nFlags			= info.m_nFlags | FIRE_BULLETS_PENETRATED_SHOT;

	// Refire the shot from the other side of the object
	FireBullets( refireInfo );
}

#define	MAX_GLASS_PENETRATION_DEPTH	16.0f
//-----------------------------------------------------------------------------
// Specific handling of glass impacts
//-----------------------------------------------------------------------------
void CBaseEntity::HandleShotImpactingGlass(const FireBulletsInfo_t &info, trace_t &tr, const Vector &vecDir, ITraceFilter *pTraceFilter)
{
	static int sBPGlassSurfaceIdx = physprops->GetSurfaceIndex("bulletproof_glass");
	// Move through the glass until we're at the other side
	Vector	testPos = tr.endpos + (vecDir * MAX_GLASS_PENETRATION_DEPTH);

	CEffectData	data;

	data.m_vNormal = tr.plane.normal;
	data.m_vOrigin = tr.endpos;

	DispatchEffect("GlassImpact", data);

	trace_t	penetrationTrace;
	FireBulletsInfo_t behindGlassInfo;

	// Re-trace as if the bullet had passed right through
	UTIL_TraceLine(testPos, tr.endpos, MASK_SHOT, pTraceFilter, &penetrationTrace);

	// We somehow didn't hit anything, ragequit.
	if (penetrationTrace.fraction == 1.0f)
		return;

	// We got stuck inside of something
	if (penetrationTrace.startsolid || tr.fraction == 0.0f)
	{
		// We got stuck inside the world, ragequit.
		if (penetrationTrace.DidHitWorld())
		{
			DoImpactEffect(tr, DMG_BULLET);
			return;
		}

		// We got stuck inside an entity, ignore it and keep going.
		if (penetrationTrace.m_pEnt)
			behindGlassInfo.m_pAdditionalIgnoreEnt = penetrationTrace.m_pEnt;
	}
	
	// Do a penetrated bullet decal
	DoImpactEffect( tr, DMG_BULLET );

	// Impact the other side (will look like an exit effect)
	DoImpactEffect( penetrationTrace, GetAmmoDef()->DamageType(info.m_iAmmoType) );

	data.m_vNormal = penetrationTrace.plane.normal;
	data.m_vOrigin = penetrationTrace.endpos;
	
	DispatchEffect( "GlassImpact", data );

#ifdef GAME_DLL
	if ( ge_debug_penetration.GetBool() )
	{
		// Spit out penetration data and draw penetration line in red
		debugoverlay->AddTextOverlay( tr.endpos, 1, 3.5f, "Depth: %0.1f", MAX_GLASS_PENETRATION_DEPTH * tr.fraction );
		debugoverlay->AddTextOverlay( tr.endpos, 2, 3.5f, "Material: %s", physprops->GetPropName(tr.surface.surfaceProps) );
		
		debugoverlay->AddLineOverlay( penetrationTrace.startpos, penetrationTrace.endpos, 255, 0, 0, true, 3.5f );
	}
#endif

	// Refire the round, as if starting from behind the glass
	behindGlassInfo.m_iShots = 1;
	behindGlassInfo.m_vecSrc = penetrationTrace.endpos;
	behindGlassInfo.m_vecDirShooting = vecDir;
	behindGlassInfo.m_vecSpread = vec3_origin;
	behindGlassInfo.m_flDistance = info.m_flDistance*( 1.0f - tr.fraction );
	behindGlassInfo.m_iAmmoType = info.m_iAmmoType;
	behindGlassInfo.m_iTracerFreq = info.m_iTracerFreq;
	behindGlassInfo.m_flDamage = info.m_flDamage;
	behindGlassInfo.m_pAttacker = info.m_pAttacker ? info.m_pAttacker : this;
	behindGlassInfo.m_nFlags = info.m_nFlags;
	behindGlassInfo.m_flPenetrateDepth = info.m_flPenetrateDepth;

	FireBullets( behindGlassInfo );
}

bool CGEPlayer::Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex /*= GE_RIGHT_HAND*/ )
{
	if ( !pWeapon )
		return false;
	
	pWeapon->SetViewModelIndex( viewmodelindex );
	bool res = BaseClass::Weapon_Switch( pWeapon, viewmodelindex );

	if ( res )
	{
	#ifdef GAME_DLL
		GEStats()->Event_WeaponSwitch( this, GetLastWeapon(), GetActiveWeapon() );
	#else
		// Kill off any remaining particle effects
		CBaseViewModel *pViewModel = GetViewModel( viewmodelindex );
		if ( pViewModel )
			pViewModel->ParticleProp()->StopEmission();
	#endif
	}

	ResetAimMode();

	return res;
}

void CGEPlayer::ResetAimMode( bool forced /*=false*/ )
{
	m_flFullZoomTime = 0;

#ifdef CLIENT_DLL
	// Only the client needs to actually unzoom
	if (forced)
		SetZoom(0, true);

	m_iNewZoomOffset = 0;
#endif
}

bool CGEPlayer::IsInAimMode( void )
{
	return (m_flFullZoomTime != 0 && m_flFullZoomTime < gpGlobals->curtime);
}

// AIM MODE
void CGEPlayer::CheckAimMode( void )
{
	CGEWeapon *pWeapon = (CGEWeapon*) GetActiveWeapon();
	if ( !pWeapon )
		return;

	// Don't allow zooming functions while dead, we'll allow pWeapon->m_bInReload for now.
	if ( !IsAlive() || !pWeapon->IsWeaponVisible())
	{
		if (m_flFullZoomTime > 0)
			ResetAimMode(true);

		return;
	}

	// Get out of aim mode if we're currently in it and not pressing the button.
	if ( !(m_nButtons & IN_AIMMODE) && m_flFullZoomTime > 0)
	{
		ResetAimMode();
	}
	else if (m_nButtons & IN_AIMMODE && m_flFullZoomTime == 0)
	{
		if (pWeapon->GetWeaponID() == WEAPON_SNIPER_RIFLE) // Lazy fix for sniper so the variable zoom doesn't cause prediction errors.
			m_flFullZoomTime = gpGlobals->curtime + abs(-50 / WEAPON_ZOOM_RATE);
		else if (pWeapon->GetZoomOffset() != 0)
			m_flFullZoomTime = gpGlobals->curtime + abs(pWeapon->GetZoomOffset() / WEAPON_ZOOM_RATE);
		else
			m_flFullZoomTime = gpGlobals->curtime + GE_AIMMODE_DELAY;

#ifdef CLIENT_DLL
		m_iNewZoomOffset = (int)(pWeapon->GetZoomOffset()); //Update our desired zoom offset.
#endif
	}
}
