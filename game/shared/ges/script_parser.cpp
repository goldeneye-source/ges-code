//////////  Copyright © 2011, Goldeneye Source. All rights reserved. ///////////
// 
// File: script_parser.cpp
// Description:
//      See Header
//
// Created On: 7/15/2011
// Created By: Killermonkey
////////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include "filesystem.h"
#include "script_parser.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define FILE_PATH_MAX_LENGTH 256

CScriptParser::CScriptParser()
{
	m_bAlert = true;
	m_bParsed = false;
}

//
// This method starts the parser that will either load one file
//	or many based on pszFilePath.
// Input: pszFilePath - FS path to KeyValue file(s) for parsing.
//						extentions are defined using .X
//
void CScriptParser::InitParser( const char *pszPath, bool bAllowNonEncryptedSearch /*=true*/, bool bAllowEncryptedSearch /*=true*/ )
{
	// If you hit this assert, check to see where your instancing 
	// CScriptParser, make sure it isn't before the filesystem inits
	Assert( filesystem );
	Assert( pszPath );

	// Have we already been parsed?
	// TODO: Maybe just issue a DevWarning here instead of bailing
	if( m_bParsed )
		return;

	// Copy the file path to tweak the extensions and fix it up
	char szFilePath[FILE_PATH_MAX_LENGTH];
	V_FixupPathName( szFilePath, FILE_PATH_MAX_LENGTH, pszPath );

	//Search for unencrypted files
	if( bAllowNonEncryptedSearch )
	{
		Q_SetExtension( szFilePath, GetNonEncryptedEXT(), FILE_PATH_MAX_LENGTH );
		SearchForFiles( szFilePath );
	}

	//Search for encrypted files
	if( bAllowEncryptedSearch )
	{
		Q_SetExtension( szFilePath, GetEncryptedEXT(), FILE_PATH_MAX_LENGTH );
		SearchForFiles( szFilePath );
	}

	m_bParsed = true;
}

void CScriptParser::SearchForFiles( const char *szWildcardPath )
{
	char filePath[FILE_PATH_MAX_LENGTH];
	char basePath[FILE_PATH_MAX_LENGTH];
	Q_strncpy( basePath, szWildcardPath, FILE_PATH_MAX_LENGTH );
	V_StripFilename( basePath );

	FileFindHandle_t findHandle;
	const char *fileName = filesystem->FindFirstEx( szWildcardPath, GetFSSearchPath(), &findHandle );
	
	while ( fileName != NULL )
	{
		Q_ComposeFileName( basePath, fileName, filePath, FILE_PATH_MAX_LENGTH );

		if( !ParseFile( filePath ) )
		{
			if ( m_bAlert )				
				DevWarning( "[script_parser] Unable to parse '%s'!\n", filePath );
		}

		fileName = filesystem->FindNext( findHandle );
	}

	filesystem->FindClose( findHandle );
}

//
// This method handles parsing a single file, it can be called
//  many times if a wildcard path was passed to InitParser.
// Input: pszFilePath - FS path to a KeyValue file for parsing.
//		  bWildcard - Is this file the only one to be parsed? or will more come...
//
bool CScriptParser::ParseFile( const char *pszFilePath )
{
	// Load the file into a buffer (null-terminated)
	char *buffer = (char*) UTIL_LoadFileForMe( pszFilePath, NULL );
	if ( !buffer )
		return false;

	// If we are encrypted, decrypt us
	const char *fileName = Q_UnqualifiedFileName( pszFilePath );
	if( CompareExtensions( fileName, GetEncryptedEXT() ) )
		UTIL_DecodeICE( (unsigned char*)buffer, Q_strlen(buffer), g_pGameRules->GetEncryptionKey() );

	// Remove the extension
	char fileNameNoExt[64];
	Q_StripExtension( fileName, fileNameNoExt, 64 );

	KeyValues *pKV = new KeyValues( fileNameNoExt );
	if( !pKV->LoadFromBuffer( fileNameNoExt, buffer ) )
	{
		pKV->deleteThis();
		delete [] buffer;
		return false;
	}

	Parse( pKV, fileNameNoExt );

	pKV->deleteThis();
	delete[] buffer;
	return true;
}

//
// Takes in pszPath, and compares the extension on it to pszExt
// if they match, returns true, otherwise false.
//
bool CScriptParser::CompareExtensions( const char *pszPath, const char *pszExt )
{
	if ( !pszPath || !pszExt )
		return false;

	const int pathLen = Q_strlen( pszPath );
	const int extLen = Q_strlen( pszExt );

	if ( extLen >= pathLen )
		return false;

	const char *end = pszPath + pathLen - extLen;
	return Q_stricmp( end, pszExt ) == 0;
}
