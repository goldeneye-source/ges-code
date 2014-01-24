///////////// Copyright © 2012, Goldeneye: Source. All rights reserved. /////////////
// 
// File: c_npc_gebase.h
// Description:
//     Client-side implementation of the base generic NPC (mainly for conversions)
//
// Created On: 21 Apr 2012
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#ifndef C_NPC_GEBASE_H
#define C_NPC_GEBASE_H
#pragma once

#include "c_gebot_player.h"
#include "c_ai_basenpc.h"

class C_NPC_GEBase : public C_AI_BaseNPC
{
public:
	C_NPC_GEBase();

	DECLARE_CLASS( C_NPC_GEBase, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

	C_GEBotPlayer *GetBotPlayer() { return m_hBotPlayer.Get(); }

	// Tweak this at some point to fix in-eye spectating of bots
	virtual const Vector &GetViewOffset() { return BaseClass::GetViewOffset(); }
	virtual ShadowType_t ShadowCastType( void );

	virtual int  DrawModel( int flags );
	virtual bool ShouldDraw();

	virtual void OnDataChanged( DataUpdateType_t type );
	virtual void PostDataUpdate( DataUpdateType_t updateType );

private:
	bool m_bSpawnInterpCounter;
	bool m_bSpawnInterpCounterCache;
	CHandle<C_BaseEntity>  m_hRagdoll;
	CHandle<C_GEBotPlayer> m_hBotPlayer;
};

#endif
