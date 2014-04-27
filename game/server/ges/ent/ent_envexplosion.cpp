///////////// Copyright © 2008 LodleNet. All rights reserved. /////////////
//
//   Project     : Server (GES)
//   File        : ent_envexplosion.cpp
//   Description :
//      [TODO: Write the purpose of ent_envexplosion.cpp.]
//
//   Created On: 1/4/2009 5:36:42 PM
//   Created By: Mark Chandler <mailto:admin@lodle.net>
////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "explode.h"
#include "ent_envexplosion.h"
#include "ge_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef GE_USE_ROLLINGEXP

LINK_ENTITY_TO_CLASS( env_explosion, CEnvExplosion );

BEGIN_DATADESC( CEnvExplosion )

	DEFINE_KEYFIELD( m_iMagnitude, FIELD_INTEGER, "iMagnitude" ),
	DEFINE_KEYFIELD( m_iRadiusOverride, FIELD_INTEGER, "iRadiusOverride" ),
	DEFINE_KEYFIELD( m_flDamageForce, FIELD_FLOAT, "DamageForce" ),
	DEFINE_FIELD( m_hInflictor, FIELD_EHANDLE ),
	DEFINE_FIELD( m_iCustomDamageType, FIELD_INTEGER ),

	DEFINE_FIELD( m_iClassIgnore, FIELD_INTEGER ),
	DEFINE_KEYFIELD( m_hEntityIgnore, FIELD_EHANDLE, "ignoredEntity" ),

	// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "Explode", InputExplode),

END_DATADESC()

void CEnvExplosion::Spawn( void )
{ 
	Precache();

	BaseClass::Spawn();

	SetModelName( NULL_STRING );//invisible
	SetSolid( SOLID_NONE );

	AddEffects( EF_NODRAW );
	SetMoveType( MOVETYPE_NONE );
	m_iCustomDamageType = -1;
}

bool CEnvExplosion::KeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "Inflictor") && szValue)
	{
		m_hInflictor = EHANDLE::FromIndex(atoi(szValue));
	}
	else if (FStrEq(szKeyName, "iRadiusOverride") && szValue)
	{
		m_iRadiusOverride = atoi(szValue);
	}
	else
	{
		return BaseClass::KeyValue( szKeyName, szValue );
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for making the explosion explode.
//-----------------------------------------------------------------------------
void CEnvExplosion::InputExplode( inputdata_t &inputdata )
{ 
	trace_t tr;

	Vector vecSpot = GetAbsOrigin() + Vector( 0 , 0 , 8 );
	UTIL_TraceLine( vecSpot, vecSpot + Vector( 0, 0, -40 ), (MASK_SOLID_BRUSHONLY | MASK_WATER), this, COLLISION_GROUP_NONE, &tr );
	
	// Pull out of the wall a bit. We used to move the explosion origin itself, but that seems unnecessary, not to mention a
	// little weird when you consider that it might be in hierarchy. Instead we just calculate a new virtual position at
	// which to place the explosion. We don't use that new position to calculate radius damage because according to Steve's
	// comment, that adversely affects the force vector imparted on explosion victims when they ragdoll.
	Vector vecExplodeOrigin = GetAbsOrigin();
	if ( tr.fraction != 1.0 )
	{
		vecExplodeOrigin = tr.endpos + (tr.plane.normal * 24 );
	}

	CBaseEntity *pAttacker = GetOwnerEntity() ? GetOwnerEntity() : this;

	// Get the radius override if specified
	float fRadius = ( m_iRadiusOverride > 0 ) ? m_iRadiusOverride : ( m_iMagnitude * 1.25f );

	Create_GEExplosion( pAttacker,  m_hInflictor ? m_hInflictor : this, vecExplodeOrigin, m_iMagnitude, fRadius );

	if ( !(m_spawnflags & SF_ENVEXPLOSION_REPEATABLE) )
	{
		GEUTIL_DelayRemove( this, GE_EXP_MAX_DURATION );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CEnvExplosion::DrawDebugTextOverlays( void ) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];

		Q_snprintf(tempstr,sizeof(tempstr),"    magnitude: %i", m_iMagnitude);
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}

#endif