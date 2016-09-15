///////////// Copyright © 2009 LodleNet. All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_ai.h
//   Description :
//      [TODO: Write the purpose of ge_ai.h.]
//
//   Created On: 9/8/2009 8:38:07 PM
//   Created By: Mark Chandler <mailto:mark@moddb.com>
////////////////////////////////////////////////////////////////////////////

#ifndef GE_AI_H
#define GE_AI_H

#include "ai_npcstate.h"

class CNPC_GEBase;
struct Task_t;

// --------------------
// Creation, Shutdown, Reboot
// --------------------
void CreateAiManager();
void ShutdownAiManager();

// --------------------
// The basic NPC <-> Python link
// --------------------
class CGEBaseNPC
{
public:
	CGEBaseNPC( CNPC_GEBase *pOuter ) { m_pOuter = pOuter; }
	virtual CNPC_GEBase *GetOuter() { return m_pOuter; }
	virtual const char* GetIdent()=0;
	virtual void Cleanup()=0;

	virtual const char *GetNPCModel()=0;
	virtual Class_T Classify()=0;

	virtual void OnSpawn()=0;
	virtual void OnSetDifficulty( int level )=0;
	virtual void OnLooked( int dist )=0;
	virtual void OnListened()=0;

	virtual bool IsValidEnemy( CBaseEntity *pEnemy )=0;

	virtual void OnPickupItem()=0;
	virtual void GatherConditions()=0;
	virtual NPC_STATE SelectIdealState()=0;
	virtual int  SelectSchedule()=0;
	virtual int  TranslateSchedule( int schedule )=0;
	virtual bool ShouldInterruptSchedule( int schedule )=0;
	virtual bool CheckStartTask( const Task_t *task )=0;
	virtual bool CheckRunTask( const Task_t *task )=0;

	virtual void OnDebugCommand( const char *cmd )=0;

protected:
	CNPC_GEBase *m_pOuter;
};

// -------------------------
// AI Manager that lets us create NPCs and load schedules/conditions/tasks
// -------------------------
class CGEAiManager
{
public:
	// Python overloads
	virtual CGEBaseNPC* CreateNPC( const char *name, CNPC_GEBase *parent )=0;
	virtual void RemoveNPC( CGEBaseNPC *npc )=0;
	virtual void LoadSchedules()=0;
};

CGEAiManager* const GEAi();

#endif //DESURA_GE_AI_H
