///////////// Copyright © 2011, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ent_capturearea.h
// Description:
//      A generic trigger area for "capturing" things in gameplay
//
// Created On: 01 Aug 11
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#ifndef ENT_CAPTUREAREA_H
#define ENT_CAPTUREAREA_H
#ifdef _WIN32
#pragma once
#endif

#include "baseanimating.h"

class CGECaptureArea : public CBaseAnimating
{
public:
	DECLARE_CLASS( CGECaptureArea, CBaseAnimating );
	DECLARE_SERVERCLASS();

	CGECaptureArea();

	virtual void Spawn();
	virtual void Precache();
	virtual void UpdateOnRemove();

	virtual void Touch( CBaseEntity *pOther );
	virtual void EndTouch( CBaseEntity *pOther );

	void ClearTouchingEntities();

	// TokenManager Settings
	void SetGroupName( const char* name );
	const char *GetGroupName() { return m_szGroupName; }
	void SetRadius( float radius );

	void SetupGlow( bool state, Color glowColor = Color(255,255,255), float glowDist = 350.0f );

private:
#ifdef CLIENT_DLL
	CEntGlowEffect *m_pEntGlowEffect;
#endif

	CNetworkVar( bool, m_bEnableGlow );
	CNetworkVar( int, m_GlowColor );
	CNetworkVar( float, m_GlowDist );

	char m_szGroupName[32];
	float m_fRadius;

	// Entities currently being touched by this capture area
	CUtlVector< EHANDLE >	m_hTouchingEntities;
};

#endif
