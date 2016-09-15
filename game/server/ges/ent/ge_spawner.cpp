//////////  Copyright © 2011, Goldeneye Source. All rights reserved. ///////////
// 
// File: ge_spawner.cpp
// Description: See header.
//
// Created On: 7-19-11
// Created By: KillerMonkey
///////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include "gemp_gamerules.h"
#include "ge_tokenmanager.h"
#include "ge_spawner.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SPAWNER_THINK_INTERVAL		0.5f
#define SPAWNER_MAX_MOVE_DIST		4096.0f	//This value is 64 squared.
#define SPAWNER_MOVE_CHECK_INTERVAL 5.0f

BEGIN_DATADESC( CGESpawner )
	DEFINE_KEYFIELD( m_iSlot, FIELD_INTEGER, "slot" ),
	DEFINE_KEYFIELD( m_bOrigDisableState, FIELD_BOOLEAN, "StartDisabled"),
	DEFINE_KEYFIELD( m_bResetOnNewRound, FIELD_BOOLEAN, "ResetOnNewRound"),
	// Think
	DEFINE_THINKFUNC( Think ),
	// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
	DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),
	DEFINE_INPUTFUNC(FIELD_VOID, "Toggle", InputToggle),

	DEFINE_INPUTFUNC(FIELD_VOID, "EnableResetting", InputEnableResetting),
	DEFINE_INPUTFUNC(FIELD_VOID, "DisableResetting", InputDisableResetting),
	// Outputs
	DEFINE_OUTPUT( m_OnPickedUp, "OnPickedUp" ),
	DEFINE_OUTPUT( m_OnRespawn, "OnRespawn" ),
END_DATADESC();

// Generic spawners, basically used as point entities
LINK_ENTITY_TO_CLASS( ge_tokenspawner, CGESpawner );
LINK_ENTITY_TO_CLASS( ge_tokenspawner_mi6, CGESpawner );
LINK_ENTITY_TO_CLASS( ge_tokenspawner_janus, CGESpawner );
LINK_ENTITY_TO_CLASS( ge_capturearea_spawn, CGESpawner );
LINK_ENTITY_TO_CLASS( ge_capturearea_spawn_mi6, CGESpawner );
LINK_ENTITY_TO_CLASS( ge_capturearea_spawn_janus, CGESpawner );

ConVar ge_debug_itemspawns( "ge_debug_itemspawns", "0", FCVAR_CHEAT | FCVAR_GAMEDLL, "Visualize spawners and show their slot information" );

CGESpawner::CGESpawner( void )
{
	m_iSlot = -1;
	m_bOrigDisableState = false;
	m_bDisabled = false;
	m_bResetOnNewRound = true;
	m_szBaseEntity[0] = m_szOverrideEntity[0] = '\0';
}

void CGESpawner::Spawn( void )
{
	BaseClass::Spawn();

	// Move us up 10 units to curtail things getting stuck
	SetAbsOrigin( GetAbsOrigin() + Vector(0,0,10) );

	m_bDisabled = m_bOrigDisableState;

	SetThink( &CGESpawner::Think );
	SetNextThink( gpGlobals->curtime );
}

void CGESpawner::Init( void )
{
	OnInit();
	m_fNextSpawnTime = gpGlobals->curtime;

	if (m_bResetOnNewRound)
		m_bDisabled = m_bOrigDisableState;
}

void CGESpawner::Think( void )
{
	CBaseEntity *pCurrEnt = m_hCurrentEntity.Get();

	// We have been picked if we hit this block
	if ( (pCurrEnt && pCurrEnt->GetOwnerEntity() != this) ||
		 (!pCurrEnt && m_fNextSpawnTime == 0) )
	{
		OnEntPicked();
		pCurrEnt = NULL;
	}

	int spawnstate = ShouldRespawn();

	// This may call into a higher class if it's overridden
	if ( spawnstate > 1 || (!pCurrEnt && spawnstate) )
		SpawnEnt( spawnstate );


	if ( pCurrEnt && gpGlobals->curtime >= m_fNextMoveCheck )
	{
		if ( GetAbsOrigin().DistToSqr( pCurrEnt->GetAbsOrigin() ) > SPAWNER_MAX_MOVE_DIST )
			pCurrEnt->Teleport( &GetAbsOrigin(), &GetAbsAngles(), &vec3_origin );

		m_fNextMoveCheck = gpGlobals->curtime + SPAWNER_MOVE_CHECK_INTERVAL;
	}

	// Show a debug box if we are asking for it
	if ( ge_debug_itemspawns.GetBool() )
	{
		Color c( 120, 120, 120, 120 );
		if ( IsOverridden() )
		{
			c.SetColor( 200, 200, 200, 200 );
		}
		else
		{
			switch ( GetSlot() )
			{
			case 0: c.SetColor( 96, 0, 120, 120 ); break;
			case 1: c.SetColor( 0, 24, 248, 120 ); break;
			case 2: c.SetColor( 0, 192, 216, 120 ); break;
			case 3: c.SetColor( 0, 180, 0, 120 ); break;
			case 4: c.SetColor( 141, 252, 0, 120 ); break;
			case 5: c.SetColor( 227, 252, 0, 120 ); break;
			case 6: c.SetColor( 248, 143, 0, 120 ); break;
			case 7: c.SetColor( 248, 0, 0, 120 ); break;
			}
		}

		if ( ge_debug_itemspawns.GetInt() < 2 || IsOverridden() )
		{
			Vector mins( -20, -30, 0 );
			Vector maxs( 20, 30, 20 );
			debugoverlay->AddBoxOverlay2( GetAbsOrigin(), mins, maxs, GetAbsAngles(), c, c, SPAWNER_THINK_INTERVAL );

			char tempstr[64];
			int pos = 1;
			EntityText( pos, GetClassname(), SPAWNER_THINK_INTERVAL );

			if ( GetSlot() >= 0 )
			{
				Q_snprintf( tempstr, 64, "Slot: %d", GetSlot() + 1 );
				EntityText( ++pos, tempstr, SPAWNER_THINK_INTERVAL );
			}

			if ( !pCurrEnt && (IsValid() || IsOverridden()) && gpGlobals->curtime < m_fNextSpawnTime )
			{
				Q_snprintf( tempstr, 64, "Spawn in %0.1f secs", m_fNextSpawnTime - gpGlobals->curtime );
				EntityText( ++pos, tempstr, SPAWNER_THINK_INTERVAL );
			}

			if ( IsOverridden() )
			{
				Q_snprintf( tempstr, 64, "OVERIDDEN (%s)", m_szOverrideEntity );
				EntityText( ++pos, tempstr, SPAWNER_THINK_INTERVAL );
			}
		}
	}

	SetNextThink( gpGlobals->curtime + SPAWNER_THINK_INTERVAL );
}

int CGESpawner::ShouldRespawn( void )
{
	// Always deny if we are disabled
	if ( m_bDisabled )
		return 0;

	// Overrides defer to the token manager
	if ( IsOverridden() && GEMPRules()->GetTokenManager()->ShouldSpawnToken(m_szOverrideEntity, this) )
		return 2;
	else if ( IsValid() && !m_hCurrentEntity.Get() && gpGlobals->curtime >= m_fNextSpawnTime )
	{
		// If we are not overridden, don't spawn a registered token type
		if ( !GEMPRules()->GetTokenManager()->IsValidToken( m_szBaseEntity ) )
			return 1;
	}

	return 0;
}

float CGESpawner::GetRespawnInterval( void )
{
	// Some static interval
	// TODO: Should this be context sensitive?
	return 10.0f;
}

void CGESpawner::SetOverride( const char *szClassname, float secToSpawn /*=0*/ )
{
	if ( !szClassname )
	{
		m_szOverrideEntity[0] = '\0';
		RemoveEnt();
	}
	else if ( CanCreateEntityClass( szClassname ) )
	{
		Q_strncpy( m_szOverrideEntity, szClassname, MAX_ENTITY_NAME );
		// Remove any currently held entity and override
		RemoveEnt();
		// Notify of override
		OnEntOverridden( m_szOverrideEntity );
		// Set our next respawn time
		m_fNextSpawnTime = gpGlobals->curtime + secToSpawn;
	}
	else
	{
		DevWarning( "[GESpawner] Invalid entity type passed as override: %s\n", szClassname );
	}
}

void CGESpawner::SetBaseEnt( const char *szClassname )
{
	if ( !szClassname )
	{
		m_szBaseEntity[0] = '\0';
		RemoveEnt();
	}
	else if ( CanCreateEntityClass( szClassname ) )
	{
		Q_strncpy( m_szBaseEntity, szClassname, MAX_ENTITY_NAME );
		// Remove any currently held entity
		RemoveEnt();
		// Set our next respawn time
		m_fNextSpawnTime = gpGlobals->curtime; // + GetRespawnDelay()??
	}
	else
	{
		DevWarning( "[GESpawner] Invalid entity type passed as base ent: %s\n", szClassname );
	}
}

void CGESpawner::SpawnEnt( int spawnstate )
{
	// Just in case
	RemoveEnt();
	m_OnRespawn.FireOutput(this, this);

	if ( spawnstate == 2 )
	{
		if (!IsOverridden())
			return;

		if ( CanCreateEntityClass( m_szOverrideEntity ) )
		{
			m_hCurrentEntity = CBaseEntity::Create( m_szOverrideEntity, GetAbsOrigin(), GetAbsAngles(), this );
			OnEntSpawned( m_szOverrideEntity );
			EmitSound("Item.Materialize");
		}
		// Don't try again even if we fail
		m_fNextSpawnTime = 0;
	}
	else if ( spawnstate == 1 )
	{
		if ( !IsValid() )
			return;

		if ( CanCreateEntityClass( m_szBaseEntity ) )
		{
			m_hCurrentEntity = CBaseEntity::Create( m_szBaseEntity, GetAbsOrigin(), GetAbsAngles(), this );	
			OnEntSpawned( m_szBaseEntity );
			EmitSound("Item.Materialize");
		}
		// Don't try again even if we fail
		m_fNextSpawnTime = 0;
	}
	else
	{
		DevWarning( "[GESpawner] Spawn attempted at invalid spawner %s, slot %i\n", GetClassname(), GetSlot() );
	}
}

void CGESpawner::RemoveEnt( void )
{
	if ( m_hCurrentEntity.Get() )
	{
		m_hCurrentEntity->Remove();
	}
	m_hCurrentEntity = NULL;
}

void CGESpawner::OnEntPicked( void )
{
	m_hCurrentEntity = NULL;
	m_fNextSpawnTime = gpGlobals->curtime + GetRespawnInterval();

	m_OnPickedUp.FireOutput(this, this);
}

void CGESpawner::OnEnabled( void )
{
	SetThink( &CGESpawner::Think );
	SetNextThink( gpGlobals->curtime );
	m_fNextSpawnTime = gpGlobals->curtime;
}

void CGESpawner::OnDisabled( void )
{
	SetThink( NULL );
	SetNextThink( TICK_NEVER_THINK );
	RemoveEnt();
}

void CGESpawner::SetEnabled( bool state )
{
	inputdata_t nullData;
	if ( state )
		InputEnable( nullData );
	else
		InputDisable( nullData );
}

void CGESpawner::InputEnable( inputdata_t &inputdata )
{
	m_bDisabled = false;
	OnEnabled();
}

void CGESpawner::InputDisable( inputdata_t &inputdata )
{
	m_bDisabled = true;
	OnDisabled();
}

void CGESpawner::InputEnableResetting(inputdata_t &inputdata)
{
	m_bResetOnNewRound = true;
}

void CGESpawner::InputDisableResetting(inputdata_t &inputdata)
{
	m_bResetOnNewRound = false;
}

void CGESpawner::InputToggle( inputdata_t &inputdata )
{
	m_bDisabled = !m_bDisabled;
	m_bDisabled ? OnDisabled() : OnEnabled();
}
