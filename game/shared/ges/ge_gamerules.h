///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_gamerules.h
// Description:
//      GoldenEye game rules baseclass
//
// Created On: 28 Feb 08, 23:00
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#ifndef GE_GAMERULES_H
#define GE_GAMERULES_H

#include "ge_shareddefs.h"
#include "multiplay_gamerules.h"

#ifdef CLIENT_DLL
	#define CGERules C_GERules
#endif

class CGEPlayer;

// Mins & Maxs for dynamic collision bounds for proper hit detection
static Vector VEC_DUCK_SHOT_MIN( -20, -20,  0 );
static Vector VEC_DUCK_SHOT_MAX(  35,  20, 60 );
static Vector VEC_SHOT_MIN( -20, -20,  0 );
static Vector VEC_SHOT_MAX(  40,  20, 72 );

class CGERules : public CMultiplayRules
{
public:
	DECLARE_CLASS( CGERules, CMultiplayRules );
	
	CGERules();
	~CGERules();

	bool CheckInPVS(CBaseEntity *ent, byte iPVS[MAX_MAP_CLUSTERS / 8]);

	// ---------------
	// GES Functions
#ifdef GAME_DLL
	virtual void ClientActive( CBasePlayer *pPlayer ) { }
	virtual bool ShouldForcePickup( CBasePlayer *pPlayer, CBaseEntity *pItem );

	virtual CBaseEntity	*GetSpectatorSpawnSpot( CGEPlayer *pPlayer );

	const CUtlVector<EHANDLE> *GetSpawnersOfType( int type );
	void UpdateSpawnerLocations();
	bool GetSpawnerStats( int spawn_type, Vector &mins, Vector &maxs, Vector &centroid );

	// Respawns all active players
	void SpawnPlayers();

	bool HaveStatusListsUpdated() { return m_bStatusListsUpdated; }

protected:
	// Reload the game world
	void WorldReload();

	void LoadMapCycle();
	void ClearSpawnerLocations();

	bool CheckVotekick();

	char m_pKickTargetID[64];
	CGEPlayer *m_pLastKickCaller;
	float m_flLastKickCall;
	float m_flKickEndTime;

	int m_iVoteCount;
	int m_iVoteGoal;
	float m_flVoteFrac;
	CUtlVector<int> m_vVoterIDs;
#endif

	// -------------------
	// Client-Server Shared
public:
	virtual void Precache();

	virtual bool ShouldCollide( int collisionGroup0, int collisionGroup1 );
	virtual bool Damage_ShouldGibCorpse( int iDmgType ) { return false; };

	virtual const CViewVectors* GetViewVectors() const;

	virtual const unsigned char *GetEncryptionKey();

	virtual bool ShouldUseRobustRadiusDamage( CBaseEntity *pEntity );

	// -------------------
	// Server Only - Inherited
#ifdef GAME_DLL
	// ---------------
	// AutoGameSystemPerFrame
	virtual void LevelInitPostEntity();

	// ---------------
	// GameRules Functions
	virtual void RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore );

	virtual bool CanHaveItem( CBasePlayer *pPlayer, CItem *pItem );
	virtual bool CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon );
	
	virtual bool ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );
	virtual bool ClientCommand( CBaseEntity *pEdict, const CCommand &args );
	virtual void ClientSettingsChanged( CBasePlayer *pPlayer );
	
	virtual bool FlPlayerFallDeathDoesScreenFade( CBasePlayer *pl ) { return false; }
	virtual bool FPlayerCanRespawn( CBasePlayer *pPlayer );
	virtual bool FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker );
	virtual bool FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon );
	
	virtual const char *GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer );
	virtual const char *GetGameDescription() { return "GoldenEye: Source"; }
	virtual void		GetTaggedConVarList( KeyValues *pCvarTagList );

	virtual void InitDefaultAIRelationships();
	virtual void InitHUD( CBasePlayer *pl );

	virtual int  IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	virtual bool IsSpawnPointValid( CBaseEntity *pSpot, CBasePlayer *pPlayer  );

	virtual int  PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );

	virtual void Think();

private:
	bool m_bStatusListsUpdated;

	// Spawner Stats
	struct SpawnerStats
	{
		SpawnerStats() : centroid(vec3_origin), mins(-MAX_TRACE_LENGTH), maxs(MAX_TRACE_LENGTH) { }
		Vector centroid;
		Vector mins;
		Vector maxs;
	};
	CUtlMap<int, SpawnerStats*> m_vSpawnerStats;
	CUtlMap<int,CUtlVector<EHANDLE>*> m_vSpawnerLocations;
#endif
};

inline CGERules* GERules()
{
	return (CGERules*) g_pGameRules;
}

#endif //GE_GAMERULES_H
