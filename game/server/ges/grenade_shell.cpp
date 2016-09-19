///////////// Copyright © 2009, Goldeneye: Source. All rights reserved. /////////////
// 
// File: grenade_shell.cpp
// Description:
//      This is what makes the shells go BOOM
//
// Created On: 9/29/2008
// Created By: Jonathan White <killermonkey01@gmail.com>
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "grenade_shell.h"
#include "explode.h"
#include "ge_utils.h"
#include "gamevars_shared.h"
#include "gebot_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define GE_SHELL_FUSETIME  3.8f

LINK_ENTITY_TO_CLASS( npc_shell, CGEShell );

BEGIN_DATADESC( CGEShell )
	// Function Pointers
	DEFINE_ENTITYFUNC( PlayerTouch ),
	DEFINE_THINKFUNC( ExplodeThink ),
END_DATADESC()

void CGEShell::Spawn( void )
{
	Precache();
	BaseClass::Spawn();

	m_takedamage	= DAMAGE_YES;
	m_iHealth		= 1;
	m_iBounceCount	= 0;
	m_bHitPlayer	= false;

	// Default Damages they should be modified by the thrower
	SetDamage( 320 );
	SetDamageRadius( 260 );

	SetModel( "models/weapons/gl/grenadeprojectile.mdl" );
	SetSize( -Vector(3,3,3), Vector(3,3,3) );
	
	// So NPC's can "see" us
	AddFlag( FL_OBJECT );

	// Init our physics definition
	VPhysicsInitNormal( SOLID_VPHYSICS, GetSolidFlags() | FSOLID_TRIGGER, false );
	SetMoveType( MOVETYPE_VPHYSICS );
	SetCollisionGroup( COLLISION_GROUP_GRENADE );

	// Detonate after a set amount of time
	SetThink( &CGEShell::ExplodeThink );
	SetNextThink( gpGlobals->curtime + GE_SHELL_FUSETIME / MAX(phys_timescale.GetFloat(), 0.01) );

	// Instantly Explode when we hit a player
	SetTouch( &CGEShell::PlayerTouch );

	// Give us a little ass juice
	CreateSmokeTrail();

	// No air-drag, lower friction (NOTE: Cannot set specific gravity for HAVOK physics)
	VPhysicsGetObject()->EnableDrag( false );
	float flDamping = 0.0f;
	float flAngDamping = 0.4f;
	VPhysicsGetObject()->SetDamping( &flDamping, &flAngDamping );

	m_flFloorTimeout = gpGlobals->curtime + 0.75f;

	AddSolidFlags( FSOLID_NOT_STANDABLE );
}

void CGEShell::Precache( void )
{
	PrecacheModel("models/weapons/gl/grenadeprojectile.mdl");
	BaseClass::Precache();
}

void CGEShell::SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		pPhysicsObject->SetVelocityInstantaneous( &velocity, &angVelocity );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGEShell::CreateSmokeTrail( void )
{
	if ( m_hSmokeTrail )
		return;

	// Smoke trail.
	if ( (m_hSmokeTrail = SmokeTrail::CreateSmokeTrail()) != NULL )
	{
		m_hSmokeTrail->m_Opacity = 0.25f;
		m_hSmokeTrail->m_SpawnRate = 70;
		m_hSmokeTrail->m_ParticleLifetime = 0.9f;
		m_hSmokeTrail->m_StartColor.Init( 0.5f, 0.5f , 0.5f );
		m_hSmokeTrail->m_EndColor.Init( 0, 0, 0 );
		m_hSmokeTrail->m_StartSize = 10;
		m_hSmokeTrail->m_EndSize = 22;
		m_hSmokeTrail->m_SpawnRadius = 4;
		m_hSmokeTrail->m_MinSpeed = 4;
		m_hSmokeTrail->m_MaxSpeed = 24;
		
		m_hSmokeTrail->SetLifetime( 10.0f );
		m_hSmokeTrail->FollowEntity( this, "smoke" );
	}
}

bool CGEShell::CanExplode()
{
	return m_flFloorTimeout < gpGlobals->curtime;
}

void CGEShell::ExplodeThink() 
{
	if( m_hSmokeTrail )
	{
		m_hSmokeTrail->SetLifetime( 0.5f );
		m_hSmokeTrail = NULL;
	}

	Explode();
}

void CGEShell::PlayerTouch( CBaseEntity *pOther )
{
	// This catches self hits
	if ( !PassServerEntityFilter( this, pOther) )
		return;

	// Don't collide with teammates
	int myteam = GetThrower()->GetTeamNumber();
	if ( myteam >= FIRST_GAME_TEAM && pOther->GetTeamNumber() == myteam && !friendlyfire.GetBool() )
		return;

	// Always explode immediately upon hitting another player, and kill them instantly.
	if ( pOther->IsPlayer() || pOther->IsNPC() )
	{
		SetNextThink( gpGlobals->curtime );

		if (!m_bHitPlayer) // Only do this if we haven't already done it.  We don't want to get 2 directs with one grenade.
		{
			Vector playercenter = pOther->GetAbsOrigin() + Vector(0, 0, 38);
			Vector impactforce = playercenter - GetAbsOrigin();
			VectorNormalize(impactforce);

			impactforce *= 1000;

			m_bHitPlayer = true;
			CTakeDamageInfo shellinfo(this, GetThrower(), impactforce, GetAbsOrigin(), 398.0f, DMG_BLAST);
			pOther->TakeDamage(shellinfo);
		}
	}
}

// This function is in replacement of "Touch" since we are using the VPhysics
// and that does not stimulate "Touch" responses if we collide with the world
void CGEShell::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	// Call the baseclass first so we don't interfere with the normal running of things
	BaseClass::VPhysicsCollision( index, pEvent );

	// Grab what we hit
	CBaseEntity *pOther = pEvent->pEntities[!index];
	if ( !pOther )
		return;

	if ( pOther->IsWorld() )
	{
		surfacedata_t *phit = physprops->GetSurfaceData( pEvent->surfaceProps[!index] );
		if ( phit && phit->game.material == 'X' )
		{
			// Game Over, we hit the sky box, remove the rocket from the world (no explosion)
			PhysCallbackRemove( GetNetworkable() );
			return;
		}
	}

	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS) )
		return;

	if ( !PassServerEntityFilter( this, pOther) )
		return;

	if ( !g_pGameRules->ShouldCollide( GetCollisionGroup(), pOther->GetCollisionGroup() ) )
		return;

	// Don't collide with teammates
	int myteam = GetThrower()->GetTeamNumber();
	if ( myteam >= FIRST_GAME_TEAM && pOther->GetTeamNumber() == myteam )
		return;

	trace_t tr;
	CollisionEventToTrace( index, pEvent, tr );

	// If we hit another player or hit the floor after a wall/ceiling bounce, instantly explode
	if ( pOther->IsPlayer() || pOther->IsNPC() || ( CanExplode() && tr.plane.normal.z < -0.6f ) )
	{
		SetNextThink( gpGlobals->curtime );
		return;
	}

	// Start with our velocity before the collision
	Vector vecCalcVelocity = pEvent->preVelocity[index];
	float pre_speed = vecCalcVelocity.NormalizeInPlace();

#if 0
	// Debug output!
	Vector vecFinalVelocity = pEvent->postVelocity[index];
	float post_speed = vecFinalVelocity.NormalizeInPlace();

	Vector pre_start, post_end;
	VectorMA( GetAbsOrigin(), 75.0f, vecCalcVelocity, pre_start );
	VectorMA( GetAbsOrigin(), 75.0f, vecFinalVelocity, post_end );
#endif

	// First bounce retains 60% energy, subsequent only 45% energy
	if ( !m_iBounceCount )
		vecCalcVelocity *= pre_speed * 0.6f;
	else
		vecCalcVelocity *= pre_speed * 0.45f;

#if 0
	// Debug output!
	DevMsg( "Bounce!! Pre Speed: %0.1f, Post Speed: %0.1f, Calc Speed: %0.1f\n", pre_speed, post_speed, vecCalcVelocity.Length() );
	
	debugoverlay->AddLineOverlay( pre_start, GetAbsOrigin(), 255, 0, 0, false, 10.0f );
	debugoverlay->AddLineOverlay( GetAbsOrigin(), post_end, 0, 255, 0, false, 10.0f );
#endif

	// Set our final velocity based on the calcs above
	//PhysCallbackSetVelocity( pEvent->pObjects[index], vecCalcVelocity );
	
	// Update our bounce count
	++m_iBounceCount;

	m_flFloorTimeout = MIN(m_flFloorTimeout, gpGlobals->curtime + 0.2f);
}

int CGEShell::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	if ( m_takedamage == DAMAGE_NO )
		return 0;

	// Manually apply vphysics because BaseCombatCharacter takedamage doesn't call back to CBaseEntity OnTakeDamage
	VPhysicsTakeDamage( inputInfo );

	// Grenades only suffer blast damage, but are somewhat robust and can pass around the edges of explosions.
	if (inputInfo.GetDamageType() & DMG_BLAST && inputInfo.GetDamage() > 120)
	{
		m_iHealth -= inputInfo.GetDamage();
		if ( m_iHealth <= 0 )
		{
			if (inputInfo.GetAttacker()->IsPlayer())
			{
				SetThrower(inputInfo.GetAttacker()->MyCombatCharacterPointer());
			}
			else if (inputInfo.GetAttacker()->IsNPC())
			{
				CNPC_GEBase *npc = (CNPC_GEBase*)inputInfo.GetAttacker();
				if (npc->GetBotPlayer())
					SetThrower(npc->GetBotPlayer());
			}

			ExplodeThink();
		}

		return inputInfo.GetDamage();
	}

	return 0;
}
