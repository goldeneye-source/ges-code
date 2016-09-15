///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_door_interp.cpp
// Description:
//      Emulates ge_door movement on the client for less jittery elevators.  Does not move at all on the server
//		so make sure anything parented to it is purely visual!
//		
//
// Created On: 2/20/2016
// Created By: Check Github for list of contributors
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "doors.h"
#include "ge_door.h"
#include "entitylist.h"
#include "physics.h"
#include "ndebugoverlay.h"
#include "engine/IEngineSound.h"
#include "physics_npc_solver.h"
#include "gemp_gamerules.h"

#include "ge_door_interp.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(ge_door_interp, CGEDoorInterp);

BEGIN_DATADESC(CGEDoorInterp)
DEFINE_KEYFIELD(m_sTargetDoor, FIELD_STRING, "TargetDoor"),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CGEDoorInterp, DT_GEDoorInterp)
SendPropEHandle(SENDINFO(m_pTargetDoor)),
SendPropVector(SENDINFO(m_vecSpawnPos)),

SendPropExclude("DT_BaseEntity", "m_vecAbsOrigin"), //Don't tell the client about the location or velocity of this entity as that completely ruins the point.
SendPropExclude("DT_BaseEntity", "m_vecOrigin"),
SendPropExclude("DT_BaseEntity", "m_vecAbsVelocity"),
SendPropExclude("DT_BaseEntity", "m_vecVelocity"),
END_SEND_TABLE()

CGEDoorInterp::CGEDoorInterp()
{
	m_pTargetDoor = NULL;
}

void CGEDoorInterp::Spawn(void)
{
	m_vecSpawnPos = GetLocalOrigin();
	m_pTargetDoor = GetTarget();

	BaseClass::Spawn();
}

CGEDoor *CGEDoorInterp::GetTarget() //No need for me to make this twice, it's just a modified version of the ge_door function.
{
	string_t newPartner = m_sTargetDoor;
	CGEDoor *partnerEnt = NULL;

	if (newPartner != NULL_STRING) // Make sure the mapper assigned a partner.
	{
		CBaseEntity *pPartner = gEntList.FindEntityByName(NULL, newPartner, NULL);

		if (pPartner == NULL)
			Msg("Entity %s(%s) has bad target entity %s\n", STRING(GetEntityName()), GetDebugName(), STRING(newPartner));
		else
			partnerEnt = static_cast<CGEDoor*>(pPartner); //Actually return a partner ent.
	}
	return partnerEnt;
}