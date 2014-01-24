///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: c_ge_playerresource.cpp
// Description:
//      Collects each player's GE specific information from server
//
// Created On: 15 Sep 08
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include "ge_character_data.h"
#include "c_ge_playerresource.h"
#include "ge_gamerules.h"
#include "gemp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT(C_GEPlayerResource, DT_GEPlayerResource, CGEPlayerResource)
	RecvPropArray( RecvPropString( RECVINFO( m_szCharIdent[0]) ), m_szCharIdent ),
	RecvPropArray3( RECVINFO_ARRAY(m_iFavWeapon), RecvPropInt( RECVINFO(m_iFavWeapon[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_iDevStatus), RecvPropInt( RECVINFO(m_iDevStatus[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_iCampingPercent), RecvPropInt( RECVINFO(m_iCampingPercent[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_iFrags), RecvPropInt( RECVINFO(m_iFrags[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_iScoreBoardColor), RecvPropInt( RECVINFO(m_iScoreBoardColor[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_bIsActive), RecvPropBool( RECVINFO(m_bIsActive[0]))),
END_RECV_TABLE()

C_GEPlayerResource *g_GePR;

//we export this for the sdk classes
C_PlayerResource * g_PR;

C_GEPlayerResource *GEPlayerRes()
{
	return g_GePR;
}

IGameResources * GameResources( void ) { return g_GePR; }

C_GEPlayerResource::C_GEPlayerResource()
{
	memset( m_iDevStatus, 0, sizeof(m_iDevStatus) );
	memset( m_szCharIdent, '\0', sizeof(m_szCharIdent) );
	memset( m_iCampingPercent, 0, sizeof(m_iCampingPercent) );
	memset( m_iScoreBoardColor, 0, sizeof(m_iScoreBoardColor) );

	m_Colors[TEAM_JANUS]		=	COLOR_JANUS;
	m_Colors[TEAM_MI6]			=	COLOR_MI6;
	m_Colors[TEAM_UNASSIGNED]	=	COLOR_NOTEAM;

	g_GePR = this;
	g_PR = this;
}

C_GEPlayerResource::~C_GEPlayerResource()
{
	g_PR = g_GePR = NULL;
}

const char* C_GEPlayerResource::GetCharIdent( int index )
{
	if ( !IsConnected( index ) )
		return "";

	return m_szCharIdent[index];
}

const char* C_GEPlayerResource::GetCharName( int index )
{
	if ( !IsConnected( index ) || !GECharacters()->Get( m_szCharIdent[index] ) )
		return "";

	return GECharacters()->Get( m_szCharIdent[index] )->szShortName;
}

const int C_GEPlayerResource::GetDevStatus( int index )
{
	if ( !IsConnected( index ) )
		return 0;

	return m_iDevStatus[index];
}

const int C_GEPlayerResource::GetFavoriteWeapon( int index )
{
	if ( !IsConnected( index ) )
		return 0;

	return m_iFavWeapon[index];
}

const int C_GEPlayerResource::GetCampingPercent( int index )
{
	if ( !IsConnected( index ) )
		return 0;

	return m_iCampingPercent[index];
}

int C_GEPlayerResource::GetFrags( int index )
{
	if ( !IsConnected( index ) )
		return 0;

	return m_iFrags[index];
}

const int C_GEPlayerResource::GetScoreBoardColor( int index )
{
	if ( !IsConnected( index ) )
		return 0;

	return m_iScoreBoardColor[index];
}

const bool C_GEPlayerResource::IsActive( int index )
{
	if ( !IsConnected( index ) )
		return false;

	return m_bIsActive[index];
}
