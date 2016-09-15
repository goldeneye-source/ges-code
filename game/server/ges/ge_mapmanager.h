//////////  Copyright © 2016, Goldeneye Source. All rights reserved. ///////////
// 
// File: ge_mapmanager.h
// Description: Keeps track of all relevant map data
//
///////////////////////////////////////////////////////////////////////////////

#ifndef GE_MAPMANAGER_H
#define GE_MAPMANAGER_H

#include "ge_gameplay.h"
#include "ge_loadout.h"
#include "ge_spawner.h"

// Struct that holds data relevant to map selection
struct MapSelectionData
{
	char	mapname[32];
	int		baseweight;
	int		minplayers;
	int		maxplayers;
	int		teamthreshold;
	int		resintensity;
};

struct MapWeightData
{
	char	mapname[32];
	int		weight;
};

class CGEMapManager
{
public:
	DECLARE_CLASS_NOBASE(CGEMapManager);
	
	CGEMapManager(void);
	~CGEMapManager(void);

	// Go through the map script files and store all of their data relevant to selection in m_pSelectionData
	void ParseMapSelectionData(void);

	// Parse our rotation logs so we don't pick the same maps over and over
	void ParseLogData(void);

	// Get the current map script file and load all of its data into m_pCurrentSelectionData
	void ParseMapData( const char *mapname );

	// Get the current map script file and load all of its data into m_pCurrentSelectionData.
	void ParseCurrentMapData(void);

	// Prints out the entire map registry to the server console.
	void PrintMapSelectionData(void);

	// Prints out the gamemode weights and set blacklists to the console.
	void PrintMapDataLists(void);

	// Prints out the liklyhood of picking a given map at this playercount.
	void PrintMapSelectionWeights( int pcount, bool sorted, bool consoleprint );

	// Prints out if we can switch to this map or not.
	void PrintMapSwitchStatus(const char* mapname);

	// Get selection data for a specific map and write it to mincount and maxcount
	MapSelectionData* GetMapSelectionData(const char *mapname);

	// Get selection data for a specific map and write it to mincount and maxcount
	MapSelectionData* GetCurrentMapSelectionData()	{ return m_pCurrentSelectionData; };

	// Get the gameplay list for the map and write to the given addresses.
	void GetMapGameplayList( CUtlVector<char*> &gameplays, CUtlVector<int> &weights, bool teamplay = false );

	// Get list of maps suitable for current playercount and conditions.
	void GetViableMapList( int iNumPlayers, CUtlVector<char*> &mapnames, CUtlVector<int> &mapweights );

	// Get the gameplay list for the map and write to the given addresses.
	void GetRecentMaps( CUtlVector<const char*> &mapnames );

	// Pick and return a new map using the current server conditions.
	const char* SelectNewMap();

	// Get the weaponset blacklist for the map
	void GetSetBlacklist( CUtlVector<char*> &sets, CUtlVector<int> &weights );

private:

	// Array that holds data from all the maps relevant to selection.
	CUtlVector< MapSelectionData* >	m_pSelectionData;

	// Pointer to the selection data of the current map
	MapSelectionData* m_pCurrentSelectionData;

	// Pointer to the default map selection data
	MapSelectionData* m_pDefaultSelectionData;

	// Array that holds names of the weaponsets that have their weights overridden by the map.  It doesn't have to be a blacklist.
	CUtlVector<char*>	m_pLoadoutBlacklist;
	// Array that holds the map weight override values.
	CUtlVector<int>	m_pLoadoutWeightOverrides;

	// Array that holds data from the current map regarding gamemodes
	CUtlVector<char*>	m_pMapGamemodes;
	// Array that holds data from the current map regarding gamemode weights
	CUtlVector<int>		m_pMapGamemodeWeights;

	// Array that holds data from the current map regarding team gamemodes
	CUtlVector<char*>	m_pMapTeamGamemodes;
	// Array that holds data from the current map regarding team gamemode weights
	CUtlVector<int>		m_pMapTeamGamemodeWeights;

	// Array that holds the recently played maps, logged in a text file.
	CUtlVector< MapSelectionData* >	m_pRecentMaps;
};

#endif //GE_MAPMANAGER_H
