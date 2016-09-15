///////////// Copyright © 2006, Scott Loyd. All rights reserved. /////////////
// 
// File: ge_weapon_parse.h
// Description:
//      Holds values for specific weapons script arguments.
//
// Created On: 3/7/2006 2:39:28 PM
// Created By: Scott Loyd <mailto:scottloyd@gmail.com> 
/////////////////////////////////////////////////////////////////////////////


#ifndef GE_WEAPON_PARSE_H
#define GE_WEAPON_PARSE_H
#ifdef _WIN32
#pragma once
#endif


#include "hl2mp_weapon_parse.h"
#include "networkvar.h"


//--------------------------------------------------------------------------------------------------------
class CGEWeaponInfo : public FileWeaponInfo_t
{
public:
	DECLARE_CLASS_GAMEROOT( CGEWeaponInfo, FileWeaponInfo_t );
	
	CGEWeaponInfo();
	
	virtual void Parse( ::KeyValues *pKeyValuesData, const char *szWeaponName );

	int		m_iDamage;
	int		m_iDamageCap;
	int		m_iAmmoIcon;
	float	m_flFireDelay;
	float	m_flDamageRadius;
	float	m_flRateOfFire;
	float	m_flClickRateOfFire;
	float	m_flAccurateRateOfFire;
	int		m_flAccurateShots;
	Vector	m_vecSpread;
	Vector	m_vecMaxSpread;
	int		m_iZoomOffset;
	float	m_flMaxPenetration;
	int		m_iTracerFreq;
	int		m_flGaussFactor;
	int		m_flGaussPenalty;
	float	m_flAimBonus;
	float	m_flJumpPenalty;

	float	m_flMinNPCRange;
	float	m_flMaxNPCRange;

	struct _HitBoxDamage
	{
		float	fDamageHead;
		float	fDamageChest;
		float	fDamageStomach;
		float	fLeftArm;
		float	fRightArm;
		float	fLefLeg;
		float	fRightLeg;

		void NotSet()
		{
			fDamageHead		= 1.0f;
			fDamageChest	= 0.5f;
			fDamageStomach	= 0.5f;
			fLeftArm		= 0.25f;
			fRightArm		= 0.25f;
			fLefLeg			= 0.25f;
			fRightLeg		= 0.25f;
		}
	} HitBoxDamage;
	
	struct _Kick
	{
		float x_min;
		float x_max;
		float y_min;
		float y_max;
		float z_min;
		float z_max;

		void NotSet()
		{
			x_min = 0.0f;
			x_max = 0.0f;
			y_min = 0.0f;
			y_max = 0.0f;
			z_min = 0.0f;
			z_max = 0.0f;
		};
	} Kick;

	char m_szAnimExtension[16];		// string used to generate player animations with this weapon
	char m_szSpecialAttributes[32];		// string used to store special attributes for the help
};


#endif // GE_WEAPON_PARSE_H
