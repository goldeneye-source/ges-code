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
	DEFINE_KEYFIELD( m_iSpawnCheckRadius, FIELD_INTEGER, "CheckRadius"),
	DEFINE_KEYFIELD( m_bEnabled, FIELD_BOOLEAN, "StartDisabled"),
	// Function Pointers
	DEFINE_ENTITYFUNC( ItemTouch ),
	DEFINE_THINKFUNC( AliveThink ),
	DEFINE_THINKFUNC( RespawnThink ),
	// Inputs
	DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
	DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),
	DEFINE_INPUTFUNC(FIELD_VOID, "Toggle", InputToggle),
END_DATADESC();

#define AliveThinkInterval		1.0f

ConVar ge_armorrespawntime("ge_armorrespawntime", "14", FCVAR_REPLICATED | FCVAR_NOTIFY, "Minimum time in seconds before armor respawns.");
ConVar ge_armorrespawn_pc_scale("ge_armorrespawn_pc_scale", "15.0", FCVAR_REPLICATED, "Multiplier applied to playercount. ge_armorrespawntime * 10 - ge_armorrespawn_pc_scale * (playercount - 1)^ge_armorrespawn_pc_pow is the full equation.");
ConVar ge_armorrespawn_pc_pow("ge_armorrespawn_pc_pow", "0.5", FCVAR_REPLICATED, "Power applied to playercount. ge_armorrespawntime * 10 - ge_armorrespawn_pc_scale * (playercount - 1)^ge_armorrespawn_pc_pow is the full equation.");

ConVar ge_debug_armorspawns("ge_debug_armorspawns", "0", FCVAR_CHEAT | FCVAR_GAMEDLL, "Debug armor respawn progress.");


CGEArmorVest::CGEArmorVest( void )
{
	m_bEnabled = false;
	m_iAmount = MAX_ARMOR;
	m_iSpawnCheckRadius = 512;
}

void CGEArmorVest::Spawn( void )
{
	m_bEnabled = !m_bEnabled; // Flip this due to our shoddy "StartDisabled" hammer fix.
	Precache();

	if ( !Q_strcmp(GetClassname(), "item_armorvest_half") )
	{
		SetModel("models/weapons/halfarmor/halfarmor.mdl");
		m_iAmount = MAX_ARMOR / 2.0f;
	}
	else
		SetModel("models/weapons/armor/armor.mdl");

	SetOriginalSpawnOrigin( GetAbsOrigin() );
	SetOriginalSpawnAngles( GetAbsAngles() );

	SetCollisionGroup( COLLISION_GROUP_WEAPON ); // Might as well treat this the same way we do weapons.
	
	BaseClass::Spawn();

	// So NPC's can "see" us
	AddFlag( FL_OBJECT );

	m_iSpawnCheckRadiusSqr = m_iSpawnCheckRadius * m_iSpawnCheckRadius;
	m_iSpawnCheckHalfRadiusSqr = m_iSpawnCheckRadiusSqr * 0.25;

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		m_iPlayerPointContribution[i] = 0;
	}

	// Notify Python about the armor
	if ( GetScenario() )
	{
		GetScenario()->OnArmorSpawned( this );
	}

	// Override base's ItemTouch for NPC's
	SetTouch( &CGEArmorVest::ItemTouch );

	// Start thinking like a healthy, alive armorvest.
	SetThink(&CGEArmorVest::AliveThink);
	SetNextThink(gpGlobals->curtime + AliveThinkInterval);

	// Except we might be delusional and really a DEAD, UNHEALTHY ARMORVEST.
	if (!m_bEnabled)
		OnDisabled();
}

void CGEArmorVest::Precache( void )
{
	PrecacheModel( "models/weapons/armor/armor.mdl" );
	PrecacheModel( "models/weapons/halfarmor/halfarmor.mdl" );
	PrecacheScriptSound( "ArmorVest.Pickup" );

	BaseClass::Precache();
}

CBaseEntity *CGEArmorVest::Respawn(void)
{
	BaseClass::Respawn();

	if ( GetScenario() )
	{
		// Notify Python about the armor
		GetScenario()->OnArmorRemoved( this );
	}

	SetSolid( SOLID_NONE );
	RemoveSolidFlags( FSOLID_TRIGGER );

	m_iSpawnpointsgoal = (int)(ge_armorrespawntime.GetInt() * 10 - ge_armorrespawn_pc_scale.GetFloat() * pow(MAX((float)GEMPRules()->GetNumAlivePlayers() - 1, 0), ge_armorrespawn_pc_pow.GetFloat()));

	// Let interested developers know our new goal.

	Vector curPos = GetAbsOrigin();
	DevMsg("Spawnpoints goal for armor at location %f, %f, %f set to %d\n", curPos.x, curPos.y, curPos.z, m_iSpawnpointsgoal);

	ClearAllSpawnProgress();

	SetThink(&CGEArmorVest::RespawnThink);
	SetNextThink(gpGlobals->curtime + 1);
	return this;
}

void CGEArmorVest::RespawnThink(void)
{
	int newpoints;
	
	newpoints = CalcSpawnProgress();
	m_iSpawnpoints += newpoints;

	if (ge_debug_armorspawns.GetBool())
		DEBUG_ShowProgress(1, newpoints);

	if (m_iSpawnpointsgoal <= m_iSpawnpoints)
		Materialize();
	else
		SetNextThink(gpGlobals->curtime + 1);
}

void CGEArmorVest::AliveThink(void)
{
	float distfromspawnersqr = (GetOriginalSpawnOrigin() - GetAbsOrigin()).LengthSqr();

	// Only materialize if we are enabled and allowed
	if (distfromspawnersqr > 65536)
	{
		// Destroy and remake the vphysics object to prevent oddities.
		VPhysicsDestroyObject();

		UTIL_SetOrigin( this, GetOriginalSpawnOrigin() );
		SetAbsAngles( GetOriginalSpawnAngles() );

		CreateItemVPhysicsObject();
	}

	if (ge_debug_armorspawns.GetBool())
		DEBUG_ShowProgress(1, 10);

	SetNextThink(gpGlobals->curtime + AliveThinkInterval);
}

void CGEArmorVest::Materialize(void)
{
	// Only materialize if we are enabled and allowed
	if (GEMPRules()->ArmorShouldRespawn() && m_bEnabled)
	{
		BaseClass::Materialize();

		// Override base's ItemTouch for NPC's
		if ( GetScenario() )
		{
			GetScenario()->OnArmorSpawned( this );
		}

		// Override base's ItemTouch for NPC's
		SetTouch(&CGEArmorVest::ItemTouch);

		SetThink(&CGEArmorVest::AliveThink);
		SetNextThink(gpGlobals->curtime + AliveThinkInterval);
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


// Finds two different values and uses them to compute a point total
// The first is a point value based on how many players are in range and how close they are.
// The second is the distance between the vest and the closest player.

// The closer the closest player is, the slower the armor will respawn.
// However, the weighted average of all the other nearby players can serve to counteract this.
// This means that heavily contested armor spawns still appear fairly frequently.
// A player within 64 units will completely freeze the timer, however.

int CGEArmorVest::CalcSpawnProgress()
{
	// On teamplay just ignore this fancy stuff, people clump together much more and it's not really as big of a deal.
	if ( GEMPRules()->IsTeamplay() )
		return 10;

	float length1 = m_iSpawnCheckRadiusSqr + 1; //Shortest length
	float length2 = m_iSpawnCheckRadiusSqr + 1; //Second shortest length
	float currentlength = 0;

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CGEMPPlayer *pPlayer = ToGEMPPlayer(UTIL_PlayerByIndex(i));
		if (pPlayer && pPlayer->IsConnected() && !pPlayer->IsObserver())
		{
			if (!pPlayer->IsAlive() || !pPlayer->CheckInPVS(this)) // We don't care about players who can't see us.
				continue;

			currentlength = (pPlayer->GetAbsOrigin() - GetAbsOrigin()).LengthSqr();
			if (currentlength < length1)
			{
				length2 = length1; //We have to shift length1 down one.  We already know it's lower than length2.
				length1 = currentlength;
			}
			else if (currentlength < length2)
				length2 = currentlength;
		}
	}

	// If the distances aren't low enough then we just tick down at max speed without calculating anything.
	if (length1 > m_iSpawnCheckHalfRadiusSqr || length2 > m_iSpawnCheckRadiusSqr)
		return 10;

	// Basically, 2 people being within the check distance of the armor with one of them within a quarter of it is the only way to get it to tick down at less than max rate.
	// Discourages standing on top of the armor spawn in the middle of a fight because "oh man i need 3 gauges of health"

	// It will never spawn if one of the players is standing right on top of it and the other player is within half the radius.
	if (length1 < 64 && length2 < m_iSpawnCheckHalfRadiusSqr)
		return 0;

	// Otherwise we only care about length 2.  Linear falloff, tick up faster the further away player2 is.
	float metric = 1 - sqrt(length2) / m_iSpawnCheckRadius;

	// If player2 is within 17% of the radius, the armor does not tick down at all.
	return (int)clamp(10 - 12 * metric, 0, 10);
}

void CGEArmorVest::UpdateOnRemove(void)
{
	BaseClass::UpdateOnRemove();

	// Notify Python about the ammobox
	if (GetScenario())
	{
		GetScenario()->OnArmorRemoved(this);
	}
}

void CGEArmorVest::AddSpawnProgressMod(CBasePlayer *pPlayer, int amount)
{
	int cappedamount = MIN(amount, m_iSpawnpoints * -1); // Can't put the armor below 0 spawn points.

	if (pPlayer)
	{
		int iPID = pPlayer->GetUserID();
		m_iPlayerPointContribution[iPID] += cappedamount;
	}

	m_iSpawnpoints += cappedamount;
}

void CGEArmorVest::ClearSpawnProgressMod(CBasePlayer *pPlayer)
{
	if (!pPlayer)
		return;

	int iPID = pPlayer->GetUserID();

	if (m_iSpawnpoints < m_iSpawnpointsgoal - 20) // Only do this next bit if we won't subtract points.
		m_iSpawnpoints = MIN(m_iSpawnpoints - m_iPlayerPointContribution[iPID], m_iSpawnpointsgoal - 20); // Add back all the spawn points that we stole but avoid having the armor respawn instantly after we died.

	m_iPlayerPointContribution[iPID] = 0;
}

void CGEArmorVest::ClearAllSpawnProgress()
{
	m_iSpawnpoints = 0;

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		m_iPlayerPointContribution[i] = 0;
	}
}

void CGEArmorVest::OnEnabled( void )
{
	// Respawn and wait for materialize
	Respawn();
	Materialize();
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

// Just going to recycle the one from the playerspawns because it is so good.
void CGEArmorVest::DEBUG_ShowProgress(float duration, int progress)
{
	// Only show this on local host servers
	if (engine->IsDedicatedServer())
		return;

	int line = 0;
	char tempstr[64];

	int r = (int)RemapValClamped(progress, 0, 10.0f, 255.0f, 0);
	int g = (int)RemapValClamped(progress, 0, 10.0f, 0, 255.0f);

	Color c(r, g, 0, 200);
	Vector mins = {-12, -12, 0};
	Vector maxes = {12, 12, 4};
	debugoverlay->AddBoxOverlay2(GetAbsOrigin(), mins, maxes, GetAbsAngles(), c, c, duration);

	Q_snprintf(tempstr, 64, "%s", GetClassname());
	EntityText(++line, tempstr, duration);

	if (!IsEnabled())
	{
		// We are disabled
		Q_snprintf(tempstr, 64, "DISABLED");
		EntityText(++line, tempstr, duration);
	}
	else if (!IsEffectActive(EF_NODRAW))
	{
		// We've spawned
		Q_snprintf(tempstr, 64, "SPAWNED");
		EntityText(++line, tempstr, duration);
	}
	else
	{
		// We are enabled, show our desirability and stats
		Q_snprintf(tempstr, 64, "Progress: %i/%i", m_iSpawnpoints, m_iSpawnpointsgoal);
		EntityText(++line, tempstr, duration);

		Q_snprintf(tempstr, 64, "Rate: %i", progress);
		EntityText(++line, tempstr, duration);

		Q_snprintf(tempstr, 64, "Radius: %i", m_iSpawnCheckRadius);
		EntityText(++line, tempstr, duration);
	}

	
}
