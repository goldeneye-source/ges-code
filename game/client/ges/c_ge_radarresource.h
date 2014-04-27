///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: c_ge_radarresource.h
// Description:
//      see c_ge_radarresource.cpp
//
// Created On: 19 Jan 09
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#ifndef C_GE_RADARRESOURCE
#define C_GE_RADARRESOURCE
#ifdef _WIN32
#pragma once
#endif

#include "ge_shareddefs.h"
#include "c_baseentity.h"

class C_GERadarResource : public C_BaseEntity
{
	DECLARE_CLASS( C_GERadarResource, C_BaseEntity );
public:
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	C_GERadarResource( void );
	~C_GERadarResource( void );
	
	bool		ForceRadarOn( void ) { return m_bForceOn; };

	EHANDLE		GetEnt( int idx );
	int			GetEntSerial( int idx );
	int			GetEntTeam( int idx );
	float		GetRangeModifier( int idx );
	Vector		GetOrigin( int idx );
	int			GetType( int idx );
	int			GetState( int idx );
	bool		GetAlwaysVisible( int idx );
	const char *GetIcon( int idx );
	Color		GetColor( int idx );

	bool		IsObjective( int idx );
	int			GetObjectiveTeam( int idx );
	Color		GetObjectiveColor( int idx );
	const char *GetObjectiveToken( int idx );
	const char *GetObjectiveText( int idx );
	int			GetObjectiveMinDist( int idx );
	bool		GetObjectivePulse( int idx );

	bool		FindEntity( int iSerial );
	int			FindEntityIndex( int iSerial );

protected:
	bool		m_bForceOn;

	EHANDLE		m_hEnt			[MAX_NET_RADAR_ENTS];
	int			m_iEntTeam		[MAX_NET_RADAR_ENTS];
	float		m_flRangeMod	[MAX_NET_RADAR_ENTS];
	Vector		m_vOrigin		[MAX_NET_RADAR_ENTS];
	int			m_iType			[MAX_NET_RADAR_ENTS];
	int			m_iState		[MAX_NET_RADAR_ENTS];
	bool		m_bAllVisible	[MAX_NET_RADAR_ENTS];
	color32		m_Color			[MAX_NET_RADAR_ENTS];
	char		m_szIcon		[MAX_NET_RADAR_ENTS][255];

	bool		m_ObjStatus		[MAX_NET_RADAR_ENTS];
	int			m_ObjTeam		[MAX_NET_RADAR_ENTS];
	color32		m_ObjColor		[MAX_NET_RADAR_ENTS];
	char		m_ObjToken		[MAX_NET_RADAR_ENTS][255];
	char		m_ObjText		[MAX_NET_RADAR_ENTS][32];
	int			m_ObjMinDist	[MAX_NET_RADAR_ENTS];
	bool		m_ObjPulse		[MAX_NET_RADAR_ENTS];
};

extern C_GERadarResource *g_RR;

#endif
