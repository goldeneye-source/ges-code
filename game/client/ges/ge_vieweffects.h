///////////// Copyright � 2010, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_vieweffects.h
// Description:
//      Adds client view effects for GE (namely breathing effect)
//
// Created On: 12 Jan 2010
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#ifndef GE_VIEWEFFECTS_H
#define GE_VIEWEFFECTS_H
#ifdef _WIN32
#pragma once
#endif

#include "c_ge_player.h"
#include "view_effects.h"

#define NUM_ATTRACTORS 3

struct attractor_t
{
	Vector attr;
	float spawntime;
	float stepsize;
};

class CGEViewEffects : public CViewEffects
{
public:
	CGEViewEffects();
	~CGEViewEffects();

	virtual void VidInit();
	virtual void LevelInit();

	// Breath Effect Functions
	void ResetBreath();
	void ApplyBreath();
	void BreathMouseMove( float *x, float *y );

	// Crosshair Functions
	void DrawCrosshair();

	// Objective Functions -- Future!

	// Teamplay Functions -- Future!

protected:
	void GenerateRandomAttractor( attractor_t &attr );
	bool ShouldDrawCrosshair( C_GEPlayer *pPlayer );

private:
	// Breath effect variables
	int			m_iNextAttractor;
	float		m_flAttractorTimeout;
	attractor_t	m_vAttractors[NUM_ATTRACTORS];
	Vector		m_vOldBO;
	Vector		m_vBO;

	float		m_flPixelScale;

	// Crosshair variables
	IMaterial		*m_MatCrosshair;
};

extern CGEViewEffects g_ViewEffects;
inline CGEViewEffects *GEViewEffects( void ) { return &g_ViewEffects; }

#endif
