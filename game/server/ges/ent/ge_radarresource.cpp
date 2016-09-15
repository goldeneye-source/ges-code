///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_radarresource.cpp
// Description:
//      Collects radar contacts from Python and sends them to the clients (efficient!)
//
// Created On: 19 Jan 09
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include "ge_radarresource.h"
#include "ge_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern void SendProxy_String_tToString( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );

IMPLEMENT_SERVERCLASS_ST( CGERadarResource, DT_GERadarResource )
	SendPropBool( SENDINFO( m_bForceOn ) ),

	SendPropArray3( SENDINFO_ARRAY3(m_hEnt), SendPropEHandle( SENDINFO_ARRAY(m_hEnt) ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iEntTeam), SendPropInt( SENDINFO_ARRAY(m_iEntTeam) ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_flRangeMod), SendPropFloat( SENDINFO_ARRAY(m_flRangeMod) ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_vOrigin), SendPropVector( SENDINFO_ARRAY(m_vOrigin), -1, SPROP_COORD_MP_LOWPRECISION|SPROP_CHANGES_OFTEN ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iType), SendPropInt( SENDINFO_ARRAY(m_iType), 4 ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iState), SendPropInt( SENDINFO_ARRAY(m_iState), 4, SPROP_CHANGES_OFTEN ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_bAllVisible), SendPropBool( SENDINFO_ARRAY(m_bAllVisible) ) ),
	SendPropArray( SendPropStringT( SENDINFO_ARRAY( m_szIcon ) ), m_szIcon ),
	SendPropArray3( SENDINFO_ARRAY3(m_Color), SendPropInt( SENDINFO_ARRAY(m_Color), 32, SPROP_UNSIGNED, SendProxy_Color32ToInt ) ),

	SendPropArray3( SENDINFO_ARRAY3(m_ObjStatus), SendPropBool( SENDINFO_ARRAY(m_ObjStatus) ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_ObjTeam), SendPropInt( SENDINFO_ARRAY(m_ObjTeam), 4 ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_ObjColor), SendPropInt( SENDINFO_ARRAY(m_ObjColor), 32, SPROP_UNSIGNED, SendProxy_Color32ToInt ) ),
	SendPropArray( SendPropStringT( SENDINFO_ARRAY(m_ObjToken) ), m_ObjToken ),
	SendPropArray( SendPropStringT( SENDINFO_ARRAY(m_ObjText) ), m_ObjText ),
	SendPropArray3( SENDINFO_ARRAY3(m_ObjMinDist), SendPropInt( SENDINFO_ARRAY(m_ObjMinDist), 16 ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_ObjPulse), SendPropBool( SENDINFO_ARRAY(m_ObjPulse) ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CGERadarResource )
	DEFINE_FUNCTION( RadarThink ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( radar_resource, CGERadarResource );

CGERadarResource *g_pRadarResource;

extern ConVar ge_allowradar;

void CGERadarResource::Spawn( void )
{
	m_bForceOn = false;

	for ( int i=0; i < MAX_NET_RADAR_ENTS; i++ )
		ResetContact( i );

	AddEFlags( EFL_KEEP_ON_RECREATE_ENTITIES );

	SetThink( &CGERadarResource::RadarThink );
	SetNextThink( gpGlobals->curtime + RADAR_THINK_INT );
}

int CGERadarResource::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}

void CGERadarResource::SetForceRadar( bool state )
{
	m_bForceOn.Set(state);
}

void CGERadarResource::SetRangeModifier( CBaseEntity *pEnt, float mod )
{
	int pos = FindEntity( pEnt );

	if ( !pEnt || pos == -1 )
		return;

	m_flRangeMod.Set( pos, mod );
}	

void CGERadarResource::AddRadarContact( CBaseEntity *pEnt, int type /*=RADAR_TYPE_DEFAULT*/, bool alwaysVis /*=false*/, const char *icon /*=""*/, Color col /*=Color()*/ )
{
	if ( !pEnt )
		return;

	// If we already exist, just update the contact
	int pos = FindEntity( pEnt );

	// Otherwise, start at the next available position
	if ( pos == -1 )
	{
		pos = FindNextOpenSlot( pEnt );

		// We reached our limit!
		if ( pos == -1 )
			return;

		// Clear any residual
		ResetContact( pos );

		// Initialize relevant fields
		m_hEnt.Set( pos, pEnt );
		m_iEntTeam.Set( pos, pEnt->GetTeamNumber() );
		m_vOrigin.Set( pos, pEnt->GetAbsOrigin() );
	}
	
	// Populate any new fields
	color32 col32 = { col.r(), col.g(), col.b(), col.a() };
	
	m_iType.Set( pos, type );
	m_bAllVisible.Set( pos, alwaysVis );
	m_szIcon.Set( pos, AllocPooledString(icon) );
	m_Color.Set( pos, col32 );

	// Tokens and objectives automatically garner objective status
	if ( type == RADAR_TYPE_OBJECTIVE )
		m_ObjStatus.Set( pos, true );
}

void CGERadarResource::DropRadarContact( CBaseEntity *pEnt )
{
	int pos = FindEntity( pEnt );
	if ( pos != -1 )
	{
		// We never drop a player from the radar ent because they are only added at spawn
		// and they are never "cloaked" so reset their parameters and return
		if ( pEnt && pEnt->IsPlayer() )
		{
			color32 col32 = { 0, 0, 0, 0 };
			m_bAllVisible.Set( pos, false );
			m_flRangeMod.Set( pos, 1.0f );
			m_szIcon.Set( pos, AllocPooledString("") );
			m_Color.Set( pos, col32 );
		}
		else
		{
			// Wipe them off the radar
			m_hEnt.Set( pos, NULL );
			m_iState.Set( pos, RADAR_STATE_NODRAW );
		}

		ResetObjective( pos );
	}
}

bool CGERadarResource::IsRadarContact( CBaseEntity *pEnt )
{
	return FindEntity( pEnt ) != -1;
}

void CGERadarResource::SetupObjective( CBaseEntity *pEnt, int team_filter /*=TEAM_UNASSIGNED*/, const char *token_filter /*=""*/, const char *text /*=""*/, Color color /*=Color()*/, int min_dist /*=0*/, bool pulse /*=false*/ )
{
	int pos = FindEntity( pEnt );
	if ( pos != -1 )
	{
		char text_clip[32];
		Q_strncpy( text_clip, text, 32 );
		color32 c32 = { color.r(), color.g(), color.b(), color.a() };

		m_ObjStatus.Set( pos, true );
		m_ObjTeam.Set( pos, team_filter );
		m_ObjText.Set( pos, AllocPooledString(text_clip) );
		m_ObjToken.Set( pos, AllocPooledString(token_filter) );
		m_ObjColor.Set( pos, c32 );
		m_ObjMinDist.Set( pos, min_dist );
		m_ObjPulse.Set( pos, pulse );
	}
}

void CGERadarResource::SetupObjective2( CBaseEntity *pEnt, int *team_filter, const char *token_filter, const char *text, Color *color, int *min_dist, bool *pulse )
{
	int pos = FindEntity( pEnt );
	if ( pos != -1 )
	{
		m_ObjStatus.Set( pos, true );

		if ( team_filter )
			m_ObjTeam.Set( pos, *team_filter );
		if ( token_filter )
			m_ObjToken.Set( pos, AllocPooledString(token_filter) );
		if ( min_dist )
			m_ObjMinDist.Set( pos, *min_dist );
		if ( pulse )
			m_ObjPulse.Set( pos, *pulse );

		if ( text )
		{
			char text_clip[32];
			Q_strncpy( text_clip, text, 32 );
			m_ObjText.Set( pos, AllocPooledString(text_clip) );
		}

		if ( color )
		{
			color32 c32 = { color->r(), color->g(), color->b(), color->a() };
			m_ObjColor.Set( pos, c32 );
		}
	}
}

void CGERadarResource::ClearObjective( CBaseEntity *pEnt )
{
	int pos = FindEntity( pEnt );
	if ( pos != -1 )
		ResetObjective( pos );
}

bool CGERadarResource::IsObjective( CBaseEntity *pEnt )
{
	int pos = FindEntity( pEnt );
	if ( pos != -1 )
		return m_ObjStatus.Get( pos );
	return false;
}

void CGERadarResource::DropAllContacts( void )
{
	for ( int i=0; i < MAX_NET_RADAR_ENTS; ++i )
	{
		// We pass this off to this function so we don't tamper with player contacts
		DropRadarContact( m_hEnt.Get(i) );
		ResetObjective( i );
	}
}

void CGERadarResource::RadarThink( void )
{
	CBaseEntity *pEnt;	
	for ( int i=0; i < MAX_NET_RADAR_ENTS; ++i )
	{
		// Skip over blanks
		if ( !m_hEnt.Get(i).IsValid() )
			continue;

		// Prune extinct contacts
		pEnt = m_hEnt.Get(i).Get();
		if ( !pEnt )
		{
			ResetContact( i );
			ResetObjective( i );
			continue;
		}

		if ( m_bForceOn || ge_allowradar.GetBool() || m_ObjStatus.Get(i) == true )
		{
			// Update our draw state
			UpdateState(i);

			// Update our location if we will be drawn
			if ( m_iState.Get(i) == RADAR_STATE_DRAW )
			{
				// Update our known position (within 8 units) or any change in z direction
				Vector origin = pEnt->GetAbsOrigin();

				if ( m_vOrigin.Get(i).DistToSqr(origin) >= 64.0f || abs(m_vOrigin.Get(i).z - origin.z) >= 1.0f )
					m_vOrigin.Set( i, origin );
			}
		}
	}

	SetNextThink( gpGlobals->curtime + RADAR_THINK_INT );
}

void CGERadarResource::UpdateState( int pos )
{
	if ( pos < 0 || pos >= MAX_NET_RADAR_ENTS )
		return;

	CBaseEntity *pEnt = m_hEnt.Get(pos).Get();
	if ( pEnt )
	{
		// Update team information
		m_iEntTeam.Set( pos, pEnt->GetTeamNumber() );

		if ( pEnt->IsPlayer() )
		{
			CGEPlayer *pPlayer = ToGEPlayer(pEnt);

			if (!pPlayer->IsAlive() || pPlayer->IsObserver() || pPlayer->GetTeamNumber() == TEAM_SPECTATOR || pPlayer->IsRadarCloaked())
				m_iState.Set( pos, RADAR_STATE_NODRAW );
			else
				m_iState.Set( pos, RADAR_STATE_DRAW );
		}
		else
		{
			if ( pEnt->IsEFlagSet( EF_NODRAW ) )
				m_iState.Set( pos, RADAR_STATE_NODRAW );
			else
				m_iState.Set( pos, RADAR_STATE_DRAW );
		}
	}
	else
		m_iState.Set( pos, RADAR_STATE_NODRAW );
}

int CGERadarResource::FindEntity( EHANDLE hEnt )
{
	if ( !hEnt.IsValid() )
		return -1;

	for ( int i=0; i < MAX_NET_RADAR_ENTS; ++i )
	{
		if ( m_hEnt.Get(i) == hEnt )
			return i;
	}

	return -1;
}

int CGERadarResource::FindNextOpenSlot( CBaseEntity *pEnt )
{
	if ( pEnt->IsPlayer() )
	{
		// Players always occupy their designated slot [0 - gpGlobals->maxClients-1]
		return pEnt->entindex() - 1;
	}
	else
	{
		// If we are not a player start from maxClients and move up
		for ( int i=gpGlobals->maxClients; i < MAX_NET_RADAR_ENTS; ++i )
		{
			if ( !m_hEnt.Get(i).IsValid() )
				return i;
		}
	}
	
	return -1;
}

void CGERadarResource::ResetContact( int pos )
{
	color32 col32 = { 0, 0, 0, 0 };

	m_hEnt.Set( pos, NULL );
	m_iEntTeam.Set( pos, TEAM_UNASSIGNED );
	m_flRangeMod.Set( pos, 1.0f );
	m_vOrigin.Set( pos, Vector() );
	m_iType.Set( pos, 0 );
	m_iState.Set( pos, RADAR_STATE_NONE );
	m_bAllVisible.Set( pos, false );
	m_szIcon.Set( pos, AllocPooledString("") );
	m_Color.Set( pos, col32 );
}

void CGERadarResource::ResetObjective( int pos )
{
	color32 col32 = { 0, 0, 0, 0 };

	m_ObjStatus.Set( pos, false );
	m_ObjTeam.Set( pos, TEAM_UNASSIGNED );
	m_ObjColor.Set( pos, col32 );
	m_ObjText.Set( pos, AllocPooledString("") );
	m_ObjToken.Set( pos, AllocPooledString("") );
	m_ObjMinDist.Set( pos, 0 );
	m_ObjPulse.Set( pos, false );
}
