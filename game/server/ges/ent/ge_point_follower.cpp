///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_door.cpp
// Description:
//      Fancy door that accelerates and can change direction during motion.  Also supports paired doors to
//		avoid a lot of the nonsense caused by double door entity systems.
//
// Created On: 9/30/2015
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

#include "ge_point_follower.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(ge_point_follower, CGEFollower);

BEGIN_DATADESC(CGEFollower)

DEFINE_KEYFIELD(m_sTargetEnt, FIELD_STRING, "TargetEnt"),
DEFINE_KEYFIELD(m_flInterpInterval, FIELD_FLOAT, "InterpInterval"),
//DEFINE_INPUTFUNC(FIELD_VOID, "ForceToggle", InputForceToggle),
//DEFINE_OUTPUT(m_FirstOpen, "OnFirstOpen"),
//DEFINE_THINKFUNC(Think),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST(CGEFollower, DT_GEFollower)
	SendPropFloat(SENDINFO(m_flInterpInterval)),
	SendPropEHandle(SENDINFO(m_pTargetEnt)),
	SendPropExclude("DT_BaseEntity", "m_vecAbsOrigin"), //Don't tell the client about the location or velocity of this entity as that completely ruins the point.
	SendPropExclude("DT_BaseEntity", "m_vecOrigin"),
	SendPropExclude("DT_BaseEntity", "m_vecAbsVelocity"),
	SendPropExclude("DT_BaseEntity", "m_vecVelocity"),
END_SEND_TABLE()

CGEFollower::CGEFollower()
{
	m_flInterpInterval = 0.1;
	
}

void CGEFollower::Spawn(void)
{
	m_pTargetEnt = GetTarget();
	SetThink(&CGEFollower::Think);
	SetNextThink(gpGlobals->curtime);

	BaseClass::Spawn();
}

CBaseEntity *CGEFollower::GetTarget() //No need for me to make this twice, it's just a modified version of the ge_door function.
{
	string_t newPartner = m_sTargetEnt;
	CBaseEntity *partnerEnt = NULL;

	if (newPartner != NULL_STRING) // Make sure the mapper assigned a partner.
	{
		CBaseEntity *pPartner = gEntList.FindEntityByName(NULL, newPartner, NULL);

		if (pPartner == NULL)
			Msg("Entity %s(%s) has bad target entity %s\n", STRING(GetEntityName()), GetDebugName(), STRING(newPartner));
		else
			partnerEnt = pPartner; //Actually return a partner ent.
	}
	return partnerEnt;
}

void CGEFollower::SetTarget(string_t newPartner)
{
	m_sTargetEnt = newPartner;

	m_pTargetEnt = GetTarget();
}

void CGEFollower::Think(void)
{
	SetLocalOrigin(m_pTargetEnt->GetLocalOrigin());

	SetNextThink(gpGlobals->curtime + m_flInterpInterval*0.1);
}