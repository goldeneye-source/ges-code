///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_weaponpistol.h
// Description:
//      All Goldeneye PISTOLS derive from this class
//
// Created On: 3/5/2008 1800
// Created By: Jonathan White <killermonkey>
/////////////////////////////////////////////////////////////////////////////

#ifndef GE_WEAPON_PISTOL_H
#define GE_WEAPON_PISTOL_H
#ifdef _WIN32
#pragma once
#endif

#include "ge_shareddefs.h"
#include "ge_weapon.h"

#if defined( CLIENT_DLL )
	#define CGEWeaponPistol C_GEWeaponPistol
#endif

class CGEWeaponPistol : public CGEWeapon
{
#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif

	DECLARE_CLASS( CGEWeaponPistol, CGEWeapon );

public:
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CGEWeaponPistol();

	virtual void	ItemPostFrame( void );

	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack( void ) { };
	virtual bool	Reload( void );

#ifdef GAME_DLL
	virtual int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
#endif

	void	AddViewKick( void );

	Activity	GetPrimaryAttackActivity( void );

public:
	// Ensure these vars can be read/modified by our children
	CNetworkVar( float,	m_flSoonestPrimaryAttack );
	CNetworkVar( float,	m_flLastAttackTime );
	CNetworkVar( int,	m_nNumShotsFired );

private:
	CGEWeaponPistol( const CGEWeaponPistol & );
};

#endif //GE_WEAPON_PISTOL_H