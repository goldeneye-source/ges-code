///////////// Copyright © 2006, Killer Monkey. All rights reserved. /////////////
// 
// File: ge_character_data.cpp
// Description:
//      see ge_character_data.h
//
// Created On: 9/16/2008
// Created By: Killer Monkey (original by Scott Lloyd)
/////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include "ge_character_data.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Define our global accessor to the characters
CGECharacters g_GECharacters;

const CGECharacters *GECharacters()
{
	return &g_GECharacters;
}


CGECharData::CGECharData()
{
	// Null out all our strings to make sure we don't print gibberish
	iTeam = TEAM_UNASSIGNED;
	iWeight = 0;
	szIdentifier[0] = '\0';
	szPrintName[0]	= '\0';
	szShortName[0]	= '\0';
	szBio[0]		= '\0';
}

CGECharData::~CGECharData()
{
	m_pSkins.PurgeAndDeleteElements();
}

GECharSkin::GECharSkin()
{
	szIdent[0] = szModel[0] = szHatModel[0] = szPreviewImg[0] = '\0';
	iWorldSkin = iViewSkin = 0;
}


// BEGIN CHARACTER CONTAINER CLASS
CGECharacters::CGECharacters()
{
	m_pRandomChar = NULL;
}

CGECharacters::~CGECharacters()
{
	m_pCharacters.PurgeAndDeleteElements();
}

//Get a specific element.
const CGECharData *CGECharacters::Get( int idx ) const
{
	if( !m_pCharacters.IsValidIndex(idx) )
		return NULL;

	return m_pCharacters[idx];
}

//Find us an element based on the character ident
const CGECharData *CGECharacters::Get( const char *szIdent ) const
{
	for (int i=0; i< m_pCharacters.Count(); i++)
	{
		if ( !Q_stricmp(m_pCharacters[i]->szIdentifier, szIdent) )
			return m_pCharacters[i];
	}
	return NULL;
}

const CGECharData *CGECharacters::GetFirst( int team ) const
{
	for (int i=0; i< m_pCharacters.Count(); i++)
	{
		const CGECharData *pChar = m_pCharacters[i];
		if ( (team == TEAM_UNASSIGNED || pChar->iTeam == team) && pChar->iWeight > 0 && pChar->m_pSkins.Count() > 0 )
			return pChar;
	}

	return NULL;
}

const CGECharData *CGECharacters::GetRandomChar() const
{
	return m_pRandomChar;
}

void CGECharacters::GetTeamMembers( int team, CUtlVector<const CGECharData*> &members ) const
{
	if ( !m_pCharacters.Count() )
		return;

	members.RemoveAll();

	// Add our sorted list next, filter for team
	for ( int i=0; i < m_pCharacters.Count(); i++ )
	{
		if ( team == TEAM_UNASSIGNED || m_pCharacters[i]->iTeam == team )
			members.AddToTail( m_pCharacters[i] );
	}
}

int CGECharacters::GetIndex( const CGECharData *pChar ) const
{
	for (int i=0; i < m_pCharacters.Count(); i++)
	{
		if ( pChar == m_pCharacters[i] )
			return i;
	}
	return -1;
}

int CGECharacters::GetIndex( const char *szIdent ) const
{
	for (int i=0; i < m_pCharacters.Count(); i++)
	{
		if ( !Q_stricmp(m_pCharacters[i]->szIdentifier, szIdent) )
			return i;
	}
	return -1;
}

CGECharData *CGECharacters::Add( const char *szIdent )
{
	if ( !szIdent )
		return NULL;

	CGECharData *pChar = new CGECharData;
	Q_strncpy( pChar->szIdentifier, szIdent, MAX_CHAR_IDENT );

	int idx = m_pCharacters.AddToTail( pChar );

	return m_pCharacters[idx];
}

void CGECharacters::CreateRandomCharacter()
{
	if ( m_pCharacters.Count() > 0 )
		return;

	if ( m_pRandomChar )
		delete m_pRandomChar;

	m_pRandomChar = new CGECharData;
	Q_strncpy( m_pRandomChar->szIdentifier, CHAR_RANDOM_IDENT, MAX_CHAR_IDENT );
	Q_strcpy( m_pRandomChar->szPrintName, "#GE_RANDOM_CHAR" );
	Q_strcpy( m_pRandomChar->szBio, "#GE_CharBio_Random" );
	m_pRandomChar->iWeight = -1;
	m_pRandomChar->iTeam = -1;
	m_pRandomChar->m_pSkins.AddToTail( new GECharSkin );
}

int GECharSort( CGECharData * const *a, CGECharData * const *b )
{
	// Highest weight to the top
	return (*b)->iWeight - (*a)->iWeight;
}

void CGECharacters::Sort()
{
	m_pCharacters.Sort( GECharSort );
}

//--------------------------------------------------------------------------
//
// CGECharacterParser - Handles parsing all the character data
//
CGECharacterDataParser GECharacterDataParser;

void GECharacters_Precache( void *pUser )
{
	// This only runs once
	if ( !GECharacterDataParser.HasBeenParsed() )
	{
		g_GECharacters.CreateRandomCharacter();
		GECharacterDataParser.InitParser("scripts/character/*.X");
		g_GECharacters.Sort();
	}

	// Always precache the models
	for ( int i=0; i < GECharacters()->Count(); i++ )
	{
		const CGECharData *pChar = GECharacters()->Get( i );
		for ( int k=0; pChar && k < pChar->m_pSkins.Count(); k++ )
		{
			if ( pChar->m_pSkins[k]->szModel[0] != '\0' )
				CBaseEntity::PrecacheModel( pChar->m_pSkins[k]->szModel );
			if ( pChar->m_pSkins[k]->szHatModel[0] != '\0' )
				CBaseEntity::PrecacheModel( pChar->m_pSkins[k]->szHatModel );
		}
	}
}
PRECACHE_REGISTER_FN( GECharacters_Precache );

void CGECharacterDataParser::Parse( KeyValues *pKeyValuesData, const char *szFileWithoutEXT )
{
	CGECharData *pChar = g_GECharacters.Add( szFileWithoutEXT );

	Q_strncpy( pChar->szPrintName,	pKeyValuesData->GetString("printname",		"#GE_UNNAMED"),	MAX_CHAR_IDENT );
	Q_strncpy( pChar->szShortName,	pKeyValuesData->GetString("shortname",		"#GE_UNNAMED"),	MAX_CHAR_NAME  );
	Q_strncpy( pChar->szBio,		pKeyValuesData->GetString("bio",			""			 ),	MAX_CHAR_BIO   );

	// Load our weight
	pChar->iWeight = pKeyValuesData->GetInt( "weight" );

	// Affiliate them with a team
	const char *team = pKeyValuesData->GetString( "team", "unassigned" );
	if ( !Q_stricmp(team, "mi6") )
		pChar->iTeam = TEAM_MI6;
	else if ( !Q_stricmp(team, "janus") )
		pChar->iTeam = TEAM_JANUS;
	else
		pChar->iTeam = TEAM_UNASSIGNED;

	const char *gender = pKeyValuesData->GetString( "gender", "m" );
	if ( gender[0] == 'f' )
		pChar->iGender = CHAR_GENDER_FEMALE;
	else
		pChar->iGender = CHAR_GENDER_MALE;

	// Load our skins in
	KeyValues *pSkinValues = pKeyValuesData->FindKey( "skins" );
	if ( pSkinValues )
	{
		KeyValues *pSkinData = pSkinValues->GetFirstSubKey();
		for( int i=0; i < MAX_CHAR_SKINS; i++ )
		{
			if( pSkinData == NULL )
				break;

			GECharSkin *skin = new GECharSkin;

			Q_strncpy( skin->szIdent,		pSkinData->GetName(),								MAX_CHAR_IDENT );
			Q_strncpy( skin->szPreviewImg,	pSkinData->GetString("previewimg", "unknown"	 ),	MAX_CHAR_NAME  );
			Q_strncpy( skin->szModel,		pSkinData->GetString("model",	   ""			 ),	MAX_MODEL_PATH );
			Q_strncpy( skin->szHatModel,	pSkinData->GetString("hatmodel",   ""			 ),	MAX_MODEL_PATH );
			
			skin->iViewSkin	 = pSkinData->GetInt( "viewmodelskin",  0 );
			skin->iWorldSkin = pSkinData->GetInt( "worldmodelskin", 0 );

			// TODO: We need to error check this input....
			pChar->m_pSkins.AddToTail( skin );

			pSkinData = pSkinData->GetNextKey();
		}
	}
}
