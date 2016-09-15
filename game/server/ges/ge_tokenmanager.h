///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_tokenmanager.h
// Description:
//      see ge_tokenresource.cpp
//
// Created On: 06 SEP 09
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#ifndef GE_TOKENMANAGER
#define GE_TOKENMANAGER
#ifdef _WIN32
#pragma once
#endif

#include "baseentity.h"
#include "ge_player.h"
#include "ge_weapon.h"
#include "ge_spawner.h"
#include "ent_capturearea.h"
#include "ge_shareddefs.h"

class CGETokenDef
{
public:
	CGETokenDef();

	char	szClassName[MAX_ENTITY_NAME];
	char	szModelName[2][MAX_MODEL_PATH];		// 0 = View Model, 1 = World Model
	char	szPrintName[MAX_ENTITY_NAME];		// Custom print name for token_* types

	int		iTeam;				// For generic tokens, restrict pickup to certain teams
	int		iLimit;				// Maximum number that can be spawned at one time
	int		iLocations;			// Where can we spawn?
	float	flRespawnDelay;		// Static respawn delay (default = 10sec)
	float	flDmgMod;			// Modify the base damage of the token_* types
	bool	bAllowSwitch;		// Can we switch weapons from this token?
	int		nSkin;				// Custom token skin to use (default 0)
	Color	glowColor;			// Glow color to make tokens stand out
	float	glowDist;			// Glow distance

	bool	bDirty;				// Indicates we need to refresh our spawned tokens

	// Remember which spawners we affected
	CUtlVector< CHandle<CGESpawner> > vSpawners;
	// Retain a list of tokens we know about
	CUtlVector<EHANDLE> vTokens;

private:
	// No copy!
	CGETokenDef( CGETokenDef & ) { }
};

class CGECaptureAreaDef
{
public:
	CGECaptureAreaDef();

	char  szClassName[MAX_ENTITY_NAME];
	char  szModelName[MAX_MODEL_PATH];
	int	  nSkin;
	Color glowColor;		// Glow color to make areas stand out
	float glowDist;			// Glow distance
	int	  iLimit;
	int   iLocations;
	float fRadius;
	float fSpread;
	// Requirements to "enter" the capture area
	char  rqdToken[MAX_ENTITY_NAME];
	int   rqdTeam;

	bool  bDirty;	// Indicates we need to refresh our spawned capture areas

	CUtlVector< CHandle<CGECaptureArea> > vAreas;

private:
	// No copy!
	CGECaptureAreaDef( CGECaptureAreaDef & ) { }
};

// -------------------------
// Class: CGETokenManager
// -------------------------
class CGETokenManager
{
	DECLARE_CLASS_NOBASE( CGETokenManager );

public:
	CGETokenManager( void );
	~CGETokenManager( void );

	// Tokens
	CGETokenDef* GetTokenDefForEdit( const char *szName );
	void		 RemoveTokenDef( const char *szName );

	bool IsValidToken( const char *szClassName )	{ return GetTokenDef(szClassName) != NULL; }
	int  GetNumTokenDef( void )						{ return m_vTokenTypes.Count(); }

	void RemoveTokenEnt( CGEWeapon *pToken, bool bAdjustLimit = true );
	void TransferToken( CGEWeapon *pToken, CGEPlayer *pNewOwner );
	void CaptureToken( CGEWeapon *pToken );

	// Capture Areas
	CGECaptureAreaDef*	GetCaptureAreaDefForEdit( const char *szName );
	void				RemoveCaptureAreaDef( const char *szName );

	bool				CanTouchCaptureArea( const char *szName, CBaseEntity *pEntity );
	const char*			GetCaptureAreaToken( const char *szName );

	// Global ammo helpers
	void SetGlobalAmmo( const char *szClassName, int amount = -1 );
	void RemoveGlobalAmmo( const char *szClassName );
	void ClearGlobalAmmo();

	bool HasGlobalAmmo() { return m_vGlobalAmmo.Count() > 0; }
	bool GiveGlobalAmmo( CBasePlayer *pPlayer );

	// Returns the list of tokens of a specific type
	void FindTokens( const char *szClassName, CUtlVector<EHANDLE> &tokens );

	// Internal checkers for spawner entities and weapons
	bool ShouldSpawnToken( const char *szClassName, CGESpawner *pSpawner );
	bool GetTokenRespawnDelay( const char *szClassName, float *flDelay );

	// TODO: Replace all these public functions with "ResetWorld"
	// Functions for Token and Capture Area Spawning
	void SpawnTokens( const char *token = NULL );
	void RemoveTokens( const char *token = NULL, int count = -1 );
	void ResetTokenSpawners( const char *token = NULL, int count = -1 );

	void SpawnCaptureAreas( const char *name = NULL );
	void RemoveCaptureAreas( const char *name = NULL, int count = -1 );

	// Called from CGEWeapon
	void OnTokenSpawned( CGEWeapon *pToken );
	void OnTokenRemoved( CGEWeapon *pToken );
	void OnTokenPicked( CGEWeapon *pToken, CGEPlayer *pPlayer );
	void OnTokenDropped( CGEWeapon *pToken, CGEPlayer *pPlayer );

	// Called from CGECaptureArea
	void OnCaptureAreaSpawned( CGECaptureArea *pArea );
	void OnCaptureAreaRemoved( CGECaptureArea *pArea );

	// Should only be called by GERules in the think cycle
	void EnforceTokens();
	// Called when the gameplay is loaded
	void Reset();

	enum LocationFlag
	{
		LOC_NONE		= 0,
		LOC_AMMO		= 0x1,	 // The default location
		LOC_WEAPON		= 0x2,
		LOC_TOKEN		= 0x4,
		LOC_CAPAREA		= 0x8,
		LOC_PLAYERSPAWN = 0x10,
		LOC_MYTEAM		= 0x20,	 // Make an effort to be near our associated team
		LOC_OTHERTEAM	= 0x40,	 // Make an effort to be near the opposite team
		LOC_SPECIALONLY = 0x80,	 // Make an effort to be at "special" spawns only
		LOC_ALWAYSNEW	= 0x100, // Find a new spawn on each respawn
		LOC_FIXEDMATCH	= 0x200, // The spawn location will remain the same throughout the match
		LOC_MAX,
	};

private:
	// Accessors to our dictionary
	CGETokenDef			*GetTokenDef( const char *szClassName );
	CGECaptureAreaDef	*GetCapAreaDef( const char *szName );

	void ApplyTokenSettings( CGETokenDef *ttype, CGEWeapon *pToken = NULL );
	void ApplyCapAreaSettings( CGECaptureAreaDef *ca, CGECaptureArea *pArea = NULL );

	// Uses our location flags to build a list of spawners sorted by desirability
	void FindSpawnersForToken( CGETokenDef *token, CUtlVector<EHANDLE> &spawners );
	int  FilterSpawnersForToken( int spawner_type, int count, CUtlVector<EHANDLE> &out, bool special_only, Vector centroid = vec3_origin, float max_dist = MAX_TRACE_LENGTH );
	
	void FindSpawnersForCapArea( CGECaptureAreaDef *ca, CUtlVector<EHANDLE> &spawners );
	int  FilterSpawnersForCapArea( CGECaptureAreaDef *ca, int spawner_type, int count, CUtlVector<Vector> &existing, CUtlVector<EHANDLE> &out );

	struct sGlobalAmmo
	{
		int	aID;
		int amount;
	};

	float m_flNextEnforcement;
	float m_flNextCapSpawn;

	// Lists of tokens, global ammo, and capture areas
	CUtlDict<CGETokenDef*, int>			m_vTokenTypes;
	CUtlDict<sGlobalAmmo, int>			m_vGlobalAmmo;
	CUtlDict<CGECaptureAreaDef*, int>	m_vCaptureAreas;
};

#endif
