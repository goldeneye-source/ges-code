#ifndef PY_PRECOM_H
#define PY_PRECOM_H

#ifdef _WIN32
	#pragma once
	#pragma warning(disable : 4049) // Locally defined symbol
	#pragma warning(disable : 4217)
#endif

#include "cbase.h"

// Undefine max/min
#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#include <boost/python.hpp>
namespace bp = boost::python;

// Functions that can be hooked are exposed in the GEGlobal module
// Registering a hook is done in GEUtil module
enum GES_EVENTHOOK
{
	FUNC_INVALID = -1,

	// Gameplay Function Hooks
	FUNC_GP_THINK,
	FUNC_GP_PLAYERCONNECT,
	FUNC_GP_PLAYERDISCONNECT,
	FUNC_GP_PLAYERSPAWN,
	FUNC_GP_PLAYEROBSERVER,
	FUNC_GP_PLAYERKILLED,
	FUNC_GP_PLAYERTEAM,
	FUNC_GP_ROUNDBEGIN,
	FUNC_GP_ROUNDEND,
	FUNC_GP_WEAPONSPAWNED,
	FUNC_GP_WEAPONREMOVED,
	FUNC_GP_ARMORSPAWNED,
	FUNC_GP_ARMORREMOVED,
	FUNC_GP_AMMOSPAWNED,
	FUNC_GP_AMMOREMOVED,

	// Ai Function Hooks
	FUNC_AI_ONSPAWN,
	FUNC_AI_ONLOOKED,
	FUNC_AI_ONLISTENED,
	FUNC_AI_PICKUPITEM,
	FUNC_AI_GATHERCONDITIONS,
	FUNC_AI_SELECTSCHEDULE,
	FUNC_AI_SETDIFFICULTY,
};

#define PY_CALLHOOKS( hook, args ) TRYFUNC( this->get_override("CallEventHooks")( hook, args ) )

#endif
