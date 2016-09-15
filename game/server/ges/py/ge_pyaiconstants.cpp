///////////// Copyright © 2009 LodleNet. All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_pyaiconstants.cpp
//   Description :
//
//   Created On: 02/27/11
//   Created By: Jonathan White <killermonkey>
////////////////////////////////////////////////////////////////////////////

#include "ge_pyprecom.h"
#include "ge_shareddefs.h"
#include "ai_default.h"
#include "ai_basenpc.h"
#include "ai_condition.h"
#include "ai_task.h"
#include "ai_npcstate.h"
#include "ai_senses.h"
#include "baseentity.h"
#include "npc_gebase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Redefine this here since we don't want to clutter our scope...
enum Memory_t
{
	MEM_CLEAR = MEMORY_CLEAR,
	MEMORY_PROVOKED = bits_MEMORY_PROVOKED,
	MEMORY_INCOVER = bits_MEMORY_INCOVER,
	MEMORY_SUSPICIOUS = bits_MEMORY_SUSPICIOUS,
	MEMORY_TASK_EXPENSIVE = bits_MEMORY_TASK_EXPENSIVE,
	MEMORY_PATH_FAILED = bits_MEMORY_PATH_FAILED,
	MEMORY_FLINCHED = bits_MEMORY_FLINCHED,
	MEMORY_TOURGUIDE = bits_MEMORY_TOURGUIDE,
	MEMORY_LOCKED_HINT = bits_MEMORY_LOCKED_HINT,
	MEMORY_TURNING = bits_MEMORY_TURNING,
	MEMORY_TURNHACK = bits_MEMORY_TURNHACK,
	MEMORY_HAD_ENEMY = bits_MEMORY_HAD_ENEMY,
	MEMORY_HAD_PLAYER = bits_MEMORY_HAD_PLAYER,
	MEMORY_HAD_LOS = bits_MEMORY_HAD_LOS,
	MEMORY_MOVED_FROM_SPAWN = bits_MEMORY_MOVED_FROM_SPAWN,
	MEMORY_CUSTOM4 = bits_MEMORY_CUSTOM4,
	MEMORY_CUSTOM3 = bits_MEMORY_CUSTOM3,
	MEMORY_CUSTOM2 = bits_MEMORY_CUSTOM2,
	MEMORY_CUSTOM1 = bits_MEMORY_CUSTOM1,
};

#define ENUM_ATTR( value ) (int) value

BOOST_PYTHON_MODULE(GEAiConst)
{
	using namespace boost::python;

	// TODO: We need to convert this into a dynamic thing
	enum_< Class_T  >("Class")
		.value("NONE", CLASS_NONE)
		.value("PLAYER", CLASS_PLAYER)
		.value("PLAYER_ALLY", CLASS_PLAYER_ALLY)
		.value("GES_BOT", CLASS_GESBOT)
		.value("GES_GUARD", CLASS_GESGUARD)
		.value("GES_NONCOMBATANT", CLASS_GESNONCOMBAT);

	enum_< Disposition_t >("Disposition")
		.value("HATE", D_HT)
		.value("FEAR", D_FR)
		.value("LIKE", D_LI)
		.value("NEUTRAL", D_NU);

	enum_< NPC_STATE >("State")
		.value("NO_SELECTION", NPC_STATE_INVALID)
		.value("NONE", NPC_STATE_NONE)
		.value("IDLE", NPC_STATE_IDLE)
		.value("ALERT", NPC_STATE_ALERT)
		.value("COMBAT", NPC_STATE_COMBAT)
		.value("DEAD", NPC_STATE_DEAD);

	enum_< NPC_LEVEL >("Level")
		.value("AGENT", LEVEL_AGENT)
		.value("SECRET_AGENT", LEVEL_SECRET_AGENT)
		.value("00_AGENT", LEVEL_00_AGENT)
		.value("007", LEVEL_007);

	enum_< goalType_t >("Goal")
		.value("NONE", GOAL_NONE)
		.value("ENEMY", GOAL_ENEMY)
		.value("TARGET", GOAL_TARGET)
		.value("ENEMY_LKP", GOAL_ENEMY_LKP)
		.value("SAVED_POSITION", GOAL_SAVED_POSITION);

	enum_< pathType_t >("Path")
		.value("NONE", PATH_NONE)
		.value("TRAVEL", PATH_TRAVEL)
		.value("LOS", PATH_LOS)
		.value("COVER", PATH_COVER);
	
	class CCapability {};
	class_<CCapability>("Capability")
		.setattr("MOVE_GROUND", ENUM_ATTR(bits_CAP_MOVE_GROUND))
		.setattr("MOVE_JUMP", ENUM_ATTR(bits_CAP_MOVE_JUMP))
		.setattr("MOVE_FLY", ENUM_ATTR(bits_CAP_MOVE_FLY))
		.setattr("MOVE_CLIMB", ENUM_ATTR(bits_CAP_MOVE_CLIMB))
		.setattr("MOVE_SWIM", ENUM_ATTR(bits_CAP_MOVE_SWIM))
		.setattr("MOVE_SHOOT", ENUM_ATTR(bits_CAP_MOVE_SHOOT))
		.setattr("SKIP_NAV_GROUND_CHECK", ENUM_ATTR(bits_CAP_SKIP_NAV_GROUND_CHECK))
		.setattr("USE", ENUM_ATTR(bits_CAP_USE))
		.setattr("USE_DOORS", ENUM_ATTR(bits_CAP_AUTO_DOORS | bits_CAP_OPEN_DOORS))
		.setattr("TURN_HEAD", ENUM_ATTR(bits_CAP_TURN_HEAD))
		.setattr("WEAPON_RANGE_ATTACK", ENUM_ATTR(bits_CAP_WEAPON_RANGE_ATTACK1))
		.setattr("WEAPON_MELEE_ATTACK", ENUM_ATTR(bits_CAP_WEAPON_MELEE_ATTACK1))
		.setattr("USE_WEAPONS", ENUM_ATTR(bits_CAP_USE_WEAPONS))
		.setattr("ANIMATEDFACE", ENUM_ATTR(bits_CAP_ANIMATEDFACE))
		.setattr("USE_SHOT_REGULATOR", ENUM_ATTR(bits_CAP_USE_SHOT_REGULATOR))
		.setattr("FRIENDLY_DMG_IMMUNE", ENUM_ATTR(bits_CAP_FRIENDLY_DMG_IMMUNE))
		.setattr("SQUAD", ENUM_ATTR(bits_CAP_SQUAD))
		.setattr("DUCK", ENUM_ATTR(bits_CAP_DUCK))
		.setattr("NO_HIT_PLAYER", ENUM_ATTR(bits_CAP_NO_HIT_PLAYER))
		.setattr("AIM_GUN", ENUM_ATTR(bits_CAP_AIM_GUN))
		.setattr("NO_HIT_SQUADMATES", ENUM_ATTR(bits_CAP_NO_HIT_SQUADMATES));

	class CMemory {};
	class_<CMemory>("Memory")
		.setattr("CLEAR", ENUM_ATTR(MEM_CLEAR))
		.setattr("PROVOKED", ENUM_ATTR(MEMORY_PROVOKED))
		.setattr("INCOVER", ENUM_ATTR(MEMORY_INCOVER))
		.setattr("SUSPICIOUS", ENUM_ATTR(MEMORY_SUSPICIOUS))
		.setattr("TASK_EXPENSIVE", ENUM_ATTR(MEMORY_TASK_EXPENSIVE))
		.setattr("PATH_FAILED", ENUM_ATTR(MEMORY_PATH_FAILED))
		.setattr("FLINCHED", ENUM_ATTR(MEMORY_FLINCHED))
		.setattr("TOURGUIDE", ENUM_ATTR(MEMORY_TOURGUIDE))
		.setattr("LOCKED_HINT", ENUM_ATTR(MEMORY_LOCKED_HINT))
		.setattr("TURNING", ENUM_ATTR(MEMORY_TURNING))
		.setattr("TURNHACK", ENUM_ATTR(MEMORY_TURNHACK))
		.setattr("HAD_ENEMY", ENUM_ATTR(MEMORY_HAD_ENEMY))
		.setattr("HAD_PLAYER", ENUM_ATTR(MEMORY_HAD_PLAYER))
		.setattr("HAD_LOS", ENUM_ATTR(MEMORY_HAD_LOS))
		.setattr("MOVED_FROM_SPAWN", ENUM_ATTR(MEMORY_MOVED_FROM_SPAWN))
		.setattr("STUCK", ENUM_ATTR(MEMORY_CUSTOM4))
		.setattr("CUSTOM3", ENUM_ATTR(MEMORY_CUSTOM3))
		.setattr("CUSTOM2", ENUM_ATTR(MEMORY_CUSTOM2))
		.setattr("CUSTOM1", ENUM_ATTR(MEMORY_CUSTOM1));
}

#define SCHED( name )	#name, sched( #name, (int) SCHED_##name )
class CDummySchedule {};

BOOST_PYTHON_MODULE(GEAiSched)
{
	using namespace boost::python;

	object ai = import( "GEAi" );
	object sched = ai.attr( "ISchedule" );

	class_<CDummySchedule>( "Sched" )
		.setattr( "NO_SELECTION", sched( "NO_SELECTION", (int) SCHED_NONE ) )
		.setattr( SCHED( IDLE_STAND ) )
		.setattr( SCHED( IDLE_WALK ) )
		.setattr( SCHED( IDLE_WANDER ) )
		.setattr( SCHED( WAKE_ANGRY ) )
		.setattr( SCHED( ALERT_FACE ) )
		.setattr( SCHED( ALERT_FACE_BESTSOUND ) )
		.setattr( SCHED( ALERT_REACT_TO_COMBAT_SOUND ) )
		.setattr( SCHED( ALERT_SCAN ) )
		.setattr( SCHED( ALERT_STAND ) )
		.setattr( SCHED( ALERT_WALK ) )
		.setattr( SCHED( INVESTIGATE_SOUND ) )
		.setattr( SCHED( COMBAT_FACE ) )
		.setattr( SCHED( COMBAT_SWEEP ) )
		.setattr( SCHED( FEAR_FACE ) )
		.setattr( SCHED( COMBAT_STAND ) )
		.setattr( SCHED( COMBAT_WALK ) )
		.setattr( SCHED( CHASE_ENEMY ) )
		.setattr( SCHED( CHASE_ENEMY_FAILED ) )
		.setattr( SCHED( TARGET_FACE ) )
		.setattr( SCHED( TARGET_CHASE ) )
		.setattr( SCHED( BACK_AWAY_FROM_ENEMY ) )
		.setattr( SCHED( MOVE_AWAY_FROM_ENEMY ) )
		.setattr( SCHED( BACK_AWAY_FROM_SAVE_POSITION ) )
		.setattr( SCHED( TAKE_COVER_FROM_ENEMY ) )
		.setattr( SCHED( TAKE_COVER_FROM_BEST_SOUND ) )
		.setattr( SCHED( FLEE_FROM_BEST_SOUND ) )
		.setattr( SCHED( TAKE_COVER_FROM_ORIGIN ) )
		.setattr( SCHED( FAIL_TAKE_COVER ) )
		.setattr( SCHED( RUN_FROM_ENEMY ) )
		.setattr( SCHED( RUN_FROM_ENEMY_FALLBACK ) )
		.setattr( SCHED( MOVE_TO_WEAPON_RANGE ) )
		.setattr( SCHED( ESTABLISH_LINE_OF_FIRE ) )
		.setattr( SCHED( ESTABLISH_LINE_OF_FIRE_FALLBACK ) )
		.setattr( SCHED( FAIL_ESTABLISH_LINE_OF_FIRE ) )
		.setattr( SCHED( SHOOT_ENEMY_COVER ) )
		.setattr( SCHED( MELEE_ATTACK1 ) )
		.setattr( SCHED( MELEE_ATTACK2 ) )
		.setattr( SCHED( RANGE_ATTACK1 ) )
		.setattr( SCHED( RANGE_ATTACK2 ) )
		.setattr( SCHED( SPECIAL_ATTACK1 ) )
		.setattr( SCHED( SPECIAL_ATTACK2 ) )
		.setattr( SCHED( STANDOFF ) )
		.setattr( SCHED( HIDE_AND_RELOAD ) )
		.setattr( SCHED( RELOAD ) )
		.setattr( SCHED( DIE ) )
		.setattr( SCHED( DIE_RAGDOLL ) )
		.setattr( SCHED( MOVE_AWAY ) )
		.setattr( SCHED( MOVE_AWAY_FAIL ) )
		.setattr( SCHED( MOVE_AWAY_END ) )
		.setattr( SCHED( FORCED_GO ) )
		.setattr( SCHED( FORCED_GO_RUN ) )
		.setattr( SCHED( NPC_FREEZE ) )
		.setattr( SCHED( PATROL_WALK ) )
		.setattr( SCHED( COMBAT_PATROL ) )
		.setattr( SCHED( PATROL_RUN ) )
		.setattr( SCHED( RUN_RANDOM ) )
		.setattr( SCHED( FALL_TO_GROUND ) )
		.setattr( SCHED( FAIL ) )
		.setattr( SCHED( FAIL_NOSTOP ) )
		.setattr( SCHED( RUN_FROM_ENEMY_MOB ) )
		.setattr( SCHED( SLEEP ) );
}

#define TASK( name )	#name, task( #name, (int) TASK_##name )
class CDummyTask {};

BOOST_PYTHON_MODULE(GEAiTasks)
{
	using namespace boost::python;

	object ai( import( "GEAi" ) );
	object task = ai.attr( "ITask" );

	class_<CDummyTask>("Task")
		.setattr( TASK(INVALID) )
		.setattr( TASK(RESET_ACTIVITY) )
		.setattr( TASK(WAIT) )
		.setattr( TASK(ANNOUNCE_ATTACK) )
		.setattr( TASK(WAIT_FACE_ENEMY) )
		.setattr( TASK(WAIT_FACE_ENEMY_RANDOM) )
		.setattr( TASK(WAIT_PVS) )
		.setattr( TASK(SUGGEST_STATE) )
		.setattr( TASK(TARGET_PLAYER) )
		.setattr( TASK(MOVE_TO_TARGET_RANGE) )
		.setattr( TASK(MOVE_AWAY_PATH) )
		.setattr( TASK(GET_PATH_AWAY_FROM_BEST_SOUND) )
		.setattr( TASK(GET_PATH_TO_ENEMY) )
		.setattr( TASK(GET_PATH_TO_ENEMY_LKP) )
		.setattr( TASK(GET_CHASE_PATH_TO_ENEMY) )
		.setattr( TASK(GET_PATH_TO_ENEMY_LKP_LOS) )
		.setattr( TASK(GET_PATH_TO_ENEMY_CORPSE) )
		.setattr( TASK(GET_PATH_TO_PLAYER) )
		.setattr( TASK(GET_PATH_TO_ENEMY_LOS) )
		.setattr( TASK(GET_FLANK_RADIUS_PATH_TO_ENEMY_LOS) )
		.setattr( TASK(GET_FLANK_ARC_PATH_TO_ENEMY_LOS) )
		.setattr( TASK(GET_PATH_TO_RANGE_ENEMY_LKP_LOS) )
		.setattr( TASK(GET_PATH_TO_TARGET) )
		.setattr( TASK(GET_PATH_TO_HINTNODE) )
		.setattr( TASK(STORE_LASTPOSITION) )
		.setattr( TASK(CLEAR_LASTPOSITION) )
		.setattr( TASK(STORE_POSITION_IN_SAVEPOSITION) )
		.setattr( TASK(STORE_BESTSOUND_IN_SAVEPOSITION) )
		.setattr( TASK(STORE_BESTSOUND_REACTORIGIN_IN_SAVEPOSITION) )
		.setattr( TASK(REACT_TO_COMBAT_SOUND) )
		.setattr( TASK(STORE_ENEMY_POSITION_IN_SAVEPOSITION) )
		.setattr( TASK(GET_PATH_TO_LASTPOSITION) )
		.setattr( TASK(GET_PATH_TO_SAVEPOSITION) )
		.setattr( TASK(GET_PATH_TO_SAVEPOSITION_LOS) )
		.setattr( TASK(GET_PATH_TO_RANDOM_NODE) )
		.setattr( TASK(GET_PATH_TO_BESTSOUND) )
		.setattr( TASK(GET_PATH_TO_BESTSCENT) )
		.setattr( TASK(RUN_PATH) )
		.setattr( TASK(WALK_PATH) )
		.setattr( TASK(WALK_PATH_TIMED) )
		.setattr( TASK(WALK_PATH_WITHIN_DIST) )
		.setattr( TASK(WALK_PATH_FOR_UNITS) )
		.setattr( TASK(RUN_PATH_FLEE) )
		.setattr( TASK(RUN_PATH_TIMED) )
		.setattr( TASK(RUN_PATH_FOR_UNITS) )
		.setattr( TASK(RUN_PATH_WITHIN_DIST) )
		.setattr( TASK(STRAFE_PATH) )
		.setattr( TASK(CLEAR_MOVE_WAIT) )
		.setattr( TASK(FACE_IDEAL) )
		.setattr( TASK(FACE_REASONABLE) )
		.setattr( TASK(FACE_PATH) )
		.setattr( TASK(FACE_PLAYER) )
		.setattr( TASK(FACE_ENEMY) )
		.setattr( TASK(FACE_HINTNODE) )
		.setattr( TASK(FACE_TARGET) )
		.setattr( TASK(FACE_LASTPOSITION) )
		.setattr( TASK(FACE_SAVEPOSITION) )
		.setattr( TASK(FACE_AWAY_FROM_SAVEPOSITION) )
		.setattr( TASK(RANGE_ATTACK1) )
		.setattr( TASK(RANGE_ATTACK2) )
		.setattr( TASK(MELEE_ATTACK1) )
		.setattr( TASK(MELEE_ATTACK2) )
		.setattr( TASK(RELOAD) )
		.setattr( TASK(SPECIAL_ATTACK1) )
		.setattr( TASK(SPECIAL_ATTACK2) )
		.setattr( TASK(FIND_HINTNODE) )
		.setattr( TASK(FIND_LOCK_HINTNODE) )
		.setattr( TASK(CLEAR_HINTNODE) )
		.setattr( TASK(LOCK_HINTNODE) )
		.setattr( TASK(SOUND_ANGRY) )
		.setattr( TASK(SOUND_DEATH) )
		.setattr( TASK(SOUND_IDLE) )
		.setattr( TASK(SOUND_WAKE) )
		.setattr( TASK(SOUND_PAIN) )
		.setattr( TASK(SOUND_DIE) )
		.setattr( TASK(SET_ACTIVITY) )
		.setattr( TASK(SET_SCHEDULE) )
		.setattr( TASK(SET_FAIL_SCHEDULE) )
		.setattr( TASK(SET_TOLERANCE_DISTANCE) )
		.setattr( TASK(SET_ROUTE_SEARCH_TIME) )
		.setattr( TASK(CLEAR_FAIL_SCHEDULE) )
		.setattr( TASK(FIND_COVER_FROM_BEST_SOUND) )
		.setattr( TASK(FIND_COVER_FROM_ENEMY) )
		.setattr( TASK(FIND_LATERAL_COVER_FROM_ENEMY) )
		.setattr( TASK(FIND_BACKAWAY_FROM_SAVEPOSITION) )
		.setattr( TASK(FIND_NODE_COVER_FROM_ENEMY) )
		.setattr( TASK(FIND_NEAR_NODE_COVER_FROM_ENEMY) )
		.setattr( TASK(FIND_FAR_NODE_COVER_FROM_ENEMY) )
		.setattr( TASK(FIND_COVER_FROM_ORIGIN) )
		.setattr( TASK(DIE) )
		.setattr( TASK(WAIT_RANDOM) )
		.setattr( TASK(WAIT_INDEFINITE) )
		.setattr( TASK(STOP_MOVING) )
		.setattr( TASK(TURN_LEFT) )
		.setattr( TASK(TURN_RIGHT) )
		.setattr( TASK(WAIT_FOR_MOVEMENT) )
		.setattr( TASK(WAIT_FOR_MOVEMENT_STEP) )
		.setattr( TASK(WAIT_UNTIL_NO_DANGER_SOUND) )
		.setattr( TASK(WEAPON_RUN_PATH) )
		.setattr( TASK(ITEM_RUN_PATH) )
		.setattr( TASK(USE_SMALL_HULL) )
		.setattr( TASK(FALL_TO_GROUND) )
		.setattr( TASK(WANDER) )
		.setattr( TASK(FREEZE) )
		.setattr( TASK(GATHER_CONDITIONS) )
		.setattr( TASK(IGNORE_OLD_ENEMIES) )
		.setattr( TASK(ADD_GESTURE_WAIT) )
		.setattr( TASK(ADD_GESTURE) )
		.setattr( "CLEAR_GOAL", task( "CLEAR_GOAL", (int) CNPC_GEBase::TASK_CLEAR_GOAL ) );

		enum_<AI_BaseTaskFailureCodes_t>("TaskFail")
			.value("NO_FAILURE", NO_TASK_FAILURE)
			.value("NO_TARGET", FAIL_NO_TARGET)
			.value("WEAPON_OWNED", FAIL_WEAPON_OWNED)
			.value("ITEM_NO_FIND", FAIL_ITEM_NO_FIND)
			.value("NO_HINT_NODE", FAIL_NO_HINT_NODE)
			.value("SCHEDULE_NOT_FOUND", FAIL_SCHEDULE_NOT_FOUND)
			.value("NO_ENEMY", FAIL_NO_ENEMY)
			.value("NO_BACKAWAY_NODE", FAIL_NO_BACKAWAY_NODE)
			.value("NO_COVER", FAIL_NO_COVER)
			.value("NO_FLANK", FAIL_NO_FLANK)
			.value("NO_SHOOT", FAIL_NO_SHOOT)
			.value("NO_ROUTE", FAIL_NO_ROUTE)
			.value("NO_ROUTE_GOAL", FAIL_NO_ROUTE_GOAL)
			.value("NO_ROUTE_BLOCKED", FAIL_NO_ROUTE_BLOCKED)
			.value("NO_ROUTE_ILLEGAL", FAIL_NO_ROUTE_ILLEGAL)
			.value("NO_WALK", FAIL_NO_WALK)
			.value("ALREADY_LOCKED", FAIL_ALREADY_LOCKED)
			.value("NO_SOUND", FAIL_NO_SOUND)
			.value("NO_SCENT", FAIL_NO_SCENT)
			.value("BAD_ACTIVITY", FAIL_BAD_ACTIVITY)
			.value("NO_GOAL", FAIL_NO_GOAL)
			.value("NO_PLAYER", FAIL_NO_PLAYER)
			.value("NO_REACHABLE_NODE", FAIL_NO_REACHABLE_NODE)
			.value("NO_AI_NETWORK", FAIL_NO_AI_NETWORK)
			.value("BAD_POSITION", FAIL_BAD_POSITION)
			.value("BAD_PATH_GOAL", FAIL_BAD_PATH_GOAL)
			.value("STUCK_ONTOP", FAIL_STUCK_ONTOP)
			.value("ITEM_TAKEN", FAIL_ITEM_TAKEN);
}

#define COND( name )	#name, cond( #name, (int) COND_##name )
class CDummyCondition {};

BOOST_PYTHON_MODULE(GEAiCond)
{
	using namespace boost::python;

	object ai( import( "GEAi" ) );
	object cond = ai.attr( "ICondition" );

	class_<CDummyCondition>( "Cond" )
		.setattr( COND(NONE) )
		.setattr( COND(IN_PVS) )
		.setattr( COND(IDLE_INTERRUPT) )
		.setattr( COND(LOW_PRIMARY_AMMO) )
		.setattr( COND(LOW_WEAPON_WEIGHT) )
		.setattr( COND(NO_PRIMARY_AMMO) )
		.setattr( COND(NO_SECONDARY_AMMO) )
		.setattr( COND(NO_WEAPON) )
		.setattr( COND(SEE_HATE) )
		.setattr( COND(SEE_FEAR) )
		.setattr( COND(SEE_DISLIKE) )
		.setattr( COND(SEE_ENEMY) )
		.setattr( COND(LOST_ENEMY) )
		.setattr( COND(ENEMY_WENT_NULL) )
		.setattr( COND(ENEMY_OCCLUDED) )
		.setattr( COND(TARGET_OCCLUDED) )
		.setattr( COND(HAVE_ENEMY_LOS) )
		.setattr( COND(HAVE_TARGET_LOS) )
		.setattr( COND(LIGHT_DAMAGE) )
		.setattr( COND(HEAVY_DAMAGE) )
		.setattr( COND(PHYSICS_DAMAGE) )
		.setattr( COND(REPEATED_DAMAGE) )
		.setattr( COND(CAN_RANGE_ATTACK1) )
		.setattr( COND(CAN_RANGE_ATTACK2) )
		.setattr( COND(CAN_MELEE_ATTACK1) )
		.setattr( COND(CAN_MELEE_ATTACK2) )
		.setattr( COND(PROVOKED) )
		.setattr( COND(NEW_ENEMY) )
		.setattr( COND(ENEMY_TOO_FAR) )
		.setattr( COND(ENEMY_FACING_ME) )
		.setattr( COND(BEHIND_ENEMY) )
		.setattr( COND(ENEMY_DEAD) )
		.setattr( COND(ENEMY_UNREACHABLE) )
		.setattr( COND(SEE_PLAYER) )
		.setattr( COND(LOST_PLAYER) )
		.setattr( COND(SEE_NEMESIS) )
		.setattr( COND(TASK_FAILED) )
		.setattr( COND(SCHEDULE_DONE) )
		.setattr( COND(SMELL) )
		.setattr( COND(TOO_CLOSE_TO_ATTACK) )
		.setattr( COND(TOO_FAR_TO_ATTACK) )
		.setattr( COND(NOT_FACING_ATTACK) )
		.setattr( COND(WEAPON_HAS_LOS) )
		.setattr( COND(WEAPON_BLOCKED_BY_FRIEND) )
		.setattr( COND(WEAPON_PLAYER_IN_SPREAD) )
		.setattr( COND(WEAPON_PLAYER_NEAR_TARGET) )
		.setattr( COND(WEAPON_SIGHT_OCCLUDED) )
		.setattr( COND(BETTER_WEAPON_AVAILABLE) )
		.setattr( COND(HEALTH_ITEM_AVAILABLE) )
		.setattr( COND(GIVE_WAY) )
		.setattr( COND(WAY_CLEAR) )
		.setattr( COND(HEAR_DANGER) )
		.setattr( COND(HEAR_THUMPER) )
		.setattr( COND(HEAR_BUGBAIT) )
		.setattr( COND(HEAR_COMBAT) )
		.setattr( COND(HEAR_WORLD) )
		.setattr( COND(HEAR_PLAYER) )
		.setattr( COND(HEAR_BULLET_IMPACT) )
		.setattr( COND(HEAR_PHYSICS_DANGER) )
		.setattr( COND(HEAR_MOVE_AWAY) )
		.setattr( COND(HEAR_SPOOKY) )
		.setattr( COND(NO_HEAR_DANGER) )
		.setattr( COND(FLOATING_OFF_GROUND) )
		.setattr( COND(MOBBED_BY_ENEMIES) )
		.setattr( COND(RECEIVED_ORDERS) )
		.setattr( COND(PLAYER_ADDED_TO_SQUAD) )
		.setattr( COND(PLAYER_REMOVED_FROM_SQUAD) )
		.setattr( COND(PLAYER_PUSHING) )
		.setattr( COND(NPC_FREEZE) )
		.setattr( COND(NPC_UNFREEZE) );
}
