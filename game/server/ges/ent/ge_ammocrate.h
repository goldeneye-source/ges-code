//////////  Copyright © 2011, Goldeneye Source. All rights reserved. ///////////
// 
// ge_ammocrate.h
//
// Description:
//      An ammo crate that gives a certain amount of ammo of different types
//
// Created On: 6/20/2011
// Created By: Killer Monkey
////////////////////////////////////////////////////////////////////////////////
#ifndef GE_AMMOCRATE_H
#define GE_AMMOCRATE_H

#include "items.h"
#include "player.h"
#include "ge_shareddefs.h"

class CGEAmmoCrate : public CItem														
{																					
public:																				
	DECLARE_CLASS( CGEAmmoCrate, CItem );
	DECLARE_DATADESC();

	CGEAmmoCrate();

	// CBaseEntity functions
	virtual void Spawn();																																			
	virtual void Precache();

	// CItem functions
	virtual void ItemTouch( CBaseEntity *pEntity );
	virtual bool MyTouch( CBasePlayer *pPlayer );

	virtual void Materialize();
	virtual CBaseEntity *Respawn();

	// Special refill function if we only take partial ammo
	void RefillThink();
	
	// Ammo Crate specific functions
	void SetAmmoType( int id );
	int GetAmmoType();
	bool HasAmmo();
	void UpdateOnRemove();

	void SetWeaponID( int id );
	int GetWeaponID() { return m_iWeaponID; }
	int GetWeight() { return m_iWeight; }

private:
	int  m_iAmmoID;
	int  m_iAmmoAmount;
	bool m_bGaveGlobalAmmo;
	int  m_iWeaponID;
	int  m_iWeight;
};

#endif
