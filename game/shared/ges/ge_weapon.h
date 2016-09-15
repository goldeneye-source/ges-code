///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_weapon.h
// Description:
//      All Goldeneye Weapons derive from this class.
//
// Created On: 2/25/2008 1800
// Created By: Jonathan White <killermonkey>
/////////////////////////////////////////////////////////////////////////////

#ifndef GE_WEAPON_H
#define GE_WEAPON_H
#ifdef _WIN32
#pragma once
#endif

#include "ge_shareddefs.h"
#include "ge_weapon_parse.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"

#ifdef CLIENT_DLL
	#include "c_ge_player.h"
	#include "ge_screeneffects.h"
#else
	#include "ge_player.h"
	#include "ai_basenpc.h"
#endif

#if defined( CLIENT_DLL )
	#define CGEWeapon C_GEWeapon
#endif

typedef enum
{
	Primary_Mode = 0,
	Secondary_Mode,
} GEWeaponMode;


class CGEWeapon : public CBaseHL2MPCombatWeapon
{
#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif

	DECLARE_CLASS( CGEWeapon, CBaseHL2MPCombatWeapon );

public:
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CGEWeapon();
	
#ifdef GAME_DLL
	~CGEWeapon();
#endif

	virtual void	Precache();

	virtual GEWeaponID GetWeaponID() const { return WEAPON_NONE; }

	// For Python and NPC's
	virtual bool	IsExplosiveWeapon() { return false; }
	virtual bool	IsAutomaticWeapon() { return false; }
	virtual bool	IsThrownWeapon()	{ return false; }

	virtual void	PrimaryAttack();
	virtual bool	Reload();
	virtual void	OnReloadOffscreen();
	virtual void	DryFire();
	virtual void	PrepareFireBullets(int number, CBaseCombatCharacter *pOperator, Vector vecShootOrigin, Vector vecShootDir, bool haveplayer);
	virtual void	PreOwnerDeath(); //Fired when owner dies but before anything else.

#ifdef GAME_DLL
	virtual void	Spawn();
	virtual void	UpdateOnRemove();
	virtual int		UpdateTransmitState();

	virtual float	GetHeldTime() { return gpGlobals->curtime - m_flDeployTime; };
	virtual bool	CanEquip( CBaseCombatCharacter *pOther ) { return true; };

	// NPC Functions
	virtual void	FireNPCPrimaryAttack( CBaseCombatCharacter *pOperator, bool bUseWeaponAngles );
	virtual void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	virtual void 	DefaultTouch( CBaseEntity *pOther );

	virtual void	SetupGlow( bool state, Color glowColor = Color(255,255,255), float glowDist = 350.0f );

	Vector	GetOriginalSpawnOrigin( void ) { return m_vOriginalSpawnOrigin;	}
	QAngle	GetOriginalSpawnAngles( void ) { return m_vOriginalSpawnAngles;	}
#endif

	// Get GE weapon specific weapon data.
	// Get a pointer to the player that owns this weapon
	CGEWeaponInfo const	&GetGEWpnData() const;

	virtual float			GetFireRate( void );
	virtual float			GetClickFireRate(void);
	virtual const Vector&	GetBulletSpread( void );
	virtual int				GetGaussFactor( void );
	virtual float			GetFireDelay( void );
	virtual int				GetTracerFreq( void ) { return GetGEWpnData().m_iTracerFreq; };
	virtual const char*		GetSpecAttString(void) { return GetGEWpnData().m_szSpecialAttributes; };

	virtual int		GetDamageCap(void) { return GetGEWpnData().m_iDamageCap; };

	virtual float	GetMaxPenetrationDepth( void ) { return GetGEWpnData().m_flMaxPenetration; };
	virtual void	AddAccPenalty(float numshots);
	virtual void	UpdateAccPenalty( void );
	virtual float	GetAccPenalty(void) { return m_flAccuracyPenalty; };

	// Fixes for NPC's
	virtual bool	HasAmmo( void );
	virtual bool	HasShotsLeft( void );
	virtual bool	IsWeaponVisible( void );

	virtual void	ItemPostFrame( void );

	virtual void	Equip( CBaseCombatCharacter *pOwner );
	virtual void	Drop( const Vector &vecVelocity );
	virtual	bool	Deploy( void );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );



	virtual void	SetViewModel();
	virtual void	SetSkin( int skin );
	virtual int		GetSkin()						{ return (int)m_nSkin; }

	virtual int		GetBodygroupFromName(const char* name);
	virtual void	SwitchBodygroup( int group, int value );
	
	virtual void	SetPickupTouch( void );
	virtual void	MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType );

	virtual float	GetWeaponDamage()				{ return GetGEWpnData().m_iDamage;					}
	virtual float	GetHitBoxDataModifierHead()		{ return GetGEWpnData().HitBoxDamage.fDamageHead;	}
	virtual float	GetHitBoxDataModifierChest()	{ return GetGEWpnData().HitBoxDamage.fDamageChest;	}
	virtual float	GetHitBoxDataModifierStomach()	{ return GetGEWpnData().HitBoxDamage.fDamageStomach;}
	virtual float	GetHitBoxDataModifierLeftArm()	{ return GetGEWpnData().HitBoxDamage.fLeftArm;		}
	virtual float	GetHitBoxDataModifierRightArm() { return GetGEWpnData().HitBoxDamage.fRightArm;		}
	virtual float	GetHitBoxDataModifierLeftLeg()	{ return GetGEWpnData().HitBoxDamage.fLefLeg;		}
	virtual float	GetHitBoxDataModifierRightLeg() { return GetGEWpnData().HitBoxDamage.fRightLeg;		}

	virtual bool	CanBeSilenced( void ) { return false; };
	virtual bool	IsSilenced() { return m_bSilenced; };
	virtual bool	IsShotgun() { return false; };
	virtual void	ToggleSilencer( bool doanim = true );
	virtual void	SetAlwaysSilenced( bool set ) { m_bIsAlwaysSilent = set; };
	virtual bool	IsAlwaysSilenced() { return m_bIsAlwaysSilent; };
	virtual float	GetAccFireRate(){ return GetGEWpnData().m_flAccurateRateOfFire; }
	virtual int		GetAccShots(){ return GetGEWpnData().m_flAccurateShots; }

	virtual int		GetTracerAttachment( void );

#ifdef CLIENT_DLL
	virtual float CalcViewmodelBob( void );
	virtual void AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles );
	virtual void PostDataUpdate( DataUpdateType_t updateType );
	virtual void UpdateVisibility();
#endif

	virtual bool	 PlayEmptySound( void );

	virtual Activity GetPrimaryAttackActivity( void );
	virtual Activity GetDrawActivity( void );
	virtual void	 WeaponIdle( void );

	virtual float GetZoomOffset();

	// NPC Functions
	virtual float GetMinRestTime() { return GetClickFireRate(); }
	virtual float GetMaxRestTime() { return GetFireRate(); }

	// Variable that stores our accuracy penalty time limit
	CNetworkVar( float,	m_flAccuracyPenalty );
	CNetworkVar( float, m_flCoolDownTime );

protected:
	bool			m_bLowered;			// Whether the viewmodel is raised or lowered
	float			m_flRaiseTime;		// If lowered, the time we should raise the viewmodel
	float			m_flHolsterTime;	// When the weapon was holstered

	// Used to control the barrel smoke (we don't care about synching these it is not necessary)
	void			RecordShotFired(int count = 1);
	float			m_flSmokeRate;
	int				m_iShotsFired;
	float			m_flLastShotTime;

private:
#ifdef GAME_DLL
	void SetEnableGlow( bool state );
	bool m_bServerGlow;
#else
	CEntGlowEffect *m_pEntGlowEffect;
	bool m_bClientGlow;
	float m_flLastBobCalc;
	float m_flLastSpeedrat;
	float m_flLastFallrat;
#endif

	CNetworkVar( bool, m_bEnableGlow );
	CNetworkVar( color32, m_GlowColor );
	CNetworkVar( float, m_GlowDist );
	CNetworkVar( bool,	m_bSilenced );
	bool m_bIsAlwaysSilent;

#ifdef GAME_DLL
	Vector			m_vOriginalSpawnOrigin;
	QAngle			m_vOriginalSpawnAngles;

	float			m_flDeployTime;
#endif

	CGEWeapon( const CGEWeapon & );
};

inline CGEWeapon *ToGEWeapon( CBaseCombatWeapon *pWeapon )
{
#ifdef _DEBUG
	return dynamic_cast<CGEWeapon*>(pWeapon);
#else
	return static_cast<CGEWeapon*>(pWeapon);
#endif
}

#endif // WEAPON_GEBASE_H
