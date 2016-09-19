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
	int prevgroup = 1;

	// Load the weapons
	CUtlVector<int> vWeaponList, vWeaponWeights, vWeaponLevels;

	vWeaponList.RemoveAll();
	vWeaponWeights.RemoveAll();
	vWeaponLevels.RemoveAll();

	CGELoadoutManager::GetWeaponLists( vWeaponList, vWeaponWeights );

	// Build table of weapon weights for random weapon selection
	for (int i = 0; i < WEAPON_RANDOM_MAX; i++)
	{
		if (WeaponIDToAlias(i))
			vWeaponLevels.AddToTail( GEWeaponInfo[i].strength );
		else
			vWeaponLevels.AddToTail(-1);
	} 

	for ( int i=0; i < MAX_WEAPON_SPAWN_SLOTS; i++ )
	{
		weapid = m_vLoadedWeapons[i];

		// If we want a random weapon, make one up
		if (weapid == WEAPON_RANDOM)
			weapid = PickGroupWeapon(vWeaponList, vWeaponWeights, vWeaponLevels, prevgroup, 1);
		else if (weapid == WEAPON_RANDOM_HITSCAN)
			weapid = PickGroupWeapon(vWeaponList, vWeaponWeights, vWeaponLevels, prevgroup, 0);
		else if (weapid == WEAPON_RANDOM_EXPLOSIVE)
			weapid = PickGroupWeapon(vWeaponList, vWeaponWeights, vWeaponLevels, prevgroup, 2);
		else if (weapid == WEAPON_ULTRA_RANDOM)
		{
			for (int i = 0; i < vWeaponWeights.Count(); i++)
				vWeaponWeights[i] = 10;

			weapid = PickGroupWeapon(vWeaponList, vWeaponWeights, vWeaponLevels, -1, 1);
		}
		
		// Set the weapon as active
		m_vActiveWeapons[i] = weapid;

		// Set the previous level to this level and increment it if this weapon has already shown up in the set.  Prevents weapon spam.
		prevgroup = vWeaponLevels[weapid];


		if (prevgroup < 8) //Don't increment if prevgroup is already as high as it can go
		{
			for (int g = 0; g < i; g++)
			{
				if (weapid == m_vActiveWeapons[g])
				{
					prevgroup++;
					break;
				}
			}
		}
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
		DevMsg( "  %s\n", WeaponIDToAlias( GetWeapon(i) ) );	
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

int CGELoadout::PickGroupWeapon(CUtlVector<int> &weapons, CUtlVector<int> &weights, CUtlVector<int> &strengths, int basegroup, int allowexplosives)
{
	bool explosive;
	int desiredstrength, seed;
	CUtlVector<int> vDesiredWeaponList, vDesiredWeaponWeights;
	int explosivelist[6] = { WEAPON_TIMEDMINE, WEAPON_REMOTEMINE, WEAPON_PROXIMITYMINE, WEAPON_GRENADE, WEAPON_ROCKET_LAUNCHER, WEAPON_GRENADE_LAUNCHER };

	vDesiredWeaponList.RemoveAll();
	vDesiredWeaponWeights.RemoveAll();

	seed = rand() % (14 - basegroup); //Advancing a level is less likley for higher tiers.

	if (basegroup == -1)
		desiredstrength = (rand() % 8) + 1;
	else if (seed < 6)
		desiredstrength = basegroup;
	else if (seed < 11  )
		desiredstrength = basegroup + 1;
	else
		desiredstrength = basegroup + 2;

	desiredstrength = MIN(desiredstrength, 8);

	// Go through and create a new weapon list with only weapons that have our desired strength
	for (int i = 0; i < weapons.Count(); i++)
	{
		if (strengths[weapons[i]] == desiredstrength)
		{
			vDesiredWeaponList.AddToTail(weapons[i]);
			vDesiredWeaponWeights.AddToTail(weights[i]);
		}
	}

	// Filter out explosives or hitscan from the new list depending on settings
	if (allowexplosives != 1)
	{
		for (int i = 0; i < WEAPON_RANDOM_MAX;) //Don't increment here because we might remove a list entry
		{
			if (i >= vDesiredWeaponList.Count()) //Kill loop once we hit the end of the list.
				break;

			explosive = false;

			for (int l = 0; l < 6; l++)
			{
				if (vDesiredWeaponList[i] == explosivelist[l])
				{
					explosive = true;
				}
			}

			if ((allowexplosives == 0 && explosive) || (allowexplosives == 2 && !explosive))
			{
				vDesiredWeaponList.Remove(i);
				vDesiredWeaponWeights.Remove(i);
			}
			else
				i++;
		}
	}

	//This usually only happens if explosive weapon is desired but slot doesn't have any.  Can also be a failsafe in case of freak accident though.
	if (vDesiredWeaponList.Count() < 1)
	{
		vDesiredWeaponList.AddToTail(WEAPON_GRENADE);
		vDesiredWeaponWeights.AddToTail(10);
		vDesiredWeaponList.AddToTail(WEAPON_TIMEDMINE);
		vDesiredWeaponWeights.AddToTail(10);
	}

	// Return a random weapon ID from the filtered list
	return GERandomWeighted<int, int>(vDesiredWeaponList.Base(), vDesiredWeaponWeights.Base(), vDesiredWeaponList.Count());
}
