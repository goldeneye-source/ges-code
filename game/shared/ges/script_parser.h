//////////  Copyright © 2011, Goldeneye Source. All rights reserved. ///////////
// 
// File: script_parser.h
// Description:
//      Generic parsing class that searches a directory for files to parse into
//		KeyValue sets. Must be extended to use by implemented Parse(...)
//
// Created On: 7/15/2011
// Created By: Killermonkey
////////////////////////////////////////////////////////////////////////////////
#ifndef SCRIPT_PARSE_H
#define SCRIPT_PARSE_H
#ifdef _WIN32
#pragma once
#endif

class CScriptParser
{
public:
	CScriptParser();

	//Parser Methods to initially open and setup the KeyValues
	void InitParser( const char *pszPath, bool bAllowNonEncryptedSearch = true, bool bAllowEncryptedSearch = true );
	
	bool HasBeenParsed() { return m_bParsed; }
	void SetHasBeenParsed( bool p ) { m_bParsed = p; }

	// You can override these if you need to...
	virtual const char *GetPlaceHolderEXT()		{ return ".X"; }
	virtual const char *GetNonEncryptedEXT()	{ return ".txt"; }
	virtual const char *GetEncryptedEXT()		{ return ".ctx"; }

	// Search path used by IFileSystem
	virtual const char *GetFSSearchPath() { return "MOD"; };

protected:
	//You will need to set this up in your own class.
	virtual void Parse( KeyValues *pKeyValuesData, const char *szFileWithoutEXT ) = 0;

private:
	void SearchForFiles( const char *szWildcardPath );
	bool ParseFile( const char *pszFilePath );

	// Compare extentions
	bool CompareExtensions( const char *pszPath, const char *pszExt );
	
	bool m_bParsed;
	bool m_bAlert;
};

#endif //SCRIPT_PARSE_H

