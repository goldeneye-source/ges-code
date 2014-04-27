///////////// Copyright © 2009 LodleNet. All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_pybaseentity.cpp
//   Description :
//      [TODO: Write the purpose of ge_pybaseentity.cpp.]
//
//   Created On: 9/1/2009 10:19:15 PM
//   Created By: Mark Chandler <mailto:mark@moddb.com>
////////////////////////////////////////////////////////////////////////////

#include "ge_pyprecom.h"
#include "baseentity.h"
#include "ge_player.h"
#include "ehandle.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int pyGetEntSerial( CBaseEntity *pEnt )
{
	if ( !pEnt ) return INVALID_EHANDLE_INDEX;
	return pEnt->GetRefEHandle().ToInt();
}

CBaseEntity *pyGetEntByUniqueId( int serial )
{
	if ( serial < 0 || serial == INVALID_EHANDLE_INDEX )
		return NULL;

	// Extract the ent index and serial number from the serial given
	int iEntity = serial & ((1 << MAX_EDICT_BITS) - 1);
	int iSerialNum = serial >> NUM_ENT_ENTRY_BITS;
	EHANDLE hEnt( iEntity, iSerialNum );

	return hEnt.Get();
}

bp::list pyGetEntitiesInBox( const char *classname, Vector origin, Vector mins, Vector maxs )
{
	CBaseEntity *pEnts[256];
	int found = UTIL_EntitiesInBox( pEnts, 256, origin + mins, origin + maxs, FL_OBJECT );

	bp::list ents;
	for ( int i=0; i < found; i++ )
	{
		if ( !classname || FClassnameIs( pEnts[i], classname ) )
			ents.append( bp::ptr(pEnts[i]) );
	}

	return ents;
}

// The follow funcs are for simple virtualization
const QAngle &pyEyeAngles( CBaseEntity *pEnt )
{
	return pEnt->EyeAngles();
}

bool pyIsPlayer( CBaseEntity *pEnt )
{
	return pEnt->IsPlayer();
}

bool pyIsNPC( CBaseEntity *pEnt )
{
	return pEnt->IsNPC();
}

void pySetTargetName( CBaseEntity *pEnt, const char *name )
{
	if ( pEnt )
		pEnt->SetName( AllocPooledString( name ) );
}

const char *pyGetTargetName( CBaseEntity *pEnt )
{
	if ( pEnt )
		return pEnt->GetEntityName().ToCStr();
	return "";
}

CGEPlayer *pyGetOwner( CBaseEntity *pEnt )
{
	return ToGEPlayer( pEnt->GetOwnerEntity() );
}

static const char *EHANDLE_str( CHandle<CBaseEntity> &h )
{
	if ( h.Get() )
		return h->GetClassname();
	else
		return "Invalid Entity Handle";
}

// Compares CBaseEntity, EHANDLE to CBaseEntity, EHANDLE, and UID (int)
bool CBaseEntity_eq( bp::object a, bp::object b )
{
	CBaseEntity *a_cmp;
	bp::extract<CBaseEntity*> a_ent( a );
	bp::extract< EHANDLE > a_hent( a );

	if ( a_ent.check() )
		a_cmp = a_ent();
	else if ( a_hent.check() )
		a_cmp = a_hent().Get();
	else
		// Bad first param
		return false;

	bp::extract<CBaseEntity*>	b_ent( b );
	bp::extract< EHANDLE >		b_hent( b );
	bp::extract<int>			b_ient( b );

	if ( b_ent.check() )
		return a_cmp == b_ent();
	else if ( b_hent.check() )
		return a_cmp->GetRefEHandle() == b_hent();
	else
		return b_ient.check() && a_cmp->GetRefEHandle().ToInt() == b_ient();
}

// Anti-Compares CBaseEntity, EHANDLE to CBaseEntity, EHANDLE, and UID (int)
bool CBaseEntity_ne( bp::object a, bp::object b )
{
	CBaseEntity *a_cmp;
	bp::extract<CBaseEntity*> a_ent( a );
	bp::extract< EHANDLE > a_hent( a );

	if ( a_ent.check() )
		a_cmp = a_ent();
	else if ( a_hent.check() )
		a_cmp = a_hent().Get();
	else
		// Bad first param
		return false;

	bp::extract<CBaseEntity*>	b_ent( b );
	bp::extract< EHANDLE >		b_hent( b );
	bp::extract<int>			b_ient( b );

	if ( b_ent.check() )
		return a_cmp != b_ent();
	else if ( b_hent.check() )
		return a_cmp->GetRefEHandle() != b_hent();
	else
		return !b_ient.check() || a_cmp->GetRefEHandle().ToInt() != b_ient();
}

BOOST_PYTHON_MODULE(GEEntity)
{
	using namespace boost::python;

	def("GetUniqueId", pyGetEntSerial);
	def("GetUID", pyGetEntSerial);
	def("GetEntByUniqueId", pyGetEntByUniqueId, return_value_policy<reference_existing_object>());
	def("GetEntByUID", pyGetEntByUniqueId, return_value_policy<reference_existing_object>());

	def("GetEntitiesInBox", pyGetEntitiesInBox);

	class_< EHANDLE >("EntityHandle", init<CBaseEntity*>())
		.def("__str__", EHANDLE_str)
		.def("__eq__", CBaseEntity_eq)
		.def("__ne__", CBaseEntity_ne)
		.def("Get", &CHandle<CBaseEntity>::Get, return_value_policy<reference_existing_object>())
		.def("GetUID", &CBaseHandle::ToInt);

	class_<CBaseEntity, boost::noncopyable>("CBaseEntity", no_init)
		.def("__str__", &CBaseEntity::GetClassname)
		.def("__eq__", CBaseEntity_eq)
		.def("__ne__", CBaseEntity_ne)
		.def("GetParent", &CBaseEntity::GetParent, return_value_policy<reference_existing_object>())
		.def("GetPlayerOwner", pyGetOwner, return_value_policy<reference_existing_object>())
		.def("GetOwner", &CBaseEntity::GetOwnerEntity, return_value_policy<reference_existing_object>())
		.def("GetClassname", &CBaseEntity::GetClassname)
		.def("SetTargetName", pySetTargetName)
		.def("GetTargetName", pyGetTargetName)
		.def("SetModel", &CBaseEntity::SetModel)
		.def("IsAlive", &CBaseEntity::IsAlive)
		.def("IsPlayer", pyIsPlayer)
		.def("IsNPC", pyIsNPC)
		.def("GetTeamNumber", &CBaseEntity::GetTeamNumber)
		.def("GetAbsOrigin", &CBaseEntity::GetAbsOrigin, return_value_policy<copy_const_reference>())
		.def("SetAbsOrigin", &CBaseEntity::SetAbsOrigin)
		.def("GetAbsAngles", &CBaseEntity::GetAbsAngles, return_value_policy<copy_const_reference>())
		.def("SetAbsAngles", &CBaseEntity::SetAbsAngles)
		.def("GetEyeAngles", pyEyeAngles, return_value_policy<copy_const_reference>())
		.def("GetUID", pyGetEntSerial)
		.def("GetIndex", &CBaseEntity::entindex);
}
