///////////// Copyright � 2012 GoldenEye: Source All rights reserved. /////////////
//
//   Project     : Server (GES)
//   File        : npc_gebase.cpp
//   Description :
//      Generic NPC entity that utilizes the core HL2 AI and a python add-in for the rest
//
//   Created On: 01/01/2012
//   Created By: Jonathan White <KillerMonkey>
////////////////////////////////////////////////////////////////////////////
#include "cbase.h"

#include "doors.h"
#include "BasePropDoor.h"
#include "func_break.h"
#include "eventqueue.h"
#include "ammodef.h"

#include "ai_memory.h"
#include "ai_pathfinder.h"

#include "ge_shareddefs.h"
#include "ent_hat.h"
#include "ge_ai.h"
#include "ge_weapon.h"
#include "ge_gameplay.h"
#include "ge_tokenmanager.h"
#include "ge_door.h"
#include "gemp_gamerules.h"

#include "npc_gebase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void BotDifficulty_Callback( IConVar *var, const char *pOldString, float flOldValue );

ConVar ge_ai_debug( "ge_ai_debug", "0", FCVAR_CHEAT, "0 = disable, 1 = normal, 2 = verbose" );
ConVar ge_bot_difficulty( "ge_bot_difficulty", "5", FCVAR_GAMEDLL, "Sets the average difficulty [0-9] of the AI", true, 0, true, 9, BotDifficulty_Callback );

#define DEFAULT_MODEL "models/players/bond/bond.mdl"
#define TASK_MAX_RUNTIME 8.0f

// GE AI Global Vars
int g_GEAINextScheduleID	= (int) CNPC_GEBase::NEXT_SCHEDULE;
int g_GEAINextConditionID	= (int) CNPC_GEBase::NEXT_CONDITION;
int g_GEAINextTaskID		= (int) CNPC_GEBase::NEXT_TASK;

// NPC Level Probabilities
static const float NPCLevelProbs[][4] = 
{
	{ 1,  0,  0,  0},	// 0 
	{.7, .3,  0,  0},	// 1 - LEVEL_EASY
	{.5, .5,  0,  0},	// 2
	{.3, .7,  0,  0},	// 3 - LEVEL_MEDIUM
	{.1, .9,  0,  0},	// 4 
	{.2, .3, .5,  0},	// 5 
	{ 0, .3, .7,  0},	// 6 - LEVEL_HARD
	{ 0, .1, .6, .3},	// 7 
	{ 0,  0, .4, .6},	// 8 
	{ 0,  0, .2, .8},	// 9 - LEVEL_UBER
};

// -------------------------------------------------------------------------------- //
// NPC Ragdoll entities.
// -------------------------------------------------------------------------------- //

class CGENPCRagdoll : public CBaseAnimatingOverlay
{
public:
	DECLARE_CLASS( CGENPCRagdoll, CBaseAnimatingOverlay );
	DECLARE_SERVERCLASS();

	// Transmit ragdolls to everyone.
	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

public:
	// In case the client has the NPC entity, we transmit the NPC's index.
	// In case the client doesn't have it, we transmit the NPC's model index, origin, and angles
	// so they can create a ragdoll in the right place.
	CNetworkHandle( CBaseEntity, m_hNPC );	// networked entity handle 
	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
};

LINK_ENTITY_TO_CLASS( ge_npc_ragdoll, CGENPCRagdoll );

IMPLEMENT_SERVERCLASS_ST_NOBASE( CGENPCRagdoll, DT_GENPCRagdoll )
	SendPropVector( SENDINFO(m_vecRagdollOrigin), -1,  SPROP_COORD ),
	SendPropEHandle( SENDINFO( m_hNPC ) ),
	SendPropModelIndex( SENDINFO( m_nModelIndex ) ),
	SendPropInt( SENDINFO(m_nForceBone), 8, 0 ),
	SendPropVector( SENDINFO(m_vecForce), -1, SPROP_NOSCALE ),
	SendPropVector( SENDINFO( m_vecRagdollVelocity ) )
END_SEND_TABLE()

void CNPC_GEBase::UpdateOnRemove( void )
{
	if ( m_hRagdoll )
	{
		UTIL_RemoveImmediate( m_hRagdoll );
		m_hRagdoll = NULL;
	}

	BaseClass::UpdateOnRemove();
}

bool CNPC_GEBase::CanBecomeRagdoll( void )
{
	// Block default behavior
	return false;
}

bool CNPC_GEBase::BecomeRagdollOnClient( const Vector &force )
{
	// Block default behavior
	return false;
}

void CNPC_GEBase::CreateRagdoll( const Vector &force )
{
	if ( m_hRagdoll )
	{
		UTIL_RemoveImmediate( m_hRagdoll );
		m_hRagdoll = NULL;
	}

	// Clamp the force vector
	Vector vecClampedForce;
	ClampRagdollForce( force, &vecClampedForce );
	m_vecForce = vecClampedForce;

	// If we already have a ragdoll, don't make another one.
	CGENPCRagdoll *pRagdoll = dynamic_cast< CGENPCRagdoll* >( m_hRagdoll.Get() );

	if ( !pRagdoll )
	{
		// create a new one
		pRagdoll = dynamic_cast< CGENPCRagdoll* >( CreateEntityByName( "ge_npc_ragdoll" ) );
	}

	if ( pRagdoll )
	{
		pRagdoll->m_hNPC = this;
		pRagdoll->m_vecRagdollOrigin = GetAbsOrigin();
		pRagdoll->m_vecRagdollVelocity = GetSmoothedVelocity();
		pRagdoll->m_nModelIndex = m_nModelIndex;
		pRagdoll->m_nForceBone = m_nForceBone;
		pRagdoll->m_vecForce = force;
		pRagdoll->SetAbsOrigin( GetAbsOrigin() );
	}

	// ragdolls will be removed on round restart automatically
	m_hRagdoll = pRagdoll;
}

// -------------------------------------------------------------------------------- //
// Generic NPC definition
// -------------------------------------------------------------------------------- //

LINK_ENTITY_TO_CLASS( npc_gebase, CNPC_GEBase );

BEGIN_DATADESC( CNPC_GEBase )
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CNPC_GEBase, DT_NPC_GEBase )
	SendPropEHandle( SENDINFO( m_hRagdoll) ),
	SendPropEHandle( SENDINFO( m_hBotPlayer ) ),
	SendPropBool( SENDINFO( m_bSpawnInterpCounter ) ),
END_SEND_TABLE()

CNPC_GEBase::CNPC_GEBase()
{
	m_pPyNPC = NULL;
	m_szIdent[0] = 0;
	m_bTraceCalled = false;
	m_bSpawnInterpCounter = false;

	// Default difficulty settings
	m_fSpreadMod = 1.0f;
	m_fShotBias = 1.0f;
	m_fHeadShotProb = 0.3f;
	m_fBurstMod = 1.0f;
	m_fFocusTime = 0.8f;
	m_fReactionDelay = 0.05f;
}

CNPC_GEBase::~CNPC_GEBase()
{
	RemovePyInterface();
}

void CNPC_GEBase::Precache( void )
{
	const char* model = GetNPCModel();
	if ( model )
		PrecacheModel( model );

	BaseClass::Precache();
}

bool CNPC_GEBase::InitPyInterface( const char *ident )
{
	if ( !GEAi() )
		return false;

	// Remove the old brain, if exists
	RemovePyInterface();

	// This is where we create our brain in Python
	// We seperate the brain from the AI here because it allows us to reload
	// the AI dynamically without respawning the entire NPC!
	m_pPyNPC = GEAi()->CreateNPC( ident, this );
	if ( !m_pPyNPC )
		return false;

	// Set the difficulty
	SetDifficulty( ge_bot_difficulty.GetInt() );

	Q_strncpy( m_szIdent, m_pPyNPC->GetIdent(), 64 );
	return true;
}

void CNPC_GEBase::RemovePyInterface( void )
{
	// NOTE: Our try/catch statements will prevent crashes
	if ( GEAi() )
		GEAi()->RemoveNPC( m_pPyNPC );

	m_pPyNPC = NULL;
}

void CNPC_GEBase::InitBotInterface( CGEBotPlayer *bot )
{
	// Associate us with a player proxy
	m_hBotPlayer = bot;
}

void CNPC_GEBase::SetDifficulty( int difficulty )
{
	difficulty = clamp( difficulty, 0, 9 );
	const float *probs = NPCLevelProbs[ difficulty ];
	int levels[] = { LEVEL_AGENT, LEVEL_SECRET_AGENT, LEVEL_00_AGENT, LEVEL_007 };

	m_iLevel = GERandomWeighted<int, float>( levels, probs, 4 );

	if ( m_hBotPlayer.Get() )
		Msg( "%s set to level %s!\n", m_hBotPlayer->GetCleanPlayerName(), TranslateLevel( m_iLevel ) );
	else
		DevMsg( "NPC set to level %s!\n", TranslateLevel( m_iLevel ) );

	m_fShotBias		 = RemapValClamped( m_iLevel, LEVEL_AGENT, LEVEL_007, 1.2f, 0.3f );
	m_fSpreadMod	 = RemapValClamped( m_iLevel, LEVEL_AGENT, LEVEL_007, 1.1f, 0.2f );
	m_fHeadShotProb  = RemapValClamped( m_iLevel, LEVEL_AGENT, LEVEL_007, 0.3f,	0.8f );
	m_fFocusTime	 = RemapValClamped( m_iLevel, LEVEL_AGENT, LEVEL_007, 1.5f, 0.1f );
	m_fReactionDelay = RemapValClamped( m_iLevel, LEVEL_AGENT, LEVEL_007, 0.1f, 0.01f );

	try {
		m_pPyNPC->OnSetDifficulty( m_iLevel );
	} catch (...) { }
}

const char *CNPC_GEBase::TranslateLevel( int level )
{
	switch (level)
	{
	case LEVEL_AGENT:			return "Agent";
	case LEVEL_SECRET_AGENT:	return "Secret Agent";
	case LEVEL_00_AGENT:		return "00 Agent";
	case LEVEL_007:				return "007";
	// Catch invalid entries
	default:					return "Invalid Level";
	}
}

void CNPC_GEBase::SendDebugCommand( const char *cmd )
{
	try {
		// Simple python link to send a debug command for output or setting variables
		m_pPyNPC->OnDebugCommand( cmd );
	} catch(...) { }
}

void CNPC_GEBase::Spawn( void )
{
	// Don't call base class spawning (nothing there anyway)
	Precache();

	SetRenderMode( kRenderNormal );
	m_nRenderFX = kRenderFxNone;

	m_bSpawnInterpCounter = !m_bSpawnInterpCounter;

	if ( !m_hBotPlayer.Get() )
		SetModel( GetNPCModel() );

	// Force our hull size setting
	SetHullType( HULL_HUMAN );
	SetHullSizeNormal( true );
	
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );

	RemoveSolidFlags( FSOLID_NOT_SOLID );
	RemoveEffects( EF_NODRAW | EF_NOINTERP );

	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );

	m_iHealth			= MAX_HEALTH;
	m_flSumDamage		= 0;
	m_NPCState			= NPC_STATE_IDLE;
	m_lifeState			= LIFE_ALIVE;
	m_flFieldOfView		= 0.3;
	m_bDidDeathCleanup	= false;

	// We want to attack bullseyes!
	AddClassRelationship( CLASS_BULLSEYE, D_HT, 1 );

	// Totally clear our memory
	delete m_pEnemies;
	m_pEnemies = new CAI_Enemies;

	try {
		m_pPyNPC->OnSpawn();
	} catch (...) {
		DevWarning( "[GEAi] NPC spawned without Python being initialized!\n" );
	}

	NPCInit();
	
	// Set these after so that we can see past the normal range
	m_flDistTooFar	= 1e9f;
	SetDistLook( 6000.0 );
}

Class_T CNPC_GEBase::Classify( void )
{
	try {
		return (Class_T) m_pPyNPC->Classify();
	} catch ( ... ) {
		return CLASS_NONE;
	}
}

const char* CNPC_GEBase::GetNPCModel()
{
	try {
		const char *model = m_pPyNPC->GetNPCModel();
		return model ? model : DEFAULT_MODEL;
	} catch ( ... ) {
		return DEFAULT_MODEL;
	}
}

int CNPC_GEBase::GetSoundInterests( void )
{
	return SOUND_WORLD | SOUND_COMBAT | SOUND_DANGER | SOUND_PLAYER | SOUND_PLAYER_VEHICLE | SOUND_BULLET_IMPACT;
}

void CNPC_GEBase::NPCThink( void )
{
	if ( IsAlive() )
	{
		BaseClass::NPCThink();

		// Valve decided they needed to do this for player.cpp, and it catches holes in player_lagcompensation.cpp
		// so we're stuck doing it here too.  If you fix the lag compensation resets then you can remove this.
		// but right now we need it to stop the bots from literally going huge.
		if (GetFlags() & FL_DUCKING)
		{
			SetCollisionBounds(VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX);
		}
		else
		{
			SetCollisionBounds(VEC_HULL_MIN, VEC_HULL_MAX);
		}
	}
	else
		SetNextThink( gpGlobals->curtime + 0.1f );

	// Ensure our bot player runs prethink/think/postthink cycles
	if ( m_hBotPlayer.Get() )
	{
		// Get's the player proxy to "think"
		m_hBotPlayer->RunNullCommand();
		
		// Force us to gather conditions outside of PVS
		if ( IsAlive() )
			ForceGatherConditions();
	}
}

void CNPC_GEBase::GatherConditions( void )
{
	BaseClass::GatherConditions();

	// Give Python a shot (call after base to let python unset conditions)
	try {
		m_pPyNPC->GatherConditions();
	} catch ( ... ) {	}
}

void CNPC_GEBase::OnLooked( int dist )
{
	try {
		m_pPyNPC->OnLooked( dist );
	} catch ( ... ) {	}

	BaseClass::OnLooked( dist );
}

void CNPC_GEBase::OnListened( void )
{
	try {
		m_pPyNPC->OnListened();
	} catch ( ... ) {	}

	BaseClass::OnListened();
}

int	CNPC_GEBase::SelectSchedule()
{
	try {
		int schedule = m_pPyNPC->SelectSchedule();
		if ( schedule != SCHED_NONE )
			return schedule;
	} catch ( ... ) {	}

	return BaseClass::SelectSchedule();
}

int	CNPC_GEBase::TranslateSchedule( int scheduleType )
{
	try {
		int schedule = m_pPyNPC->TranslateSchedule( scheduleType );
		if ( schedule != SCHED_NONE )
			return schedule;
	} catch ( ... ) { }

	return BaseClass::TranslateSchedule( scheduleType );
}

bool CNPC_GEBase::IsScheduleValid()
{
	bool ret = BaseClass::IsScheduleValid();
	if ( ret && GetCurSchedule() && !HasCondition( COND_NPC_FREEZE ) )
	{
		try {
			// Note we negate the answer!
			ret = !m_pPyNPC->ShouldInterruptSchedule( GetCurSchedule()->GetId() );
		} catch ( ... ) { }
	}
	return ret;
}

NPC_STATE CNPC_GEBase::SelectIdealState( void )
{
	try {
		NPC_STATE state = m_pPyNPC->SelectIdealState();
		if ( state != NPC_STATE_INVALID )
			return state;
	} catch ( ... ) {	}

	return BaseClass::SelectIdealState();
}

void CNPC_GEBase::OnStartSchedule( int scheduleType )
{
	if ( ge_ai_debug.GetInt() >= 1 && IsSelected() )
		Msg( "[GEAi] Started Schedule %s\n", GetScheduleName(scheduleType) );

	BaseClass::OnStartSchedule( scheduleType );
}

void CNPC_GEBase::StartTask( const Task_t *pTask )
{
	if ( ge_ai_debug.GetInt() >= 2 && IsSelected() )
		Msg( "\tStarted Task: %s\n", GetTaskName(pTask->iTask) );

	// Attempt to call into python
	bool ret = false;
	try {
		ret = m_pPyNPC->CheckStartTask( pTask );
	} catch (...) { }

	switch( pTask->iTask )
	{
	// Support GES Reload anims
	case TASK_RELOAD:
		RestartGesture( TranslateActivity(ACT_GESTURE_RELOAD) );
		ret = true;
		break;

	// Support GES anims
	case TASK_MELEE_ATTACK1:
		// This function handles everything for us
		StartTaskRangeAttack1( pTask );
		ret = true;
		break;

	case TASK_GET_PATH_TO_TARGET:
	case TASK_GET_PATH_TO_TARGET_WEAPON:
		if ( m_vTargetPos != vec3_origin )
		{
			if ( !GetNavigator()->SetGoal( m_vTargetPos ) )
				TaskFail(FAIL_NO_ROUTE);
			else
				GetNavigator()->SetGoalTolerance( 48 );

			m_vTargetPos = vec3_origin;
			TaskComplete();
			ret = true;
		}
		break;
	
	// Force clearing a latent goal, useful between calls to GET_PATH_TO_RANDOM_NODE
	case TASK_CLEAR_GOAL:
		TaskComplete();
		GetNavigator()->ClearGoal();
		ret = true;
		break;
	};

	if ( !ret )
		BaseClass::StartTask( pTask );
}

void CNPC_GEBase::RunTask( const Task_t *pTask )
{
	/*
	// If we having a moving task, are not moving, and overran our timeout, cancel the task
	if ( IsCurTaskContinuousMove() 
		&& !HasCondition( COND_NPC_FREEZE )
		&& GetMotor()->GetCurSpeed() < 3.0f 
		&& (gpGlobals->curtime - GetTimeTaskStarted()) > TASK_MAX_RUNTIME )
	{
		DevMsg( "GES NPC: Task %s forcefully ended\n", GetTaskName( pTask->iTask) );
		TaskComplete();
		return;
	}
	*/

	bool ret = false;
	try {
		ret = m_pPyNPC->CheckRunTask( pTask );
	} catch (...) { }

	switch ( pTask->iTask )
	{
	case TASK_WEAPON_RUN_PATH:
	case TASK_ITEM_RUN_PATH:
		{
			ret = true;

			CBaseEntity *pTarget = GetTarget();
			if ( pTarget )
			{
				CBaseEntity *pOwner = pTarget->GetOwnerEntity();
				if ( pOwner && pOwner != this && (pOwner->IsPlayer() || pOwner->IsNPC()) )
					TaskComplete();
				else if ( GetNavigator()->GetGoalType() == GOALTYPE_NONE )
					TaskComplete();
				else if ( !GetNavigator()->IsGoalActive() )
					TaskComplete();
				else if ( (GetLocalOrigin() - GetNavigator()->GetGoalPos()).Length() <= pTask->flTaskData )
					TaskComplete();
			}
			else
			{
				if ( GetNavigator()->GetGoalType() != GOALTYPE_LOCATION )
					TaskComplete();
			}
		}
		break;

	case TASK_WALK_PATH_WITHIN_DIST:
	case TASK_RUN_PATH_WITHIN_DIST:
		// Make sure our goal is still valid otherwise we get stuck in the task!
		if ( GetNavigator()->GetGoalType() == GOALTYPE_NONE || !GetNavigator()->IsGoalActive() )
		{
			ret = true;
			TaskComplete();
		}
		break;

	case TASK_RELOAD:
		RunTaskReload( pTask );
		ret = true;
		break;

	case TASK_MELEE_ATTACK1:
		// This function handles everything for us
		RunTaskRangeAttack1( pTask );
		ret = true;
		break;
	}

	if ( !ret )
		BaseClass::RunTask( pTask );
}

void CNPC_GEBase::TaskFail( AI_TaskFailureCode_t code )
{
	if ( ge_ai_debug.GetInt() >= 1 && IsSelected() )
		Warning( "[GEAi] Task Failed %s\n\n", TaskFailureToString(code) );

	BaseClass::TaskFail( code );
}

void CNPC_GEBase::SetTargetPos( const Vector &target )
{
	m_vTargetPos = target;
}

bool CNPC_GEBase::IsValidEnemy( CBaseEntity *pEnemy )
{
	try {
		// If python returns false this is an early out
		if ( !m_pPyNPC->IsValidEnemy( pEnemy ) )
			return false;
	} catch ( ... ) {	}

	// The proxy player can never be an enemy...
	if ( (pEnemy->GetFlags() & FL_FAKECLIENT) != 0 )
		return false;

	return BaseClass::IsValidEnemy( pEnemy );
}

void CNPC_GEBase::FireBullets( const FireBulletsInfo_t &info )
{
	FireBulletsInfo_t modinfo = info;

	// Register a shot fired with our player (the proxy NEVER directly fires a bullet)
	if ( m_hBotPlayer.Get() )
	{
		modinfo.m_pAttacker = m_hBotPlayer;
		m_hBotPlayer->FireBullets( info );
	}

	BaseClass::FireBullets( modinfo );
}

bool CNPC_GEBase::IsNavigationUrgent()
{
	// Hell yes
	return true;
}

bool CNPC_GEBase::ShouldFailNav( bool bMovementFailed )
{
	// We handle nav failure in a better, more logical, manner than Valve
	return false;
}

bool CNPC_GEBase::ShouldBruteForceFailedNav()
{
	static const int min_retries = 1;
	static const int max_retries = 3;

	int failures = GetNavigator()->GetNavFailCounter();
	if ( failures >= max_retries )
	{
		Vector pos = GetAbsOrigin();
		DevMsg( "NPC Brute Forcing Nav at %.1f, %.1f, %.1f\n", pos.x, pos.y, pos.z );
		return true;
	}
	else if ( failures >= min_retries )
	{
		GetNavigator()->RefindPathToGoal();
	}

	return false;
}

bool CNPC_GEBase::ShouldLookForBetterWeapon()
{
	// Searching for weapons is entirely handled in Python
	return false;
}

WeaponProficiency_t CNPC_GEBase::CalcWeaponProficiency( CBaseCombatWeapon *pWeapon )
{
	// We don't want to modify our spread, GE:S has it's own dynamic spread to worry about!
	return WEAPON_PROFICIENCY_PERFECT;
}

bool CNPC_GEBase::Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex )
{
	bool ret = BaseClass::Weapon_Switch( pWeapon, viewmodelindex );
	if ( ret )
	{
		// Show our fancy draw animations
		Activity draw_sequence = TranslateActivity(ACT_GES_DRAW);
		AddGesture( draw_sequence );

		ResetActivity();
		ResetIdealActivity( ACT_IDLE );

		// Do an update if we switched
		OnUpdateShotRegulator();

		GetShotRegulator()->FireNoEarlierThan( gpGlobals->curtime + SequenceDuration( draw_sequence ) );
	}

	return ret;
}

void CNPC_GEBase::Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget /* = NULL */, const Vector *pVelocity /* = NULL */ )
{
	bool bWasActiveWeapon = false;
	if ( pWeapon == GetActiveWeapon() )
		bWasActiveWeapon = true;

	BaseClass::Weapon_Drop( pWeapon, pvecTarget, pVelocity );

	if ( bWasActiveWeapon )
		SwitchToNextBestWeapon( NULL );
}

Vector CNPC_GEBase::Weapon_ShootPosition( void )
{
	return EyePosition();
}

Activity CNPC_GEBase::Weapon_TranslateActivity( Activity baseAct, bool *pRequired )
{
	if ( m_hBotPlayer.Get() )
	{
		switch ( baseAct )
		{
		case ACT_GESTURE_RANGE_ATTACK1:
			baseAct = ACT_MP_ATTACK_STAND_PRIMARYFIRE;
			break;

		case ACT_GESTURE_RELOAD:
			baseAct = ACT_MP_RELOAD_STAND;
			break;

		case ACT_IDLE:
		case ACT_IDLE_ANGRY:
		case ACT_IDLE_AGITATED:
			baseAct = ACT_MP_STAND_IDLE;
			break;

		case ACT_WALK:
		case ACT_WALK_AIM:
			baseAct = ACT_MP_WALK;
			break;

		case ACT_RUN:
		case ACT_RUN_AIM:
			baseAct = ACT_MP_RUN;
			break;

		case ACT_ARM:
			baseAct = ACT_GES_DRAW;
			break;
		}
	}

	return BaseClass::Weapon_TranslateActivity( baseAct, pRequired );
}

void CNPC_GEBase::HandleAnimEvent( animevent_t *pEvent )
{
	switch( pEvent->event )
	{
	case EVENT_WEAPON_RELOAD:
	case EVENT_WEAPON_RELOAD_FILL_CLIP:
		{
			CBaseCombatWeapon *pWeap = GetActiveWeapon();
			if ( pWeap )
			{
				if ( pWeap->Reload() )
				{
					// Actually do the reload
					pWeap->FinishReload();  					
				}
				else if ( pWeap->GetPrimaryAmmoCount() <= 0 )
				{
					SetCondition(COND_NO_PRIMARY_AMMO);
					break;
				}
				
				ClearCondition(COND_LOW_PRIMARY_AMMO);
				ClearCondition(COND_NO_PRIMARY_AMMO);
				ClearCondition(COND_NO_SECONDARY_AMMO);	
			}
			break;
		}

	case AE_NPC_BODYDROP_HEAVY:
		break;

	default:
		BaseClass::HandleAnimEvent( pEvent );
		break;
	}
}

float CNPC_GEBase::GetSequenceGroundSpeed( CStudioHdr *pStudioHdr, int iSequence )
{
	if ( m_hBotPlayer.Get() )
		return GE_NORM_SPEED * GEMPRules()->GetSpeedMultiplier( m_hBotPlayer );

	return BaseClass::GetSequenceGroundSpeed( pStudioHdr, iSequence );
}

int CNPC_GEBase::SelectFlinchSchedule( void )
{
	if ( m_hBotPlayer.Get() )
		return SCHED_NONE;

	return BaseClass::SelectFlinchSchedule();
}

void CNPC_GEBase::OnUpdateShotRegulator( void )
{
	CGEWeapon *pWeapon = ToGEWeapon( GetActiveWeapon() );
	if ( !pWeapon )
		return;

	float minRest, maxRest;
	float minRate, maxRate;
	int minBurst = 1, maxBurst = 1;

	// Hack for now to achieve appropriate fire rate?? (maybe change to GetNPCFireRate in the future?)
	if ( pWeapon->GetWeaponID() == WEAPON_COUGAR_MAGNUM )
	{
		minRate = maxRate = pWeapon->GetFireRate() * 1.2f;
		minRest = maxRest = pWeapon->GetFireRate() * 1.2f;
	}
	else if ( pWeapon->GetWeaponID() == WEAPON_GOLDENGUN )
	{
		minRate = maxRate = pWeapon->GetFireRate() * 2.0f;
		minRest = maxRest = pWeapon->GetFireRate() * 2.0f;
	}
	else if ( pWeapon->IsExplosiveWeapon() )
	{
		minRate = maxRate = pWeapon->GetFireRate() * 1.2f;
		minRest = maxRest = pWeapon->GetFireRate() * 1.5f;
	}
	else
	{
		minRate = pWeapon->GetClickFireRate();
		maxRate = pWeapon->GetFireRate();
		minRest = pWeapon->GetMinRestTime();
		maxRest = pWeapon->GetMaxRestTime();
	}

	if ( pWeapon->UsesClipsForAmmo1() && pWeapon->m_iClip1 > 1 )
	{
		minBurst = MAX( 1, pWeapon->m_iClip1 / 3 );
		maxBurst = MIN( minBurst + GERandom<int>(pWeapon->GetMaxClip1() - minBurst), pWeapon->m_iClip1 );
	}

	GetShotRegulator()->SetBurstShotCountRange( minBurst, maxBurst );
	GetShotRegulator()->SetBurstInterval( minRate, maxRate );
	GetShotRegulator()->SetRestInterval( minRest, maxRest );
}

// Check if another combat character has a weapon displayed
bool CNPC_GEBase::HasWeapon( CBaseEntity *pEnt )
{
	if (pEnt)
	{
		CBaseCombatCharacter *pCBCC = dynamic_cast<CBaseCombatCharacter*>(pEnt);
		if ( pCBCC )
		{
			CGEWeapon *pCGEW = dynamic_cast<CGEWeapon*>(pCBCC->GetActiveWeapon());

			if ( pCGEW && pCGEW->GetWeaponID() != WEAPON_SLAPPERS )
				return true;
		}
	}
	return false;
}

bool CNPC_GEBase::BumpWeapon( CGEWeapon *pWeapon )
{
	bool canHaveWeapon = true;

	// Bots get extra scrutiny due to gameplay restrictions
	CGEBotPlayer *pBot = m_hBotPlayer.Get();
	if ( pBot )
	{
		if ( !GEGameplay()->GetScenario()->CanPlayerHaveItem(pBot, pWeapon) )
			return false;

		if ( !g_pGameRules->CanHavePlayerItem( pBot, pWeapon ) )
			return false;
	}

	// Can I have this weapon type?
	if ( !IsAllowedToPickupWeapons() || !pWeapon->CanEquip(this) )
		canHaveWeapon = false;

	if ( pWeapon->GetOwner() || !Weapon_CanUse( pWeapon ) )
		canHaveWeapon = false;

	// Don't let the NPC fetch weapons through walls (use MASK_SOLID so that you can't pickup through windows)
	if( !pWeapon->FVisible( this, MASK_SOLID ) && !(GetFlags() & FL_NOTARGET) )
		canHaveWeapon = false;

	if ( canHaveWeapon && !!Weapon_OwnsThisType( pWeapon->GetClassname(), pWeapon->GetSubType() ) ) 
	{
		//If we have room for the ammo, then "take" the weapon too.
		if ( Weapon_EquipAmmoOnly( pWeapon ) )
			UTIL_Remove( pWeapon );

		// We don't want to pickup this weapon
		canHaveWeapon = false;
	}

	if ( canHaveWeapon )
	{
		// Give us the clip ammo
		GiveAmmo( pWeapon->GetDefaultClip1(), pWeapon->GetPrimaryAmmoType() ); 
		Weapon_Equip( pWeapon );
		pWeapon->SetWeaponVisible( false );

		ClearCondition(COND_LOW_WEAPON_WEIGHT);

		if (pBot)
		{
			try {
				m_pPyNPC->OnPickupItem();
			}
			catch (...) {
				DevWarning("[GEAi] OnPickupItem exception!\n");
			}
		}
	}
	else if ( pBot && (GERules()->ShouldForcePickup(pBot, pWeapon) || pWeapon->GetWeaponID() == WEAPON_MOONRAKER) )
	{
		// If we didn't pick it up and the game rules say we should have anyway remove it from the world
		pWeapon->SetOwner( NULL );
		pWeapon->SetOwnerEntity( NULL );
		UTIL_Remove(pWeapon);
	}

	return canHaveWeapon;
}

CBaseEntity	*CNPC_GEBase::GiveNamedItem( const char *pszName, int iSubType )
{
	// If I already own this type don't create one
	if ( Weapon_OwnsThisType(pszName, iSubType) )
		return NULL;

	// Make sure we have a valid factory for this entity (eg don't spawn non existent weapons!
	if ( !CanCreateEntityClass(pszName) )
	{
		Warning( "Tried to create unknown entity %s\n", pszName );
		return NULL;
	}

	CBaseEntity *pent = CreateEntityByName(pszName);
	if ( pent == NULL )
	{
		Msg( "NULL Ent in GiveNamedItem!\n" );
		return NULL;
	}

	pent->SetLocalOrigin( GetLocalOrigin() );
	pent->AddSpawnFlags( SF_NORESPAWN );

	DispatchSpawn( pent );

	CBaseCombatWeapon *pWeapon = dynamic_cast<CBaseCombatWeapon*>( (CBaseEntity*)pent );
	if ( pWeapon )
	{
		Weapon_Equip( pWeapon );
		pWeapon->SetWeaponVisible( false );

		// Handle this case
		OnGivenWeapon( pWeapon );
	}

	return pent;
}

const char* CNPC_GEBase::GetScheduleName( int id )
{
	int global = GetClassScheduleIdSpace()->ScheduleLocalToGlobal( id );
	return GetSchedulingSymbols()->ScheduleIdToSymbol( global );
}

const char* CNPC_GEBase::GetTaskName( int id )
{
	int global = GetClassScheduleIdSpace()->TaskLocalToGlobal( id );
	return GetSchedulingSymbols()->TaskIdToSymbol( global );
}

bool CNPC_GEBase::IsHeavyDamage( const CTakeDamageInfo &info )
{
	return ( info.GetDamage() > (m_iMaxHealth / 3.0f) );
}

void CNPC_GEBase::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	// Doesn't hurt to force this setting, in case we use the bot player
	SetLastHitGroup( ptr->hitgroup );
	m_nForceBone = ptr->physicsbone;		// save this bone for physics forces

	if ( m_hBotPlayer.Get() )
		m_hBotPlayer->TraceAttack( inputInfo, vecDir, ptr, pAccumulator );
	else
		// Skip over sneak attack bullshit in CBaseHumanoid
		CAI_BaseNPC::TraceAttack( inputInfo, vecDir, ptr, pAccumulator );
}

int CNPC_GEBase::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( m_hBotPlayer.Get() )
		return m_hBotPlayer->OnTakeDamage( info );
	else
		return BaseClass::OnTakeDamage( info );
}

int CNPC_GEBase::OnTakeArmorDamage( const CTakeDamageInfo &info )
{
	//
	// NOTE: This is just REACTION effects to damage. It does not remove any health!
	//
	Forget( bits_MEMORY_INCOVER );

	if ( GetSleepState() == AISS_WAITING_FOR_THREAT )
		Wake();

	if( (info.GetDamageType() & DMG_CRUSH) && !(info.GetDamageType() & DMG_PHYSGUN) )
	{
		SetCondition( COND_PHYSICS_DAMAGE );
	}

	// react to the damage (get mad)
	if ( ( (GetFlags() & FL_NPC) == 0 ) || !info.GetAttacker() )
		return 1;

	// If the attacker was an NPC or client update my position memory
	if ( info.GetAttacker()->GetFlags() & (FL_NPC | FL_CLIENT) )
	{
		// ------------------------------------------------------------------
		//				DO NOT CHANGE THIS CODE W/O CONSULTING
		// Only update information about my attacker I don't see my attacker
		// ------------------------------------------------------------------
		if ( !FInViewCone( info.GetAttacker() ) || !FVisible( info.GetAttacker() ) )
		{
			// -------------------------------------------------------------
			//  If I have an inflictor (enemy / grenade) update memory with
			//  position of inflictor, otherwise update with an position
			//  estimate for where the attack came from
			// ------------------------------------------------------
			Vector vAttackPos;
			if (info.GetInflictor())
			{
				vAttackPos = info.GetInflictor()->GetAbsOrigin();
			}
			else
			{
				vAttackPos = (GetAbsOrigin() + ( g_vecAttackDir * 64 ));
			}


			// ----------------------------------------------------------------
			//  If I already have an enemy, assume that the attack
			//  came from the enemy and update my enemy's position
			//  unless I already know about the attacker or I can see my enemy
			// ----------------------------------------------------------------
			if ( GetEnemy() != NULL							&&
				!GetEnemies()->HasMemory( info.GetAttacker() )			&&
				!HasCondition(COND_SEE_ENEMY)	)
			{
				UpdateEnemyMemory(GetEnemy(), vAttackPos, GetEnemy());
			}
			// ----------------------------------------------------------------
			//  If I already know about this enemy, update his position
			// ----------------------------------------------------------------
			else if (GetEnemies()->HasMemory( info.GetAttacker() ))
			{
				UpdateEnemyMemory(info.GetAttacker(), vAttackPos);
			}
			// -----------------------------------------------------------------
			//  Otherwise just note the position, but don't add enemy to my list
			// -----------------------------------------------------------------
			else
			{
				UpdateEnemyMemory(NULL, vAttackPos);
			}
		}

		// add pain to the conditions
		if ( IsLightDamage( info ) )
			SetCondition( COND_LIGHT_DAMAGE );

		if ( IsHeavyDamage( info ) )
			SetCondition( COND_HEAVY_DAMAGE );

		ForceGatherConditions();

		m_flLastDamageTime = gpGlobals->curtime;
		if ( info.GetAttacker() && info.GetAttacker()->IsPlayer() )
			m_flLastPlayerDamageTime = gpGlobals->curtime;
		GetEnemies()->OnTookDamageFrom( info.GetAttacker() );

		NotifyFriendsOfDamage( info.GetAttacker() );
	}

	// ---------------------------------------------------------------
	//  Insert a combat sound so that nearby NPCs know I've been hit
	// ---------------------------------------------------------------
	CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 1024, 0.5, this, SOUNDENT_CHANNEL_INJURY );

	return 1;
}

void CNPC_GEBase::Event_Killed( const CTakeDamageInfo &info )
{
	// Call into our base class
	BaseClass::Event_Killed(info);

	// If we are a bot, take special actions
	if ( m_hBotPlayer.Get() )
	{
		// Kill us
		m_lifeState = LIFE_DEAD;

		// Create our ragdoll
		CreateRagdoll( m_hBotPlayer->m_vecTotalBulletForce );

		// Reset our think function
		SetThink( &CAI_BaseNPC::CallNPCThink );
		SetNextThink( gpGlobals->curtime );

		ClearAllSchedules();
		// Freeze us in place
		SetCondition( COND_NPC_FREEZE );

		VPhysicsDestroyObject();

		// Hide us
		AddSolidFlags( FSOLID_NOT_SOLID );
		SetGroundEntity( NULL );
	}

	// If we define any special tokens, always drop them
	// We do it after the BaseClass stuff so we are consistent with KILL -> DROP
	if ( GEMPRules()->GetTokenManager()->GetNumTokenDef() )
	{
		for ( int i=0; i < MAX_WEAPONS; i++ )
		{
			CBaseCombatWeapon *pWeapon = GetWeapon(i);
			if ( !pWeapon )
				continue;

			if ( GEMPRules()->GetTokenManager()->IsValidToken( pWeapon->GetClassname() ) )
				Weapon_Drop( pWeapon );
		}
	}

	// Remove whatever is left
	RemoveAllWeapons();
	RemoveAllAmmo();
}

void CNPC_GEBase::Event_KilledOther( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
	BaseClass::Event_KilledOther( pVictim, info );

	// Force an update on our enemy
	if ( GetEnemy() == pVictim )
	{
		SetCondition( COND_ENEMY_DEAD );
		ClearCondition( COND_ENEMY_OCCLUDED );
		ClearCondition( COND_SEE_ENEMY );
	}

	if ( m_hBotPlayer.Get() )
		m_hBotPlayer->Event_KilledOther( pVictim, info );
}

bool CNPC_GEBase::IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos ) const
{
	// Disable jumping UP for NPC's, maintain drop downs though!
	const float MAX_JUMP_RISE		= 0;
	const float MAX_JUMP_DISTANCE	= 30.0f;
	const float MAX_JUMP_DROP		= 192.0f;

	return BaseClass::IsJumpLegal( startPos, apex, endPos, MAX_JUMP_RISE, MAX_JUMP_DROP, MAX_JUMP_DISTANCE );
}

bool CNPC_GEBase::OnObstructionPreSteer( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	CBaseEntity *pObsEnt = pMoveGoal->directTrace.pObstruction;
	CBreakable *pBreak = dynamic_cast<CBreakable*>( pObsEnt );
	if ( pObsEnt && pBreak != NULL )
	{
		if ( !pObsEnt->IsBulletProof() && pBreak->IsBreakable() && !pBreak->Explodable()
			&& pObsEnt->GetHealth() > 0 && pObsEnt->GetHealth() < 300 )
		{
			// 60 dmg every 0.4 seconds
			float delay = pObsEnt->GetHealth() / 140.0f;

			variant_t value;
			value.SetEntity( this );
			g_EventQueue.AddEvent( pObsEnt, "Break", value, delay, this, this );

			DelayMoveStart( delay + 0.2f );

			/*
			Vector aimpos = pMoveGoal->directTrace.vEndPosition;
			aimpos.z = EyePosition().z;

			CNPC_Bullseye *pTarget = (CNPC_Bullseye*) CreateCustomTarget( aimpos, 2.5f );
			if ( pTarget )
			{
				pTarget->SetPainPartner( pObsEnt );
				pTarget->SetHealth( pObsEnt->GetHealth() );

				DelayMoveStart( 0.5f );
			}
			*/
		}
	}
	
	return BaseClass::OnObstructionPreSteer( pMoveGoal, distClear, pResult );
}

bool CNPC_GEBase::OnObstructingDoor( AILocalMoveGoal_t *pMoveGoal, CBaseDoor *pDoor, float distClear, AIMoveResult_t *pResult )
{
	if ( pMoveGoal->maxDist < distClear )
		return false;

	if ( pDoor->m_toggle_state ==  TS_AT_BOTTOM || pDoor->m_toggle_state == TS_GOING_DOWN )
	{
		// If we are locked and closed / closing we lose!
		if ( pDoor->m_bLocked || pDoor->HasSpawnFlags(SF_DOOR_NONPCS) || !pDoor->HasSpawnFlags(SF_DOOR_PUSE) )
		{
			*pResult = AIMR_BLOCKED_ENTITY;
			return true;
		}
		
		// Otherwise attempt to open me
		pDoor->Use( this, this, USE_TOGGLE, 0 );
		
		// Calculate the amount of time I have to wait
		float moveDist = 0;
		if ( FClassnameIs(pDoor, "func_door_rotating") || FClassnameIs(pDoor, "func_ge_door_rotating") )
			moveDist = (pDoor->m_vecAngle2 - pDoor->m_vecAngle1).Length();
		else
			moveDist = pDoor->m_vecPosition1.DistTo( pDoor->m_vecPosition2 );
		
		float delay;

		if (FClassnameIs(pDoor, "func_ge_door_rotating") || FClassnameIs(pDoor, "func_ge_door"))
		{
			CGEDoor *pGEDoor = static_cast<CGEDoor*>(pDoor);
			Vector diffvector = pDoor->m_vecPosition2 - pDoor->m_vecPosition1;
			float botHeight = GetHullHeight() * 1.2; // Includes a bit of a fudge factor to account for integrator error and make sure they don't instantly go under doors.

			// The time it takes us to accelerate, decelerate, and the time we move at max speed.
			// can't use m_flacceltime for this as that's the desired acceltime, not the acceltime in practice.

			float fullAccelTime = sqrt((pGEDoor->m_flDeccelDist * 2) / pGEDoor->m_flAccelSpeed);
			float topspeeddist = (moveDist - pGEDoor->m_flDeccelDist * 2);

			// We might not want to wait for the entire opening sequence though, if the door goes up.
			if (botHeight < diffvector.z && abs(diffvector.x) + abs(diffvector.y) <= 6) // Only do this logic if the door is moving almost entirely upward and goes high enough for us to pass under.
			{
				if (botHeight < pGEDoor->m_flDeccelDist) // We can actually pass under it before it's even finished accelerating.
					delay = sqrt((GetHullHeight() * 3) / pGEDoor->m_flAccelSpeed);
				else if (botHeight < pGEDoor->m_flDeccelDist + topspeeddist) // We can pass under while it's moving at full speed
					delay = fullAccelTime + (botHeight - pGEDoor->m_flDeccelDist) / pDoor->m_flSpeed;
				else // While it's decelerating
					delay = fullAccelTime + topspeeddist / pDoor->m_flSpeed + sqrt(((GetHullHeight() - pGEDoor->m_flDeccelDist - topspeeddist) * 3) / pGEDoor->m_flAccelSpeed);
			}
			else // Just wait for the entire thing to open, since it's not going up high enough for us to pass under.
				delay = fullAccelTime * 2 + topspeeddist / pDoor->m_flSpeed;
		}
		else
			delay = moveDist / pDoor->m_flSpeed;

		DelayMoveStart( delay );
		
		pMoveGoal->maxDist = distClear;

		*pResult = AIMR_OK;
		return true;
	}

	return false;
}

bool CNPC_GEBase::OnUpcomingPropDoor( AILocalMoveGoal_t *pMoveGoal, CBasePropDoor *pDoor, float distClear, AIMoveResult_t *pResult )
{
	if ( (pMoveGoal->flags & AILMG_TARGET_IS_GOAL) && pMoveGoal->maxDist < distClear )
		return false;

	if ( pMoveGoal->maxDist + GetHullWidth() * .25 < distClear )
		return false;

	if ( pDoor == m_hOpeningDoor )
	{
		if ( pDoor->IsNPCOpening( this ) )
		{
			// We're in the process of opening the door, don't be blocked by it.
			pMoveGoal->maxDist = distClear;
			*pResult = AIMR_OK;
			return true;
		}
		m_hOpeningDoor = NULL;
	}

	if ((CapabilitiesGet() & bits_CAP_DOORS_GROUP) && !pDoor->IsDoorLocked() && (pDoor->IsDoorClosed() || pDoor->IsDoorClosing()))
	{
		OpenPropDoorNow( pDoor );
		m_hOpeningDoor = pDoor;
		return true;
	}

	return false;
}

void CNPC_GEBase::RunTaskRangeAttack1( const Task_t *pTask )
{
	if ( ( CapabilitiesGet() & bits_CAP_USE_SHOT_REGULATOR ) == 0 )
	{
		BaseClass::RunTask( pTask );
		return;
	}

	AutoMovement( );

	Vector vecEnemyLKP = GetEnemyLKP();

	// If our enemy was killed, but I'm not done animating, the last known position comes
	// back as the origin and makes the me face the world origin if my attack schedule
	// doesn't break when my enemy dies. (sjb)
	if( vecEnemyLKP != vec3_origin )
	{
		if ( ( pTask->iTask == TASK_RANGE_ATTACK1 || pTask->iTask == TASK_RELOAD ) && 
			 ( CapabilitiesGet() & bits_CAP_AIM_GUN ) && 
			 FInAimCone( vecEnemyLKP ) )
		{
			// Arms will aim, so leave body yaw as is
			GetMotor()->SetIdealYawAndUpdate( GetMotor()->GetIdealYaw(), AI_KEEP_YAW_SPEED );
		}
		else
		{
			GetMotor()->SetIdealYawToTargetAndUpdate( vecEnemyLKP, AI_KEEP_YAW_SPEED );
		}
	}

	// If our enemy died we don't need to keep shooting at them
	if ( !GetEnemy() || !GetEnemy()->IsAlive() )
	{
		TaskComplete();
		return;
	}
		
	// Check to see if we can shoot
	if ( !GetShotRegulator()->IsInRestInterval() )
	{
		CGEWeapon *pWeapon = ToGEWeapon( GetActiveWeapon() );
		if ( pWeapon && pWeapon->HasShotsLeft() )
		{
			// Check if we have a burst left and shoot again if we do
			if ( GetShotRegulator()->ShouldShoot() )
			{
				OnRangeAttack1();
				RestartGesture( TranslateActivity( ACT_GESTURE_RANGE_ATTACK1 ) );
			}
			// We don't want to stop our task
			return;
		}
		else if ( pWeapon )
		{
			DevWarning( "NPC ran out of bullets in a burst with %s\n", pWeapon->GetClassname() );
			GetShotRegulator()->Reset( false );
		}
	}

	// If we got here it means we are really done
	TaskComplete();
}

void CNPC_GEBase::RunTaskReload( const Task_t *pTask )
{
	AutoMovement();
	Vector vecEnemyLKP = GetEnemyLKP();

	// If our enemy was killed, but I'm not done animating, the last known position comes
	// back as the origin and makes the me face the world origin if my attack schedule
	// doesn't break when my enemy dies. (sjb)
	if ( vecEnemyLKP != vec3_origin )
	{
		if ( ( pTask->iTask == TASK_RANGE_ATTACK1 || pTask->iTask == TASK_RELOAD ) && 
			( CapabilitiesGet() & bits_CAP_AIM_GUN ) && FInAimCone( vecEnemyLKP ) )
		{
			// Arms will aim, so leave body yaw as is
			GetMotor()->SetIdealYawAndUpdate( GetMotor()->GetIdealYaw(), AI_KEEP_YAW_SPEED );
		}
		else
		{
			GetMotor()->SetIdealYawToTargetAndUpdate( vecEnemyLKP, AI_KEEP_YAW_SPEED );
		}
	}

	if ( !IsPlayingGesture( TranslateActivity(ACT_GESTURE_RELOAD) ) )
	{	
		if ( GetShotRegulator() )
			GetShotRegulator()->Reset( false );

		TaskComplete();
	}
}

bool CNPC_GEBase::IsCurTaskContinuousMove()
{
	const Task_t* pTask = GetTask();

	// This bit of logic strikes me funny, but the case does exist. (sjb)
	if( !pTask )
		return true;

	switch( pTask->iTask )
	{
	case TASK_ITEM_RUN_PATH:
	case TASK_WEAPON_RUN_PATH:
	case TASK_MOVE_TO_GOAL_RANGE:
	case TASK_MOVE_TO_TARGET_RANGE:
	case TASK_MOVE_AWAY_PATH:
	case TASK_STRAFE_PATH:
		return true;
		break;
	}

	return BaseClass::IsCurTaskContinuousMove();
}

void BotDifficulty_Callback( IConVar *var, const char *pOldString, float flOldValue )
{
	ConVar *cvar = (ConVar*) var;

	FOR_EACH_BOTPLAYER( pBot )
		CNPC_GEBase *npc = pBot->GetNPC();
		if ( npc )
			npc->SetDifficulty( cvar->GetInt() );
	END_OF_PLAYER_LOOP()
}

#ifdef DEBUG
CON_COMMAND( ge_npc_debugammo, "Dumps the amount and type of ammo the named NPC holds" )
{
	if ( args.ArgC() < 2 )
	{
		Msg( "%s\n", ge_npc_debugammo_command.GetHelpText() );
		return;
	}

	CNPC_GEBase *pNPC = NULL;
	FOR_EACH_BOTPLAYER( pBot )
		if ( !Q_stricmp( pBot->GetCleanPlayerName(), args[1] ) )
		{
			pNPC = pBot->GetNPC();
			break;
		}
	END_OF_PLAYER_LOOP()

	if ( pNPC )
	{
		Msg( "%s's Ammo Stock\n", args[1] );

		for ( int i=0; i < GetAmmoDef()->m_nAmmoIndex; i++ )
		{
			int myCount = pNPC->GetAmmoCount( i );
			if ( myCount > 0 )
			{
				Ammo_t *pAmmo = GetAmmoDef()->GetAmmoOfIndex(i);
				Msg( "%s, %i/%i\n", pAmmo->pName, myCount, pAmmo->pMaxCarry );
			}
		}
		
		Msg( "------------------------------\n" );
	}
	else
	{
		Msg( "Could not find the named Bot!\n" );
	}
}

CON_COMMAND( ge_npc_create, "Creates an NPC for use in GE:S" )
{
	if ( args.ArgC() < 2 ) {
		Msg( "You must supply a NPC identity! Use ge_npc_listidents to see available identities.\n" );
		return;
	}

	MDLCACHE_CRITICAL_SECTION();

	bool allowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	// Try to create entity
	CNPC_GEBase *baseNPC = dynamic_cast< CNPC_GEBase * >( CreateEntityByName("npc_gebase") );
	if ( baseNPC )
	{
		// First things first, link in with python
		if ( !baseNPC->InitPyInterface( args[1] ) )
		{
			baseNPC->SUB_Remove();
			Warning( "GE Ai: Failed to create Python link, aborting spawn!\n" );
			CBaseEntity::SetAllowPrecache( allowPrecache );
			return;
		}

		baseNPC->Precache();

		if ( args.ArgC() == 3 )
		{
			baseNPC->SetName( AllocPooledString( args[2] ) );
		}

		DispatchSpawn(baseNPC);
		// Now attempt to drop into the world
		CBasePlayer* pPlayer = UTIL_GetCommandClient();
		trace_t tr;
		Vector forward;
		pPlayer->EyeVectors( &forward );
		AI_TraceLine(pPlayer->EyePosition(),
			pPlayer->EyePosition() + forward * MAX_TRACE_LENGTH,MASK_NPCSOLID, 
			pPlayer, COLLISION_GROUP_NONE, &tr );
		if ( tr.fraction != 1.0)
		{
			if (baseNPC->CapabilitiesGet() & bits_CAP_MOVE_FLY)
			{
				Vector pos = tr.endpos - forward * 36;
				baseNPC->Teleport( &pos, NULL, NULL );
			}
			else
			{
				// Raise the end position a little up off the floor, place the npc and drop him down
				tr.endpos.z += 12;
				baseNPC->Teleport( &tr.endpos, NULL, NULL );
				UTIL_DropToFloor( baseNPC, MASK_NPCSOLID );
			}

			// Now check that this is a valid location for the new npc to be
			Vector	vUpBit = baseNPC->GetAbsOrigin();
			vUpBit.z += 1;

			AI_TraceHull( baseNPC->GetAbsOrigin(), vUpBit, baseNPC->GetHullMins(), baseNPC->GetHullMaxs(), 
				MASK_NPCSOLID, baseNPC, COLLISION_GROUP_NONE, &tr );
			if ( tr.startsolid || (tr.fraction < 1.0) )
			{
				baseNPC->SUB_Remove();
				DevMsg("Can't create %s.  Bad Position!\n",args[1]);
				NDebugOverlay::Box(baseNPC->GetAbsOrigin(), baseNPC->GetHullMins(), baseNPC->GetHullMaxs(), 255, 0, 0, 0, 0);
			}
		}

		baseNPC->Activate();
	}
	CBaseEntity::SetAllowPrecache( allowPrecache );
}
#endif

AI_BEGIN_CUSTOM_NPC( npc_gebase, CNPC_GEBase )
{ // <-- This bracket is to keep us within the function so we can load GE schedules after init
	DECLARE_TASK( TASK_CLEAR_GOAL );

	// Reset the global id's
	g_GEAINextScheduleID	= (int) NEXT_SCHEDULE;
	g_GEAINextConditionID	= (int) NEXT_CONDITION;
	g_GEAINextTaskID		= (int) NEXT_TASK;

	AI_END_CUSTOM_NPC()

	GEAi()->LoadSchedules();
}
