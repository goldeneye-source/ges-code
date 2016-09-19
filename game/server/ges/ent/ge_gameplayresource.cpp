///////////// Copyright © 2011, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_gameplayresource.cpp
// Description:
//      see Header
//
// Created On: 25 Jul 11
// Created By: Jonathan White <killermonkey> 
////////////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include "ge_gameplayresource.h"
#include "ge_weapon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern void SendProxy_String_tToString( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );

IMPLEMENT_SERVERCLASS_ST( CGEGameplayResource, DT_GEGameplayResource )
	// Gameplay Data
	SendPropString( SENDINFO(m_GameplayIdent) ),
	SendPropString( SENDINFO(m_GameplayName) ),
	SendPropInt( SENDINFO(m_GameplayHelp) ),
	SendPropBool( SENDINFO(m_GameplayOfficial) ),
	SendPropInt( SENDINFO(m_GameplayRoundNum) ),
	SendPropFloat( SENDINFO(m_GameplayRoundStart) ),
	SendPropBool( SENDINFO(m_GameplayIntermission) ),
	// Loadout Data
	SendPropString( SENDINFO(m_LoadoutIdent) ),
	SendPropString( SENDINFO(m_LoadoutName) ),
	SendPropArray3( SENDINFO_ARRAY3(m_LoadoutWeapons), SendPropInt( SENDINFO_ARRAY(m_LoadoutWeapons) ) ),
	// Character Data
	SendPropString( SENDINFO(m_CharExclusion) ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( gameplay_resource, CGEGameplayResource );

CGEGameplayResource *g_pGameplayResource = NULL;

void CGEGameplayResource::Spawn( void )
{
	BaseClass::Spawn();
	AddEFlags( EFL_KEEP_ON_RECREATE_ENTITIES );
}

//-----------------------------------------------------------------------------
// Purpose: The gameplay resource is always transmitted to clients
//-----------------------------------------------------------------------------
int CGEGameplayResource::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}

void CGEGameplayResource::OnGameplayEvent( GPEvent event )
{
	if ( event == SCENARIO_INIT )
	{
		CGEBaseScenario *pScenario = GEGameplay()->GetScenario();
		if ( !pScenario )
			return;

		Q_strncpy( m_GameplayIdent.GetForModify(), pScenario->GetIdent(),	  32 );
		Q_strncpy( m_GameplayName.GetForModify(),  pScenario->GetPrintName(), 64 );
		m_GameplayOfficial = pScenario->IsOfficial();
		m_GameplayIntermission = true;
		m_GameplayRoundNum = 0;
	}
	else if ( event == SCENARIO_POST_INIT )
	{
		CGEBaseScenario *pScenario = GEGameplay()->GetScenario();
		if (!pScenario)
			return;

		m_GameplayHelp = pScenario->GetHelpString();
	}
	else if ( event == ROUND_START )
	{
		m_GameplayIntermission = false;
		m_GameplayRoundStart = gpGlobals->curtime;
		m_GameplayRoundNum++;
	}
	else if ( event == ROUND_END || event == MATCH_END )
	{
		m_GameplayIntermission = true;
	}
}

void CGEGameplayResource::OnLoadoutChanged( const char *ident, const char *name, CUtlVector<int>& weapons )
{
	Q_strncpy( m_LoadoutIdent.GetForModify(), ident, 32 );
	Q_strncpy( m_LoadoutName.GetForModify(),  name,  64 );
	
	int count = weapons.Count();
	for ( int i=0; i < MAX_WEAPON_SPAWN_SLOTS; i++ )
	{
		if ( i < count )
			m_LoadoutWeapons.Set( i, weapons[i] );
		else
			m_LoadoutWeapons.Set( i, WEAPON_NONE );
	}
}

void CGEGameplayResource::SetCharacterExclusion( const char *str_list )
{
	char tmp[64];
	int i, k;
	for ( i=0, k=0; i < 64; i++ )
	{
		if ( str_list[i] == '\0' )
			break;
		
		// Only allow [0-9], [A-Z], [a-z], and [,_]
		if ( (str_list[i] >= 48 && str_list[i] <= 57)  ||
			 (str_list[i] >= 65 && str_list[i] <= 90)  || 
			 (str_list[i] >= 97 && str_list[i] <= 122) ||
			 str_list[i] == 44 || str_list[i] == 95 )
		{
			tmp[k] = str_list[i];
			k++;
		}
	}

	tmp[MIN(k,63)] = '\0';
	Q_strncpy( m_CharExclusion.GetForModify(), tmp, 64 );
}
