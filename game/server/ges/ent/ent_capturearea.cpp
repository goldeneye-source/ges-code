///////////// Copyright © 2011, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ent_capturearea.cpp
// Description:
//      See header
//
// Created On: 01 Aug 11
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include "gemp_gamerules.h"
#include "ge_tokenmanager.h"
#include "ge_gameplay.h"
#include "npc_gebase.h"
#include "ent_capturearea.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define CAPTURE_MODEL "models/gameplay/capturepoint.mdl"

LINK_ENTITY_TO_CLASS( ge_capturearea, CGECaptureArea );

PRECACHE_REGISTER( ge_capturearea );

IMPLEMENT_SERVERCLASS_ST( CGECaptureArea, DT_GECaptureArea )
	SendPropBool( SENDINFO(m_bEnableGlow) ),
	SendPropInt( SENDINFO(m_GlowColor), 32, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO(m_GlowDist) ),
END_SEND_TABLE()

CGECaptureArea::CGECaptureArea()
{
	m_szGroupName[0] = '\0';
	m_fRadius = 32.0f;

	AddEFlags( EFL_USE_PARTITION_WHEN_NOT_SOLID );
}

void CGECaptureArea::Spawn( void )
{
	Precache();
	BaseClass::Spawn();

	// Do a trace from the origin of the object straight down to find the floor
	trace_t tr;
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + Vector(0,0,-1024), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
	if ( !tr.startsolid && tr.fraction < 1.0 )
		SetAbsOrigin( tr.endpos );
	
	AddFlag( FL_OBJECT );

	VPhysicsInitNormal( SOLID_BBOX, GetSolidFlags() | FSOLID_TRIGGER, false );
	SetMoveType( MOVETYPE_NONE );
	SetBlocksLOS( false );
	SetCollisionGroup( COLLISION_GROUP_CAPAREA );
	
	// See if we are touching anything when we spawn
	ClearTouchingEntities();
	PhysicsTouchTriggers();

	GEMPRules()->GetTokenManager()->OnCaptureAreaSpawned( this );
}

void CGECaptureArea::Precache( void )
{
	PrecacheModel( CAPTURE_MODEL );
	BaseClass::Precache();
}

void CGECaptureArea::UpdateOnRemove( void )
{
	BaseClass::UpdateOnRemove();
	if ( GEMPRules() )
		GEMPRules()->GetTokenManager()->OnCaptureAreaRemoved( this );
}

void CGECaptureArea::SetGroupName( const char* name )
{
	Q_strncpy( m_szGroupName, name, 32 );
}

void CGECaptureArea::SetRadius( float radius )
{
	// Warn if we exceed our threshold radius
	if ( radius > 255.0f )
	{
		Warning( "[CAP AREA] The radius of a capture area cannot exceed 255 units, clipping!\n" );
		radius = 255.0f;
	}

	// Set the collision bloat, this makes sure we don't affect tracing!
	if ( radius > 0 )
	{
		CollisionProp()->UseTriggerBounds( true, radius );
		m_fRadius = radius;
	}
	else
	{
		CollisionProp()->UseTriggerBounds( false );
		m_fRadius = CollisionProp()->BoundingRadius2D();
	}

	// Recalculate our bounds
	CollisionProp()->MarkSurroundingBoundsDirty();

	// See if our new bounds encapsulates anyone nearby
	PhysicsTouchTriggers();
}

void CGECaptureArea::SetupGlow( bool state, Color glowColor /*=Color(255,255,255)*/, float glowDist /*=250.0f*/ )
{
	m_GlowColor.Set( glowColor.GetRawColor() );
	m_GlowDist.Set( glowDist );
	m_bEnableGlow = state;
}

void CGECaptureArea::ClearTouchingEntities( void )
{
	m_hTouchingEntities.RemoveAll();
}

void CGECaptureArea::Touch( CBaseEntity *pOther )
{
	if ( !pOther->IsPlayer() && !pOther->IsNPC() )
		return;

	if ( !GEMPRules()->GetTokenManager()->CanTouchCaptureArea( m_szGroupName, pOther ) )
		return;

	// Check if we are touching us already
	EHANDLE hPlayer = pOther->GetRefEHandle();
	if ( m_hTouchingEntities.Find( hPlayer ) != m_hTouchingEntities.InvalidIndex() )
		return;

	// We have passed all filters and this is a fresh touch, now enforce the radius
	if ( pOther->GetAbsOrigin().DistTo( GetAbsOrigin() ) <= m_fRadius )
	{
		// Put us on the touching list
		m_hTouchingEntities.AddToTail( hPlayer );

		// Resolve our held token (note we must use the pOther)
		const char *szToken = GEMPRules()->GetTokenManager()->GetCaptureAreaToken(m_szGroupName);
		CGEWeapon *pToken = (CGEWeapon*) pOther->MyCombatCharacterPointer()->Weapon_OwnsThisType( szToken );

		// Resolve the player (or bot proxy)
		CGEPlayer *pPlayer = ToGEMPPlayer( pOther );
		if ( !pPlayer )
			pPlayer = ToGEBotPlayer( pOther );

		// Notify the Gameplay
		GEGameplay()->GetScenario()->OnCaptureAreaEntered( this, pPlayer, pToken );
	}
}

void CGECaptureArea::EndTouch( CBaseEntity *pOther )
{
	EHANDLE hPlayer = pOther->GetRefEHandle();
	if ( m_hTouchingEntities.FindAndRemove( hPlayer ) )
	{
		// Resolve the player (or bot proxy)
		CGEPlayer *pPlayer = ToGEMPPlayer( pOther );
		if ( !pPlayer )
			pPlayer = ToGEBotPlayer( pOther );

		if ( GEGameplay() )
			GEGameplay()->GetScenario()->OnCaptureAreaExited( this, ToGEMPPlayer(pOther) );
	}
}
