///////////// Copyright © 2011, Goldeneye: Source. All rights reserved. /////////////
// 
// File: c_gebot_player.cpp
// Description:
//     See header
//
// Created On: 30 Sep 11
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include "c_gebot_player.h"
#include "c_ai_basenpc.h"
#include "ge_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Don't alias here
#if defined( CGEBotPlayer )
#undef CGEBotPlayer	
#endif

IMPLEMENT_CLIENTCLASS_DT(C_GEBotPlayer, DT_GEBot_Player, CGEBotPlayer)
	RecvPropEHandle( RECVINFO(m_pNPC) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_GEBotPlayer )
END_PREDICTION_DATA()

void C_GEBotPlayer::AdjustCollisionBounds( bool inflate )
{
	// We adjust the NPC instead of us
	C_BaseEntity *pNPC = m_pNPC.Get();
	if ( pNPC )
	{
		if ( inflate )
		{
			// Don't worry about ducking for now
			pNPC->SetCollisionBounds( VEC_SHOT_MIN, VEC_SHOT_MAX );
		}
		else
		{
			// Put the collision bounds back
			pNPC->SetCollisionBounds( VEC_HULL_MIN, VEC_HULL_MAX );
		}
	}
}

void C_GEBotPlayer::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );
}

bool C_GEBotPlayer::ShouldDraw()
{
	// We NEVER draw the player proxy
	return false;
}

// -------------------------------------------------------------------------------- //
// NPC Ragdoll entities.
// -------------------------------------------------------------------------------- //

IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_GENPCRagdoll, DT_GENPCRagdoll, CGENPCRagdoll )
	RecvPropVector( RECVINFO(m_vecRagdollOrigin) ),
	RecvPropEHandle( RECVINFO( m_hNPC ) ),
	RecvPropInt( RECVINFO( m_nModelIndex ) ),
	RecvPropInt( RECVINFO(m_nForceBone) ),
	RecvPropVector( RECVINFO(m_vecForce) ),
	RecvPropVector( RECVINFO( m_vecRagdollVelocity ) )
END_RECV_TABLE()

C_GENPCRagdoll::C_GENPCRagdoll()
{

}

C_GENPCRagdoll::~C_GENPCRagdoll()
{
	PhysCleanupFrictionSounds( this );

	if ( m_hNPC )
		m_hNPC->CreateModelInstance();
}

void C_GENPCRagdoll::Interp_Copy( C_BaseAnimatingOverlay *pSourceEntity )
{
	if ( !pSourceEntity )
		return;

	VarMapping_t *pSrc = pSourceEntity->GetVarMapping();
	VarMapping_t *pDest = GetVarMapping();

	// Find all the VarMapEntry_t's that represent the same variable.
	for ( int i = 0; i < pDest->m_Entries.Count(); i++ )
	{
		VarMapEntry_t *pDestEntry = &pDest->m_Entries[i];
		const char *pszName = pDestEntry->watcher->GetDebugName();
		for ( int j=0; j < pSrc->m_Entries.Count(); j++ )
		{
			VarMapEntry_t *pSrcEntry = &pSrc->m_Entries[j];
			if ( !Q_strcmp( pSrcEntry->watcher->GetDebugName(), pszName ) )
			{
				pDestEntry->watcher->Copy( pSrcEntry->watcher );
				break;
			}
		}
	}
}

void C_GENPCRagdoll::ImpactTrace( trace_t *pTrace, int iDamageType, char *pCustomImpactName )
{
	static bool bInTrace = false;
	if ( bInTrace )
		return;

	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

	if( !pPhysicsObject )
		return;

	Vector dir = pTrace->endpos - pTrace->startpos;

	if ( iDamageType == DMG_BLAST )
	{
		dir *= 4000;  // adjust impact strength

		// apply force at object mass center
		pPhysicsObject->ApplyForceCenter( dir );
	}
	else
	{
		Vector hitpos;  

		VectorMA( pTrace->startpos, pTrace->fraction, dir, hitpos );
		VectorNormalize( dir );

		dir *= 4000;  // adjust impact strength

		// apply force where we hit it
		pPhysicsObject->ApplyForceOffset( dir, hitpos );	

		UTIL_BloodDrips( pTrace->endpos, dir, BLOOD_COLOR_RED, 20 );
		bInTrace = true; //<-- Prevent infinite recursion!
		C_BaseEntity::ImpactTrace( pTrace, iDamageType, pCustomImpactName );
		bInTrace = false;
	}

	m_pRagdoll->ResetRagdollSleepAfterTime();
}


void C_GENPCRagdoll::CreateRagdoll( void )
{
	// First, initialize all our data. If we have the player's entity on our client,
	// then we can make ourselves start out exactly where the player is.
	C_AI_BaseNPC *pNPC = dynamic_cast< C_AI_BaseNPC* >( m_hNPC.Get() );

	// Cache away our skin before we start meddling (setting it later down)
	int skin = 0;
	if ( pNPC )
		skin = pNPC->GetSkin();

	if ( pNPC && !pNPC->IsDormant() )
	{
		// move my current model instance to the ragdoll's so decals are preserved.
		pNPC->SnatchModelInstance( this );

		// Copy all the interpolated vars from the player entity.
		// The entity uses the interpolated history to get bone velocity.
		Interp_Copy( pNPC );

		SetAbsAngles( pNPC->GetRenderAngles() );
		GetRotationInterpolator().Reset();

		m_flAnimTime = pNPC->m_flAnimTime;
		SetSequence( pNPC->GetSequence() );
		m_flPlaybackRate = pNPC->GetPlaybackRate();
	}
	else
	{
		// overwrite network origin so later interpolation will
		// use this position
		SetNetworkOrigin( m_vecRagdollOrigin );

		SetAbsOrigin( m_vecRagdollOrigin );
		SetAbsVelocity( m_vecRagdollVelocity );

		Interp_Reset( GetVarMapping() );
	}

	SetModelIndex( m_nModelIndex );

	// Make us a ragdoll..
	m_nRenderFX = kRenderFxRagdoll;

	matrix3x4_t boneDelta0[MAXSTUDIOBONES];
	matrix3x4_t boneDelta1[MAXSTUDIOBONES];
	matrix3x4_t currentBones[MAXSTUDIOBONES];
	const float boneDt = 0.05f;

	if ( pNPC && !pNPC->IsDormant() )
	{
		pNPC->GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
	}
	else
	{
		GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
	}

	// Finally reset our skin
	m_nSkin = skin;

	InitAsClientRagdoll( boneDelta0, boneDelta1, currentBones, boneDt );
}


void C_GENPCRagdoll::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		CreateRagdoll();
	}
}

IRagdoll* C_GENPCRagdoll::GetIRagdoll() const
{
	return m_pRagdoll;
}

void C_GENPCRagdoll::UpdateOnRemove( void )
{
	VPhysicsSetObject( NULL );

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: clear out any face/eye values stored in the material system
//-----------------------------------------------------------------------------
void C_GENPCRagdoll::SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights )
{
	BaseClass::SetupWeights( pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights );

	static float destweight[128];
	static bool bIsInited = false;

	CStudioHdr *hdr = GetModelPtr();
	if ( !hdr )
		return;

	int nFlexDescCount = hdr->numflexdesc();
	if ( nFlexDescCount )
	{
		Assert( !pFlexDelayedWeights );
		memset( pFlexWeights, 0, nFlexWeightCount * sizeof(float) );
	}

	if ( m_iEyeAttachment > 0 )
	{
		matrix3x4_t attToWorld;
		if (GetAttachment( m_iEyeAttachment, attToWorld ))
		{
			Vector local, tmp;
			local.Init( 1000.0f, 0.0f, 0.0f );
			VectorTransform( local, attToWorld, tmp );
			modelrender->SetViewTarget( GetModelPtr(), GetBody(), tmp );
		}
	}
}

#include "c_npc_gebase.h"

C_GEBotPlayer* ToGEBotPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity )
		return NULL;

	if ( pEntity->IsNPC() )
	{
#ifdef _DEBUG
		return dynamic_cast<C_GEBotPlayer*>( ((C_NPC_GEBase*)pEntity)->GetBotPlayer() );
#else
		return static_cast<C_GEBotPlayer*>( ((C_NPC_GEBase*)pEntity)->GetBotPlayer() );
#endif
	}
	else if ( pEntity->IsPlayer() )
	{
#ifdef _DEBUG
		return dynamic_cast<C_GEBotPlayer*>( pEntity );
#else
		return static_cast<C_GEBotPlayer*>( pEntity );
#endif
	}

	return NULL;
}
