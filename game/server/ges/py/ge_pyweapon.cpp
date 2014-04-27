///////////// Copyright © 2009 LodleNet. All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_pyweapon.cpp
//   Description :
//      [TODO: Write the purpose of ge_pyweapon.cpp.]
//
//   Created On: 9/1/2009 10:27:58 PM
//   Created By: Mark Chandler <mailto:mark@moddb.com>
////////////////////////////////////////////////////////////////////////////
#include "ge_pyprecom.h"

#include "ammodef.h"

#include "ge_ai.h"
#include "ge_weapon.h"
#include "ge_weapon_parse.h"
#include "ge_loadoutmanager.h"
#include "gemp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CGEWeapon *pyToGEWeapon( CBaseEntity *pEnt )
{
	if ( !pEnt )
		return NULL;

	if ( Q_stristr( pEnt->GetClassname(), "weapon_" ) ||  Q_stristr( pEnt->GetClassname(), "token_" ) )
		return ToGEWeapon( (CBaseCombatWeapon*)pEnt );

	return NULL;
}

const char *pyGetAmmoType( CGEWeapon *pWeap )
{
	if ( pWeap )
	{
		int id = pWeap->GetPrimaryAmmoType();
		Ammo_t *ammo = GetAmmoDef()->GetAmmoOfIndex( id );
		return ammo ? ammo->pName : "";
	}

	return "";
}

int pyGetAmmoCount( CGEWeapon *pWeap )
{
	if ( !pWeap )
		return 0;

	CBaseCombatCharacter *pOwner = pWeap->GetOwner();
	if ( pOwner && (pOwner->IsPlayer() || pOwner->IsNPC()) )
		return pOwner->GetAmmoCount( pWeap->GetPrimaryAmmoType() );
	else
		return pWeap->GetDefaultClip1();
}

void pySetAmmoCount( CGEWeapon *pWeap, int amt )
{
	if ( pWeap )
	{
		CBaseCombatCharacter *pOwner = pWeap->GetOwner();
		if ( pOwner && (pOwner->IsPlayer() || pOwner->IsNPC()) )
		{
			int aid = pWeap->GetPrimaryAmmoType();
			pOwner->SetAmmoCount( amt, aid );
		}
	}
}

int pyGetMaxAmmoCount( CGEWeapon *pWeap )
{
	if ( !pWeap )
		return 0;

	if ( pWeap->UsesPrimaryAmmo() )
		return GetAmmoDef()->MaxCarry( pWeap->GetPrimaryAmmoType() );
	else
		return 999;
}

int pyGetWeaponSlot( CGEWeapon *pWeap )
{
	if ( !pWeap )
		return -1;

	// Return the highest slot position for this weapon id
	int weapid = pWeap->GetWeaponID();
	int slot = 0;
	for ( int i=0; i < MAX_WEAPON_SPAWN_SLOTS; i++ )
	{
		if ( GEMPRules()->GetLoadoutManager()->GetWeaponInSlot(i) == weapid )
			slot = i;
	}

	return slot;
}

const char *pyWeaponAmmoType( bp::object weap )
{
	bp::extract<char*> to_name( weap );
	bp::extract<int> to_id( weap );

	if ( to_name.check() )
		return GetAmmoForWeapon( AliasToWeaponID( to_name() ) );
	else if ( to_id.check() )
		return GetAmmoForWeapon( to_id() );
	else
		return "";
}

const char* pyWeaponPrintName( bp::object weap )
{
	bp::extract<char*> to_name( weap );
	bp::extract<int> to_id( weap );

	if ( to_name.check() )
		return GetWeaponPrintName( AliasToWeaponID( to_name() ) );
	else if ( to_id.check() )
		return GetWeaponPrintName( to_id() );
	else
		return "";
}

const char* pyWeaponClassname( int id )
{
	// Only makes sense to accept an id
	const char *name = WeaponIDToAlias(id);
	return name != NULL ? name : "";
}

void pyWeaponSetAbsOrigin( CGEWeapon *weap, Vector origin )
{
	if ( !weap || weap->GetOwner() )
		return;

	weap->SetAbsOrigin( origin );
}

void pyWeaponSetAbsAngles( CGEWeapon *weap, QAngle angles )
{
	if ( !weap || weap->GetOwner() )
		return;

	weap->SetAbsAngles( angles );
}

bp::dict pyWeaponInfo( bp::object weap, CBaseCombatCharacter *pOwner = NULL )
{
	bp::dict info;
	bp::extract<char*> to_name( weap );
	bp::extract<int> to_id( weap );

	// Extract the weapon id
	int weap_id = WEAPON_NONE;
	if ( to_name.check() )
		weap_id = AliasToWeaponID( to_name() );
	else if ( to_id.check() )
		weap_id = to_id();

	// Get our info
	const char *name = WeaponIDToAlias( weap_id );
	if ( name && name[0] != '\0' )
	{
		int h = LookupWeaponInfoSlot( name );
		if ( h == GetInvalidWeaponInfoHandle() )
			return info;

		CGEWeaponInfo *weap = dynamic_cast<CGEWeaponInfo*>( GetFileWeaponInfoFromHandle( h ) );
		if ( weap )
		{
			info["id"] = weap_id;
			info["classname"] = name;
			info["printname"] = weap->szPrintName;
			info["weight"] = weap->iWeight;
			info["damage"] = weap->m_iDamage;
			info["penetration"] = weap->m_flMaxPenetration > 1.5f ? weap->m_flMaxPenetration : 0;
			info["viewmodel"] = weap->szViewModel;
			info["worldmodel"] = weap->szWorldModel;
			info["melee"] = weap->m_bMeleeWeapon;
			info["uses_clip"] = weap->iMaxClip1 != WEAPON_NOCLIP;
			info["clip_size"] = weap->iMaxClip1;
			info["clip_def"] = weap->iDefaultClip1;
			
			int aid = GetAmmoDef()->Index( GetAmmoForWeapon( weap_id ) );
			Ammo_t *ammo = GetAmmoDef()->GetAmmoOfIndex( aid );
			if ( ammo )
			{
				info["ammo_type"] = GetAmmoForWeapon( weap_id );
				info["ammo_max"] = ammo->pMaxCarry;

				if ( pOwner )
					info["ammo_count"] = pOwner->GetAmmoCount( aid );
			}			
		}
	}

	return info;
}

BOOST_PYTHON_FUNCTION_OVERLOADS(WeaponInfo_overloads, pyWeaponInfo, 1, 2);
BOOST_PYTHON_MODULE(GEWeapon)
{
	using namespace boost::python;

	def("ToGEWeapon", pyToGEWeapon, return_value_policy<reference_existing_object>());
	
	// Quick access to weapon information
	def("WeaponAmmoType", pyWeaponAmmoType);
	def("WeaponClassname", pyWeaponClassname);
	def("WeaponPrintName", pyWeaponPrintName);
	def("WeaponInfo", pyWeaponInfo, WeaponInfo_overloads());

	// Here for function calls only
	class_<CBaseCombatWeapon, bases<CBaseEntity>, boost::noncopyable>("CBaseCombatWeapon", no_init);

	// The weapon class definition
	class_<CGEWeapon, bases<CBaseCombatWeapon>, boost::noncopyable>("CGEWeapon", no_init)
		.def("GetWeight", &CGEWeapon::GetWeight)
		.def("GetPrintName", &CGEWeapon::GetPrintName)
		.def("IsMeleeWeapon", &CGEWeapon::IsMeleeWeapon)
		.def("IsExplosiveWeapon", &CGEWeapon::IsExplosiveWeapon)
		.def("IsAutomaticWeapon", &CGEWeapon::IsAutomaticWeapon)
		.def("IsThrownWeapon", &CGEWeapon::IsThrownWeapon)
		.def("CanHolster", &CGEWeapon::CanHolster)
		.def("HasAmmo", &CGEWeapon::HasPrimaryAmmo)
		.def("UsesAmmo", &CGEWeapon::UsesPrimaryAmmo)
		.def("GetAmmoType", pyGetAmmoType)
		.def("GetAmmoCount", pyGetAmmoCount)
		.def("SetAmmoCount", pySetAmmoCount)
		.def("GetMaxAmmoCount", pyGetMaxAmmoCount)
		.def("GetClip", &CGEWeapon::Clip1)
		.def("GetMaxClip", &CGEWeapon::GetMaxClip1)
		.def("GetDefaultClip", &CGEWeapon::GetDefaultClip1)
		.def("GetDamage", &CGEWeapon::GetWeaponDamage)
		.def("GetWeaponId", &CGEWeapon::GetWeaponID)
		.def("GetWeaponSlot", pyGetWeaponSlot)
		.def("SetSkin", &CGEWeapon::SetSkin)
		// The following override CBaseEntity to prevent movement when held
		.def("SetAbsOrigin", pyWeaponSetAbsOrigin)
		.def("SetAbsAngles", pyWeaponSetAbsAngles);
}
