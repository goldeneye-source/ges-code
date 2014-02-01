///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_radarresource.h
// Description:
//      see ge_radarresource.cpp
//
// Created On: 19 Jan 09
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#ifndef GE_RADARRESOURCE
#define GE_RADARRESOURCE
#ifdef _WIN32
#pragma once
#endif

#include "ge_shareddefs.h"
#include "baseentity.h"

class CGERadarResource : public CBaseEntity
{
	DECLARE_CLASS( CGERadarResource, CBaseEntity );
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual void Spawn( void );
	virtual int  UpdateTransmitState( void );

	void SetForceRadar( bool state );
	void SetRangeModifier( CBaseEntity *pEnt, float mod );

	void AddRadarContact( CBaseEntity *pEnt, int type = RADAR_TYPE_DEFAULT, bool alwaysVis = false, const char *icon = "", Color color = Color() );
	void DropRadarContact( CBaseEntity *pEnt );
	bool IsRadarContact( CBaseEntity *pEnt );

	void SetupObjective( CBaseEntity *pEnt, int team_filter = TEAM_UNASSIGNED, const char *token_filter = "", const char *text = "", Color color = Color(), int min_dist = 0, bool pulse = false );
	void SetupObjective2( CBaseEntity *pEnt, int *team_filter, const char *token_filter, const char *icon, Color *color, int *min_dist, bool *pulse );
	void ClearObjective( CBaseEntity *pEnt );
	bool IsObjective( CBaseEntity *pEnt );

	void DropAllContacts( void );

private:
	void RadarThink( void );
	void UpdateState( int pos );

	int  FindEntity( EHANDLE hEnt );
	int  FindNextOpenSlot( CBaseEntity *pEnt );

	void ResetContact( int pos );
	void ResetObjective( int pos );

// Public so we can expose info to NPC's
public:
	CNetworkVar( bool, m_bForceOn );

	CNetworkArray( EHANDLE,		m_hEnt,			MAX_NET_RADAR_ENTS );
	CNetworkArray( int,			m_iEntTeam,		MAX_NET_RADAR_ENTS );
	CNetworkArray( float,		m_flRangeMod,	MAX_NET_RADAR_ENTS );	// Only used by players
	CNetworkArray( Vector,		m_vOrigin,		MAX_NET_RADAR_ENTS );
	CNetworkArray( int,			m_iType,		MAX_NET_RADAR_ENTS );
	CNetworkArray( int,			m_iState,		MAX_NET_RADAR_ENTS );
	CNetworkArray( bool,		m_bAllVisible,	MAX_NET_RADAR_ENTS );
	CNetworkArray( string_t,	m_szIcon,		MAX_NET_RADAR_ENTS );
	CNetworkArray( color32,		m_Color,		MAX_NET_RADAR_ENTS );

	CNetworkArray( bool,		m_ObjStatus,	MAX_NET_RADAR_ENTS );
	CNetworkArray( int,			m_ObjTeam,		MAX_NET_RADAR_ENTS );
	CNetworkArray( color32,		m_ObjColor,		MAX_NET_RADAR_ENTS );
	CNetworkArray( string_t,	m_ObjToken,		MAX_NET_RADAR_ENTS );
	CNetworkArray( string_t,	m_ObjText,		MAX_NET_RADAR_ENTS );
	CNetworkArray( int,			m_ObjMinDist,	MAX_NET_RADAR_ENTS );
	CNetworkArray( bool,		m_ObjPulse,		MAX_NET_RADAR_ENTS );
};

extern CGERadarResource *g_pRadarResource;

#endif
