///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_point_follower.h
// Description:
//      Follows a given target entity independantly on server and client.
//		
//
// Created On: 2/15/2016
// Created By: Check Github for list of contributors
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"

class CGEFollower : public CBaseEntity
{
public:
	DECLARE_CLASS(CGEFollower, CBaseEntity);
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CGEFollower();

	string_t m_sTargetEnt; // Name of the entity that this one will follow around.

	virtual void Spawn();
	virtual void Think(void);

	virtual void SetTarget(string_t newTarget);
	CBaseEntity *GetTarget();

	CNetworkVar( float, m_flInterpInterval ); // How frequently the follower re-evaluates its posistion.
	CNetworkHandle(CBaseEntity, m_pTargetEnt); // Pointer to target entity
};