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
#include "grenade_gebase.h"
#include "ent_hat.h"
#include "gemp_gamerules.h"
#include "ge_radarresource.h"

#include "ge_player.h"
#include "gemp_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


IMPLEMENT_SERVERCLASS_ST(CGEPlayer, DT_GE_Player)
	SendPropEHandle( SENDINFO( m_hActiveLeftWeapon ) ),
	SendPropInt( SENDINFO( m_ArmorValue ) ),
	SendPropInt( SENDINFO( m_iMaxArmor ) ),
	SendPropInt( SENDINFO( m_iMaxHealth ) ),

	SendPropFloat(SENDINFO(m_flFullZoomTime)),

	SendPropEHandle( SENDINFO( m_hHat ) ),
	SendPropInt( SENDINFO( m_takedamage ) ),
END_SEND_TABLE()

BEGIN_DATADESC( CGEPlayer )
END_DATADESC()

#define GE_COMMAND_MAX_RATE		0.5f
#define GE_HITOTHER_DELAY		0.15f

#define INVULN_PERIOD			0.5f

#ifdef _MSC_VER
#pragma warning( disable : 4355 )
#endif

ConVar ge_weapondroplimit("ge_weapondroplimit", "1", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Cap on the number of weapons a player can drop on death in addition to their active one.");
ConVar ge_limithalfarmorpickup("ge_limithalfarmorpickup", "0", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Prevent players from getting more than half armor from a half armor pickup."); //--TAKE OUT LATER--

ConVar ge_exp_pushscale("ge_exp_pushscale", "1", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Scale the amount of push force from explosions on players.");

CGEPlayer::CGEPlayer()
{
	m_bInSpawnInvul = false;

	m_pHints = new CHintSystem;
	m_pHints->Init( this, 2, NULL );

	m_flFullZoomTime = 0.0f;
	m_flDamageMultiplier = 1.0f;
	m_flSpeedMultiplier = 1.0f;
	m_flEndInvulTime = 0.0f;

	m_iCharIndex = m_iSkinIndex = -1;
	m_bRemovableHat = true;

	m_iFavoriteWeapon = WEAPON_NONE;
	m_iScoreBoardColor = 0;

	m_bKilledOtherThisFrame = false;
	m_iFrameDamageOutput = 0;

	memset(m_iPVS, 0, sizeof(m_iPVS));
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
	PrecacheScriptSound( "GEPlayer.HitOtherHead" );
	PrecacheScriptSound( "GEPlayer.HitOtherLimb" );
	PrecacheScriptSound( "GEPlayer.HitOtherInvuln" );
	PrecacheScriptSound( "GEPlayer.KillOther" );
#if 0
	PrecacheScriptSound( "GEPlayer.HitPainMale" );
	PrecacheScriptSound( "GEPlayer.HitPainFemale" );
#endif

	BaseClass::Precache();
}

bool CGEPlayer::AddArmor( int amount )
{
	// If player can't carry any more armor or the vest is a half vest and the player has more than half armor, return false.
	if (ArmorValue() < GetMaxArmor() && ( !ge_limithalfarmorpickup.GetBool() || !(amount < MAX_ARMOR && ArmorValue() >= MAX_ARMOR * 0.5)))
	{
		GEStats()->Event_PickedArmor(this, MIN(GetMaxArmor() - ArmorValue(), amount));

		// Use a different cap depending on which armor it is.
		if (ge_limithalfarmorpickup.GetBool() && amount < MAX_ARMOR)
			IncrementArmorValue(amount, GetMaxArmor()/2);
		else
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

		// Picking up armor ends spawn invulnerability.
		if ( GEMPRules()->GetSpawnInvulnCanBreak() )
			StopInvul();

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

	// Quick fix for the ammo deduction that ammo spawn weapons get causing the cheat to not give full ammo for them.
	CBasePlayer::GiveAmmo(AMMO_REMOTEMINE_MAX, AMMO_REMOTEMINE);
	CBasePlayer::GiveAmmo(AMMO_PROXIMITYMINE_MAX, AMMO_PROXIMITYMINE);
	CBasePlayer::GiveAmmo(AMMO_TIMEDMINE_MAX, AMMO_TIMEDMINE);
	CBasePlayer::GiveAmmo(AMMO_GRENADE_MAX, AMMO_GRENADE);
	CBasePlayer::GiveAmmo(AMMO_TKNIFE_MAX, AMMO_TKNIFE);
}

int CGEPlayer::GiveAmmo( int nCount, int nAmmoIndex, bool bSuppressSound )
{
	int amt = CBasePlayer::GiveAmmo(nCount, nAmmoIndex, bSuppressSound);

	if( amt > 0 )
	{
		GEStats()->Event_PickedAmmo( this, amt );
		
		// If we picked up mines or grenades and the player doesn't already have them give them a weapon!
		char *name = GetAmmoDef()->GetAmmoOfIndex(nAmmoIndex)->pName;

		// Tell plugins what we grabbed
		NotifyPickup( name, 1 );

		// If we surpressed the sound it wasn't a normal pickup so we don't deserve to get a bonus weapon from it.
		if (bSuppressSound)
			return amt;

		const char* weaponname = NULL;

		if ( Q_strstr(name,"mine") )
		{
			// Ok we picked up mines
			if ( !Q_strcmp(name,AMMO_PROXIMITYMINE) )
				weaponname = "weapon_proximitymine";
			else if ( !Q_strcmp(name,AMMO_TIMEDMINE) )
				weaponname = "weapon_timedmine";
			else if ( !Q_strcmp(name,AMMO_REMOTEMINE) )
				weaponname = "weapon_remotemine";
		}
		else if ( !Q_strcmp(name,AMMO_GRENADE) )
		{
			weaponname = "weapon_grenade";
		}
		else if ( !Q_strcmp(name,AMMO_TKNIFE) )
		{
			weaponname = "weapon_knife_throwing";
		}
		
		if (weaponname)
		{
			// If we actually got one of these weapons through an ammo box, we shouldn't also get the weapon's defualt clip.
			// default clip should only be given from player drops.
			CBaseEntity *pWeapon = GiveNamedItem(weaponname);
			if (pWeapon)
			{
				CGEWeapon *pGEWeapon = ToGEWeapon((CBaseCombatWeapon*)pWeapon);

				if (pGEWeapon)
					RemoveAmmo(pGEWeapon->GetDefaultClip1(), GetAmmoDef()->Index(name));
			}
		}
	}

	return amt;
}				


void CGEPlayer::PreThink( void )
{
	// See if the player is pressing the AIMMODE button
	CheckAimMode();

	// Invulnerability checks (make sure we are never invulnerable forever!)
	if ( m_bInSpawnInvul && m_flEndInvulTime < gpGlobals->curtime && !IsObserver() )
		StopInvul();

	if ( GetObserverMode() > OBS_MODE_FREEZECAM )
	{
		CheckObserverSettings();	// do this each frame
	}

	BaseClass::PreThink();
}

void CGEPlayer::PostThink( void )
{
	if (m_iDmgTakenThisFrame > 0)
	{
		DevMsg("Original damage force is %f\n", m_vDmgForceThisFrame.Length());
		DevMsg("Frame damage is %d\n", m_iDmgTakenThisFrame);
		// Scale damage push force based on damage, giving it a sort of parabolic curve.
		m_vDmgForceThisFrame *= m_iDmgTakenThisFrame / 160.0;
		// Push force from explosions does not get scaled this way, however.
		// Potential issue here in that simultaneous hits can result in weird push vectors, but it should be very uncommon.
		m_vDmgForceThisFrame += m_vExpDmgForceThisFrame * ge_exp_pushscale.GetFloat();;
		DevMsg("Scaled damage force is %f\n", m_vDmgForceThisFrame.Length());
	}

	BaseClass::PostThink();

	// Clamp force on alive players to 1000
	if ( m_iHealth > 0 && m_vDmgForceThisFrame.Length() > 1000.0f )
	{
		m_vDmgForceThisFrame.NormalizeInPlace();
		m_vDmgForceThisFrame *= 1000.0f;
	}

	// Check to see if we're below 300 velocity or if the force would slow us down
	// This lets weapons like shotguns apply a single large push but prevents automatic weapons from stun locking someone.
	// Some fancy vector projection stuff might be possible to cap out the speed you can go in one direction but still allow
	// Players to propell you perpindicular to that direction, but chances are if you're taking that much push force you're already dead.
	if (GetAbsVelocity().Length() < 300 || (GetAbsVelocity() + m_vDmgForceThisFrame).Length() < GetAbsVelocity().Length())
		ApplyAbsVelocityImpulse( m_vDmgForceThisFrame );


	// Play hitsound based on total damage dealt this frame, if we did any.
	if (m_iFrameDamageOutput > 0)
	{
		DevMsg("~~Frame damage output is %d in postframe~~ \n", m_iFrameDamageOutput);
		PlayHitsound(m_iFrameDamageOutput, m_iFrameDamageOutputType);
	}
	else if (m_iFrameDamageOutput == -1)
	{
		DevMsg("~~Frame damage output is 0 in postframe~~ \n");
		PlayHitsound(0, DMG_BULLET);
	}

	if (m_bKilledOtherThisFrame)
		PlayKillsound();

	// Reset our damage statistics
	m_iViewPunchScale = 0;
	m_iFrameDamageOutput = 0;
	m_iFrameDamageOutputType = 0;
	m_iDmgTakenThisFrame = 0;
	m_bKilledOtherThisFrame = false;
	m_vDmgForceThisFrame = vec3_origin;

	m_vExpDmgForceThisFrame = vec3_origin;
}

void CGEPlayer::CheatImpulseCommands( int iImpulse )
{
	switch ( iImpulse )
	{
		case 28:
		{
			if (sv_cheats->GetBool())
			{
				// Give ourselves a radar color based on our steam ID.
				// This is good for discussion during tests

				int steamID = GetSteamIDAsUInt64();
				int first3 = Floor2Int(steamID / 100000);
				int mid3 = Floor2Int((steamID % 1000000) / 1000);
				int last3 = steamID % 1000;

				// Shifted green and blue because HAHAHA I GET TO BE DESATURATED CYAN, LOSERS!
				Color col(first3 % 255, (mid3 + 230) % 255, (last3 + 170) % 255, 255);
				g_pRadarResource->AddRadarContact(this, RADAR_TYPE_PLAYER, false, "", col);
			}
		}
		break;

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
	// Copy over inputinfo so we can modify the damage when we pass it to the baseclass.
	CTakeDamageInfo info = inputinfo;

	// Get the attacker and convert them to GE player, if they cannot be converted do not run GE:S specific damage code.
	CGEPlayer *pGEAttacker = ToGEPlayer(inputinfo.GetAttacker());

	if (!pGEAttacker)
	{
		SetLastHitGroup(HITGROUP_GENERIC); // Set this here to avoid world kills counting as headshots.
		return BaseClass::OnTakeDamage(inputinfo);
	}

	if (!g_pGameRules->FPlayerCanTakeDamage(this, info.GetAttacker(), inputinfo))
		return 0;

	// Make sure that our attacker has us loaded and we have our attacker loaded.
	if (inputinfo.GetDamageType() & DMG_BULLET && !CheckInPVS(pGEAttacker) && !pGEAttacker->CheckInPVS(this))
		return 0;

	// Scale the damage before doing invuln related stuff.
	info.ScaleDamage(pGEAttacker->GetDamageMultiplier());

	// First apply explosion invulnerability, which is seperate from regular invuln in that it just tries to use the highest explosion damage taken in a given interval.
	// This will make explosion damage more consistent and running through the center of explosions way more consistently painful.
	if (info.GetDamageType() & DMG_BLAST)
	{
		if ( m_flEndExpDmgTime < gpGlobals->curtime)
		{
			m_flEndExpDmgTime = gpGlobals->curtime + INVULN_PERIOD;
			m_iExpDmgTakenThisInterval = 0;
		}

		// If the new damage is at least two bars higher than the old damage, hit them again.
		if ( m_iExpDmgTakenThisInterval == 0 || m_iExpDmgTakenThisInterval < info.GetDamage() - 40 )
		{
			info.SetDamage(info.GetDamage() - m_iExpDmgTakenThisInterval); // Adjust it so that they are only hit for the difference.
			m_iExpDmgTakenThisInterval = info.GetDamage();
			m_flEndExpDmgTime = gpGlobals->curtime + INVULN_PERIOD; // If we get hit again, we get more invuln time.

			if (inputinfo.GetDamage() == 398)
				SetLastHitGroup(HITGROUP_HEAD); // Used to mark direct hits
			else
				SetLastHitGroup(HITGROUP_GENERIC); // Set this here to avoid explosive headshot kills
		}
		else if (m_flEndExpDmgTime - INVULN_PERIOD == gpGlobals->curtime) // Add up any damage that was applied on the same frame as the invuln trigger.  
		{
			// Lower the damage from successive hits based on how much damage has already been done this frame.
			int weighteddamage = info.GetDamage() * info.GetDamage() / (m_iExpDmgTakenThisInterval + info.GetDamage());

			info.ScaleDamageForce(info.GetDamage() / (m_iExpDmgTakenThisInterval + info.GetDamage()));
			m_iExpDmgTakenThisInterval += weighteddamage;
			info.SetDamage(weighteddamage);
		}
		else // If this hit isn't strong enough or didn't happen on the same frame as a relevant one, don't go any further.
		{
			return 0;
		}
	}



	int weapid = WEAPON_NONE;

	// Get the weapon ID for invuln calcs and event notifications.
	CGEWeapon *pAttackerWep = NULL;

	if (inputinfo.GetWeapon())
		pAttackerWep = ToGEWeapon((CBaseCombatWeapon*)info.GetWeapon());

	if (pAttackerWep)
		weapid = pAttackerWep->GetWeaponID();
	else if (info.GetInflictor() && !info.GetInflictor()->IsNPC() && Q_stristr(info.GetInflictor()->GetClassname(), "npc_"))
		weapid = ToGEGrenade(info.GetInflictor())->GetWeaponID();
	else if (inputinfo.GetDamageType() & DMG_BLAST)
		weapid = WEAPON_EXPLOSION;

	int adjdmg = CalcInvul(info.GetDamage(), pGEAttacker, weapid);

	// Now set the damage in our custom damage info and give it to the base function to deal with.
	info.SetDamage(adjdmg);

	int dmg = BaseClass::OnTakeDamage(info);

	// Did we actually take damage?
	if (adjdmg > 0)
	{
		// Record our attacker, the time of the attack, and the damage taken.  Only player attacks make it this far.

		// If the last attacker is the current one, and they are still relevant, just add the damage onto the existing damage.
		// GetLastAttacker() returns null if the last attacker hasn't done enough damage recent enough.  If this check passes the timer is still positive.
		// we don't want to reset the time here if we're adding to the damage since adding to the damage is essentially adding more time to an already valid timer.
		if (GetLastAttacker() == inputinfo.GetAttacker())
			m_iLastAttackedDamage += adjdmg;
		else if ( inputinfo.GetAttacker() != this && gpGlobals->curtime + MIN(adjdmg * 0.1, 10) > m_flLastAttackedTime + MIN(m_iLastAttackedDamage * 0.1, 10)) // We have a different attacker, but let's make sure they will be doing enough damage to extend the timer and take the title.
		{
			m_iLastAttackedDamage = adjdmg; // We reset everything and don't add here because even though they extended the timer, they don't deserve extra time for damage they didn't do.
			m_flLastAttackedTime = gpGlobals->curtime;
			m_pLastAttacker = inputinfo.GetAttacker();
		}

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
		angles.y *= -0.015f;
		angles.z *= 0.012f;
		angles.x *= -0.0200f;

		// Apply viewpunch if we were hit hard enough
		DevMsg("%d framedamage", m_iDmgTakenThisFrame);

		if (m_iDmgTakenThisFrame > 120)
		{
			ViewPunch(angles*0.6f*(pow(0.5f, m_iViewPunchScale)));
			++m_iViewPunchScale;
		}

		// Print this out when we get damage (developer 1 must be on)
		DevMsg( "%s took %i damage to hitbox %i, %i HP / %i AP remaining.\n", GetPlayerName(), adjdmg, LastHitGroup(), GetHealth(), ArmorValue() );

		// Tell our attacker that they damaged me
		pGEAttacker->Event_DamagedOther(this, adjdmg, info);

		// Play our hurt sound
		HurtSound( inputinfo );

		// Tell everything that we got hurt
		IGameEvent *event = gameeventmanager->CreateEvent( "player_hurt" );
		if( event )
		{
			event->SetInt( "userid", GetUserID() );
			event->SetInt( "attacker", pGEAttacker->GetUserID() );
			event->SetInt( "weaponid", weapid );
			event->SetInt( "health", MAX(0, GetHealth()) );
			event->SetInt( "armor", ArmorValue() );
			event->SetInt( "damage", adjdmg );
			event->SetInt( "dmgtype", inputinfo.GetDamageType() );
			event->SetInt( "hitgroup", LastHitGroup() );
			event->SetBool( "penetrated", FBitSet(inputinfo.GetDamageStats(), FIRE_BULLETS_PENETRATED_SHOT)?true:false );
			gameeventmanager->FireEvent( event );
		}
	}
	else
	{
		// Tell our attacker that they didn't damage me but still hit me
		pGEAttacker->Event_DamagedOther(this, 0, info);

		// Upset accuracy based on how many half gauges of life would have been lost. 
		// More substancial than you'd think because of the exponential nature of the accuracy recovery
		CBaseEntity *pHeldWep;
		pHeldWep = GetActiveWeapon();

		if (pHeldWep)
		{
			CGEWeapon *pOurWeapon = ToGEWeapon((CBaseCombatWeapon*)pHeldWep);

			if (pOurWeapon)
			{
				pOurWeapon->AddAccPenalty(dmg / 80);
			}
		}
		DevMsg("%s's invulnerability blocked that hit!\n", GetPlayerName());
	}

	return dmg;
}

bool CGEPlayer::CheckInPVS(CBaseEntity *pEnt)
{
	int clusterIndex = engine->GetClusterForOrigin(EyePosition());
	engine->GetPVSForCluster(clusterIndex, sizeof(m_iPVS), m_iPVS);

	if (pEnt->NetworkProp()->IsInPVS(edict(), m_iPVS, sizeof(m_iPVS)))
		return true;

	return false;
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

	DevMsg("%f basedamage\n", inputInfo.GetBaseDamage());

	// Limit our up/down force if this wasn't an explosion
	if ( inputInfo.GetDamageType() &~ DMG_BLAST )
		force.z = clamp( force.z, -250.0f, 250.0f );
	else if ( m_iHealth > 0 )
		force.z = clamp( force.z, -450.0f, 450.0f );

	// Seperate explosion damage force from normal damage force because explosion damage force gets scaled differently.
	if (inputInfo.GetDamageType() & DMG_BLAST)
		m_vExpDmgForceThisFrame += force;
	else
		m_vDmgForceThisFrame += force;

	return BaseClass::OnTakeDamage_Alive( inputInfo );
}

// This gets called first, then ontakedamage, which can trigger ontakedamage_alive when the baseclass is called.
void CGEPlayer::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	CTakeDamageInfo info = inputInfo;
	bool doTrace = false;

	// Try to get the weapon and see if it wants hit location damage
	CGEWeapon *pWeapon = NULL;
	if ( info.GetWeapon() )
	{
		pWeapon = ToGEWeapon( (CBaseCombatWeapon*)info.GetWeapon() );

		// Only do this calculation if we allow it for the weapon
		if ( pWeapon->GetGEWpnData().iFlags & ITEM_FLAG_DOHITLOCATIONDMG )
			doTrace = true;
	}

	// If we want hitlocation dmg and we have a weapon to refer to do it!
	if ( doTrace && pWeapon )
	{
		// See if we would have hit a more sensitive area if this one hadn't blocked the shot.
		// Used to prevent nonsense like attacking from the side and certain animations causing
		// nearly all shots that would have been chest hits to be limb hits

		trace_t tr2;
		Vector tracedist = ptr->endpos - ptr->startpos;
		Vector tracedir = tracedist / tracedist.Length();
		Vector lastendpos = ptr->endpos;
		int newhitgroup = ptr->hitgroup;

		if (ptr->endpos != ptr->startpos)
		{
			// Do a series of traces through the player to see if we can hit anything better than the starting hitgroup.
			for (int i = 0; i < 8; i++)
			{
				// Headshots are the best hits so abort the loop if we got one.
				if (newhitgroup == HITGROUP_HEAD)
					break;

				UTIL_TraceLine(lastendpos, lastendpos + (tracedir * 2), MASK_ALL, NULL, COLLISION_GROUP_NONE, &tr2);

				// Adjust the starting point of the next trace so it keeps going through the player.
				lastendpos = lastendpos + (tracedir * 3);

				// If we didn't hit anything or we what we did hit wasn't the player, don't worry about finding a hitgroup.
				if (tr2.fraction >= 1.0f || tr2.m_pEnt != ptr->m_pEnt)
					continue;

				// Lower hitgroup IDs are almost always better.  0 is the default hitgroup though so ignore that one.
				if (tr2.hitgroup > 0 && tr2.hitgroup < newhitgroup)
					newhitgroup = tr2.hitgroup;
			}
		}
		// Set the last part where we got hit
		SetLastHitGroup( newhitgroup );

		switch ( newhitgroup )
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

	BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
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
	// If we damaged ourselves we didn't really damage someone else.
	if (pOther == this)
		return;

	if (dmgTaken <= 0) // We hit someone but didn't actually do any damage.
	{
		if (m_iFrameDamageOutput <= 0)  // We haven't hit anyone yet, add a flag that says we hit someone in invuln.
			m_iFrameDamageOutput = -1;
	}
	else
	{
		m_iFrameDamageOutput = MAX(m_iFrameDamageOutput, 0);  //Get rid of any invuln hit flags.
		m_iFrameDamageOutput += dmgTaken;
	}

	m_iFrameDamageOutputType = inputInfo.GetDamageType();


	if ( pOther->GetHealth() <= 0 )
		m_bKilledOtherThisFrame = true;

	// TraceBleed occurs in TraceAttack and is defined in baseentity_shared.cpp
	// SpawnBlood occurs in TraceAttack and is defined as UTIL_BloodDrips in util_shared.cpp
	// GE:S spawns blood on the client, the server blood is for all other players
}

ConVar ge_explosiveheadshots("ge_explosiveheadshots", "0", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Headshot kills create explosions!");

void CGEPlayer::Event_Killed( const CTakeDamageInfo &info )
{
	ShowBloodScreen();

	// Let the weapon know that the player has died, this is used for stuff like grenades.
	if (GetActiveWeapon() && ToGEWeapon(GetActiveWeapon()))
		ToGEWeapon(GetActiveWeapon())->PreOwnerDeath();

	if ( ge_explosiveheadshots.GetBool() && info.GetAttacker() )
	{
		if (LastHitGroup() == HITGROUP_HEAD)
			ExplosionCreate(EyePosition(), GetAbsAngles(), info.GetAttacker(), 500, 100,
			SF_ENVEXPLOSION_NOSMOKE | SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS, 0.0f, NULL);
	}

	KnockOffHat(!m_bRemovableHat); // If our hat isn't removable we have to delete it on death.

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

static int weaponstrengthSort(CGEWeapon * const *a, CGEWeapon * const *b)
{
	int IDa = (*a)->GetWeaponID();
	int IDb = (*b)->GetWeaponID();

	if (GEWeaponInfo[IDa].strength > GEWeaponInfo[IDb].strength)
		return -1;
	else if (GEWeaponInfo[IDa].strength == GEWeaponInfo[IDb].strength)
		return 0;
	else return 1;
}

void CGEPlayer::DropTopWeapons()
{
	CUtlVector<CGEWeapon*> topWeapons;

	// Scan the player's held weapons and build a list of those eligible to be dropped.
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		CBaseCombatWeapon *pWeapon = GetWeapon(i);
		if (!pWeapon)
			continue;

		// We drop the active weapon by default so don't worry about it.
		if (GetActiveWeapon() == pWeapon)
			continue;

		CGEWeapon *pGEWeapon = ToGEWeapon(pWeapon);
		if (!pGEWeapon)
			continue;

		// Weapons with strength lower than 1 do not have pickups and thus should not be dropped.
		if (GEWeaponInfo[pGEWeapon->GetWeaponID()].strength < 1)
			continue;

		topWeapons.AddToTail(pGEWeapon);
	}

	// Sort that list based on weaponstrengths, so the first ones dropped are the strongest.
	topWeapons.Sort(weaponstrengthSort);

	// Determine how many weapons to drop
	int droplength;

	if (topWeapons.Count() > ge_weapondroplimit.GetInt())
		droplength = ge_weapondroplimit.GetInt();
	else
		droplength = topWeapons.Count();

	// Drop the strongest weapons the player has with variable force and direction
	for (int i = 0; i < droplength; i++)
	{
		Vector forcevector = Vector((rand() % 150) - 75, (rand() % 150) - 75, rand() % 200) + GetAbsVelocity()/2;

		Weapon_Drop(topWeapons[i], NULL, &forcevector);
	}
}

CON_COMMAND(ge_knockoffhat, "Usage: ge_knockoffhat playername deletehat")
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
		return;

	CBasePlayer *pTarget = NULL;
	bool removehat = false;

	if (args.ArgC() < 2)
		pTarget = UTIL_GetCommandClient();
	else
		pTarget = UTIL_PlayerByName(args[1]);

	if (args.ArgC() > 2)
		removehat = atoi(args[2]) > 0;

	CGEPlayer *pGETarget = ToGEPlayer(pTarget);

	if (pGETarget)
		pGETarget->KnockOffHat( removehat );
}

ConVar ge_sillyhats("ge_sillyhats", "0", FCVAR_GAMEDLL, "Hats get extra silly!");

void CGEPlayer::KnockOffHat( bool bRemove /* = false */, const CTakeDamageInfo *dmg /* = NULL */ )
{
	CBaseEntity *pHat = m_hHat;

	if ( bRemove )
	{
		UTIL_Remove(pHat);
	}
	else if ( !m_bRemovableHat )
		return; // If it's not possible to knock off don't knock it off.
	else if ( pHat )
	{
		Vector origin = EyePosition();
		origin.z += 8.0f;

		pHat->Activate();
		pHat->SetAbsOrigin(origin);
		pHat->RemoveEffects(EF_NODRAW);

		// Punt the hat appropriately
		if (dmg)
			pHat->ApplyAbsVelocityImpulse(dmg->GetDamageForce() * 0.75f);

		if ( ge_sillyhats.GetBool() )
		{
			if ( dmg && dmg->GetAttacker() )
				ExplosionCreate(origin, GetAbsAngles(), dmg->GetAttacker(), 500, 100,
				SF_ENVEXPLOSION_NOSMOKE | SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS, 0.0f, NULL);
			else
				ExplosionCreate(origin, GetAbsAngles(), this, 500, 100,
				SF_ENVEXPLOSION_NOSMOKE | SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS, 0.0f, NULL);

			UTIL_Remove(pHat);
		}
	}

	SetHitboxSetByName( "default" );
	m_hHat = NULL;
}

void CGEPlayer::GiveHat()
{
	if (IsObserver())
	{
		// Make sure we DO NOT have a hat as an observer
		KnockOffHat(true);
		return;
	}

	if ( m_hHat.Get() )
		return;

	const char* hatModel = GECharacters()->Get(m_iCharIndex)->m_pSkins[m_iSkinIndex]->szHatModel;
	if ( !hatModel || hatModel[0] == '\0' )
		return;

	SpawnHat( hatModel );
}

CON_COMMAND(ge_giveplayerhat, "Usage: ge_giveplayerhat hatmodel playername canshootoff")
{
	if (!UTIL_IsCommandIssuedByServerAdmin())
		return;

	if (args.ArgC() < 2)
		return;

	CBasePlayer *pTarget = NULL;
	bool removable = true;

	if (args.ArgC() == 2)
		pTarget = UTIL_GetCommandClient();
	else
		pTarget = UTIL_PlayerByName( args[2] );

	if (args.ArgC() > 3)
		removable = atoi(args[3]) > 0;

	CGEPlayer *pGETarget = ToGEPlayer(pTarget);

	if ( pGETarget )
		pGETarget->SpawnHat( args[1], removable );
}

void CGEPlayer::SpawnHat( const char* hatModel, bool canBeRemoved )
{
	if (IsObserver() && !IsAlive()) // Observers can't have any hats.  Need this here so direct hat assignment doesn't make floating hats.
		return;

	// Get rid of any hats we're currently wearing.
	if (m_hHat.Get())
		KnockOffHat(true);

	// Simple check to ensure consistency
	int setnum, boxnum;
	if (LookupHitbox("hat", setnum, boxnum))
		SetHitboxSet(setnum);

	CBaseEntity *hat = CreateNewHat(EyePosition(), GetAbsAngles(), hatModel);
	hat->FollowEntity(this, true);
	m_hHat = hat;

	m_bRemovableHat = canBeRemoved;
}

void CGEPlayer::InitialSpawn( void )
{
	BaseClass::InitialSpawn();

	// Early out if we are a bot
	if ( IsBot() )
		return;

	CBaseViewModel *vm = (CBaseViewModel*)CreateEntityByName( "viewmodel" );
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

	// Nearly disable npc interp if we are localhost
	// if ( !engine->IsDedicatedServer() && entindex() == 1 )
	//	engine->ClientCommand( edict(), "cl_interp_npcs 0.01" );
}

void CGEPlayer::Spawn()
{
	BaseClass::Spawn();

	HideBloodScreen();

	m_iDmgTakenThisFrame = 0;
	m_iFrameDamageOutput = 0;
	m_flEndExpDmgTime = 0;
	m_iExpDmgTakenThisInterval = 0;
	m_vExpDmgForceThisFrame = vec3_origin;

	for (int i = 0; i < 16; i++)
	{
		m_iAttackListTimes[i] = 0.0;
		m_iAttackList[i] = 0;
	}

	SetMaxSpeed(GE_NORM_SPEED * GEMPRules()->GetSpeedMultiplier(this));
	ResetAimMode();
}

void CGEPlayer::SetSpeedMultiplier(float mult)	
{
	m_flSpeedMultiplier = clamp(mult, 0.5f, 1.5f); 
	SetMaxSpeed(GE_NORM_SPEED * GEMPRules()->GetSpeedMultiplier(this)); 
}

void CGEPlayer::ShowBloodScreen( void )
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
}

void CGEPlayer::HideBloodScreen( void )
{
	if ( !IsBotPlayer() )
	{
	    CBaseViewModel *pViewModel = GetViewModel( 1 );
	    if ( pViewModel )
	    {
		    pViewModel->SendViewModelMatchingSequence( 0 );
		    pViewModel->AddEffects( EF_NODRAW );
	    }
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

// This is pretty much just used for spawn invuln now.
void CGEPlayer::StartInvul( float time )
{
	DevMsg( "%s was given spawn invuln for %0.2f seconds...\n", GetPlayerName(), time );
	m_bInSpawnInvul = true;
	m_flEndInvulTime = gpGlobals->curtime + time;
}

// This is pretty much just used for stopping spawn invuln now
void CGEPlayer::StopInvul(void)
{
	DevMsg("%s lost invulnerability...\n", GetPlayerName());
	m_takedamage = DAMAGE_YES;
	m_bInSpawnInvul = false;
}

// Adjusts incoming damage as needed to conform with invuln.  Was coded with arrays, as I was still learning the ropes when I made it
// If you ever want to do something more fancy with it I reccomend recoding it to use util vectors, which are more versitle.

int CGEPlayer::CalcInvul(int damage, CGEPlayer *pAttacker, int wepID)
{
	DevMsg("%s took %d damage...", GetPlayerName(), damage);

	// Spawn invuln negates incoming damage regardless of previous damage taken
	if (m_bInSpawnInvul || m_takedamage == DAMAGE_NO)
		return 0;

	float lowtime = gpGlobals->curtime - INVULN_PERIOD;
	int emptyid = -1;

	// Scans for empty and expired damage slots.
	for (int i = 0; i < 16; i++)
	{
		if (m_iAttackListTimes[i] < lowtime)
		{
			m_iAttackListTimes[i] = 0.0;
			m_iAttackList[i] = 0;
			emptyid = i;
		}
	}


	// If all damage slots are filled somehow, the player gets to cheat the system.  They earned it.
	// Should be impossible with everything but the autoshotgun, with 16 leg hits coming in at 256 damage which is lower
	// than the cap of 320.  Would require 4 players attacking at the same time to get this to happen due to shotgun firing interval.

	if (emptyid == -1)
		return 0;

	// If there's no attacker, then invuln does not get applied.
	if (!pAttacker)
		return damage;

	// Default damage cap is set to golden gun damage.  It's much higher than the max health, which is 320.
	int damagecap = 5000;

	// But if there's a GE weapon involved then the damage cap gets overwritten with that weapon's cap.
	if (wepID != WEAPON_NONE)
		damagecap = round(pAttacker->GetDamageMultiplier() * WeaponMaxDamageFromID(wepID));

	// Calculate the damage taken in the last invuln period.
	int totaldamage = 0;

	for (int i = 0; i < 16; i++)
	{
		totaldamage += m_iAttackList[i];
	}

	// If we've hit the cap already, return 0.
	if (totaldamage >= damagecap)
		return 0;
	
	//Otherwise it's time to add the entry to the list and announce the target has been hit.

	damage = MIN(damage, damagecap - totaldamage);

	m_iAttackListTimes[emptyid] = gpGlobals->curtime;
	m_iAttackList[emptyid] = damage;

	DevMsg("...weapon damage cap is %d...", damagecap);
	DevMsg("...total period damage is %d...", totaldamage);
	DevMsg("...so the original damage became %d damage after invuln calcs!\n", damage);

	return damage;
}

void CGEPlayer::PlayHitsound(int dmgTaken, int dmgtype)
{
	// All the reasons we might not want to play hitsounds.
	int noSound = atoi(engine->GetClientConVarValue(entindex(), "cl_ge_nohitsound"));
	if (noSound != 0 || IsBotPlayer())
		return;
	

	float playAt = gpGlobals->curtime - (g_pPlayerResource->GetPing(entindex()) / 1000) + GE_HITOTHER_DELAY;
	CSingleUserRecipientFilter filter(this);
	filter.MakeReliable();

	float damageratio;
	CGEWeapon *pGEWeapon = ToGEWeapon(GetActiveWeapon());
	
	int maxwepdamage = 160;

	if (pGEWeapon)
	{
		maxwepdamage = pGEWeapon->GetWeaponDamage();

		if (pGEWeapon->IsShotgun()) // If the weapon is a shotgun it can do 5 times the damage of a single pellet because it shoots 5 pellets per shot
			maxwepdamage *= 4; // But we should also be forgiving since doing max shotgun damage is hard.  4/5 * 4/5 = 16/25 or roughly 64% damage triggers headshot sound.
	}

	// We adjust our hitsound conditions depending on the weapon so they still make sense.  
	// Bullet is all hitscan, slash and club are knife and melee.
	// Minor glitch here where if the player pulls out a gun right before hitting someone with a throwing knife they'll get the wrong sound.
	if (dmgtype & DMG_BULLET || dmgtype & DMG_SLASH || dmgtype & DMG_CLUB)
		damageratio = dmgTaken / (float)maxwepdamage;
	else if (dmgtype & DMG_BLAST) // Explosives and explosions.  "Headshot" is getting more than a gauge of damage. 160/0.8
		damageratio = dmgTaken / 200.0f;
	else // Some other weapon, probably a map entity.  Use default sound.
		damageratio = 0.5;

	if (damageratio == 0)
		EmitSound(filter, entindex(), "GEPlayer.HitOtherInvuln", 0, playAt);
	else if (damageratio < 0.3)
		EmitSound(filter, entindex(), "GEPlayer.HitOtherLimb", 0, playAt);
	else if (damageratio < 0.8)
		EmitSound(filter, entindex(), "GEPlayer.HitOther", 0, playAt);
	else
		EmitSound(filter, entindex(), "GEPlayer.HitOtherHead", 0, playAt);
}

void CGEPlayer::PlayKillsound()
{
	// All the reasons we might not want to play killsounds
	int noSound = atoi(engine->GetClientConVarValue(entindex(), "cl_ge_nokillsound"));
	if (noSound != 0 || IsBotPlayer())
		return;


	float playAt = gpGlobals->curtime - (g_pPlayerResource->GetPing(entindex()) / 1000) + GE_HITOTHER_DELAY;
	CSingleUserRecipientFilter filter(this);
	filter.MakeReliable();
	EmitSound(filter, entindex(), "GEPlayer.KillOther", 0, playAt);
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

CBaseEntity* CGEPlayer::GetLastAttacker(bool onlyrecent)
{
	// If we only want an actually relevant last attacker, compare the last time we were attacked to the amount of damage that attack did.
	if (onlyrecent && m_flLastAttackedTime + MIN(m_iLastAttackedDamage * 0.1, 10) < gpGlobals->curtime)// 20 points of damages give 1 second of kill tracking.
		return NULL;
	else
		return m_pLastAttacker;
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
