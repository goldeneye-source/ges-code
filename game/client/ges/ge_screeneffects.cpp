///////////// Copyright � 2009, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_screeneffects.cpp
// Description:
//      Post process effects for GoldenEye: Source
//
// Created On: 25 Nov 09
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "rendertexture.h"
#include "model_types.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterialvar.h"
#include "cdll_client_int.h"
#include "materialsystem/itexture.h"
#include "KeyValues.h"
#include "clienteffectprecachesystem.h"

#include "ge_screeneffects.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ADD_SCREENSPACE_EFFECT( CEntGlowEffect, ge_entglow );

static float rBlack[4] = {0,0,0,1};
static float rWhite[4] = {1,1,1,1};

void CEntGlowEffect::Init( void ) 
{
	// Initialize the white overlay material to render our model with
	KeyValues *pVMTKeyValues = new KeyValues( "VertexLitGeneric" );
	pVMTKeyValues->SetString( "$basetexture", "vgui/white" );
	pVMTKeyValues->SetInt( "$selfillum", 1 );
	pVMTKeyValues->SetString( "$selfillummask", "vgui/white" );
	pVMTKeyValues->SetInt( "$vertexalpha", 1 );
	m_WhiteMaterial.Init( "__geglowwhite", TEXTURE_GROUP_CLIENT_EFFECTS, pVMTKeyValues );
	m_WhiteMaterial->Refresh();

	// Initialize the Effect material that will be blitted to the Frame Buffer
	KeyValues *pVMTKeyValues2 = new KeyValues( "UnlitGeneric" );
	pVMTKeyValues2->SetString( "$basetexture", "_rt_FullFrameFB" );
	pVMTKeyValues2->SetInt( "$additive", 1 );
	m_EffectMaterial.Init( "__geglowcomposite", TEXTURE_GROUP_CLIENT_EFFECTS, pVMTKeyValues2 );
	m_EffectMaterial->Refresh();

	// Initialize render targets for our blurring
	m_GlowBuff1.InitRenderTarget( ScreenWidth()/2, ScreenHeight()/2, RT_SIZE_DEFAULT, IMAGE_FORMAT_RGBA8888, MATERIAL_RT_DEPTH_SEPARATE, false, "_rt_geglowbuff1" );
	m_GlowBuff2.InitRenderTarget( ScreenWidth()/2, ScreenHeight()/2, RT_SIZE_DEFAULT, IMAGE_FORMAT_RGBA8888, MATERIAL_RT_DEPTH_SEPARATE, false, "_rt_geglowbuff2" );
	
	// Load the blur textures
	m_BlurX.Init( materials->FindMaterial("pp/ge_blurx", TEXTURE_GROUP_OTHER, true) );
	m_BlurY.Init( materials->FindMaterial("pp/ge_blury", TEXTURE_GROUP_OTHER, true) );
}

void CEntGlowEffect::Shutdown( void )
{
	// Tell the materials we are done referencing them
	m_WhiteMaterial.Shutdown();
	m_EffectMaterial.Shutdown();

	m_GlowBuff1.Shutdown();
	m_GlowBuff2.Shutdown();

	m_BlurX.Shutdown();
	m_BlurY.Shutdown();
}

//-----------------------------------------------------------------------------
// Purpose: Render the effect
//-----------------------------------------------------------------------------
void CEntGlowEffect::Render( int x, int y, int w, int h )
{
	if ( !m_vGlowEnts.Count() )
		return;

	// Grab the render context
	CMatRenderContextPtr pRenderContext( materials );

	// Apply our glow buffers as the base textures for our blurring operators
	IMaterialVar *var;
	// Set our effect material to have a base texture of our primary glow buffer
	var = m_BlurX->FindVar( "$basetexture", NULL );
	var->SetTextureValue( m_GlowBuff1 );
	var = m_BlurY->FindVar( "$basetexture", NULL );
	var->SetTextureValue( m_GlowBuff2 );
	var = m_EffectMaterial->FindVar( "$basetexture", NULL );
	var->SetTextureValue( m_GlowBuff1 );

	var = m_BlurX->FindVar( "$bloomscale", NULL );
	var->SetFloatValue( 2.0f );
	var = m_BlurY->FindVar( "$bloomamount", NULL );
	var->SetFloatValue( 2.0f );

	// Clear the glow buffer from the previous iteration
	pRenderContext->ClearColor4ub( 0, 0, 0, 255 );
	pRenderContext->PushRenderTargetAndViewport( m_GlowBuff1 );
	pRenderContext->ClearBuffers( true, true );
	pRenderContext->PopRenderTargetAndViewport();

	pRenderContext->PushRenderTargetAndViewport( m_GlowBuff2 );
	pRenderContext->ClearBuffers( true, true );
	pRenderContext->PopRenderTargetAndViewport();

	// Clear the stencil buffer in case someone dirtied it this frame
	pRenderContext->ClearStencilBufferRectangle( 0, 0, ScreenWidth(), ScreenHeight(), 0 );
	
	// Iterate over our registered entities and add them to the cut-out stencil and the glow buffer
	for ( int i=0; i < m_vGlowEnts.Count(); i++ )
	{
		RenderToStencil( i, pRenderContext );
		RenderToGlowTexture( i, pRenderContext );
	}

	// Now we take the built up glow buffer (m_GlowBuff1) and blur it two ways
	// the intermediate buffer (m_GlowBuff2) allows us to do this properly
	pRenderContext->PushRenderTargetAndViewport( m_GlowBuff2 );
		pRenderContext->DrawScreenSpaceQuad( m_BlurX );
	pRenderContext->PopRenderTargetAndViewport();

	pRenderContext->PushRenderTargetAndViewport( m_GlowBuff1 );
		pRenderContext->DrawScreenSpaceQuad( m_BlurY );
	pRenderContext->PopRenderTargetAndViewport();

	// Setup the renderer to only draw where the stencil is not 1
	pRenderContext->SetStencilEnable( true );
	pRenderContext->SetStencilReferenceValue( 0 );
	pRenderContext->SetStencilTestMask( 1 );
	pRenderContext->SetStencilCompareFunction( STENCILCOMPARISONFUNCTION_EQUAL );
	pRenderContext->SetStencilPassOperation( STENCILOPERATION_ZERO );

	// Finally draw our blurred result onto the screen
	pRenderContext->DrawScreenSpaceQuad( m_EffectMaterial );

	pRenderContext->SetStencilEnable( false );
}

void CEntGlowEffect::RenderToStencil( int idx, IMatRenderContext *pRenderContext )
{
	if ( idx < 0 || idx >= m_vGlowEnts.Count() )
		return;

	C_BaseEntity *pEnt = m_vGlowEnts[idx]->m_hEnt.Get();
	if ( !pEnt )
	{
		// We don't exist anymore, remove us!
		delete m_vGlowEnts[idx];
		m_vGlowEnts.Remove(idx);
		return;
	}

	// Don't draw this if we are out of range
	if ( GetGlowAlphaDistMod(idx) == 0 )
		return;

	pRenderContext->SetStencilEnable( true );
	pRenderContext->SetStencilFailOperation( STENCILOPERATION_KEEP );
	pRenderContext->SetStencilZFailOperation( STENCILOPERATION_KEEP );
	pRenderContext->SetStencilPassOperation( STENCILOPERATION_REPLACE );
	pRenderContext->SetStencilCompareFunction( STENCILCOMPARISONFUNCTION_ALWAYS );
	pRenderContext->SetStencilWriteMask( 1 );
	pRenderContext->SetStencilReferenceValue( 1 );

	pRenderContext->DepthRange( 0.0f, 0.01f );
	render->SetBlend( 0 );

	modelrender->ForcedMaterialOverride( m_WhiteMaterial );
		pEnt->DrawModel( STUDIO_RENDER );
	modelrender->ForcedMaterialOverride( NULL );

	render->SetBlend( 1 );
	pRenderContext->DepthRange( 0.0f, 1.0f );

	pRenderContext->SetStencilEnable( false );
}

void CEntGlowEffect::RenderToGlowTexture( int idx, IMatRenderContext *pRenderContext )
{
	if ( idx < 0 || idx >= m_vGlowEnts.Count() )
		return;

	C_BaseEntity *pEnt = m_vGlowEnts[idx]->m_hEnt.Get();
	if ( !pEnt )
	{
		// We don't exist anymore, remove us!
		delete m_vGlowEnts[idx];
		m_vGlowEnts.Remove(idx);
		return;
	}

	float color[4];
	memcpy( &color, &(m_vGlowEnts[idx]->m_fColor), sizeof(color) );

	color[3] *= GetGlowAlphaDistMod( idx );

	// Don't even bother drawing us if we are out of range
	if ( color[3] == 0 )
		return;

	pRenderContext->PushRenderTargetAndViewport( m_GlowBuff1 );

	modelrender->SuppressEngineLighting( true );

	IMaterialVar *var = m_WhiteMaterial->FindVar( "$selfillumtint", NULL, false );
	var->SetVecValue( color, 4 );

	var = m_WhiteMaterial->FindVar( "$alpha", NULL, false );
	var->SetFloatValue( color[3] );

	modelrender->ForcedMaterialOverride( m_WhiteMaterial );
		pEnt->DrawModel( STUDIO_RENDER );
	modelrender->ForcedMaterialOverride( NULL );
	
	modelrender->SuppressEngineLighting( false );

	pRenderContext->PopRenderTargetAndViewport();
}

float CEntGlowEffect::GetGlowAlphaDistMod( int idx )
{
	if ( idx < 0 || idx >= m_vGlowEnts.Count() )
		return 0;

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return 0;

	float startFade = m_vGlowEnts[idx]->m_fMaxDistSq * 0.85f;
	float distTo = pPlayer->GetAbsOrigin().DistToSqr( m_vGlowEnts[idx]->m_hEnt->GetAbsOrigin() );

	if ( distTo > m_vGlowEnts[idx]->m_fMaxDistSq )
		return 0;
	else if ( distTo < startFade )
		return 1.0f;
	else
		return RemapValClamped( distTo, startFade, m_vGlowEnts[idx]->m_fMaxDistSq, 1.0f, 0 ); 
}

void CEntGlowEffect::RegisterEnt( EHANDLE hEnt, Color glowColor /*=Color(255,255,255,64)*/, float fMaxDist /*=250.0f*/ )
{
	// Don't add duplicates
	if ( FindGlowEnt(hEnt) != -1 || !hEnt.Get() )
		return;

	sGlowEnt *newEnt = new sGlowEnt;
	newEnt->m_hEnt = hEnt;
	newEnt->m_fColor[0] = glowColor.r() / 255.0f;
	newEnt->m_fColor[1] = glowColor.g() / 255.0f;
	newEnt->m_fColor[2] = glowColor.b() / 255.0f;
	newEnt->m_fColor[3] = glowColor.a() / 255.0f;
	newEnt->m_fMaxDistSq = fMaxDist * fMaxDist;
	m_vGlowEnts.AddToTail( newEnt );
}

void CEntGlowEffect::DeregisterEnt( EHANDLE hEnt )
{
	int idx = FindGlowEnt(hEnt);
	if ( idx == -1 )
		return;

	delete m_vGlowEnts[idx];
	m_vGlowEnts.Remove( idx );
}

void CEntGlowEffect::SetEntColor( EHANDLE hEnt, Color glowColor )
{
	int idx = FindGlowEnt(hEnt);
	if ( idx == -1 )
		return;

	m_vGlowEnts[idx]->m_fColor[0] = glowColor.r() / 255.0f;
	m_vGlowEnts[idx]->m_fColor[1] = glowColor.g() / 255.0f;
	m_vGlowEnts[idx]->m_fColor[2] = glowColor.b() / 255.0f;
	m_vGlowEnts[idx]->m_fColor[3] = glowColor.a() / 255.0f;
}

void CEntGlowEffect::SetEntMaxGlowDist( EHANDLE hEnt, float fMaxDist )
{
	int idx = FindGlowEnt(hEnt);
	if ( idx == -1 )
		return;

	m_vGlowEnts[idx]->m_fMaxDistSq = fMaxDist * fMaxDist;
}

int CEntGlowEffect::FindGlowEnt( EHANDLE hEnt )
{
	for ( int i=0; i < m_vGlowEnts.Count(); i++ )
	{
		if ( m_vGlowEnts[i]->m_hEnt == hEnt )
			return i;
	}

	return -1;
}
