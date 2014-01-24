///////////// Copyright © 2008 LodleNet. All rights reserved. /////////////
//
//   Project     : Client (GES)
//   File        : npc_guard.h
//   Description :
//      [TODO: Write the purpose of npc_guard.h.]
//
//   Created On: 12/25/2008 8:17:24 PM
//   Created By: Mark Chandler <mailto:admin@lodle.net>
////////////////////////////////////////////////////////////////////////////

#ifndef MC_NPC_GUARD_H
#define MC_NPC_GUARD_H
#ifdef _WIN32
#pragma once
#endif

#include "npc_gebase.h"

class CNPC_GESGuard : public CNPC_GEBase
{
public:
	CNPC_GESGuard();
	~CNPC_GESGuard();

	DECLARE_CLASS( CNPC_GESGuard, CNPC_GEBase );
	DEFINE_CUSTOM_AI;

	void	Precache( void );
	void	Spawn( void );
	Class_T Classify( void );

	const char* GetNPCModel();

protected:
	int SelectSchedule( void );
	int SelectScheduleAttack();
	int SelectCombatSchedule();

	int TranslateSchedule( int scheduleType );
	int SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
	Activity NPC_TranslateActivity( Activity eNewActivity );

	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );
	void BuildScheduleTestBits( void );

	NPC_STATE SelectIdealState( void );

	WeaponProficiency_t CalcWeaponProficiency( CBaseCombatWeapon *pWeapon );

private:
	CBaseEntity		*m_pAlarmEnt;
	
private:
	enum
	{
		SCHED_GESGUARD_PRESS_ATTACK = BaseClass::NEXT_SCHEDULE,
		SCHED_GESGUARD_RANGE_ATTACK1,
		SCHED_GESGUARD_PATROL,
		SCHED_GESGUARD_GUARD,
		SCHED_GESGUARD_SOUNDALARM,
		SCHED_GESGUARD_CROUCH,
		SCHED_GESGUARD_STAND,
		SCHED_GESGUARD_FLEE,
		SCHED_GESGUARD_CHASE,
		SCHED_GESGUARD_IDLE,
		SCHED_GESGUARD_ESTABLISH_LINE_OF_FIRE,
		NEXT_SCHEDULE,
	};


	enum 
	{
		TASK_GESGUARD_SET_STANDING = BaseClass::NEXT_TASK,
		TASK_GESGUARD_GET_ALARM,
		TASK_GESGUARD_FACE_ALARM,
		TASK_GESGUARD_USE_ALARM,
		NEXT_TASK,
	};

	enum 
	{
		COND_GESGUARD_SHOULD_PATROL = BaseClass::NEXT_CONDITION,
		NEXT_CONDITION,
	};


};



#endif //MC_NPC_GUARD_H
