///////////// Copyright © 2006, Killer Monkey. All rights reserved. /////////////
// 
// File: ge_character_data.h
// Description:
//      Stores and acts on the character data
//
// Created On: 9/16/2008
// Created By: Killer Monkey (original by Scott Lloyd)
/////////////////////////////////////////////////////////////////////////////
#ifndef GE_CHARACTER_DATA_H
#define GE_CHARACTER_DATA_H

#include "ge_shareddefs.h"
#include "script_parser.h"

#define MAX_CHAR_BIO	256
#define MAX_CHAR_NAME	32
#define MAX_CHAR_IDENT	32
#define MAX_CHAR_SKINS	4

#define CHAR_RANDOM_IDENT "_random"
#define CHAR_GENDER_MALE	1
#define CHAR_GENDER_FEMALE	2

struct GECharSkin
{
	GECharSkin();

	// Skin Identity
	char szIdent[MAX_CHAR_IDENT];
	// Small preview image for Character Select
	char szPreviewImg[MAX_CHAR_NAME];
	// Player Model
	char szModel[MAX_MODEL_PATH];
	// Hat Model
	char szHatModel[MAX_MODEL_PATH];
	//World Model Skin
	int	 iWorldSkin;
	//View Model Skin
	int	 iViewSkin;
};

struct CGECharData
{
	CGECharData();
	~CGECharData();

	// Identifier
	char szIdentifier[MAX_CHAR_IDENT];
	// Name for Character Select
	char szPrintName[MAX_CHAR_NAME];
	// Name for Scoreboard, After Action, etc.
	char szShortName[MAX_CHAR_NAME];
	// Character's Team
	int  iTeam;
	// Character's Weight (used for sorting)
	int	 iWeight;
	// Gender identification
	int	 iGender;

	// Biography info for Character Select
	char szBio[MAX_CHAR_BIO];

	// Our skins
	CUtlVector<GECharSkin*> m_pSkins;
};

class CGECharacters
{
public:
	CGECharacters();
	~CGECharacters();

	//Get a specific element.
	const CGECharData *Get( int idx ) const;
	//Use an identifier to find some character's data
	const CGECharData *Get( const char *szIdent ) const;
	// Get the first valid character on a particular team
	const CGECharData *GetFirst( int team ) const;
	// Get the random character instance
	const CGECharData *GetRandomChar() const;

	// Get a list of all the members on a specified team
	void GetTeamMembers( int team, CUtlVector<const CGECharData*> &members ) const;
	
	//Use an identifier to get the corresponding index (useful for passing integers instead of strings)
	int GetIndex( const char *szIdent ) const;
	int GetIndex( const CGECharData *pChar )  const;

	int Count( void ) const { return m_pCharacters.Count(); };

	// This is used by the parser to add a new character to the list
	CGECharData *Add( const char *szIdent );

	// Creates the internal random character (only once!)
	void CreateRandomCharacter();

	// Sort the list based on weight
	void Sort();

private:
	CGECharData *m_pRandomChar;
	CUtlVector<CGECharData*> m_pCharacters;
};

class CGECharacterDataParser : public CScriptParser
{
protected:
	void Parse( KeyValues *pKeyValuesData, const char *szFileWithoutEXT );
};

extern const CGECharacters *GECharacters();

#endif //GE_CHARACTER_DATA_H
