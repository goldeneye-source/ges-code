/////////////  Copyright � 2006, Scott Loyd. All rights reserved.  /////////////
// 
// ge_gameinterface.cpp
//
// Description:
//      Goldeneye Specific definition of some mod-specific values.
//		 there is no class of it's own, it is defining methods part of 
//		 gameinterface.cpp/h.
//		 This also includes map parsing hooks used on map load, and used by our
//		 gamerules code to reload entities for a round restart.
//
// Created On: 8/21/2006 1:38:47 PM
////////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include "ge_gamerules.h"
#include "ge_gameinterface.h"
#include "gamestats.h"
#include "mapentities.h"
#include "ge_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Called in gameinterface.cpp line 650
void GE_OverrideCommands()
{
	// First override the changelevel command (defined in ge_gameplay.cpp)
	GEUTIL_OverrideCommand( "changelevel", "__real_changelevel", "__ovr_changelevel", 
		"Change the current level after ending the current round/match. Use `changelevel [mapname] 0` to change immediately." );
}

void CServerGameClients::GetPlayerLimits( int& minplayers, int& maxplayers, int &defaultMaxPlayers ) const
{
	minplayers = 2;
	maxplayers = MAX_PLAYERS;
	defaultMaxPlayers = 16;
}

void CServerGameClients::ClientDisconnect( edict_t *pEdict )
{
	CBasePlayer *player = ( CBasePlayer * )CBaseEntity::Instance( pEdict );
	if ( player )
	{
		player->SetMaxSpeed( 0.0f );

		CSound *pSound;
		pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( pEdict ) );
		{
			// since this client isn't around to think anymore, reset their sound. 
			if ( pSound )
			{
				pSound->Reset();
			}
		}

		// since the edict doesn't get deleted, fix it so it doesn't interfere.
		player->RemoveFlag( FL_AIMTARGET ); // don't attract autoaim
		player->AddFlag( FL_DONTTOUCH );	// stop it touching anything
		player->AddFlag( FL_NOTARGET );	// stop NPCs noticing it
		player->AddSolidFlags( FSOLID_NOT_SOLID );		// nonsolid

		if ( g_pGameRules )
		{
			g_pGameRules->ClientDisconnected( pEdict );
			gamestats->Event_PlayerDisconnected( player );
		}

		// Make sure all Untouch()'s are called for this client leaving
		CBaseEntity::PhysicsRemoveTouchedList( player );
		CBaseEntity::PhysicsRemoveGroundList( player );

#if !defined( NO_ENTITY_PREDICTION )
		// Make sure anything we "own" is simulated by the server from now on
		player->ClearPlayerSimulationList();
#endif
	}
}

void CServerGameDLL::LevelInit_ParseAllEntities( const char *pMapEntities )
{
}

//
// List of entities that are preserved during round-restarts
//
static const char *s_PreserveEnts[] =
{
	"ai_network",
	"ai_hint",
	"team_manager",
	"player_manager",
	"env_soundscape",
	"env_soundscape_proxy",
	"env_soundscape_triggerable",
	"env_sun",
	"env_wind",
	"env_fog_controller",
//	"func_brush",
	"func_wall",
	"func_illusionary",
	"infodecal",
	"info_projecteddecal",
	"info_node",
	"info_target",
	"info_node_hint",
	"info_player_deathmatch",
	"info_map_parameters",
	"keyframe_rope",
	"move_rope",
	"info_ladder",
	"player",
	"point_viewcontrol",
	"scene_manager",
	"shadow_control",
	"sky_camera",
	"soundent",
	"trigger_soundscape",
	"viewmodel",
	"predicted_viewmodel",
	"worldspawn",
	"point_devshot_camera",
//	"func_door",
	"phys_lengthconstraint",
//	"func_physbox",
	//GE Specific Ones
	"ge_gamerules",
	"gemp_gamerules",
	"gesp_gamerules",
	"ge_team_manager",
	"radar_resource",
	"gameplay_resource",
	"info_player_spectator",
	"info_player_mi6",
	"info_player_janus",
	"ge_logic_bitflag",
	"ge_game_timer",
	"func_smokevolume",
	"func_precipitation",
	"func_GE_precipitation",
	"mp_player",
	"sp_player",
	"gebloodscreen",
	"npc_gebase",
	"bot_player",
	"", // END Marker
};

class CGEReloadEntityFilter : public IMapEntityFilter
{
public:
	bool ShouldRecreateEntity(const char *pEntName)
	{
		int i = 0;
		while ( s_PreserveEnts[i][0] != 0 )
		{
			if ( Q_stricmp( s_PreserveEnts[i], pEntName ) == 0 )
			{
				// Increment our iterator since it's not going to call CreateNextEntity for this ent.
				if ( m_iMapEntityRefMover != g_MapEntityRefs.InvalidIndex() )
					m_iMapEntityRefMover = g_MapEntityRefs.Next( m_iMapEntityRefMover );
				return false;
			}
			i++;
		}
		return true;
	};

	virtual bool ShouldCreateEntity( const char *pClassname )
	{ 
		return ShouldRecreateEntity(pClassname);
	};

	CBaseEntity	*CreateNewEntity(const char *pClassname)
	{
		CBaseEntity *pRet = CreateEntityByName( pClassname );
		if(!pRet)
			return NULL;

		CMapEntityRef ref;
		ref.m_iEdict = -1;
		ref.m_iSerialNumber = -1;

		g_MapEntityRefs.AddToTail( ref );
		return pRet;
	};
	
	virtual CBaseEntity* CreateNextEntity( const char *pClassname )
	{
		Assert(! (m_iMapEntityRefMover == g_MapEntityRefs.InvalidIndex()) );

		//Check to see if a slot exists, if not just make the entity in a new slot.
		CMapEntityRef &ref = g_MapEntityRefs[m_iMapEntityRefMover];
		m_iMapEntityRefMover = g_MapEntityRefs.Next( m_iMapEntityRefMover );// Seek to the next entity.

		if ( ref.m_iEdict == -1 || engine->PEntityOfEntIndex( ref.m_iEdict ) )
		{
			// Doh! The entity was delete and its slot was reused.
			// Just use any old edict slot. This case sucks because we lose the baseline.
			return CreateEntityByName( pClassname );
		}
		else
		{
			// Cool, the slot where this entity was is free again.
			// Now create an entity with this specific index.
			return CreateEntityByName( pClassname, ref.m_iEdict );
		}
	};

	void SetMover(int iValue) { m_iMapEntityRefMover = iValue; };

private:
	int m_iMapEntityRefMover;
};

//Accessor
CGEReloadEntityFilter g_MapEntityFilter;
IMapEntityFilter *GEMapEntityFilter()
{
	g_MapEntityFilter.SetMover(g_MapEntityRefs.Head());
	return &g_MapEntityFilter;
}
