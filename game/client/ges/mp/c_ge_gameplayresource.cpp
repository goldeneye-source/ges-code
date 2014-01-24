///////////// Copyright © 2011, Goldeneye: Source. All rights reserved. /////////////
// 
// File: c_ge_gameplayresource.cpp
// Description:
//      see Header
//
// Created On: 25 Jul 11
// Created By: Jonathan White <killermonkey> 
////////////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include "c_ge_gameplayresource.h"
#include "networkstringtabledefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT(C_GEGameplayResource, DT_GEGameplayResource, CGEGameplayResource)
	// Gameplay Data
	RecvPropString( RECVINFO(m_GameplayIdent) ),
	RecvPropString( RECVINFO(m_GameplayName) ),
	RecvPropInt( RECVINFO(m_GameplayHelp) ),
	RecvPropBool( RECVINFO(m_GameplayOfficial) ),
	RecvPropInt( RECVINFO(m_GameplayRoundNum) ),
	RecvPropFloat( RECVINFO(m_GameplayRoundStart) ),
	RecvPropBool( RECVINFO(m_GameplayIntermission) ),
	// Loadout Data
	RecvPropString( RECVINFO(m_LoadoutIdent) ),
	RecvPropString( RECVINFO(m_LoadoutName) ),
	RecvPropArray3( RECVINFO_ARRAY(m_LoadoutWeapons), RecvPropInt( RECVINFO(m_LoadoutWeapons[0]) ) ),
	// Character Data
	RecvPropString( RECVINFO(m_CharExclusion) ),
END_RECV_TABLE()

// -----------------------------
// Global Variable Definitions
// -----------------------------
CUtlVector<C_GEGameplayResourceListener*> g_geGPRListeners;

C_GEGameplayResource *g_geGR;
C_GEGameplayResource *GEGameplayRes()
{
	return g_geGR;
}

// -------------------
// GameplayResourceListener definition
// -------------------
C_GEGameplayResourceListener::C_GEGameplayResourceListener()
{
	g_geGPRListeners.AddToTail( this );
}

C_GEGameplayResourceListener::~C_GEGameplayResourceListener()
{
	g_geGPRListeners.FindAndRemove( this );
}


// -------------------
// GameplayResource Client definition
// -------------------
C_GEGameplayResource::C_GEGameplayResource()
{
	m_GameplayIdent[0] = m_GameplayName[0] = '\0';
	m_LoadoutIdent[0]  = m_LoadoutName[0]  = '\0';

	m_GameplayHelp = INVALID_STRING_INDEX;
	m_GameplayOfficial = false;
	m_GameplayIntermission = true;
	m_GameplayRoundNum = 0;
	m_GameplayRoundStart = 0;

	memset( m_LoadoutWeapons, WEAPON_NONE, sizeof(m_LoadoutWeapons) );

	g_geGR = this;
}

C_GEGameplayResource::~C_GEGameplayResource()
{
	g_geGR = NULL;
}

void C_GEGameplayResource::OnDataChanged( DataUpdateType_t type )
{
	for ( int i=0; i < g_geGPRListeners.Count(); i++ )
		g_geGPRListeners[i]->OnGameplayDataUpdate();
}

void C_GEGameplayResource::GetLoadoutWeaponList( CUtlVector<int>& list )
{
	list.RemoveAll();
	list.AddMultipleToTail( MAX_WEAPON_SPAWN_SLOTS, m_LoadoutWeapons );
}

int C_GEGameplayResource::GetLoadoutWeapon( int slot )
{
	if ( slot < 0 || slot >= MAX_WEAPON_SPAWN_SLOTS )
		return WEAPON_NONE;

	return m_LoadoutWeapons[slot];
}

void C_GEGameplayResource::GetCharacterExclusion( CUtlVector<char*> &list )
{
	list.PurgeAndDeleteElements();
	Q_SplitString( m_CharExclusion, ",", list );
}
