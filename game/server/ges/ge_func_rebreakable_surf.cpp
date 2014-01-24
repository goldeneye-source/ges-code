///////////// Copyright © 2006, Scott Loyd. All rights reserved. /////////////
// 
// File: func_rebreakable_surf.cpp
// Description:
//      A breakable surface, that rebuilds itself!
//
// Created On: 3/9/2006 6:02:46 AM
// Created By: Scott Loyd <mailto:scottloyd@gmail.com> 
/////////////////////////////////////////////////////////////////////////////


#include "cbase.h"
#include "ndebugoverlay.h"
#include "filters.h"
#include "player.h"
#include "func_breakablesurf.h"
#include "shattersurfacetypes.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "globals.h"
#include "physics_impact_damage.h"
#include "te_effect_dispatch.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


class CREBreakableSurface : public CBreakableSurface
{
public:
	DECLARE_CLASS( CREBreakableSurface , CBreakableSurface );
	DECLARE_DATADESC();

	void Spawn(void);
	void RespawnThink(void);

	void CorrectPosition(void);
	void FixAllPanes(void);

	float m_flRespawnTimer;
private:
	enum {
		RESPAWN_NOT_SET = 0,
		RESPAWNING
	};
	int m_iFlags;
	int m_iOrigHealth;

	//Position Correcting
	Vector		m_vOrigNormal;
	Vector		m_vOrigLLVertex;
	Vector		m_vOrigULVertex;
	Vector		m_vOrigLRVertex;
	Vector		m_vOrigURVertex;
};

LINK_ENTITY_TO_CLASS( func_rebreakable_surf, CREBreakableSurface  );


BEGIN_DATADESC( CREBreakableSurface )
	DEFINE_KEYFIELD( m_flRespawnTimer,	FIELD_FLOAT, "RespawnTime" ),

	DEFINE_FIELD( m_iFlags,	FIELD_INTEGER),	

	// Function Pointers
	DEFINE_THINKFUNC( RespawnThink ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: Called on spawn
//-----------------------------------------------------------------------------
void CREBreakableSurface::Spawn(void)
{
	BaseClass::Spawn();
	m_iFlags = RESPAWN_NOT_SET;
	m_iOrigHealth = m_iHealth;

	//Position Correcting
	m_vOrigNormal	= m_vNormal;
	m_vOrigLLVertex = m_vLLVertex;
	m_vOrigULVertex = m_vULVertex;
	m_vOrigLRVertex = m_vLRVertex;
	m_vOrigURVertex = m_vURVertex;

	SetContextThink( &CREBreakableSurface::RespawnThink, gpGlobals->curtime, "RespawnThink" );
}

//------------------------------------------------------------------------------
// Purpose : should think one time every second, handle respawning.
//------------------------------------------------------------------------------
void CREBreakableSurface::RespawnThink(void)
{
	//If it has been shot, shattered, whatever this will be true.
	if(BaseClass::m_bIsBroken == true)
	{
		if(m_iFlags == RESPAWN_NOT_SET)
		{
			m_iFlags = RESPAWNING;
			SetContextThink( &CREBreakableSurface::RespawnThink, gpGlobals->curtime + m_flRespawnTimer, "RespawnThink" );
			return;
		}
		else
		{
			CorrectPosition();
			FixAllPanes();
			m_iFlags = RESPAWN_NOT_SET;
		}
	}
	SetContextThink( &CREBreakableSurface::RespawnThink, gpGlobals->curtime +1.0f, "RespawnThink" );
}

//Purpose:  Fixes bug #143, 202, where the glass was moving off of the door.
//				I believe the glass moves off the door because it causes the panes to break
//				and not but out as a crappy looking texture that you can't break.
void CREBreakableSurface::CorrectPosition(void)
{
	m_vLLVertex = m_vOrigLLVertex;
	m_vLRVertex = m_vOrigLRVertex;
	m_vULVertex = m_vOrigULVertex;
	m_vURVertex = m_vOrigURVertex;
	m_vNormal	= m_vOrigNormal;
}

#define WINDOW_PANE_HEALTHY			1

void CREBreakableSurface::FixAllPanes(void)
{
	m_iHealth = m_iOrigHealth;
	if ( !m_iHealth || FBitSet( m_spawnflags, SF_BREAK_TRIGGER_ONLY ) )
	{
		// This allows people to shoot at the glass (since it's penetrable)
		if ( m_Material == matGlass )
		{
			m_iHealth = 1;
		}

		m_takedamage = DAMAGE_NO;
	}
	else
	{
		m_takedamage = DAMAGE_YES;
	}

	m_iMaxHealth = ( m_iHealth > 0 ) ? m_iHealth : 1;
  
	SetSolid( SOLID_BSP );
    SetMoveType( MOVETYPE_PUSH );

	SetTouch( &CBreakable::BreakTouch );
	if ( FBitSet( m_spawnflags, SF_BREAK_TRIGGER_ONLY ) )		// Only break on trigger
	{
		SetTouch( NULL );
	}

	// Flag unbreakable glass as "worldbrush" so it will block ALL tracelines
	if ( !IsBreakable() && m_nRenderMode != kRenderNormal )
		AddFlag( FL_WORLDBRUSH );

	if ( m_impactEnergyScale == 0 )
	{
		m_impactEnergyScale = 1.0;
	}

	//CBreakAbleSurface
	RemoveSolidFlags( FSOLID_NOT_SOLID );
	SetCollisionGroup( COLLISION_GROUP_BREAKABLE_GLASS ); 
	m_bIsBroken = false;

	for (int width=0;width<m_nNumWide;width++)
	{
		for (int height=0;height<m_nNumHigh;height++)
		{
			SetSupport( width, height, WINDOW_PANE_HEALTHY );
		}
	}
	m_nNumBrokenPanes = 0;
}