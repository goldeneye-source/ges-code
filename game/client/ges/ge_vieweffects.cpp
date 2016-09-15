///////////// Copyright © 2010, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_vieweffects.cpp
// Description:
//      Adds client view effects for GE (namely breathing effect)
//
// Created On: 12 Jan 2010
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ivieweffects.h"
#include "view_shared.h"
#include "iviewrender.h"
#include "viewrender.h"
#include "view.h"

#include "ge_utils.h"
#include "ge_shareddefs.h"
#include "ge_vieweffects.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define CROSSHAIR_SCALE	0.5f

static CGEViewEffects g_ViewEffects;
IViewEffects *vieweffects = ( IViewEffects * )&g_ViewEffects;

#define ATTR_TIMEOUT 3.0f
#define ATTR_ACCELTIME ATTR_TIMEOUT * 0.35f
#define ATTR_DEACCELTIME ATTR_TIMEOUT * 0.75f

CGEViewEffects::CGEViewEffects( void )
{
	ResetBreath();
	m_MatCrosshair = NULL;
}

CGEViewEffects::~CGEViewEffects( void )
{
	if ( m_MatCrosshair )
	{
		m_MatCrosshair->DecrementReferenceCount();
		m_MatCrosshair = NULL;
	}
}

// Called from cdll_client_int.cpp in CHLClient::HudVidInit()
void CGEViewEffects::VidInit( void )
{
	// Reset the pixel scale to our new dimensions
	m_flPixelScale = XRES( 28.0f );

	// Reload the crosshair material
	if ( m_MatCrosshair )
	{
		m_MatCrosshair->DecrementReferenceCount();
		m_MatCrosshair = NULL;
	}

	m_MatCrosshair = materials->FindMaterial( "sprites/crosshair", TEXTURE_GROUP_VGUI );
	m_MatCrosshair->IncrementReferenceCount();
}

void CGEViewEffects::LevelInit( void )
{
	// Reset our breath state
	ResetBreath();
	CViewEffects::LevelInit();
}

void CGEViewEffects::ResetBreath( void )
{
	m_iNextAttractor = 0;
	m_flAttractorTimeout = 0;

	for ( int i=0; i < NUM_ATTRACTORS; i++ )
		GenerateRandomAttractor( m_vAttractors[i] );

	m_vBO = vec3_origin;
	m_vOldBO = vec3_origin;
}

void CGEViewEffects::ApplyBreath( void )
{
	if ( gpGlobals->curtime >= m_flAttractorTimeout )
	{
		GenerateRandomAttractor( m_vAttractors[m_iNextAttractor] );
		m_iNextAttractor = m_iNextAttractor == (NUM_ATTRACTORS-1) ? 0 : ++m_iNextAttractor;
		m_flAttractorTimeout = m_vAttractors[m_iNextAttractor].spawntime + ATTR_TIMEOUT;
	}

	m_vOldBO = m_vBO;

	float pull, timealive, influence;
	Vector newBO = m_vBO, dir;
	for ( int i=0; i < NUM_ATTRACTORS; i++ )
	{
		dir = m_vAttractors[i].attr - m_vBO;
		timealive = gpGlobals->curtime - m_vAttractors[i].spawntime;

		// Figure out our influence (deaccelerate after half of our lifetime
		if ( timealive >= ATTR_DEACCELTIME )
			influence = RemapValClamped( timealive, ATTR_DEACCELTIME, ATTR_TIMEOUT, 1.15f, 0.4f );
		else if ( timealive <= ATTR_ACCELTIME )
			influence = RemapValClamped( timealive, 0, ATTR_ACCELTIME, 0.1f, 1.15f );
		else
			influence = 1.15f;

		// Calculate our pull by taking an influenced step 
		pull = influence * m_vAttractors[i].stepsize * dir.NormalizeInPlace();
		
		VectorMA( newBO, pull, dir, newBO );
	}

	m_vBO = newBO;
}

void CGEViewEffects::BreathMouseMove( float *x, float *y )
{
	C_BasePlayer *pLocal = C_BasePlayer::GetLocalPlayer();

	if ( pLocal && !pLocal->IsObserver() && pLocal->IsAlive() )
	{
		// Simply take the distance of m_vBO from the center of the screen and add it to x and y :-D
		*y += (m_vBO[PITCH] - m_vOldBO[PITCH]) * m_flPixelScale;
		*x += (m_vBO[YAW] - m_vOldBO[YAW]) * m_flPixelScale;
	}
}

void CGEViewEffects::GenerateRandomAttractor( attractor_t &attractor )
{
	float radius = GERandom<float>( 0.64f );
	float angle = GERandom<float>( M_TWOPI );

	attractor.attr[PITCH] = radius * cos( angle );
	attractor.attr[YAW] = 1.3333 * radius * sin( angle );
	attractor.attr[ROLL] = 0;

	attractor.spawntime = gpGlobals->curtime;
	attractor.stepsize = (m_vBO.DistTo( attractor.attr ) / ATTR_TIMEOUT) * gpGlobals->frametime;
}

// Called from RenderGEPlayerSprites() in clientmode_ge.cpp
// Draw a 3D crosshair scaled by our current zoom
void CGEViewEffects::DrawCrosshair( void )
{
	C_GEPlayer *pPlayer = ToGEPlayer( C_BasePlayer::GetLocalPlayer() );

	// Check our draw conditions
	if ( ShouldDrawCrosshair( pPlayer ) )
	{
		// Default size
		float size = YRES( 50 );

		// Base offset from the camera
		Vector offset( 1200.0f, 0, 0 );

		// Adjust distance for zoom
		float zoom = pPlayer->GetZoom() + pPlayer->GetDefaultFOV();
		if (zoom < 90.0f )
			offset.x *= 1 / tan( DEG2RAD(zoom) / 2.0f );

		int alpha = 255;

		if (!pPlayer->IsInAimMode() && !pPlayer->IsObserver())
			alpha = (int)clamp((1 - (pPlayer->GetFullyZoomedTime() - gpGlobals->curtime) / GE_AIMMODE_DELAY) * 255, 0, 255); //The clamp is important because zooming weapons can have longer aimmode delays.

		GEUTIL_DrawSprite3D( m_MatCrosshair, offset, size, size, alpha );
	}
}

bool CGEViewEffects::ShouldDrawCrosshair( C_GEPlayer *pPlayer )
{
	if ( !pPlayer )
		return false;

	// Check observer logic
	if ( pPlayer->IsObserver() )
	{
		C_GEPlayer *obsTarget = ToGEPlayer( pPlayer->GetObserverTarget() );
		if ( obsTarget && obsTarget->IsPlayer() )
		{
			// Only show crosshair if we are observing "In Eye"
			if ( obsTarget->StartedAimMode() && pPlayer->GetObserverMode() == OBS_MODE_IN_EYE )
				return true;
		}

		return false;
	}

	// Only draw the crosshair when we are actually established in aim mode
	if ( pPlayer->StartedAimMode() )
		return true;

	return false;
}
