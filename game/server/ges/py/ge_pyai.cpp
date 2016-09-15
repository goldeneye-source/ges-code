///////////// Copyright � 2012 GoldenEye: Source, All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_pyai.cpp
//   Description :
//     Defines the binding between HL2 Ai <-> Python Ai
//
//   Created On: 01/01/2012
//   Created By: Killermonkey <killermonkey01@gmail.com>
////////////////////////////////////////////////////////////////////////////
#include "ge_pyprecom.h"

#include "soundent.h"
#include "ammodef.h"

#include "ai_senses.h"
#include "ai_task.h"
#include "ai_hint.h"
#include "ai_behavior.h"
#include "ai_npcstate.h"

#include "ge_pymanager.h"
#include "npc_gebase.h"
#include "ge_ai.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// External functions defined elsewhere but used in our py defs
bool pyHasWeapon( CBaseCombatCharacter *, bp::object );
void Host_Say( edict_t *pEdict, const CCommand &args, bool teamonly );

// ------
// Python Task interface
// ------
class CGEPyTask : public bp::wrapper<CGEPyTask>
{
public:
	CGEPyTask() : id_(-1), name("NO_NAME") { }

	CGEPyTask( std::string my_name, int my_id ) : id_( my_id ), name( my_name ) { }

	void Register( const char *task_name )
	{
		// Rename us to our given name to ensure consistency
		name = task_name;

		int id_check = CNPC_GEBase::GetSchedulingSymbols()->TaskSymbolToId( name.c_str() );
		if ( id_check == -1 )
		{
			// Add the task, increment the global counter
			id_ = g_GEAINextTaskID;
			CNPC_GEBase::AccessClassScheduleIdSpaceDirect().AddTask( name.c_str(), id_, "CNPC_GEBase" );
			g_GEAINextTaskID++;
		}
		else
		{
			id_ = CNPC_GEBase::AccessClassScheduleIdSpaceDirect().TaskGlobalToLocal( id_check );
		}
	}

	int id_;
	std::string name;
};


// ------
// Python Condition interface
// ------
class CGEPyCondition : public bp::wrapper<CGEPyCondition>
{
public:
	CGEPyCondition() : id_(-1), name("NO_NAME") { }

	CGEPyCondition( std::string my_name, int my_id ) : id_( my_id ), name( my_name ) { }

	void Register( const char *cond_name )
	{
		// Rename us to our given name to ensure consistency
		name = cond_name;

		int id_check = CNPC_GEBase::GetSchedulingSymbols()->ConditionSymbolToId( name.c_str() );
		if ( id_check == -1 )
		{
			id_ = g_GEAINextConditionID;
			CNPC_GEBase::AccessClassScheduleIdSpaceDirect().AddCondition( name.c_str(), id_, "CNPC_GEBase" );
			g_GEAINextConditionID++;
		}
		else
		{
			id_ = CNPC_GEBase::AccessClassScheduleIdSpaceDirect().TaskGlobalToLocal( id_check );
		}
	}

	int id_;
	std::string name;
};

// ------
// Python Schedule interface
// ------
class CGEPySchedule : public bp::wrapper<CGEPySchedule>
{
public:
	CGEPySchedule() : id_(-1), name("NO_NAME") { }

	CGEPySchedule( std::string my_name, int my_id ) : id_( my_id ), name( my_name ) { }

	bp::object eq( bp::object other )
	{
		bp::extract<int> to_int( other );
		if ( to_int.check() )
			return bp::object( this->id_ == to_int() );
		return bp::object( bp::handle<>( bp::borrowed( Py_NotImplemented ) ) );
	}

	bp::object ne( bp::object other )
	{
		bp::extract<int> to_int( other );
		if ( to_int.check() )
			return bp::object( this->id_ != to_int() );
		return bp::object( bp::handle<>( bp::borrowed( Py_NotImplemented ) ) );
	}

	void Register( const char *sched_name )
	{
		// Rename us to our given name to ensure consistency
		name = sched_name;

		// Build a schedule definition based off the Python class contents
		CAI_Schedule *sched = NULL;
		id_ = CNPC_GEBase::GetSchedulingSymbols()->ScheduleSymbolToId( name.c_str() );

		if (id_ == -1 )
		{
			// Schedule name does not exist yet
			// Create the schedule in the namespace and schedule manager
			id_ = g_GEAINextScheduleID;
			CNPC_GEBase::AccessClassScheduleIdSpaceDirect().AddSchedule( name.c_str(), id_, "CNPC_GEBase" );
			sched = g_AI_SchedulesManager.CreateSchedule( const_cast<char*>(name.c_str()), CNPC_GEBase::GetScheduleID(name.c_str()) );
			g_GEAINextScheduleID++;
		}
		else
		{
			// Schedule exists
			// Clear out task list and interrupts
			sched = g_AI_SchedulesManager.GetScheduleFromID( id_ );
			delete [] sched->m_pTaskList;
			sched->m_InterruptMask.ClearAll();

			id_ = CNPC_GEBase::AccessClassScheduleIdSpaceDirect().ScheduleGlobalToLocal( id_ );
		}

		// Load our tasks from Python
		bp::list pyTasks;
		try {
			pyTasks = bp::call<bp::list>( this->get_override("GetTasks").ptr() );
		} catch ( bp::error_already_set const& ) {
			HandlePythonException();
		}

		// Setup the task list
		int numTasks = bp::len( pyTasks );
		sched->m_pTaskList = new Task_t[numTasks];
		sched->m_iNumTasks = numTasks;

		// Load the python tasks into our schedule
		for ( int i=0; i < numTasks; i++ )
		{
			bp::list pyTask = bp::extract<bp::list>( pyTasks[i] );
			if ( bp::len( pyTask ) > 1 )
			{
				try {
					CGEPyTask *task = bp::extract<CGEPyTask*>( pyTask[0] );
					float data = bp::extract<float>( pyTask[1] );
					if ( task->id_ == -1 )
						throw bp::error_already_set();

					sched->m_pTaskList[i].iTask = task->id_;
					sched->m_pTaskList[i].flTaskData = data;
				} catch ( bp::error_already_set const& ) {
					Warning( "GEAi: Invalid task id or data for task %i in schedule %s!\n", i+1, name.c_str() );
				}
			}
			else
			{
				Warning( "GEAi: Missing data for task %i in schedule %s: [task_id, task_data]!\n", i+1, name.c_str() );
			}
		}

		// Load our interrupts from Python
		bp::list pyInterrupts;
		try {
			pyInterrupts = bp::call<bp::list>( this->get_override("GetInterrupts").ptr() );
		} catch ( bp::error_already_set const& ) {
			HandlePythonException();
		}

		// Load the interrupts into our schedule
		int num_inter = bp::len( pyInterrupts );
		for ( int i=0; i < num_inter; i++ )
		{
			try {
				CGEPyCondition* cond = bp::extract<CGEPyCondition*>( pyInterrupts[i] );
				if ( cond->id_ == -1 )
					throw bp::error_already_set();
				sched->m_InterruptMask.Set( cond->id_ );
			} catch ( bp::error_already_set const& ) {
				Warning( "GEAi: Invalid interrupt specified at %i in schedule %s!\n", i+1, name.c_str() );
			}
		}
	}

	int id_;
	std::string name;
};

// ------
// Python Base NPC interface
// ------
class CGEPyBaseNPC : public CGEBaseNPC, public bp::wrapper<CGEPyBaseNPC>
{
public:
	CGEPyBaseNPC( CNPC_GEBase *pOuter ) : CGEBaseNPC( pOuter ) { }

	~CGEPyBaseNPC()
	{
		DevWarning( 2, "CGEPyBaseNPC: Destroyed\n" );
	}

	bp::object self;

	void Cleanup()
	{
		TRYFUNC( this->get_override("Cleanup")() );
		self = bp::object();
	}

	const char* GetIdent() { return bp::extract<const char*>( self.attr( "__class__" ).attr( "__name__" ) ); }

	// Returns the bot player instance if it exists
	CGEBotPlayer *GetPlayer() { return m_pOuter->GetBotPlayer(); }

	const char *GetNPCModel( void )
	{
		TRYFUNCRET( this->get_override("GetModel")(), NULL );
	}

	Class_T Classify( void )
	{
		TRYFUNCRET( this->get_override("Classify")(), CLASS_NONE );
	}

	void OnSpawn( void )
	{
		TRYFUNC( this->get_override("OnSpawn")() );
		PY_CALLHOOKS( FUNC_AI_ONSPAWN, bp::make_tuple() );
	}

	void OnSetDifficulty( int level )
	{
		TRYFUNC( this->get_override("OnSetDifficulty")(level) );
		PY_CALLHOOKS( FUNC_AI_SETDIFFICULTY, bp::make_tuple(level) );
	}

	void OnLooked( int dist )
	{
		PY_CALLHOOKS( FUNC_AI_ONLOOKED, bp::make_tuple(dist) );
		TRYFUNC( this->get_override("OnLooked")(dist) );
	}

	void OnListened( void )
	{
		PY_CALLHOOKS( FUNC_AI_ONLISTENED, bp::make_tuple() );
		TRYFUNC( this->get_override("OnListened")() );
	}

	void OnPickupItem( void )
	{
		PY_CALLHOOKS( FUNC_AI_PICKUPITEM, bp::make_tuple() );
	}

	bool IsValidEnemy( CBaseEntity *pEnemy )
	{
		TRYFUNCRET( this->get_override("IsValidEnemy")(bp::ptr(pEnemy)), true );
	}

	void GatherConditions( void )
	{
		TRYFUNC( this->get_override("GatherConditions")() );
		PY_CALLHOOKS( FUNC_AI_GATHERCONDITIONS, bp::make_tuple() );
	}

	NPC_STATE SelectIdealState( void )
	{
		TRYFUNCRET( this->get_override("SelectIdealState")(), NPC_STATE_INVALID );
	}

	int SelectSchedule( void )
	{
		PY_CALLHOOKS( FUNC_AI_SELECTSCHEDULE, bp::make_tuple() );
		try {
			CGEPySchedule *sched = this->get_override("SelectSchedule")();
			return sched->id_;
		} catch ( bp::error_already_set const& ) { 
			HandlePythonException();
		}
		return SCHED_NONE;
	}

	int  TranslateSchedule( int schedule )
	{
		try {
			CGEPySchedule *sched = this->get_override("TranslateSchedule")( schedule );
			return sched->id_;
		} catch ( bp::error_already_set const& ) { 
			HandlePythonException();
		}
		return SCHED_NONE;
	}

	bool ShouldInterruptSchedule( int schedule )
	{
		TRYFUNCRET( this->get_override("ShouldInterruptSchedule")(schedule), false );
	}

	bool CheckStartTask( const Task_t* task ) 
	{
		TRYFUNCRET( this->get_override("CheckStartTask")(task->iTask, task->flTaskData), false );
	}

	bool CheckRunTask( const Task_t* task ) 
	{
		TRYFUNCRET( this->get_override("CheckRunTask")(task->iTask, task->flTaskData), false );
	}

	void OnDebugCommand( const char *cmd )
	{
		TRYFUNC( this->get_override("OnDebugCommand")(cmd) );
	}

	// Senses
	bp::list GetSeenEntities()
	{ 
		bp::list seen;
		
		CAI_Senses *pSense = m_pOuter->GetSenses();
		if ( !pSense )
			return seen;

		AISightIter_t iter;
		CBaseEntity *pSightEnt = pSense->GetFirstSeenEntity( &iter );
		while( pSightEnt )
		{
			seen.append( bp::ptr(pSightEnt) );
			pSightEnt = pSense->GetNextSeenEntity( &iter );
		}
		return seen;
	}

	bp::list GetHeardSounds()
	{
		bp::list heard;

		CAI_Senses *pSense = m_pOuter->GetSenses();
		if ( !pSense )
			return heard;

		AISoundIter_t iter;
		CSound *pHeard = pSense->GetFirstHeardSound( &iter );
		while( pHeard )
		{
			heard.append( bp::ptr(pHeard) );
			pHeard = pSense->GetNextHeardSound( &iter );
		}
		return heard;
	}

	bp::list GetHeldWeapons()
	{
		bp::list weaps;
		// Iterate all the weapon slots and add the ones that exist
		CGEWeapon *pWeap = NULL;
		for ( int i=0; i < MAX_WEAPONS; i++ )
		{
			pWeap = ToGEWeapon( m_pOuter->GetWeapon(i) );
			if ( pWeap )
				weaps.append( bp::ptr(pWeap) );
		}
		return weaps;
	}

	bp::list GetHeldWeaponIds()
	{
		bp::list weaps;
		// Iterate all the weapon slots and add the ones that exist
		CGEWeapon *pWeap = NULL;
		for ( int i=0; i < MAX_WEAPONS; i++ )
		{
			pWeap = ToGEWeapon( m_pOuter->GetWeapon(i) );
			if ( pWeap )
				weaps.append( (int) pWeap->GetWeaponID() );
		}
		return weaps;
	}

	void Say( const char *msg )
	{
		CGEBotPlayer *pBot = m_pOuter->GetBotPlayer();
		if ( pBot )
		{
			char cmd[256];
			Q_snprintf( cmd, 256, "say %s", msg );
			CCommand args;
			args.Tokenize( cmd );
			Host_Say( pBot->edict(), args, false );
		}
	}

	void SayTeam( const char *msg )
	{
		CGEBotPlayer *pBot = m_pOuter->GetBotPlayer();
		if ( pBot )
		{
			char cmd[256];
			Q_snprintf( cmd, 256, "say_team %s", msg );
			CCommand args;
			args.Tokenize( cmd );
			Host_Say( pBot->edict(), args, true );
		}
	}
	
	//--------
	// PASS THROUGH FUNCTIONS!
	//--------

	// Basic Functions
	int GetTeamNumber()							{ return m_pOuter->GetTeamNumber(); }

	// Ai Functions
	CBaseEntity *GetEnemy()						{ return m_pOuter->GetEnemy(); }
	void SetEnemy( CBaseEntity *pEnt )			{ m_pOuter->SetEnemy( pEnt ); }
	CBaseEntity *GetTarget()					{ return m_pOuter->GetTarget(); }
	void SetTarget( CBaseEntity *target )		{ m_pOuter->SetTarget( target ); }
	void SetTargetPos( const Vector &origin )	{ m_pOuter->SetTargetPos( origin ); }
	bool IsSelected()							{ return (m_pOuter->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT) != 0; }

	// State
	int GetMaxHealth()							{ return m_pOuter->GetBotPlayer()->GetMaxHealth(); }
	int GetArmor()								{ return m_pOuter->GetBotPlayer()->ArmorValue(); }
	int GetMaxArmor()							{ return m_pOuter->GetBotPlayer()->GetMaxArmor(); }
	void SetArmor( int val )					{ m_pOuter->GetBotPlayer()->SetArmorValue( val ); }

	// Weapons
	bool HasWeapon( bp::object weapon )			{ return pyHasWeapon( m_pOuter, weapon ); }
	CGEWeapon *FindWeapon( Vector dist )		{ return (CGEWeapon*) m_pOuter->Weapon_FindUsable( dist ); }
	CGEWeapon *GiveWeapon( const char *ident )	{ return (CGEWeapon*) m_pOuter->GiveNamedItem( ident ); }
	void GiveAmmo( int amt, const char *ident ) { m_pOuter->GiveAmmo( amt, ident, true ); }
	int  GetAmmoCount( char *ammo )				{ return m_pOuter->GetAmmoCount( ammo ); }
	int  GetMaxAmmoCount( char *ammo )			{ return GetAmmoDef()->MaxCarry( GetAmmoDef()->Index(ammo) ); }

	// Capabilities
	void AddCapabilities( int cap )				{ m_pOuter->CapabilitiesAdd( cap ); }
	void RemoveCapabilities( int cap )			{ m_pOuter->CapabilitiesRemove( cap ); }
	void ClearCapabilities()					{ m_pOuter->CapabilitiesClear(); }
	bool HasCapability( int cap )				{ return (m_pOuter->CapabilitiesGet() & cap) == cap; }

	// Schedules, Tasks, Conditions
	void SetCondition( CGEPyCondition* cond )	{ m_pOuter->SetCondition( cond->id_ ); }
	void ClearCondition( CGEPyCondition* cond )	{ m_pOuter->ClearCondition( cond->id_ ); }
	bool HasCondition( CGEPyCondition* cond )	{ return m_pOuter->HasCondition( cond->id_ ); }
	int  GetState()								{ return m_pOuter->GetState(); }
	void TaskComplete( bool ignore )			{ m_pOuter->TaskComplete( ignore ); }
	void TaskFail( AI_TaskFailureCode_t code )	{ m_pOuter->TaskFail( code ); }
	void TaskFailStr( const char *reason )		{ m_pOuter->CAI_BaseNPC::TaskFail( reason ); }
	bool SetSchedule( CGEPySchedule *schedule )	{ return m_pOuter->SetSchedule( schedule->id_  ); }
	void ClearSchedule()						{ m_pOuter->ClearSchedule( "Python cleared schedule" ); }

	// Memory
	void Remember( int memory )					{ m_pOuter->Remember( memory ); }
	void Forget( int memory )					{ m_pOuter->Forget( memory ); }
	bool HasMemory( int memory )				{ return m_pOuter->HasMemory( memory ); }

	// Relationships
	void AddRelationship( CBaseEntity *pEnt, Disposition_t disp, int priority )		{ m_pOuter->AddEntityRelationship( pEnt, disp, priority ); }
	void AddClassRelationship( Class_T nClass, Disposition_t disp, int priority )	{ m_pOuter->AddClassRelationship( nClass, disp, priority ); }
	void RemoveRelationship( CBaseEntity *pEnt )									{ m_pOuter->RemoveEntityRelationship( pEnt ); }
	Disposition_t GetRelationship( CBaseEntity *pEnt )								{ return m_pOuter->IRelationType( pEnt ); }
};


// ------
// Python AIManager interface
// ------
class CGEPyAiManager : public CGEAiManager, public IPythonListener, public bp::wrapper<CGEAiManager>
{
public:
	CGEPyAiManager()
	{
		// Do nothing for now
	}

	~CGEPyAiManager()
	{
		for ( int i=0; i < bp::len(npcs); i++ )
		{
			bp::extract<CGEPyBaseNPC*> tmp( npcs[i] );
			if ( tmp.check() )
				tmp()->Cleanup();
		}
	}

	virtual void OnPythonShutdown()
	{
		// Do something?
	}

	CGEBaseNPC* CreateNPC( const char *name, CNPC_GEBase *parent )
	{
		try {
			bp::object npc = bp::call<bp::object>( this->get_override("CreateNPC").ptr(), name, bp::ptr(parent) );

			if ( npc.is_none() )
			{
				DevWarning( "GE NPC: Failed to associate python with %s\n", name );
				return NULL;
			}
			
			// Store internally
			npcs.append( npc );

			// Take out our usable pointer
			CGEPyBaseNPC *pNPC = bp::extract<CGEPyBaseNPC*>( npc );
			pNPC->self = npc;
			return pNPC;
		} catch (...) {
			HandlePythonException();
			return NULL;
		}
	}

	void RemoveNPC( CGEBaseNPC *pNPC )
	{
		if ( !pNPC )
			return;

		pNPC->Cleanup();

		for ( int i=0; i < bp::len(npcs); i++ )
		{
			bp::extract<CGEPyBaseNPC*> tmp( npcs[i] );
			if ( tmp.check() && tmp() == pNPC )
			{
				npcs[i].del();
				return;
			}
		}
	}

	void LoadSchedules( void )
	{
		TRYCALL( this->get_override("ClearSchedules")() );
		TRYCALL( this->get_override("LoadSchedules")() );
	}

private:
	bp::list npcs;
};

int pyHintFlags( const char *token )
{
	return CAI_HintManager::GetFlags( token );
}

int pyActivityId( const char *token )
{
	int actID = CAI_BaseNPC::GetActivityID( token );
	if ( actID == -1 )
		Warning( "Invalid activity `%s` given to converter\n", token );

	return actID;
}

int pyScheduleID( const char *sched )
{
	int id = CNPC_GEBase::GetSchedulingSymbols()->ScheduleSymbolToId( sched );
	return CNPC_GEBase::AccessClassScheduleIdSpaceDirect().ScheduleGlobalToLocal( id );
}

int pyTaskID( const char *task )
{
	int id = CNPC_GEBase::GetSchedulingSymbols()->TaskSymbolToId( task );
	return CNPC_GEBase::AccessClassScheduleIdSpaceDirect().TaskGlobalToLocal( id );
}

int pyConditionID( const char *cond )
{
	int id = CNPC_GEBase::GetSchedulingSymbols()->ConditionSymbolToId( cond );
	return CNPC_GEBase::AccessClassScheduleIdSpaceDirect().ConditionGlobalToLocal( id );
}

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(CNPC_GEBase_HasCondition_overloads, CNPC_GEBase::pyHasCondition, 1, 2);

BOOST_PYTHON_MODULE(GEAi)
{
	bp::def("HintFlags", pyHintFlags);
	bp::def("ActivityId", pyActivityId);

	bp::def("ScheduleID", pyScheduleID);
	bp::def("TaskID", pyTaskID);
	bp::def("ConditionID", pyConditionID);

	bp::class_<CGEPyBaseNPC, boost::noncopyable>("CBaseNPC", bp::init<CNPC_GEBase*>())
		.def("GetNPC", &CGEPyBaseNPC::GetOuter, bp::return_value_policy<bp::reference_existing_object>())
		.def("GetPlayer", &CGEPyBaseNPC::GetPlayer, bp::return_value_policy<bp::reference_existing_object>())
		// Custom functions
		.def("GetSeenEntities", &CGEPyBaseNPC::GetSeenEntities)
		.def("GetHeardSounds", &CGEPyBaseNPC::GetHeardSounds)
		.def("GetHeldWeapons", &CGEPyBaseNPC::GetHeldWeapons)
		.def("GetHeldWeaponIds", &CGEPyBaseNPC::GetHeldWeaponIds)
		.def("Say", &CGEPyBaseNPC::Say)
		.def("SayTeam", &CGEPyBaseNPC::SayTeam)
		// Pass-Through functions (to CNPC_GEBase)
		.def("HasWeapon", &CGEPyBaseNPC::HasWeapon)
		.def("GiveWeapon", &CGEPyBaseNPC::GiveWeapon, bp::return_value_policy<bp::reference_existing_object>())
		.def("GiveAmmo", &CGEPyBaseNPC::GiveAmmo)
		.def("GetAmmoCount", &CGEPyBaseNPC::GetAmmoCount)
		.def("GetMaxAmmoCount", &CGEPyBaseNPC::GetMaxAmmoCount)
		.def("GetMaxHealth", &CGEPyBaseNPC::GetMaxHealth)
		.def("GetArmor", &CGEPyBaseNPC::GetArmor)
		.def("GetMaxArmor", &CGEPyBaseNPC::GetMaxArmor)
		.def("SetArmor", &CGEPyBaseNPC::SetArmor)
		.def("GetEnemy", &CGEPyBaseNPC::GetEnemy, bp::return_value_policy<bp::reference_existing_object>())
		.def("SetEnemy", &CGEPyBaseNPC::SetEnemy)
		.def("SetTarget", &CGEPyBaseNPC::SetTarget)
		.def("SetTargetPos", &CGEPyBaseNPC::SetTargetPos)
		.def("GetTarget", &CGEPyBaseNPC::GetTarget, bp::return_value_policy<bp::reference_existing_object>())
		.def("FindWeapon", &CGEPyBaseNPC::FindWeapon, bp::return_value_policy<bp::reference_existing_object>())
		.def("AddCapabilities", &CGEPyBaseNPC::AddCapabilities)
		.def("HasCapability", &CGEPyBaseNPC::HasCapability)
		.def("RemoveCapabilities", &CGEPyBaseNPC::RemoveCapabilities)
		.def("ClearCapabilities", &CGEPyBaseNPC::ClearCapabilities)
		.def("AddRelationship", &CGEPyBaseNPC::AddRelationship)
		.def("RemoveRelationship", &CGEPyBaseNPC::RemoveRelationship)
		.def("AddClassRelationship", &CGEPyBaseNPC::AddClassRelationship)
		.def("GetRelationship", &CGEPyBaseNPC::GetRelationship)
		.def("SetCondition", &CGEPyBaseNPC::SetCondition)
		.def("HasCondition", &CGEPyBaseNPC::HasCondition)
		.def("ClearCondition", &CGEPyBaseNPC::ClearCondition)
		.def("GetState", &CGEPyBaseNPC::GetState)
		.def("TaskComplete", &CGEPyBaseNPC::TaskComplete)
		.def("TaskFail", &CGEPyBaseNPC::TaskFail)
		.def("TaskFail", &CGEPyBaseNPC::TaskFailStr)
		.def("SetSchedule", &CGEPyBaseNPC::SetSchedule)
		.def("ClearSchedule", &CGEPyBaseNPC::ClearSchedule)
		.def("Remember", &CGEPyBaseNPC::Remember)
		.def("Forget", &CGEPyBaseNPC::Forget)
		.def("HasMemory", &CGEPyBaseNPC::HasMemory)
		.def("GetTeamNumber", &CGEPyBaseNPC::GetTeamNumber)
		.def("IsSelected", &CGEPyBaseNPC::IsSelected);

	bp::class_<CGEPyTask>("ITask", bp::init<>())
		.def(bp::init<std::string, int>())
		.def("Register", &CGEPyTask::Register)
		.def_readonly("name", &CGEPyTask::name)
		.def_readonly("id_", &CGEPyTask::id_);

	bp::class_<CGEPyCondition>("ICondition", bp::init<>())
		.def(bp::init<std::string, int>())
		.def("Register", &CGEPyCondition::Register)
		.def_readonly("name", &CGEPyCondition::name)
		.def_readonly("id_", &CGEPyCondition::id_);

	bp::class_<CGEPySchedule>("ISchedule", bp::init<>())
		.def(bp::init<std::string, int>())
		.def("__eq__", &CGEPySchedule::eq)
		.def("__ne__", &CGEPySchedule::ne)
		.def("Register", &CGEPySchedule::Register)
		.def_readonly("name", &CGEPySchedule::name)
		.def_readonly("id_", &CGEPySchedule::id_);

	bp::class_<CGEPyAiManager, boost::noncopyable>("CAiManager");
}

extern CGEAiManager *g_pAiManager;

void CreateAiManager()
{
	if ( GEAi() )
	{
		DevWarning( "Ai Manager already exists, shutting it down!\n" );
		ShutdownAiManager();
	}

	try {
		// Load our manager
		bp::object init = bp::import( "GESInit" );
		bp::object aimgr = init.attr( "LoadManager" )("AiManager");

		g_pAiManager = bp::extract<CGEPyAiManager*>( aimgr );

		// Check for manager load failure
		if ( !g_pAiManager )
			throw bp::error_already_set();
	} catch (...) {
		HandlePythonException();
		AssertFatal( "Failed to load python Ai manager!" );
	}
}

void ShutdownAiManager()
{
	// Remove the instances from the NPC's
	for ( int i=0; i < g_AI_Manager.NumAIs(); i++ )
	{
		CNPC_GEBase *npc = dynamic_cast<CNPC_GEBase*>( g_AI_Manager.AccessAIs()[i] );
		if ( npc )
			npc->RemovePyInterface();
	}

	try {
		bp::object init = bp::import( "GESInit" );
		init.attr( "UnloadManager" )( "AiManager" );
	} catch (bp::error_already_set const &) { }

	g_pAiManager = NULL;
}


CON_COMMAND( ge_ai_reboot, "Reboots the AI Manager and all bots" )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	// Reboot the Ai Manager
	ShutdownAiManager();
	CreateAiManager();

	// Loop through all npc_gebase entities and reboot their python link
	CNPC_GEBase *pNPC = (CNPC_GEBase*) gEntList.FindEntityByClassname( NULL, "npc_gebase" );
	while( pNPC )
	{
		pNPC->ClearSchedule( "Reboot" );
		pNPC->InitPyInterface( pNPC->GetIdent() );
		pNPC = (CNPC_GEBase*) gEntList.FindEntityByClassname( pNPC, "npc_gebase" );
	}

	// Reload the tasks, schedules, and conditions from python
	GEAi()->LoadSchedules();
}

// TODO: This needs to be rethinked
CON_COMMAND( ge_ai_debugcmd, "Send a command to the named NPC's OnDebugCommand function" )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( args.ArgC() < 2 )
	{
		Warning( "Usage: ge_ai_debugcmd [botname] [command], Ex: ge_ai_debugcmd Bot1 dumpmemory\n" );
		Warning( "You need to supply the name of the bot and the command to send!\n" );
		return;
	}

	bool cmdSent = false;
	FOR_EACH_BOTPLAYER( pBot )
		// If we only supplied a command just send it to the first bot
		if ( pBot->GetNPC() && args.ArgC() < 3 )
		{
			pBot->GetNPC()->SendDebugCommand( args[1] );
			cmdSent = true;
			break;
		}

		if ( pBot->GetNPC() && !Q_stricmp(pBot->GetCleanPlayerName(), args[1]) )
		{
			pBot->GetNPC()->SendDebugCommand( args[2] );
			cmdSent = true;
			break;
		}
	END_OF_PLAYER_LOOP()

	if ( !cmdSent )
		Warning( "Bot was not found, command not sent!\n" );
}
