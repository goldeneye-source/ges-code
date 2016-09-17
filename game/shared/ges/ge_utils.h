///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_utils.h
// Description:
//      Various utilities that valve "forgot" to make
//
// Created On: 08 Sep 08
// Created By: Jonathan White <killermonkey> 
/////////////////////////////////////////////////////////////////////////////'
#ifndef GE_UTILS_H
#define GE_UTILS_H

#include "mathlib/IceKey.H"
#include "filesystem.h"

// Encrypt/Decrypt using ICE
void GEUTIL_EncryptICE( unsigned char * buffer, int size, const unsigned char *key );
bool GEUTIL_DEncryptFile(const char* filename, const unsigned char* key, bool bEncrypt = true, bool bChangeExt = false);

// Get the secret hash we use for encryption
const unsigned char *GEUTIL_GetSecretHash();

// Override a Console Command, smartly
void GEUTIL_OverrideCommand( const char *real_name, const char *new_name, const char *override_name, const char *override_desc = "" );

// String utils
void ClearStringVector( CUtlVector<char*> &vec );
void UTIL_ColorToString( Color col, char *str, int size );

// Skips spaces and "data" in a given string starting at offset
// TODO: Candidate for deletion
const char *Q_SkipSpaces( const char *in, int offset = 0 );
const char *Q_SkipData( const char *in, int offset = 0 );

// Extract data from a given string into the provided vector using Q_SkipSpaces above
// TODO: Candidate for deletion
int Q_ExtractData( const char *in, CUtlVector<char*> &out );

#ifdef CLIENT_DLL

// Draw a sprite in 3D space
// offset.x = forward, offset.y = right, offset.z = up
void GEUTIL_DrawSprite3D( IMaterial *pMaterial, Vector offset, float width, float height, int alpha = 255 );

// Returns true on successful parsing, false if it isn't localizable or fails parsing
void GEUTIL_ParseLocalization( wchar_t *out, int size, const char *input );

// VGUI Helper, calculates the width and height of the passed in text
void GEUTIL_GetTextSize( const wchar_t *utext, vgui::HFont font, int &wide, int &tall );
void GEUTIL_GetTextSize( const char *text, vgui::HFont font, int &wide, int &tall );

// Get the current gameplay name, appends "(MOD)" when appropriate
wchar_t *GEUTIL_GetGameplayName( wchar_t *out, int byte_size );

// Encrypt/Decrypt using some dinky method I threw together because I actually don't do any encryption stuff.
uint64 GEUTIL_GetUniqueSkinData( int steamhash );
void GEUTIL_WriteUniqueSkinData( uint64 value, int steamhash );

uint64 GEUTIL_EventCodeToSkin( int code );
#else

#include "ge_player.h"
// Shared python functions that can be used in C++ as well
void pyShowPopupHelp( CGEPlayer *pPlayer, const char *title, const char *msg, const char *img = "", float holdtime = 5.0f, bool canArchive = false );
void pyShowPopupHelpTeam( int team, const char *title, const char *msg, const char *img = NULL, float holdtime = 5.0f, bool canArchive = false );

// Removes cut off multibyte characters from the end of a name and stores the new string in pOutputName.
// Returns true if cleanup was neccecery.
bool GEUTIL_CleanupNameEnding(const char* pName, char* pOutputName);
#endif

// Color hints (^a, ^1, etc.)
bool GEUTIL_IsValidColorHint( int ch );
char *GEUTIL_RemoveColorHints( char *src );
wchar_t *GEUTIL_RemoveColorHints( wchar_t *src );

void GEUTIL_StripWhitespace( char *str );

// Delays removal of the given entity by the given time in seconds
// it hides the entity from everyone until removal time
void GEUTIL_DelayRemove( CBaseEntity *pEnt, float delay );

// Get the current version of GES as defined in version.txt
bool GEUTIL_GetVersion( string_t &text, int &major, int &minor, int &client, int &server );

// Calculates the MD5 Sum of the given string (defined in ge_md5util.cpp)
void GEUTIL_MD5( const char *str, char *out, int out_len );

#endif
