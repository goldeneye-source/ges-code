#include "cbase.h"
#include "mathlib/IceKey.H"
#include "filesystem.h"
#include "utlbuffer.h"
#include "gamestringpool.h"
#include "ge_shareddefs.h"

#ifdef CLIENT_DLL
	#include <vgui/VGUI.h>
	#include <vgui/ILocalize.h>
	#include <vgui/ISurface.h>
	#include "vgui_controls/controls.h"
	#include "c_ge_gameplayresource.h"
	#include "view.h"
	#include <string>
	#include <sstream>
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define GES_ENABLE_MEMDEBUG 0

void GEUTIL_EncryptICE( unsigned char * buffer, int size, const unsigned char *key )
{
	if (!key)
		return;

	//Lets start the ice goodies!
	IceKey ice( 0 ); // level 0 = 64bit (8B) key
	ice.set( key ); // set key

	int blockSize = ice.blockSize();

	unsigned char *temp = (unsigned char *)_alloca( PAD_NUMBER( size, blockSize ) );
	unsigned char *p1 = buffer;
	unsigned char *p2 = temp;

	// encrypt data in 8 byte blocks
	int bytesLeft = size;

	while ( bytesLeft >= blockSize )
	{
		ice.encrypt( p1, p2 );

		bytesLeft -= blockSize;
		p1+=blockSize;
		p2+=blockSize;
	}

	// copy encrypted data back to original buffer
	Q_memcpy( buffer, temp, size-bytesLeft );
}

bool GEUTIL_DEncryptFile(const char* filename, const unsigned char* key, bool bEncrypt = true, bool bChangeExt = false)
{
	//Open up the file and load it into memory!
	FileHandle_t fileHandle = filesystem->Open( filename, "rb", "MOD" );
	if(!fileHandle)
		return false;

	// load file into a null-terminated buffer
	int fileSize = filesystem->Size(fileHandle);

	unsigned char *pBuff; // Holds the input
	unsigned char *pOutBuff; //Holds the output goodies

	// allocate memory to contain the whole file.
	pBuff = (unsigned char*) malloc (fileSize);
	pOutBuff = (unsigned char*) malloc (fileSize);

	if (pBuff == NULL || pOutBuff == NULL)
	{
		filesystem->Close( fileHandle );
        return false;
	}

	// copy the file into the buffer.
	filesystem->Read( pBuff, fileSize, fileHandle ); // read into local buffer
	
	//clean the output buffer
	memset( pOutBuff, NULL, fileSize );

	filesystem->Close( fileHandle );

	//Lets start the ice goodies!
	IceKey ice( 0 ); // level 0 = 64bit (8B) key
	ice.set( key ); // set key

	int blockSize = ice.blockSize();

	unsigned char *p1 = pBuff;
	unsigned char *p2 = pOutBuff;

	// encrypt data in 8 byte blocks
	int bytesLeft = fileSize;

	while ( bytesLeft >= blockSize )
	{
		if ( bEncrypt )
			ice.encrypt( p1, p2 );
		else
			ice.decrypt( p1, p2 );

		bytesLeft -= blockSize;
		p1+=blockSize;
		p2+=blockSize;
	}

	//The end chunk doesn't get an encryption?  that sux...
	memcpy( p2, p1, bytesLeft );

	fileHandle = filesystem->Open( filename, "wb", "MOD" );
	if ( !fileHandle )
		return false;

	filesystem->Write( pOutBuff, fileSize, fileHandle );
	filesystem->Close( fileHandle );

	free(pBuff);
	free(pOutBuff);

	return true;
}

void GEUTIL_OverrideCommand( const char *real_name, const char *new_name, const char *override_name, const char *override_desc /* = "" */ )
{
	// Swap the specified command with our own
	ConCommandBase *_realMapCommand = dynamic_cast< ConCommandBase* >( g_pCVar->FindCommand( real_name ) );
	ConCommandBase *_overMapCommand = dynamic_cast< ConCommandBase* >( g_pCVar->FindCommand( override_name ) );

	if ( !_realMapCommand )
		return;
	if ( !_overMapCommand )
		return;

	_realMapCommand->Shutdown();
	_realMapCommand->Create( new_name, "", FCVAR_HIDDEN );       //Tony; create and hide it from the list.
	_realMapCommand->Init();

	_overMapCommand->Shutdown();
	_overMapCommand->Create( real_name, override_desc );
	_overMapCommand->Init();
}

void UTIL_ColorToString( Color col, char *str, int size )
{
	int r,g,b,a;
	col.GetColor( r, g, b, a );

	Q_snprintf( str, size, "%d %d %d %d", r, g, b, a );
}

void ClearStringVector( CUtlVector<char*> &vec )
{
	for ( int i=vec.Count()-1; i >= 0; i-- )
		delete [] vec[i];
	vec.RemoveAll();
}

const char *Q_SkipSpaces( const char *in, int offset = 0 )
{
	const char *x = in + offset;
	while ( *x )
	{
		if ( *x == ' ' || *x == '\t' || *x == '\r' )
			x++;
		else
			return x;
	}

	// No more data
	return x;
}

const char *Q_SkipData( const char *in, int offset = 0 )
{
	const char *x = in + offset;
	while ( *x )
	{
		if ( *x != ' ' && *x != '\t' )
			x++;
		else
			return x;
	}
	
	// No more spaces
	return x;
}

int Q_ExtractData( const char *in, CUtlVector<char*> &out )
{
	#pragma warning( disable : 4309 )

	int cnt = 0;
	const char *x = Q_SkipSpaces( in );
	const char *y = Q_SkipData( x );
	char *data = NULL;

	while ( *x )
	{
		int len = (int)(y-x) + 1;
		data = new char[len+1];
		Q_strncpy( data, x, len );
		data[len] = '/0';

		out.AddToTail( data );
		cnt++;
	
		x = Q_SkipSpaces( y );
		y = Q_SkipData( x );		
	}

	return cnt;
}

#ifdef CLIENT_DLL

void GEUTIL_DrawSprite3D( IMaterial *pMaterial, Vector offset, float width, float height )
{
	// Make sure we are allowed to access our orientation
	Assert( IsCurrentViewAccessAllowed() );

	// Vectors for drawing
	Vector vForward = CurrentViewForward();
	Vector vUp = CurrentViewUp();
	Vector vRight = CurrentViewRight();

	vRight.z = 0;
	VectorNormalize( vRight );

	// Move the camera origin out the desired offset
	Vector vOrigin = CurrentViewOrigin();

	VectorMA( vOrigin, offset.x, vForward, vOrigin );
	VectorMA( vOrigin, offset.y, vRight, vOrigin );
	VectorMA( vOrigin, offset.z, vUp, vOrigin );

	// Cut width/height in half
	width = width / 2.0f;
	height = height / 2.0f;

	// Figure out our bounds
	Vector left = vRight * -width;
	Vector right = vRight * width;
	Vector top = vUp * height;
	Vector bottom = vUp * -height;

	CMatRenderContextPtr pRenderContext( materials );
	pRenderContext->Bind( pMaterial );

	IMesh *pMesh = pRenderContext->GetDynamicMesh();
	CMeshBuilder meshBuilder;
	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.TexCoord2f( 0,0,0 );
	meshBuilder.Position3fv( (vOrigin + left + top).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.TexCoord2f( 0,1,0 );
	meshBuilder.Position3fv( (vOrigin + right + top).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.TexCoord2f( 0,1,1 );
	meshBuilder.Position3fv( (vOrigin + right + bottom).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( 255, 255, 255, 255 );
	meshBuilder.TexCoord2f( 0,0,1 );
	meshBuilder.Position3fv( (vOrigin + left + bottom).Base() );
	meshBuilder.AdvanceVertex();
	meshBuilder.End();
	pMesh->Draw();
}

void GEUTIL_ReplaceKeyBindings( const wchar_t *in, wchar_t *out, int size )
{
	if ( !in || !out )
		return;

	if ( in[0] != L'\0' )
	{
		wchar_t *tmpOut = new wchar_t[size];
		UTIL_ReplaceKeyBindings( in, 0, tmpOut, sizeof(tmpOut) );
		wcsncpy( out, tmpOut, size );
		delete [] tmpOut;
	}
	else if ( in != out )
	{
		wcsncpy( out, in, size );
	}
}

bool GEUTIL_IsValidTokenChar( const char c )
{
	// 0-9, A-Z, a-z, _
	if ( (c >= 48 && c <= 57) || (c >= 65 && c <= 90) || (c >= 97 && c <= 122) || c == 95 )
		return true;
	return false;
}

void GEUTIL_Split( std::string s, char delim, CUtlVector<std::string> &elems )
{
	std::string::size_type pos = s.find( delim );
	if ( pos == s.npos )
		elems.AddToTail( s );
	else
		elems.AddToTail( s.substr( 0, pos ) );

	while ( pos != s.npos )
	{
		// Hop over our last find
		pos += 1;

		// Find the next entry, or the end of the string
		std::string::size_type next_pos = s.find( delim, pos );
		
		if ( next_pos != s.npos )
			// If we didn't hit the end copy the portion in between
			elems.AddToTail( s.substr( pos, next_pos-pos ) );
		else
			// This means we hit the end of the string
			elems.AddToTail( s.substr( pos ) );

		// Move forward
		pos = next_pos;
	}
}

void GEUTIL_ParseInternalLocalization( wchar_t *out, int size, const char *in, const char *trigger = "##" )
{
	std::string input( in );
	std::wstring output;

	std::string::size_type pos = input.find( trigger );
	std::string::size_type last_pos = pos;

	// Copy the first part of the input into our output, or duck out
	if ( pos == input.npos )
		output.assign( input.begin(), input.end() );
	else
		output.assign( input.begin(), input.begin() + pos );

	while ( pos != input.npos )
	{
		// We found a start token, now find the end
		std::string::size_type tkn_start = pos + 1;
		std::string::size_type tkn_size = 1;

		for ( std::string::iterator itr = input.begin() + tkn_start + 1; itr != input.end(); itr++ )
		{
			if ( !GEUTIL_IsValidTokenChar( *itr ) )
				break;
			++tkn_size;
		}

		std::string token = input.substr( tkn_start, tkn_size );
		if ( token[0] == '#' )
		{
			wchar_t *result = g_pVGuiLocalize->Find( token.c_str() );
			if ( result )
				output.append( result );
			else
				output.append( input.begin() + tkn_start - 1, input.begin() + tkn_start + tkn_size );
		}
		else
		{
			output.append( input.begin() + tkn_start - 1, input.begin() + tkn_start + tkn_size );
		}

		// Find our next token
		last_pos = tkn_start + tkn_size;
		pos = input.find( trigger, last_pos );

		// If there are no more tokens, copy the rest of the input into the output
		if ( pos == input.npos )
			output.append( input.begin() + last_pos, input.end() );
		else
			output.append( input.begin() + last_pos, input.begin() + pos );
	}

	// Final copy
	wcsncpy( out, output.c_str(), size );
}

void GEUTIL_ParseLocalization( wchar_t *out, int size, const char *input )
{
	static wchar_t* tkn_names[] = { L"%s1", L"%s2", L"%s3", L"%s4", L"%s5", L"%s6", L"%s7", L"%s8", L"%s9", L"%s10" };
	static int max_tkn_count = 10;

	// Default is direct input -> output passthrough
	Q_strtowcs( input, -1, out, size );

	if ( !input || Q_strlen(input) < 2 )
		return;

	// Check for non-localization or the trigger as the first portion
	if ( input[0] != '#' || ( input[0] == '#' && input[1] == '#' ) )
	{
		// Parse internal localization (GES Specific!), looking for ## triggers
		GEUTIL_ParseInternalLocalization( out, size, input );
		// Replace the key bindings
		GEUTIL_ReplaceKeyBindings( out, out, size );
		return;
	}

	// First parse out our information
	CUtlVector<std::string> entries;
	GEUTIL_Split( input, '\r', entries );
	// If we don't have entries, then we are done (this should be impossible at this point)
	if ( !entries.Count() )
		return;

	// Convert the base and remove it as a token
	wchar_t *found = g_pVGuiLocalize->Find( entries[0].c_str() );
	entries.Remove( 0 );

	if ( found && entries.Count() )
	{
		std::wstring finalstr( found );
		std::wstring wcstoken;

		int tkn_count = min( entries.Count(), max_tkn_count );
		for ( int i=0; i < tkn_count; i++ )
		{
			// Resolve our token entry, NOTE: the copy to std::string prevents a crash
			std::string token( entries[i] );
			wcstoken.assign( token.begin(), token.end() );

			// If we are localizable, give it a go
			if ( wcstoken[0] == L'#' )
			{
				wchar_t *wTkn = g_pVGuiLocalize->Find( entries[i].c_str() );
				if ( wTkn )
					wcstoken.assign( wTkn );
			}
			
			// Find the corresponding predefined token in the localization
			std::string::size_type pos = finalstr.find( tkn_names[i] );
			if ( pos == finalstr.npos )
				break;

			// Replace the token with our entry
			finalstr.replace( pos, wcslen(tkn_names[i]), wcstoken );
		}

		// Finally copy our built string into our output
		wcsncpy( out, finalstr.c_str(), size );
	}
	else if ( found )
	{
		// We don't have any tokens, copy the localized text to the output
		wcsncpy( out, found, size );
	}

	// Finally parse out key bindings
	GEUTIL_ReplaceKeyBindings( out, out, size );
}

void GEUTIL_GetTextSize( const wchar_t *utext, vgui::HFont font, int &wide, int &tall )
{
	wide = 0;
	tall = 0;
	int maxWide = 0;
	const wchar_t *text = utext;

	if ( font == vgui::INVALID_FONT )
		return;

	// For height, use the remapped font
	int fontHeight = vgui::surface()->GetFontTall( font );
	tall = fontHeight;

	int textLen = wcslen(text);
	for ( int i = 0; i < textLen; i++ )
	{
		// handle stupid special characters, these should be removed
		if ( text[i] == '&' && text[i + 1] != 0 )
			i++;

		int a, b, c;
		vgui::surface()->GetCharABCwide( font, text[i], a, b, c );
		wide += (a + b + c);

		if ( text[i] == '\n' )
		{
			tall += fontHeight;
			if ( wide > maxWide ) 
				maxWide = wide;

			// new line, wide is reset...
			wide = 0;
		}
	}

	// maxWide only gets set if a newline is in the label
	if ( wide < maxWide )
		wide = maxWide;
}

void GEUTIL_GetTextSize( const char *text, vgui::HFont font, int &wide, int &tall )
{
	wchar_t utext[2048];
	GEUTIL_ParseLocalization( utext, 2048, text );
	GEUTIL_GetTextSize( utext, font, wide, tall );
}

wchar_t *GEUTIL_GetGameplayName( wchar_t *out, int byte_size )
{
	if ( !GEGameplayRes() )
		return out;

	wchar_t *found = g_pVGuiLocalize->Find( GEGameplayRes()->GetGameplayName() );
	if ( found )
		Q_wcsncpy( out, found, byte_size );
	else
		g_pVGuiLocalize->ConvertANSIToUnicode( GEGameplayRes()->GetGameplayName(), out, byte_size );

	if ( !GEGameplayRes()->GetGameplayOfficial() )
		wcsncat( out, L" (MOD)", byte_size );

	return out;
}
#endif

// Valid hints are 0->9, a->z, |
bool GEUTIL_IsValidColorHint( int ch )
{
	if ( (ch >= 48 && ch <= 57) || (ch >= 97 && ch <= 122) || ch == 124 )
		return true;
	else
		return false;
}

char *GEUTIL_RemoveColorHints( char *str )
{
	if ( !str ) return NULL;

	char *out = str;
	for ( char *in = str; *in != 0; ++in )
	{
		if (*in == '^')
		{
			if ( GEUTIL_IsValidColorHint(*(in+1)) )
				in = in+2;	// Skip ^X
		}

		*out = *in;
		++out;
	}
	*out = 0;

	return str;
}

wchar_t *GEUTIL_RemoveColorHints( wchar_t *str )
{
	if ( !str ) return NULL;

	wchar_t *out = str;
	for ( wchar_t *in = str; *in != 0; ++in )
	{
		if (*in == L'^')
		{
			if ( GEUTIL_IsValidColorHint(*(in+1)) )
				in = in+2;	// Skip ^X
		}

		*out = *in;
		++out;
	}
	*out = 0;

	return str;
}

void GEUTIL_DelayRemove( CBaseEntity *pEnt, float delay )
{
	if ( !pEnt )
		return;

	pEnt->m_takedamage = DAMAGE_NO;
	pEnt->SetModelName( NULL_STRING );
	// Hide us from view
	pEnt->AddSolidFlags( FSOLID_NOT_SOLID );
	pEnt->SetSolid( SOLID_NONE );
	pEnt->AddEffects( EF_NODRAW );
	// Tell us to remove ourselfs "delay" seconds from now
	pEnt->SetThink( &CBaseEntity::SUB_Remove );
	pEnt->SetNextThink( gpGlobals->curtime + delay );
	pEnt->SetTouch(NULL);
	
	pEnt->SetAbsVelocity( vec3_origin );
}

bool GEUTIL_GetVersion( string_t &text, int &major, int &minor, int &client, int &server )
{
	KeyValues *kv = new KeyValues( "ges_version" );
	if ( kv->LoadFromFile( filesystem, "version.txt", "MOD" ) )
	{
		text = AllocPooledString( kv->GetString( "text", "invalid_version" ) );
		major = kv->GetInt( "major", 0 );
		minor = kv->GetInt( "minor", 0 );
		client = kv->GetInt( "client", 0 );
		server = kv->GetInt( "server", 0 );

		kv->deleteThis();
		return true;
	}
	else
	{
		text = MAKE_STRING("invalid_version");
		major = minor = client = server = 0;

		kv->deleteThis();
		return false;
	}
}

// Taken from http://www.partow.net/programming/hashfunctions/index.html
// Author: Arash Partow - 2002
unsigned int RSHash(const char *str)
{
   unsigned int b    = 378551;
   unsigned int a    = 63689;
   unsigned int hash = 0;

   for(size_t i = 0; i < (size_t)Q_strlen(str); i++)
   {
      hash = hash * a + str[i];
      a    = a * b;
   }

   return hash;
}

#ifdef GAME_DLL
bool IsOnList( int listnum, const unsigned int hash )
{
	switch ( listnum ) {
	case LIST_DEVELOPERS:
		return vDevsHash.Find(hash) != -1;
	case LIST_TESTERS:
		return vTestersHash.Find(hash) != -1;
	case LIST_BANNED:
		return vBannedHash.Find(hash) != -1;
	}

	return false;
}

// These are extern'd in shareddefs.h
CUtlVector<unsigned int> vDevsHash;
CUtlVector<unsigned int> vTestersHash;
CUtlVector<unsigned int> vBannedHash;

void InitStatusLists( void )
{
	int i = 0;
	while ( _vDevsHash[i] != HASHLIST_END )
	{
		vDevsHash.AddToTail( _vDevsHash[i] );
		i++;
	}

	i = 0;
	while ( _vTestersHash[i] != HASHLIST_END )
	{
		vTestersHash.AddToTail( _vTestersHash[i] );
		i++;
	}
}

void UpdateStatusLists( const char *data )
{
	KeyValues *kv = new KeyValues("hashes");
	if ( !kv->LoadFromBuffer( "hashes", data ) )
	{
		Warning( "Failed Status Check: KeyValues Corrupted!\n" );
		kv->deleteThis();
		return;
	}

	// Parse Developers
	KeyValues *hash = kv->GetFirstSubKey();
	while ( hash )
	{
		unsigned int uHash = strtoul( hash->GetString(), NULL, 0 );

		if ( !Q_stricmp("dev", hash->GetName()) )
		{
			if ( vDevsHash.Find( uHash ) == -1 )
				vDevsHash.AddToTail( uHash );
		}
		else if ( !Q_stricmp("bt", hash->GetName()) )
		{
			if ( vTestersHash.Find( uHash ) == -1 )
				vTestersHash.AddToTail( uHash );
		}
		else if ( !Q_stricmp("ban", hash->GetName()) )
		{
			if ( vBannedHash.Find( uHash ) == -1 )
				vBannedHash.AddToTail( uHash );
		}

		hash = hash->GetNextKey();
	}

	kv->deleteThis();
}
#endif

const unsigned char *GEUTIL_GetSecretHash( void )
{
	// It's not really a secret...
	return (unsigned char*) "Gr3naDes";
}

// Functions used to access awards information from the shared defs
const char *AwardIDToIdent( int id )
{
	if ( (id < 0) || (id >= GE_AWARD_GIVEMAX) )
		return "";

	return GEAwardInfo[id].ident;
}
int AwardIdentToID( const char *ident )
{
	if (ident)
	{
		for( int i = 0; i < GE_AWARD_GIVEMAX; ++i ) {
			if (!GEAwardInfo[i].ident)
				continue;
			if (!Q_stricmp( GEAwardInfo[i].ident, ident ))
				return i;
		}
	}

	return -1;
}
const char *GetIconForAward( int id )
{
	if ( (id < 0) || (id >= GE_AWARD_GIVEMAX) )
		return "";

	return GEAwardInfo[id].icon;
}
const char *GetAwardPrintName( int id )
{
	if ( (id < 0) || (id >= GE_AWARD_GIVEMAX) )
		return "";

	return GEAwardInfo[id].printname;
}
bool GetAwardSort( int id )
{
	if ( (id < 0) || (id >= GE_AWARD_GIVEMAX) )
		return false;

	return GEAwardInfo[id].sortdir;
}
// End award helper functions


// Given an alias, return the associated weapon ID
//
int AliasToWeaponID( const char *alias )
{
	if (alias)
	{
		for( int i = WEAPON_NONE; i < WEAPON_MAX; ++i ) {
			if (!GEWeaponInfo[i].alias)
				continue;
			if (!Q_stricmp( GEWeaponInfo[i].alias, alias ))
				return i;
		}
	}

	return WEAPON_NONE;
}

// Given a weapon ID, return it's alias
//
const char *WeaponIDToAlias( int id )
{
	if ( (id < 0) || (id >= WEAPON_MAX) )
		return "";

	return GEWeaponInfo[id].alias;
}

const char *GetAmmoForWeapon( int id )
{
	if ( (id < 0) || (id >= WEAPON_MAX) )
		return "";

	return GEWeaponInfo[id].ammo;
}

const char *GetWeaponPrintName( int id )
{
	if ( (id < 0) || (id >= WEAPON_MAX) )
		return "";

	return GEWeaponInfo[id].printname;
}

int GetRandWeightForWeapon( int id )
{
	if ( (id < 0) || (id >= WEAPON_MAX) )
		return 0;

	return GEWeaponInfo[id].randweight;
}
// End weapon helper functions

#ifdef GAME_DLL
// Spawner Helper Functions
const char *SpawnerTypeToClassName( int id )
{
	if ( id < SPAWN_NONE || id >= SPAWN_MAX )
		return "";

	return GESpawnerInfo[id].classname;
}
// End Spawner helper functions
#endif

// LINUX G++ 4.7 doesn't play well with our min/max defs
#ifdef _LINUX
	#undef min
	#undef max
#endif

#include "memdbgoff.h"
#include <list>

typedef struct {
	unsigned long	address;
	unsigned long	size;
	char	file[64];
	unsigned long	line;
} ALLOC_INFO;

typedef std::list<ALLOC_INFO*> AllocList;
AllocList *allocList;

void AddTrack(unsigned long addr,  unsigned long asize,  const char *fname, unsigned long lnum)
{
#if GES_ENABLE_MEMDEBUG
	ALLOC_INFO *info;

	if( !allocList )
		allocList = new( AllocList );

	info = new( ALLOC_INFO );
	info->address = addr;
	Q_strncpy( info->file, Q_UnqualifiedFileName( fname ), 63 );
	info->line = lnum;
	info->size = asize;
	allocList->insert( allocList->begin(), info );
#endif
}

void RemoveTrack(unsigned long addr)
{
#if GES_ENABLE_MEMDEBUG
	AllocList::iterator i;

	if(!allocList)
		return;
	for(i = allocList->begin(); i != allocList->end(); i++)
	{
		if( (*i)->address == addr )
		{
			allocList->remove( (*i) );
			break;
		}
	}
#endif
}

void GE_DumpMemoryLeaks( void )
{
#if GES_ENABLE_MEMDEBUG
	AllocList::iterator i;
	CUtlBuffer buf;
	buf.PutString( "FILE,LINE,ADDRESS,SIZE\n");

	for( i = allocList->begin(); i != allocList->end(); i++ )
		buf.Printf( "%s,%d,%d,%d\n", (*i)->file, (*i)->line, (*i)->address, (*i)->size );

#ifdef GAME_DLL
	filesystem->WriteFile( "memdump_server.csv", ".", buf );
#else
	filesystem->WriteFile( "memdump_client.csv", ".", buf );
#endif
#endif
}
