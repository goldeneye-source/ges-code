///////////// Copyright © 2008 GoldenEye: Source. All rights reserved. /////////////
//
//   Project     : Server (GES)
//   File        : gemp_player.h
//
////////////////////////////////////////////////////////////////////////////

#ifndef MC_GEMP_PLAYER_H
#define MC_GEMP_PLAYER_H

#include "ge_player.h"
#include "ge_gameplay.h"

class CGEMPPlayer : public CGEPlayer, public CGEGameplayEventListener
{
public:
	DECLARE_CLASS( CGEMPPlayer, CGEPlayer );

	CGEMPPlayer();
	~CGEMPPlayer();

	bool IsMPPlayer() { return true;  }
	bool IsSPPlayer() { return false; }

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	float GetSpawnTime() { return m_flSpawnTime; };

	virtual bool IsInRound() { return (!IsSpawnState( SS_BLOCKED_GAMEPLAY ) && IsActive()); }
	virtual bool IsActive()  { return (!m_bPreSpawn && GetTeamNumber() != TEAM_SPECTATOR); }

	virtual bool ShouldCollide( int collisionGroup0, int contentsMask ) const;

    virtual void PreThink();
	virtual void PostThink();

	virtual void Precache();

	// -------------------------------------------------
	// Scoring System
	// -------------------------------------------------
	virtual int  GetRoundScore()			{ return m_iRoundScore; }
	virtual void SetRoundScore( int val )	{ m_iRoundScore = val; }
	virtual void AddRoundScore( int val )	{ m_iRoundScore += val; }
	virtual void ResetRoundScore()			{ m_iRoundScore = 0; }
	
	virtual int  GetMatchScore()			{ return m_iMatchScore; }
	virtual void SetMatchScore( int val )	{ m_iMatchScore = val; }
	virtual void AddMatchScore( int val )	{ m_iMatchScore += val; }
	virtual void ResetMatchScores()			{ m_iMatchScore = 0; m_iMatchDeaths = 0; }

	virtual void ResetScores()				{ BaseClass::ResetScores(); ResetRoundScore(); }

	virtual int	 GetRoundDeaths()			{ return DeathCount(); }
	virtual int  GetMatchDeaths()			{ return m_iMatchDeaths; }
	virtual void AddMatchDeaths( int val )	{ m_iMatchDeaths += val; }
	virtual void SetDeaths( int amt )		{ ResetDeathCount(); IncrementDeathCount(amt); }

	// -------------------------------------------------
	// Player Models and Teams
	// -------------------------------------------------
	virtual void SetPlayerModel( const char* szCharName, int iCharSkin = 0, bool bNoSelOverride = false );
	virtual void SetPlayerTeamModel();
	virtual void PickDefaultSpawnTeam();
	virtual void ChangeTeam( int iTeam, bool bWasForced = false );
	virtual bool HandleCommand_JoinTeam( int team );
	virtual void ShowTeamNotice( const char *title, const char *msg, const char *img );

	// Observer functions to work with Bots
	virtual bool StartObserverMode( int mode );
	virtual bool SetObserverTarget( CBaseEntity* target );
	virtual bool IsValidObserverTarget( CBaseEntity* target );
	virtual void CheckObserverSettings();
	virtual int	 GetNextObserverSearchStartPoint( bool bReverse );
	virtual void ObserverTransistion();

	virtual CBaseEntity* EntSelectSpawnPoint();
	virtual CBaseEntity* EntFindSpawnPoint( int iStart, int iType, int &iReturn,bool bAllowWrap = true, bool bRandomize = false );
	virtual bool CheckForValidSpawns( int iSpawnerType = -1 );

	// Spawning functions
	virtual void InitialSpawn();
	virtual void Spawn();
	virtual void ForceRespawn();
	// Makes sure the player will not be active until they choose a character
	virtual void SetPreSpawn()					{ m_bPreSpawn = true; SetSpawnState( SS_INITIAL ); }
	virtual bool IsPreSpawn()					{ return m_bPreSpawn; }
	// NOTE: Used by Python ONLY
	virtual bool IsInitialSpawn()				{ return m_bFirstSpawn; }
	virtual void SetInitialSpawn()				{ m_bFirstSpawn = true; }

	// Death functions
	virtual void Event_Killed( const CTakeDamageInfo &info );
	virtual void NotifyOnDeath();

	// GE Special Functions
	virtual void UpdateCampingTime();
	virtual int  GetCampingPercent();

	virtual int      GetSteamHash()  { return m_iSteamIDHash; }
	virtual int      GetDevStatus()   { return m_iDevStatus; }
	virtual uint64  GetSkinsCode()   { return m_iSkinsCode; }

	virtual void		 SetPlayerName( const char *name );
	virtual const char  *GetCleanPlayerName()	{ return m_szCleanName; }

	// Name that's safe for python to use, without color tags.
	virtual const char  *GetSafeCleanPlayerName()	{ return m_szSafeCleanName; }

	// Name that's safe for python to use, still contains color tags.
	virtual const char  *GetSafePlayerName()	{ return m_szSafeName; }

	virtual int		GetUsedWeaponSkin(int weapid)				{ return weapid < WEAPON_RANDOM ? m_iWeaponSkinInUse.Get(weapid) : 0; }
	virtual void	SetUsedWeaponSkin(int weapid, int value)	{ if (weapid < WEAPON_RANDOM) m_iWeaponSkinInUse.Set(weapid, value); }

	virtual void  ShowScenarioHelp();

	// Gameplay Events
	void OnGameplayEvent( GPEvent event );

	// Called when the player disconnects so that we don't have unassigned objects we had control over
	virtual void  AddThrownObject( CBaseEntity *pObj );
	virtual void  RemoveLeftObjects();

	virtual void  FreezePlayer( bool state = true );
	virtual void  QuickStartObserverMode();

	virtual float GetTeamJoinTime()				{ return m_flTeamJoinTime; }
	virtual int	  GetAutoTeamSwaps()			{ return m_iAutoTeamSwaps; }
	virtual float GetNextJumpTime()				{ return m_flNextJumpTime; }
	virtual void  SetNextJumpTime( float time ) { m_flNextJumpTime = time; }

	virtual float GetJumpPenalty()				{ return m_flJumpPenalty; }
	virtual void  SetJumpPenalty(float time)	{ m_flJumpPenalty = time; }

	virtual void  SetStartJumpZ( float val )	{ m_flStartJumpZ = val; }
	virtual float GetStartJumpZ()				{ return m_flStartJumpZ; }

	virtual float GetLastJumpTime()				{ return m_flLastJumpTime; }
	virtual void  SetLastJumpTime(float time)	{ m_flLastJumpTime = time; }

	virtual void  SetLastLandingVelocity(float vel)	{ m_flLastLandVelocity = vel; }
	virtual float GetLastLandingVelocity()		{ return m_flLastLandVelocity; }

	virtual void UpdateJumpPenalty();

	virtual float GetRunStartTime()					{ return m_flRunTime; }
	virtual void  SetRunStartTime(float time)		{ m_flRunTime = time; }
	
	virtual int   GetRunCode()					{ return m_flRunCode; }
	virtual void  SetRunCode(float code)		{ m_flRunCode = code; }

	virtual CBaseEntity	*GiveNamedItem( const char *szName, int iSubType = 0 );
	virtual void  GiveDefaultItems();
	virtual bool  ClientCommand( const CCommand &args );

	virtual float GetLastNameChangeTime()		{ return m_flLastNameChange; };
	virtual int GetNameChangeCount()			{ return m_iNameChangeCount; };

	virtual void SetLastNameChangeTime( float newtime )	{ m_flLastNameChange = newtime; };
	virtual void SetNameChangeCount( int amount )	{ m_iNameChangeCount = amount; };

	virtual bool  BumpWeapon( CBaseCombatWeapon *pWeapon );

	virtual void  FinishClientPutInServer();

	enum SpawnState {
		SS_INITIAL = 1,
		SS_ACTIVE,
		SS_BLOCKED_NO_SPOT,
		SS_BLOCKED_GAMEPLAY,
	};

	// Spawn system info functions
	Vector	GetLastDeath() { return m_vLastDeath; }
	Vector	GetLastSpawn() { return m_vLastSpawn; }
	float	GetLastSpawnTime() { return m_flSpawnTime; }


protected:
	// Spawn state management
	void SetSpawnState( SpawnState state );
	SpawnState GetSpawnState() { return m_iSpawnState; }
	bool IsSpawnState( SpawnState state ) { return state == m_iSpawnState; }

	// Used during team swaps
	float m_flTeamJoinTime;
	int m_iAutoTeamSwaps;

	// Used to throttle bunnyhopping
	CNetworkVar( float,	m_flStartJumpZ );
	CNetworkVar( float,	m_flNextJumpTime );
	CNetworkVar( float, m_flLastJumpTime );
	CNetworkVar( float, m_flJumpPenalty );
	CNetworkVar( float, m_flLastLandVelocity);

	CNetworkVar( float, m_flRunTime );
	CNetworkVar( int, m_flRunCode );
	CNetworkArray( int, m_iWeaponSkinInUse, WEAPON_RANDOM ); //Random marks the last of the actual weapons.

	CNetworkVar(int, m_iSteamIDHash);

	char m_szCleanName[32];

	char m_szSafeName[32]; // Name that's safe for python to use.
	char m_szSafeCleanName[32]; // Clean name that's safe for python to use.

	int m_iMatchScore;
	int m_iMatchDeaths;

	int m_iDevStatus;
	uint64 m_iSkinsCode;
	uint64 m_iClientSkinsCode;

	// This vector keeps track of all the objects thrown by the player
	// so they don't overpopulate the world
	CUtlVector<CBaseHandle> m_hThrownObjects;

	// Used to determine camping
	Vector	m_vLastOrigin;		
	float	m_flNextCampCheck;
	float	m_flLastMoveTime;
	float	m_flCampingTime;
	int		m_iCampingState;

	// Achievement report
	char	m_szAchList[1028];
	int		m_iAchCount;
	bool	m_bAchReportPending;

	int		m_iLastSpecSpawn;

	float	m_flSpawnTime;
	float	m_flNextModelChangeTime;
	float	m_flNextTeamChangeTime;

	// Used to indicate the status of the player
	SpawnState m_iSpawnState;
	float	m_flNextSpawnTry;
	float	m_flNextSpawnWaitNotice;
	bool	m_bFirstSpawn;
	bool	m_bPreSpawn;
	bool	m_bSpawnFromDeath;

	float	m_flLastNameChange;
	int		m_iNameChangeCount;

	// Spawn system personal modifiers.
	Vector	m_vLastDeath;
	Vector	m_vLastSpawn;
};

CGEMPPlayer *ToGEMPPlayer( CBaseEntity *pEntity );

#define FOR_EACH_MPPLAYER( var ) \
	for( int _iter=1; _iter <= gpGlobals->maxClients; ++_iter ) { \
		CGEMPPlayer *var = ToGEMPPlayer( UTIL_PlayerByIndex( _iter ) ); \
		if ( var == NULL || FNullEnt( var->edict() ) || !var->IsPlayer() ) \
			continue;

#define END_OF_PLAYER_LOOP() }

#endif //MC_GEMP_PLAYER_H
