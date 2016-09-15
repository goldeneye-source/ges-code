///////////// Copyright © 2008, Killer Monkey. All rights reserved. /////////////
// 
// File: weapon_moonraker.cpp
// Description:
//      Implementation of the Moonraker Laser
//
// Created On: 12/5/09
// Created By: Killer Monkey
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "r_efx.h"
	#include "ivrenderview.h"
	#include "dlight.h"
	#include "c_ge_player.h"
#else
	#include "ge_player.h"
#endif

#include "ge_weaponpistol.h"

#ifdef CLIENT_DLL
#define CWeaponMoonraker C_WeaponMoonraker
#endif

//-----------------------------------------------------------------------------
// CWeaponMoonraker
//-----------------------------------------------------------------------------

class CWeaponMoonraker : public CGEWeaponPistol
{
public:
	DECLARE_CLASS( CWeaponMoonraker, CGEWeaponPistol );

	CWeaponMoonraker(void);

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	virtual void Precache( void );
	virtual void Equip( CBaseCombatCharacter *pOwner );
	virtual void MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );
#ifdef CLIENT_DLL
	virtual void ProcessMuzzleFlashEvent();
#else
	virtual int		CapabilitiesGet( void ) { return bits_CAP_WEAPON_RANGE_ATTACK1; }
	// NPC Functions
	virtual void	FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles );
#endif

	virtual void PrimaryAttack( void );

	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_MOONRAKER; }

	DECLARE_ACTTABLE();

private:
	CWeaponMoonraker( const CWeaponMoonraker& );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponMoonraker, DT_WeaponMoonraker )

BEGIN_NETWORK_TABLE( CWeaponMoonraker, DT_WeaponMoonraker )
#ifdef CLIENT_DLL
	RecvPropInt( RECVINFO( m_nMuzzleFlashParity )),
#else
	SendPropInt( SENDINFO( m_nMuzzleFlashParity ), EF_MUZZLEFLASH_BITS, SPROP_UNSIGNED ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponMoonraker )
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_moonraker, CWeaponMoonraker );
PRECACHE_WEAPON_REGISTER( weapon_moonraker );

acttable_t CWeaponMoonraker::m_acttable[] = 
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
	{ ACT_GES_CJUMP,					ACT_GES_CJUMP_PISTOL,						false },
};
IMPLEMENT_ACTTABLE( CWeaponMoonraker );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponMoonraker::CWeaponMoonraker( void )
{
}

void CWeaponMoonraker::Precache( void )
{
	PrecacheModel("models/weapons/moonraker/v_moonraker.mdl");
	PrecacheModel("models/weapons/moonraker/w_moonraker.mdl");

	PrecacheMaterial("sprites/hud/weaponicons/moonraker");
	PrecacheMaterial("sprites/hud/ammoicons/ammo_moonraker");

	PrecacheScriptSound("Weapon_moonraker.Single");
	PrecacheScriptSound("Weapon_moonraker.NPC_Single");

	BaseClass::Precache();
	PrecacheParticleSystem( "tracer_laser" );
}

void CWeaponMoonraker::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip( pOwner );
	GetOwner()->SetAmmoCount( AMMO_MOONRAKER_MAX, m_iPrimaryAmmoType );
}

void CWeaponMoonraker::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType )
{
	CBaseEntity *pOwner = GetOwner();

	if ( pOwner == NULL )
	{
		BaseClass::MakeTracer( vecTracerSrc, tr, iTracerType );
		return;
	}

	Vector vNewSrc = vecTracerSrc;
	int iEntIndex = pOwner->entindex();

	if ( g_pGameRules->IsMultiplayer() )
	{
		iEntIndex = entindex();
	}

	int iAttachment = GetTracerAttachment();
	UTIL_ParticleTracer( "tracer_laser", vNewSrc, tr.endpos, iEntIndex, iAttachment, false );
}

#ifdef CLIENT_DLL

extern ConVar muzzleflash_light;
extern void FormatViewModelAttachment( Vector &vOrigin, bool bInverse );
void CWeaponMoonraker::ProcessMuzzleFlashEvent()
{
	// If we have an attachment, then stick a light on it.
	if ( muzzleflash_light.GetBool() )
	{
		Vector vAttachment;
		QAngle dummyAngles;

		if ( !GetAttachment( "muzzle", vAttachment, dummyAngles ) )
			vAttachment = GetAbsOrigin();

		// Format the position for first person view
		if ( GetOwner() == CBasePlayer::GetLocalPlayer() )
			::FormatViewModelAttachment( vAttachment, true );
	
		// Make an elight (entities)
		dlight_t *el = effects->CL_AllocElight( LIGHT_INDEX_MUZZLEFLASH + index );
		el->origin = vAttachment;
		el->radius = random->RandomInt( 64, 80 ); 
		el->decay = el->radius / 0.05f;
		el->die = gpGlobals->curtime + 0.05f;
		el->color.r = 83;
		el->color.g = 169;
		el->color.b = 255;
		el->color.exponent = 5;

		// Make a dlight (world)
		dlight_t *dl = effects->CL_AllocDlight( LIGHT_INDEX_MUZZLEFLASH + index );
		dl->origin = vAttachment;
		dl->radius = random->RandomInt( 64, 80 ); 
		dl->decay = el->radius / 0.05f;
		dl->flags |= DLIGHT_NO_MODEL_ILLUMINATION;
		dl->die = gpGlobals->curtime + 0.05f;
		dl->color.r = 83;
		dl->color.g = 169;
		dl->color.b = 255;
		dl->color.exponent = 5;
	}
}

#endif

void CWeaponMoonraker::PrimaryAttack( void )
{
	BaseClass::PrimaryAttack();
	GetOwner()->SetAmmoCount( AMMO_MOONRAKER_MAX, m_iPrimaryAmmoType );
}

#ifdef GAME_DLL
void CWeaponMoonraker::FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles )
{
	BaseClass::FireNPCPrimaryAttack( pOperator, bUseWeaponAngles );
	GetOwner()->SetAmmoCount( AMMO_MOONRAKER_MAX, m_iPrimaryAmmoType );
}
#endif