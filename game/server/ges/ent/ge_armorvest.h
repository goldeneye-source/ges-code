//////////  Copyright © 2008, Goldeneye Source. All rights reserved. ///////////
// 
// ge_ammocrate.h
//
// Description:
//      An ammo crate that gives a certain amount of ammo of different types
//
// Created On: 3/22/2008 1200
// Created By: KillerMonkey
////////////////////////////////////////////////////////////////////////////////
#ifndef GE_ARMORVEST_H
#define GE_ARMORVEST_H

#include "items.h"

class CGEArmorVest : public CItem														
{																					
public:																				
	DECLARE_CLASS( CGEArmorVest, CItem );
	DECLARE_DATADESC();

	CGEArmorVest();

	virtual void Spawn( void );																																			
	virtual void Precache( void );

	virtual CBaseEntity *Respawn( void );
	virtual void Materialize( void );

	virtual void ItemTouch( CBaseEntity *pEntity );
	virtual bool MyTouch( CBasePlayer *pPlayer );

	void SetEnabled( bool state );
	bool IsEnabled() { return m_bEnabled; }

	// Input handlers
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );	

protected:
	void OnEnabled();
	void OnDisabled();

private:
	int		m_iAmount;
	bool	m_bEnabled;
};

#endif
