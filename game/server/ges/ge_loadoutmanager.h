//////////  Copyright © 2008, Goldeneye Source. All rights reserved. ///////////
// 
// File: ge_spawnermanager.h
// Description: Manages all spawner types and links them with the loadouts
//
// Created On: 3/22/2008
// Created By: KillerMonkey
///////////////////////////////////////////////////////////////////////////////

#ifndef GE_SPAWNERMANAGER_H
#define GE_SPAWNERMANAGER_H

#include "ge_gameplay.h"
#include "ge_loadout.h"
#include "ge_spawner.h"

class CGELoadoutManager
{
public:
	DECLARE_CLASS_NOBASE(CGELoadoutManager);

	CGELoadoutManager( void );
	~CGELoadoutManager( void );

	void ParseLoadouts( void );
	void AddLoadout( CGELoadout *pNew );

	void		GetLoadouts( CUtlVector<CGELoadout*> &loadouts );
	CGELoadout *GetLoadout( const char* szIdent );
	bool		IsLoadout( const char *name );
	
	inline const CGELoadout *CurrLoadout() const { return m_pCurrentLoadout; }

	// Get the current active weapon in the slot provided or the entire enchilada
	int  GetWeaponInSlot( int slot );
	bool GetWeaponSet( CUtlVector<int> &set );
	void GetRecentLoadouts(CUtlVector<CGELoadout*> &loadouts);

	void OnTokenAdded( const char *szClassname );

	bool SpawnWeapons();
	void RemoveWeapons( const char *weap = NULL );

	void KeepLoadoutOnNextChange() { m_bKeepCurrLoadout = true; }

	static void GetWeaponLists( CUtlVector<int> &vWeaponIds, CUtlVector<int> &vWeaponWeights );
	
private:
	void ParseGameplayAffinity( void );
	void ParseLogData( void );
	void ClearLoadouts( void );

	// Adjusts weights according to weaponset grouping rules.
	bool AdjustWeights(CUtlVector<int> &groups, CUtlVector<int> &weights);

	struct GameplaySet
	{
		CUtlVector<char*>	loadouts;
		CUtlVector<int>		weights;
		CUtlVector<int>		groups;
	};

	// Our loadout information
	CGELoadout *m_pCurrentLoadout;
	int			m_iCurrentGroup[3];
	CUtlDict<CGELoadout*, int>	m_Loadouts;
	CUtlDict<GameplaySet*, int> m_GameplaySets;
	// Used to select random_loadout or cycle_loadout
	CUtlVector<char*>	m_RandomSet;
	CUtlVector<int>		m_RandomSetWeights;
	// Flags
	bool				m_bKeepCurrLoadout;

	CUtlVector<CGELoadout*>	m_pRecentLoadouts;
};

#endif //GE_SPAWNERMANAGER_H
