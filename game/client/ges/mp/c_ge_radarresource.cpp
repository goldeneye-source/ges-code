///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: c_ge_radarresource.cpp
// Description:
//      Recieves radar contacts that are parsed on the server for dissemination on the client's radar
//
// Created On: 19 Jan 09
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include "c_ge_radarresource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern void RecvProxy_String_tToString( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );

IMPLEMENT_CLIENTCLASS_DT(C_GERadarResource, DT_GERadarResource, CGERadarResource)
	RecvPropBool( RECVINFO( m_bForceOn ) ),

	RecvPropArray3( RECVINFO_ARRAY(m_hEnt), RecvPropEHandle( RECVINFO(m_hEnt[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_iEntTeam), RecvPropInt( RECVINFO(m_iEntTeam[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_flRangeMod), RecvPropFloat( RECVINFO(m_flRangeMod[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_vOrigin), RecvPropVector( RECVINFO(m_vOrigin[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_iType), RecvPropInt( RECVINFO(m_iType[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_iState), RecvPropInt( RECVINFO(m_iState[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_bAllVisible), RecvPropBool( RECVINFO(m_bAllVisible[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_Color), RecvPropInt( RECVINFO(m_Color[0]), 0, RecvProxy_IntToColor32 ) ),
	RecvPropArray( RecvPropString( RECVINFO( m_szIcon[0]) ), m_szIcon ),

	RecvPropArray3( RECVINFO_ARRAY(m_ObjStatus), RecvPropBool( RECVINFO(m_ObjStatus[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_ObjTeam), RecvPropInt( RECVINFO(m_ObjTeam[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_ObjColor), RecvPropInt( RECVINFO(m_ObjColor[0]), 0, RecvProxy_IntToColor32 ) ),
	RecvPropArray( RecvPropString( RECVINFO(m_ObjToken[0]) ), m_ObjToken ),
	RecvPropArray( RecvPropString( RECVINFO(m_ObjText[0]) ), m_ObjText ),
	RecvPropArray3( RECVINFO_ARRAY(m_ObjMinDist), RecvPropInt( RECVINFO(m_ObjMinDist[0]) ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_ObjPulse), RecvPropBool( RECVINFO(m_ObjPulse[0]) ) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_GERadarResource )
END_PREDICTION_DATA()

C_GERadarResource *g_RR;

C_GERadarResource::C_GERadarResource( void )
{
	m_bForceOn = false;

	memset( m_hEnt, INVALID_EHANDLE_INDEX, sizeof( m_hEnt ) );
	memset( m_iEntTeam, TEAM_UNASSIGNED, sizeof( m_iEntTeam ) );
	memset( m_vOrigin, 0, sizeof( m_vOrigin ) );
	memset( m_iType, -1, sizeof( m_iType ) );
	memset( m_iState, RADAR_STATE_NONE, sizeof( m_iState ) );
	memset( m_bAllVisible, 0, sizeof( m_bAllVisible ) );
	memset( m_szIcon, '\0', sizeof( m_szIcon ) );
	memset( m_Color, 0, sizeof( m_Color ) );

	memset( m_ObjStatus, 0, sizeof( m_ObjStatus ) );
	memset( m_ObjTeam, TEAM_UNASSIGNED, sizeof( m_ObjTeam ) );
	memset( m_ObjColor, 0, sizeof( m_ObjColor ) );
	memset( m_ObjText, '\0', sizeof( m_ObjText ) );
	memset( m_ObjToken, '\0', sizeof( m_ObjToken ) );

	g_RR = this;
}

C_GERadarResource::~C_GERadarResource( void )
{
	g_RR = NULL;
}

EHANDLE C_GERadarResource::GetEnt( int idx )
{
	if ( idx < 0 || idx >= MAX_NET_RADAR_ENTS )
		return NULL;

	return m_hEnt[ idx ];
}

int C_GERadarResource::GetEntSerial( int idx )
{
	if ( idx < 0 || idx >= MAX_NET_RADAR_ENTS )
		return INVALID_EHANDLE_INDEX;

	return m_hEnt[ idx ].ToInt();
}

int C_GERadarResource::GetEntTeam( int idx )
{
	if ( idx < 0 || idx >= MAX_NET_RADAR_ENTS )
		return TEAM_UNASSIGNED;

	return m_iEntTeam[ idx ];
}

float C_GERadarResource::GetRangeModifier( int idx )
{
	if ( idx < 0 || idx >= MAX_NET_RADAR_ENTS )
		return 1.0f;

	return m_flRangeMod[ idx ];
}

Vector C_GERadarResource::GetOrigin( int idx )
{
	if ( idx < 0 || idx >= MAX_NET_RADAR_ENTS )
		return Vector();

	return m_vOrigin[ idx ];
}

int C_GERadarResource::GetType( int idx )
{
	if ( idx < 0 || idx >= MAX_NET_RADAR_ENTS )
		return RADAR_TYPE_DEFAULT;

	return m_iType[ idx ];
}

int C_GERadarResource::GetState( int idx )
{
	if ( idx < 0 || idx >= MAX_NET_RADAR_ENTS )
		return RADAR_STATE_NONE;

	return m_iState[ idx ];
}

bool C_GERadarResource::GetAlwaysVisible( int idx )
{
	if ( idx < 0 || idx >= MAX_NET_RADAR_ENTS )
		return false;

	return m_bAllVisible[ idx ];
}

const char *C_GERadarResource::GetIcon( int idx )
{
	if ( idx < 0 || idx >= MAX_NET_RADAR_ENTS )
		return "";

	return m_szIcon[ idx ];
}

Color C_GERadarResource::GetColor( int idx )
{
	Color col;

	if ( idx < 0 || idx >= MAX_NET_RADAR_ENTS )
		return col;

	color32 col32 = m_Color[ idx ];

	col.SetColor( col32.r, col32.g, col32.b, col32.a );
	return col;
}

bool C_GERadarResource::IsObjective( int idx )
{
	if ( idx < 0 || idx >= MAX_NET_RADAR_ENTS )
		return false;

	return m_ObjStatus[ idx ];
}

int C_GERadarResource::GetObjectiveTeam( int idx )
{
	if ( idx < 0 || idx >= MAX_NET_RADAR_ENTS )
		return TEAM_UNASSIGNED;

	return m_ObjTeam[ idx ];
}

Color C_GERadarResource::GetObjectiveColor( int idx )
{
	Color col;

	if ( idx < 0 || idx >= MAX_NET_RADAR_ENTS )
		return col;

	color32 col32 = m_ObjColor[ idx ];

	col.SetColor( col32.r, col32.g, col32.b, col32.a );
	return col;
}

const char *C_GERadarResource::GetObjectiveToken( int idx )
{
	if ( idx < 0 || idx >= MAX_NET_RADAR_ENTS )
		return "";

	return m_ObjToken[idx];
}

const char *C_GERadarResource::GetObjectiveText( int idx )
{
	if ( idx < 0 || idx >= MAX_NET_RADAR_ENTS )
		return "";

	return m_ObjText[idx];
}

int C_GERadarResource::GetObjectiveMinDist( int idx )
{
	if ( idx < 0 || idx >= MAX_NET_RADAR_ENTS )
		return 0;

	return m_ObjMinDist[idx];
}

bool C_GERadarResource::GetObjectivePulse( int idx )
{
	if ( idx < 0 || idx >= MAX_NET_RADAR_ENTS )
		return false;

	return m_ObjPulse[idx];
}

bool C_GERadarResource::FindEntity( int iSerial )
{
	for ( int i=0; i < MAX_NET_RADAR_ENTS; ++i )
	{
		if ( m_hEnt[i].ToInt() == iSerial )
			return true;
	}

	return false;
}

int C_GERadarResource::FindEntityIndex( int iSerial )
{
	for ( int i=0; i < MAX_NET_RADAR_ENTS; ++i )
	{
		if ( m_hEnt[i].ToInt() == iSerial )
			return i;
	}

	return -1;
}
