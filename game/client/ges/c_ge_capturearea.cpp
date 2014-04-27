///////////// Copyright © 2013, Goldeneye: Source. All rights reserved. /////////////
// 
// File: c_ge_capturearea.cpp
// Description:
//      Implements glow for capture areas on clients
//
// Created On: 28 Mar 2013
// Created By: Jonathan White <killermonkey> 
////////////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include "ge_screeneffects.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_GECaptureArea : public C_BaseAnimating
{
	DECLARE_CLASS( C_GECaptureArea, C_BaseAnimating );

public:
	DECLARE_CLIENTCLASS();

	C_GECaptureArea();

	virtual void PostDataUpdate( DataUpdateType_t updateType );
	virtual void UpdateVisibility();

private:
	CEntGlowEffect *m_pEntGlowEffect;

	bool m_bEnableGlow;
	bool m_bEnableGlowCache;

	int m_GlowColor;
	float m_GlowDist;
};

IMPLEMENT_CLIENTCLASS_DT( C_GECaptureArea, DT_GECaptureArea, CGECaptureArea )
	RecvPropBool( RECVINFO(m_bEnableGlow) ),
	RecvPropInt( RECVINFO(m_GlowColor) ),
	RecvPropFloat( RECVINFO(m_GlowDist) ),
END_RECV_TABLE()

C_GECaptureArea::C_GECaptureArea()
{
	m_GlowColor = 0;
	m_GlowDist = 250.0f;
	m_bEnableGlow = false;
	m_bEnableGlowCache = false;

	m_pEntGlowEffect = (CEntGlowEffect*) g_pScreenSpaceEffects->GetScreenSpaceEffect( "ge_entglow" );
}

void C_GECaptureArea::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );

	if ( m_bEnableGlowCache != m_bEnableGlow )
	{
		if ( m_bEnableGlow )
		{
			m_pEntGlowEffect->RegisterEnt( this, m_GlowColor, m_GlowDist );
		}
		else
		{
			m_pEntGlowEffect->DeregisterEnt( this );
		}

		m_bEnableGlowCache = m_bEnableGlow;
	}
	else
	{
		m_pEntGlowEffect->SetEntColor( this, Color( m_GlowColor ) );
		m_pEntGlowEffect->SetEntMaxGlowDist( this, m_GlowDist );
	}
}

void C_GECaptureArea::UpdateVisibility( void )
{
	// If we are glowing always make us visible so that we can draw the glow event
	if ( m_bEnableGlow )
		AddToLeafSystem();
	else
		BaseClass::UpdateVisibility();
}