///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: grenade_ge.h
// Description:
//      This is what makes the grenades go BOOM
//
// Created On: 9/29/2008
// Created By: Jonathan White <killermonkey01@gmail.com> (Some code taken from HL2)
/////////////////////////////////////////////////////////////////////////////

#ifndef	GEGRENADE_H
#define	GEGRENADE_H

#ifdef _WIN32
#pragma once
#endif

#include "grenade_gebase.h"

class CGEGrenade : public CGEBaseGrenade
{
	DECLARE_CLASS( CGEGrenade, CGEBaseGrenade );

#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif

public:
	virtual void	Spawn( void );
	virtual void	Precache( void );

	virtual void	SetTimer( float detonateDelay );
	virtual void	SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity );
	virtual int		OnTakeDamage( const CTakeDamageInfo &inputInfo );
	virtual void	VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );

	virtual bool		IsInAir( void ) { return true; };
	virtual GEWeaponID	GetWeaponID( void ) { return WEAPON_GRENADE; };
	virtual const char* GetPrintName( void ) { return "#GE_Grenade"; };
	virtual int			GetCustomData(void) { return m_bDroppedOnDeath * 2 + !m_bHitSomething; };

	// Functions
	virtual void	DelayThink();
	bool m_bDroppedOnDeath;
	bool m_bHitSomething;
};

#endif