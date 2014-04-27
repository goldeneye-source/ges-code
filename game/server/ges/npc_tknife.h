///////////// Copyright © 2008, Goldeneye: Source All rights reserved. /////////////
// 
// File: npc_tknife.h
// Description:
//      Weapon Throwing Knife Projectile definition.  *shing shing* 
//
// Created On: 11/23/2008
// Created By: Killermonkey 
/////////////////////////////////////////////////////////////////////////////
#ifndef NPC_TKNIFE_H
#define NPC_TKNIFE_H

#include "grenade_gebase.h"
#include "ge_player.h"

class CGETKnife : public CGEBaseGrenade
{
public:
	DECLARE_CLASS( CGETKnife, CGEBaseGrenade );

	CGETKnife( void );

	DECLARE_DATADESC();

	virtual bool		IsInAir( void ) { return m_bInAir; };
	virtual GEWeaponID	GetWeaponID( void ) { return WEAPON_KNIFE_THROWING; };
	virtual const char *GetPrintName( void ) { return "#GE_ThrowingKnife"; };

	virtual void Spawn( void );
	virtual void Precache( void );

	virtual void SoundThink( void );
	virtual void RemoveThink( void );

	virtual void SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity );
	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	virtual void CollisionEventToTrace( int index, gamevcollisionevent_t *pEvent, trace_t &tr );

	virtual void DamageTouch( CBaseEntity *pOther );
	virtual void PickupTouch( CBaseEntity *pOther );
	virtual bool MyTouch( CBaseCombatCharacter *pToucher );

	virtual float	GetDamage() { return m_flDamage; }
	virtual void	SetDamage(float flDamage) { m_flDamage = flDamage; }

private:
	bool m_bInAir;
	float m_flDamage;
	float m_flStopFlyingTime;
};

#endif
