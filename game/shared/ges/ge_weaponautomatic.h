///////////// Copyright © 2008, Goldeneye Source. All rights reserved. /////////////
// 
// File: ge_weaponautomatic.h
// Description:
//      All Goldeneye AUTOMATICS derive from this class
//
// Created On: 3/15/2008 13:45
// Created By: Mario Cacciatore
/////////////////////////////////////////////////////////////////////////////

#ifndef GE_WEAPON_AUTOMATIC_H
#define GE_WEAPON_AUTOMATIC_H
#ifdef _WIN32
#pragma once
#endif

#include "ge_shareddefs.h"
#include "ge_weapon.h"

#if defined( CLIENT_DLL )
	#define CGEWeaponAutomatic C_GEWeaponAutomatic
#endif

class CGEWeaponAutomatic : public CGEWeapon
{
public:
	DECLARE_CLASS( CGEWeaponAutomatic, CGEWeapon );
	DECLARE_DATADESC();
	DECLARE_PREDICTABLE();

	CGEWeaponAutomatic();

	DECLARE_NETWORKCLASS();

	virtual bool	IsAutomaticWeapon() { return true; }

	virtual void	ItemPostFrame( void );
	virtual bool	Deploy( void );

	virtual void	PrimaryAttack( void );
	virtual bool	Reload( void );

	// NPC Functions
	virtual int		GetMinBurst() { return 3; }
	virtual int		GetMaxBurst() { return 5; }

#ifdef GAME_DLL
	virtual int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
#endif

	int				WeaponSoundRealtime( WeaponSound_t shoot_type );

	void	AddViewKick( void );
	// utility function
	static void DoMachineGunKick( CBasePlayer *pPlayer, float dampEasy, float maxVerticleKickAngle, float fireDurationTime, float slideLimitTime );

public:
	// Ensure these vars can be read/modified by our children
	CNetworkVar( int,	m_nNumShotsFired );

	float m_flNextSoundTime;

private:
	CGEWeaponAutomatic( const CGEWeaponAutomatic & );
};

#endif //GE_WEAPON_AUTOMATIC_H