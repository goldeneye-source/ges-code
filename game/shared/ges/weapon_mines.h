///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: weapon_mines.h
// Description:
//      This file includes the weapon classes for mines, since a lot of the 
//		 methods will be the same, a base class was also used to help make
//		 things easier.
//
// Created On: 7/23/2008
// Created By: Jonathan White <killermonkey01@gmail.com> (Some code taken from Scott Lloyd)
/////////////////////////////////////////////////////////////////////////////

#ifndef	GE_MINE_H
#define	GE_MINE_H
#ifdef _WIN32
#pragma once
#endif

#include "ge_shareddefs.h"
#include "ge_weapon.h"
#include "basegrenade_shared.h"

#if defined( CLIENT_DLL )
	#define CGEWeaponMine			C_GEWeaponMine

	#define CWeaponRemoteMine		C_WeaponRemoteMine
	#define CWeaponTimedMine		C_WeaponTimedMine
	#define CWeaponProximityMine	C_WeaponProximityMine
#endif

// Mine states
enum MineState
{
	GE_MINE_THROW, // Primary Attack will throw the mine
	GE_MINE_ATTACH, // Primary Attack will plant the mine
	GE_MINE_DETONATE, // Only used by remote mine, primary attack detonates all mines
	GE_MINE_WATCH, // Only used by remote mine, 
};

// To save from having multiple versions of the mine we just textured it differently
#define FAMILY_GROUP_REMOTE		0
#define FAMILY_GROUP_TIMED		1
#define FAMILY_GROUP_PROXIMITY	2

//------------------------------------------------------------------------------------------
//
// Base Mine weapon class, all mines will derive from this.
//
class CGEWeaponMine : public CGEWeapon
{
public:
	DECLARE_CLASS( CGEWeaponMine, CGEWeapon );
#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif
	
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	
	CGEWeaponMine( void );

	virtual bool IsExplosiveWeapon() { return true; }
	virtual bool IsThrownWeapon()	 { return true; }

	virtual void Precache( void );
	const char  *GetWorldModel( void ) const;
	virtual int  GetMineType( void ) { return -1; }

	virtual bool Deploy( void );
	virtual void Drop( const Vector &vecVelocity );

	virtual void PrimaryAttack( void );

	virtual void ItemPreFrame( void );
	virtual void ItemPostFrame( void );

	void ThrowMine( void );

#ifdef CLIENT_DLL
	virtual int  GetWorldModelIndex( void );
#else
	virtual int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	// NPC Functions
	virtual void	FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles );
#endif

protected:
	CNetworkVar( bool, m_bPreThrow );
	CNetworkVar( float, m_flReleaseTime );
	CNetworkVar( int, m_iCrateModelIndex );
	CNetworkVar( MineState, m_iMineState );

private:
	CGEWeaponMine( const CGEWeaponMine & );
};

//------------------------------------------------------------------------------------------
//
// Remote Mine weapon class
//
class CWeaponRemoteMine : public CGEWeaponMine
{
public:
	DECLARE_CLASS( CWeaponRemoteMine, CGEWeaponMine );
#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif

	// Explicitly declare the activity tables to account for watch behavior
	static acttable_t m_acttable_mine[];
	static acttable_t m_acttable_watch[];
	acttable_t *ActivityList( void );
	int ActivityListCount( void );

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_REMOTEMINE; }

	CWeaponRemoteMine( void ) { };
	virtual void Spawn( void );
	virtual void Precache( void );
	virtual void Equip( CBaseCombatCharacter *pOwner );
	virtual void Drop( const Vector &vecVelocity );

	virtual void PrimaryAttack( void );
	virtual void SecondaryAttack( void );

	virtual void ToggleWatch( void );
	virtual void WeaponIdle( void );
	virtual void HandleFireOnEmpty( void );

	virtual Activity GetDrawActivity( void );

	virtual void ItemPreFrame( void );
	virtual void ItemPostFrame( void );

	CNetworkVar( bool, m_bWatchDeployed ); // Do we currently have the watch out?
#ifdef GAME_DLL
	bool m_bSwitchOnBlow;
#else
	virtual int  GetWorldModelIndex( void );
#endif

//	CNetworkVar( int, m_iWatchModelIndex );

private:
	CWeaponRemoteMine( const CWeaponRemoteMine & );
};

//------------------------------------------------------------------------------------------
//
// Proximity Mine weapon class
//
class CWeaponProximityMine : public CGEWeaponMine
{
public:
	DECLARE_CLASS( CWeaponProximityMine, CGEWeaponMine );
#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif

	DECLARE_ACTTABLE();

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_PROXIMITYMINE; }

	CWeaponProximityMine( void ) { };
	virtual void Precache(void);
	virtual void Spawn( void );
	virtual void Equip( CBaseCombatCharacter *pOwner );

private:
	CWeaponProximityMine( const CWeaponProximityMine & );
};

//------------------------------------------------------------------------------------------
//
// Timed Mine weapon class
//
class CWeaponTimedMine : public CGEWeaponMine
{
public:
	DECLARE_CLASS( CWeaponTimedMine, CGEWeaponMine );
#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif

	DECLARE_ACTTABLE();

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_TIMEDMINE; }

	CWeaponTimedMine( void ) { };
	virtual void Precache(void);
	virtual void Spawn( void );
	virtual void Equip( CBaseCombatCharacter *pOwner );

private:
	CWeaponTimedMine( const CWeaponTimedMine & );
};

#endif
