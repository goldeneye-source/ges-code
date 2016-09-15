///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_door_interp.h
// Description:
//      Func ge_door that moves independantly on client and server to prevent jittery elevators.
//		
//
// Created On: 2/20/2016
// Created By: Check Github for list of contributors
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"

class CGEDoorInterp : public CBaseEntity
{
public:
	DECLARE_CLASS(CGEDoorInterp, CBaseEntity);
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	CGEDoorInterp();

	string_t m_sTargetDoor; // Name of the door this entity mimics

	void Spawn();

	CGEDoor *GetTarget();

	CNetworkVector(m_vecSpawnPos);
	CNetworkHandle(CBaseEntity, m_pTargetDoor); // Pointer to target entity
};