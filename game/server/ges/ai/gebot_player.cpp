///////////// Copyright © 2008 LodleNet. All rights reserved. /////////////
//
//   Project     : Server (GES)
//   File        : gebot_player.cpp
//   Description :
//      Encapsulates an NPC to use as a player entity
//
//   Created On: 3/23/2011
//   Created By: Jonathan White <killermonkey01@gmail.com>
////////////////////////////////////////////////////////////////////////////

#include "cbase.h"

#include "viewport_panel_names.h"
#include "predicted_viewmodel.h"
#include "in_buttons.h"
#include "ammodef.h"
#include "team.h"

#include "ge_utils.h"
#include "ent_hat.h"
#include "ge_gameplay.h"
#include "ge_loadoutmanager.h"
#include "ge_tokenmanager.h"
#include "ge_radarresource.h"
#include "ge_character_data.h"
#include "ge_achievement_defs.h"
#include "ge_weapon.h"
#include "ge_stats_recorder.h"
#include "gemp_gamerules.h"

#include "gebot_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define BOT_MAX_RESPAWN_DELAY 4.0f

LINK_ENTITY_TO_CLASS( bot_player, CGEBotPlayer );

IMPLEMENT_SERVERCLASS_ST( CGEBotPlayer, DT_GEBot_Player )
	SendPropEHandle( SENDINFO(m_pNPC) ),
END_SEND_TABLE()

BEGIN_DATADESC( CGEBotPlayer )
	DEFINE_THINKFUNC( PlayerDeathThink ),
END_DATADESC()

PRECACHE_REGISTER(bot_player);

CGEBotPlayer::CGEBotPlayer()
{
	m_flRespawnDelay = BOT_MAX_RESPAWN_DELAY;
}

CGEBotPlayer::~CGEBotPlayer( void )
{
	KnockOffHat( true );
	UTIL_Remove( m_pNPC.Get() );
}

bool CGEBotPlayer::LoadBrain( void )
{
	if ( !m_pNPC.Get() )
		return false;

	char bot_class[64];
	char gp_ident[64];
	Q_strncpy( gp_ident, GEGameplay()->GetScenario()->GetIdent(), 64 );
	Q_snprintf( bot_class, 64, "bot_%s", Q_strlower(gp_ident) );

	// Attempt to load the current gameplay's AI definition, otherwise fallback to DM
	if ( !m_pNPC->InitPyInterface( bot_class ) )
	{
		if ( !m_pNPC->InitPyInterface( "bot_deathmatch" ) )
		{
			// We failed to link into python, abort the spawn and remove ourselves
			m_pNPC->SUB_Remove();
			SUB_Remove();
			Warning( "GE Ai: Failed to create Python link for bot\n" );
			return false;
		}
	}

	return true;
}

void CGEBotPlayer::InitialSpawn( void )
{
	// Call down to initialize stuff. We set the player model to avoid null pointers
	CGEPlayer::SetPlayerModel();
	BaseClass::InitialSpawn();
	
	ClearFlags();
	AddFlag( FL_CLIENT | FL_FAKECLIENT );

	bool allowPrecache = CBaseEntity::IsPrecacheAllowed();
	CBaseEntity::SetAllowPrecache( true );

	// Create the actual NPC entity
	m_pNPC = dynamic_cast<CNPC_GEBase*>( CreateEntityByName("npc_gebase") );
	if ( m_pNPC )
	{
		// Attempt to load our brain from python
		if ( !LoadBrain() )
		{
			CBaseEntity::SetAllowPrecache( allowPrecache );
			return;
		}
		
		// Now Link in the player
		m_pNPC->InitBotInterface( this );
		m_pNPC->Precache();

		CBaseEntity::ChangeTeam( TEAM_SPECTATOR );
		PickDefaultSpawnTeam();

		// Spawn, Hide, and Freeze the NPC
		int tmp;
		CBaseEntity *pSpawn = EntFindSpawnPoint( 0, SPAWN_PLAYER_SPECTATOR, tmp );
		if ( pSpawn )
		{
			m_pNPC->SetAbsOrigin( pSpawn->GetAbsOrigin() );
			m_pNPC->SetLocalAngles( pSpawn->GetLocalAngles() );
		}
		else
		{
			m_pNPC->SetAbsOrigin( vec3_origin );
			m_pNPC->SetAbsAngles( vec3_angle );
		}
			
		m_pNPC->Spawn();
		m_pNPC->m_lifeState = LIFE_DEAD;
		m_pNPC->AddSolidFlags( FSOLID_NOT_SOLID );
		m_pNPC->SetCondition( COND_NPC_FREEZE );
		m_pNPC->ClearAllSchedules();

		// Kill off some client specific stuff
		SetSpawnState( SS_ACTIVE );
		m_bPreSpawn = false;
	}

	CBaseEntity::SetAllowPrecache( allowPrecache );
}

ConVar ge_bot_givespawninvuln("ge_bot_givespawninvuln", "0", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Bots are allowed to have spawn invulnerability.");

void CGEBotPlayer::Spawn( void )
{
	Vector oldPos = GetAbsOrigin();

	BaseClass::Spawn();

	// Ensure the proxy player doesn't interact with anything
	AddFlag( FL_NOTARGET ); // Don't let NPC's "see" us
	SetMoveType( MOVETYPE_NOCLIP );
	AddSolidFlags( FSOLID_NOT_SOLID );
	SetSolid( SOLID_NONE );
	// Make sure we are virginal
	RemoveAllAmmo();
	RemoveAllWeapons();

	// Only spawn the NPC if we spawned
	if ( m_pNPC.Get() && IsSpawnState( SS_ACTIVE ) && !IsObserver() )
	{
		m_pNPC->SetLocalOrigin( GetAbsOrigin() + Vector(0,0,5) );
		m_pNPC->SetAbsVelocity( vec3_origin );
		m_pNPC->SetLocalAngles( GetLocalAngles() );
		m_pNPC->CleanupScriptsOnTeleport( false );

		m_pNPC->Spawn();
		m_pNPC->Activate();
		m_pNPC->Wake( false );

		// Freeze us if we are in intermission
		if ( GEMPRules()->IsIntermission() )
		{
			m_pNPC->ClearAllSchedules();
			m_pNPC->SetCondition( COND_NPC_FREEZE );
		}

		// Setup collision rules based on teamplay
		if ( GERules()->IsTeamplay() )	
		{		
			if ( GetTeamNumber() == TEAM_MI6 )			
				m_pNPC->SetCollisionGroup( COLLISION_GROUP_MI6 );
			else if (GetTeamNumber() == TEAM_JANUS)		
				m_pNPC->SetCollisionGroup(COLLISION_GROUP_JANUS);
		}	
		else
		{		
			m_pNPC->SetCollisionGroup( COLLISION_GROUP_PLAYER );			
		}
	}
	else
	{
		// Make sure we are hidden!
		m_pNPC->m_lifeState = LIFE_DEAD;
		m_pNPC->RemoveAllWeapons();
		m_pNPC->AddSolidFlags( FSOLID_NOT_SOLID );
		m_pNPC->ClearAllSchedules();
		m_pNPC->SetCondition( COND_NPC_FREEZE );
	}

	if ( !ge_bot_givespawninvuln.GetBool() )
		StopInvul(); // Bots don't usually get spawn invuln since they don't care if they get spawn killed.
}

void CGEBotPlayer::ForceRespawn( void )
{
	if ( !GERules()->FPlayerCanRespawn(this) || m_bPreSpawn )
		return;
	
	Spawn();
}

void CGEBotPlayer::FreezePlayer( bool state /*=true*/ )
{
	if ( m_pNPC.Get() )
	{
		m_pNPC->ClearAllSchedules();
		m_pNPC->SetCondition( COND_NPC_FREEZE );
	}
	else
	{
		m_pNPC->SetCondition( COND_NPC_UNFREEZE );
	}
}

int CGEBotPlayer::UpdateTransmitState( void )
{
	// This ensures our NODRAW status doesn't trump network updates
	return SetTransmitState( FL_EDICT_PVSCHECK );
}

void CGEBotPlayer::PickDefaultSpawnTeam( void )
{
	if ( GERules()->IsTeamplay() == false )
	{
		ChangeTeam( TEAM_UNASSIGNED, true );
	}
	else
	{
		CTeam *pJanus = g_Teams[TEAM_JANUS];
		CTeam *pMI6 = g_Teams[TEAM_MI6];

		if ( pJanus == NULL || pMI6 == NULL )
		{
			ChangeTeam( random->RandomInt( TEAM_MI6, TEAM_JANUS ), true );
		}
		else
		{
			if ( pJanus->GetNumPlayers() > pMI6->GetNumPlayers() )
			{
				ChangeTeam( TEAM_MI6, true );
			}
			else if ( pJanus->GetNumPlayers() < pMI6->GetNumPlayers() )
			{
				ChangeTeam( TEAM_JANUS, true );
			}
			else
			{
				ChangeTeam( random->RandomInt( TEAM_MI6, TEAM_JANUS ), true );
			}
		}
	}
}

void CGEBotPlayer::ChangeTeam( int iTeam, bool bWasForced /* = false */ )
{
	// Bots never join the spectator team
	if ( iTeam == TEAM_SPECTATOR )
		return;

	// Don't let the bot join a team when teamplay is disabled
	if ( !GERules()->IsTeamplay() )
		iTeam = TEAM_UNASSIGNED;

	// Don't worry about this switch if they aren't actually switching teams
	if ( iTeam == GetTeamNumber() )
		return;

	// See if the gameplay will let us switch teams
	// TODO: Do we even need to make this a criteria for bots? Or just call in for awareness...
	if ( !bWasForced && !GEGameplay()->GetScenario()->CanPlayerChangeTeam( this, GetTeamNumber(), iTeam ) )
		return;

	// Skip GEMPPlayer explicitly
	CGEPlayer::ChangeTeam( iTeam );

	// We are changing teams, drop any tokens we might have
	DropAllTokens();

	// Set our bot to this team
	if ( m_pNPC )
		m_pNPC->ChangeTeam( iTeam );

	if ( GEMPRules()->IsTeamplay() )
		SetPlayerTeamModel();
	else
		SetPlayerModel();

	if ( !GEMPRules()->IsIntermission() && !m_bPreSpawn )
		Spawn();
}

bool CGEBotPlayer::PickPlayerModel( int team )
{
	// Randomly select a character for us to be (skipping over random)
	CUtlVector<const CGECharData*> vChars;
	GECharacters()->GetTeamMembers( team, vChars );

	while ( vChars.Count() > 0 )
	{
		int sel = GERandom<int>( vChars.Count() );
		const CGECharData *pChar = vChars[sel];

		if ( GEGameplay()->GetScenario()->CanPlayerChangeChar(this, pChar->szIdentifier) )
		{
			// We found one, set and forget
			int skin = GERandom<int>( pChar->m_pSkins.Count() );
			m_pNPC->SetModel( pChar->m_pSkins[skin]->szModel );
			SetModel( pChar->m_pSkins[skin]->szModel );
			m_iCharIndex = GECharacters()->GetIndex( pChar );
			m_iSkinIndex = skin;

			KnockOffHat( true );
			GiveHat();

			return true;
		}
		else
		{
			// Don't choose him again
			vChars.Remove( sel );
		}
	}

	return false;
}

void CGEBotPlayer::SetPlayerModel( void )
{
	if ( !m_pNPC.Get() )
		return;

	if ( GECharacters()->Count() == 0 )
	{
		Warning( "Character scripts are missing or invalid!\n" );
		return;
	}

	// Choose a random character from the list obeying gameplay rules
	if ( !PickPlayerModel( TEAM_UNASSIGNED ) )
	{
		// Uh oh, no characters?
		DevWarning( "Failed to find a character for %s, removing bot!\n", GetCleanPlayerName() );
		SUB_Remove();
	}
	
}

void CGEBotPlayer::SetPlayerTeamModel( void )
{
	if ( !m_pNPC.Get() )
		return;

	if ( !PickPlayerModel( GetTeamNumber() ) )
	{
		// Uh oh, no one on our team?
		DevWarning( "Failed to find a team character for %s!\n", GetCleanPlayerName() );
		SetPlayerModel();
	}
}

// Used by gameplay scenarios
void CGEBotPlayer::SetPlayerModel( const char* szCharName, int iCharSkin /*=0*/, bool bNoSelOverride /*=false*/ )
{
	if ( !m_pNPC.Get() )
		return;

	if ( !GEGameplay()->GetScenario()->CanPlayerChangeChar(this, szCharName) )
	{
		// Gameplay denied their selection (why??) go to random select
		if ( GEMPRules()->IsTeamplay() )
			SetPlayerTeamModel();
		else
			SetPlayerModel();

		return;
	}

	const CGECharData *pChar = GECharacters()->Get( szCharName );
	if( !pChar )
		return;

	// Make sure we aren't trying to join a class that is not a part
	// of the player's active team
	if ( GERules()->IsTeamplay() && pChar->iTeam != GetTeamNumber() ) 
		return;

	KnockOffHat( true );

	// Set our NPC model and skin
	m_pNPC->SetModel( pChar->m_pSkins[iCharSkin]->szModel );
	m_pNPC->m_nSkin.Set( pChar->m_pSkins[iCharSkin]->iWorldSkin );

	// Intentional skip of CGEMPPlayer
	CGEPlayer::SetPlayerModel( pChar->szIdentifier, iCharSkin );

	UTIL_ClientPrintAll( HUD_PRINTTALK, "#PLAYER_CHANGECHAR", GetPlayerName(), pChar->szShortName );

	// Put us somewhere new and reset our state
	// This fixes bots doing nothing after a character change
	ForceRespawn();
}

void CGEBotPlayer::FireBullets( const FireBulletsInfo_t &info )
{
	// We never fire bullets, this is just to disable spawn invuln when we shoot essentially
	if ( m_bInSpawnInvul && !IsObserver() && GEMPRules()->GetSpawnInvulnCanBreak() )
		StopInvul();
}

int CGEBotPlayer::OnTakeDamage( const CTakeDamageInfo &info )
{
	int prev_health = GetHealth();
	int prev_armor = ArmorValue();

	int dmg = BaseClass::OnTakeDamage( info );

	// Only call into the bot's damage func if we took health damage
	if ( GetHealth() < prev_health )
	{
		// Call into the bot to ensure consistency of health
		// CAI_BaseNPC::OnTakeDamage_Alive handles conditions but not death
		m_pNPC->OnTakeDamage_Alive( info );
	}
	else if ( ArmorValue() < prev_armor )
	{
		// Does everything CAI_BaseNPC::OnTakeDamage_Alive does W/O subtracting health!
		m_pNPC->OnTakeArmorDamage( info );
	}

	return dmg;
}

void CGEBotPlayer::Event_Killed( const CTakeDamageInfo &info )
{
	// Make sure we are clean (bot-player side)
	RemoveAllWeapons();
	RemoveAllAmmo();

	NotifyOnDeath();

	// Set us up to respawn in a random interval
	m_flRespawnDelay = 3.0f + GERandom<float>( BOT_MAX_RESPAWN_DELAY );

	// NOTE: We skip over CGEMPPlayer Event_Killed intentionally
	CGEPlayer::Event_Killed( info );

	// Look ahead, set this early so gameplays can use it right away
	if ( !GERules()->FPlayerCanRespawn(this) )
		SetSpawnState( SS_BLOCKED_GAMEPLAY );

	// Call back into the bot
	m_pNPC->Event_Killed( info );
}

void CGEBotPlayer::DropAllTokens( void )
{
	// If we define any special tokens, always drop them
	// We do it after the BaseClass stuff so we are consistent with KILL -> DROP
	if ( GEMPRules()->GetTokenManager()->GetNumTokenDef() )
	{
		for ( int i=0; i < MAX_WEAPONS; i++ )
		{
			CBaseCombatWeapon *pWeapon = m_pNPC->GetWeapon(i);
			if ( !pWeapon )
				continue;

			if ( GEMPRules()->GetTokenManager()->IsValidToken( pWeapon->GetClassname() ) )
				m_pNPC->Weapon_Drop( pWeapon );
		}
	}
}

void CGEBotPlayer::CreateRagdollEntity()
{
	// Bot's don't participate in the HL2MP ragdoll
}

void CGEBotPlayer::PreThink( void )
{
	BaseClass::PreThink();

	// Move us to where the NPC is for radar (mainly)
	SetAbsOrigin( m_pNPC->GetAbsOrigin() );
	SetLocalAngles( m_pNPC->GetLocalAngles() );
	SnapEyeAngles( m_pNPC->GetLocalAngles() );
	SetAbsVelocity( m_pNPC->GetAbsVelocity() );
}

void CGEBotPlayer::PostThink( void )
{
	// Clamp force on alive players to 1000
	if ( m_iHealth > 0 && m_vDmgForceThisFrame.Length() > 1000.0f )
	{
		m_vDmgForceThisFrame.NormalizeInPlace();
		m_vDmgForceThisFrame *= 1000.0f;
	}

	// Apply any damage forces
	m_pNPC->ApplyAbsVelocityImpulse( m_vDmgForceThisFrame );

	BaseClass::PostThink();
}

void CGEBotPlayer::GiveHat()
{
	if (IsObserver())
	{
		// Make sure we DO NOT have a hat as an observer
		KnockOffHat(true);
		return;
	}

	if (m_hHat.Get())
		return;

	const char* hatModel = GECharacters()->Get(m_iCharIndex)->m_pSkins[m_iSkinIndex]->szHatModel;
	if (!hatModel || hatModel[0] == '\0')
		return;

	SpawnHat(hatModel);
}

void CGEBotPlayer::SpawnHat(const char* hatModel, bool canBeRemoved)
{
	if (IsObserver() && !IsAlive()) // Observers can't have any hats.  Need this here so direct hat assignment doesn't make floating hats.
		return;

	// Get rid of any hats we're currently wearing.
	if (m_hHat.Get())
		KnockOffHat(true);

	// Simple check to ensure consistency
	int setnum, boxnum;
	if (m_pNPC->LookupHitbox("hat", setnum, boxnum))
		m_pNPC->SetHitboxSet(setnum);

	CBaseEntity *hat = CreateNewHat(EyePosition(), GetAbsAngles(), hatModel);
	hat->FollowEntity(m_pNPC, true);
	m_hHat = hat;
}

void CGEBotPlayer::KnockOffHat( bool bRemove /* = false */, const CTakeDamageInfo *dmg /* = NULL */ )
{
	if ( !m_pNPC.Get() )
		return;

	CBaseEntity *pHat = m_hHat.Get();

	if ( bRemove )
	{
		UTIL_Remove( pHat );
	}
	else if ( pHat )
	{
		Vector origin = m_pNPC->EyePosition();
		origin.z += 8.0f;

		pHat->Activate();
		pHat->SetLocalOrigin( origin );

		// Punt the hat appropriately
		if ( dmg )
			pHat->ApplyAbsVelocityImpulse( dmg->GetDamageForce() * 0.75f );
	}

	m_pNPC->SetHitboxSetByName( "default" );
	m_hHat = NULL;
}

extern ConVar ge_startarmed;
// Start armed settings:
//  0 = Only Slappers
//  1 = Give a knife
//  2 = Give first weapon in loadout
void CGEBotPlayer::GiveDefaultItems( void )
{
	EquipSuit( false );

	CBaseEntity *pGivenWeapon = NULL;
	// Always give slappers
	pGivenWeapon = m_pNPC->GiveNamedItem( "weapon_slappers" );

	const CGELoadout *loadout = GEMPRules()->GetLoadoutManager()->CurrLoadout();
	if ( loadout )
	{		
		// Give the weakest weapon in our set if more conditions are met
		if ( ge_startarmed.GetInt() >= 1 )
		{
			int wID = loadout->GetFirstWeapon();
			int aID = GetAmmoDef()->Index( GetAmmoForWeapon(wID) );

			// Give the player a slice of ammo for the lowest ranked weapon in the set
			m_pNPC->GiveAmmo( GetAmmoDef()->CrateAmount(aID), aID );
			pGivenWeapon = m_pNPC->GiveNamedItem( WeaponIDToAlias(wID) );
		}

		//if (ge_startarmed.GetInt() >= 2)
		//	pGivenWeapon = m_pNPC->GiveNamedItem("weapon_knife");
	} 
	else
	{
		// No loadout? Give a knife so they can still have some fun
		pGivenWeapon = m_pNPC->GiveNamedItem( "weapon_knife" );
	}

	// Switch to the best given weapon
	if ( pGivenWeapon )
		m_pNPC->Weapon_Switch((CBaseCombatWeapon*)pGivenWeapon);
}

CBaseCombatWeapon *CGEBotPlayer::GetActiveWeapon()
{
	return m_pNPC->GetActiveWeapon();
}

bool CGEBotPlayer::Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex /*=0*/ )
{
	return m_pNPC->Weapon_Switch( pWeapon, viewmodelindex );
}

void CGEBotPlayer::GiveNamedWeapon( const char* ident, int ammoCount, bool stripAmmo )
{
	if ( ammoCount > 0 )
	{
		int id = AliasToWeaponID( ident );
		const char* ammoId = GetAmmoForWeapon( id );

		int aID = GetAmmoDef()->Index( ammoId );
		m_pNPC->GiveAmmo( ammoCount, aID, false );
	}

	CGEWeapon *pWeapon = (CGEWeapon*) m_pNPC->GiveNamedItem( ident );

	if ( stripAmmo && pWeapon )
	{
		m_pNPC->RemoveAmmo( pWeapon->GetDefaultClip1(), pWeapon->m_iPrimaryAmmoType );
	}

}

int	CGEBotPlayer::GiveAmmo( int iCount, int iAmmoIndex, bool bSuppressSound )
{
	int amt = m_pNPC->GiveAmmo( iCount, iAmmoIndex, bSuppressSound );

	if( amt > 0 )
	{
		GEStats()->Event_PickedAmmo( this, amt );

		// If we picked up mines or grenades and the player doesn't already have them give them a weapon!
		char *name = GetAmmoDef()->GetAmmoOfIndex(iAmmoIndex)->pName;
		
		if ( Q_strstr(name,"mine") )
		{
			// Ok we picked up mines
			if ( !Q_strcmp(name, AMMO_PROXIMITYMINE) )
				m_pNPC->GiveNamedItem("weapon_proximitymine");
			else if ( !Q_strcmp(name, AMMO_TIMEDMINE) )
				m_pNPC->GiveNamedItem("weapon_timedmine");
			else if ( !Q_strcmp(name, AMMO_REMOTEMINE) )
				m_pNPC->GiveNamedItem("weapon_remotemine");
		}
		else if ( !Q_strcmp(name, AMMO_GRENADE) )
		{
			m_pNPC->GiveNamedItem("weapon_grenade");
		}
		else if ( !Q_strcmp(name, AMMO_TKNIFE) )
		{
			m_pNPC->GiveNamedItem("weapon_knife_throwing");
		}
	}

	return amt;
}

bool CGEBotPlayer::BumpWeapon( CBaseCombatWeapon *pWeapon )
{
	if ( !m_pNPC )
		return false;

	return m_pNPC->BumpWeapon( (CGEWeapon*) pWeapon );
}

void CGEBotPlayer::StripAllWeapons()
{
	if ( !m_pNPC )
		return;

	if ( m_pNPC->GetActiveWeapon() )
		m_pNPC->GetActiveWeapon()->Holster();

	m_pNPC->RemoveAllAmmo();
	m_pNPC->RemoveAllWeapons();
}

void CGEBotPlayer::RemoveAllItems( bool removeSuit )
{
	BaseClass::RemoveAllItems( removeSuit );
	StripAllWeapons();
}

extern void respawn(CBaseEntity *pEdict, bool fCopyCorpse);
void CGEBotPlayer::PlayerDeathThink( void )
{
	SetNextThink( gpGlobals->curtime + 0.1f );

	// Make sure we are dead
	m_lifeState = LIFE_DEAD;
	m_pNPC->m_lifeState = LIFE_DEAD;

	if ( IsSpawnState( SS_BLOCKED_NO_SPOT ) && gpGlobals->curtime < m_flNextSpawnTry )
		return;

	if ( gpGlobals->curtime > (m_flDeathTime + m_flRespawnDelay) && g_pGameRules->FPlayerCanRespawn( this ) )
	{
		m_nButtons = 0;
		m_iRespawnFrames = 0;

		respawn( this, false );
	}
}

CGEBotPlayer *ToGEBotPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity )
		return NULL;

	if ( pEntity->IsNPC() )
	{
#ifdef _DEBUG
		return dynamic_cast<CGEBotPlayer*>( ((CNPC_GEBase*)pEntity)->GetBotPlayer() );
#else
		return static_cast<CGEBotPlayer*>( ((CNPC_GEBase*)pEntity)->GetBotPlayer() );
#endif
	}
	else if ( pEntity->IsPlayer() && (pEntity->GetFlags() & FL_FAKECLIENT) )
	{
#ifdef _DEBUG
		return dynamic_cast<CGEBotPlayer*>( pEntity );
#else
		return static_cast<CGEBotPlayer*>( pEntity );
#endif
	}

	return NULL;
}
