///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_playerresource.h
// Description:
//      see ge_playerresource.cpp
//
// Created On: 15 Sep 08
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#ifndef GE_PLAYERRESOURCE
#define GE_PLAYERRESOURCE
#ifdef _WIN32
#pragma once
#endif

#include "ge_shareddefs.h"
#include "player_resource.h"

class CGEPlayerResource : public CPlayerResource
{
	DECLARE_CLASS( CGEPlayerResource, CPlayerResource );
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual void Spawn( void );

	virtual void UpdatePlayerData( void );
	int GetPing( int entindex );

protected:
	CNetworkArray( string_t, m_szCharIdent, MAX_PLAYERS+1 );
	CNetworkArray( int, m_iFavWeapon, MAX_PLAYERS+1 );
	CNetworkArray( int, m_iDevStatus, MAX_PLAYERS+1 );
	CNetworkArray( int, m_iCampingPercent, MAX_PLAYERS+1 );
	CNetworkArray( int, m_iFrags, MAX_PLAYERS+1 );
	CNetworkArray( int, m_iScoreBoardColor, MAX_PLAYERS+1 );
	CNetworkArray( bool, m_bIsActive, MAX_PLAYERS+1 );
};

extern CGEPlayerResource *g_pPlayerResource;

#endif
