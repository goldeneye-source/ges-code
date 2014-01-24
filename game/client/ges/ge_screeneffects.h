///////////// Copyright © 2009, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_screeneffects.h
// Description:
//      Post process effects for GoldenEye: Source
//
// Created On: 25 Nov 09
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////

#ifndef GE_SCREENSPACEEFFECTS_H
#define GE_SCREENSPACEEFFECTS_H
#ifdef _WIN32
#pragma once
#endif

#include "ScreenSpaceEffects.h"

class CEntGlowEffect : public IScreenSpaceEffect
{
public:
	CEntGlowEffect( void ) { };

	virtual void Init( void );
	virtual void Shutdown( void );
	virtual void SetParameters( KeyValues *params ) {};
	virtual void Enable( bool bEnable ) {};
	virtual bool IsEnabled( void ) { return true; }

	virtual void RegisterEnt( EHANDLE hEnt, Color glowColor = Color(255,255,255,64), float fMaxDist = 350.0f );
	virtual void DeregisterEnt( EHANDLE hEnt );

	virtual void SetEntColor( EHANDLE hEnt, Color glowColor );
	virtual void SetEntMaxGlowDist( EHANDLE hEnt, float fMaxDist );

	virtual void Render( int x, int y, int w, int h );

protected:
	int FindGlowEnt( EHANDLE hEnt );
	void RenderToStencil( int idx, IMatRenderContext *pRenderContext );
	void RenderToGlowTexture( int idx, IMatRenderContext *pRenderContext );
	float GetGlowAlphaDistMod( int idx );

private:
	struct sGlowEnt
	{
		EHANDLE	m_hEnt;
		float	m_fColor[4];
		float	m_fMaxDistSq;
	};

	CUtlVector<sGlowEnt*>	m_vGlowEnts;

	CTextureReference	m_GlowBuff1;
	CTextureReference	m_GlowBuff2;

	CMaterialReference	m_WhiteMaterial;
	CMaterialReference	m_EffectMaterial;

	CMaterialReference	m_BlurX;
	CMaterialReference	m_BlurY;
};

#endif
