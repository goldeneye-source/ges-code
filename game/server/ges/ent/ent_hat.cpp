///////////// Copyright � 2008 LodleNet. All rights reserved. /////////////
//
//   Project     : Server (GES)
//   File        : ent_hat.cpp
//   Description :
//      [TODO: Write the purpose of ent_hat.cpp.]
//
//   Created On: 12/28/2008 5:10:22 PM
//   Created By: Mark Chandler <mailto:admin@lodle.net>
////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ent_hat.h"
#include "ge_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CGE_Hat : public CBaseAnimating
{
public:
	DECLARE_CLASS( CGE_Hat, CBaseAnimating );
	DECLARE_DATADESC();

	void Spawn( void );
	void Precache( void );

	void RemoveThink();
	void KnockHatOff();
	void Activate();
	void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );

private:
	char *m_szModel;
};

LINK_ENTITY_TO_CLASS( prop_hat, CGE_Hat );
BEGIN_DATADESC( CGE_Hat )
	DEFINE_KEYFIELD(m_szModel, FIELD_STRING, "HatModel"),
	DEFINE_THINKFUNC(RemoveThink),
END_DATADESC()


void CGE_Hat::Precache( void )
{
	if (m_szModel)
		PrecacheModel( m_szModel );
}

void CGE_Hat::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	ApplyAbsVelocityImpulse(info.GetDamageForce());
	BaseClass::TraceAttack(info, vecDir, ptr);
}

void CGE_Hat::Spawn( void )
{
	if (!m_szModel || m_szModel[0] == '\0')
	{
		Warning("Tried to spawn hat with NULL Model.\n");
		UTIL_Remove(this);
		return;
	}

	Precache();
	SetModel( m_szModel );

	SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	VPhysicsInitNormal( SOLID_VPHYSICS, 0, false );
}

void CGE_Hat::Activate()
{
	KnockHatOff();
}

void CGE_Hat::KnockHatOff()
{
	StopFollowingEntity();

	VPhysicsDestroyObject();
	VPhysicsInitNormal( SOLID_VPHYSICS, 0, false );
	
	SetThink( &CGE_Hat::RemoveThink );
	SetNextThink( gpGlobals->curtime + 10.0);
}

void CGE_Hat::RemoveThink()
{
	UTIL_Remove(this);
}

CBaseEntity *CreateNewHat( const Vector &position, const QAngle &angles, const char* hatModel)
{
	if (!hatModel || hatModel[0] == '\0')
	{
		Warning("Trying to create a Null Hat!!\n");
		return NULL;
	}

	CGE_Hat *pHat = (CGE_Hat *)CBaseEntity::CreateNoSpawn( "prop_hat", position, angles, NULL );
	pHat->KeyValue("HatModel", hatModel);
	DispatchSpawn( pHat );
	return pHat;
}

