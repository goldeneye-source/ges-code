///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_tokenresource.cpp
// Description:
//      Manages gameplay defined tokens. Tokens are defined entities with set 
//		spawn limits, spawn locations, and other properties.
//
// Created On: 06 SEP 09
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"

#include "ammodef.h"

#include "ge_utils.h"
#include "gemp_gamerules.h"
#include "ge_generictoken.h"
#include "ge_loadoutmanager.h"

#include "ge_tokenmanager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// -------------------- //
//     Definitions      //
// -------------------- //
CGETokenDef::CGETokenDef()
{
	szClassName[0] = szModelName[0][0] = szModelName[1][0] = szPrintName[0] = '\0';
	iLimit = iTeam = 0;
	iLocations = CGETokenManager::LOC_NONE;
	flRespawnDelay = 10.0f;
	flDmgMod = 1.0f;
	nSkin = 0;
	glowColor.SetColor(0,0,0,0);
	glowDist = 350.0f;
	bAllowSwitch = true;
	bDirty = true;
}

CGECaptureAreaDef::CGECaptureAreaDef()
{
	Q_strcpy( szModelName, "models/gameplay/capturepoint.mdl" );
	szClassName[0] = rqdToken[0] = '\0';
	rqdTeam = TEAM_UNASSIGNED;
	nSkin = 0;
	glowColor.SetColor(0,0,0,0);
	glowDist = 350.0f;
	iLimit = 0;
	fRadius = 32.0f;
	fSpread = 500.0f;
	iLocations = CGETokenManager::LOC_NONE;
	bDirty = true;
}


// -------------------- //
//   CGETokenManager    //
// -------------------- //
CGETokenManager::CGETokenManager( void )
{
	m_flNextEnforcement = 0;
	m_flNextCapSpawn = 0;
}

CGETokenManager::~CGETokenManager( void )
{
	m_vTokenTypes.PurgeAndDeleteElements();
	m_vCaptureAreas.PurgeAndDeleteElements();
}

void CGETokenManager::Reset( void )
{
	// Remove all tracked entities
	RemoveTokens();
	ResetTokenSpawners();
	ClearGlobalAmmo();

	// Remove capture areas
	RemoveCaptureAreas();

	// Remove the definitions
	m_vTokenTypes.PurgeAndDeleteElements();
	m_vCaptureAreas.PurgeAndDeleteElements();

	// Reset our timers
	m_flNextEnforcement = 0;
	m_flNextCapSpawn = 0;
}

CGETokenDef* CGETokenManager::GetTokenDefForEdit( const char *szClassName )
{
	CGETokenDef *pDef = GetTokenDef( szClassName );
	if ( !pDef )
	{
		// Only allow certain types of token entities and make sure we can actually make this entity
		if ( !(Q_stristr(szClassName, "weapon_") || Q_stristr(szClassName, "token_")) || !CanCreateEntityClass( szClassName ) )
		{
			Warning( "Invalid Token Type Specified: %s\n", szClassName );
			return NULL;
		}

		pDef = new CGETokenDef;
		Q_strncpy( pDef->szClassName, szClassName, MAX_ENTITY_NAME );
		m_vTokenTypes.Insert( szClassName, pDef );

		// TODO: This should be handled by an interface
		// Notify our loadout manager in case we need to adjust the weapon set
		GEMPRules()->GetLoadoutManager()->OnTokenAdded( szClassName );
	}

	pDef->bDirty = true;
	return pDef;
}

void CGETokenManager::RemoveTokenDef( const char *szClassName )
{
	CGETokenDef *pDef = GetTokenDef( szClassName );
	if ( pDef )
	{
		// Remove latent global ammo if applicable
		RemoveGlobalAmmo( szClassName );
		// Remove tokens of this type from the world
		RemoveTokens( szClassName );
		// Remove the actual definition
		m_vTokenTypes.Remove( szClassName );
		delete pDef;
	}
}

void CGETokenManager::TransferToken( CGEWeapon *pToken, CGEPlayer *pNewOwner )
{
	if ( !pToken )
		return;

	CBaseCombatCharacter *pOldOwner = pToken->GetOwner();
	if ( !pOldOwner || pToken->IsMarkedForDeletion() )
		return;

	// Make sure we set the generic token to switch off so we can take the weapon
	CGenericToken *pGeneric = dynamic_cast<CGenericToken*>(pToken);
	bool allowSwitch = true;
	if ( pGeneric )
	{
		allowSwitch = pGeneric->GetAllowSwitch();
		pGeneric->SetAllowSwitch( true );
	}

	if ( pOldOwner->GetActiveWeapon() == pToken )
		pOldOwner->SwitchToNextBestWeapon( pToken );

	if ( pNewOwner )
	{
		// Detach the token from the old owner and "drop" it in Python
		pOldOwner->Weapon_Detach( pToken );
		OnTokenDropped( pToken, ToGEPlayer(pOldOwner) );

		// Give the token to the new owner. The "pickup" is handled in this call for us.
		pNewOwner->BumpWeapon( pToken );
	}
	else
	{
		pOldOwner->Weapon_Drop( pToken );
	}

	if ( pGeneric )
		pGeneric->SetAllowSwitch( allowSwitch );
}

void CGETokenManager::CaptureToken( CGEWeapon *pToken )
{
	// Remove the token from play, never adjust out limit
	RemoveTokenEnt( pToken, false );
}

CGECaptureAreaDef* CGETokenManager::GetCaptureAreaDefForEdit( const char *szName )
{
	CGECaptureAreaDef *ca = GetCapAreaDef( szName );
	if ( !ca )
	{
		ca = new CGECaptureAreaDef;
		Q_strncpy( ca->szClassName, szName, MAX_ENTITY_NAME );
		m_vCaptureAreas.Insert( szName, ca );
	}

	// Always mark us dirty
	ca->bDirty = true;

	return ca;
}

void CGETokenManager::RemoveCaptureAreaDef( const char *szName )
{
	CGECaptureAreaDef *ca = GetCapAreaDef( szName );
	if ( ca )
	{
		RemoveCaptureAreas( szName );
		m_vCaptureAreas.Remove( szName );
		delete ca;
	}
}

bool CGETokenManager::CanTouchCaptureArea( const char *szName, CBaseEntity *pEntity )
{
	CGECaptureAreaDef *ca = GetCapAreaDef( szName );
	CBaseCombatCharacter *pPlayer = pEntity->MyCombatCharacterPointer();

	if ( !ca || !pPlayer )
		return false;

	// Test for token ownership if required
	if ( ca->rqdToken[0] && !pPlayer->Weapon_OwnsThisType( ca->rqdToken ) )
		return false;

	// Test for teamplay membership if required
	if ( ca->rqdTeam != TEAM_UNASSIGNED && pPlayer->GetTeamNumber() != ca->rqdTeam )
		return false;

	return true;
}

const char *CGETokenManager::GetCaptureAreaToken( const char *szName )
{
	CGECaptureAreaDef *ca = GetCapAreaDef( szName );
	if ( ca )
		return ca->rqdToken;
	return NULL;
}

bool CGETokenManager::GiveGlobalAmmo( CBasePlayer *pPlayer )
{
	if ( !pPlayer )
		return false;

	int amount = 0;
	FOR_EACH_DICT( m_vGlobalAmmo, idx )
		amount += pPlayer->GiveAmmo( m_vGlobalAmmo[idx].amount, m_vGlobalAmmo[idx].aID, true );

	return (amount > 0);
}

void CGETokenManager::SetGlobalAmmo( const char *szClassName, int amount /*=-1*/ )
{
	const char *ammo = GetAmmoForWeapon( AliasToWeaponID( szClassName ) );
	if ( !ammo[0] )
		return;

	int ammoid = GetAmmoDef()->Index( ammo );
	if ( amount == -1 )
		amount = GetAmmoDef()->CrateAmount( ammoid ) / 2;

	int idx = m_vGlobalAmmo.Find( szClassName );
	if ( idx != m_vGlobalAmmo.InvalidIndex() )
	{
		m_vGlobalAmmo[idx].amount = amount;
	}
	else
	{
		idx = m_vGlobalAmmo.Insert( szClassName );
		m_vGlobalAmmo[idx].aID = ammoid;
		m_vGlobalAmmo[idx].amount = amount;
	}
}

void CGETokenManager::RemoveGlobalAmmo( const char *szClassName )
{
	m_vGlobalAmmo.Remove( szClassName );
}

void CGETokenManager::ClearGlobalAmmo( void )
{
	m_vGlobalAmmo.RemoveAll();
}

bool CGETokenManager::ShouldSpawnToken( const char *szClassName, CGESpawner *pSpawner )
{
	CGETokenDef *ttype = GetTokenDef( szClassName );
	if ( !ttype )
		return true;

	// If we defined us as "unlimited" then spawn it
	if ( ttype->iLimit == -1 )
		return true;

	// Check to see if we are missing a token(s)
	if ( ttype->vTokens.Count() < ttype->iLimit )
		return true;

	return false;
}

bool CGETokenManager::GetTokenRespawnDelay( const char *szClassName, float *flDelay )
{
	CGETokenDef *ttype = GetTokenDef(szClassName);
	if ( !ttype )
		return false;

	*flDelay = ttype->flRespawnDelay;
	return true;
}

void CGETokenManager::OnTokenSpawned( CGEWeapon *pToken )
{
	CGETokenDef *ttype = GetTokenDef( pToken->GetClassname() );
	if ( !ttype )
		return;

	// Checks to make sure we are a valid token
	CGESpawner *pSpawner = dynamic_cast<CGESpawner*>( pToken->GetOwnerEntity() );
	if ( pSpawner )
	{
		// Find our spawner
		int idx = ttype->vSpawners.Find( pSpawner->GetRefEHandle() );
		if ( idx != ttype->vSpawners.InvalidIndex() )
		{
			// Configure the token
			ApplyTokenSettings( ttype, pToken );

			// Add us to the list
			ttype->vTokens.AddToTail( pToken );
				
			// Tell the Python scenario
			GEGameplay()->GetScenario()->OnTokenSpawned( pToken );

			// We're done!
			return;
		}
	}

	// We are invalid, remove us and issue a warning
	RemoveTokenEnt( pToken, false );
	DevWarning( "[TknMgr] Token (%s) spawned outside of a valid spawner!\n", pToken->GetClassname() );	
}

void CGETokenManager::OnTokenRemoved( CGEWeapon *pToken )
{
	CGETokenDef *ttype = GetTokenDef( pToken->GetClassname() );
	if ( ttype )
	{
		// Notify the Python scenario
		GEGameplay()->GetScenario()->OnTokenRemoved( pToken );

		// Remove us from the list
		ttype->vTokens.FindAndRemove( pToken );
	}
}

void CGETokenManager::OnTokenPicked( CGEWeapon *pToken, CGEPlayer *pPlayer )
{
	const char *szClassName = pToken->GetClassname();
	if ( GetTokenDef(szClassName) )
		GEGameplay()->GetScenario()->OnTokenPicked( pToken, pPlayer );
}

void CGETokenManager::OnTokenDropped( CGEWeapon *pToken, CGEPlayer *pPlayer )
{
	const char *szClassName = pToken->GetClassname();
	if ( GetTokenDef(szClassName) )
		GEGameplay()->GetScenario()->OnTokenDropped( pToken, pPlayer );
}

void CGETokenManager::OnCaptureAreaSpawned( CGECaptureArea *pArea )
{
	CGECaptureAreaDef *ca = GetCapAreaDef( pArea->GetGroupName() );
	if ( ca )
	{
		ca->vAreas.AddToTail( pArea->GetRefEHandle() );
		GEGameplay()->GetScenario()->OnCaptureAreaSpawned( pArea );
	}
	else
	{
		Warning( "[TknMgr] Capture area spawned without an associated group!\n" );
	}
}

void CGETokenManager::OnCaptureAreaRemoved( CGECaptureArea *pArea )
{
	CGECaptureAreaDef *ca = GetCapAreaDef( pArea->GetGroupName() );
	if ( ca )
	{
		GEGameplay()->GetScenario()->OnCaptureAreaRemoved( pArea );

		for ( int i=0; i < ca->vAreas.Count(); i++ )
		{
			if ( ca->vAreas[i] == pArea )
			{
				ca->vAreas.Remove(i);
				return;
			}
		}
	}
}

void CGETokenManager::SpawnTokens( const char *name /*=NULL*/ )
{
	if ( !GEGameplay()->IsInRound() )
		return;

	FOR_EACH_DICT( m_vTokenTypes, idx )
	{
		// If no name is specified, we go through each token type
		if ( !name || !Q_stricmp(name, m_vTokenTypes.GetElementName(idx)) )
		{
			// Only find new spawners if we need to
			CGETokenDef *token = m_vTokenTypes[idx];
			if ( token->vSpawners.Count() < token->iLimit )
			{
				CUtlVector<EHANDLE> spawners;
				FindSpawnersForToken( token, spawners );

				int numToSpawn = token->iLimit - token->vTokens.Count();
				for ( int i=0; i < spawners.Count() && numToSpawn > 0; i++ )
				{
					CGESpawner *pSpawner = (CGESpawner*) spawners[i].Get();
					if ( pSpawner )
					{
						// Actually perform the override to spawn the token
						pSpawner->SetOverride( token->szClassName );
						numToSpawn--;

						// Add the spawner to our list
						token->vSpawners.AddToTail( pSpawner );
					}
				}

				if ( numToSpawn > 0 )
					DevWarning( "[TknMgr] Ran out of spawners for token type %s!\n", name );
			}

			// We are done here if we named a token def
			if ( name )
				break;
		}
	}
}

void CGETokenManager::RemoveTokens( const char *name /*=NULL*/, int count /*=-1*/ )
{
	FOR_EACH_DICT( m_vTokenTypes, idx )
	{
		// If no name is specified, we go through each token type
		if ( !name || !Q_stricmp(name, m_vTokenTypes.GetElementName(idx)) )
		{
			CGETokenDef *ttype = m_vTokenTypes[idx];
			// Avoid corruption of the list while iterating
			CUtlVector<EHANDLE> tokens;
			tokens.AddVectorToTail( ttype->vTokens );

			int toRemove = (count > 0) ? MIN( count, tokens.Count() ) : tokens.Count();
			for ( int i=0; i < toRemove; i++ )
			{
				// Issue a removal, limits and lists are adjusted elsewhere
				RemoveTokenEnt( (CGEWeapon*) tokens[i].Get(), false );
			}

			// Realign our spawner count based on our limit
			toRemove = ttype->vSpawners.Count() - ttype->iLimit;
			for ( int i=0; i < toRemove && ttype->vSpawners.Count(); i++ )
			{
				CGESpawner *pSpawner = ttype->vSpawners[0].Get();
				if ( pSpawner && pSpawner->IsEmpty() )
				{
					pSpawner->SetOverride( NULL );
					ttype->vSpawners.Remove( 0 );
				}
			}

			// TODO: Check if we wiped out our limit? If so remove global ammo...
			// We are done here if we named a token def
			if ( name )
				break;
		}
	}
}

void CGETokenManager::ResetTokenSpawners( const char *name /*=NULL*/, int count /*=-1*/ )
{
	FOR_EACH_DICT( m_vTokenTypes, idx )
	{
		// If no name is specified, we go through each token type
		if ( !name || !Q_stricmp(name, m_vTokenTypes.GetElementName(idx)) )
		{
			CGETokenDef *ttype = m_vTokenTypes[idx];

			// Calculate how many spawners to reset
			int toRemove;
			if ( count > 0 )
				toRemove = MIN( count, ttype->vSpawners.Count() );
			else
				toRemove = ttype->vSpawners.Count();

			// Perform the removal
			for ( int i=0; i < toRemove && ttype->vSpawners.Count(); i++ )
			{
				CGESpawner *pSpawner = ttype->vSpawners[0].Get();
				if ( pSpawner )
					pSpawner->SetOverride( NULL );
					
				ttype->vSpawners.FastRemove( 0 );
			}

			// Finally subtract from our token limit
			ttype->iLimit -= toRemove;
		}
	}
}

void CGETokenManager::RemoveTokenEnt( CGEWeapon *pToken, bool bAdjustLimit /*=true*/ )
{
	if ( !pToken )
		return;

	CGETokenDef *ttype = GetTokenDef( pToken->GetClassname() );
	if ( !ttype )
		return;
		
	// Detach from a player if they are holding us
	CBaseCombatCharacter *pOwner = pToken->GetOwner();
	if ( pOwner )
	{
		CGenericToken *pGeneric = dynamic_cast<CGenericToken*>( pToken );
		if ( pGeneric )
			pGeneric->SetAllowSwitch( true );

		if ( pOwner->GetActiveWeapon() == pToken )
			pOwner->SwitchToNextBestWeapon( pToken );

		pOwner->Weapon_Detach( pToken );
	}

	// Actually remove the token
	UTIL_Remove( pToken );

	// If we asked to adjust limit, do so and reset a spawner
	if ( bAdjustLimit )
	{
		ttype->iLimit--;

		// Reset the first empty spawner
		for ( int i=0; i < ttype->vSpawners.Count(); i++ )
		{
			CGESpawner *pSpawner = ttype->vSpawners[i].Get();
			if ( pSpawner && pSpawner->IsEmpty() )
			{
				pSpawner->SetOverride( NULL );
				ttype->vSpawners.Remove( i );
				break;
			}
		}
	}
}

void CGETokenManager::SpawnCaptureAreas( const char *name /*=NULL*/ )
{
	if ( !GEGameplay()->IsInRound() )
		return;

	FOR_EACH_DICT( m_vCaptureAreas, idx )
	{
		// If no name is specified, we go through each capture area
		if ( !name || !Q_stricmp(name, m_vCaptureAreas.GetElementName(idx)) )
		{
			CGECaptureAreaDef *ca = m_vCaptureAreas[idx];

			// Get the full list of available spawning places
			// this is based on our parameters set in Python
			CUtlVector<EHANDLE> spawners;
			FindSpawnersForCapArea( ca, spawners );

			// Spawn capture areas using the spawners gathered above
			int numToSpawn = ca->iLimit - ca->vAreas.Count();
			for ( int i=0; i < spawners.Count() && numToSpawn > 0; i++ )
			{
				CBaseEntity *pSpawner = spawners[i].Get();
				if ( pSpawner )
				{
					CGECaptureArea *pCapture = (CGECaptureArea*) CBaseEntity::CreateNoSpawn( "ge_capturearea", pSpawner->GetAbsOrigin(), vec3_angle );
					pCapture->SetGroupName( m_vCaptureAreas.GetElementName(idx) );
					ApplyCapAreaSettings( ca, pCapture );
					DispatchSpawn( pCapture );

					numToSpawn--;
				}
			}

			if ( numToSpawn > 0 )
				m_flNextCapSpawn = gpGlobals->curtime + 0.1;
			else if (m_flNextCapSpawn < gpGlobals->curtime)
				m_flNextCapSpawn = 0; // So make sure we don't continue remaking the capture points.

			// We are done if we named an area specifically
			if ( name )
				break;
		}
	}
}

void CGETokenManager::RemoveCaptureAreas( const char *name /*=NULL*/, int count /*= -1*/ )
{
	CGECaptureAreaDef *ca = GetCapAreaDef( name );
	if ( ca )
	{
		// We specified a token type, remove it's known capture points up to "count"
		if ( count < 0 || count >= ca->vAreas.Count() )
			count = ca->vAreas.Count();

		while ( count > 0 ) {
			// TODO: This errors out on Remove function.....
			int idx = GERandom<int>( ca->vAreas.Count() );
			ca->vAreas[idx]->Remove();
			count--;
		}
	}
	else
	{
		// Remove all the capture points from the world
		CBaseEntity *pCP = gEntList.FindEntityByClassname( NULL, "ge_capturearea" );
		while( pCP )
		{
			pCP->Remove();
			pCP = gEntList.FindEntityByClassname( pCP, "ge_capturearea" );
		}

		FOR_EACH_DICT( m_vCaptureAreas, idx )
			m_vCaptureAreas[idx]->vAreas.RemoveAll();
	}
}

// TODO: Min Token Spread??
void CGETokenManager::FindSpawnersForToken( CGETokenDef *token, CUtlVector<EHANDLE> &spawners )
{
	int locations	= token->iLocations;
	int team		= token->iTeam;

	int numToSpawn	= token->iLimit - token->vTokens.Count();
	if ( numToSpawn <= 0 )
		return;

	// Set our qualifiers
	bool bMyTeam		= (locations & LOC_MYTEAM)		? true : false;
	bool bOtherTeam		= (locations & LOC_OTHERTEAM)	? true : false;
	bool bSpecialOnly	= (locations & LOC_SPECIALONLY) ? true : false;

	// Swap our team to match our spawns
	if ( GEMPRules()->IsTeamSpawnSwapped() && team >= FIRST_GAME_TEAM )
		team = (team == TEAM_MI6) ? TEAM_JANUS : TEAM_MI6;

	Vector search_centroid(0);
	float search_extent = MAX_TRACE_LENGTH;

	// Gather stats on our desired team spawns
	if ( (bMyTeam || bOtherTeam) && team >= FIRST_GAME_TEAM )
	{
		// We desire to be close to a team so use our team's spawn info
		int desTeamSpawn;
		if ( bMyTeam )
			desTeamSpawn = (team == TEAM_MI6) ? SPAWN_PLAYER_MI6 : SPAWN_PLAYER_JANUS;
		else
			desTeamSpawn = (team == TEAM_MI6) ? SPAWN_PLAYER_JANUS : SPAWN_PLAYER_MI6;

		Vector mins, maxs;
		GEMPRules()->GetSpawnerStats( desTeamSpawn, mins, maxs, search_centroid );

		// Grow our search by 20%
		Vector extent = maxs - mins;
		if ( extent.x > extent.y )
			search_extent = abs(extent.x) * 1.2f;
		else
			search_extent = abs(extent.y) * 1.2f;
	}

	// Now go through each spawn type and find some candidates
	if ( locations & LOC_TOKEN )
	{
		// Check our team specific conditions
		if ( (bMyTeam || bOtherTeam) && team >= FIRST_GAME_TEAM )
		{
			int desTeamSpawn;
			if ( bMyTeam )
				desTeamSpawn = (team == TEAM_MI6) ? SPAWN_TOKEN_MI6 : SPAWN_TOKEN_JANUS;
			else
				desTeamSpawn = (team == TEAM_MI6) ? SPAWN_TOKEN_JANUS : SPAWN_TOKEN_MI6;

			numToSpawn -= FilterSpawnersForToken( desTeamSpawn, numToSpawn, spawners, false );
		}

		// Always add the generic one (if needed)
		numToSpawn -= FilterSpawnersForToken( SPAWN_TOKEN, numToSpawn, spawners, false, search_centroid, search_extent );
	}

	if ( locations & LOC_WEAPON )
		numToSpawn -= FilterSpawnersForToken( SPAWN_WEAPON, numToSpawn, spawners, bSpecialOnly, search_centroid, search_extent );

	// If we failed all other locations put them in an ammo spawner
	if ( numToSpawn > 0 )
		FilterSpawnersForToken( SPAWN_AMMO, numToSpawn, spawners, bSpecialOnly, search_centroid, search_extent );
}

// TODO: Use radius distance or bounding box?
// TODO: Use TokenDef?
int CGETokenManager::FilterSpawnersForToken( int spawner_type, int count, CUtlVector<EHANDLE> &out, bool special_only, Vector centroid /*=vec3_origin*/, float max_dist /*=MAX_TRACE_LENGTH*/ )
{
	// Sanity Checks
	if ( spawner_type < SPAWN_FIRST_GESPAWNER || spawner_type > SPAWN_LAST_GESPAWNER || count <= 0 )
		return 0;

	// Load our list of spawners
	CUtlVector<EHANDLE> spawns;
	spawns.AddVectorToTail( *GEMPRules()->GetSpawnersOfType( spawner_type ) );

	// Prune the list
	for ( int i=0; i < spawns.Count(); i++ )
	{
		CGESpawner *pSpawn = (CGESpawner*) spawns[i].Get();
		if ( pSpawn->IsOverridden() || (special_only && !pSpawn->IsSpecial()) || pSpawn->IsDisabled() || 
			(max_dist < MAX_TRACE_LENGTH && spawns[i]->GetAbsOrigin().DistTo(centroid) > max_dist) )
		{
			spawns.FastRemove( i-- );
		}
	}

	// If our mapper has marked any special weapon spawns, we want to use those instead of the high level weapon spawns.
	if (spawner_type == SPAWN_WEAPON && spawns.Count() > 1)
	{
		CUtlVector<EHANDLE> specialSpawns;

		for ( int i = 0; i < spawns.Count(); i++ )
		{
			CGESpawner *pSpawn = (CGESpawner*)spawns[i].Get();
			if (pSpawn->IsVerySpecial())
			{
				specialSpawns.AddToTail( pSpawn );
			}
		}

		// Just because we had more than one viable spawner doesn't mean any of them were marked by the mapper.
		// If we actually have any, wipe our spawns vector and replace it with the specialspawns vector.
		if (specialSpawns.Count() > 0)
		{
			spawns.RemoveAll();
			spawns.AddVectorToTail(specialSpawns);
		}
	}

	// Find spawners at random using whatevers left from the pruning
	int num_found = 0;

	while ( spawns.Count() && count > 0 )
	{
		int rnd = GERandom<int>( spawns.Count() );

		out.AddToTail( spawns[rnd] );
		spawns.FastRemove( rnd );

		count--;
		num_found++;
	}

	return num_found;
}

void CGETokenManager::FindSpawnersForCapArea( CGECaptureAreaDef *ca, CUtlVector<EHANDLE> &spawners )
{
	static float mincpdist = 1048576.0f;

	// Find the number of CP's we need to spawn
	int numToSpawn = ca->iLimit - ca->vAreas.Count();
	if ( numToSpawn <= 0 )
		return;

	int origDesired = numToSpawn;

	// Build list of existing areas
	CUtlVector<Vector> existing;
	FOR_EACH_DICT( m_vCaptureAreas, idx )
	{
		CGECaptureAreaDef *area = m_vCaptureAreas[idx];
		for ( int i=0; i < area->vAreas.Count(); i++ )
			existing.AddToTail( area->vAreas[i]->GetAbsOrigin() );
	}

	// Append our associated token locations (so we don't spawn too close!)
	CGETokenDef *token = GetTokenDef( ca->rqdToken );
	for ( int i=0; token && i < token->vSpawners.Count(); i++ )
	{
		if ( token->vSpawners[i].Get() )
			existing.AddToTail( token->vSpawners[i]->GetAbsOrigin() );
		else
			DevWarning( "Failed to get spawner instance...\n" );
	}

	// Teamplay considerations
	int team = ca->rqdTeam;
	int locations = ca->iLocations;

	bool bMyTeam	= (locations & LOC_MYTEAM)		? true : false;
	bool bOtherTeam	= (locations & LOC_OTHERTEAM)	? true : false;
	
	if ( GEMPRules()->IsTeamSpawnSwapped() && team >= FIRST_GAME_TEAM )
		team = (team == TEAM_MI6) ? TEAM_JANUS : TEAM_MI6;

	// Try to find defined capture area spawners first
	if ( locations & LOC_CAPAREA )
	{
		int teamspawnernum = 0;

		if ( (bMyTeam || bOtherTeam) && team >= FIRST_GAME_TEAM )
		{
			int desTeamSpawn;
			if ( bMyTeam )
				desTeamSpawn = (team == TEAM_MI6) ? SPAWN_CAPAREA_MI6 : SPAWN_CAPAREA_JANUS;
			else
				desTeamSpawn = (team == TEAM_MI6) ? SPAWN_CAPAREA_JANUS : SPAWN_CAPAREA_MI6;

			numToSpawn -= FilterSpawnersForCapArea( ca, desTeamSpawn, numToSpawn, existing, spawners );

			teamspawnernum = GEMPRules()->GetSpawnersOfType(desTeamSpawn)->Count();
			// We might have enough they're just not enabled yet.
			if (!m_flNextCapSpawn && teamspawnernum >= origDesired)
				return;
		}

		// Always add the generic one (if needed)
		numToSpawn -= FilterSpawnersForCapArea( ca, SPAWN_CAPAREA, numToSpawn, existing, spawners );

		// We might have enough they're just not enabled yet.
		if (!m_flNextCapSpawn && GEMPRules()->GetSpawnersOfType(SPAWN_CAPAREA)->Count() + teamspawnernum >= origDesired)
			return;
	}

	// Default to player spawns otherwise
	if ( numToSpawn > 0 )
	{
		if ( (bMyTeam || bOtherTeam) && team >= FIRST_GAME_TEAM )
		{
			int desTeamSpawn;
			if ( bMyTeam )
				desTeamSpawn = (team == TEAM_MI6) ? SPAWN_PLAYER_MI6 : SPAWN_PLAYER_JANUS;
			else
				desTeamSpawn = (team == TEAM_MI6) ? SPAWN_PLAYER_JANUS : SPAWN_PLAYER_MI6;

			numToSpawn -= FilterSpawnersForCapArea( ca, desTeamSpawn, numToSpawn, existing, spawners );
		}

		// Finally try normal player spawns
		numToSpawn -= FilterSpawnersForCapArea( ca, SPAWN_PLAYER, numToSpawn, existing, spawners );
	}

	// Failsafe to spawn at least ONE, but we will refuse to overlap another spawner
	if ( ca->vAreas.Count() == 0 && numToSpawn )
	{
		CUtlVector<EHANDLE> temp;
		temp.AddVectorToTail( *GEMPRules()->GetSpawnersOfType(SPAWN_PLAYER) );
		for ( int i=0; i < temp.Count(); i++ )
		{
			for ( int k=0; k < existing.Count(); k++ )
			{
				// Don't spawn within 50 units of an existing area
				if ( existing[k].DistToSqr( temp[i]->GetAbsOrigin() ) > 2500 )
				{
					spawners.AddToTail( temp[i] );
					return;
				}
			}
		}
	}
}

int CGETokenManager::FilterSpawnersForCapArea( CGECaptureAreaDef *ca, int spawner_type, int count, CUtlVector<Vector> &existing, CUtlVector<EHANDLE> &chosen )
{
	// Sanity Checks
	if ( spawner_type < SPAWN_FIRST_POINTENT || spawner_type > SPAWN_LAST_POINTENT || count <= 0 )
		return 0;

	// Load our list of spawners
	CUtlVector<EHANDLE> spawns;
	spawns.AddVectorToTail( *GEMPRules()->GetSpawnersOfType( spawner_type ) );

	// Check for disabled spawns
	for ( int i = 0; i < spawns.Count(); i++ )
	{
		CGESpawner *pSpawn = dynamic_cast<CGESpawner*>( spawns[i].Get() );

		if ( pSpawn && pSpawn->IsDisabled() )
		{
			spawns.FastRemove( i-- );
			continue;
		}
	}

	// Maintain minimum spread from other areas and tokens
	for ( int i=0; i < existing.Count(); i++ )
	{
		for ( int k=0; k < spawns.Count(); k++ )
		{
			if ( existing[i].DistTo( spawns[k]->GetAbsOrigin() ) < ca->fSpread )
				spawns.FastRemove( k-- );
		}
	}

	// Now choose random spots up to our count
	int num_found = 0;
	while ( spawns.Count() && count > 0 )
	{
		// Choose a random spot
		int rnd = GERandom<int>( spawns.Count() );

		// Add the chosen to our list and mark it's location for subsequent searches
		chosen.AddToTail( spawns[rnd] );
		existing.AddToTail( spawns[rnd]->GetAbsOrigin() );
		
		// Remove this choice from our pool
		spawns.FastRemove( rnd );

		count--;
		num_found++;
	}

	// Return how many spots we were able to find
	return num_found;
}

void CGETokenManager::EnforceTokens( void )
{
	if ( gpGlobals->curtime < m_flNextEnforcement || !GEGameplay()->IsInRound() )
		return;

	// This will pretty much -never- be called, but needs to exist for maps that disable/enable areas at round start.
	if (m_flNextCapSpawn && gpGlobals->curtime > m_flNextCapSpawn && GEGameplay()->IsInRound())
	{
		RemoveCaptureAreas();
		SpawnCaptureAreas();
	}

	FOR_EACH_DICT( m_vTokenTypes, idx )
	{
		CGETokenDef *pDef = m_vTokenTypes[idx];

		// Update known token locations
		pDef->vTokens.RemoveAll();
		FindTokens( pDef->szClassName, pDef->vTokens );

		// Check dirty flag
		if ( pDef->bDirty )
		{
			// Apply new settings to all existing tokens
			ApplyTokenSettings( pDef );
			pDef->bDirty = false;
		}

		// Always make sure our known spawner list is valid
		for( int k=0; k < pDef->vSpawners.Count(); k++ )
		{
			CGESpawner *pSpawner = pDef->vSpawners[k].Get();
			if ( !pSpawner || pSpawner->IsDisabled() )
			{
				if ( pSpawner )
					pSpawner->SetOverride( NULL );

				pDef->vSpawners.Remove(k--);
			}
		}

		// Adjust our token amounts if needed
		int diff = pDef->iLimit - pDef->vTokens.Count();
		if ( diff > 0 )
			SpawnTokens( pDef->szClassName );
		else if ( diff < 0 )
			RemoveTokens( pDef->szClassName, abs(diff) );
	}

	FOR_EACH_DICT( m_vCaptureAreas, idx )
	{
		CGECaptureAreaDef *pDef = m_vCaptureAreas[idx];
		if ( pDef->bDirty )
		{
			ApplyCapAreaSettings( pDef );
			pDef->bDirty = false;

			// Adjust capture area amounts if needed
			int diff = pDef->iLimit - pDef->vAreas.Count();
			if ( diff > 0 )
				SpawnCaptureAreas( pDef->szClassName );
			else if ( diff < 0 )
				RemoveCaptureAreas( pDef->szClassName, abs(diff) );
		}
	}

	m_flNextEnforcement = gpGlobals->curtime + 0.25;
}

void CGETokenManager::ApplyTokenSettings( CGETokenDef *ttype, CGEWeapon *pToken /*=NULL*/ )
{
	if ( !ttype )
		return;

	CUtlVector<EHANDLE> tokens;

	if ( !pToken )
		tokens.AddVectorToTail( ttype->vTokens );
	else
		tokens.AddToTail( pToken );

	for ( int i=0; i < tokens.Count(); i++ )
	{
		CGEWeapon *pWeapon = (CGEWeapon*) tokens[i].Get();
		if ( pWeapon )
		{
			// Always set the glow
			bool bGlow = ttype->glowColor.a() > 0;
			pWeapon->SetupGlow( bGlow, ttype->glowColor, ttype->glowDist );
		}

		CGenericToken *pGenToken = dynamic_cast<CGenericToken*>( pWeapon );
		if ( pGenToken )
		{
			if ( !pGenToken->GetOwner() )
			{
				// Set the view model
				if ( ttype->szModelName[0][0] != '\0' )
					pGenToken->SetViewModel( ttype->szModelName[0] );

				// Set the world model
				if ( ttype->szModelName[1][0] != '\0' )
					pGenToken->SetWorldModel( ttype->szModelName[1] );
			}
			
			// Set the print name
			if ( ttype->szPrintName[0] != '\0' )
				pGenToken->SetPrintName( ttype->szPrintName );

			pGenToken->SetAllowSwitch( ttype->bAllowSwitch );
			pGenToken->SetTeamRestriction( ttype->iTeam );

			pGenToken->SetDamageModifier( ttype->flDmgMod );
			if (ttype->nSkin != -1)
				pGenToken->SetSkin( ttype->nSkin );
		}
	}
}

void CGETokenManager::ApplyCapAreaSettings( CGECaptureAreaDef *ca, CGECaptureArea *pArea /*=NULL*/ )
{
	if ( !ca )
		return;

	CUtlVector< CHandle<CGECaptureArea> > areas;

	if ( !pArea )
		areas.AddVectorToTail( ca->vAreas );
	else
		areas.AddToTail( pArea );

	for ( int i=0; i < areas.Count(); i++ )
	{
		CGECaptureArea *pCapture = areas[i].Get();
		if ( pCapture )
		{
			// Always set the glow
			bool bGlow = ca->glowColor.a() > 0;
			pCapture->SetupGlow( bGlow, ca->glowColor, ca->glowDist );
			pCapture->SetModel( ca->szModelName );
			pCapture->SetRadius( ca->fRadius );
			pCapture->ChangeTeam( ca->rqdTeam );
			pCapture->m_nSkin = ca->nSkin;
		}
	}
}

void CGETokenManager::FindTokens( const char *szClassName, CUtlVector<EHANDLE> &tokens )
{
	tokens.RemoveAll();

	CGETokenDef *pDef = GetTokenDef( szClassName );
	if ( !pDef )
		return;

	CBaseEntity *pEnt = gEntList.FindEntityByClassname( NULL, szClassName );
	while ( pEnt )
	{
		tokens.AddToTail( pEnt );
		pEnt = gEntList.FindEntityByClassname( pEnt, szClassName );
	}
}

CGETokenDef *CGETokenManager::GetTokenDef( const char *szClassName )
{
	int idx = m_vTokenTypes.Find( szClassName );
	if ( idx != m_vTokenTypes.InvalidIndex() )
		return m_vTokenTypes[idx];

	return NULL;
}

CGECaptureAreaDef *CGETokenManager::GetCapAreaDef( const char *szName )
{
	int idx = m_vCaptureAreas.Find( szName );
	if ( idx != m_vCaptureAreas.InvalidIndex() )
		return m_vCaptureAreas[idx];

	return NULL;
}

