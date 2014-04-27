///////////// Copyright © 2008, Goldeneye Source. All rights reserved. /////////////
// 
// File: weapon_klobb.cpp
// Description:
//      Implementation of weapon Klobb.
//
// Created On: 7/24/2008
// Created By: Killermonkey
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"
#include "ge_weaponautomatic.h"

#ifdef CLIENT_DLL
#define CWeaponKlobb C_WeaponKlobb
#include "c_ge_playerresource.h"
#include "c_gemp_player.h"
#else

#include "gemp_player.h"

#endif

//-----------------------------------------------------------------------------
// CWeaponKlobb
//-----------------------------------------------------------------------------

class CWeaponKlobb : public CGEWeaponAutomatic
{
public:
	DECLARE_CLASS( CWeaponKlobb, CGEWeaponAutomatic );

	CWeaponKlobb(void);

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual void Equip( CBaseCombatCharacter *pOwner );
	virtual void SetViewModel();

	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_KLOBB; }
	
	DECLARE_ACTTABLE();

private:
	CWeaponKlobb( const CWeaponKlobb & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponKlobb, DT_WeaponKlobb )

BEGIN_NETWORK_TABLE( CWeaponKlobb, DT_WeaponKlobb )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponKlobb )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_klobb, CWeaponKlobb );
PRECACHE_WEAPON_REGISTER( weapon_klobb );

acttable_t CWeaponKlobb::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_GES_IDLE_PISTOL,						false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_PISTOL,				false },

	{ ACT_MP_RUN,						ACT_GES_RUN_PISTOL,							false },
	{ ACT_MP_WALK,						ACT_GES_WALK_PISTOL,						false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_PISTOL,				false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_PISTOL,		false },
	{ ACT_GES_ATTACK_RUN_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_PISTOL,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_PISTOL,		false },

	{ ACT_GES_DRAW,						ACT_GES_GESTURE_DRAW_PISTOL,				false },
	{ ACT_GES_DRAW_RUN,					ACT_GES_GESTURE_DRAW_PISTOL_RUN,			false },

	{ ACT_MP_RELOAD_STAND,				ACT_GES_GESTURE_RELOAD_PISTOL,				false },
	{ ACT_MP_RELOAD_CROUCH,				ACT_GES_GESTURE_RELOAD_PISTOL,				false },

	{ ACT_MP_JUMP,						ACT_GES_JUMP_PISTOL,						false },
};
IMPLEMENT_ACTTABLE( CWeaponKlobb );

CWeaponKlobb::CWeaponKlobb()
{
	// NPC Ranging
	m_fMaxRange1 = 1024;
}

void CWeaponKlobb::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip( pOwner );

	CGEMPPlayer *pGEPlayer = ToGEMPPlayer( pOwner );
	if ( !pGEPlayer )
	{
		m_nSkin = 0;
		return;
	}

#ifdef CLIENT_DLL
	if ( GEPlayerRes()->GetDevStatus( pGEPlayer->entindex() ) == GE_DEVELOPER )
		m_nSkin = 2;
	else if ( GEPlayerRes()->GetDevStatus( pGEPlayer->entindex() ) == GE_BETATESTER )
		m_nSkin = 1;
#else
	if ( pGEPlayer->GetDevStatus() == GE_DEVELOPER )
		m_nSkin = 2;
	else if ( pGEPlayer->GetDevStatus() == GE_BETATESTER )
		m_nSkin = 1;
#endif
	else
		m_nSkin = 0;
}

void CWeaponKlobb::SetViewModel()
{
	BaseClass::SetViewModel();

	CGEPlayer *pGEPlayer = ToGEPlayer( GetOwner() );
	if ( !pGEPlayer )
		return;
	CBaseViewModel *vm = pGEPlayer->GetViewModel( m_nViewModelIndex );
	if ( vm == NULL )
		return;

	vm->m_nSkin = m_nSkin;
}
