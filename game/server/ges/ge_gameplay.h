///////////// Copyright © 2013, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_gameplay.h
// Description:
//      Gameplay Manager Definition
//
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////

#ifndef MC_GE_GAMEPLAY_H
#define MC_GE_GAMEPLAY_H

class CGEPlayer;
class CBaseEntity;
class CGEWeapon;
class CGEArmorVest;
class CGECaptureArea;

// --------------------
// Control of the gameplay manager
// --------------------
extern void CreateGameplayManager();
extern void ShutdownGameplayManager();

// --------------------
// Base Scenario Definition
// --------------------
class CGEBaseScenario
{
public:
	CGEBaseScenario();

protected:
	virtual void Init()=0;
	virtual void Shutdown()=0;

	friend class CGEBaseGameplayManager;

public:
	// Convar controls
	void LoadConfig();
	void CreateCVar(const char* name, const char* defValue, const char* help);
	void UnloadConfig();

	// Check official
	bool IsOfficial()  { return m_bIsOfficial; }
	void SetIsOfficial( bool state )  { m_bIsOfficial = state; }

	virtual const char* GetIdent()=0;
	virtual const char* GetGameDescription()=0;
	virtual const char* GetPrintName()=0;
	virtual int GetHelpString()=0;
	virtual int GetTeamPlay()=0;

	virtual void ClientConnect(CGEPlayer *pPlayer)=0;
	virtual void ClientDisconnect(CGEPlayer *pPlayer)=0;

	virtual void OnPlayerSpawn(CGEPlayer *pPlayer)=0;
	virtual void OnPlayerObserver(CGEPlayer *pPlayer)=0;
	virtual void OnPlayerKilled(CGEPlayer *pVictim, CGEPlayer *pKiller, CBaseEntity *pWeapon)=0;
	virtual bool OnPlayerSay(CGEPlayer* pPlayer, const char* text)=0;

	virtual bool CanPlayerRespawn(CGEPlayer *pPlayer)=0;
	virtual bool CanPlayerChangeChar(CGEPlayer* pPlayer, const char* szIdent)=0;
	virtual bool CanPlayerChangeTeam(CGEPlayer *pPlayer, int iOldTeam, int iNewTeam, bool wasForced = false)=0;
	virtual bool CanPlayerHaveItem(CGEPlayer *pPlayer, CBaseEntity *pEntity)=0;
	virtual bool ShouldForcePickup(CGEPlayer *pPlayer, CBaseEntity *pEntity)=0;
	virtual void CalculateCustomDamage(CGEPlayer *pVictim, const CTakeDamageInfo &inputInfo, float &health, float &armor)=0;

	virtual void BeforeSetupRound()=0;
	virtual void OnRoundBegin()=0;
	virtual void OnRoundEnd()=0;
	virtual void OnThink()=0;
	virtual void OnCVarChanged(const char* name, const char* oldvalue, const char* newvalue)=0;

	virtual bool CanRoundEnd()=0;
	virtual bool CanMatchEnd()=0;

	virtual void OnCaptureAreaSpawned(CGECaptureArea *pCapture)=0;
	virtual void OnCaptureAreaRemoved(CGECaptureArea *pCapture)=0;
	virtual void OnCaptureAreaEntered(CGECaptureArea *pCapture, CGEPlayer *pPlayer, CGEWeapon *pToken)=0;
	virtual void OnCaptureAreaExited(CGECaptureArea *pCapture, CGEPlayer *pPlayer)=0;

	virtual void OnTokenSpawned(CGEWeapon *pToken)=0;
	virtual void OnTokenRemoved(CGEWeapon *pToken)=0;
	virtual void OnTokenPicked(CGEWeapon *pToken, CGEPlayer *pPlayer)=0;
	virtual void OnTokenDropped(CGEWeapon *pToken, CGEPlayer *pPlayer)=0;
	virtual void OnEnemyTokenTouched(CGEWeapon *pToken, CGEPlayer *pPlayer) = 0;
	virtual void OnTokenAttack(CGEWeapon *pToken, CGEPlayer *pPlayer, Vector position, Vector forward)=0;

	virtual void OnWeaponSpawned(CGEWeapon *pWeapon)=0;
	virtual void OnWeaponRemoved(CGEWeapon *pWeapon)=0;

	virtual void OnArmorSpawned(CBaseEntity *pArmor)=0;
	virtual void OnArmorRemoved(CBaseEntity *pArmor)=0;

	virtual void OnAmmoSpawned(CBaseEntity *pAmmo)=0;
	virtual void OnAmmoRemoved(CBaseEntity *pAmmo)=0;

private:
	bool m_bIsOfficial;
	CUtlVector<ConVar*> m_vCVarList;

	// No copies allowed
	CGEBaseScenario(const CGEBaseScenario &) { }
};


// Class that enables event based messaging of gameplay events
// to all registered listeners. Listeners are registered automatically
// in the CGEGameplayEventListener constructor
enum GPEvent {
	SCENARIO_INIT,
	SCENARIO_POST_INIT,
	MATCH_START,
	ROUND_START,
	ROUND_END,
	MATCH_END,
	SCENARIO_SHUTDOWN
};

class CGEGameplayEventListener
{
public:
	// Self register/deregister
	CGEGameplayEventListener();
	~CGEGameplayEventListener();

	virtual void OnGameplayEvent( GPEvent event )=0;
};


class CGEBaseGameplayManager
{
public:
	CGEBaseGameplayManager();
	~CGEBaseGameplayManager();

// Abstract methods to access Python Manager implementation
protected:
	// Internal loader for scenarios
	virtual bool DoLoadScenario( const char *ident )=0;
public:
	virtual CGEBaseScenario* GetScenario()=0;
	virtual bool IsValidScenario()=0;
// End Abstract
	
	virtual void Init();
	virtual void Shutdown();

	// Loads the next scenario to play
	bool LoadScenario();
	// Loads the named scenario to play
	bool LoadScenario( const char *ident );

	void GetRecentModes(CUtlVector<const char*> &modenames);

	// Round controls (does not check conditions)
	void StartRound();
	void EndRound( bool showreport = true );

	// Match controls (does not check conditions)
	void StartMatch();
	void EndMatch();

	// Round state checks
	bool IsInRound();
	int  GetRoundCount()	{ return m_iRoundCount; }

	// Round locking [TODO: Move into Python Scenario]
	void SetRoundLocked( bool state )	{ m_bRoundLocked = state; }
	bool IsRoundLocked()				{ return m_bRoundLocked; }
	
	// Intermission checks
	bool IsInIntermission();
	bool IsInRoundIntermission();
	bool IsInFinalIntermission();
	bool IsGameOver()				{ return m_bGameOver; }
	float GetRemainingIntermission();
	
	void LoadGamePlayList( const char* path );
	void PrintGamePlayList();
	// Merely verifies that the python file exists
	bool IsValidGamePlay( const char *ident );

	void OnThink();

protected:
	// Return the next scenario to load
	const char *GetNextScenario();

private:
	// Gameplay cycle management
	void InitScenario();
	void ShutdownScenario();
	void ParseLogData();

	void BroadcastMatchStart();
	void BroadcastRoundStart();
	void BroadcastRoundEnd( bool showreport );
	void BroadcastMatchEnd();

	bool ShouldEndRound();
	bool ShouldEndMatch();

	void LoadScenarioCycle();
	void ResetState();
	void ResetGameState();

	// Round State enum for tracking
	enum RoundState {
		NONE,  // No state yet, this is the null condition
		PRE_START, // First round is ready to start
		PLAYING,  // Round is being played (timer active)
		INTERMISSION, // In an intermission (timer inactive)
		GAME_OVER,  // The match is over, waiting to change map
	};

	// Think Timer
	float m_flNextThink;

	// Round and Match Timers
	float m_flRoundStart;
	float m_flMatchStart;
	
	// Round state tracker
	int m_iRoundState;
	int	m_iRoundCount;
	bool m_bRoundLocked;
	bool m_bGameOver;	// Set only when you call EndMatch(), indicates map change is coming
	float m_flIntermissionEndTime;
	
	CUtlVector<char*> m_vScenarioList;
	CUtlVector<string_t> m_vScenarioCycle;
	CUtlVector<const char*> m_vRecentScenarioList;

protected:
	// To satisfy Boost::Python requirements of a wrapper
	CGEBaseGameplayManager( const CGEBaseGameplayManager& ) { };
};

extern CGEBaseGameplayManager* GEGameplay();
extern CGEBaseScenario *GetScenario();

#endif //MC_GE_GAMEPLAY_H
