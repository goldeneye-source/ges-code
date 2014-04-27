///////////// Copyright � 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_materialproxies.cpp
// Description:
//      Custom Material Proxies for use in GE: Source
//
// Created On: 18 Dec 08
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "materialsystem/imaterialproxy.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/itexture.h"
#include "functionproxy.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//proxy to change frame number over time.
class CFrameScrollMP : public IMaterialProxy
{
public:
	CFrameScrollMP();
	virtual ~CFrameScrollMP();
	virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
	virtual void OnBind( void *pC_BaseEntity );
	virtual void Release( void ) { delete this; }
	virtual IMaterial *GetMaterial( void ) { return m_pMaterial; };

private:
	IMaterialVar *m_pTextureScrollVar;

	CFloatInput m_FrameMax;
	CFloatInput m_FrameOffset;
	CFloatInput m_FrameRunTime;
	CFloatInput m_FrameDelay;
	CFloatInput m_FrameLoop;

	float m_fStartTime;
	bool m_bNeedReset;

	IMaterial *m_pMaterial;

#ifdef _XBOX
	bool m_bWaterShader;
#endif
};

CFrameScrollMP::CFrameScrollMP()
{
	m_pTextureScrollVar = NULL;
	m_pMaterial = NULL;
	m_bNeedReset = false;

#ifdef _XBOX
	m_bWaterShader = false;
#endif
}

CFrameScrollMP::~CFrameScrollMP()
{
}

bool CFrameScrollMP::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
	m_pMaterial = pMaterial;

	char const* pScrollVarName = pKeyValues->GetString( "frameVar" );
	if( !pScrollVarName )
		return false;

	bool foundVar;
	m_pTextureScrollVar = pMaterial->FindVar( pScrollVarName, &foundVar, true );
	if( !foundVar )
		return false;

	m_FrameMax.Init( pMaterial, pKeyValues, "frameMax", 1.0f );
	m_FrameOffset.Init( pMaterial, pKeyValues, "frameOffset", 0.0f );
	m_FrameRunTime.Init( pMaterial, pKeyValues, "frameRunTime", 5.0f );
	m_FrameDelay.Init( pMaterial, pKeyValues, "frameDelay", 0.0f );
	m_FrameLoop.Init( pMaterial, pKeyValues, "frameLoop", 0.0f );

	m_fStartTime = gpGlobals->curtime;

#ifdef _XBOX
	const char *pShaderName = pMaterial->GetShaderName();
	m_bWaterShader = !Q_stricmp( pShaderName, "water" ) ||
		!Q_stricmp( pShaderName, "water_dx80" ) ||	!Q_stricmp( pShaderName, "water_dudv" ) ||
		!Q_stricmp( pShaderName, "water_firstpass" ) ||	!Q_stricmp( pShaderName, "water_secondpass" );
#endif

	return true;
}

void CFrameScrollMP::OnBind( void *pC_BaseEntity )
{
	if( !m_pTextureScrollVar )
	{
		return;
	}
/*
	C_BaseEntity* pPlayer = C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	if ( !pPlayer->IsAlive() )
	{
		m_fStartTime = gpGlobals->curtime;
		m_bNeedReset = false;
	}
	else if ( pPlayer->IsAlive() )
		m_bNeedReset = true;
*/
	float max, offset, runTime, delay, loop;

	// set default values if these variables don't exist.
	max			= m_FrameMax.GetFloat();
	offset		= m_FrameOffset.GetFloat();
	runTime		= m_FrameRunTime.GetFloat();
	delay		= m_FrameDelay.GetFloat();
	loop		= m_FrameLoop.GetFloat();

#ifdef _XBOX
	// Hack for water
	if ( m_bWaterShader )
	{
		scale = 0.5f;
	}
#endif

	float time, count, frac;
	
	count = gpGlobals->curtime - m_fStartTime;
	
	if ( count <= delay )
	{
		m_pTextureScrollVar->SetIntValue(0);
		return;
	}

	time = count - delay;

	// if over time period restart
	if ( time > runTime )
	{
		if ( loop )
			m_fStartTime = gpGlobals->curtime;
		else
			time = runTime;
	}

	//work out the fraction for it
	if ( time > 0 )
		frac = time / runTime;
	else 
		frac = 0;

	frac = clamp(frac, 0, 1);

	//Msg("Setting frame to: %d\n", (int)(max * frac));
	m_pTextureScrollVar->SetIntValue((int)(max * frac));
}

EXPOSE_INTERFACE( CFrameScrollMP, IMaterialProxy, "GEFrameScroll" IMATERIAL_PROXY_INTERFACE_VERSION );
