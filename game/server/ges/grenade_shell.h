///////////// Copyright © 2009, Goldeneye: Source. All rights reserved. /////////////
// 
// File: grenade_shell.h
// Description:
//      This is what makes the shells bounce and go BOOOM
//
// Created On: 4/5/2009
// Created By: Jonathan White <killermonkey01@gmail.com>
/////////////////////////////////////////////////////////////////////////////

#ifndef	GESHELL_H
#define	GESHELL_H

#ifdef _WIN32
#pragma once
#endif

#include "smoke_trail.h"
#include "grenade_gebase.h"
#include "ge_shareddefs.h"

class CGEShell : public CGEBaseGrenade
{
public:
	DECLARE_CLASS( CGEShell, CGEBaseGrenade );
	DECLARE_DATADESC();

	virtual void	Spawn( void );
	virtual void	Precache( void );

	virtual bool		IsInAir( void )			{ return true; }
	virtual GEWeaponID	GetWeaponID( void )		{ return WEAPON_GRENADE_LAUNCHER; }
	virtual const char* GetPrintName( void )	{ return "#GE_GrenadeLauncher"; }
	virtual int			GetCustomData( void )	{ return m_iBounceCount; }

	virtual void	SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity );
	virtual void	VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	virtual int		OnTakeDamage( const CTakeDamageInfo &inputInfo );

	// Functions
	virtual void	PlayerTouch( CBaseEntity *pOther );
	virtual void	ExplodeThink();
	virtual void	CreateSmokeTrail();
	virtual bool	CanExplode();

private:
	int m_iBounceCount;
	CHandle<SmokeTrail> m_hSmokeTrail;
	float m_flFloorTimeout;
	bool m_bHitPlayer;
};

#endif
