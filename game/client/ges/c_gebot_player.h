///////////// Copyright © 2011, Goldeneye: Source. All rights reserved. /////////////
// 
// File: c_gebot_player.h
// Description:
//     Client-side implementation of a bot player proxy
//
// Created On: 30 Sep 11
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#ifndef GEBOT_PLAYER_H
#define GEBOT_PLAYER_H
#pragma once

#include "c_gemp_player.h"
#include "c_ai_basenpc.h"

class C_GEBotPlayer : public C_GEMPPlayer
{
public:
	DECLARE_CLASS( C_GEBotPlayer, C_GEMPPlayer );

	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	virtual bool  IsBotPlayer()	{ return true; }
	C_AI_BaseNPC* GetNPC()		{ return m_pNPC; }

	virtual void AdjustCollisionBounds( bool inflate );
	virtual void PostDataUpdate( DataUpdateType_t updateType );
	virtual bool ShouldDraw();
private:
	CHandle<C_AI_BaseNPC> m_pNPC;
};

class C_GENPCRagdoll : public C_BaseAnimatingOverlay
{
public:
	DECLARE_CLASS( C_GENPCRagdoll, C_BaseAnimatingOverlay );
	DECLARE_CLIENTCLASS();

	C_GENPCRagdoll();
	~C_GENPCRagdoll();

	virtual void OnDataChanged( DataUpdateType_t type );

	virtual int BloodColor() { return BLOOD_COLOR_RED; }

	int GetPlayerEntIndex() const;
	IRagdoll* GetIRagdoll() const;

	void ImpactTrace( trace_t *pTrace, int iDamageType, char *pCustomImpactName );
	void UpdateOnRemove( void );
	virtual void SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights );

private:

	C_GENPCRagdoll( const C_GENPCRagdoll & ) {}

	void Interp_Copy( C_BaseAnimatingOverlay *pDestinationEntity );
	void CreateRagdoll( void );

private:

	EHANDLE	m_hNPC;
	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
};

C_GEBotPlayer *ToGEBotPlayer( CBaseEntity *pEntity );

#endif