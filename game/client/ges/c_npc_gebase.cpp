///////////// Copyright © 2012, Goldeneye: Source. All rights reserved. /////////////
// 
// File: c_npc_gebase.cpp
// Description:
//     See header
//
// Created On: 21 Apr 2012
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include "c_npc_gebase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT(C_NPC_GEBase, DT_NPC_GEBase, CNPC_GEBase)
	RecvPropEHandle( RECVINFO( m_hRagdoll ) ),
	RecvPropEHandle( RECVINFO(m_hBotPlayer) ),
	RecvPropBool( RECVINFO(m_bSpawnInterpCounter) ),
END_RECV_TABLE()

C_NPC_GEBase::C_NPC_GEBase()
{
	m_bSpawnInterpCounterCache = false;
}

bool C_NPC_GEBase::ShouldDraw()
{
	if ( !IsAlive() )
		return false;

	return BaseClass::ShouldDraw();
}

int C_NPC_GEBase::DrawModel( int flags )
{
	if ( !m_bReadyToDraw )
		return 0;

	return BaseClass::DrawModel(flags);
}

void C_NPC_GEBase::OnDataChanged( DataUpdateType_t type )
{
	UpdateVisibility();
	BaseClass::OnDataChanged( type );
}

void C_NPC_GEBase::PostDataUpdate( DataUpdateType_t updateType )
{
	if ( m_bSpawnInterpCounter != m_bSpawnInterpCounterCache )
	{
		MoveToLastReceivedPosition( true );
		ResetLatched();
		m_bSpawnInterpCounterCache = m_bSpawnInterpCounter;
	}

	BaseClass::PostDataUpdate( updateType );
}

ShadowType_t C_NPC_GEBase::ShadowCastType( void )
{
	if ( !IsVisible() )
		return SHADOWS_NONE;

	return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
}