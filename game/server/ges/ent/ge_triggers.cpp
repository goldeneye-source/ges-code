//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Spawn and use functions for GE:S unique editor-placed triggers.
//
//===========================================================================//

#include "cbase.h"
#include "ai_basenpc.h"
#include "player.h"
#include "triggers.h"
#include "ge_triggers.h"
#include "gemp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//------------
//trigger_trap
//------------

BEGIN_DATADESC(CTriggerTrap)

	DEFINE_KEYFIELD(m_sDeathMessage, FIELD_STRING, "DeathMessage"),
	DEFINE_KEYFIELD(m_sSuicideMessage, FIELD_STRING, "SuicideMessage"),
	DEFINE_KEYFIELD(m_sKillMessage, FIELD_STRING, "KillMessage"),
	// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "BecomeOwner", InputBecomeOwner),
	DEFINE_INPUTFUNC(FIELD_VOID, "VoidOwner", InputVoidOwner),
	DEFINE_INPUT(m_sDeathMessage, FIELD_STRING, "SetDeathMessage"),
	DEFINE_INPUT(m_sSuicideMessage, FIELD_STRING, "SetSuicideMessage"),
	DEFINE_INPUT(m_sKillMessage, FIELD_STRING, "SetKillMessage"),

END_DATADESC()


LINK_ENTITY_TO_CLASS(trigger_trap, CTriggerTrap);


// When we spawn, add ourselves to the trap list so we can easily clear ownership when we need to.
void CTriggerTrap::Spawn(void)
{
	BaseClass::Spawn();

	GEMPRules()->AddTrapToList(this);
}

//------------------------------------------------------------------------------
// Purpose: Make the activator the owner of the trap.
//------------------------------------------------------------------------------
void CTriggerTrap::InputBecomeOwner(inputdata_t &inputdata)
{
	if (inputdata.pActivator && inputdata.pActivator->IsPlayer())
		m_hTrapOwner = inputdata.pActivator;
}

//------------------------------------------------------------------------------
// Purpose: Remove the owner from the trap
//------------------------------------------------------------------------------
void CTriggerTrap::InputVoidOwner(inputdata_t &inputdata)
{
		m_hTrapOwner = NULL;
}

//------------------------------------------------------------------------------
// Purpose: Input handler to turn on this trigger.
//------------------------------------------------------------------------------
void CTriggerTrap::InputEnable(inputdata_t &inputdata)
{
	if (inputdata.pActivator && inputdata.pActivator->IsPlayer())
		m_hTrapOwner = inputdata.pActivator;

	BaseClass::InputEnable(inputdata);
}


//------------------------------------------------------------------------------
// Purpose: Input handler to turn off this trigger.
//------------------------------------------------------------------------------
void CTriggerTrap::InputDisable(inputdata_t &inputdata)
{
	if (inputdata.pActivator && inputdata.pActivator->IsPlayer())
		m_hTrapOwner = inputdata.pActivator;

	BaseClass::InputDisable(inputdata);
}

//-----------------------------------------------------------------------------
// Purpose: Toggles this trigger between enabled and disabled.
//-----------------------------------------------------------------------------
void CTriggerTrap::InputToggle(inputdata_t &inputdata)
{
	if (inputdata.pActivator && inputdata.pActivator->IsPlayer())
		m_hTrapOwner = inputdata.pActivator;

	BaseClass::InputToggle(inputdata);
}