///////////// Copyright © 2008 LodleNet. All rights reserved. /////////////
//
//   Project     : Server (GES)
//   File        : gebot_player.h
//   Description :
//      Encapsulates an NPC to use as a player entity
//
//   Created On: 3/23/2011
//   Created By: Jonathan White <killermonkey01@gmail.com>
////////////////////////////////////////////////////////////////////////////

#ifndef MC_GEBOT_PLAYER_H
#define MC_GEBOT_PLAYER_H
#ifdef _WIN32
#pragma once
#endif

class CGEBotPlayer;

#include "gemp_player.h"
#include "npc_gebase.h"

class CGEBotPlayer : public CGEMPPlayer
{
public:
	DECLARE_CLASS( CGEBotPlayer, CGEMPPlayer );

	CGEBotPlayer();
	~CGEBotPlayer();

	virtual bool IsBot() const		  { return true;  }
	virtual bool IsBotPlayer() const  { return true;  }
	virtual bool IsNetClient() const  { return false; }
	virtual bool IsFakeClient() const { return true;  }

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CNPC_GEBase *GetNPC() { return m_pNPC; }

	bool LoadBrain();

	virtual void InitialSpawn();
	virtual void Spawn();
	virtual void ForceRespawn();
	virtual void FreezePlayer( bool state = true );
	virtual int  UpdateTransmitState( void );

	virtual void PickDefaultSpawnTeam();
	virtual void ChangeTeam( int iTeam, bool bWasForced = false );

	virtual void SetPlayerModel();
	virtual void SetPlayerTeamModel();
	virtual void SetPlayerModel( const char* szCharName, int iCharSkin = 0, bool bNoSelOverride = false );
	virtual bool PickPlayerModel( int team );

	virtual void FireBullets( const FireBulletsInfo_t &info );
	virtual int  OnTakeDamage( const CTakeDamageInfo &info );
	virtual void Event_Killed( const CTakeDamageInfo &info );
	virtual void DropAllTokens();
	virtual void PlayerDeathThink();
	virtual void CreateRagdollEntity( void );

	virtual void PreThink();
	virtual void PostThink();

	virtual void GiveHat();
	virtual void SpawnHat(const char* hatModel, bool canBeRemoved = true);
	virtual void KnockOffHat( bool bRemove = false, const CTakeDamageInfo *dmg = NULL );

	// Bots don't need help!
	virtual void ShowScenarioHelp() { }

	virtual void GiveDefaultItems();

	virtual bool Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex = 0 );
	virtual void GiveNamedWeapon( const char* ident, int ammoCount, bool stripAmmo = false );
	virtual int	 GiveAmmo( int iCount, int iAmmoIndex, bool bSuppressSound = false );
	virtual bool BumpWeapon( CBaseCombatWeapon *pWeapon );

	virtual void StripAllWeapons();
	virtual void RemoveAllItems( bool removeSuit );
	virtual CBaseCombatWeapon *GetActiveWeapon();

private:
	CNetworkHandle( CNPC_GEBase, m_pNPC );
	float m_flRespawnDelay;
};

CGEBotPlayer *ToGEBotPlayer( CBaseEntity *pEntity );

#define FOR_EACH_BOTPLAYER( var ) \
	for( int _iter=1; _iter <= gpGlobals->maxClients; ++_iter ) { \
		CGEBotPlayer *var = ToGEBotPlayer( UTIL_PlayerByIndex( _iter ) ); \
		if ( var == NULL || FNullEnt( var->edict() ) || !var->IsPlayer() ) \
			continue;

#define END_OF_PLAYER_LOOP() }

#endif //MC_GEBOT_PLAYER_H
