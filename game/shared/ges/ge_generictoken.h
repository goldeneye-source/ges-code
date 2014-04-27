///////////// Copyright © 2009 GoldenEye: Source. All rights reserved. /////////////
//
//   Project     : Server (GES)
//   File        : ge_generictoken.h
//   Description :
//      Implements a generic token entity that can be used for gameplay purposes
//		It inherits CGEWeapon so that the player can hold it
//		The world and view model are set by Python (default to flag)
//
//   Created On: 11/11/09
//   Created By: Jonathan White <killermonkey>
////////////////////////////////////////////////////////////////////////////

#ifndef GE_GENERIC_TOKEN_H
#define GE_GENERIC_TOKEN_H

#include "ge_shareddefs.h"
#include "ge_weaponmelee.h"

#if defined( CLIENT_DLL )
	#define CGenericToken C_GenericToken
#endif

class CGenericToken : public CGEWeaponMelee
{
	DECLARE_CLASS( CGenericToken, CGEWeaponMelee );

public:
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CGenericToken( void );
	~CGenericToken( void );

	DECLARE_ACTTABLE();

	virtual void Spawn( void );
	virtual void Precache( void );

	virtual int	GetPosition( void ) const;
	virtual GEWeaponID GetWeaponID( void ) const { return WEAPON_TOKEN; }

	virtual void OnPickedUp( CBaseCombatCharacter *pNewOwner );
	virtual bool CanHolster( void );

	virtual void SecondaryAttack( void ) { };
	virtual float GetRange( void )	{	return	55.0f;	}

	virtual	void ImpactEffect( trace_t &trace );
	virtual float GetDamageForActivity( Activity hitActivity );

	virtual void AddViewKick( void );

	// For weapon switching to match our real classname with our entity (vice using the script file)
	virtual const char *GetName( void ) const;

#ifdef GAME_DLL
	virtual void PrimaryAttack( void );
	virtual bool CanEquip( CBaseCombatCharacter *pOther );

	virtual void SetTeamRestriction( int teamNum );
	virtual bool GetAllowSwitch()				{ return m_bAllowWeaponSwitch; }
	virtual void SetAllowSwitch( bool state )	{ m_bAllowWeaponSwitch = state; }
	virtual void SetWorldModel( const char *szWorld );
	virtual void SetViewModel( const char *szView );
	virtual void SetPrintName( const char *szPrintName );
	virtual void SetDamageModifier( float mod ) { m_flDmgMod = mod; };

	// Animation events
	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
	void HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
#endif

	virtual void SetViewModel();
	virtual const char	*GetViewModel( int viewmodelindex = 0 ) const;
	virtual const char	*GetWorldModel( void ) const;
	virtual char const	*GetPrintName( void ) const;

#ifdef CLIENT_DLL
	virtual const char *GetClassname();
#endif

private:
	CNetworkVar( int, m_iPosition );
	CNetworkVar( bool, m_bAllowWeaponSwitch );
	CNetworkVar( float, m_flDmgMod );
	CNetworkString( m_szViewModel, MAX_MODEL_PATH );
	CNetworkString( m_szWorldModel, MAX_MODEL_PATH );
	CNetworkString( m_szPrintName, 32 );
	CNetworkString( m_szRealClassname, 32 );
};

#endif