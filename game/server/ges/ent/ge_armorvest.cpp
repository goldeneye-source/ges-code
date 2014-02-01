//////////  Copyright © 2008, Goldeneye Source. All rights reserved. ///////////
// 
// ge_ammocrate.cpp
//
// Description:
//      An ammo crate that gives a certain amount of ammo of different types
//
// Created On: 3/22/2008 1200
// Created By: KillerMonkey
////////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ge_armorvest.h"
#include "ge_shareddefs.h"
#include "gemp_gamerules.h"
#include "ge_player.h"
#include "gebot_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( item_armorvest, CGEArmorVest );
LINK_ENTITY_TO_CLASS( item_armorvest_half, CGEArmorVest );
PRECACHE_REGISTER( item_armorvest );
PRECACHE_REGISTER( item_armorvest_half );

BEGIN_DATADESC( CGEArmorVest )
	// Function Pointers
	DEFINE_ENTITYFUNC( ItemTouch ),
	// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
	DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),
	DEFINE_INPUTFUNC(FIELD_VOID, "Toggle", InputToggle),
END_DATADESC();

CGEArmorVest::CGEArmorVest( void )
{
	m_bEnabled = true;
	m_iAmount = MAX_ARMOR;
}

void CGEArmorVest::Spawn( void )
{
	Precache();

	if ( !Q_strcmp(GetClassname(), "item_armorvest_half") )
		m_iAmount = MAX_ARMOR / 2.0f;

	SetModel( "models/weapons/armor/armor.mdl" );

	SetOriginalSpawnOrigin( GetAbsOrigin() );
	SetOriginalSpawnAngles( GetAbsAngles() );
	
	BaseClass::Spawn();

	// So NPC's can "see" us
	AddFlag( FL_OBJECT );

	// Override base's ItemTouch for NPC's
	SetTouch( &CGEArmorVest::ItemTouch );
}

void CGEArmorVest::Precache( void )
{
	PrecacheModel( "models/weapons/armor/armor.mdl" );
	PrecacheScriptSound( "ArmorVest.Pickup" );

	BaseClass::Precache();
}

CBaseEntity *CGEArmorVest::Respawn( void )
{
	 BaseClass::Respawn();
	 SetNextThink( gpGlobals->curtime + GEMPRules()->FlArmorRespawnTime(this) );
	 return this;
}

void CGEArmorVest::Materialize( void )
{
	// Only materialize if we are enabled and allowed
	if ( GEMPRules()->ArmorShouldRespawn() && m_bEnabled )
	{
		BaseClass::Materialize();
		// Override base's ItemTouch for NPC's
		SetTouch( &CGEArmorVest::ItemTouch );
	}
}

void CGEArmorVest::ItemTouch( CBaseEntity *pOther )
{
	if ( pOther->IsNPC() )
	{
		// If the NPC is a Bot hand it off to it's proxy, normal NPC's don't use armor
		CBaseCombatCharacter *pNPC = pOther->MyCombatCharacterPointer();
		CGEBotPlayer *pBot = ((CNPC_GEBase*)pOther)->GetBotPlayer();

		if ( pBot && pNPC )
		{
			// ok, a player is touching this item, but can he have it?
			if ( !g_pGameRules->CanHaveItem( pBot, this ) )
				return;

			if ( MyTouch( pBot ) )
			{
				SetTouch( NULL );
				SetThink( NULL );

				// player grabbed the item. 
				g_pGameRules->PlayerGotItem( pBot, this );
				Respawn();
			}
		}
	}
	else
	{
		BaseClass::ItemTouch( pOther );
	}
}

bool CGEArmorVest::MyTouch( CBasePlayer *pPlayer )
{
	CGEPlayer *pGEPlayer = dynamic_cast<CGEPlayer*>( pPlayer );
	if ( !pGEPlayer )
		return false;

	return pGEPlayer->AddArmor(m_iAmount) || GERules()->ShouldForcePickup(pPlayer, this);
}

void CGEArmorVest::OnEnabled( void )
{
	// Respawn and wait for materialize
	Respawn();
}

void CGEArmorVest::OnDisabled( void )
{
	// "Respawn", but we won't materialize
	Respawn();
}

void CGEArmorVest::SetEnabled( bool state )
{
	inputdata_t nullData;
	if ( state )
		InputEnable( nullData );
	else
		InputDisable( nullData );
}

void CGEArmorVest::InputEnable( inputdata_t &inputdata )
{
	m_bEnabled = true;
	OnEnabled();
}

void CGEArmorVest::InputDisable( inputdata_t &inputdata )
{
	m_bEnabled = false;
	OnDisabled();
}

void CGEArmorVest::InputToggle( inputdata_t &inputdata )
{
	m_bEnabled = !m_bEnabled;
	m_bEnabled ? OnEnabled() : OnDisabled();
}

