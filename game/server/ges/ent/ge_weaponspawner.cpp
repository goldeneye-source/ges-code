//////////  Copyright © 2006, Goldeneye Source. All rights reserved. ///////////
// 
// File: ge_weaponspawner.h
// Description: Weapon spawner implementation.
//
// Created On: 3/12/2008
// Created By: Killer Monkey (With Help from Mario Cacciatore <mailto:mariocatch@gmail.com>)
///////////////////////////////////////////////////////////////////////////////
#include "cbase.h"

#include "ge_shareddefs.h"
#include "gemp_gamerules.h"
#include "ge_loadoutmanager.h"
#include "ge_weapon.h"
#include "ge_spawner.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CGEWeaponSpawner : public CGESpawner
{
public:
	DECLARE_CLASS( CGEWeaponSpawner, CGESpawner );
	DECLARE_DATADESC();

	CGEWeaponSpawner();

	virtual bool IsSpecial(void) { return ( m_bAllowSpecial || BaseClass::IsSpecial() ); };
	virtual bool IsVerySpecial() { return m_bAllowSpecial; } // Spawns we've marked are very special and will be used over ones that aren't marked.
	virtual bool KeyValue( const char *szKeyName, const char *szValue );

protected:
	virtual void OnInit( void );

	virtual int  ShouldRespawn( void );
	virtual float GetRespawnInterval( void );

private:
	bool m_bAllowSpecial;		// Explicitly allow a special token to spawn here
};

LINK_ENTITY_TO_CLASS(ge_weaponspawner, CGEWeaponSpawner);

BEGIN_DATADESC( CGEWeaponSpawner )
	DEFINE_KEYFIELD( m_bAllowSpecial, FIELD_BOOLEAN, "allowgg" ),
END_DATADESC();

CGEWeaponSpawner::CGEWeaponSpawner( void )
{
	m_bAllowSpecial = false;
}

bool CGEWeaponSpawner::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( !Q_stricmp(szKeyName, "allowgg") )
	{
		if ( atoi(szValue) != 0 ) {
			m_bAllowSpecial = true;
			return true;
		}
	}

	return BaseClass::KeyValue( szKeyName, szValue );
}

void CGEWeaponSpawner::OnInit( void )
{
	int weapid = GEMPRules()->GetLoadoutManager()->GetWeaponInSlot( GetSlot() );
	if ( weapid != WEAPON_NONE )
		SetBaseEnt( WeaponIDToAlias( weapid ) );
}

int CGEWeaponSpawner::ShouldRespawn( void )
{
	int basestate = BaseClass::ShouldRespawn();

	if ( IsOverridden() && basestate > 1 )
		return 2;

	// Obey our game rules
	if ( !GEMPRules()->WeaponShouldRespawn(GetBaseClass()) )
		return 0;

	return basestate;
}

float CGEWeaponSpawner::GetRespawnInterval( void )
{ 
	return 10;
}
