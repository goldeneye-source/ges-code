///////////// Copyright 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_player.cpp
// Description:
//      Server side version of the player class used in the GE Game.
//
// Created On: 23 Feb 08, 16:35
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"

#include "in_buttons.h"
#include "ammodef.h"
#include "predicted_viewmodel.h"
#include "viewport_panel_names.h"
#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "ilagcompensationmanager.h"

#include "ge_tokenmanager.h"
#include "ge_player_shared.h"
#include "ge_playerresource.h"
#include "ge_character_data.h"
#include "ge_stats_recorder.h"
#include "ge_weapon.h"
#include "ge_bloodscreenvm.h"
#include "grenade_gebase.h"
#include "ent_hat.h"
#include "gemp_gamerules.h"

#include "ge_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


IMPLEMENT_SERVERCLASS_ST(CGEPlayer, DT_GE_Player)
	SendPropEHandle( SENDINFO( m_hActiveLeftWeapon ) ),
	SendPropInt( SENDINFO( m_ArmorValue ) ),
	SendPropInt( SENDINFO( m_iMaxArmor ) ),
	SendPropInt( SENDINFO( m_iMaxHealth ) ),

	SendPropBool( SENDINFO( m_bInAimMode) ),

	SendPropEHandle( SENDINFO( m_hHat ) ),
	SendPropInt( SENDINFO( m_takedamage ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CGEPlayer )
END_DATADESC()

#define GE_COMMAND_MAX_RATE		0.5f
#define GE_HITOTHER_DELAY		0.15f

#ifdef _MSC_VER
#pragma warning( disable : 4355 )
#endif

CGEPlayer::CGEPlayer()
{
	m_iAimModeState = AIM_NONE;
	m_bInSpawnInvul = false;

	m_pHints = new CHintSystem;
	m_pHints->Init( this, 2, NULL );

	m_flFullZoomTime = 0.0f;
	m_flDamageMultiplier = 1.0f;
	m_flSpeedMultiplier = 1.0f;
	m_flEndInvulTime = 0.0f;

	m_iCharIndex = m_iSkinIndex = -1;

	m_iFavoriteWeapon = WEAPON_NONE;
	m_iScoreBoardColor = 0;
}

CGEPlayer::~CGEPlayer( void )
{
	delete m_pHints;
}

void CGEPlayer::Precache( void )
{
	PrecacheParticleSystem( "ge_impact_add" );
	PrecacheModel( "models/VGUI/bloodanimation.mdl" );
	PrecacheScriptSound( "GEPlayer.HitOther" );
#if 0
	PrecacheScriptSound( "GEPlayer.HitPainMale" );
	PrecacheScriptSound( "GEPlayer.HitPainFemale" );
#endif

	BaseClass::Precache();
}

bool CGEPlayer::AddArmor( int amount )
{
	if ( ArmorValue() < GetMaxArmor() )
	{
		GEStats()->Event_PickedArmor( this, min( GetMaxArmor() - ArmorValue(), amount ) );
		IncrementArmorValue( amount, GetMaxArmor() );

		CPASAttenuationFilter filter( this, "ArmorVest.Pickup" );
		EmitSound( filter, entindex(), "ArmorVest.Pickup" );

		CSingleUserRecipientFilter user( this );
		user.MakeReliable();
		UserMessageBegin( user, "ItemPickup" );
			WRITE_STRING( "item_armorvest" );
		MessageEnd();

		// Tell plugins what we picked up
		NotifyPickup( amount < MAX_ARMOR ? "item_armorvest_half" : "item_armorvest", 2 );

		return true;
	}
	return false;
}

// Impulse 101 was issued!
void CGEPlayer::GiveAllItems( void )
{
	EquipSuit(false);

	SetHealth( GetMaxHealth() );
	AddArmor( GetMaxArmor() );

	CBasePlayer::GiveAmmo( AMMO_9MM_MAX, AMMO_9MM );
	CBasePlayer::GiveAmmo( AMMO_GOLDENGUN_MAX, AMMO_GOLDENGUN );
	CBasePlayer::GiveAmmo( AMMO_MAGNUM_MAX, AMMO_MAGNUM );
	CBasePlayer::GiveAmmo( AMMO_RIFLE_MAX, AMMO_RIFLE );
	CBasePlayer::GiveAmmo( AMMO_BUCKSHOT_MAX, AMMO_BUCKSHOT );
	CBasePlayer::GiveAmmo( AMMO_REMOTEMINE_MAX, AMMO_REMOTEMINE );
	CBasePlayer::GiveAmmo( AMMO_PROXIMITYMINE_MAX, AMMO_PROXIMITYMINE );
	CBasePlayer::GiveAmmo( AMMO_TIMEDMINE_MAX, AMMO_TIMEDMINE );
	CBasePlayer::GiveAmmo( AMMO_GRENADE_MAX, AMMO_GRENADE );
	CBasePlayer::GiveAmmo( AMMO_TKNIFE_MAX, AMMO_TKNIFE );
	CBasePlayer::GiveAmmo( AMMO_SHELL_MAX, AMMO_SHELL );
	CBasePlayer::GiveAmmo( AMMO_ROCKET_MAX, AMMO_ROCKET );

	GiveNamedItem( "weapon_slappers" );
	GiveNamedItem( "weapon_knife" );
	GiveNamedItem( "weapon_knife_throwing" );

	GiveNamedItem( "weapon_pp7" );
	GiveNamedItem( "weapon_pp7_silenced" );
	GiveNamedItem( "weapon_silver_pp7" );
	GiveNamedItem( "weapon_golden_pp7" );
	GiveNamedItem( "weapon_golden_gun" );
	GiveNamedItem( "weapon_cmag" );
	GiveNamedItem( "weapon_dd44" );

	GiveNamedItem( "weapon_sniper_rifle" );

	GiveNamedItem( "weapon_kf7" );
	GiveNamedItem( "weapon_ar33" );
	GiveNamedItem( "weapon_rcp90" );
	GiveNamedItem( "weapon_zmg" );
	GiveNamedItem( "weapon_d5k" );
	GiveNamedItem( "weapon_d5k_silenced" );
	GiveNamedItem( "weapon_klobb" );
	GiveNamedItem( "weapon_phantom" );

	GiveNamedItem( "weapon_shotgun" );
	GiveNamedItem( "weapon_auto_shotgun" );

	GiveNamedItem( "weapon_grenade_launcher" );
	GiveNamedItem( "weapon_rocket_launcher" );

	GiveNamedItem( "weapon_moonraker" );

	GiveNamedItem( "weapon_remotemine" );
	GiveNamedItem( "weapon_proximitymine" );
	GiveNamedItem( "weapon_timedmine" );
	GiveNamedItem( "weapon_grenade" );
}

int CGEPlayer::GiveAmmo( int nCount, int nAmmoIndex, bool bSuppressSound )
{
	int amt = BaseClass::GiveAmmo( nCount, nAmmoIndex, bSuppressSound );

	if( amt > 0 )
	{
		GEStats()->Event_PickedAmmo( this, amt );

		// If we picked up mines or grenades and the player doesn't already have them give them a weapon!
		char *name = GetAmmoDef()->GetAmmoOfIndex(nAmmoIndex)->pName;

		// Tell plugins what we grabbed
		NotifyPickup( name, 1 );
		
		if ( Q_strstr(name,"mine") )
		{
			// Ok we picked up mines
			if ( !Q_strcmp(name,AMMO_PROXIMITYMINE) )
				GiveNamedItem("weapon_proximitymine");
			else if ( !Q_strcmp(name,AMMO_TIMEDMINE) )
				GiveNamedItem("weapon_timedmine");
			else if ( !Q_strcmp(name,AMMO_REMOTEMINE) )
				GiveNamedItem("weapon_remotemine");
		}
		else if ( !Q_strcmp(name,AMMO_GRENADE) )
		{
			GiveNamedItem("weapon_grenade");
		}
		else if ( !Q_strcmp(name,AMMO_TKNIFE) )
		{
			GiveNamedItem("weapon_knife_throwing");
		}
	}

	return amt;
}				


void CGEPlayer::PreThink( void )
{
	// See if the player is pressing the AIMMODE button
	CheckAimMode();

	// Adjust our speed! (NOTE: crouch speed is taken into account in CGameMovement::HandleDuckingSpeedCrop)
	if ( IsInAimMode() )
		SetMaxSpeed( GE_AIMED_SPEED * GEMPRules()->GetSpeedMultiplier( this ) );
	else
		SetMaxSpeed( GE_NORM_SPEED * GEMPRules()->GetSpeedMultiplier( this ) );

	// Invulnerability checks (make sure we are never invulnerable forever!)
	if ( m_takedamage == DAMAGE_NO && m_flEndInvulTime < gpGlobals->curtime && !IsObserver() )
		StopInvul();

	if ( GetObserverMode() > OBS_MODE_FREEZECAM )
	{
		CheckObserverSettings();	// do this each frame
	}

	// Reset per-frame variables
	m_bSentHitSoundThisFrame = false;

	BaseClass::PreThink();
}

void CGEPlayer::PostThink( void )
{
	BaseClass::PostThink();

	// Don't apply invuln if the damage was caused by us
	if ( m_iDmgTakenThisFrame > 0 && m_pCurrAttacker != this )
	{
		// Record our attacker for inter-invuln attacks
		m_pLastAttacker = m_pCurrAttacker;
		m_iPrevDmgTaken = m_iDmgTakenThisFrame;

		// Give us some invuln and a little bonus if we are hurting
		float flTime = 0.62f; //0.667f;
		if ( GetHealth() < (GetMaxHealth() / 2) )
			flTime += 0.006f * (((float)GetMaxHealth() / 2.0f) - GetHealth());
		
		StartInvul( flTime );
	}
	else if ( m_pCurrAttacker == this )
	{
		// If we hurt ourselves give us half the normal invuln time
		StartInvul( 0.33f );
	}

	// Clamp force on alive players to 1000
	if ( m_iHealth > 0 && m_vDmgForceThisFrame.Length() > 1000.0f )
	{
		m_vDmgForceThisFrame.NormalizeInPlace();
		m_vDmgForceThisFrame *= 1000.0f;
	}

	ApplyAbsVelocityImpulse( m_vDmgForceThisFrame );

	// Reset our damage statistics
	m_pCurrAttacker = NULL;
	m_iViewPunchScale = 0;
	m_iDmgTakenThisFrame = 0;
	m_vDmgForceThisFrame = vec3_origin;
}

void CGEPlayer::CheatImpulseCommands( int iImpulse )
{
	switch ( iImpulse )
	{
		case 101:
			{
				if( sv_cheats->GetBool() )
					GiveAllItems();
			}
			break;

		default:
			BaseClass::CheatImpulseCommands( iImpulse );
	}
}

bool CGEPlayer::ShouldRunRateLimitedCommand( const CCommand &args )
{
	int i = m_RateLimitLastCommandTimes.Find( args[0] );
	if ( i == m_RateLimitLastCommandTimes.InvalidIndex() )
	{
		m_RateLimitLastCommandTimes.Insert( args[0], gpGlobals->curtime );
		return true;
	}
	else if ( (gpGlobals->curtime - m_RateLimitLastCommandTimes[i]) < GE_COMMAND_MAX_RATE )
	{
		// Too fast.
		return false;
	}
	else
	{
		m_RateLimitLastCommandTimes[i] = gpGlobals->curtime;
		return true;
	}
}

// This function checks to see if we are invulnerable, if we are and the time limit hasn't passed by
// then don't do the damage applied. Otherwise, go for it!
int CGEPlayer::OnTakeDamage( const CTakeDamageInfo &inputinfo )
{
	// Reset invulnerability if we take world or self damage
	if ( inputinfo.GetAttacker()->IsWorld() || inputinfo.GetAttacker() == this || Q_stristr("trigger_", inputinfo.GetAttacker()->GetClassname()) )
		StopInvul();

	int dmg = BaseClass::OnTakeDamage(inputinfo);

	// Did we accumulate more damage?
	if ( m_iDmgTakenThisFrame < (m_DmgTake + m_DmgSave) )
	{
		// Record our attacker
		m_pCurrAttacker = inputinfo.GetAttacker();

		// Add this damage to our frame's total damage
		// m_DmgTake and m_DmgSave accumulate in the frame so we can just keep overwriting this value
		m_iDmgTakenThisFrame = m_DmgTake + m_DmgSave;

		// Do a view punch in the proper direction (opposite of the damage favoring left/right)
		// based on the damage direction
		Vector view;
		QAngle angles;
		EyeVectors(&view);
		
		Vector dmgdir = view - inputinfo.GetDamageForce();

		VectorNormalize(dmgdir);
		VectorAngles( dmgdir, angles);

		// Scale the punch in the different directions. Prefer yaw(y) and negative pitch(x)
		angles.y *= -0.030f;
		angles.z *= 0.025f;
		angles.x *= -0.0375f;

		float aimscale = IsInAimMode() ? 0.3f : 1.0f;

		// Apply the scaled down view punch
		ViewPunch( angles*0.6f*aimscale*(pow(0.5f,m_iViewPunchScale)) );
		++m_iViewPunchScale;

		// Print this out when we get damage (developer 1 must be on)
		float flFractionalDamage = m_DmgTake + m_DmgSave - floor( inputinfo.GetDamage() );
		float flIntegerDamage = m_DmgTake + m_DmgSave - flFractionalDamage;
		DevMsg( "%s took %0.1f damage to hitbox %i, %i HP / %i AP remaining.\n", GetPlayerName(), flIntegerDamage, LastHitGroup(), GetHealth(), ArmorValue() );

		CBasePlayer *pAttacker = ToBasePlayer( inputinfo.GetAttacker() );
		if ( !pAttacker )
			return dmg;

		// Tell our attacker that they damaged me
		ToGEPlayer( pAttacker )->Event_DamagedOther( this, flIntegerDamage, inputinfo );

		// Play our hurt sound
		HurtSound( inputinfo );

		CBaseEntity *pInflictor;
		if ( pAttacker == inputinfo.GetInflictor() )
			pInflictor = pAttacker->GetActiveWeapon();
		else
			pInflictor = inputinfo.GetInflictor();

		if ( !pInflictor )
			return dmg;

		int weapid = WEAPON_NONE;
		if ( Q_stristr(pInflictor->GetClassname(), "weapon_") )
			weapid = ToGEWeapon( (CBaseCombatWeapon*)pInflictor )->GetWeaponID();
		else if ( !pInflictor->IsNPC() && Q_stristr(pInflictor->GetClassname(), "npc_") )
			weapid = ToGEGrenade( pInflictor )->GetWeaponID();
		else if ( inputinfo.GetDamageType() & DMG_BLAST )
			weapid = WEAPON_EXPLOSION;

		// Tell everything that we got hurt
		IGameEvent *event = gameeventmanager->CreateEvent( "player_hurt" );
		if( event )
		{
			event->SetInt( "userid", GetUserID() );
			event->SetInt( "attacker", pAttacker->GetUserID() );
			event->SetInt( "weaponid", weapid );
			event->SetInt( "health", max(0, GetHealth()) );
			event->SetInt( "armor", ArmorValue() );
			event->SetInt( "damage", flIntegerDamage );
			event->SetInt( "dmgtype", inputinfo.GetDamageType() );
			event->SetInt( "hitgroup", LastHitGroup() );
			event->SetBool( "penetrated", FBitSet(inputinfo.GetDamageStats(), FIRE_BULLETS_PENETRATED_SHOT)?true:false );
			gameeventmanager->FireEvent( event );
		}
	}
	else
	{
		DevMsg( "%s's invulnerability blocked that hit!\n", GetPlayerName() );
	}

	return dmg;
}

// This simply accumulates our force from damages in this frame to be applied in PostThink
extern float DamageForce( const Vector &size, float damage );
int CGEPlayer::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	CBaseEntity * attacker = inputInfo.GetAttacker();
	if ( !attacker )
		return 0;

	// Check to see if we have already calculated a damage force
	Vector force = inputInfo.GetDamageForce();
	if ( force == vec3_origin )
	{
		Vector vecDir = vec3_origin;
		if ( inputInfo.GetInflictor() && GetMoveType() == MOVETYPE_WALK && !attacker->IsSolidFlagSet(FSOLID_TRIGGER) )
		{
			vecDir = inputInfo.GetInflictor()->WorldSpaceCenter() - Vector ( 0, 0, 10 ) - WorldSpaceCenter();
			VectorNormalize( vecDir );

			force = vecDir * -DamageForce( WorldAlignSize(), inputInfo.GetBaseDamage() );
		}
	}

	// Limit our up/down force if this wasn't an explosion
	if ( inputInfo.GetDamageType() &~ DMG_BLAST )
		force.z = clamp( force.z, -250.0f, 250.0f );
	else if ( m_iHealth > 0 )
		force.z = clamp( force.z, -450.0f, 450.0f );

	m_vDmgForceThisFrame += force;

	return BaseClass::OnTakeDamage_Alive( inputInfo );
}


void CGEPlayer::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr )
{
	CTakeDamageInfo info = inputInfo;
	bool doTrace = false;
	// World or self damage removes our invuln status (and it doesn't get reapplied!)
	// VC: If a DIFFERENT attacker gets to us during our invuln period we will *test* to see if they inflict MORE damage than
	//     our previous attacker and we will apply the difference of the damages and reapply invuln
	if ( info.GetAttacker()->IsWorld() || info.GetAttacker() == this )
		StopInvul();
	else if ( m_pLastAttacker && m_pLastAttacker != info.GetAttacker() )
		m_takedamage = DAMAGE_YES;

	if ( m_takedamage != DAMAGE_NO )
	{
		// Try to get the weapon and see if it wants hit location damage
		CGEWeapon *pWeapon = NULL;
		if ( info.GetWeapon() )
		{
			pWeapon = ToGEWeapon( (CBaseCombatWeapon*)info.GetWeapon() );

			// Only do this calculation if we allow it for the weapon
			if ( pWeapon->GetGEWpnData().iFlags & ITEM_FLAG_DOHITLOCATIONDMG )
				doTrace = true;
		}

		// Apply attacker's damage multiplier
		CGEPlayer *pGEPlayer = ToGEPlayer(info.GetAttacker());
		if ( pGEPlayer )
			info.ScaleDamage( pGEPlayer->GetDamageMultiplier() );

		// If we want hitlocation dmg and we have a weapon to refer to do it!
		if ( doTrace && pWeapon )
		{
			// Set the last part where we got hit
			SetLastHitGroup( ptr->hitgroup );

			switch ( ptr->hitgroup )
			{
			case HITGROUP_GENERIC:
				break;
			case HITGROUP_GEAR:
				KnockOffHat( false, &info );
				info.ScaleDamage( 0 );
				break;
			case HITGROUP_HEAD:
				KnockOffHat( false, &info );
				info.ScaleDamage( pWeapon->GetHitBoxDataModifierHead() );
				break;
			case HITGROUP_CHEST:
				info.ScaleDamage( pWeapon->GetHitBoxDataModifierChest() );
				break;
			case HITGROUP_STOMACH:
				info.ScaleDamage( pWeapon->GetHitBoxDataModifierStomach() );
				break;
			case HITGROUP_LEFTARM:
				info.ScaleDamage( pWeapon->GetHitBoxDataModifierLeftArm() );
				break;
			case HITGROUP_RIGHTARM:
				info.ScaleDamage( pWeapon->GetHitBoxDataModifierRightArm() );
				break;
			case HITGROUP_LEFTLEG:
				info.ScaleDamage( pWeapon->GetHitBoxDataModifierLeftLeg() );
				break;
			case HITGROUP_RIGHTLEG:
				info.ScaleDamage( pWeapon->GetHitBoxDataModifierRightLeg() );
				break;
			default:
				break;
			}
		}
		else
		{
			SetLastHitGroup( HITGROUP_GENERIC );
		}

		// Since we scaled our damage, now check to see if we did enough damage to override the invuln
		// if we didn't we set us back to invuln (pending we actually were in it previously)
		if ( info.GetAttacker() != m_pLastAttacker && info.GetDamage() > m_iPrevDmgTaken )
			info.SetDamage( info.GetDamage() - m_iPrevDmgTaken );
		else if ( m_pLastAttacker )
			m_takedamage = DAMAGE_NO;

		BaseClass::TraceAttack( info, vecDir, ptr );
	}
}

void CGEPlayer::HurtSound( const CTakeDamageInfo &info )
{
#if 0
	const CGECharData *pChar = GECharacters()->Get( m_iCharIndex );
	if ( pChar )
	{
		// Play the hurt sound to all players within PAS except us, we hear our own gasp
		if ( pChar->iGender == CHAR_GENDER_MALE )
		{
			CPASAttenuationFilter filter( this, "GEPlayer.HitPainMale" );
			filter.RemoveRecipient( this );
			EmitSound( filter, entindex(), "GEPlayer.HitPainMale" );
		}
		else
		{
			CPASAttenuationFilter filter( this, "GEPlayer.HitPainFemale" );
			filter.RemoveRecipient( this );
			EmitSound( filter, entindex(), "GEPlayer.HitPainFemale" );
		}
	}
#endif
}

void CGEPlayer::Event_DamagedOther( CGEPlayer *pOther, int dmgTaken, const CTakeDamageInfo &inputInfo )
{
	if ( !IsBotPlayer() )
	{
		int noSound = atoi( engine->GetClientConVarValue( entindex(), "cl_ge_nohitsound" ) );
		if ( !m_bSentHitSoundThisFrame && (inputInfo.GetDamageType() & DMG_BULLET) == DMG_BULLET && noSound == 0 )
		{
			float playAt = gpGlobals->curtime - (g_pPlayerResource->GetPing(entindex()) / 1000) + GE_HITOTHER_DELAY;
			CSingleUserRecipientFilter filter( this );
			filter.MakeReliable();
			EmitSound( filter, entindex(), "GEPlayer.HitOther", 0, playAt );

			m_bSentHitSoundThisFrame = true;
		}
	}
	
	// TraceBleed occurs in TraceAttack and is defined in baseentity_shared.cpp
	// SpawnBlood occurs in TraceAttack and is defined as UTIL_BloodDrips in util_shared.cpp
	// GE:S spawns blood on the client, the server blood is for all other players
}

void CGEPlayer::Event_Killed( const CTakeDamageInfo &info )
{
	if ( !IsBotPlayer() )
	{
		CBaseViewModel *pViewModel = GetViewModel( 1 );
		int noBlood = atoi( engine->GetClientConVarValue( entindex(), "cl_ge_disablebloodscreen" ) );
		if ( noBlood == 0 && pViewModel )
		{
			pViewModel->RemoveEffects( EF_NODRAW );
			int seq = GERandom<int>(4) + 1;
			pViewModel->SendViewModelMatchingSequence( seq );
		}
	}

	KnockOffHat();
	DropAllTokens();

	BaseClass::Event_Killed( info );
}

void CGEPlayer::DropAllTokens()
{
	// If we define any special tokens, always drop them
	// We do it after the BaseClass stuff so we are consistent with KILL -> DROP
	if ( GEMPRules()->GetTokenManager()->GetNumTokenDef() )
	{
		for ( int i=0; i < MAX_WEAPONS; i++ )
		{
			CBaseCombatWeapon *pWeapon = GetWeapon(i);
			if ( !pWeapon )
				continue;

			if ( GEMPRules()->GetTokenManager()->IsValidToken( pWeapon->GetClassname() ) )
				Weapon_Drop( pWeapon );
		}
	}
}

void CGEPlayer::KnockOffHat( bool bRemove /* = false */, const CTakeDamageInfo *dmg /* = NULL */ )
{
	CBaseEntity *pHat = m_hHat;

	if ( bRemove )
	{
		UTIL_Remove( pHat );
	}
	else if ( pHat )
	{
		Vector origin = EyePosition();
		origin.z += 8.0f;

		pHat->Activate();
		pHat->SetAbsOrigin( origin );
		pHat->RemoveEffects( EF_NODRAW );

		// Punt the hat appropriately
		if ( dmg )
			pHat->ApplyAbsVelocityImpulse( dmg->GetDamageForce() * 0.75f );
	}
	
	SetHitboxSetByName( "default" );
	m_hHat = NULL;
}

void CGEPlayer::GiveHat()
{
	if ( IsObserver() )
	{
		// Make sure we DO NOT have a hat as an observer
		KnockOffHat( true );
		return;
	}

	if ( m_hHat.Get() )
		return;

	const char* hatModel = GECharacters()->Get(m_iCharIndex)->m_pSkins[m_iSkinIndex]->szHatModel;
	if ( !hatModel || hatModel[0] == '\0' )
		return;

	// Simple check to ensure consistency
	int setnum, boxnum;
	if ( LookupHitbox( "hat", setnum, boxnum ) )
		SetHitboxSet( setnum );

	CBaseEntity *hat = CreateNewHat( EyePosition(), GetAbsAngles(), hatModel );
	hat->FollowEntity( this, true );
	m_hHat = hat;
}

void CGEPlayer::InitialSpawn( void )
{
	BaseClass::InitialSpawn();

	// Early out if we are a bot
	if ( IsBot() )
		return;

	CGEBloodScreenVM *vm = (CGEBloodScreenVM*)CreateEntityByName( "gebloodscreen" );
	if ( vm )
	{
		vm->SetAbsOrigin( GetAbsOrigin() );
		vm->SetOwner( this );
		vm->SetIndex( 1 );
		DispatchSpawn( vm );
		vm->FollowEntity( this, false );
		m_hViewModel.Set( 1, vm );

		vm->SetModel( "models/VGUI/bloodanimation.mdl" );
	}

	HideBloodScreen();

	// Disable npc interp if we are localhost
	if ( !engine->IsDedicatedServer() && entindex() == 1 )
		engine->ClientCommand( edict(), "cl_interp_npcs 0" );
}

void CGEPlayer::Spawn()
{
	BaseClass::Spawn();

	HideBloodScreen();

	m_iDmgTakenThisFrame = 0;
	m_iAimModeState = AIM_NONE;
}

void CGEPlayer::HideBloodScreen( void )
{
	// Bots don't have blood screens
	if ( IsBot() )
		return;

	CBaseViewModel *pViewModel = GetViewModel( 1 );
	if ( pViewModel )
	{
		pViewModel->SendViewModelMatchingSequence( 0 );
		pViewModel->AddEffects( EF_NODRAW );
	}
}

bool CGEPlayer::StartObserverMode( int mode )
{
	VPhysicsDestroyObject();
	HideBloodScreen();

	// Disable the flashlight
	FlashlightTurnOff();

	return CBasePlayer::StartObserverMode( mode );
}

void CGEPlayer::FinishClientPutInServer()
{
	InitialSpawn();
	Spawn();
}

void CGEPlayer::StartInvul( float time )
{
	DevMsg( "%s is invulnerable for %0.2f seconds...\n", GetPlayerName(), time );
	m_takedamage = DAMAGE_NO;
	m_flEndInvulTime = gpGlobals->curtime + time;
}

void CGEPlayer::StopInvul( void )
{
	DevMsg( "%s lost invulnerability...\n", GetPlayerName() );
	m_pLastAttacker = m_pCurrAttacker = NULL;
	m_iPrevDmgTaken = 0;
	m_takedamage = DAMAGE_YES;
	m_bInSpawnInvul = false;	
}

// Event notifier for plugins to tell when a player picks up stuff
void CGEPlayer::NotifyPickup( const char *classname, int type )
{
	IGameEvent *event = gameeventmanager->CreateEvent( "item_pickup" );
	if ( event )
	{
		event->SetInt( "userid", GetUserID() );
		event->SetString( "classname", classname );
		event->SetInt( "type", type );
		gameeventmanager->FireEvent( event, true );
	}
}

void CGEPlayer::SetPlayerModel( void )
{
	const CGECharData *pChar = GECharacters()->GetFirst( TEAM_UNASSIGNED );
	if ( !pChar )
	{
		Warning( "Character scripts are missing or invalid!\n" );
		return;
	}

	SetModel( pChar->m_pSkins[0]->szModel );
	m_nSkin.Set( pChar->m_pSkins[0]->iWorldSkin );

	m_iCharIndex = GECharacters()->GetIndex( pChar );
	m_iSkinIndex = 0;

	KnockOffHat( true );
	GiveHat();
}

void CGEPlayer::SetPlayerModel( const char* szCharIdent, int iCharSkin /*=0*/, bool bNoSelOverride /*=false*/ )
{
	const CGECharData *pChar = GECharacters()->Get( szCharIdent );
	if( !pChar )
	{
		Warning( "Invalid character ident %s chosen!\n", szCharIdent );
		return;
	}

	// Make sure our skin is "in bounds"
	iCharSkin = clamp( iCharSkin, 0, pChar->m_pSkins.Count()-1 );
	
	// Set the players models and skin based on the skin they chose
	SetModel( pChar->m_pSkins[iCharSkin]->szModel );
	m_nSkin.Set( pChar->m_pSkins[iCharSkin]->iWorldSkin );

	m_iCharIndex = GECharacters()->GetIndex( pChar );
	m_iSkinIndex = iCharSkin;

	// Tell everyone what character we changed too
	IGameEvent *event = gameeventmanager->CreateEvent( "player_changeident" );
	if( event )
	{
		event->SetInt( "playerid", entindex() );
		event->SetString( "ident", pChar->szIdentifier );
		gameeventmanager->FireEvent( event );
	}

	// Deal with hats
	KnockOffHat( true );
	GiveHat();
}

const char* CGEPlayer::GetCharIdent( void )
{
	const CGECharData *pChar = GECharacters()->Get( m_iCharIndex );
	return pChar ? pChar->szIdentifier : "";
}

void CGEPlayer::GiveNamedWeapon(const char* ident, int ammoCount, bool stripAmmo)
{
	if (ammoCount > 0)
	{
		int id = AliasToWeaponID( ident );
		const char* ammoId = GetAmmoForWeapon(id);

		int aID = GetAmmoDef()->Index( ammoId );
		GiveAmmo( ammoCount, aID, false );
	}

	CGEWeapon *pWeapon = (CGEWeapon*)GiveNamedItem(ident);

	if (stripAmmo && pWeapon)
	{
		RemoveAmmo( pWeapon->GetDefaultClip1(), pWeapon->m_iPrimaryAmmoType );
	}

}

void CGEPlayer::StripAllWeapons()
{
	if ( GetActiveWeapon() )
		GetActiveWeapon()->Holster();

	RemoveAllItems(false);
}

#include "npc_gebase.h"

CGEPlayer* ToGEPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity )
		return NULL;

	if ( pEntity->IsNPC() )
	{
#ifdef _DEBUG
		return dynamic_cast<CGEPlayer*>( ((CNPC_GEBase*)pEntity)->GetBotPlayer() );
#else
		return static_cast<CGEPlayer*>( ((CNPC_GEBase*)pEntity)->GetBotPlayer() );
#endif
	}
	else if ( pEntity->IsPlayer() )
	{
#ifdef _DEBUG
		return dynamic_cast<CGEPlayer*>( pEntity );
#else
		return static_cast<CGEPlayer*>( pEntity );
#endif
	}

	return NULL;
}
