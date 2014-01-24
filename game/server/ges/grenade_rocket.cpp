///////////// Copyright © 2009, Goldeneye: Source. All rights reserved. /////////////
// 
// File: grenade_rocket.cpp
// Description:
//      See header
//
// Created On: 12/5/2009
// Created By: Jonathan White <killermonkey01@gmail.com>
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "grenade_rocket.h"
#include "explode.h"
#include "ge_utils.h"
#include "particle_parse.h"
#include "gebot_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define GE_ROCKET_FUSETIME  4.0f
#define GE_ROCKET_ACCELTIME	0.75f
#define GE_ROCKET_MAXVEL	3840.0f

LINK_ENTITY_TO_CLASS( npc_rocket, CGERocket );

BEGIN_DATADESC( CGERocket )
	// Function Pointers
	DEFINE_ENTITYFUNC( ExplodeTouch ),
	DEFINE_THINKFUNC( IgniteThink ),
	DEFINE_THINKFUNC( AccelerateThink ),
	DEFINE_THINKFUNC( FlyThink ),
END_DATADESC()

void CGERocket::Spawn( void )
{
	Precache();
	BaseClass::Spawn();

	m_takedamage	= DAMAGE_YES;
	m_iHealth		= 1;

	// Default Damages they should be modified by the thrower
	SetDamage( 320 );
	SetDamageRadius( 260 );

	SetModel( "models/weapons/rocket_launcher/w_rocket.mdl" );
	
	// Init our physics definition
	SetSolid( SOLID_VPHYSICS );
	SetMoveType( MOVETYPE_FLY );
	
	// So NPC's can "see" us
	AddFlag( FL_OBJECT );

//	UTIL_SetSize( this, Vector(-10,-5,-5), Vector(10,5,5) );
 	
	SetCollisionGroup( COLLISION_GROUP_PROJECTILE );

	SetThink( &CGERocket::IgniteThink );
	SetNextThink( gpGlobals->curtime );

	m_flSpawnTime = gpGlobals->curtime;

	// Explode if we hit anything solid
	SetTouch( &CGERocket::ExplodeTouch );

	AddSolidFlags( FSOLID_NOT_STANDABLE );
}

void CGERocket::Precache( void )
{
	PrecacheModel("models/weapons/rocket_launcher/w_rocket.mdl");
	PrecacheScriptSound( "Weapon_RocketLauncher.Ignite" );
	PrecacheParticleSystem( "ge_rocket_trail" );
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGERocket::CreateSmokeTrail( void )
{
	const CBaseEntity *host = te->GetSuppressHost();
	te->SetSuppressHost( NULL );

	DispatchParticleEffect( "ge_rocket_trail", PATTACH_POINT_FOLLOW, this, "smoke" );

	te->SetSuppressHost( (CBaseEntity*)host );
}

void CGERocket::IgniteThink( void )
{
	EmitSound( "Weapon_RocketLauncher.Ignite" );

	AngleVectors( GetLocalAngles(), &m_vForward );
	SetAbsVelocity( m_vForward * GE_ROCKET_MAXVEL * 0.1f );

	SetThink( &CGERocket::AccelerateThink );
	SetNextThink( gpGlobals->curtime + 0.1f );

	CreateSmokeTrail();
}

void CGERocket::AccelerateThink( void ) 
{
	float lifetime = gpGlobals->curtime - m_flSpawnTime;
	if ( lifetime > 0.75f )
	{
		SetThink( &CGERocket::FlyThink );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
	else
	{
		SetAbsVelocity( m_vForward * GE_ROCKET_MAXVEL * (lifetime < 0.75f ? lifetime : 0.75f) );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}

void CGERocket::FlyThink( void )
{
	if ( gpGlobals->curtime > m_flSpawnTime + GE_ROCKET_FUSETIME )
	{
		Explode();
		return;
	}

	SetNextThink( gpGlobals->curtime + 0.1 );
}

// Always explode immediately upon hitting anything
void CGERocket::ExplodeTouch( CBaseEntity *pOther )
{
	if ( !pOther->IsSolid() )
		return;

	if ( !g_pGameRules->ShouldCollide( GetCollisionGroup(), pOther->GetCollisionGroup() ) )
		return;

	// Don't collide with teammates
	int myteam = GetThrower()->GetTeamNumber();
	if ( myteam >= FIRST_GAME_TEAM && pOther->GetTeamNumber() == myteam && !friendlyfire.GetBool() )
		return;

	trace_t tr;
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + (m_vForward * 32), MASK_SOLID, this, GetCollisionGroup(), &tr );
	if( tr.surface.flags & SURF_SKY )
	{
		// Game Over, we hit the sky box, remove the rocket from the world (no explosion)
		UTIL_Remove(this);
		return;
	}

	// If we are a bot, see if we hit our own NPC
	CGEBotPlayer *pBot = ToGEBotPlayer( GetThrower() );
	if ( pBot && pBot->GetNPC() == pOther )
		return;

	if ( pOther != GetThrower() )
		Explode();
}

int CGERocket::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	if ( m_takedamage == DAMAGE_NO )
		return 0;

	// Manually apply vphysics because BaseCombatCharacter takedamage doesn't call back to CBaseEntity OnTakeDamage
	VPhysicsTakeDamage( inputInfo );

	// Rockets take Blast AND Bullet damage
	if( inputInfo.GetDamageType() & DMG_BLAST )
	{
		m_iHealth -= inputInfo.GetDamage();
		if ( m_iHealth <= 0 )
			Explode();

		return inputInfo.GetDamage();
	}
	else if ( inputInfo.GetDamageType() & DMG_BULLET )
	{
		// Bullet damage transfers ownership to the attacker instead of the thrower
		m_iHealth -= inputInfo.GetDamage();
		if ( m_iHealth <= 0 )
		{
			if ( inputInfo.GetAttacker()->IsPlayer() )
				SetThrower( ToBasePlayer(inputInfo.GetAttacker()) );

			Explode();
		}

		return inputInfo.GetDamage();
	}

	return 0;
}

void CGERocket::Explode()
{
	StopParticleEffects( this );
	StopSound( "Weapon_RocketLauncher.Ignite" );

	BaseClass::Explode();
}
