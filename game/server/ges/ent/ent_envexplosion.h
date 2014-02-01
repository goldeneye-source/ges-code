///////////// Copyright © 2008 LodleNet. All rights reserved. /////////////
//
//   Project     : Server (GES)
//   File        : ent_geexplosion.h
//   Description :
//      [TODO: Write the purpose of ent_geexplosion.h.]
//
//   Created On: 12/30/2008 5:04:04 PM
//   Created By: Mark Chandler <mailto:admin@lodle.net>
////////////////////////////////////////////////////////////////////////////

#ifndef MC_ENT_GEEXPLOSION_H
#define MC_ENT_GEEXPLOSION_H
#ifdef _WIN32
#pragma once
#endif

#ifdef GE_USE_ROLLINGEXP

CBaseEntity* Create_GEExplosion( CBaseEntity* owner, CBaseEntity* activator, const Vector pos, float dmg, float dmgRadius );
CBaseEntity* Create_GEExplosion( CBaseEntity* activator, const Vector pos, float dmg, float dmgRadius );

class CEnvExplosion : public CPointEntity
{
	DECLARE_CLASS( CEnvExplosion, CPointEntity );

public:
	CEnvExplosion( void ) { };

	void Spawn();
	void SetCustomDamageType( int iType ) { m_iCustomDamageType = iType; }
	bool KeyValue( const char *szKeyName, const char *szValue );
	int DrawDebugTextOverlays(void);

	// Input handlers
	void InputExplode( inputdata_t &inputdata );

	DECLARE_DATADESC();

	int m_iMagnitude;		// how large is the fireball? how much damage?
	int m_iRadiusOverride;	// For use when m_iMagnitude results in larger radius than designer desires.
	float m_flDamageForce;	// How much damage force should we use?

	EHANDLE m_hInflictor;
	int m_iCustomDamageType;

	// passed along to the RadiusDamage call
	int m_iClassIgnore;
	EHANDLE m_hEntityIgnore;
};

#endif

#endif //MC_ENT_GEEXPLOSION_H
