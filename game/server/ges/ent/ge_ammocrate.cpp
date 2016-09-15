//////////  Copyright © 2011, Goldeneye Source. All rights reserved. ///////////
// 
// ge_ammocrate.cpp
//
// Description:
//      An ammo crate that gives a certain amount of ammo of different types
//
// Created On: 6/20/2011
// Created By: Killer Monkey
////////////////////////////////////////////////////////////////////////////////
#include "cbase.h"

#include "ammodef.h"

#include "ge_shareddefs.h"
#include "ge_tokenmanager.h"
#include "gebot_player.h"
#include "gemp_gamerules.h"

#include "ge_ammocrate.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( ge_ammocrate, CGEAmmoCrate );
PRECACHE_REGISTER( ge_ammocrate );

BEGIN_DATADESC( CGEAmmoCrate )
	// Function Pointers
	DEFINE_ENTITYFUNC( ItemTouch ),
	DEFINE_THINKFUNC( RefillThink ),
END_DATADESC();

ConVar ge_partialammopickups("ge_partialammopickups", "0", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Allow players to pick up ammo from a crate without picking up all of it.");

CGEAmmoCrate::CGEAmmoCrate( void )
{
	m_iAmmoID = -1;
	m_iAmmoAmount = 0;
	m_bGaveGlobalAmmo = false;
	m_iWeaponID = 0;
	m_iWeight = 0;
}

void CGEAmmoCrate::Spawn( void )
{
	Precache();

	SetModel( AMMOCRATE_MODEL );
	BaseClass::Spawn();
	
	// Override base's ItemTouch for NPC's
	SetTouch( &CGEAmmoCrate::ItemTouch );

	// So NPC's can "see" us
	AddFlag( FL_OBJECT );

	Respawn();
}

void CGEAmmoCrate::Precache( void )
{
	PrecacheModel( AMMOCRATE_MODEL );
	PrecacheModel( BABYCRATE_MODEL );
	BaseClass::Precache();
}

void CGEAmmoCrate::Materialize( void )
{
	BaseClass::Materialize();

	// Notify Python about the ammobox
	// Item tracker does not do well with ammocrates that have no assigned ammo type.
	if ( GetScenario() && m_iAmmoID != -1 ) // CHECKING AMMOID IS A CRAPPY HOTFIX FOR NOW, FIX ITEM TRACKER.
	{
		GetScenario()->OnAmmoSpawned( this );
	}

	// Override base's ItemTouch for NPC's
	SetTouch( &CGEAmmoCrate::ItemTouch );
}

CBaseEntity *CGEAmmoCrate::Respawn( void )
{
	// Reset our global ammo flag
	if ( GetScenario() && m_iAmmoID != -1 ) // CHECKING AMMOID IS A CRAPPY HOTFIX FOR NOW, FIX ITEM TRACKER.
	{
		GetScenario()->OnAmmoRemoved( this );
	}

	// Reset our global ammo flag
	m_bGaveGlobalAmmo = false;

	// Add our base ammo
	if ( m_iAmmoID != -1 )
	{
		Ammo_t *pAmmo = GetAmmoDef()->GetAmmoOfIndex( m_iAmmoID );

		if (pAmmo)
		{
			// Set the crate amount
			m_iAmmoAmount = pAmmo->nCrateAmt;

			// Set our skin as appropriate
			if (Q_stristr(pAmmo->pName, "mine"))
				m_nSkin = AMMOCRATE_SKIN_MINES;
			else if (!Q_stricmp(pAmmo->pName, AMMO_GRENADE))
				m_nSkin = AMMOCRATE_SKIN_GRENADES;
			else
				m_nSkin = AMMOCRATE_SKIN_DEFAULT;
		}
	}

	// Don't show us if we won't give out any ammo
	if ( !HasAmmo() )
	{
		BaseClass::Respawn();
		SetThink( NULL );
		return this;
	}

	return BaseClass::Respawn();
}

void CGEAmmoCrate::RefillThink( void )
{
	m_bGaveGlobalAmmo = false;

	if ( m_iAmmoID != -1 )
	{
		Ammo_t *pAmmo = GetAmmoDef()->GetAmmoOfIndex( m_iAmmoID );
		Assert( pAmmo );
		// Set the crate amount
		m_iAmmoAmount = pAmmo->nCrateAmt;
	}
}

void CGEAmmoCrate::SetWeaponID( int id )
{
	m_iWeaponID = id;

	// Get our info
	const char *name = WeaponIDToAlias( id );
	if ( name && name[0] != '\0' )
	{
		int h = LookupWeaponInfoSlot( name );
		if ( h == GetInvalidWeaponInfoHandle() )
			return;

		CGEWeaponInfo *weap = dynamic_cast<CGEWeaponInfo*>( GetFileWeaponInfoFromHandle( h ) );
		if ( weap )
		{
			m_iWeight = weap->iWeight;
		}
	}
}

void CGEAmmoCrate::SetAmmoType( int id )
{ 
	if ( GetAmmoDef()->GetAmmoOfIndex( id ) )
	{
		m_iAmmoID = id;
		Respawn();
	}
	else
	{
		DevWarning( "[AmmoCrate] Invalid ammo type passed, ignored\n" );
	}
}

int CGEAmmoCrate::GetAmmoType( void )
{
	return m_iAmmoID;
}

bool CGEAmmoCrate::HasAmmo( void )
{
	// We have ammo if we have a base ammount or global ammo
	return m_iAmmoAmount > 0 || (!m_bGaveGlobalAmmo && GEMPRules()->GetTokenManager()->HasGlobalAmmo());
}

void CGEAmmoCrate::ItemTouch( CBaseEntity *pOther )
{
	if ( pOther->IsNPC() )
	{
		// If the NPC is a Bot hand it off to it's proxy, normal NPC's don't use ammo crates
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

bool CGEAmmoCrate::MyTouch( CBasePlayer *pPlayer )
{
	// All checks have already been done (called by ItemTouch)
	// so just give the ammo to the player and return true!
	if ( !HasAmmo() )
	{
		DevWarning( "[GEAmmoCrate] Crate touched with no ammo in it\n" );
		return true;
	}

	int ammotaken = pPlayer->GiveAmmo(m_iAmmoAmount, m_iAmmoID);

	m_iAmmoAmount -= ammotaken; // Subtract the amount that was actually given to the player from our carried ammo.

	// Subtract the amount that was actually given to the player
	if (!m_bGaveGlobalAmmo && GEMPRules()->GetTokenManager()->GiveGlobalAmmo(pPlayer))
	{
		if (!ammotaken) //This way we get the sound even if we only pick up special ammo.
			EmitSound("BaseCombatCharacter.AmmoPickup"); // We can't just surpress the sound normally since the surpress sound flag on the giveammo function also tells the game not to give you ammo based weapons.

		m_bGaveGlobalAmmo = true;
		ammotaken += 1; // Tells us later that we took something
	}

	if ( ammotaken )
	{
		if (!ge_partialammopickups.GetBool()) // If we picked up anything, WE PICKED UP IT ALL!...but only if we want to.
		{
			m_iAmmoAmount = 0;
			m_bGaveGlobalAmmo = true;
		}
	}

	if ( !HasAmmo() || GERules()->ShouldForcePickup( pPlayer, this ) )
		return true;
	
	// We still have ammo left in the crate, schedule a refill
	SetThink( &CGEAmmoCrate::RefillThink );
	SetNextThink( gpGlobals->curtime + g_pGameRules->FlItemRespawnTime( this ) );

	return false;
}

void CGEAmmoCrate::UpdateOnRemove(void)
{
	BaseClass::UpdateOnRemove();

	// Notify Python about the ammobox
	if ( GetScenario() && m_iAmmoID != -1 ) // CHECKING AMMOID IS A CRAPPY HOTFIX FOR NOW, FIX ITEM TRACKER.
	{
		GetScenario()->OnAmmoRemoved( this );
	}
}
