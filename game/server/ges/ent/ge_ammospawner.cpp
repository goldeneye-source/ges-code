//////////  Copyright © 2011, Goldeneye Source. All rights reserved. ///////////
// 
// File: ge_ammospawner.cpp
// Description: See Header
//
// Created On: 7-19-11
// Created By: KillerMonkey
///////////////////////////////////////////////////////////////////////////////
#include "cbase.h"

#include "ammodef.h"

#include "ge_ammocrate.h"
#include "ge_loadoutmanager.h"
#include "gemp_gamerules.h"
#include "ge_spawner.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CGEAmmoSpawner : public CGESpawner
{
public:
	DECLARE_CLASS( CGEAmmoSpawner, CGESpawner );
	
protected:
	virtual void OnInit( void );
	virtual void OnEntSpawned( const char *szClassname );
	
	virtual int  ShouldRespawn( void );
	virtual float GetRespawnInterval( void );
};

LINK_ENTITY_TO_CLASS( ge_ammospawner, CGEAmmoSpawner );

void CGEAmmoSpawner::OnInit( void )
{
	SetBaseEnt( "ge_ammocrate" );
}

void CGEAmmoSpawner::OnEntSpawned( const char *szClassname )
{
	// If we are not spawning an ammo crate ignore
	if ( IsOverridden() )
		return;

	// if we failed to spawn just stop right now
	CGEAmmoCrate *pCrate = (CGEAmmoCrate*) GetEnt();
	if ( !pCrate )
		return;

	pCrate->SetOriginalSpawnOrigin( GetAbsOrigin() );
	pCrate->SetOriginalSpawnAngles( GetAbsAngles() );

	// Load us up with the appropriate ammo
	int weapid = GEMPRules()->GetLoadoutManager()->GetWeaponInSlot( GetSlot() );
	pCrate->SetWeaponID( weapid );
	pCrate->SetAmmoType( GetAmmoDef()->Index( GetAmmoForWeapon(weapid) ) );
	// Force a respawn NOW (default is to wait item respawn time)
	pCrate->SetNextThink( gpGlobals->curtime );
}

int CGEAmmoSpawner::ShouldRespawn( void )
{
	int basestate = BaseClass::ShouldRespawn();

	if ( IsOverridden() && basestate > 1 )
		return 2;

	if ( !GEMPRules()->AmmoShouldRespawn() )
		return 0;

	return basestate;
}

float CGEAmmoSpawner::GetRespawnInterval( void )
{
	return IsOverridden() ? 0 : GERules()->FlItemRespawnTime(NULL);
}
