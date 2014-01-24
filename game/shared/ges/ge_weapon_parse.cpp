///////////// Copyright © 2006, Scott Loyd. All rights reserved. /////////////
// 
// File: ge_weapon_parse.h
// Description:
//      Holds values for specific weapons script arguments.
//
// Created On: 3/7/2006 2:39:28 PM
// Created By: Scott Loyd <mailto:scottloyd@gmail.com> 
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include <KeyValues.h>
#include "ge_weapon_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

FileWeaponInfo_t* CreateWeaponInfo()
{
	return new CGEWeaponInfo;
}


CGEWeaponInfo::CGEWeaponInfo()
{
}

void CGEWeaponInfo::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{
	BaseClass::Parse( pKeyValuesData, szWeaponName );

	m_iDamage		= pKeyValuesData->GetInt( "Damage", 42 );
	m_iAmmoIcon		= pKeyValuesData->GetInt( "ammo_icon", -1 );

	m_flFireDelay	= pKeyValuesData->GetFloat( "fire_delay", 0.0f );
	m_flDamageRadius	= pKeyValuesData->GetFloat( "DamageRadius", 0.0f );
	m_flRateOfFire	= pKeyValuesData->GetFloat( "rof", 1.0f );
	m_flClickRateOfFire = pKeyValuesData->GetFloat( "c_rof", 0.21f );

	m_flRecoil = pKeyValuesData->GetFloat( "recoil", 0.0f );

	m_iZoomOffset = pKeyValuesData->GetInt( "zoom_offset", 0 );

	m_flMaxPenetration = pKeyValuesData->GetFloat("max_penetration", 0);
	m_iTracerFreq = pKeyValuesData->GetInt("tracer_freq", 2);

	m_flMinNPCRange = pKeyValuesData->GetInt( "min_npc_range", 24 );
	m_flMinNPCRange = pKeyValuesData->GetInt( "max_npc_range", 2000 );

	KeyValues *pKickData = pKeyValuesData->FindKey( "Kickback" );
	if( pKickData )
	{
		Kick.x_min = pKickData->GetFloat( "x_min", 0.0f );
		Kick.x_max = pKickData->GetFloat( "x_max", 0.0f );

		Kick.y_min = pKickData->GetFloat( "y_min", 0.0f );
		Kick.y_max = pKickData->GetFloat( "y_max", 0.0f );

		Kick.z_min = pKickData->GetFloat( "z_min", 0.0f );
		Kick.z_max = pKickData->GetFloat( "z_max", 0.0f );
	} else //The key might not exist, we don't want ghost values.
		Kick.NotSet();

	KeyValues *pHitBoxData = pKeyValuesData->FindKey( "HitBoxData" );
	if( pHitBoxData )
	{

		HitBoxDamage.fDamageHead	= pHitBoxData->GetFloat( "Head", 1.0f );

		HitBoxDamage.fDamageChest	= pHitBoxData->GetFloat( "Chest", 0.5f );

		HitBoxDamage.fDamageStomach	= pHitBoxData->GetFloat( "Stomach", 0.5f );

		HitBoxDamage.fLeftArm		= pHitBoxData->GetFloat( "LeftArm", 0.25f );
		HitBoxDamage.fRightArm		= pHitBoxData->GetFloat( "RightArm", 0.25f );

		HitBoxDamage.fLefLeg		= pHitBoxData->GetFloat( "LeftLeg", 0.25f );
		HitBoxDamage.fRightLeg		= pHitBoxData->GetFloat( "RightLeg", 0.25f );

	} else //The key might not exist, we don't want ghost values.
		HitBoxDamage.NotSet();

	// Regular Spread
	KeyValues *pSpreadData = pKeyValuesData->FindKey( "Spread" );
	if( pSpreadData )
	{
		m_vecSpread.x = pSpreadData->GetFloat( "x", 0.0f );
		m_vecSpread.y = pSpreadData->GetFloat( "y", 0.0f );
		m_vecSpread.z = pSpreadData->GetFloat( "z", 0.0f );
	} else {
		m_vecSpread.x = m_vecSpread.y = m_vecSpread.z = 0;
	}
	// Sighed Spread
	KeyValues *pSpreadDataSighted = pKeyValuesData->FindKey( "SpreadSighted" );
	if( pSpreadDataSighted )
	{
		m_vecSpreadSighted.x = pSpreadDataSighted->GetFloat( "x", 0.0f );
		m_vecSpreadSighted.y = pSpreadDataSighted->GetFloat( "y", 0.0f );
		m_vecSpreadSighted.z = pSpreadDataSighted->GetFloat( "z", 0.0f );
	}
	else 
	{
		m_vecSpreadSighted.x = m_vecSpreadSighted.y = m_vecSpreadSighted.z = 0;
	}

	const char *pAnimEx = pKeyValuesData->GetString( "PlayerAnimationExtension", "mp5" );
	Q_strncpy( m_szAnimExtension, pAnimEx, sizeof( m_szAnimExtension ) );
}
