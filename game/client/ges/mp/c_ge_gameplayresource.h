///////////// Copyright © 2011, Goldeneye: Source. All rights reserved. /////////////
// 
// File: c_ge_gameplayresource.h
// Description:
//      Receives gameplay related information from server for HUD/VGUI updates
//
// Created On: 25 Jul 11
// Created By: Jonathan White <killermonkey> 
////////////////////////////////////////////////////////////////////////////////////
#ifndef C_GEGAMEPLAYRESOURCE_H
#define C_GEGAMEPLAYRESOURCE_H
#ifdef _WIN32
#pragma once
#endif

#include "ge_shareddefs.h"
#include "c_baseentity.h"

class C_GEGameplayResourceListener
{
public:
	C_GEGameplayResourceListener();
	~C_GEGameplayResourceListener();

	virtual void OnGameplayDataUpdate()=0;
};

class C_GEGameplayResource : public C_BaseEntity
{
	DECLARE_CLASS( C_GEGameplayResource, C_BaseEntity );

public:
	DECLARE_CLIENTCLASS();

	C_GEGameplayResource();
	~C_GEGameplayResource();

	virtual void OnDataChanged( DataUpdateType_t type );

	const char* GetGameplayIdent()	{ return m_GameplayIdent; }
	const char* GetGameplayName()	{ return m_GameplayName; }
	const char* GetLoadoutIdent()	{ return m_LoadoutIdent; }
	const char* GetLoadoutName()	{ return m_LoadoutName; }

	int	  GetGameplayHelp()			{ return m_GameplayHelp; }
	bool  GetGameplayOfficial()		{ return m_GameplayOfficial; }
	int   GetGameplayRoundNum()		{ return m_GameplayRoundNum; }
	float GetGameplayRoundStart()	{ return m_GameplayRoundStart; }
	bool  GetGameplayIntermission()	{ return m_GameplayIntermission; }

	int  GetLoadoutWeapon( int slot );
	void GetLoadoutWeaponList( CUtlVector<int>& list );

	void GetCharacterExclusion( CUtlVector<char*> &list );

protected:
	// Gameplay Data
	char  m_GameplayIdent[32];
	char  m_GameplayName[64];
	int   m_GameplayHelp;
	bool  m_GameplayOfficial;
	int   m_GameplayRoundNum;
	float m_GameplayRoundStart;
	bool  m_GameplayIntermission;

	// Loadout Data
	char m_LoadoutIdent[32];
	char m_LoadoutName[64];
	int  m_LoadoutWeapons[MAX_WEAPON_SPAWN_SLOTS];

	// Character Data
	char m_CharExclusion[64];
};

extern C_GEGameplayResource *GEGameplayRes();

#endif
