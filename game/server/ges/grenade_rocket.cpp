///////////// Copyright � 2009, Goldeneye: Source. All rights reserved. /////////////
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
	m_bHitPlayer	= false;

	// Default Damages they should be modified by the thrower
	SetDamage( 512 );
	SetDamageRadius( 125 );

	SetModel( "models/weapons/rocket_launcher/w_rocket.mdl" );
	
	// Init our physics definition
	SetSolid( SOLID_VPHYSICS );
	SetMoveType( MOVETYPE_FLY );
	
	// So NPC's can "see" us
	AddFlag( FL_OBJECT );

//	UTIL_SetSize( this, Vector(-10,-5,-5), Vector(10,5,5) );
 	
	SetCollisionGroup( COLLISION_GROUP_GRENADE );

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
	AngleVectors(GetLocalAngles() + QAngle(-90, 0, 0), &m_vUp);

	m_vRight = CrossProduct(m_vForward, m_vUp);

//	SetAbsVelocity( m_vForward * GE_ROCKET_MAXVEL * 0.1f );

	SetThink( &CGERocket::AccelerateThink );
	SetNextThink(gpGlobals->curtime + m_fthinktime);

	m_fthinktime = 0.1;
	m_fFuseTime = gpGlobals->curtime + GE_ROCKET_FUSETIME / MAX(phys_timescale.GetFloat(), 0.01);

	CreateSmokeTrail();

/*
	DevMsg("modifiers are..");
	if (m_iseed1 < 50) // Vertical Sine Wave
		DevMsg(", sine");
	if (m_iseed1 % 50 < 25) // Horizontal Cosine Wave, overlap with sine wave causes spiral.
		DevMsg(", cosine");
	if (m_iseed1 % 25 < 5) // Comes back
		DevMsg(", return");
	if (m_iseed1 % 20 < 5) // Gravity
		DevMsg(", gravity");
	if (m_iseed1 % 10 < 4) // Random Jitter
		DevMsg(", jitter");

	DevMsg(", and seed 1 is %d, seed 2 is %d, seed 3 is %d.", m_iseed1, m_iseed2, m_iseed3); 
*/
}

void CGERocket::AccelerateThink( void ) 
{
	float lifetime = gpGlobals->curtime - m_flSpawnTime;
	if ( lifetime > 0.75f )
	{
		SetAbsVelocity(m_vForward * GE_ROCKET_MAXVEL * (lifetime < 0.75f ? lifetime : 0.75f) * phys_timescale.GetFloat());
		SetThink( &CGERocket::FlyThink );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
	else
	{
		float timescale = phys_timescale.GetFloat();
		float lifetime = (gpGlobals->curtime - m_flSpawnTime) * timescale;
		Vector flypath = m_vForward * GE_ROCKET_MAXVEL * MIN(lifetime, 0.75) * timescale;

		SetAbsVelocity( flypath );
		SetNextThink( gpGlobals->curtime + m_fthinktime );
	}
}

void CGERocket::FlyThink( void )
{
	if ( gpGlobals->curtime > m_fFuseTime )
	{
		Explode();
		return;
	}

	SetAbsVelocity(m_vForward * GE_ROCKET_MAXVEL * 0.75f * phys_timescale.GetFloat());
	SetNextThink( gpGlobals->curtime + 0.1 );
}

// Always explode immediately upon hitting anything
void CGERocket::ExplodeTouch( CBaseEntity *pOther )
{
	if ( !pOther->IsSolid() )
		return;

	// This also handles teammate collisions.
	if ( !g_pGameRules->ShouldCollide( GetCollisionGroup(), pOther->GetCollisionGroup() ) )
		return;

	trace_t tr;
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + (m_vForward * 32), MASK_SOLID, this, GetCollisionGroup(), &tr );
	if( tr.surface.flags & SURF_SKY )
	{
		// Game Over, we hit the sky box, remove the rocket from the world (no explosion)
		StopSound("Weapon_RocketLauncher.Ignite");
		UTIL_Remove(this);
		return;
	}

	// If we are a bot, see if we hit our own NPC
	CGEBotPlayer *pBot = ToGEBotPlayer( GetThrower() );
	if ( pBot && pBot->GetNPC() == pOther )
		return;

	if (pOther != GetThrower())
	{
		// Check if they're a player, if they are deal out instant death.
		if (!m_bHitPlayer && (pOther->IsPlayer() || pOther->IsNPC()))
		{
			Vector playercenter = pOther->GetAbsOrigin() + Vector(0, 0, 38);
			Vector impactforce = playercenter - GetAbsOrigin();
			VectorNormalize(impactforce);

			impactforce *= 1000;

			CTakeDamageInfo rocketinfo(this, GetThrower(), impactforce, GetAbsOrigin(), 398.0f, DMG_BLAST);
			pOther->TakeDamage(rocketinfo);

			m_bHitPlayer = true; // Only deal one direct hit per projectile.
		}
		Explode();
	}
}

int CGERocket::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	if ( m_takedamage == DAMAGE_NO )
		return 0;

	// Manually apply vphysics because BaseCombatCharacter takedamage doesn't call back to CBaseEntity OnTakeDamage
	VPhysicsTakeDamage( inputInfo );

	// Rockets take Blast AND Bullet damage, though blast damage requires more.
	if ((inputInfo.GetDamageType() & DMG_BLAST && inputInfo.GetDamage() > 80) || inputInfo.GetDamageType() & DMG_BULLET)
	{
		m_iHealth -= inputInfo.GetDamage();
		if (m_iHealth <= 0)
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
