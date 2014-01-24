///////////// Copyright 2008, Jonathan White. All rights reserved. /////////////
// 
// File: ge_gamerules.cpp
// Description:
//      GoldenEye Game Rules
//
// Created On: 28 Feb 08, 22:55
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#include "cbase.h"

#include "ammodef.h"
#include "ge_utils.h"
#include "ge_character_data.h"

#ifdef CLIENT_DLL
	#include "c_ge_player.h"
#else
	#include "game.h"
	#include "ge_webrequest.h"
	#include "ge_gameplay.h"
	#include "eventqueue.h"
	#include "items.h"
	#include "in_buttons.h"
	#include "voice_gamemgr.h"
	#include "Sprite.h"
	#include "utlbuffer.h"
	#include "filesystem.h"

	#include "ge_player.h"
	#include "ge_playerspawn.h"
	#include "ge_weapon.h"
	#include "grenade_gebase.h"
	#include "npc_tknife.h"
	#include "team.h"
	#include "ge_gameinterface.h"
	#include "ge_playerresource.h"
	#include "ge_radarresource.h"
	#include "ge_bot.h"
	#include "gebot_player.h"
	
	extern void respawn(CBaseEntity *pEdict, bool fCopyCorpse);
	class CGEReloadEntityFilter;
#endif

#include "ge_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
ConVar cl_autowepswitch( "cl_autowepswitch", "1", FCVAR_ARCHIVE | FCVAR_USERINFO, "Automatically switch to picked up weapons (if more powerful)" );
#endif

ConVar sv_report_client_settings("sv_report_client_settings", "0", FCVAR_GAMEDLL | FCVAR_NOTIFY );

static CViewVectors g_GEViewVectors(
	Vector( 0, 0, 64 ),        //m_vView
							  
	Vector(-12, -12, 0 ),	  //m_vHullMin
	Vector( 12,  12,  72 ),	  //m_vHullMax
							  
	Vector(-12, -12, 0 ),	  //m_vDuckHullMin
	Vector( 12,  12,  36 ),	  //m_vDuckHullMax
	Vector( 0, 0, 28 ),		  //m_vDuckView
							  
	Vector(-10, -10, -10 ),	  //m_vObsHullMin
	Vector( 10,  10,  10 ),	  //m_vObsHullMax
							  
	Vector( 0, 0, 14 )		  //m_vDeadViewHeight
);

const CViewVectors* CGERules::GetViewVectors() const
{
	return &g_GEViewVectors;
}

// --------------------------------------- //
// AMMO DEFS
// --------------------------------------- //
// shared ammo definition
// JAY: Trying to make a more physical bullet response
#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.000142857143*(grains))
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			2.0
// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)

CAmmoDef *GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;
	
	if ( !bInitted )
	{
		bInitted = true;

		def.AddAmmoTypeGE( AMMO_9MM,			DMG_BULLET,	TRACER_LINE_AND_WHIZ,	AMMO_9MM_MAX,			BULLET_IMPULSE(115, 1100),	AMMO_9MM_CRATE,			  AMMO_9MM_ICON );
		def.AddAmmoTypeGE( AMMO_GOLDENGUN,		DMG_BULLET,	TRACER_LINE_AND_WHIZ,	AMMO_GOLDENGUN_MAX,		BULLET_IMPULSE(170, 1700),	AMMO_GOLDENGUN_CRATE,	  AMMO_GOLDENGUN_ICON );
		def.AddAmmoTypeGE( AMMO_MAGNUM,			DMG_BULLET,	TRACER_LINE_AND_WHIZ,	AMMO_MAGNUM_MAX,		BULLET_IMPULSE(150, 1500),	AMMO_MAGNUM_CRATE,		  AMMO_MAGNUM_ICON );
		def.AddAmmoTypeGE( AMMO_RIFLE,			DMG_BULLET,	TRACER_LINE_AND_WHIZ,	AMMO_RIFLE_MAX,			BULLET_IMPULSE(130, 2000),	AMMO_RIFLE_CRATE,		  AMMO_RIFLE_ICON );
		def.AddAmmoTypeGE( AMMO_BUCKSHOT,		DMG_BULLET,	TRACER_LINE_AND_WHIZ,	AMMO_BUCKSHOT_MAX,		BULLET_IMPULSE(54, 1225),	AMMO_BUCKSHOT_CRATE,	  AMMO_BUCKSHOT_ICON );
		def.AddAmmoTypeGE( AMMO_REMOTEMINE,		DMG_BLAST,	TRACER_NONE,			AMMO_REMOTEMINE_MAX,	50,							AMMO_REMOTEMINE_CRATE,	  AMMO_REMOTEMINE_ICON );
		def.AddAmmoTypeGE( AMMO_PROXIMITYMINE,	DMG_BLAST,	TRACER_NONE,			AMMO_PROXIMITYMINE_MAX,	50,							AMMO_PROXIMITYMINE_CRATE, AMMO_PROXIMITYMINE_ICON );
		def.AddAmmoTypeGE( AMMO_TIMEDMINE,		DMG_BLAST,	TRACER_NONE,			AMMO_TIMEDMINE_MAX,		50,							AMMO_TIMEDMINE_CRATE,	AMMO_TIMEDMINE_ICON );
		def.AddAmmoTypeGE( AMMO_GRENADE,		DMG_BLAST,	TRACER_NONE,			AMMO_GRENADE_MAX,		50, 						AMMO_GRENADE_CRATE,		AMMO_GRENADE_ICON );
		def.AddAmmoTypeGE( AMMO_TKNIFE,			DMG_SLASH,	TRACER_NONE,			AMMO_TKNIFE_MAX,		50, 						AMMO_TKNIFE_CRATE,		AMMO_TKNIFE_ICON );
		def.AddAmmoTypeGE( AMMO_SHELL,			DMG_BLAST,	TRACER_NONE,			AMMO_SHELL_MAX,			50, 						AMMO_SHELL_CRATE,		AMMO_SHELL_ICON );
		def.AddAmmoTypeGE( AMMO_ROCKET,			DMG_BLAST,	TRACER_NONE,			AMMO_ROCKET_MAX,		50, 						AMMO_ROCKET_CRATE,		AMMO_ROCKET_ICON );
		def.AddAmmoTypeGE( AMMO_MOONRAKER,		DMG_BULLET,	TRACER_LINE_AND_WHIZ,	AMMO_MOONRAKER_MAX,		BULLET_IMPULSE(130, 2600),	AMMO_MOONRAKER_CRATE,	AMMO_MOONRAKER_ICON );
	}

	return &def;
}

#ifdef GAME_DLL
bool SimpleLessFunc( const int &lhs, const int &rhs )	
{ 
	return lhs < rhs; 
}
#endif

#ifdef GAME_DLL
CGEWebRequest *g_pStatusListWebRequest = NULL;
CGEWebRequest *g_pVersionWebRequest = NULL;

void OnVersionLoad( const char *result, const char *error )
{
	if ( !error || error[0] == '\0' )
	{
		KeyValues *kv = new KeyValues("update");
		if ( kv->LoadFromBuffer( "update", result ) )
		{
			string_t ges_version_text;
			int ges_major_version, ges_minor_version, ges_client_version, ges_server_version;
			if ( !GEUTIL_GetVersion( ges_version_text, ges_major_version, ges_minor_version, ges_client_version, ges_server_version ) )
			{
				Warning( "Failed loading version.txt, your installation may be corrupt!\n" );
				kv->deleteThis();
				return;
			}

			CUtlVector<char*> version;
			Q_SplitString( kv->GetString("version"), ".", version );

			if ( version.Count() >= 4 && (atoi(version[0]) != ges_major_version || atoi(version[1]) > ges_minor_version || atoi(version[3]) > ges_server_version) )
			{
				Warning( "=====================================================================\n" );
				Warning( "              GoldenEye: Source Update Available\n" );
				Warning( "There is a new version of GES available for download!\n" );
				Warning( "Your Version: %s\n", ges_version_text );
				Warning( "New Version: %s\n", kv->GetString("version") );
				Warning( "Download: %s\n", kv->GetString("server_mirror") );
				Warning( "=====================================================================\n" );
			}

			ClearStringVector( version );
		}
		kv->deleteThis();
	}

	// NOTE: We leave the reqeust object in memory
}
#endif

// --------------------------------------------------------------------------------------------------- //
// CGERules implementation.
// --------------------------------------------------------------------------------------------------- //
CGERules::CGERules()
{
#ifdef GAME_DLL
	m_vSpawnerLocations.SetLessFunc( SimpleLessFunc );
	m_vSpawnerStats.SetLessFunc( SimpleLessFunc );

	// This won't do much of anything except prevent crashes if the 
	// spawners are accessed prior to entity loading
	UpdateSpawnerLocations();

	// Ensure that we reload our status lists every map
	if ( g_pStatusListWebRequest )
		g_pStatusListWebRequest->Destroy();

	g_pStatusListWebRequest = NULL;

	m_bStatusListsUpdated = false;

	// Loaded if needed for auto complete
	LoadMapCycle();
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CGERules::~CGERules()
{
#ifdef GAME_DLL
	ClearSpawnerLocations();

	if ( g_pStatusListWebRequest )
		g_pStatusListWebRequest->Destroy();

	g_pStatusListWebRequest = NULL;
#endif
}

void CGERules::Precache(void)
{
#ifdef GAME_DLL
	BaseClass::Precache();

	// Call the Team Manager's precaches
	for ( int i = 0; i < GetNumberOfTeams(); i++ )
	{
		CTeam *pTeam = GetGlobalTeam( i );
		pTeam->Precache();
	}

	// Precache rolling explosion effect early
	PrecacheParticleSystem("GES_Explosion_B_Tiny");
	PrecacheParticleSystem("GES_Explosion_B");

	// Precache common gameplay sounds and models
	CBaseEntity::PrecacheModel( "models/gameplay/capturepoint.mdl" );
#endif
}

#ifdef GAME_DLL

// --------------------------------------------------------------------------------------------------- //
// Voice helper
// --------------------------------------------------------------------------------------------------- //
class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
{
public:
	virtual bool CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker, bool &bProximity )
	{
		// Dead players can only be heard by other dead team mates
		if ( !pTalker->IsAlive() )
		{
			if ( !pListener->IsAlive() )
				return ( pListener->InSameTeam( pTalker ) );

			return false;
		}

		return ( pListener->InSameTeam( pTalker ) );
	}
};
CVoiceGameMgrHelper g_VoiceGameMgrHelper;
IVoiceGameMgrHelper *g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;



bool CGERules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
{
	if( BaseClass::ClientCommand( pEdict, args ) )
		return true;

	if ( pEdict->IsPlayer() && ToGEPlayer(pEdict)->ClientCommand( args ) )
		return true;

	return false;
}

void CGERules::Think()
{
	// NOTE: We don't call the baseclass explicitly
	GetVoiceGameMgr()->Update( gpGlobals->frametime );

	if ( g_pStatusListWebRequest && g_pStatusListWebRequest->IsFinished() )
	{
		if ( g_pStatusListWebRequest->HadError() )
		{
			Warning( "Failed Status Check: %s\n", g_pStatusListWebRequest->GetError() );
		}
		else
		{
			UpdateStatusLists( g_pStatusListWebRequest->GetResult() );
		}

		m_bStatusListsUpdated = true;

		g_pStatusListWebRequest->Destroy();
		g_pStatusListWebRequest = NULL;
	}
}

void CGERules::GetTaggedConVarList( KeyValues *pCvarTagList )
{
	BaseClass::GetTaggedConVarList( pCvarTagList );

	KeyValues *key = new KeyValues( "sv_alltalk" );
	key->SetString( "convar", "sv_alltalk" );
	key->SetString( "tag", "alltalk" );
	pCvarTagList->AddSubKey( key );

	key = new KeyValues( "sv_cheats" );
	key->SetString( "convar", "sv_cheats" );
	key->SetString( "tag", "CHEATS" );
	pCvarTagList->AddSubKey( key );

	key = new KeyValues( "sv_footsteps" );
	key->SetString( "convar", "sv_footsteps" );
	key->SetString( "tag", "nofootsteps" );
	pCvarTagList->AddSubKey( key );
}

bool CGERules::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker )
{
	// Always allow us to hurt ourselves
	if ( pPlayer == pAttacker )
		return true;

	// Friendly fire means everyone gets injured!
	if ( friendlyfire.GetBool() )
		return true;

	// No teams!
	if ( pPlayer->GetTeamNumber() == TEAM_UNASSIGNED )
		return true;

	// Opposite teams
	if ( pPlayer->GetTeamNumber() != pAttacker->GetTeamNumber() )
		return true;

	// Same team
	return false;
}

bool CGERules::FPlayerCanRespawn( CBasePlayer *pPlayer )
{
	return GEGameplay()->GetScenario()->CanPlayerRespawn( ToGEPlayer(pPlayer) );
}


bool CGERules::CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon )
{
	return true;
}

bool CGERules::CanHaveItem( CBasePlayer *pPlayer, CItem *pItem )
{
	return GEGameplay()->GetScenario()->CanPlayerHaveItem( ToGEPlayer(pPlayer), pItem );
}

bool CGERules::ShouldForcePickup( CBasePlayer *pPlayer, CBaseEntity *pItem )
{
	return GEGameplay()->GetScenario()->ShouldForcePickup( ToGEPlayer(pPlayer), pItem );
}
	
bool CGERules::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon )
{		
	if ( pPlayer->GetActiveWeapon() && pPlayer->IsNetClient() && !(pPlayer->GetFlags() & FL_FAKECLIENT) )
	{
		// Player has an active item, so let's check cl_autowepswitch.
		const char *cl_autowepswitch = engine->GetClientConVarValue( engine->IndexOfEdict( pPlayer->edict() ), "cl_autowepswitch" );
		if ( cl_autowepswitch && atoi( cl_autowepswitch ) <= 0 )
			return false;

		// Define some GE:S specific reasons NOT to switch
		if ( ToGEPlayer(pPlayer)->IsInAimMode() )
			return false;

		int myweapid = ToGEWeapon(pPlayer->GetActiveWeapon())->GetWeaponID();
		int weapid	 = ToGEWeapon(pWeapon)->GetWeaponID();

		// We don't want to switch off of explosives if we explicitly had them out
		if ( myweapid == WEAPON_GRENADE_LAUNCHER || myweapid == WEAPON_ROCKET_LAUNCHER || myweapid == WEAPON_GRENADE ||
			 myweapid == WEAPON_REMOTEMINE || myweapid == WEAPON_PROXIMITYMINE || myweapid == WEAPON_TIMEDMINE )
		{
			// Although we want to switch to a more desirable explosive!
			if ( weapid != WEAPON_ROCKET_LAUNCHER && weapid != WEAPON_GRENADE_LAUNCHER )
				return false;
		}
	}

	// Use the MultiplayerGameRules to sort out the rest
	return BaseClass::FShouldSwitchWeapon( pPlayer, pWeapon );
}

// a client just connected to the server (player hasn't spawned yet)
bool CGERules::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
	return BaseClass::ClientConnected(pEntity, pszName, pszAddress, reject, maxrejectlen );
}

void CGERules::LevelInitPostEntity()
{
	UpdateSpawnerLocations();

	// We do this every map load
	g_pStatusListWebRequest = new CGEWebRequest( GES_AUTH_URL );

	// We only do this once per server run cycle
	if ( engine->IsDedicatedServer() && !g_pVersionWebRequest )
		g_pVersionWebRequest = new CGEWebRequest( GES_VERSION_URL, OnVersionLoad );

	BaseClass::LevelInitPostEntity();
}

void CGERules::InitHUD( CBasePlayer *pl )
{
	// Nothing here, yet...
	BaseClass::InitHUD( pl );
}

// Track when the client changes his settings so we can perform special
// functions with teams and announce to other players
// NOTE: We explicitly do not call the BaseClass
void CGERules::ClientSettingsChanged( CBasePlayer *pPlayer )
{
	if ( pPlayer->IsBot() )
		return;

	if ( sv_report_client_settings.GetInt() == 1 )
		UTIL_LogPrintf( "\"%s\" cl_cmdrate = \"%s\"\n", pPlayer->GetPlayerName(), engine->GetClientConVarValue( pPlayer->entindex(), "cl_cmdrate" ));

	const char *pszName = engine->GetClientConVarValue( pPlayer->entindex(), "name" );

	char pszOldName[33];
	Q_strncpy( pszOldName, pPlayer->GetPlayerName(), 33 );

	// msg everyone if someone changes their name,  and it isn't the first time (changing no name to current name)
	// Note, not using FStrEq so that this is case sensitive
	if ( pszOldName[0] != 0 && Q_strcmp( pszOldName, pszName ) )
	{
		pPlayer->SetPlayerName( pszName );

		IGameEvent * event = gameeventmanager->CreateEvent( "player_changename" );
		if ( event )
		{
			event->SetInt( "userid", pPlayer->GetUserID() );
			event->SetString( "oldname", pszOldName );
			event->SetString( "newname", pPlayer->GetPlayerName() );
			gameeventmanager->FireEvent( event );
		}
	}	
}

int CGERules::IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	if ( !pKilled )
		return 0;

	if ( !pAttacker )
		return 1;

	if ( pAttacker != pKilled && PlayerRelationship( pAttacker, pKilled ) == GR_TEAMMATE )
		return -1;

	return 1;
}

// If a solid surface blocks the explosion, this is how far to creep along the surface looking for another way to the target
#define ROBUST_RADIUS_PROBE_DIST 16.0f
bool IsExplosionTraceBlocked( trace_t *ptr );

ConVar ge_debug_explosions( "ge_debug_explosions", "0", FCVAR_CHEAT | FCVAR_GAMEDLL );
ConVar ge_exp_allowz( "ge_exp_allowz", "0", FCVAR_GAMEDLL, "Allows excessive Z forces on explosions" );

// ----------------------
// Custom Radius Damage!
// ----------------------
void CGERules::RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrcIn, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore )
{
	const int MASK_RADIUS_DAMAGE = MASK_SHOT & (~CONTENTS_HITBOX);
	CBaseEntity *pEntity = NULL;
	trace_t		tr;
	float		flAdjustedDamage, falloff;
	Vector		vecSpot;

	Vector vecSrc = vecSrcIn;
	vecSrc.z += 1;

	int bInWater = (UTIL_PointContents ( vecSrc ) & MASK_WATER) ? true : false;
	if( bInWater )
	{
		// Only muffle the explosion if deeper than 2 feet in water.
		if( !(UTIL_PointContents(vecSrc + Vector(0, 0, 24)) & MASK_WATER) )
		{
			bInWater = false;
		}
	}

	if ( ge_debug_explosions.GetBool() )
	{
		NDebugOverlay::Cross3D( vecSrc, Vector(-3,-3,-3), Vector(3,3,3), 255, 0, 0, 120, 7.0f );
		NDebugOverlay::Sphere( vecSrc, vec3_angle, flRadius, 255, 0, 0, 50, false, 7.0f );
	}

	// iterate on all entities in the vicinity.
	for ( CEntitySphereQuery sphere( vecSrc, flRadius ); (pEntity = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
	{
		// This value is used to scale damage when the explosion is blocked by some other object.
		float flBlockedDamagePercent = 0.0f;
		bool bIsPlayer = pEntity->IsPlayer() || pEntity->IsNPC();

		if ( pEntity == pEntityIgnore )
			continue;

		if ( pEntity->m_takedamage == DAMAGE_NO || (bIsPlayer && !pEntity->IsAlive()) )
			continue;

		// UNDONE: this should check a damage mask, not an ignore
		if ( iClassIgnore != CLASS_NONE && pEntity->Classify() == iClassIgnore )
		{// houndeyes don't hurt other houndeyes with their attack
			continue;
		}

		// blast's don't tavel into or out of water
		if (bInWater && pEntity->GetWaterLevel() == 0)
			continue;

		if (!bInWater && pEntity->GetWaterLevel() == 3)
			continue;

		// Check that the explosion can 'see' this entity.
		vecSpot = pEntity->EyePosition();
		UTIL_TraceLine( vecSrc, vecSpot, MASK_RADIUS_DAMAGE, info.GetInflictor(), COLLISION_GROUP_NONE, &tr );
		
		if ( tr.fraction != 1.0 )
		{
			if ( IsExplosionTraceBlocked(&tr) )
			{
				if( ShouldUseRobustRadiusDamage( pEntity ) )
				{
					Vector vecToTarget = vecSpot - tr.endpos;
					VectorNormalize( vecToTarget );

					// We're going to deflect the blast along the surface that 
					// interrupted a trace from explosion to this target.
					Vector vecUp, vecDeflect;
					CrossProduct( vecToTarget, tr.plane.normal, vecUp );
					CrossProduct( tr.plane.normal, vecUp, vecDeflect );
					VectorNormalize( vecDeflect );

					// Trace along the surface that intercepted the blast...
					UTIL_TraceLine( tr.endpos, tr.endpos + vecDeflect * ROBUST_RADIUS_PROBE_DIST, MASK_RADIUS_DAMAGE, info.GetInflictor(), COLLISION_GROUP_NONE, &tr );
					if ( ge_debug_explosions.GetBool() )
						debugoverlay->AddLineOverlay( tr.startpos, tr.endpos, 255, 255, 0, false, 4 );

					// ...to see if there's a nearby edge that the explosion would 'spill over' if the blast were fully simulated.
					UTIL_TraceLine( tr.endpos, vecSpot, MASK_RADIUS_DAMAGE, info.GetInflictor(), COLLISION_GROUP_NONE, &tr );
					if ( ge_debug_explosions.GetBool() )
						debugoverlay->AddLineOverlay( tr.startpos, tr.endpos, 255, 0, 0, false, 4 );

					if( tr.fraction != 1.0 && tr.DidHitWorld() )
					{
						// Still can't reach the target.
						continue;
					}
					// else fall through
				}
				else
				{
					continue;
				}
			}

			// UNDONE: Probably shouldn't let children block parents either?  Or maybe those guys should set their owner if they want this behavior?
			// HL2 - Dissolve damage is not reduced by interposing non-world objects
			if( tr.m_pEnt && tr.m_pEnt != pEntity && tr.m_pEnt->GetOwnerEntity() != pEntity )
			{
				// Some entity was hit by the trace, meaning the explosion does not have clear
				// line of sight to the entity that it's trying to hurt. If the world is also
				// blocking, we do no damage.
				CBaseEntity *pBlockingEntity = tr.m_pEnt;
				UTIL_TraceLine( vecSrc, vecSpot, CONTENTS_SOLID, info.GetInflictor(), COLLISION_GROUP_NONE, &tr );

				if( tr.fraction != 1.0 )
					continue;

				// Now, if the interposing object is physics, block some explosion force based on its mass.
				if( pBlockingEntity->VPhysicsGetObject() )
				{
					const float MASS_ABSORB_ALL_DAMAGE = 350.0f;
					float flMass = pBlockingEntity->VPhysicsGetObject()->GetMass();
					float scale = flMass / MASS_ABSORB_ALL_DAMAGE;

					// Absorbed all the damage.
					if( scale >= 1.0f )
					{
						continue;
					}

					ASSERT( scale > 0.0f );
					flBlockedDamagePercent = scale;
				}
				else
				{
					// Some object that's not the world and not physics. Generically block 25% damage
					flBlockedDamagePercent = 0.25f;
				}
			}
		}
		
		// Small explosions get scaled sustantially
		double smallExpScale = (flRadius <= 80.0f) ? RemapValClamped( flRadius, 10.0f, 80.0f, 0.05f, 0.35f ) : 1.0f;
		double dist = (vecSrc - tr.endpos).Length();

		if ( flRadius )
			falloff = max( 1.0 - (dist / flRadius), 0 );
		else
			falloff = 1.0;

		// NOTE: dist pegs out at 152 units with dmg = 2.97
		// Players get a fixed, precisely calculated damage (for large explosions ONLY)
		if ( bIsPlayer )
			flAdjustedDamage = 943.23 / (1.0 + 0.474*exp(.04*dist)) * smallExpScale;
		else
			flAdjustedDamage = info.GetDamage() * falloff;

		// Ignore damage that is insignificant
		if ( flAdjustedDamage < 3.0f )
			continue;

		if ( ge_debug_explosions.GetBool() )
		{
			float flForce = info.GetDamageForce().Length() * falloff * 1.15f * smallExpScale;
			debugoverlay->AddLineOverlay( vecSrc, vecSpot, 255, 255, 255, false, 5.0f );
			debugoverlay->AddTextOverlay( vecSpot, 0, 7.0f, "Damage: %0.2f", flAdjustedDamage );
			debugoverlay->AddTextOverlay( vecSpot, 2, 7.0f, "Force: %0.1f", flForce );
		}

		// the explosion can 'see' this entity, so hurt them!
		if (tr.startsolid)
		{
			// if we're stuck inside them, fixup the position and distance
			tr.endpos = vecSrc;
			tr.fraction = 0.0;
		}

		CTakeDamageInfo adjustedInfo = info;
		adjustedInfo.SetDamage( flAdjustedDamage - (flAdjustedDamage * flBlockedDamagePercent) );
		adjustedInfo.SetDamagePosition( vecSrc );

		Vector dir = vecSpot - vecSrc;
		VectorNormalize( dir );

		// If we don't have a damage force, manufacture one
		if ( adjustedInfo.GetDamageForce() == vec3_origin )
		{
			if ( !( adjustedInfo.GetDamageType() & DMG_PREVENT_PHYSICS_FORCE ) )
				CalculateExplosiveDamageForce( &adjustedInfo, dir, vecSrc, smallExpScale );
		}
		else
		{
			// Assume the force passed in is the maximum force. Decay it based on falloff.
			float flForce = adjustedInfo.GetDamageForce().Length() * falloff * 1.15f * smallExpScale;
			adjustedInfo.SetDamageForce( dir * flForce );
		}

		if ( !ge_exp_allowz.GetBool() )
			adjustedInfo.SetDamageForce( adjustedInfo.GetDamageForce() * Vector( 1.0, 1.0, 0.4 ) );

		if ( tr.fraction != 1.0 && pEntity == tr.m_pEnt )
		{
			ClearMultiDamage();
			pEntity->DispatchTraceAttack( adjustedInfo, dir, &tr );
			ApplyMultiDamage();
		}
		else
		{
			pEntity->TakeDamage( adjustedInfo );
		}

		// Now hit all triggers along the way that respond to damage... 
		pEntity->TraceAttackToTriggers( adjustedInfo, vecSrc, tr.endpos, dir );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Find the relationship between players (teamplay vs. deathmatch)
//-----------------------------------------------------------------------------
int CGERules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	// half life multiplay has a simple concept of Player Relationships.
	// you are either on another player's team, or you are not.
	if ( !pPlayer || !pTarget || !pTarget->IsPlayer() || IsTeamplay() == false )
		return GR_NOTTEAMMATE;

	if ( (*GetTeamID(pPlayer) != '\0') && (*GetTeamID(pTarget) != '\0') && !stricmp( GetTeamID(pPlayer), GetTeamID(pTarget) ) )
		return GR_TEAMMATE;

	return GR_NOTTEAMMATE;
}

extern void StripChar(char *szBuffer, const char cWhiteSpace );
void CGERules::LoadMapCycle()
{
		const char *mapcfile = mapcyclefile.GetString();
		Assert( mapcfile != NULL );

		// Check the time of the mapcycle file and re-populate the list of level names if the file has been modified
		const int nMapCycleTimeStamp = filesystem->GetPathTime( mapcfile, "GAME" );

		if ( 0 == nMapCycleTimeStamp )
		{
			// Map cycle file does not exist, make a list containing only the current map
			char *szCurrentMapName = new char[32];
			Q_strncpy( szCurrentMapName, STRING(gpGlobals->mapname), 32 );
			m_MapList.AddToTail( szCurrentMapName );
		}
		else
		{
			// If map cycle file has changed or this is the first time through ...
			if ( m_nMapCycleTimeStamp != nMapCycleTimeStamp )
			{
				// Reset map index and map cycle timestamp
				m_nMapCycleTimeStamp = nMapCycleTimeStamp;
				m_nMapCycleindex = 0;

				// Clear out existing map list. Not using Purge() because it doesn't do 'delete []'
				for ( int i = 0; i < m_MapList.Count(); i++ )
					delete [] m_MapList[i];
				m_MapList.RemoveAll();

				// Repopulate map list from mapcycle file
				int nFileLength;
				char *aFileList = (char*)UTIL_LoadFileForMe( mapcfile, &nFileLength );
				if ( aFileList && nFileLength )
				{
					V_SplitString( aFileList, "\n", m_MapList );

					for ( int i = 0; i < m_MapList.Count(); i++ )
					{
						bool bIgnore = false;

						// Strip out the spaces in the name
						StripChar( m_MapList[i] , '\r');
						StripChar( m_MapList[i] , ' ');
						
						if ( !engine->IsMapValid( m_MapList[i] ) )
						{
							// If the engine doesn't consider it a valid map remove it from the list
							bIgnore = true;
							Warning( "Invalid map '%s' included in map cycle file. Ignored.\n", m_MapList[i] );
						}
						else if ( !Q_strncmp( m_MapList[i], "//", 2 ) )
						{
							// Ignore comments
							bIgnore = true;
						}

						if ( bIgnore )
						{
							delete [] m_MapList[i];
							m_MapList.Remove( i );
							--i;
						}

					}

					UTIL_FreeFile( (byte *)aFileList );
				}
			}
		}
}

void CGERules::WorldReload()
{
	// Get rid of all entities except players.
	CBaseEntity *pCur = gEntList.FirstEnt();
	while ( pCur )
	{
		// Entities are removed from the world if they will be recreated
		if ( GEMapEntityFilter()->ShouldCreateEntity(pCur->GetClassname()) )
		{
			// Leave entities owned by players (handled on respawn)
			CBaseEntity *owner = pCur->GetOwnerEntity();
			if ( !owner || !owner->IsPlayer() )
				UTIL_Remove( pCur );
		}

		pCur = gEntList.NextEnt( pCur );
	}

	// Really remove the entities so we can have access to their slots below.
	gEntList.CleanupDeleteList();
	engine->AllowImmediateEdictReuse();

	// Clear any events sent to the server (e.g., mine detonate)
	g_EventQueue.Clear();

	// with any unrequired entities removed, we reparse the map entities
	// causing them to spawn back to their positions as if the map just loaded
	MapEntity_ParseAllEntities( engine->GetMapEntitiesString(), GEMapEntityFilter(), true);

	// Refresh our spawner locations
	UpdateSpawnerLocations();
}

void CGERules::SpawnPlayers()
{
	FOR_EACH_PLAYER( pPlayer )
		pPlayer->ForceRespawn();
	END_OF_PLAYER_LOOP()
}

bool CGERules::IsSpawnPointValid( CBaseEntity *pSpot, CBasePlayer *pPlayer  )
{
	if ( !pSpot )
		return false;

	// Don't use spots that are disabled or taken
	CGEPlayerSpawn *pSpawn = (CGEPlayerSpawn*) pSpot;
	if ( pSpawn->IsOccupied() || !pSpawn->IsEnabled() )
		return false;

	// Flter spawn points for bots
	if ( pPlayer->IsBot() )
		return pSpawn->IsBotFriendly();

	return true;
}

CBaseEntity *CGERules::GetSpectatorSpawnSpot( CGEPlayer *pPlayer )
{
	if ( !pPlayer || !pPlayer->IsObserver() )
		return NULL;

	CBaseEntity *pSpawnSpot = pPlayer->EntSelectSpawnPoint();
	if ( pSpawnSpot )
		pPlayer->JumptoPosition( pSpawnSpot->GetAbsOrigin() + VEC_VIEW, pSpawnSpot->GetAbsAngles() );

	return pSpawnSpot;
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CGERules::GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer )
{
	// Filter out dedicated server output
	if ( !pPlayer )
		return NULL;

	const char *pszFormat = NULL;

	// team only
	if ( bTeamOnly == TRUE )
	{
		if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "GES_Chat_Spec";
		}
		else
		{
			if ( pPlayer->IsAlive() )
			{
				const char *chatLocation = GetChatLocation( bTeamOnly, pPlayer );
				if ( chatLocation && *chatLocation )
				{
					pszFormat = "GES_Chat_Team_Loc";
				}
				else
				{
					pszFormat = "GES_Chat_Team";
				}
			}
			else
			{
				pszFormat = "GES_Chat_Team_Dead";
			}
		}
	}
	// everyone
	else
	{
		if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "GES_Chat_AllSpec";
		}
		else if ( pPlayer->IsAlive() )
		{
			pszFormat = "GES_Chat_All";	
		}
		else
		{
			pszFormat = "GES_Chat_AllDead";
		}
	}

	return pszFormat;
}

void CGERules::UpdateSpawnerLocations()
{
	ClearSpawnerLocations();

	const char *classname = NULL;
	CBaseEntity *pEnt = NULL;
	CUtlVector<EHANDLE> *vEnts = NULL;
	SpawnerStats *pStats = NULL;

	for ( int i=SPAWN_NONE+1; i < SPAWN_MAX; i++ )
	{
		classname = SpawnerTypeToClassName( i );
		Assert( classname );

		// Troll through our entity list building our vector
		vEnts = new CUtlVector<EHANDLE>;
		pStats = new SpawnerStats;
		pEnt = gEntList.FindEntityByClassname( NULL, classname );
		while( pEnt )
		{
			vEnts->AddToTail( pEnt );
			// TODO: We might need to introduce some dynamicy to this later
			Vector origin = pEnt->GetAbsOrigin();
			pStats->maxs = pStats->maxs.Max( origin );
			pStats->mins = pStats->mins.Min( origin );
			pStats->centroid += origin;

			pEnt = gEntList.FindEntityByClassname( pEnt, classname );
		}

		if ( vEnts->Count() > 0 )
			pStats->centroid /= vEnts->Count();

		m_vSpawnerStats.Insert( i, pStats );
		m_vSpawnerLocations.Insert( i, vEnts );
	}
}

const CUtlVector<EHANDLE>* CGERules::GetSpawnersOfType( int type )
{
	unsigned int idx = m_vSpawnerLocations.Find(type);
	if ( idx == m_vSpawnerLocations.InvalidIndex() )
		return NULL;

	return m_vSpawnerLocations.Element( idx );
}

bool CGERules::GetSpawnerStats( int spawn_type, Vector &mins, Vector &maxs, Vector &centroid )
{
	int idx = m_vSpawnerStats.Find( spawn_type );
	if ( idx != m_vSpawnerStats.InvalidIndex() )
	{
		mins = m_vSpawnerStats.Element(idx)->mins;
		maxs = m_vSpawnerStats.Element(idx)->maxs;
		centroid = m_vSpawnerStats.Element(idx)->centroid;
		return true;
	}

	return false;
}

void CGERules::ClearSpawnerLocations()
{
	for( unsigned int i=0; i < m_vSpawnerLocations.Count(); i++ )
	{
		m_vSpawnerLocations[i]->RemoveAll();
		delete m_vSpawnerLocations[i];
		delete m_vSpawnerStats[i];
	}
	m_vSpawnerLocations.Purge();
	m_vSpawnerStats.Purge();
}

void CGERules::InitDefaultAIRelationships()
{
	int i, j;
	//  Allocate memory for default relationships
	CBaseCombatCharacter::AllocateDefaultRelationships();

	// --------------------------------------------------------------
	// First initialize table so we can report missing relationships
	// --------------------------------------------------------------
	for ( i=0; i < NUM_AI_CLASSES; i++ )
	{
		for ( j=0; j < NUM_AI_CLASSES; j++ )
		{
			// By default all relationships are neutral of priority zero
			CBaseCombatCharacter::SetDefaultRelationship( (Class_T)i, (Class_T)j, D_NU, 0 );
		}
	}

	// ------------------------------------------------------------
	//	> CLASS_NONE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,			CLASS_NONE,				D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,			CLASS_PLAYER,			D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,			CLASS_PLAYER_ALLY,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,			CLASS_PLAYER_ALLY_VITAL,D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,			CLASS_GESGUARD,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,			CLASS_GESNONCOMBAT,		D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_PLAYER
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_NONE,				D_NU, 0);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_PLAYER,			D_HT, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_PLAYER_ALLY,		D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_PLAYER_ALLY_VITAL,D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_GESGUARD,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_GESNONCOMBAT,		D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_PLAYER_ALLY
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,		CLASS_NONE,				D_NU, 0);		
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,		CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,		CLASS_PLAYER,			D_LI, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,		CLASS_PLAYER_ALLY,		D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,		CLASS_PLAYER_ALLY_VITAL,D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,		CLASS_GESGUARD,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,		CLASS_GESNONCOMBAT,		D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_PLAYER_ALLY_VITAL
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_NONE,				D_NU, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_PLAYER,			D_LI, 0);			
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_PLAYER_ALLY,		D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_PLAYER_ALLY_VITAL,D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_GESGUARD,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_GESNONCOMBAT,		D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_GESGUARD
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GESGUARD,		CLASS_NONE,				D_NU, 0);					
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GESGUARD,		CLASS_MISSILE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GESGUARD,		CLASS_PLAYER,			D_HT, 1);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GESGUARD,		CLASS_PLAYER_ALLY,		D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GESGUARD,		CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GESGUARD,		CLASS_GESGUARD,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GESGUARD,		CLASS_GESNONCOMBAT,		D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_GESNONCOMBAT
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GESNONCOMBAT,	CLASS_NONE,				D_NU, 0);					
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GESNONCOMBAT,	CLASS_MISSILE,			D_FR, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GESNONCOMBAT,	CLASS_PLAYER,			D_NU, 1);	
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GESNONCOMBAT,	CLASS_PLAYER_ALLY,		D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GESNONCOMBAT,	CLASS_PLAYER_ALLY_VITAL,D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GESNONCOMBAT,	CLASS_GESGUARD,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_GESNONCOMBAT,	CLASS_GESNONCOMBAT,		D_NU, 0);
}
#endif //End Server functions of GE_GAMERULES

bool CGERules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		swap(collisionGroup0,collisionGroup1);
	}
	
	//Don't stand on COLLISION_GROUP_WEAPON
	if( (collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT ||
		 collisionGroup0 == COLLISION_GROUP_PLAYER ||
		 collisionGroup0 == COLLISION_GROUP_NPC )
		 &&	collisionGroup1 == COLLISION_GROUP_WEAPON )
	{
		return false;
	}
	
	// Mines shouldn't collide with weapons, debris, players, NPCs, or projectiles
	if ( collisionGroup1 == COLLISION_GROUP_MINE )
	{
		if ( collisionGroup0 == COLLISION_GROUP_DEBRIS || 
			collisionGroup0 == COLLISION_GROUP_WEAPON ||
			collisionGroup0 == COLLISION_GROUP_PROJECTILE ||
			collisionGroup0 == COLLISION_GROUP_PLAYER ||
			collisionGroup0 == COLLISION_GROUP_NPC )
		{
			return false;
		}
	}

	if ( collisionGroup1 == COLLISION_GROUP_GRENADE )
	{
		if ( collisionGroup0 == COLLISION_GROUP_DEBRIS || 
			collisionGroup0 == COLLISION_GROUP_PROJECTILE )
		{
			return false;
		}
	}

	if ( collisionGroup1 == COLLISION_GROUP_CAPAREA )
	{
		if ( collisionGroup0 == COLLISION_GROUP_PLAYER ||
			collisionGroup0 == COLLISION_GROUP_NPC ||
			collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT )
			return false;
	}

	return BaseClass::ShouldCollide( collisionGroup0, collisionGroup1 ); 
}

const unsigned char *CGERules::GetEncryptionKey()
{
	return GEUTIL_GetSecretHash();
}

bool CGERules::ShouldUseRobustRadiusDamage( CBaseEntity *pEntity )
{
	if ( pEntity->IsPlayer() || pEntity->IsNPC() )
		return true;

	return false;
}
