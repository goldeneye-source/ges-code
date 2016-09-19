///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: grenade_ge.cpp
// Description:
//      This is what makes the grenades go BOOM
//
// Created On: 9/29/2008
// Created By: Jonathan White <killermonkey01@gmail.com> (Some code taken from HL2)
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "grenade_ge.h"
#include "explode.h"
#include "ge_utils.h"
#include "npc_gebase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const float GRENADE_REFLECTION_ATTENUATION = 0.2f;

LINK_ENTITY_TO_CLASS( npc_grenade, CGEGrenade );

BEGIN_DATADESC( CGEGrenade )
	// Function Pointers
	DEFINE_THINKFUNC( DelayThink ),
END_DATADESC()

void CGEGrenade::Spawn( void )
{
	Precache();
	BaseClass::Spawn();
	SetModel( "models/weapons/grenade/w_grenade.mdl" );
	
	// So NPC's can "see" us
	AddFlag( FL_OBJECT );

	m_takedamage	= DAMAGE_YES;
	m_iHealth		= 1;
	m_bHitSomething = false;
	m_bDroppedOnDeath = false;

	// Default Damages they should be modified by the thrower
	SetDamage( 320 );
	SetDamageRadius( 260 );

	SetSize( -Vector(4,4,4), Vector(4,4,4) );
	SetCollisionGroup( COLLISION_GROUP_MINE );
	
	// Init our physics definition
	VPhysicsInitNormal( SOLID_VPHYSICS, GetSolidFlags() | FSOLID_TRIGGER, false );
	SetMoveType( MOVETYPE_VPHYSICS );

	// Don't think until we have a time to think about!
	SetThink( NULL );

	// Bounce if we hit something!
	SetTouch( &BaseClass::BounceTouch );

	AddSolidFlags( FSOLID_NOT_STANDABLE );
}

void CGEGrenade::Precache( void )
{
	BaseClass::Precache();
	PrecacheModel("models/weapons/grenade/w_grenade.mdl");
}

void CGEGrenade::SetTimer( float detonateDelay )
{
	m_flDetonateTime = gpGlobals->curtime + detonateDelay / MAX(phys_timescale.GetFloat(), 0.01);

	SetThink( &CGEGrenade::DelayThink );
	SetNextThink( gpGlobals->curtime );
}

void CGEGrenade::DelayThink() 
{
	if( gpGlobals->curtime > m_flDetonateTime )
	{
		Explode();
		return;
	}

	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		Vector vel;
		pPhysicsObject->GetVelocity( &vel, NULL );
		if ( vel.Length() < 50.0f )
			SetCollisionGroup( COLLISION_GROUP_MINE );
	}

	SetNextThink( gpGlobals->curtime + 0.05 );
}

void CGEGrenade::SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		pPhysicsObject->AddVelocity( &velocity, &angVelocity );
	}
}

// This function is in replacement of "Touch" since we are using the VPhysics
// and that does not stimulate "Touch" responses if we collide with the world
void CGEGrenade::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	// Call the baseclass first so we don't interfere with the normal running of things
	BaseClass::VPhysicsCollision( index, pEvent );

	// Grab what we hit
	CBaseEntity *pOther = pEvent->pEntities[!index];

	if ( pOther->IsWorld() )
	{
		m_bHitSomething = true;

		surfacedata_t *phit = physprops->GetSurfaceData( pEvent->surfaceProps[!index] );
		if ( phit->game.material == 'X' )
		{
			// Game Over, we hit the sky box, remove the rocket from the world (no explosion)
			PhysCallbackRemove( GetNetworkable() );
			return;
		}
	}
}

int CGEGrenade::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	if ( m_takedamage == DAMAGE_NO )
		return 0;

	// Manually apply vphysics because BaseCombatCharacter takedamage doesn't call back to CBaseEntity OnTakeDamage
	VPhysicsTakeDamage( inputInfo );

	// Grenades take Blast AND Bullet damage
	if ((inputInfo.GetDamage() > 160 && inputInfo.GetDamageType() & DMG_BLAST) || inputInfo.GetDamageType() & DMG_BULLET)
	{
		// Bullet damage transfers ownership to the attacker instead of the thrower
		m_iHealth -= inputInfo.GetDamage();
		if ( m_iHealth <= 0 )
		{
			if ( inputInfo.GetAttacker()->IsPlayer() )
			{
				SetThrower( inputInfo.GetAttacker()->MyCombatCharacterPointer() );
			}
			else if ( inputInfo.GetAttacker()->IsNPC() )
			{
				CNPC_GEBase *npc = (CNPC_GEBase*) inputInfo.GetAttacker();
				if ( npc->GetBotPlayer() )
					SetThrower( npc->GetBotPlayer() );
			}

			Explode();
		}

		return inputInfo.GetDamage();
	}

	return 0;
}

