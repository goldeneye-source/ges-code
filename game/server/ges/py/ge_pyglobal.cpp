///////////// Copyright © 2009 LodleNet. All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_pyglobal.cpp
//   Description :
//      [TODO: Write the purpose of ge_pyglobal.cpp.]
//
//   Created On: 8/24/2009 11:01:06 PM
//   Created By: Mark Chandler <mailto:mark@moddb.com>
////////////////////////////////////////////////////////////////////////////

#include "ge_pyprecom.h"
#include "ge_shareddefs.h"
#include "ge_tokenmanager.h"
#include "gemp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BOOST_PYTHON_MODULE(GEGlobal)
{
	using namespace boost::python;

	object self = scope();

	// API Version Levels
	self.attr("API_VERSION_1_0_0") = "1.0.0";
	self.attr("API_VERSION_1_1_0") = "1.1.0";
	self.attr("API_VERSION_1_1_1") = "1.1.1";
	self.attr("API_VERSION_1_2_0") = "1.2.0";

	// Current version level of AI and GamePlay
	self.attr("API_AI_VERSION") = self.attr("API_VERSION_1_1_0");
	self.attr("API_GP_VERSION") = self.attr("API_VERSION_1_2_0");

	self.attr("SAY_COMMAND1") = "!voodoo";
	self.attr("SAY_COMMAND2") = "!gesrocks";

	enum_< GES_EVENTHOOK >("EventHooks")
		.value("GP_THINK",				FUNC_GP_THINK)
		.value("GP_PLAYERCONNECT",		FUNC_GP_PLAYERCONNECT)
		.value("GP_PLAYERDISCONNECT",	FUNC_GP_PLAYERDISCONNECT)
		.value("GP_PLAYERSPAWN",		FUNC_GP_PLAYERSPAWN)
		.value("GP_PLAYEROBSERVER",		FUNC_GP_PLAYEROBSERVER)
		.value("GP_PLAYERKILLED",		FUNC_GP_PLAYERKILLED)
		.value("GP_PLAYERTEAM",			FUNC_GP_PLAYERTEAM)
		.value("GP_ROUNDBEGIN",			FUNC_GP_ROUNDBEGIN)
		.value("GP_ROUNDEND",			FUNC_GP_ROUNDEND)
		.value("GP_WEAPONSPAWNED",		FUNC_GP_WEAPONSPAWNED)
		.value("GP_WEAPONREMOVED",		FUNC_GP_WEAPONREMOVED)
		.value("GP_ARMORSPAWNED",		FUNC_GP_ARMORSPAWNED)
		.value("GP_ARMORREMOVED",		FUNC_GP_ARMORREMOVED)
		.value("GP_AMMOSPAWNED",		FUNC_GP_AMMOSPAWNED)
		.value("GP_AMMOREMOVED",		FUNC_GP_AMMOREMOVED)
		
		.value("AI_ONSPAWN",			FUNC_AI_ONSPAWN)
		.value("AI_ONLOOKED",			FUNC_AI_ONLOOKED)
		.value("AI_ONLISTENED",			FUNC_AI_ONLISTENED)
		.value("AI_PICKUPITEM",			FUNC_AI_PICKUPITEM)
		.value("AI_GATHERCONDITIONS",	FUNC_AI_GATHERCONDITIONS)
		.value("AI_SELECTSCHEDULE",		FUNC_AI_SELECTSCHEDULE)
		.value("AI_SETDIFFICULTY",		FUNC_AI_SETDIFFICULTY);

	// Actual GES Teams
	self.attr("TEAM_NONE")		= TEAM_UNASSIGNED;
	self.attr("TEAM_SPECTATOR")	= TEAM_SPECTATOR;
	self.attr("TEAM_MI6")		= TEAM_MI6;
	self.attr("TEAM_JANUS")		= TEAM_JANUS;

	// Meta-teams for use with creating message filters
	self.attr("TEAM_OBS")			= TEAM_OBS;
	self.attr("TEAM_NO_OBS")		= TEAM_NO_OBS;
	self.attr("TEAM_MI6_OBS")		= TEAM_MI6_OBS;
	self.attr("TEAM_MI6_NO_OBS")	= TEAM_MI6_NO_OBS;
	self.attr("TEAM_JANUS_OBS")		= TEAM_JANUS_OBS;
	self.attr("TEAM_JANUS_NO_OBS")	= TEAM_JANUS_NO_OBS;

	enum_< GES_TEAMPLAY  >("Teamplay")
		.value("TEAMPLAY_NONE",	  TEAMPLAY_NOT)
		.value("TEAMPLAY_ALWAYS", TEAMPLAY_ONLY)
		.value("TEAMPLAY_TOGGLE", TEAMPLAY_TOGGLE)
		.export_values();

	enum_< CGETokenManager::LocationFlag  >("LocationFlag")
		.value("SPAWN_AMMO",		CGETokenManager::LOC_AMMO)
		.value("SPAWN_WEAPON",		CGETokenManager::LOC_WEAPON)
		.value("SPAWN_TOKEN",		CGETokenManager::LOC_TOKEN)
		.value("SPAWN_CAPAREA",		CGETokenManager::LOC_CAPAREA)
		.value("SPAWN_PLAYER",		CGETokenManager::LOC_PLAYERSPAWN)
		.value("SPAWN_MYTEAM",		CGETokenManager::LOC_MYTEAM)
		.value("SPAWN_OTHERTEAM",	CGETokenManager::LOC_OTHERTEAM)
		.value("SPAWN_SPECIALONLY", CGETokenManager::LOC_SPECIALONLY)
		.value("SPAWN_ALWAYSNEW",	CGETokenManager::LOC_ALWAYSNEW)
		.value("SPAWN_FIXEDMATCH",	CGETokenManager::LOC_FIXEDMATCH)
		.export_values();

	enum_< GES_WEAPONS >("Weapons")
		.value("WEAPON_NONE", WEAPON_NONE)
		.value("WEAPON_PP7", WEAPON_PP7)
		.value("WEAPON_PP7_SILENCED", WEAPON_PP7_SILENCED)
		.value("WEAPON_DD44", WEAPON_DD44)
		.value("WEAPON_SILVERPP7", WEAPON_SILVERPP7)
		.value("WEAPON_GOLDENPP7", WEAPON_GOLDENPP7)
		.value("WEAPON_COUGAR_MAGNUM", WEAPON_COUGAR_MAGNUM)
		.value("WEAPON_GOLDENGUN", WEAPON_GOLDENGUN)
		.value("WEAPON_SHOTGUN", WEAPON_SHOTGUN)
		.value("WEAPON_AUTO_SHOTGUN", WEAPON_AUTO_SHOTGUN)
		.value("WEAPON_KF7", WEAPON_KF7)
		.value("WEAPON_KLOBB", WEAPON_KLOBB)
		.value("WEAPON_ZMG", WEAPON_ZMG)
		.value("WEAPON_D5K", WEAPON_D5K)
		.value("WEAPON_D5K_SILENCED", WEAPON_D5K_SILENCED)
		.value("WEAPON_RCP90", WEAPON_RCP90)
		.value("WEAPON_AR33", WEAPON_AR33)
		.value("WEAPON_PHANTOM", WEAPON_PHANTOM)
		.value("WEAPON_SNIPER_RIFLE", WEAPON_SNIPER_RIFLE)
		.value("WEAPON_KNIFE_THROWING", WEAPON_KNIFE_THROWING)
		.value("WEAPON_GRENADE_LAUNCHER", WEAPON_GRENADE_LAUNCHER)
		.value("WEAPON_ROCKET_LAUNCHER", WEAPON_ROCKET_LAUNCHER)
		.value("WEAPON_MOONRAKER", WEAPON_MOONRAKER)
		.value("WEAPON_TIMEDMINE", WEAPON_TIMEDMINE)
		.value("WEAPON_REMOTEMINE", WEAPON_REMOTEMINE)
		.value("WEAPON_PROXIMITYMINE", WEAPON_PROXIMITYMINE)
		.value("WEAPON_GRENADE", WEAPON_GRENADE)
		.value("WEAPON_SLAPPERS", WEAPON_SLAPPERS)
		.value("WEAPON_KNIFE", WEAPON_KNIFE)
		.value("WEAPON_MAX", WEAPON_MAX)
		.export_values();

	self.attr("AMMO_9MM")			= AMMO_9MM;
	self.attr("AMMO_RIFLE")		= AMMO_RIFLE;
	self.attr("AMMO_BUCKSHOT")	= AMMO_BUCKSHOT;
	self.attr("AMMO_MAGNUM")		= AMMO_MAGNUM;
	self.attr("AMMO_GOLDENGUN")	= AMMO_GOLDENGUN;
	self.attr("AMMO_PROXIMITYMINE") = AMMO_PROXIMITYMINE;
	self.attr("AMMO_TIMEDMINE")	= AMMO_TIMEDMINE;
	self.attr("AMMO_REMOTEMINE")	= AMMO_REMOTEMINE;
	self.attr("AMMO_TKNIFE")		= AMMO_TKNIFE;
	self.attr("AMMO_GRENADE")		= AMMO_GRENADE;
	self.attr("AMMO_ROCKET")		= AMMO_ROCKET;
	self.attr("AMMO_SHELL")		= AMMO_SHELL;

	self.attr("HUD_PRINTNOTIFY")	= HUD_PRINTNOTIFY;
	self.attr("HUD_PRINTCONSOLE") = HUD_PRINTCONSOLE;
	self.attr("HUD_PRINTTALK")	= HUD_PRINTTALK;
	self.attr("HUD_PRINTCENTER")	= HUD_PRINTCENTER;

	self.attr("GE_MAX_ARMOR")		= MAX_ARMOR;
	self.attr("GE_MAX_HEALTH")	= MAX_HEALTH;

	self.attr("HUDPB_TITLEONLY")	= 0;
	self.attr("HUDPB_SHOWBAR")	= 0x1;
	self.attr("HUDPB_SHOWVALUE")	= 0x2;
	self.attr("HUDPB_VERTICAL")	= 0x4;

	enum_< RadarType >("RadarType")
		.value("RADAR_TYPE_DEFAULT", RADAR_TYPE_DEFAULT)
		.value("RADAR_TYPE_PLAYER", RADAR_TYPE_PLAYER)
		.value("RADAR_TYPE_TOKEN", RADAR_TYPE_TOKEN)
		.value("RADAR_TYPE_OBJECTIVE", RADAR_TYPE_OBJECTIVE)
		.export_values();

	enum_< GEScoreBoardColor >("ScoreBoardColors")
		.value("SB_COLOR_NORMAL", SB_COLOR_NORMAL)
		.value("SB_COLOR_GOLD", SB_COLOR_GOLD)
		.value("SB_COLOR_WHITE", SB_COLOR_WHITE)
		.value("SB_COLOR_ELIMINATED", SB_COLOR_ELIMINATED)
		.export_values();

	self.attr("RADAR_ALWAYSVIS") = true;
	self.attr("RADAR_NORMVIS") = false;

	enum_< SOUNDPRIORITY >("GESoundPriority")
		.value("SOUND_PRIORITY_VERY_LOW", SOUND_PRIORITY_VERY_LOW)
		.value("SOUND_PRIORITY_LOW", SOUND_PRIORITY_LOW)
		.value("SOUND_PRIORITY_NORMAL", SOUND_PRIORITY_NORMAL)
		.value("SOUND_PRIORITY_HIGH", SOUND_PRIORITY_HIGH)
		.value("SOUND_PRIORITY_VERY_HIGH", SOUND_PRIORITY_VERY_HIGH)
		.value("SOUND_PRIORITY_HIGHEST", SOUND_PRIORITY_HIGHEST)
		.export_values();

	enum_< SOUNDTYPES >("GESoundTypes")
		.value("SOUND_NONE", SOUND_NONE)
		.value("SOUND_COMBAT", SOUND_COMBAT)
		.value("SOUND_WORLD", SOUND_WORLD)
		.value("SOUND_PLAYER", SOUND_PLAYER)
		.value("SOUND_DANGER", SOUND_DANGER)
		.value("SOUND_BULLET_IMPACT", SOUND_BULLET_IMPACT)
		.value("SOUND_CARCASS", SOUND_CARCASS)
		.value("SOUND_MEAT", SOUND_MEAT)
		.value("SOUND_GARBAGE", SOUND_GARBAGE)
		.value("SOUND_THUMPER",  SOUND_THUMPER)
		.value("SOUND_BUGBAIT", SOUND_BUGBAIT)
		.value("SOUND_PHYSICS_DANGER", SOUND_PHYSICS_DANGER)
		.value("SOUND_DANGER_SNIPERONLY", SOUND_DANGER_SNIPERONLY)
		.value("SOUND_MOVE_AWAY", SOUND_MOVE_AWAY)
		.value("SOUND_PLAYER_VEHICLE", SOUND_PLAYER_VEHICLE)
		.value("SOUND_READINESS_LOW", SOUND_READINESS_LOW)
		.value("SOUND_READINESS_MEDIUM", SOUND_READINESS_MEDIUM)
		.value("SOUND_READINESS_HIGH", SOUND_READINESS_HIGH)
		.export_values();
}
