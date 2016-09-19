///////////// Copyright © 2009 GoldenEye: Source. All rights reserved. /////////////
//
//   Project     : Server (GES)
//   File        : ge_generictoken.cpp
//   Description :
//      [SEE HEADER]
//
//   Created On: 11/11/09
//   Created By: Jonathan White <killermonkey>
////////////////////////////////////////////////////////////////////////////

#include "cbase.h"

#if defined( CLIENT_DLL )
	#include "c_ge_player.h"
#else
	#include "ai_basenpc.h"
	#include "ge_player.h"
#endif

#include "npcevent.h"
#include "gemp_gamerules.h"
#include "ge_generictoken.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef GAME_DLL

static CUtlDict<int, int> *g_SlotPositions = 0;
void initTokenSlotPositions()
{
	if ( !g_SlotPositions )
	{
		g_SlotPositions = new CUtlDict<int, int>();
		g_SlotPositions->Insert( "token_mi6", 0 );
		g_SlotPositions->Insert( "token_janus", 1 );
		g_SlotPositions->Insert( "token_deathmatch", 2 );
		g_SlotPositions->Insert( "token_custom1", 3 );
		g_SlotPositions->Insert( "token_custom2", 4 );
		g_SlotPositions->Insert( "token_custom3", 5 );
		g_SlotPositions->Insert( "token_custom4", 6 );
	}
}

#endif

IMPLEMENT_NETWORKCLASS_ALIASED( GenericToken, DT_GenericToken )

BEGIN_NETWORK_TABLE( CGenericToken, DT_GenericToken )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO(m_bAllowWeaponSwitch) ),
	RecvPropFloat( RECVINFO(m_flDmgMod) ),
	RecvPropString( RECVINFO( m_szWorldModel ) ),
	RecvPropString( RECVINFO( m_szViewModel ) ),
	RecvPropString( RECVINFO( m_szPrintName ) ),
	RecvPropString( RECVINFO( m_szRealClassname ) ),
	RecvPropInt( RECVINFO( m_iPosition ) ),
#else
	SendPropBool( SENDINFO(m_bAllowWeaponSwitch) ),
	SendPropFloat( SENDINFO(m_flDmgMod) ),
	SendPropString( SENDINFO( m_szWorldModel ) ),
	SendPropString( SENDINFO( m_szViewModel ) ),
	SendPropString( SENDINFO( m_szPrintName ) ),
	SendPropString( SENDINFO( m_szRealClassname ) ),
	SendPropInt( SENDINFO( m_iPosition ), 4 ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CGenericToken )
	DEFINE_PRED_FIELD( m_bAllowWeaponSwitch, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

// Declare a bunch of different entity names so the token manager can distinguish
// between the token types when assigning models and other things
LINK_ENTITY_TO_CLASS( token_mi6, CGenericToken );
LINK_ENTITY_TO_CLASS( token_janus, CGenericToken );
LINK_ENTITY_TO_CLASS( token_deathmatch, CGenericToken );
LINK_ENTITY_TO_CLASS( token_custom1, CGenericToken );
LINK_ENTITY_TO_CLASS( token_custom2, CGenericToken );
LINK_ENTITY_TO_CLASS( token_custom3, CGenericToken );
LINK_ENTITY_TO_CLASS( token_custom4, CGenericToken );

acttable_t CGenericToken::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				ACT_GES_IDLE_KNIFE,						false },
	{ ACT_MP_CROUCH_IDLE,				ACT_HL2MP_IDLE_CROUCH_MELEE,			false },

	{ ACT_MP_RUN,						ACT_GES_RUN_KNIFE,						false },
	{ ACT_MP_WALK,						ACT_GES_WALK_KNIFE,						false },
	{ ACT_MP_CROUCHWALK,				ACT_HL2MP_WALK_CROUCH_MELEE,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_KNIFE,		false },
	{ ACT_GES_ATTACK_RUN_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_KNIFE,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	ACT_GES_GESTURE_RANGE_ATTACK_KNIFE,		false },

	{ ACT_GES_DRAW,						ACT_GES_GESTURE_DRAW_KNIFE,				false },

	{ ACT_MP_JUMP,						ACT_GES_JUMP_KNIFE,						false },
	{ ACT_GES_CJUMP,					ACT_GES_CJUMP_KNIFE,					false },
};
IMPLEMENT_ACTTABLE( CGenericToken );

CGenericToken::CGenericToken( void )
{
#ifdef GAME_DLL
	ChangeTeam( TEAM_UNASSIGNED );
#endif

	m_iPosition = 4;
	m_flDmgMod = 1.0f;
	m_bAllowWeaponSwitch = true;
	m_szWorldModel.GetForModify()[0] = '\0';
	m_szViewModel.GetForModify()[0] = '\0';
	m_szPrintName.GetForModify()[0] = '\0';
	m_szRealClassname.GetForModify()[0] = '\0';
}

CGenericToken::~CGenericToken( void )
{
}

void CGenericToken::Spawn( void )
{
	BaseClass::Spawn();

	#ifdef GAME_DLL
		// Store our real classname for the client
		Q_strncpy( m_szRealClassname.GetForModify(), GetClassname(), 32 );

		// Fix bucket position
		initTokenSlotPositions();
		int idx = g_SlotPositions->Find( GetClassname() );
		if ( g_SlotPositions->IsValidIndex( idx ) )
			m_iPosition = g_SlotPositions->Element( idx );
	#endif
}

void CGenericToken::Precache( void )
{
	// We override the baseclass precache since we have many 
	// classnames but only one script file
	bool bAllowPrecache = BaseClass::IsPrecacheAllowed();
	BaseClass::SetAllowPrecache( true );

	m_iPrimaryAmmoType = m_iSecondaryAmmoType = -1;

	// Add this weapon to the weapon registry, and get our index into it
	// Get weapon data from script file
	if ( ReadWeaponDataFromFileForSlot( filesystem, "weapon_generictoken", &m_hWeaponFileInfo, GetEncryptionKey() ) )
	{
#if defined( CLIENT_DLL )
		gWR.LoadWeaponSprites( GetWeaponFileInfoHandle() );
#endif

		// Precache models (preload to avoid hitch)
		// These are the default models for a generic token (the GoldenEye Key)
		m_iViewModelIndex = 0;
		m_iWorldModelIndex = 0;
		if ( GetViewModel() && GetViewModel()[0] )
		{
			m_iViewModelIndex = CBaseEntity::PrecacheModel( GetViewModel() );
		}
		if ( GetWorldModel() && GetWorldModel()[0] )
		{
			m_iWorldModelIndex = CBaseEntity::PrecacheModel( GetWorldModel() );
		}

		// Precache sounds, too
		for ( int i = 0; i < NUM_SHOOT_SOUND_TYPES; ++i )
		{
			const char *shootsound = GetShootSound( i );
			if ( shootsound && shootsound[0] )
			{
				CBaseEntity::PrecacheScriptSound( shootsound );
			}
		}
	}
	else
	{
		// Couldn't read data file, remove myself
		Warning( "Error reading weapon data file for: weapon_generictoken!\n" );
	}

	BaseClass::SetAllowPrecache( bAllowPrecache );
}

int CGenericToken::GetPosition() const
{
	return m_iPosition;
}

void CGenericToken::OnPickedUp( CBaseCombatCharacter *pNewOwner )
{
	BaseClass::OnPickedUp( pNewOwner );

	// If we are forced to hold the token switch to it
	if ( !m_bAllowWeaponSwitch )
		pNewOwner->Weapon_Switch( this );
}

bool CGenericToken::CanHolster( void )
{
	if ( !m_bAllowWeaponSwitch )
		return false;

	return BaseClass::CanHolster();
}

float CGenericToken::GetDamageForActivity( Activity hitActivity )
{
	return GetGEWpnData().m_iDamage * m_flDmgMod;
}

const char *CGenericToken::GetName( void ) const
{
	return STRING(m_iClassname);
}

#ifdef GAME_DLL
void CGenericToken::PrimaryAttack( void )
{
	BaseClass::PrimaryAttack();

	CGEPlayer *pOwner = ToGEPlayer( GetOwner() );
	if ( !pOwner )
		return;

	Vector swingStart = pOwner->Weapon_ShootPosition( );
	Vector forward;
	pOwner->EyeVectors( &forward );

	GEGameplay()->GetScenario()->OnTokenAttack( this, pOwner, swingStart, forward );
}

bool CGenericToken::CanEquip( CBaseCombatCharacter *pOther )
{
	// Don't allow multiple pickups of the same token
	if ( pOther->Weapon_OwnsThisType( GetClassname()) )
		return false;

	// If we are a teamplay token don't allow opposite team to pick us up
	if ( GEMPRules()->IsTeamplay() && GetTeamNumber() > LAST_SHARED_TEAM )
	{
		if ( GetTeamNumber() == pOther->GetTeamNumber() )
		{
		    // On the same team, allow pickup
			return true;
		}
		else
		{
			GEGameplay()->GetScenario()->OnEnemyTokenTouched(this, ToGEPlayer(pOther));
			return false;
		}
    }
    
	return true;
}

void CGenericToken::SetTeamRestriction( int teamNum )
{
	if ( teamNum >= 0 && teamNum < MAX_GE_TEAMS )
		ChangeTeam( teamNum );
}

void CGenericToken::SetViewModel(const char *szView)
{
	bool bAllowPrecache = BaseClass::IsPrecacheAllowed();
	BaseClass::SetAllowPrecache( true );

	m_iViewModelIndex = BaseClass::PrecacheModel( szView );
	Q_strncpy( m_szViewModel.GetForModify(), szView, MAX_MODEL_PATH );

	BaseClass::SetAllowPrecache( bAllowPrecache );
}

void CGenericToken::SetWorldModel( const char *szWorld )
{
	bool bAllowPrecache = BaseClass::IsPrecacheAllowed();
	BaseClass::SetAllowPrecache( true );

	m_iWorldModelIndex = BaseClass::PrecacheModel( szWorld );
	Q_strncpy( m_szWorldModel.GetForModify(), szWorld, MAX_MODEL_PATH );

	VPhysicsDestroyObject();

	SetModel( szWorld);
	
	VPhysicsInitNormal( SOLID_VPHYSICS, GetSolidFlags(), false );
	SetMoveType( MOVETYPE_VPHYSICS );
	SetCollisionGroup( COLLISION_GROUP_WEAPON );

	BaseClass::SetAllowPrecache( bAllowPrecache );
}

void CGenericToken::SetPrintName( const char *szPrintName )
{
	Q_strncpy( m_szPrintName.GetForModify(), szPrintName, 32 );
}

void CGenericToken::HandleAnimEventMeleeHit( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	// Trace up or down based on where the enemy is...
	// But only if we're basically facing that direction
	Vector vecDirection;
	AngleVectors( GetAbsAngles(), &vecDirection );

	Vector vecEnd;
	VectorMA( pOperator->Weapon_ShootPosition(), 50, vecDirection, vecEnd );
	CBaseEntity *pHurt = pOperator->CheckTraceHullAttack( pOperator->Weapon_ShootPosition(), vecEnd, Vector(-16,-16,-16), Vector(36,36,36), GetGEWpnData().m_iDamage, DMG_CLUB );
	
	// did I hit someone?
	if ( pHurt )
	{
		// play sound
		WeaponSound( MELEE_HIT );

		// Fake a trace impact, so the effects work out like a player's crowbaw
		trace_t traceHit;
		UTIL_TraceLine( pOperator->Weapon_ShootPosition(), pHurt->GetAbsOrigin(), MASK_SHOT_HULL, pOperator, COLLISION_GROUP_NONE, &traceHit );
		ImpactEffect( traceHit );
	}
	else
	{
		WeaponSound( MELEE_MISS );
	}
}

void CGenericToken::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
	case EVENT_WEAPON_AR2:
		Swing( false );
		break;

	case EVENT_WEAPON_MELEE_HIT:
		HandleAnimEventMeleeHit( pEvent, pOperator );
		break;

	default:
		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
	}
}

#endif

//-----------------------------------------------------------------------------
// Purpose: Add in a view kick for this weapon
//-----------------------------------------------------------------------------
void CGenericToken::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle punchAng;

	punchAng.x = random->RandomFloat( 1.0f, 2.0f );
	punchAng.y = random->RandomFloat( -2.0f, -1.0f );
	punchAng.z = 0.0f;
	
	pPlayer->ViewPunch( punchAng ); 
}

void CGenericToken::ImpactEffect( trace_t &traceHit )
{
	return; // NIEN!!!!

	// See if we hit water (we don't do the other impact effects in this case)
	if ( ImpactWater( traceHit.startpos, traceHit.endpos ) )
		return;

	if ( traceHit.m_pEnt && traceHit.m_pEnt->MyCombatCharacterPointer() )
		return;

	UTIL_ImpactTrace( &traceHit, DMG_SLASH );
}

void CGenericToken::SetViewModel()
{
	BaseClass::SetViewModel();
	SetSkin( m_nSkin );
}

const char *CGenericToken::GetViewModel( int viewmodelindex /*=0*/ ) const
{
	if ( m_szViewModel.Get()[0] )
		return m_szViewModel.Get();

	return GetWpnData().szViewModel;
}

const char *CGenericToken::GetWorldModel( void ) const
{
	if ( m_szWorldModel.Get()[0] )
		return m_szWorldModel.Get();

	return GetWpnData().szWorldModel;
}

const char *CGenericToken::GetPrintName( void ) const
{
	if ( m_szPrintName.Get()[0] )
		return m_szPrintName.Get();

	return BaseClass::GetPrintName();
}

#ifdef CLIENT_DLL
const char *CGenericToken::GetClassname()
{
	if ( m_szRealClassname[0] )
		return m_szRealClassname;
	else
		return BaseClass::GetClassname();
}
#endif
