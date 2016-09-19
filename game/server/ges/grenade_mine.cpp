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

#define MINE_LIFETIME		300.0f	// 5 Minutes
#define	MINE_POWERUPTIME	3.0f	// seconds
#define MINE_TIMEDDELAY		3.0f	// seconds

#define MINE_PROXYBEEPDISTSCALE	1.3f	// units from the edge of the explosion dist
#define MINE_PROXYDELAY			0.28f	// seconds
#define MINE_PROXBEEPDELAY		2.5f	// seconds
#define MINE_WIDTH				1.5f	// inches

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

extern ConVar sv_gravity;

void CGEMine::Spawn( void )
{
	Precache();
	BaseClass::Spawn();

	SetSolid( SOLID_VPHYSICS );
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

	//if ( angles.y != 180.0f )
	//	forward.z *= -1; // invert our z only if we aren't on the ceiling

	// Create the explosion 4 units AWAY from the surface we are attached to
	ExplosionCreate( GetAbsOrigin() + forward*4, GetAbsAngles(), inputdata.pActivator, GetDamage(), GetDamageRadius(), 
		SF_ENVEXPLOSION_NOSMOKE | SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS, 0.0f, this);

	// Effectively hide us from everyone until the explosion is done with
	GEUTIL_DelayRemove( this, GE_EXP_MAX_DURATION );
}

void CGEMine::Precache( void )
{
	BaseClass::Precache();
	PrecacheModel("models/weapons/mines/w_proximitymine.mdl");
	PrecacheModel("models/weapons/mines/w_remotemine.mdl");
	PrecacheModel("models/weapons/mines/w_timedmine.mdl");

	PrecacheScriptSound( "weapon_mines.Attach" );
	PrecacheScriptSound( "Mine.Beep" );
}

void CGEMine::SetMineType( GEWeaponID type )
{
	m_iWeaponID = type;

	if ( type == WEAPON_REMOTEMINE ) {
		SetModel("models/weapons/mines/w_remotemine.mdl");
		SetClassname( "npc_mine_remote" );
	} else if ( type == WEAPON_TIMEDMINE ) {
		SetModel("models/weapons/mines/w_timedmine.mdl");
		SetClassname( "npc_mine_timed" );
	} else if ( type == WEAPON_PROXIMITYMINE ) {
		SetModel("models/weapons/mines/w_proximitymine.mdl");
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
			if (pEntity->IsWorld() || pEntity == this || pEntity == this->GetParent())
				continue;

			// Filter out undesirable entities
			if ( pEntity->IsPlayer() )
			{
				CBasePlayer *pPlayer = ToBasePlayer( pEntity );
				// Don't let teammates explode their team's proxy mines.  Also stop observers, or people who have just spawned, from triggering any.
				if ( !GERules()->FPlayerCanTakeDamage(pPlayer, (CBasePlayer*)GetThrower()) || pPlayer->IsObserver() || ToGEPlayer(pPlayer)->IsRadarCloaked() )
					continue;

				// Trace to the player's eyes, then their feet, then their midsection for a more robust detection code.

				UTIL_TraceLine(vecMinePos, pPlayer->EyePosition(), MASK_SHOT_HULL, this, COLLISION_GROUP_DEBRIS, &tr);
				if (tr.fraction != 1.0)
				{
					UTIL_TraceLine(vecMinePos, pEntity->GetAbsOrigin(), MASK_SHOT_HULL, this, COLLISION_GROUP_DEBRIS, &tr);
					if (tr.fraction != 1.0)
					{
						UTIL_TraceLine(vecMinePos, pEntity->GetAbsOrigin() + Vector(0,0,32), MASK_SHOT_HULL, this, COLLISION_GROUP_DEBRIS, &tr);
						if (tr.fraction != 1.0)
							continue;
					}
				}
			}
			else
			{
				if (!pEntity->IsSolid())
					continue;

				// Some non-player entity, so just do a single check to the origin
				UTIL_TraceLine(vecMinePos, pEntity->GetAbsOrigin(), MASK_SHOT_HULL, this, COLLISION_GROUP_DEBRIS, &tr);
				if (tr.fraction != 1.0)
					continue;
			}

			// Distance to the target
			dist = (tr.endpos - vecMinePos).Length();

			Vector entvel, minevel;

			entvel = pEntity->GetAbsVelocity();
			minevel = vec3_origin;

			if (entvel.Length() == 0 && pEntity->VPhysicsGetObject())
				pEntity->VPhysicsGetObject()->GetVelocity(&entvel, NULL);

			if (GetParent())
			{
				if (GetParent()->GetAbsVelocity() == 0 && GetParent()->VPhysicsGetObject())
					pEntity->VPhysicsGetObject()->GetVelocity(&minevel, NULL);
				else
					minevel = GetParent()->GetAbsVelocity();
			}


			// Explode if we are in range and the object is moving fast enough
			if (dist <= range && (entvel - minevel).Length() > 150)
			{
				// Emit the beep sound
				DevMsg("Triggered on %s, which was moving %f \n", pEntity->GetClassname(), entvel.Length());
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
	if (!m_bInAir)
		return;

	if (!pOther->IsSolid())
		return;

	if ( !PassServerEntityFilter( this, pOther) )
		return;

	if ( !g_pGameRules->ShouldCollide(GetCollisionGroup(), pOther->GetCollisionGroup()) )
		return;

	//Mines can collide in midair but only bounce off of eachother.
	if (!strncmp(pOther->GetClassname(), "npc_mine_", 9) && (((CGEMine*)pOther)->m_bInAir || ((CGEMine*)pOther->GetRootMoveParent())->m_bInAir))
	{
		pOther->SetAbsVelocity(GetAbsVelocity() + pOther->GetAbsVelocity());

		if ( GetParent() || pOther->GetParent() || FirstMoveChild() || pOther->FirstMoveChild() ) //Only independant mines can attach to eachother.
		{
			g_EventQueue.AddEvent(this, "Explode", 0, GetOwnerEntity(), this);
			return;
		}
	}

	//If we passed all the checks let's try to attach to it.  This can still fail.
	AlignToSurf(pOther);
}

int CGEMine::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	if ( m_takedamage == DAMAGE_NO )
		return 0;

	// Mines can suffer blast and bullet damage
	else if ((inputInfo.GetDamageType() & DMG_BLAST && inputInfo.GetDamage() > 40) || inputInfo.GetDamageType() & DMG_BULLET)
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

void CGEMine::MineAttach(CBaseEntity *pEntity)
{
	//Mines can collide in midair but only bounce off of eachother.
	if (!strncmp(pEntity->GetClassname(), "npc_mine_", 9) && (GetParent() || FirstMoveChild()))
	{
		g_EventQueue.AddEvent(this, "Explode", 0, GetOwnerEntity(), this); //If we picked up a mine in midair then we're not in a good spot to add ourselves to another higherarchy.
		return;
	}

	// If this is true we must have hit a prop so make sure we follow it if it moves!
	if (pEntity->GetMoveType() != MOVETYPE_NONE)
	{
		SetMoveType(MOVETYPE_VPHYSICS);
		FollowEntity(pEntity, false);
		CreateVPhysics();
		AddSolidFlags(FSOLID_TRIGGER); //This lets us shoot our own mines.
		SetAbsVelocity(Vector(0, 0, 0));
	}
	else
	{
		CreateVPhysics();
		SetMoveType(MOVETYPE_NONE); //Make sure we can't knock the mines off a wall.
		AddSolidFlags(FSOLID_TRIGGER);
		SetAbsVelocity(Vector(0, 0, 0));
	}

	m_bInAir = false;
	m_flAttachTime = gpGlobals->curtime;

	EmitSound("weapon_mines.Attach");

	RemoveFlag(FL_DONTTOUCH);
}

bool CGEMine::AlignToSurf( CBaseEntity *pSurface )
{
	Vector vecVelocity, vecAiming, vecUp, vecRight, traceOrigin;
	float speed;
	trace_t tr;

	speed = GetAbsVelocity().Length();
	vecVelocity = GetAbsVelocity();
	vecAiming = vecVelocity / speed;

	VectorVectors(vecAiming, vecUp, vecRight);
	Vector traceArray[9] = { Vector(0, 0, 0), (vecRight + vecUp)*0.7, (vecRight - vecUp)*0.7, (-vecRight + vecUp)*0.7, (-vecRight - vecUp)*0.7, vecUp, vecRight, -vecUp, -vecRight };
	for (int i = 0; i < 9; i++)
	{
		traceOrigin = GetAbsOrigin() + traceArray[i] * MINE_WIDTH;

		UTIL_TraceLine(traceOrigin, traceOrigin + (vecAiming * 4), MASK_SOLID, this, GetCollisionGroup(), &tr);

		if (tr.fraction < 1.0 && tr.m_pEnt == pSurface)
			break;
	}

	// If we still haven't hit something it's time for desperate mesaures.
	if (tr.fraction == 1.0)
	{
		Vector originTrace = ( pSurface->GetAbsOrigin() - GetAbsOrigin() );

		UTIL_TraceLine(GetAbsOrigin(), GetAbsOrigin() + originTrace / originTrace.Length() * 6, MASK_SOLID, this, GetCollisionGroup(), &tr);
	}

	if( tr.surface.flags & SURF_SKY )
	{
		// Game Over, we hit the sky box, remove the mine from the world
		UTIL_Remove(this);
		return false;
	}

	if (tr.fraction < 1.0)
	{
		CBaseEntity *pEntity = tr.m_pEnt;
		if ( pEntity )
		{
			QAngle angles;
			VectorAngles(tr.plane.normal, angles);
			VPhysicsDestroyObject();

			angles.x += 90.0f;

			SetAbsOrigin( tr.endpos );

			SetAbsAngles(angles);

			MineAttach(pEntity);

			return true;
		}
	}
	return false;
}
