///////////// Copyright © 2010, Goldeneye: Source. All rights reserved. /////////////
// 
// File: grenade_gecommon.h
// Description:
//      Base class for all grenades/rockets/mines for easy casting
//
// Created On: 03/16/2010
// Created By: Jonathan White <killermonkey01@gmail.com>
/////////////////////////////////////////////////////////////////////////////

#ifndef	GEBASEGRENADE_H
#define	GEBASEGRENADE_H

#ifdef _WIN32
#pragma once
#endif

#include "explode.h"
#include "basegrenade_shared.h"
#include "ge_shareddefs.h"
#include "ge_utils.h"

class CGEBaseGrenade : public CBaseGrenade
{
	DECLARE_CLASS( CGEBaseGrenade, CBaseGrenade );

public:
	virtual bool		IsInAir( void ) { return false; };
	virtual GEWeaponID	GetWeaponID( void ) { return WEAPON_NONE; };

	//Use carefully as this can return a null pointer.
	virtual CGEWeapon*	GetSourceWeapon(void) { return m_pWeaponOwner; };
	virtual const char* GetPrintName( void ) { return "#GE_NOWEAPON"; };
	virtual int			GetCustomData( void ) { return 0; };

	virtual void SetSourceWeapon(CGEWeapon *ent)
	{
		m_pWeaponOwner = ent;
	}

	virtual bool CreateVPhysics()
	{
		VPhysicsInitNormal( SOLID_VPHYSICS, 0, false );
		return true;
	}

	virtual void CollisionEventToTrace( int index, gamevcollisionevent_t *pEvent, trace_t &tr )
	{
		UTIL_ClearTrace( tr );
		pEvent->pInternalData->GetSurfaceNormal( tr.plane.normal );
		pEvent->pInternalData->GetContactPoint( tr.endpos );
		tr.plane.dist = DotProduct( tr.plane.normal, tr.endpos );
		VectorMA( tr.endpos, -1.0f, pEvent->preVelocity[index], tr.startpos );
		tr.m_pEnt = pEvent->pEntities[!index];
		tr.fraction = 0.01f;	// spoof!
	}

protected:

	CGEWeapon *m_pWeaponOwner;
	
	virtual void Explode() {
		ExplosionCreate( GetAbsOrigin(), GetAbsAngles(), GetThrower(), GetDamage(), GetDamageRadius(), 
				SF_ENVEXPLOSION_NOSMOKE | SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS, 0.0f, this);

		// Effectively hide us from everyone until the explosion is done with
		GEUTIL_DelayRemove( this, GE_EXP_MAX_DURATION );
	};
};

inline CGEBaseGrenade *ToGEGrenade( CBaseEntity *pEnt )
{
#ifdef _DEBUG
	return dynamic_cast<CGEBaseGrenade*>(pEnt);
#else
	return static_cast<CGEBaseGrenade*>(pEnt);
#endif
}

#endif