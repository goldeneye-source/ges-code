
#include "cbase.h"
#include "func_break.h"
#include "explode.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CREBreakable : public CBreakable
{
public:
	DECLARE_CLASS( CREBreakable, CBreakable );
	DECLARE_DATADESC();

	CREBreakable();

	virtual void Spawn();

	virtual void RespawnThink();
	virtual void Die();
	virtual bool KeyValue( const char *szKeyName, const char *szValue );

	virtual int OnTakeDamage( const CTakeDamageInfo &info );

private:
	float m_flRespawnTime;

	// These store crucial initial values to enable respawning in the same spot
	int m_iRespawnHealth;
	int m_iTakeDmgValue;
	string_t m_iRespawnName;

	QAngle m_RespawnAngles;
	Vector m_RespawnOrigin;

	Vector m_CollMins;
	Vector m_CollMaxs;
};

LINK_ENTITY_TO_CLASS( func_rebreakable, CREBreakable );

BEGIN_DATADESC( CREBreakable )
	DEFINE_KEYFIELD( m_flRespawnTime, FIELD_FLOAT, "RespawnTime" ),
	// Function Pointers
	DEFINE_THINKFUNC( RespawnThink ),
END_DATADESC()

extern ConVar func_break_max_pieces;
extern ConVar func_break_reduction_factor;

CREBreakable::CREBreakable()
{
	m_iRespawnName = NULL_STRING;
	m_flRespawnTime = 0;
}

void CREBreakable::Spawn( void )
{
	BaseClass::Spawn();

	// Cache off values for our respawning
	m_iTakeDmgValue = m_takedamage;
	m_iRespawnHealth = m_iHealth;
	m_RespawnOrigin = GetAbsOrigin();
	m_RespawnAngles = GetAbsAngles();

	CollisionProp()->WorldSpaceAABB( &m_CollMins, &m_CollMaxs );
	//m_CollMins = CollisionProp()->OBBMins();
	//m_CollMaxs = CollisionProp()->OBBMaxs();
	m_CollMins -= GetAbsOrigin();
	m_CollMaxs -= GetAbsOrigin();
}

//-----------------------------------------------------------------------------
// Purpose: Breaks the breakable. m_hBreaker is the entity that caused us to break.
//-----------------------------------------------------------------------------
void CREBreakable::Die( void )
{
	Vector vecVelocity;// shard velocity
	char cFlag = 0;
	int pitch;
	float fvol;
	
	pitch = 95 + random->RandomInt(0,29);

	if (pitch > 97 && pitch < 103)
	{
		pitch = 100;
	}

	// The more negative m_iHealth, the louder
	// the sound should be.

	fvol = random->RandomFloat(0.85, 1.0) + (abs(m_iHealth) / 100.0);
	if (fvol > 1.0)
	{
		fvol = 1.0;
	}

	const char *soundname = NULL;

	switch (m_Material)
	{
	default:
		break;

	case matGlass:
		soundname = "Breakable.Glass";
		cFlag = BREAK_GLASS;
		break;

	case matWood:
		soundname = "Breakable.Crate";
		cFlag = BREAK_WOOD;
		break;

	case matComputer:
		soundname = "Breakable.Computer";
		cFlag = BREAK_METAL;
		break;

	case matMetal:
		soundname = "Breakable.Metal";
		cFlag = BREAK_METAL;
		break;

	case matFlesh:
	case matWeb:
		soundname = "Breakable.Flesh";
		cFlag = BREAK_FLESH;
		break;

	case matRocks:
	case matCinderBlock:
		soundname = "Breakable.Concrete";
		cFlag = BREAK_CONCRETE;
		break;

	case matCeilingTile:
		soundname = "Breakable.Ceiling";
		break;
	}
    
	if ( soundname )
	{
		if ( m_hBreaker && m_hBreaker->IsPlayer() )
		{
			IGameEvent * event = gameeventmanager->CreateEvent( "break_breakable" );
			if ( event )
			{
				event->SetInt( "userid", ToBasePlayer( m_hBreaker )->GetUserID() );
				event->SetInt( "entindex", entindex() );
				event->SetInt( "material", cFlag );
				gameeventmanager->FireEvent( event );
			}
		}

		CSoundParameters params;
		if ( GetParametersForSound( soundname, params, NULL ) )
		{
			CPASAttenuationFilter filter( this );

			EmitSound_t ep;
			ep.m_nChannel = params.channel;
			ep.m_pSoundName = params.soundname;
			ep.m_flVolume = fvol;
			ep.m_SoundLevel = params.soundlevel;
			ep.m_nPitch = pitch;

			EmitSound( filter, entindex(), ep );	
		}
	}
		
	switch( m_Explosion )
	{
	case expDirected:
		vecVelocity = g_vecAttackDir * -200;
		break;

	case expUsePrecise:
		{
			AngleVectors( m_GibDir, &vecVelocity, NULL, NULL );
			vecVelocity *= 200;
		}
		break;

	case expRandom:
		vecVelocity.x = 0;
		vecVelocity.y = 0;
		vecVelocity.z = 0;
		break;

	default:
		DevMsg("**ERROR - Unspecified gib dir method in func_breakable!\n");
		break;
	}

	Vector vecSpot = WorldSpaceCenter();
	CPVSFilter filter2( vecSpot );

	int iModelIndex = 0;
	CCollisionProperty *pCollisionProp = CollisionProp();

	Vector vSize = pCollisionProp->OBBSize();
	int iCount = ( vSize[0] * vSize[1] + vSize[1] * vSize[2] + vSize[2] * vSize[0] ) / ( 3 * 12 * 12 );

	if ( iCount > func_break_max_pieces.GetInt() )
	{
		iCount = func_break_max_pieces.GetInt();
	}

	ConVarRef breakable_disable_gib_limit( "breakable_disable_gib_limit" );
	if ( !breakable_disable_gib_limit.GetBool() && iCount )
	{
		if ( m_PerformanceMode == PM_NO_GIBS )
		{
			iCount = 0;
		}
		else if ( m_PerformanceMode == PM_REDUCED_GIBS )
		{
			int iNewCount = iCount * func_break_reduction_factor.GetFloat();
			iCount = MAX( iNewCount, 1 );
		}
	}

	if ( m_iszModelName != NULL_STRING )
	{
		for ( int i = 0; i < iCount; i++ )
		{

	#ifdef HL1_DLL
			// Use the passed model instead of the propdata type
			const char *modelName = STRING( m_iszModelName );
			
			// if the map specifies a model by name
			if( strstr( modelName, ".mdl" ) != NULL )
			{
				iModelIndex = modelinfo->GetModelIndex( modelName );
			}
			else	// do the hl2 / normal way
	#endif

			iModelIndex = modelinfo->GetModelIndex( g_PropDataSystem.GetRandomChunkModel(  STRING( m_iszModelName ) ) );

			// All objects except the first one in this run are marked as slaves...
			int slaveFlag = 0;
			if ( i != 0 )
			{
				slaveFlag = BREAK_SLAVE;
			}

			te->BreakModel( filter2, 0.0, 
				vecSpot, pCollisionProp->GetCollisionAngles(), vSize, 
				vecVelocity, iModelIndex, 100, 1, 2.5, cFlag | slaveFlag );
		}
	}

	ResetOnGroundFlags();
	RemoveAllDecals();

	// Don't fire something that could fire myself
	SetName( NULL_STRING );

	// Make ourselfs invisible
	AddEffects( EF_NODRAW );
	VPhysicsDestroyObject();
	SetMoveType( MOVETYPE_PUSH ); //Still allow it to be moved, so that it respawns with parent
	AddSolidFlags( FSOLID_NOT_SOLID );

	SetTouch( NULL );
	// Reset the health
	m_iHealth = m_iRespawnHealth;
	m_iTakeDmgValue = m_takedamage;
	m_takedamage = DAMAGE_NO;
	
	// Fire targets on break
	m_OnBreak.FireOutput( m_hBreaker, this );

	SetThink( &CREBreakable::RespawnThink );
	SetNextThink( gpGlobals->curtime + m_flRespawnTime );

	if ( m_iszSpawnObject != NULL_STRING )
	{
		CBaseEntity::Create( STRING(m_iszSpawnObject), vecSpot, pCollisionProp->GetCollisionAngles(), this );
	}

	if ( Explodable() )
	{
		CBaseEntity *pAttacker = this;
		if ( m_hBreaker->IsPlayer() )
			pAttacker = m_hBreaker;

		ExplosionCreate( vecSpot, pCollisionProp->GetCollisionAngles(), this, GetExplosiveDamage(), GetExplosiveRadius(), true );
	}

	// Place us back where we were
	//UTIL_SetOrigin( this, m_RespawnOrigin );
	//SetAbsAngles( m_RespawnAngles );
}

void CREBreakable::RespawnThink( void )
{
	// We need to recreate the physics so we can test collisions
	CreateVPhysics();

	// iterate on all entities in the vicinity.
	CBaseEntity *pEntity;
	Vector max = m_CollMaxs;
	for ( CEntitySphereQuery sphere( GetAbsOrigin(), max.NormalizeInPlace() ); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
	{
		// Prevent respawn if we are blocked by anything and try again in 1 second
		if (pEntity == this || pEntity->GetSolid() == SOLID_NONE || pEntity->GetSolid() == SOLID_BSP || pEntity->GetSolidFlags() & FSOLID_NOT_SOLID || pEntity == this->GetParent())
			continue;

		if ( Intersects(pEntity) )
		{
			VPhysicsDestroyObject();
			SetNextThink( gpGlobals->curtime + 1.0f );
			return;
		}
	}

	SetSolid( SOLID_BSP );
    SetMoveType( MOVETYPE_PUSH );

	RemoveEffects( EF_NODRAW );
	RemoveSolidFlags( FSOLID_NOT_SOLID );

	SetName( m_iRespawnName );

	// Recreate our physics definition
	SetTouch( &CBreakable::BreakTouch );
	if ( FBitSet( m_spawnflags, SF_BREAK_TRIGGER_ONLY ) )		// Only break on trigger
	{
		SetTouch( NULL );
	}

	// Flag unbreakable glass as "worldbrush" so it will block ALL tracelines
	if ( !IsBreakable() && m_nRenderMode != kRenderNormal )
		AddFlag( FL_WORLDBRUSH );

	// Reset our breaker
	m_hBreaker = NULL;

	m_takedamage = m_iTakeDmgValue;

	if ( m_impactEnergyScale == 0 )
	{
		m_impactEnergyScale = 1.0;
	}
}

bool CREBreakable::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq( szKeyName, "targetname" ) )
	{
		m_iRespawnName = AllocPooledString( szValue );
	}

	return BaseClass::KeyValue( szKeyName, szValue );
}

// This is replicated so that we can take damage but not get deleted by the CBaseEntity class...
int CREBreakable::OnTakeDamage( const CTakeDamageInfo &info )
{
	Vector	vecTemp;
	CTakeDamageInfo subInfo = info;

	// If attacker can't do at least the min required damage to us, don't take any damage from them
	if ( m_takedamage == DAMAGE_NO || info.GetDamage() < m_iMinHealthDmg )
		return 0;

	// Check our damage filter
	if ( !PassesDamageFilter(subInfo) )
	{
		m_bTookPhysicsDamage = false;
		return 1;
	}

	vecTemp = subInfo.GetInflictor()->GetAbsOrigin() - WorldSpaceCenter();

	if (!IsBreakable())
		return 0;

	float flPropDamage = GetBreakableDamage( subInfo, assert_cast<IBreakableWithPropData*>(this) );
	subInfo.SetDamage( flPropDamage );

	// BEGIN CBASEENTITY CODE
	if ( info.GetInflictor() )
	{
		vecTemp = info.GetInflictor()->WorldSpaceCenter() - ( WorldSpaceCenter() );
	}
	else
	{
		vecTemp.Init( 1, 0, 0 );
	}

	// this global is still used for glass and other non-NPC killables, along with decals.
	g_vecAttackDir = vecTemp;
	VectorNormalize(g_vecAttackDir);
		
	// save damage based on the target's armor level
	// figure momentum add (don't let hurt brushes or other triggers move player)
	// physics objects have their own calcs for this: (don't let fire move things around!)
	if ( !IsEFlagSet( EFL_NO_DAMAGE_FORCES ) )
	{
		if ( ( GetMoveType() == MOVETYPE_VPHYSICS ) )
		{
			VPhysicsTakeDamage( info );
		}
		else
		{
			if ( info.GetInflictor() && (GetMoveType() == MOVETYPE_WALK || GetMoveType() == MOVETYPE_STEP) && 
				!info.GetAttacker()->IsSolidFlagSet(FSOLID_TRIGGER) )
			{
				Vector vecDir, vecInflictorCentroid;
				vecDir = WorldSpaceCenter( );
				vecInflictorCentroid = info.GetInflictor()->WorldSpaceCenter( );
				vecDir -= vecInflictorCentroid;
				VectorNormalize( vecDir );

				float flForce = info.GetDamage() * ((32 * 32 * 72.0) / (WorldAlignSize().x * WorldAlignSize().y * WorldAlignSize().z)) * 5;
				
				if (flForce > 1000.0) 
					flForce = 1000.0;
				ApplyAbsVelocityImpulse( vecDir * flForce );
			}
		}
	}
	
	int iNewHealth = m_iHealth;
	if ( m_takedamage != DAMAGE_EVENTS_ONLY )
	{
		// actually do the damage
		iNewHealth -= info.GetDamage();
	}
	// END CBASEENTITY CODE

	if ( !UpdateHealth( iNewHealth, info.GetAttacker() ) )
		return 1;

	// Make a shard noise each time func breakable is hit, if it's capable of taking damage
	if ( m_takedamage == DAMAGE_YES )
	{
		// Don't play shard noise if being burned.
		// Don't play shard noise if cbreakable actually died.
		if ( ( subInfo.GetDamageType() & DMG_BURN ) == false )
		{
			DamageSound();
		}
	}

	return 1;	
}
