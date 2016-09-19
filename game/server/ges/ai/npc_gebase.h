///////////// Copyright © 2008 LodleNet. All rights reserved. /////////////
//
//   Project     : Server (GES)
//   File        : npc_gebase.h
//   Description :
//      [TODO: Write the purpose of npc_gebase.h.]
//
//   Created On: 12/27/2008 1:53:24 PM
//   Created By: Mark Chandler <mailto:admin@lodle.net>
////////////////////////////////////////////////////////////////////////////

#ifndef MC_NPC_GEBASE_H
#define MC_NPC_GEBASE_H
#ifdef _WIN32
#pragma once
#endif

class CNPC_GEBase;

#include "ai_basenpc.h"
#include "ai_basehumanoid.h"
#include "ai_behavior.h"
#include "ai_behavior_actbusy.h"
#include "ai_sentence.h"
#include "ai_baseactor.h"
#include "npcevent.h"

#include "gebot_player.h"
#include "ge_ai.h"

extern int g_GEAINextScheduleID;
extern int g_GEAINextConditionID;
extern int g_GEAINextTaskID;

enum NPC_LEVEL
{
	LEVEL_AGENT,
	LEVEL_SECRET_AGENT,
	LEVEL_00_AGENT,
	LEVEL_007
};

class CNPC_GEBase : public CAI_BaseActor
{
public:
	CNPC_GEBase();
	~CNPC_GEBase();

	DECLARE_CLASS( CNPC_GEBase, CAI_BaseActor );
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
	DEFINE_CUSTOM_AI;

// Python and Bot Interface
	virtual const char	 *GetIdent()	 { return m_szIdent; }
	virtual CGEBotPlayer *GetBotPlayer() { return m_hBotPlayer.Get(); }

	virtual bool	InitPyInterface( const char *ident );
	virtual void	RemovePyInterface();
	virtual void	InitBotInterface( CGEBotPlayer *bot );
	virtual void	SendDebugCommand( const char *cmd );
	bool			IsSelected() { return (m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) != 0; }

// Standard Entity functions
	virtual void	Precache( void );
	virtual void	Spawn( void );

// Standard NPC functions
	virtual Class_T Classify( void );
	virtual const char* GetNPCModel();

	virtual void	NPCThink();

	virtual void	OnLooked( int dist );
	virtual void	OnListened( void );

	virtual	bool	IsValidEnemy( CBaseEntity *pEnemy );
	virtual void	GatherConditions( void );
	virtual int		GetSoundInterests( void );
	NPC_STATE		SelectIdealState( void );

	virtual int		SelectSchedule();
	virtual int		TranslateSchedule( int scheduleType );
	virtual bool	IsScheduleValid();
	virtual void	OnStartSchedule( int scheduleType );

	virtual void	StartTask( const Task_t *pTask );
	virtual void	RunTask( const Task_t *pTask );

	virtual void	RunTaskRangeAttack1( const Task_t *pTask );
	virtual void	RunTaskReload( const Task_t *pTask );

	virtual void	TaskFail( AI_TaskFailureCode_t code );
	virtual bool	IsCurTaskContinuousMove();
	virtual bool	IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos ) const;

// Helper Functions
	virtual bool	HasWeapon(CBaseEntity *pEnt);
	CBaseEntity*	GiveNamedItem( const char *pszName, int iSubType = 0 );
	
	virtual void	TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator = NULL );
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	virtual int		OnTakeArmorDamage( const CTakeDamageInfo &inputinfo );
	virtual bool	IsHeavyDamage( const CTakeDamageInfo &info );

	const char*		GetScheduleName( int id );
	const char*		GetTaskName( int id );

	void			SetTargetPos( const Vector &target );
	Vector			m_vTargetPos;

// Navigation Functions
	virtual bool	IsNavigationUrgent();
	virtual bool	ShouldFailNav( bool bMovementFailed );
	virtual bool	ShouldBruteForceFailedNav();
	virtual bool	OnObstructionPreSteer( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult );
	virtual bool	OnObstructingDoor( AILocalMoveGoal_t *pMoveGoal, CBaseDoor *pDoor, float distClear, AIMoveResult_t *pResult );
	virtual bool	OnUpcomingPropDoor( AILocalMoveGoal_t *pMoveGoal, CBasePropDoor *pDoor, float distClear, AIMoveResult_t *pResult );

// Weapon Functions
	virtual bool	ShouldLookForBetterWeapon();
	virtual bool	BumpWeapon( CGEWeapon *pWeapon );
	virtual	bool	Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex = 0 );
	virtual void	Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget = NULL, const Vector *pVelocity = NULL );
	virtual void	OnUpdateShotRegulator( void );

	// Defined in npc_gebase_aiming.cpp
	virtual Vector	GetActualShootPosition( const Vector &shootOrigin );
	virtual Vector	GetActualShootTrajectory( const Vector &shootOrigin );
	virtual	Vector	GetAttackSpread( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget = NULL );
	virtual	float	GetSpreadBias( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget );
	virtual float	GetReactionDelay( CBaseEntity *pEnemy );

// Animations
	virtual Vector	 Weapon_ShootPosition( void );
	virtual Activity Weapon_TranslateActivity( Activity baseAct, bool *pRequired = NULL );
	virtual void	 HandleAnimEvent( animevent_t *pEvent );
	virtual int		 SelectFlinchSchedule();
	virtual float	 GetSequenceGroundSpeed( CStudioHdr *pStudioHdr, int iSequence );

// Hacky Overrides for bot support
	virtual bool	CanBecomeRagdoll();
	virtual void	Event_Killed( const CTakeDamageInfo &info );
	virtual void	Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info );

// Ragdoll Support
	virtual void	UpdateOnRemove();
	virtual bool	BecomeRagdollOnClient( const Vector &force );
	virtual void	CreateRagdoll( const Vector &force );

	// Tracks our ragdoll entity.
	CNetworkHandle( CBaseEntity, m_hRagdoll );

// Difficulty Settings
	virtual void	SetDifficulty( int difficulty );
	static const char* TranslateLevel( int level );

	int				m_iLevel;
	float			m_fSpreadMod;
	float			m_fShotBias;
	float			m_fHeadShotProb;
	float			m_fReactionDelay;
	float			m_fFocusTime;
	float			m_fBurstMod;

protected:
// Weapon Functions
	virtual void		FireBullets( const FireBulletsInfo_t &info );
	WeaponProficiency_t CalcWeaponProficiency( CBaseCombatWeapon *pWeapon );
	
// Python storage
	char		m_szIdent[64];
	CGEBaseNPC*	m_pPyNPC;

// Hacks
	bool		m_bTraceCalled;

// Reference back to my player proxy (if I have one)
	CNetworkHandle( CGEBotPlayer, m_hBotPlayer );
	CNetworkVar( bool, m_bSpawnInterpCounter );

public:
	enum
	{
		TASK_CLEAR_GOAL = BaseClass::NEXT_TASK,
		NEXT_TASK,
	};
};

#endif //MC_NPC_GEBASE_H
