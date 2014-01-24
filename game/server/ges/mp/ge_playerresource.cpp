///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_playerresource.cpp
// Description:
//      Collects each player's GE specific information for transmission to all clients
//
// Created On: 15 Sep 08
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include "ge_player.h"
#include "gemp_player.h"
#include "ge_character_data.h"
#include "ge_playerresource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_SERVERCLASS_ST(CGEPlayerResource, DT_GEPlayerResource)
	SendPropArray( SendPropStringT( SENDINFO_ARRAY( m_szCharIdent ) ), m_szCharIdent ),
	SendPropArray3( SENDINFO_ARRAY3(m_iFavWeapon), SendPropInt( SENDINFO_ARRAY(m_iFavWeapon), 8 ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iDevStatus), SendPropInt( SENDINFO_ARRAY(m_iDevStatus), 8 ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iCampingPercent), SendPropInt( SENDINFO_ARRAY(m_iCampingPercent), 8, SPROP_CHANGES_OFTEN ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iFrags), SendPropInt( SENDINFO_ARRAY(m_iFrags) ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iScoreBoardColor), SendPropInt( SENDINFO_ARRAY(m_iScoreBoardColor), 4 ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_bIsActive), SendPropBool( SENDINFO_ARRAY(m_bIsActive) ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CGEPlayerResource )
END_DATADESC()

LINK_ENTITY_TO_CLASS( player_manager, CGEPlayerResource );

CGEPlayerResource *g_pPlayerResource;

void CGEPlayerResource::Spawn( void )
{
	for ( int i=0; i <= MAX_PLAYERS; i++ )
	{
		m_szCharIdent.Set( i, AllocPooledString("") );
		m_iFavWeapon.Set( i, WEAPON_NONE );
		m_iDevStatus.Set( i, 0 );
		m_iCampingPercent.Set( i, 0 );
		m_iFrags.Set( i, 0 );
		m_iScoreBoardColor.Set( i, SB_COLOR_NORMAL );
	}

	BaseClass::Spawn();
}

void CGEPlayerResource::UpdatePlayerData( void )
{
	BaseClass::UpdatePlayerData();

	string_t nochar = AllocPooledString("nochar");

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CGEMPPlayer *pPlayer = ToGEMPPlayer(UTIL_PlayerByIndex( i ));
		
		if ( pPlayer && pPlayer->IsConnected() )
		{
			// Use GE's scoring system instead of kills (Python support)
			// TODO: This might not work well with our event driven system
			if ( GEGameplay()->IsInFinalIntermission() )
			{
				m_iScore.Set( i, pPlayer->GetMatchScore() );
				m_iDeaths.Set( i, pPlayer->GetMatchDeaths() );
			}
			else
			{
				m_iScore.Set( i, pPlayer->GetRoundScore() );
			}

			m_iFavWeapon.Set( i, pPlayer->GetFavoriteWeapon() );
			m_iDevStatus.Set( i, pPlayer->GetDevStatus() );
			m_iCampingPercent.Set( i, pPlayer->GetCampingPercent() );
			m_iFrags.Set( i, pPlayer->FragCount() );
			m_iScoreBoardColor.Set( i, pPlayer->GetScoreBoardColor() );
			m_bIsActive.Set( i, pPlayer->IsActive() );

			// Send off our current character!
			const CGECharData *mychar = GECharacters()->Get( pPlayer->GetCharIndex() );
			string_t ident = nochar;
			if ( mychar )
				ident = AllocPooledString(mychar->szIdentifier);

			m_szCharIdent.Set( i, ident );
		}
	}
}

int CGEPlayerResource::GetPing( int entindex )
{
	if ( entindex >=0 && entindex <= MAX_PLAYERS )
		return m_iPing[entindex];
	else
		return 0;
}