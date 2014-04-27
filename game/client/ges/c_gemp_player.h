///////////// Copyright © 2008 LodleNet. All rights reserved. /////////////
//
//   Project     : Client (GES)
//   File        : c_gemp_player.h
//   Description :
//      [TODO: Write the purpose of c_gemp_player.h.]
//
//   Created On: 2/20/2009 5:38:40 PM
//   Created By: Mark Chandler <mailto:admin@lodle.net>
////////////////////////////////////////////////////////////////////////////

#ifndef MC_C_GEMP_PLAYER_H
#define MC_C_GEMP_PLAYER_H
#ifdef _WIN32
#pragma once
#endif

#include "c_ge_player.h"

class C_GEMPPlayer : public C_GEPlayer
{
public:
	DECLARE_CLASS( C_GEMPPlayer, C_GEPlayer );

	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	C_GEMPPlayer();

	virtual bool ShouldCollide( int collisionGroup, int contentsMask ) const; // Defined in gemp_player_shared.cpp
	virtual void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );

	bool IsMPPlayer() { return true; }
	bool IsSPPlayer() { return false; }

	float GetNextJumpTime()				{ return m_flNextJumpTime; }
	void SetNextJumpTime( float time )	{ m_flNextJumpTime = time; }

	void SetStartJumpZ( float val )		{ m_flStartJumpZ=val; }
	float GetStartJumpZ()				{ return m_flStartJumpZ; }

	// Observer calcs overrides for bots
	virtual Vector GetChaseCamViewOffset( CBaseEntity *target );

private:
	float m_flStartJumpZ;
	float m_flNextJumpTime;
};

C_GEMPPlayer *ToGEMPPlayer( CBaseEntity *pEntity );

#endif //MC_C_GEMP_PLAYER_H
