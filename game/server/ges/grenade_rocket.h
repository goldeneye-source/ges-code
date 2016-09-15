///////////// Copyright © 2009, Goldeneye: Source. All rights reserved. /////////////
// 
// File: grenade_rocket.h
// Description:
//      Encapsulates a one-off rocket that explodes on impact with anything
//
// Created On: 12/5/2009
// Created By: Jonathan White <killermonkey01@gmail.com>
/////////////////////////////////////////////////////////////////////////////

#ifndef	GEROCKET_H
#define	GEROCKET_H

#ifdef _WIN32
#pragma once
#endif

#include "smoke_trail.h"
#include "grenade_gebase.h"
#include "ge_shareddefs.h"

class CGERocket : public CGEBaseGrenade
{
public:
	DECLARE_CLASS( CGERocket, CGEBaseGrenade );
	DECLARE_DATADESC();

	virtual void	Spawn( void );
	virtual void	Precache( void );

	virtual bool		IsInAir( void )		{ return true; };
	virtual GEWeaponID	GetWeaponID( void ) { return WEAPON_ROCKET_LAUNCHER; };
	virtual const char* GetPrintName( void ) { return "#GE_RocketLauncher"; };

	virtual int		OnTakeDamage( const CTakeDamageInfo &inputInfo );

	// Functions
	virtual void	ExplodeTouch( CBaseEntity *pOther );

	virtual void	IgniteThink( void );
	virtual void	AccelerateThink( void );
	virtual void	FlyThink( void );

	virtual void	CreateSmokeTrail();

protected:
	virtual void Explode();

private:
	Vector m_vForward;
	Vector m_vUp;
	Vector m_vRight;

	float m_fthinktime;
	float m_fFuseTime;

	float m_flSpawnTime;
	CHandle<SmokeTrail> m_hSmokeTrail;

	bool m_bHitPlayer;
};

#endif
