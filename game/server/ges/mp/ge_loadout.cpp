//////////  Copyright © 2008, Goldeneye Source. All rights reserved. ///////////
// 
// File: ge_loadout.cpp
// Description:
//      Stores the loadout data parsed by CGELoadoutParser
//
// Created On: 3/22/3008
// Created By: Killermonkey
////////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include "ge_loadout.h"
#include "ge_loadoutmanager.h"
#include "ge_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Sort function for our weapons based on their weights
inline int __cdecl CompareWeapId( const int *ppLeft, const int *ppRight )
{
	return GetRandWeightForWeapon( *ppRight ) - GetRandWeightForWeapon( *ppLeft );
}

CGELoadout::CGELoadout( void )
{
	m_szIdent[0] = '\0';
	m_szPrintName[0] = '\0';
	m_iWeight = 0;

	for ( int i=0; i < MAX_WEAPON_SPAWN_SLOTS; i++ )
	{
		m_vLoadedWeapons.AddToTail( WEAPON_NONE );
		m_vActiveWeapons.AddToTail( WEAPON_NONE );
	}
}

CGELoadout::~CGELoadout( void )
{

}

void CGELoadout::Activate( void )
{
	int weapid = WEAPON_NONE;
	unsigned int upperbound;

	// Load the weapons
	CUtlVector<int> vWeaponList, vWeaponWeights;
	CGELoadoutManager::GetWeaponLists( vWeaponList, vWeaponWeights );

	upperbound = vWeaponList.Count();

	// Random max # of explosives
	int iExplosiveLimit = 1 + GERandom<int>( 2 );
	int randCount = 0;

	for ( int i=0; i < MAX_WEAPON_SPAWN_SLOTS; i++ )
	{
		weapid = m_vLoadedWeapons[i];

		// If we want a random weapon, make one up
		if ( weapid == WEAPON_RANDOM )
		{
			++randCount;

			// We reached our explosive limit, don't choose anymore
			if ( iExplosiveLimit <= 0 )
				upperbound = WEAPON_SPAWNMAX - 1;

			// Set the weapon slot for uniformity sake and the ammo spawners!
			weapid = GERandomWeighted<int,int>( vWeaponList.Base(), vWeaponWeights.Base(), upperbound );
			// Decrement if we chose an explosive
			if ( weapid > WEAPON_SPAWNMAX )
				--iExplosiveLimit;
		}

		// Set the weapon as active
		m_vActiveWeapons[i] = weapid;
	}

	// More than half of this weaponset is random then we shall sort it to ensure consistency
	if ( randCount >= (MAX_WEAPON_SPAWN_SLOTS / 2) )
	{
		SortList();
		DevMsg( "[Loadout] Weapon list was sorted with %i randoms chosen\n", randCount );
	}

	// Print out the list for developers
	DevMsg( "[Loadout] Activated loadout %s\n", GetIdent() );
	for ( int i = 0; i < m_vActiveWeapons.Count(); i++ )
		DevMsg( "[Loadout] Set Weapon in Slot %i to %s\n", i, WeaponIDToAlias( m_vActiveWeapons[i] ) );
}

void CGELoadout::ResetWeaponList()
{
	for ( int i=0; i < MAX_WEAPON_SPAWN_SLOTS; i++ )
		m_vLoadedWeapons[i] = WEAPON_NONE;	
}

void CGELoadout::LoadWeapons( KeyValues *pWeapons )
{
	if ( pWeapons )
	{
		KeyValues *pWpnMover = pWeapons->GetFirstSubKey();
		while( pWpnMover )
		{
			int iSlot = atoi( pWpnMover->GetName() );
			if ( iSlot < 0 || iSlot >= MAX_WEAPON_SPAWN_SLOTS )
				return;

			m_vLoadedWeapons[iSlot] = TranslateWeaponName( pWpnMover->GetString() );
			
			pWpnMover = pWpnMover->GetNextKey();
		}
	}
	else
	{
		DevWarning( "[Loadout] Invalid loadout created with no weapons list: %s\n", GetIdent() );
	}
}

int CGELoadout::GetWeapon( int slot ) const
{
	if ( slot < 0 || slot >= MAX_WEAPON_SPAWN_SLOTS )
		return WEAPON_NONE;

	return m_vActiveWeapons[slot];
}

int CGELoadout::GetFirstWeapon( void ) const
{
	for( int i=0; i < MAX_WEAPON_SPAWN_SLOTS; ++i )
	{
		if ( m_vActiveWeapons[i] != WEAPON_NONE )
			return m_vActiveWeapons[i];
	}

	return WEAPON_NONE;
}

void CGELoadout::SortList()
{
	m_vActiveWeapons.Sort( CompareWeapId );
}

void CGELoadout::PrintList()
{
	DevMsg( "--- Weapon List For %s ---\n", GetPrintName() );

	for ( int i=0; i < MAX_WEAPON_SPAWN_SLOTS; i++ )
		DevMsg( "[%d]\n", WeaponIDToAlias( GetWeapon(i) ) );	
}

void CGELoadout::SwitchActiveWeapon( int slot )
{
	if ( slot < 0 || slot >= MAX_WEAPON_SPAWN_SLOTS )
		return;

	// Switch the weapon based on the slot
	if ( slot <= MAX_WEAPON_SPAWN_SLOTS * 0.25f )
		m_vActiveWeapons[slot] = (m_vActiveWeapons[slot] != WEAPON_PP7) ? WEAPON_PP7 : WEAPON_DD44;
	else if ( slot < MAX_WEAPON_SPAWN_SLOTS * 0.75f )
		m_vActiveWeapons[slot] = (m_vActiveWeapons[slot] != WEAPON_D5K) ? WEAPON_D5K : WEAPON_ZMG;
	else
		m_vActiveWeapons[slot] = (m_vActiveWeapons[slot] != WEAPON_AR33) ? WEAPON_AR33 : WEAPON_RCP90;
}

int CGELoadout::TranslateWeaponName( const char* szName )
{
	char szWeaponName[LOADOUT_WEAP_LEN];
	
	if ( Q_strncmp( szName, "weapon_", 7 ) != 0 )
	{
		Q_strcpy( szWeaponName, "weapon_" );
		Q_strncat( szWeaponName, szName, LOADOUT_WEAP_LEN );
	}
	else
	{
		Q_strncpy( szWeaponName, szName, LOADOUT_WEAP_LEN );
	}

	Q_strlower( szWeaponName );

	int id = AliasToWeaponID( szWeaponName );

	// Make sure we only return active weapons!
	if ( id > WEAPON_NONE && id < WEAPON_MAX )
		return id;

	// If we entered a bad name, issue a warning
	if ( Q_stricmp( szName, "NONE" ) )
		DevWarning( "[Loadout] %s contained invalid weapon name %s which was ignored\n", GetIdent(), szName );

	// Return WEAPON_NONE to prevent loading anything
	return WEAPON_NONE;
}
