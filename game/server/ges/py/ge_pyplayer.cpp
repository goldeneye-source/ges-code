///////////// Copyright © 2009 LodleNet. All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_pyplayer.cpp
//   Description :
//      [TODO: Write the purpose of ge_pyplayer.cpp.]
//
//   Created On: 8/24/2009 3:39:38 PM
//   Created By: Mark Chandler <mailto:mark@moddb.com>
////////////////////////////////////////////////////////////////////////////
#include "ge_pyprecom.h"

#include "ammodef.h"

#include "ge_player.h"
#include "gemp_player.h"
#include "gebot_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CBaseEntity *pyGetEntByUniqueId( int );

bool pyIsValidPlayerIndex(int index)
{
	return UTIL_PlayerByIndex(index) != NULL;
}

CGEMPPlayer* pyGetMPPlayer(int index)
{
	return ToGEMPPlayer( UTIL_PlayerByIndex(index) );
}

CGEMPPlayer* pyToMPPlayer( bp::object ent_or_uid )
{
	bp::extract<CBaseEntity*> to_ent( ent_or_uid );
	bp::extract<int> to_uid( ent_or_uid );

	if ( to_ent.check() )
		return ToGEMPPlayer( to_ent() );
	else if ( to_uid )
		return ToGEMPPlayer( pyGetEntByUniqueId( to_uid() ) );
	else
		return NULL;
}

CBaseCombatCharacter *pyToCombatCharacter( CBaseEntity *pEnt )
{
	return pEnt ? pEnt->MyCombatCharacterPointer() : NULL;
}

CGEWeapon *pyGetActiveWeapon( CBaseCombatCharacter *pEnt )
{
	return pEnt ? (CGEWeapon*) pEnt->GetActiveWeapon() : NULL;
}

bool pyHasWeapon( CBaseCombatCharacter *pEnt, bp::object weap_or_id )
{
	bp::extract<CGEWeapon*> to_weap( weap_or_id );
	bp::extract<char*> to_name( weap_or_id );
	bp::extract<int> to_id( weap_or_id );

	if ( to_weap.check() )
		return pEnt->Weapon_OwnsThisType( to_weap()->GetClassname() ) != NULL;
	else if ( to_name.check() )
		return pEnt->Weapon_OwnsThisType( to_name() ) != NULL;
	else if ( to_id.check() )
		return pEnt->Weapon_OwnsThisType( WeaponIDToAlias( to_id() ) ) != NULL;

	return false;
}

void pyMakeInvisible( CBaseCombatCharacter *pEnt )
{
	pEnt->SetRenderMode(kRenderNone);
}

void pyMakeVisible( CBaseCombatCharacter *pEnt )
{
	pEnt->SetRenderMode(kRenderNormal);
}

bool pyWeaponSwitch( CBaseCombatCharacter *pEnt, bp::object weap_or_id )
{
	bp::extract<CGEWeapon*> to_weap( weap_or_id );
	bp::extract<char*> to_name( weap_or_id );
	bp::extract<int> to_id( weap_or_id );

	CBaseCombatWeapon *pWeap = NULL;

	if ( to_weap.check() )
		pWeap = to_weap();
	else if ( to_name.check() )
		pWeap = pEnt->Weapon_OwnsThisType( to_name() );
	else if ( to_id.check() )
		pWeap = pEnt->Weapon_OwnsThisType( WeaponIDToAlias( to_id() ) );
	
	if ( pEnt && pWeap )
		return pEnt->Weapon_Switch( pWeap );

	return false;
}

int pyGetAmmoCount( CBaseCombatCharacter *pEnt, bp::object weap_or_ammo )
{
	bp::extract<CGEWeapon*> to_weap( weap_or_ammo );
	bp::extract<int> to_weap_id( weap_or_ammo );
	bp::extract<char*> to_ammo( weap_or_ammo );

	if ( to_weap.check() )
		return pEnt->GetAmmoCount( to_weap()->GetPrimaryAmmoType() );
	else if ( to_weap_id.check() )
		return pEnt->GetAmmoCount( const_cast<char*>(GetAmmoForWeapon( to_weap_id() )) );
	else if ( to_ammo.check() )
		return pEnt->GetAmmoCount( to_ammo() );
	
	return 0;
}

bp::list pyGetHeldWeapons( CBaseCombatCharacter *pEnt )
{
	bp::list weaps;
	if ( pEnt )
	{
		for ( int i=0; i < MAX_WEAPONS; i++ )
		{
			CGEWeapon *pWeap = (CGEWeapon*) pEnt->GetWeapon( i );
			if ( pWeap )
				weaps.append( bp::ptr( pWeap ) );
		}
	}
	return weaps;
}

bp::list pyGetHeldWeaponIds( CBaseCombatCharacter *pEnt )
{
	bp::list weaps;
	if ( pEnt )
	{
		for ( int i=0; i < MAX_WEAPONS; i++ )
		{
			CGEWeapon *pWeap = (CGEWeapon*) pEnt->GetWeapon( i );
			if ( pWeap )
				weaps.append( pWeap->GetWeaponID() );
		}
	}
	return weaps;
}

int pyGetSkin( CBaseCombatCharacter *pEnt )
{
	if ( !pEnt ) return 0;
	return pEnt->m_nSkin;
}

void pyPlayerSetAbsOrigin( CBaseCombatCharacter *player, Vector origin )
{
	// DO NOTHING!!
}

void pyPlayerSetAbsAngles( CBaseCombatCharacter *player, QAngle angles )
{
	// DO NOTHING!!
}

void pySetPlayerModel( CGEPlayer *pPlayer, const char *szChar, int nSkin )
{
	if ( pPlayer )
		pPlayer->SetPlayerModel( szChar, nSkin, true );
}

void pyGiveAmmo( CGEMPPlayer *pPlayer, const char *ammo, int amt, bool supressSound = true )
{
	if ( pPlayer )
	{
		int aID = GetAmmoDef()->Index( ammo );
		pPlayer->GiveAmmo( amt, aID, supressSound );
	}	
}

Vector pyAimDirection( CGEPlayer *pEnt )
{
	Vector forward;
	pEnt->EyeVectors( &forward );
	return forward;
}

BOOST_PYTHON_FUNCTION_OVERLOADS(GiveAmmo_overloads, pyGiveAmmo, 3, 4);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(GiveNamedWeapon_overloads, CGEPlayer::GiveNamedWeapon, 2, 3);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(BotGiveNamedWeapon_overloads, CGEBotPlayer::GiveNamedWeapon, 2, 3);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(ChangeTeam_overloads, CGEMPPlayer::ChangeTeam, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(BotChangeTeam_overloads, CGEBotPlayer::ChangeTeam, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(KnockOffHat_overloads, CGEPlayer::KnockOffHat, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(BotKnockOffHat_overloads, CGEBotPlayer::KnockOffHat, 0, 1);

BOOST_PYTHON_MODULE(GEPlayer)
{
	using namespace boost::python;

	// Imports
	import( "GEEntity" );

	//because CommitSuicideFP is overloaded we must tell it which one to use
	void (CGEPlayer::*CommitSuicideFP)(bool, bool) = &CGEPlayer::CommitSuicide;

	def("IsValidPlayerIndex", pyIsValidPlayerIndex);
	def("ToCombatCharacter", pyToCombatCharacter, return_value_policy<reference_existing_object>());
	def("ToMPPlayer", pyToMPPlayer, return_value_policy<reference_existing_object>());
	def("GetMPPlayer", pyGetMPPlayer, return_value_policy<reference_existing_object>());

	// Functions used by NPC's
	class_<CBaseCombatCharacter, bases<CBaseEntity>, boost::noncopyable>("CBaseCombatCharacter", no_init)
		.def("GetHealth", &CBaseCombatCharacter::GetHealth)
		.def("SetHealth", &CBaseCombatCharacter::SetHealth)
		.def("GetSkin", pyGetSkin)
		.def("GetActiveWeapon", pyGetActiveWeapon, return_value_policy<reference_existing_object>())
		.def("HasWeapon", pyHasWeapon)
		.def("WeaponSwitch", pyWeaponSwitch)
		.def("GetAmmoCount", pyGetAmmoCount)
		.def("GetHeldWeapons", pyGetHeldWeapons)
		.def("GetHeldWeaponIds", pyGetHeldWeaponIds)
		// Explicitly override these functions to prevent their use
		.def("SetAbsOrigin", pyPlayerSetAbsOrigin)
		.def("SetAbsAngles", pyPlayerSetAbsAngles);

	// Here for function call reasons only
	class_<CNPC_GEBase, bases<CBaseCombatCharacter>, boost::noncopyable>("CNPC_GEBase", no_init);
	class_<CBasePlayer, bases<CBaseCombatCharacter>, boost::noncopyable>("CBasePlayer", no_init);

	class_<CGEPlayer, bases<CBasePlayer>, boost::noncopyable>("CGEPlayer", no_init)
		.def("IncrementDeathCount", &CGEPlayer::IncrementDeathCount) // Deprecated
		.def("GetUserID", &CGEPlayer::GetUserID)	// Deprecated
		.def("GetFrags", &CGEPlayer::FragCount)
		.def("GetDeaths", &CGEPlayer::DeathCount)
		.def("AddDeathCount", &CGEPlayer::IncrementDeathCount)
		.def("GetArmor", &CGEPlayer::ArmorValue)
		.def("GetHealth", &CGEPlayer::GetHealth)
		.def("ResetDeathCount", &CGEPlayer::ResetDeathCount)
		.def("CommitSuicide", CommitSuicideFP)
		.def("GetPlayerName", &CGEMPPlayer::GetSafePlayerName)
		.def("GetPlayerID", &CGEPlayer::GetUserID)
		.def("GetSteamID", &CGEPlayer::GetNetworkIDString)
		.def("IsDead", &CGEPlayer::IsDead)
		.def("IsObserver", &CGEPlayer::IsObserver)
		.def("GetMaxArmor", &CGEPlayer::GetMaxArmor)
		.def("SetMaxArmor", &CGEPlayer::SetMaxArmor)
		.def("GetMaxHealth", &CGEPlayer::GetMaxHealth)
		.def("SetMaxHealth", &CGEPlayer::SetMaxHealth)
		.def("SetArmor", &CGEPlayer::SetArmorValue)
		.def("GetPlayerModel", &CGEPlayer::GetCharIdent)
		.def("SetPlayerModel", pySetPlayerModel)
		.def("SetHat", &CGEPlayer::SpawnHat)
		.def("KnockOffHat", &CGEPlayer::KnockOffHat, KnockOffHat_overloads())
		.def("MakeInvisible", pyMakeInvisible)
		.def("MakeVisible", pyMakeVisible)
		.def("SetDamageMultiplier", &CGEPlayer::SetDamageMultiplier)
		.def("SetSpeedMultiplier", &CGEPlayer::SetSpeedMultiplier)
		.def("SetScoreBoardColor", &CGEPlayer::SetScoreBoardColor)
		.def("StripAllWeapons", &CGEPlayer::StripAllWeapons)
		.def("GetAimDirection", pyAimDirection)
		.def("GetEyePosition", &CGEPlayer::EyePosition)
		.def("GiveNamedWeapon", &CGEPlayer::GiveNamedWeapon, GiveNamedWeapon_overloads());

	class_<CGEMPPlayer, bases<CGEPlayer>, boost::noncopyable>("CGEMPPlayer", no_init)
		.def("GetScore", &CGEMPPlayer::GetRoundScore)		// Deprecated
		.def("SetScore", &CGEMPPlayer::SetRoundScore)		// Deprecated
		.def("IncrementScore", &CGEMPPlayer::AddRoundScore) // Deprecated
		.def("ResetScore", &CGEMPPlayer::ResetRoundScore)	// Deprecated
		.def("IncrementMatchScore", &CGEMPPlayer::AddMatchScore) // Deprecated
		.def("GetRoundScore", &CGEMPPlayer::GetRoundScore)
		.def("SetRoundScore", &CGEMPPlayer::SetRoundScore)
		.def("AddRoundScore", &CGEMPPlayer::AddRoundScore)
		.def("ResetRoundScore", &CGEMPPlayer::ResetRoundScore)
		.def("GetMatchScore", &CGEMPPlayer::GetMatchScore)
		.def("SetMatchScore", &CGEMPPlayer::SetMatchScore)
		.def("AddMatchScore", &CGEMPPlayer::AddMatchScore)
		.def("SetDeaths", &CGEMPPlayer::SetDeaths)
		.def("ForceRespawn", &CGEMPPlayer::ForceRespawn)
		.def("ChangeTeam", &CGEMPPlayer::ChangeTeam, ChangeTeam_overloads())
		.def("GetCleanPlayerName", &CGEMPPlayer::GetSafeCleanPlayerName)
		.def("SetInitialSpawn", &CGEMPPlayer::SetInitialSpawn)
		.def("IsInitialSpawn", &CGEMPPlayer::IsInitialSpawn)
		.def("IsInRound", &CGEMPPlayer::IsInRound)
		.def("IsActive", &CGEMPPlayer::IsActive)
		.def("IsPreSpawn", &CGEMPPlayer::IsPreSpawn)
		.def("GiveDefaultWeapons", &CGEMPPlayer::GiveDefaultItems)
		.def("GiveAmmo", pyGiveAmmo, GiveAmmo_overloads());

	class_<CGEBotPlayer, bases<CGEMPPlayer>, boost::noncopyable>("CGEBotPlayer", no_init)
		.def("GiveNamedWeapon", &CGEBotPlayer::GiveNamedWeapon, BotGiveNamedWeapon_overloads())
		.def("ChangeTeam", &CGEBotPlayer::ChangeTeam, BotChangeTeam_overloads())
		.def("StripAllWeapons", &CGEBotPlayer::StripAllWeapons)
		.def("KnockOffHat", &CGEBotPlayer::KnockOffHat, BotKnockOffHat_overloads());
}
