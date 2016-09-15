//////////  Copyright © 2011, Goldeneye Source. All rights reserved. ///////////
// 
// File: ge_spawner.h
// Description: Base class for all spawners. To be used as an interface or stand alone.
//
// Created On: 7-19-11
// Created By: KillerMonkey
///////////////////////////////////////////////////////////////////////////////
#ifndef GE_SPAWNER_H
#define GE_SPAWNER_H

#include "ge_shareddefs.h"

class CGESpawner : public CPointEntity
{
public:
	DECLARE_CLASS( CGESpawner, CPointEntity );
	DECLARE_DATADESC();

	CGESpawner();

	// Basic Entity Functions
	void Spawn();

	// Basic operations
	void Init();
	void Think();

	void SpawnEnt(int spawnstate);
	void RemoveEnt();
	
	// Handle overrides to the base entity
	void		SetOverride( const char *szClassname, float secToSpawn = 0 );
	const char *GetBaseClass()	{ return m_szBaseEntity; }
	const char *GetOverrideClass()	{ return m_szOverrideEntity; }

	bool		IsOverridden()	{ return m_szOverrideEntity[0] != 0; }
	bool		IsValid()		{ return m_szBaseEntity[0] != 0; }
	bool		IsEmpty()		{ return m_hCurrentEntity.Get() == NULL || m_hCurrentEntity->IsMarkedForDeletion(); }

	bool		IsEnabled()		{ return !m_bDisabled; }
	bool		IsDisabled()	{ return m_bDisabled; }

	virtual bool IsSpecial()	{ return m_iSlot == (MAX_WEAPON_SPAWN_SLOTS-1); }
	virtual bool IsVerySpecial()	{ return false; }
	const int	 GetSlot()		{ return m_iSlot; }

	void SetEnabled( bool state );
	// Input handlers
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );

	void InputEnableResetting(inputdata_t &inputdata);
	void InputDisableResetting(inputdata_t &inputdata);

protected:
	// Should we respawn now?
	virtual int ShouldRespawn();
	virtual float GetRespawnInterval();

	// Overridable "callbacks"
	virtual void OnInit() { }
	virtual void OnEntSpawned( const char *szClassname )	{ }
	virtual void OnEntOverridden( const char *szClassname ) { }
	virtual void OnEntPicked();
	virtual void OnEnabled();
	virtual void OnDisabled();

	// Ability for inheritors to set our base entity
	void SetBaseEnt( const char *szClassname );
	CBaseEntity *GetEnt() { return m_hCurrentEntity; }

	COutputEvent m_OnPickedUp;
	COutputEvent m_OnRespawn;

private:
	// KeyValues
	// Slot determines which weapon/ammo we will spawn (default:-1)
	int  m_iSlot;
	bool m_bDisabled;
	bool m_bOrigDisableState;
	bool m_bResetOnNewRound;

	char m_szBaseEntity[MAX_ENTITY_NAME];
	char m_szOverrideEntity[MAX_ENTITY_NAME];

	float m_fNextSpawnTime;
	float m_fNextMoveCheck;

	EHANDLE m_hCurrentEntity;
};

#endif

