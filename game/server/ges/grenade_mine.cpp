///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: grenade_mine.cpp
// Description:
//      This is what makes the mines go BOOM
//
// Created On: 8/2/2008
// Created By: Jonathan White <killermonkey01@gmail.com> (Some code taken from Scott Lloyd)
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ge_player.h"
#include "soundenvelope.h"
#include "engine/IEngineSound.h"
#include "eventqueue.h"
#include "explode.h"
#include "Sprite.h"

#include "grenade_mine.h"
#include "npc_tknife.h"
#include "grenade_ge.h"
#include "ge_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MINE_LIFETIME		180.0f	// 3 Minutes
#define	MINE_POWERUPTIME	3.0f	// seconds
#define MINE_TIMEDDELAY		3.0f	// seconds

#define MINE_PROXYBEEPDISTSCALE	1.3f	// units from the edge of the explosion dist
#define MINE_PROXYDELAY			0.28f	// seconds
#define MINE_PROXBEEPDELAY		2.5f	// seconds

BEGIN_DATADESC( CGEMine )

	DEFINE_FIELD( m_bInAir, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_vLastPosition, FIELD_POSITION_VECTOR ),

	// Function Pointers
	DEFINE_THINKFUNC( MineThink ),
	DEFINE_ENTITYFUNC( MineTouch ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Explode", InputExplode),

END_DATADESC()

LINK_ENTITY_TO_CLASS( npc_mine, CGEMine );

CGEMine::CGEMine( void )
{
	m_flAttachTime = m_flSpawnTime = -1;
	m_vLastPosition.Init();
	m_iWeaponID = WEAPON_NONE;
}

CGEMine::~CGEMine( void )
{
}

void CGEMine::Spawn( void )
{
	Precache();
	BaseClass::Spawn();

	SetModel( "models/weapons/mines/w_mine.mdl" );

	SetSolid( SOLID_BBOX );
	SetMoveType( MOVETYPE_FLYGRAVITY );

	SetCollisionGroup( COLLISION_GROUP_MINE );

	SetThink( &CGEMine::MineThink );
	SetNextThink( gpGlobals->curtime + 0.1f );

	SetTouch( &CGEMine::MineTouch );
	
	// So NPC's can "see" us
	AddFlag( FL_OBJECT );

	// Default Damages they should be modified by the thrower
	SetDamage( 150 );
	SetDamageRadius( 120 );

	m_takedamage = DAMAGE_YES;
	m_iHealth	 = 5;
	m_bExploded  = false;
	m_bPreExplode = false;

	SetGravity( UTIL_ScaleForGravity( 560 ) );	// slightly lower gravity

	m_bInAir = true;

	m_flSpawnTime = gpGlobals->curtime;
	m_vLastPosition	= GetAbsOrigin();
}

void CGEMine::InputExplode( inputdata_t &inputdata )
{
	// We can only blow up ONCE :)
	if ( m_bExploded )
		return;

	Vector forward;
	QAngle angles = GetAbsAngles();
	angles.x -= 90; // To bring us back to our REAL normal

	m_bExploded = true;

	// Convert our mine angles to a Normal Vector
	AngleVectors( angles, &forward );
	if ( angles.y != 180.0f )
		forward.z *= -1; // invert our z only if we aren't on the ceiling

	// Create the explosion 16 units AWAY from the surface we are attached to
	ExplosionCreate( GetAbsOrigin() - forward*16.0f, GetAbsAngles(), inputdata.pActivator, GetDamage(), GetDamageRadius(), 
		SF_ENVEXPLOSION_NOSMOKE | SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS, 0.0f, this);

	// Effectively hide us from everyone until the explosion is done with
	GEUTIL_DelayRemove( this, GE_EXP_MAX_DURATION );
}

void CGEMine::Precache( void )
{
	BaseClass::Precache();
	PrecacheModel( "models/weapons/mines/w_mine.mdl" );
	PrecacheScriptSound( "weapon_mines.Attach" );
	PrecacheScriptSound( "Mine.Beep" );
}

void CGEMine::SetMineType( GEWeaponID type )
{
	m_iWeaponID = type;

	if ( type == WEAPON_REMOTEMINE ) {
		m_nSkin = FAMILY_GROUP_REMOTE;
		SetClassname( "npc_mine_remote" );
	} else if ( type == WEAPON_TIMEDMINE ) {
		m_nSkin = FAMILY_GROUP_TIMED;
		SetClassname( "npc_mine_timed" );
	} else if ( type == WEAPON_PROXIMITYMINE ) {
		m_nSkin = FAMILY_GROUP_PROXIMITY;
		SetClassname( "npc_mine_proximity" );
	} else {
		Warning( "Mine set to invalid weapon ID!\n");
	}
}

void CGEMine::MineThink( void )
{
	if ( gpGlobals->curtime > m_flSpawnTime + MINE_LIFETIME )
	{
		GEUTIL_DelayRemove( this, GE_EXP_MAX_DURATION );
		return;
	}

	// Don't think if we are still flying around
	if ( m_bInAir )
		goto endOfThink;

	if ( GetMoveParent() )
	{
		// Uh oh, we had attached ourselves to a prop, but it has since gone bye bye in a non explosive way
		// so lets make things a little more interesting
		if ( !GetMoveParent()->IsSolid() )
		{
			g_EventQueue.AddEvent( this, "Explode", MINE_PROXYDELAY, GetThrower(), this );
			return;
		}
	}

	if ( GetMineType() == WEAPON_PROXIMITYMINE )
	{
		// Give some delay in there so it doesn't blow up the setter right away
		if ( gpGlobals->curtime < m_flAttachTime + MINE_POWERUPTIME )
			goto endOfThink;

		CBaseEntity *pEntity = NULL;
		Vector vecMinePos = GetAbsOrigin();
		trace_t tr;
		float dist, range = GetDamageRadius() * 0.9f;
		
		for ( CEntitySphereQuery sphere( vecMinePos, GetDamageRadius() ); 
				( pEntity = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
		{
			UTIL_TraceLine( vecMinePos, pEntity->GetAbsOrigin(), MASK_SHOT_HULL, this, COLLISION_GROUP_DEBRIS, &tr );
			// If we can't see this entity, ignore it
			if ( tr.fraction != 1.0 )
				continue;

			const char *className = pEntity->GetClassname();

			// Filter out undesirable entities
			if ( pEntity->IsPlayer() )
			{
				CBasePlayer *pPlayer = ToBasePlayer( pEntity );
				// Don't let teammates explode their team's proxy mines
				if ( !GERules()->FPlayerCanTakeDamage( pPlayer, (CBasePlayer*)GetThrower() ) || pPlayer->IsObserver() )
					continue;
			}
			else if ( !pEntity->IsNPC() && !Q_strnicmp( className, "npc_", 4 ) )
			{
				if ( !Q_stricmp( className, "npc_tknife" ) && !((CGETKnife*)pEntity)->IsInAir() )
					continue;
				else if ( !Q_strnicmp( className, "npc_mine", 8 ) && !((CGEMine*)pEntity)->IsInAir() )
					continue;
				else if ( !Q_stricmp( className, "npc_grenade" ) && !((CGEGrenade*)pEntity)->IsInAir() )
					continue;
				else if ( !Q_stricmp( className, "npc_shell" ) )
					continue;
				else if ( !Q_stricmp( className, "npc_rocket" ) )
					continue;
			}
			else
				continue;

			// Distance to the target
			dist = (tr.endpos - vecMinePos).Length();

			// Explode if we are in range
			if( dist <= range )
			{
				// Emit the beep sound
				EmitSound("Mine.Beep");
				g_EventQueue.AddEvent( this, "Explode", MINE_PROXYDELAY, GetThrower(), this );
			}
		}
	}
	else if ( GetMineType() == WEAPON_TIMEDMINE )
	{
		if ( gpGlobals->curtime > m_flAttachTime + MINE_TIMEDDELAY )
			g_EventQueue.AddEvent( this, "Explode", 0, GetThrower(), this );
	}

endOfThink:
	SetNextThink( gpGlobals->curtime + 0.1 );
}

void CGEMine::MineTouch( CBaseEntity *pOther )
{
	if ( !pOther->IsSolid() )
		return;
	
	if ( !PassServerEntityFilter( this, pOther) )
		return;

	if ( !g_pGameRules->ShouldCollide( GetCollisionGroup(), pOther->GetCollisionGroup() ) )
		return;

	if( !m_bInAir )
		return;

	//If we can't align it properly, must have barely touched it, let it continue.
	if( !AlignToSurf( pOther ) )
		return;

	m_bInAir = false;
	m_flAttachTime = gpGlobals->curtime;

	EmitSound("weapon_mines.Attach");

	SetTouch( NULL );
}

int CGEMine::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	if ( m_takedamage == DAMAGE_NO )
		return 0;

	// Mines can suffer blast and bullet damage
	if( inputInfo.GetDamageType() & DMG_BLAST )
	{
		m_iHealth -= inputInfo.GetDamage();
		if ( m_iHealth <= 0 )
			g_EventQueue.AddEvent( this, "Explode", 0, GetThrower(), this );

		return inputInfo.GetDamage();
	}
	else if ( inputInfo.GetDamageType() & DMG_BULLET )
	{
		m_iHealth -= inputInfo.GetDamage();
		if ( m_iHealth <= 0 )
			g_EventQueue.AddEvent( this, "Explode", 0, inputInfo.GetAttacker(), this );

		return inputInfo.GetDamage();
	}

	return 0;
}

const char* CGEMine::GetPrintName( void )
{
	if ( GetWeaponID() == WEAPON_PROXIMITYMINE )
		return "#GE_ProximityMine";
	else if ( GetWeaponID() == WEAPON_REMOTEMINE )
		return "#GE_RemoteMine";
	else if ( GetWeaponID() == WEAPON_TIMEDMINE )
		return "#GE_TimedMine";
	else
		return "Mine";
}

bool CGEMine::AlignToSurf( CBaseEntity *pSurface )
{
	Vector vecAiming;
	trace_t tr;

	vecAiming = GetAbsVelocity() / GetAbsVelocity().Length();

	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + (vecAiming * 64), MASK_SOLID, this, GetCollisionGroup(), &tr );

	if( tr.surface.flags & SURF_SKY )
	{
		// Game Over, we hit the sky box, remove the mine from the world
		UTIL_Remove(this);
		return false;
	}
	
	if (tr.fraction < 1.0)
	{
		CBaseEntity *pEntity = tr.m_pEnt;
		if ( pEntity && !(pEntity->GetFlags() & FL_CONVEYOR) )
		{
			QAngle angles;
			VectorAngles(tr.plane.normal, angles);

			angles.x += 90.0f;

			SetAbsAngles( angles );
			SetAbsOrigin( tr.endpos );

			// If this is true we must have hit a prop so make sure we follow it if it moves!
			if ( pSurface->GetMoveType() != MOVETYPE_NONE )
			{
				VPhysicsInitNormal( SOLID_BBOX, GetSolidFlags() | FSOLID_TRIGGER, false );
				SetMoveType( MOVETYPE_VPHYSICS );
				FollowEntity( pEntity, false );
			}
			else
			{
				SetMoveType( MOVETYPE_NONE );
				IPhysicsObject *pObject = VPhysicsInitNormal( SOLID_BBOX, GetSolidFlags() | FSOLID_TRIGGER, false );
				pObject->EnableMotion( false );
				AddSolidFlags( FSOLID_NOT_SOLID );
			}

			return true;
		}
	}
	return false;
}