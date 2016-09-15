///////////// Copyright © 2009 LodleNet. All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_pygamerules.cpp
//   Description :
//      [TODO: Write the purpose of ge_pygamerules.cpp.]
//
//   Created On: 9/1/2009 10:19:15 PM
//   Created By: Mark Chandler <mailto:mark@moddb.com>
////////////////////////////////////////////////////////////////////////////
#include "ge_pyprecom.h"
#include "ge_pyfuncs.h"
#include "boost/python/raw_function.hpp"

#include "ge_shareddefs.h"
#include "ge_radarresource.h"
#include "ge_generictoken.h"
#include "ge_loadoutmanager.h"
#include "ge_tokenmanager.h"
#include "ge_spawner.h"
#include "ge_armorvest.h"
#include "gemp_gamerules.h"
#include "ge_mapmanager.h"
#include "gemp_player.h"
#include "ge_gameplayresource.h"
#include "team.h"
#include "weapon_parse.h"
#include "ge_weapon_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

float pyGetMatchTimeLeft()
{
	return GEMPRules()->GetMatchTimeRemaining();
}

float pyGetRoundTimeLeft()
{
	return GEMPRules()->GetRoundTimeRemaining();
}

void pySetRoundTimeLeft( int newtime, bool announce )
{
	return GEMPRules()->SetRoundTimer(newtime, announce);
}

void pyAddToRoundTimeLeft( int newtime, bool announce )
{
	return GEMPRules()->AddToRoundTimer( newtime, announce );
}

bool pyIsIntermission()
{
	return GEMPRules()->IsIntermission();
}

bool pyIsGameOver()
{
	return GEGameplay()->IsInFinalIntermission();
}

bool pyIsTeamplay()
{
	return GEMPRules()->IsTeamplay();
}

void pyEndMatch()
{
	// NOTE: This ignores Scenario.CanMatchEnd()!
	if ( GEGameplay() && !GEGameplay()->IsGameOver() )
		GEGameplay()->EndMatch();
}

void pyEndRound( bool bScoreRound = true, bool bChangeWeapons = true )
{
	// Don't end if we are not in a round
	if ( !GEGameplay() || !GEGameplay()->IsInRound() )
		return;

	if ( !bChangeWeapons )
		GEMPRules()->GetLoadoutManager()->KeepLoadoutOnNextChange();

	// NOTE: This ignores Scenario.CanRoundEnd()!
	GEGameplay()->EndRound( bScoreRound );
}


void pyLockRound()
{
	if ( GEGameplay() )
		GEGameplay()->SetRoundLocked( true );
}

void pyUnlockRound()
{
	if ( GEGameplay() )
		GEGameplay()->SetRoundLocked( false );
}

bool pyIsRoundLocked()
{
	return GEGameplay() && GEGameplay()->IsRoundLocked();
}

void pyToggleRoundTimer( bool state )
{
	GEMPRules()->SetRoundTimerEnabled( state );
}

void pySetSpawnInvulnTime(float duration, bool canbreak)
{
	GEMPRules()->SetSpawnInvulnCanBreak(canbreak);
	GEMPRules()->SetSpawnInvulnInterval(duration);
}


CTeam* pyGetTeam(int index)
{
	return GetGlobalTeam(index);
}

CGERadarResource* pyGetRadar()
{
	return g_pRadarResource;
}

CGETokenManager *pyGetTokenMgr()
{
	return GEMPRules()->GetTokenManager();
}


void pyDisableWeaponSpawns()
{
	GEMPRules()->SetWeaponSpawnState( false );
	GEMPRules()->RemoveWeaponsFromWorld();
}

void pyDisableAmmoSpawns()
{
	GEMPRules()->SetAmmoSpawnState(false);

	const CUtlVector<EHANDLE> *vSpawners = GERules()->GetSpawnersOfType( SPAWN_AMMO );
	if ( vSpawners )
	{
		CGESpawner *pSpawner = NULL;
		inputdata_t nullData;
		for ( int i=0; i < vSpawners->Count(); i++ )
		{
			pSpawner = (CGESpawner*) vSpawners->Element(i).Get();
			if ( pSpawner )
				pSpawner->SetEnabled( false );
		}
	}
}

void pyDisableArmorSpawns()
{
	// Disable armor spawns in the gamerules
	GEMPRules()->SetArmorSpawnState(false);

	CBaseEntity *pArmor = gEntList.FindEntityByClassname( NULL, "item_armorvest*" );
	while (pArmor)
	{
		// Respawn the armor, but it won't materialize
		pArmor->Respawn();
		pArmor = gEntList.FindEntityByClassname( pArmor, "item_armorvest*" );
	}
}

void pyEnableArmorSpawns()
{
	// Disable armor spawns in the gamerules
	GEMPRules()->SetArmorSpawnState(true);

	CBaseEntity *pArmor = gEntList.FindEntityByClassname(NULL, "item_armorvest*");
	while (pArmor)
	{
		// Respawn the armors
		pArmor->Respawn();
		((CGEArmorVest*)pArmor)->Materialize();
		pArmor = gEntList.FindEntityByClassname(pArmor, "item_armorvest*");
	}
}

void pyStagnateArmorSpawns()
{
	// Disable armor spawns in the gamerules, but keep the armor already on the ground.
	GEMPRules()->SetArmorSpawnState(false);
}

void pyEnableInfiniteAmmo()
{
	GEMPRules()->SetGamemodeInfAmmoState(true);
}

void pyDisableInfiniteAmmo()
{
	GEMPRules()->SetGamemodeInfAmmoState(false);
}

void pyEnableSuperfluousAreas()
{
	GEMPRules()->SetSuperfluousAreasState(true);
}

void pyDisableSuperfluousAreas()
{
	GEMPRules()->SetSuperfluousAreasState(false);
}

int pyGetWeaponInSlot( int iSlot )
{
	if ( !GEMPRules() )
		return WEAPON_NONE;

	return GEMPRules()->GetLoadoutManager()->CurrLoadout()->GetWeapon( iSlot );
}

bp::list pyGetWeaponLoadout( const char *szName = NULL )
{
	const CGELoadout *loadout = NULL;
	bp::list weapList;

	if ( szName )
		loadout = GEMPRules()->GetLoadoutManager()->GetLoadout( szName );
	else
		loadout = GEMPRules()->GetLoadoutManager()->CurrLoadout();

	if ( !loadout )
		return weapList;

	for ( int i=0; i < MAX_WEAPON_SPAWN_SLOTS; i++ )
		weapList.append( loadout->GetWeapon(i) );

	return weapList;
}

void pySetTeamWinner( bp::object team )
{
	bp::extract<CTeam*> to_team( team );
	int team_id = TEAM_INVALID;

	if ( to_team.check() )
		team_id = to_team()->GetTeamNumber();
	else
		team_id = bp::extract<int>( team );

	GEMPRules()->SetRoundTeamWinner( team_id );
}

void pyResetAllPlayerDeaths()
{
	// Reset all the player's deaths
	FOR_EACH_MPPLAYER( pPlayer )
		pPlayer->ResetDeathCount();
	END_OF_PLAYER_LOOP()
}

void pyResetAllPlayersScores()
{
	GEMPRules()->ResetPlayerScores();
}

void pyResetAllTeamsScores()
{
	GEMPRules()->ResetTeamScores();
}

void pyEnableStandardScoring()
{
	GEMPRules()->SetScoreboardMode(0);
}

void pyEnableTimeBasedScoring()
{
	GEMPRules()->SetScoreboardMode(1);
}

void pySetPlayerWinner(CGEMPPlayer *player)
{
	if (!player)
		return;

	GEMPRules()->SetRoundWinner( player->entindex() );
}

void pySetAllowTeamSpawns(bool state)
{
	GEMPRules()->SetTeamSpawn( state );
}

int pyGetNumAlivePlayers()
{
	return GEMPRules()->GetNumAlivePlayers();
}

int pyGetNumActivePlayers()
{
	return GEMPRules()->GetNumActivePlayers();
}

int pyGetNumInRoundPlayers()
{
	return GEMPRules()->GetNumInRoundPlayers();
}

int pyGetMapMaxPlayers()
{
	if (!GEMPRules()->GetMapManager())
	{
		Warning("Tried to call map manager before it existed!");
		return -1;
	}


	MapSelectionData *curMapData = GEMPRules()->GetMapManager()->GetCurrentMapSelectionData();

	if (curMapData)
		return curMapData->maxplayers;
	else
		return -1;
}

int pyGetMapMinPlayers()
{
	if (!GEMPRules()->GetMapManager())
	{
		Warning("Tried to call map manager before it existed!");
		return -1;
	}


	MapSelectionData *curMapData = GEMPRules()->GetMapManager()->GetCurrentMapSelectionData();

	if (curMapData)
		return curMapData->minplayers;
	else
		return -1;
}

int pyGetNumActiveTeamPlayers( bp::object team_obj )
{
	bp::extract<CTeam*> to_team( team_obj );

	int count = 0;
	int team_num = TEAM_UNASSIGNED;

	if ( to_team.check() )
		team_num = to_team()->GetTeamNumber();
	else
		team_num = bp::extract<int>( team_obj );

	FOR_EACH_MPPLAYER( pPlayer )
		if ( pPlayer->GetTeamNumber() == team_num && pPlayer->IsActive() )
			count++;
	END_OF_PLAYER_LOOP()

		return count;
}

int pyGetNumInRoundTeamPlayers( bp::object team_obj )
{
	bp::extract<CTeam*> to_team( team_obj );

	int count = 0;
	int team_num = TEAM_UNASSIGNED;

	if ( to_team.check() )
		team_num = to_team()->GetTeamNumber();
	else
		team_num = bp::extract<int>( team_obj );

	FOR_EACH_MPPLAYER( pPlayer )

		if ( pPlayer->GetTeamNumber() == team_num && pPlayer->IsInRound() )
			count++;

	END_OF_PLAYER_LOOP()

		return count;
}

void pySetExcludedCharacters( const char *str_list )
{
	if ( !str_list )
		str_list = "";

	g_pGameplayResource->SetCharacterExclusion( str_list );
}

bp::list pyGetTokens( CGETokenManager *pTM, const char *szClassName )
{
	CUtlVector<EHANDLE> tokens;
	pTM->FindTokens( szClassName, tokens );

	bp::list pyTokens;
	for ( int i=0; i < tokens.Count(); i++ )
		pyTokens.append( bp::ptr( (CGEWeapon*) tokens[i].Get() ) );

	return pyTokens;
}

bp::object pySetupToken( bp::tuple args, bp::dict kw )
{
	CGETokenManager *pTM = bp::extract<CGETokenManager*>(args[0]);

	const char *name = bp::extract<char*>(args[1]);
	CGETokenDef *pDef = pTM->GetTokenDefForEdit( name );
	if ( !pDef )
		return bp::object();

	// Custom Token Settings
	GE_Extract(kw, "view_model", pDef->szModelName[0], MAX_MODEL_PATH );
	GE_Extract(kw, "world_model", pDef->szModelName[1], MAX_MODEL_PATH);
	GE_Extract(kw, "print_name", pDef->szPrintName, MAX_ENTITY_NAME );

	pDef->flDmgMod       = GE_Extract<float>(kw, "damage_mod",	pDef->flDmgMod);
	pDef->nSkin          = GE_Extract<int>(kw, "skin",			pDef->nSkin);
	pDef->bAllowSwitch   = GE_Extract<bool>(kw,	"allow_switch", pDef->bAllowSwitch);

	// Standard Token Settings
	pDef->iLimit         = GE_Extract<int>(kw, "limit",			pDef->iLimit);
	pDef->iLocations     = GE_Extract<int>(kw, "location",		pDef->iLocations);
	pDef->glowColor      = GE_Extract<Color>(kw, "glow_color",	pDef->glowColor);
	pDef->glowDist       = GE_Extract<float>(kw, "glow_dist",	pDef->glowDist);
	pDef->flRespawnDelay = GE_Extract<float>(kw, "respawn_delay", pDef->flRespawnDelay);
	pDef->iTeam          = GE_Extract<int>(kw, "team",			pDef->iTeam);

	return bp::object();
}

bp::object pySetupCaptureArea( bp::tuple args, bp::dict kw )
{
	CGETokenManager *pTM = bp::extract<CGETokenManager*>(args[0]);

	const char *name = bp::extract<char*>(args[1]);
	CGECaptureAreaDef *pDef = pTM->GetCaptureAreaDefForEdit( name );
	if ( !pDef )
		return bp::object();

	GE_Extract(kw, "model", pDef->szModelName, MAX_MODEL_PATH );
	GE_Extract(kw, "rqd_token", pDef->rqdToken, MAX_ENTITY_NAME );

	pDef->iLimit     = GE_Extract<int>(kw, "limit", pDef->iLimit);
	pDef->iLocations = GE_Extract<int>(kw, "location", pDef->iLocations);
	pDef->rqdTeam    = GE_Extract<int>(kw, "rqd_team", pDef->rqdTeam);
	pDef->nSkin      = GE_Extract<int>(kw, "skin", pDef->nSkin);
	pDef->glowColor  = GE_Extract<Color>(kw, "glow_color",	pDef->glowColor);
	pDef->glowDist   = GE_Extract<float>(kw, "glow_dist",	pDef->glowDist);
	pDef->fRadius    = GE_Extract<float>(kw, "radius", pDef->fRadius);
	pDef->fSpread    = GE_Extract<float>(kw, "spread", pDef->fSpread);

	return bp::object();
}

extern ConVar ge_radar_range;
bp::list pyListContactsNear( const CGERadarResource *radar, const Vector &origin )
{
	const float epsilon = 10.0f;
	float range_sq = ge_radar_range.GetFloat() * ge_radar_range.GetFloat();
	bp::list contacts;
	for ( int i=0; i < MAX_NET_RADAR_ENTS; i++ )
	{
		if ( g_pRadarResource->m_hEnt.Get(i) && g_pRadarResource->m_iState.Get(i) == RADAR_STATE_DRAW )
		{
			float dist_sq = origin.DistToSqr( g_pRadarResource->m_vOrigin.Get(i) );
			if ( dist_sq > epsilon && (g_pRadarResource->m_bAllVisible.Get(i) || dist_sq < range_sq) )
			{
				bp::dict c;
				c["ent_handle"]	  = g_pRadarResource->m_hEnt.Get(i);
				c["classname"]	  = g_pRadarResource->m_hEnt.Get(i)->GetClassname();
				c["origin"]       = g_pRadarResource->m_vOrigin.Get(i);
				c["range"]		  = sqrt( dist_sq );
				c["team"]         = g_pRadarResource->m_iEntTeam.Get(i);
				c["type"]         = g_pRadarResource->m_iType.Get(i);
				c["is_objective"] = g_pRadarResource->m_ObjStatus.Get(i);

				contacts.append( c );
			}
		}		
	}

	return contacts;
}

BOOST_PYTHON_FUNCTION_OVERLOADS(pyEndRound_overloads, pyEndRound, 0, 2);
BOOST_PYTHON_FUNCTION_OVERLOADS(pyGetWeaponLoadout_overloads, pyGetWeaponLoadout, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(CGETokenManager_RemoveTokenEnt_overloads, CGETokenManager::RemoveTokenEnt, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(CGETokenManager_SetGlobalAmmo_overloads, CGETokenManager::SetGlobalAmmo, 1, 2);

BOOST_PYTHON_MODULE(GEMPGameRules)
{
	using namespace boost::python;

	def("EndMatch", pyEndMatch);
	def("EndRound", pyEndRound, pyEndRound_overloads());

	def("GetTeam", pyGetTeam, return_value_policy<reference_existing_object>());
	def("GetRadar", pyGetRadar, return_value_policy<reference_existing_object>());
	def("GetTokenMgr", pyGetTokenMgr, return_value_policy<reference_existing_object>());

	def("LockRound", pyLockRound);
	def("UnlockRound", pyUnlockRound);
	def("IsRoundLocked", pyIsRoundLocked);
	def("AllowRoundTimer", pyToggleRoundTimer);

	def("SetSpawnInvulnTime", pySetSpawnInvulnTime);

	def("DisableWeaponSpawns", pyDisableWeaponSpawns);
	def("DisableAmmoSpawns", pyDisableAmmoSpawns);
	def("DisableArmorSpawns", pyDisableArmorSpawns);
	def("EnableArmorSpawns", pyEnableArmorSpawns);
	def("StagnateArmorSpawns", pyStagnateArmorSpawns);

	def("EnableInfiniteAmmo", pyEnableInfiniteAmmo);
	def("DisableInfiniteAmmo", pyDisableInfiniteAmmo);

	def("GetMapMaxPlayers", pyGetMapMaxPlayers);
	def("GetMapMinPlayers", pyGetMapMinPlayers);

	def("EnableSuperfluousAreas", pyEnableSuperfluousAreas);
	def("DisableSuperfluousAreas", pyDisableSuperfluousAreas);

	def("GetWeaponInSlot", pyGetWeaponInSlot);
	def("GetWeaponLoadout", pyGetWeaponLoadout, pyGetWeaponLoadout_overloads());

	def("SetPlayerWinner", pySetPlayerWinner);
	def("SetTeamWinner", pySetTeamWinner);

	def("EnableTimeBasedScoring", pyEnableTimeBasedScoring);
	def("EnableStandardScoring", pyEnableStandardScoring);

	def("GetMapTimeLeft", pyGetMatchTimeLeft); // DEPRECATED
	def("GetMatchTimeLeft", pyGetMatchTimeLeft);
	def("GetRoundTimeLeft", pyGetRoundTimeLeft);
	def("SetRoundTimeLeft", pySetRoundTimeLeft, ("newtime", arg("announce") = false));
	def("AddToRoundTimeLeft", pyAddToRoundTimeLeft, ("newtime", arg("announce") = false));
	def("IsIntermission", pyIsIntermission);
	def("IsGameOver", pyIsGameOver);
	def("IsTeamplay", pyIsTeamplay);

	def("GetNumActivePlayers", pyGetNumActivePlayers);
	def("GetNumActiveTeamPlayers", pyGetNumActiveTeamPlayers);
	def("GetNumInRoundPlayers", pyGetNumInRoundPlayers);
	def("GetNumInRoundTeamPlayers", pyGetNumInRoundTeamPlayers);
	def("GetNumAlivePlayers", pyGetNumAlivePlayers);

	def("SetAllowTeamSpawns", pySetAllowTeamSpawns);
	def("ResetAllPlayerDeaths", pyResetAllPlayerDeaths);
	def("ResetAllPlayersScores", pyResetAllPlayersScores);
	def("ResetAllTeamsScores", pyResetAllTeamsScores);

	def("SetExcludedCharacters", pySetExcludedCharacters);

	class_<CTakeDamageInfo>("CTakeDamageInfo")
		.def("GetInflictor", &CTakeDamageInfo::GetInflictor, return_value_policy<reference_existing_object>())
		.def("GetAttacker", &CTakeDamageInfo::GetAttacker, return_value_policy<reference_existing_object>())
		.def("GetWeapon", &CTakeDamageInfo::GetWeapon, return_value_policy<reference_existing_object>())
		.def("GetDamage", &CTakeDamageInfo::GetDamage)
		.def("GetDamageType", &CTakeDamageInfo::GetDamageType)
		.def("GetAmmoName", &CTakeDamageInfo::GetAmmoName);

	class_<CTeam, boost::noncopyable>("CTeam", no_init)
		.def("__str__", &CTeam::GetName)
		.def("GetRoundScore", &CTeam::GetRoundScore)
		.def("SetRoundScore", &CTeam::SetRoundScore)
		.def("IncrementRoundScore", &CTeam::AddRoundScore) // Deprecated
		.def("AddRoundScore", &CTeam::AddRoundScore)
		.def("GetMatchScore", &CTeam::GetMatchScore)
		.def("SetMatchScore", &CTeam::SetMatchScore)
		.def("IncrementMatchScore", &CTeam::AddMatchScore) // Deprecated
		.def("AddMatchScore", &CTeam::AddMatchScore)
		.def("GetNumPlayers", &CTeam::GetNumPlayers)
		.def("GetName", &CTeam::GetName)
		.def("GetTeamNumber", &CTeam::GetTeamNumber);

	class_<CGERadarResource, boost::noncopyable>("CGERadar", no_init)
		.def("SetForceRadar", &CGERadarResource::SetForceRadar)
		.def("SetPlayerRangeMod", &CGERadarResource::SetRangeModifier)
		.def("AddRadarContact", &CGERadarResource::AddRadarContact, ("entity", arg("type")=RADAR_TYPE_DEFAULT, arg("always_visible")=false, 
		arg("icon")="", arg("color")=Color()))
		.def("DropRadarContact", &CGERadarResource::DropRadarContact)
		.def("IsRadarContact", &CGERadarResource::IsRadarContact)
		.def("SetupObjective", &CGERadarResource::SetupObjective, ("entity", arg("team_filter")=0, arg("token_filter")="", arg("text")="", 
		arg("color")=Color(), arg("min_dist")=0, arg("pulse")=false))
		.def("ClearObjective", &CGERadarResource::ClearObjective)
		.def("IsObjective", &CGERadarResource::IsObjective)
		.def("DropAllContacts", &CGERadarResource::DropAllContacts)
		.def("ListContactsNear", pyListContactsNear);

	class_<CGETokenManager, boost::noncopyable>("CGETokenManager", no_init)
		.def("SetupToken", raw_function(pySetupToken))
		.def("IsValidToken", &CGETokenManager::IsValidToken)
		.def("RemoveToken", &CGETokenManager::RemoveTokenDef)
		.def("RemoveTokenEnt", &CGETokenManager::RemoveTokenEnt, CGETokenManager_RemoveTokenEnt_overloads())
		.def("TransferToken", &CGETokenManager::TransferToken)
		.def("CaptureToken", &CGETokenManager::CaptureToken)
		.def("GetTokens", pyGetTokens)
		.def("SetupCaptureArea", raw_function(pySetupCaptureArea))
		.def("RemoveCaptureArea", &CGETokenManager::RemoveCaptureAreaDef)
		.def("SetGlobalAmmo", &CGETokenManager::SetGlobalAmmo, CGETokenManager_SetGlobalAmmo_overloads())
		.def("RemoveGlobalAmmo", &CGETokenManager::RemoveGlobalAmmo);

	class_<CGECaptureArea, bases<CBaseEntity>, boost::noncopyable>("CGECaptureArea", no_init)
		.def("GetGroupName", &CGECaptureArea::GetGroupName);
}
