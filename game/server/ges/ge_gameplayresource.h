///////////// Copyright © 2011, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_gameplayresource.h
// Description:
//      Transmits gameplay related information to clients for HUD/VGUI updates
//
// Created On: 25 Jul 11
// Created By: Jonathan White <killermonkey> 
////////////////////////////////////////////////////////////////////////////////////
#ifndef GE_GAMEPLAYRESOURCE
#define GE_GAMEPLAYRESOURCE
#ifdef _WIN32
#pragma once
#endif

#include "ge_gameplay.h"
#include "ge_shareddefs.h"
#include "baseentity.h"

class CGEGameplayResource : public CBaseEntity, public CGEGameplayEventListener
{
	DECLARE_CLASS( CGEGameplayResource, CBaseEntity );
public:
	DECLARE_SERVERCLASS();

	virtual void Spawn( void );
	virtual int  UpdateTransmitState( void );

	// Gameplay Events
	virtual void OnGameplayEvent( GPEvent event );

	// Loadout Events
	virtual void OnLoadoutChanged( const char *ident, const char *name, CUtlVector<int> &weapons );

	// Character Events
	virtual void SetCharacterExclusion( const char *str_list );

protected:
	// Gameplay Data
	CNetworkString( m_GameplayIdent, 32 );
	CNetworkString( m_GameplayName,  64 );
	CNetworkVar( int,  m_GameplayHelp );
	CNetworkVar( bool, m_GameplayOfficial );
	CNetworkVar( int,  m_GameplayRoundNum );
	CNetworkVar( float, m_GameplayRoundStart );
	CNetworkVar( bool, m_GameplayIntermission );
	// Loadout Data
	CNetworkString( m_LoadoutIdent, 32 );
	CNetworkString( m_LoadoutName,  64 );
	CNetworkArray( int, m_LoadoutWeapons, MAX_WEAPON_SPAWN_SLOTS );
	// Character Data
	CNetworkString( m_CharExclusion, 64 );
};

extern CGEGameplayResource *g_pGameplayResource;

#endif
