///////////// Copyright © 2009 LodleNet. All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_pyammocrate.cpp
//   Description :
//      [TODO: Write the purpose of ge_pyammocrate.cpp.]
//
//   Created On: 7/14/2016 10:27:58 PM
//   Created By: djoslin
////////////////////////////////////////////////////////////////////////////

#include "cbase.h" // <--- why do I need to include this?

#include "ge_pyprecom.h"

#include "ammodef.h"

#include "ge_ai.h"
#include "ge_ammocrate.h"
#include "ge_weapon.h"
#include "ge_weapon_parse.h"
#include "ge_loadoutmanager.h"
#include "gemp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CGEAmmoCrate *pyToGEAmmoCrate( CBaseEntity *pEnt )
{
	if ( !pEnt )
		return NULL;

	if ( Q_stristr( pEnt->GetClassname(), "ge_ammocrate" ) )
	{
#ifdef _DEBUG
		return dynamic_cast<CGEAmmoCrate*>( pEnt );
#else
		return static_cast<CGEAmmoCrate*>( pEnt );
#endif
	}

	return NULL;
}

const char *pyGetAmmoType( CGEAmmoCrate *pAmmoCrate )
{
	if ( pAmmoCrate )
	{
		int id = pAmmoCrate->GetAmmoType();
		Ammo_t *ammo = GetAmmoDef()->GetAmmoOfIndex( id );
		return ammo ? ammo->pName : "";
	}

	return "";
}

void pySetAmmoType( CGEAmmoCrate *pAmmoCrate, int ammoTypeID )
{
	if ( pAmmoCrate )
	{
		pAmmoCrate->SetAmmoType( ammoTypeID );
	}
}

void pyAmmoSetAbsOrigin( CGEAmmoCrate *pAmmoCrate, Vector origin )
{
	if ( !pAmmoCrate )
		return;

	pAmmoCrate->SetAbsOrigin( origin );
}

void pyAmmoSetAbsAngles( CGEAmmoCrate *pAmmoCrate, QAngle angles )
{
	if ( !pAmmoCrate )
		return;

	pAmmoCrate->SetAbsAngles( angles );
}

int pyAmmoGetWeaponID( CGEAmmoCrate *pAmmoCrate )
{
	if ( !pAmmoCrate )
		return 0;

	return pAmmoCrate->GetWeaponID();
}

int pyAmmoGetWeight( CGEAmmoCrate *pAmmoCrate )
{
	if ( !pAmmoCrate )
		return 0;

	return pAmmoCrate->GetWeight();
}

BOOST_PYTHON_MODULE(GEAmmoCrate)
{
	using namespace boost::python;

	def("ToGEAmmoCrate", pyToGEAmmoCrate, return_value_policy<reference_existing_object>());

	// The ammocrate class definition
	class_<CGEAmmoCrate, bases<CBaseEntity>, boost::noncopyable>("CGEAmmoCrate", no_init)
		.def("GetAmmoType", pyGetAmmoType)
		.def("SetAmmoType", pySetAmmoType)
		.def("GetWeaponId", pyAmmoGetWeaponID)
		.def("GetWeight", pyAmmoGetWeight)
		// The following override CBaseEntity to prevent movement when held
		.def("SetAbsOrigin", pyAmmoSetAbsOrigin)
		.def("SetAbsAngles", pyAmmoSetAbsAngles);
}
