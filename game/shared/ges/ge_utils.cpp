#include "cbase.h"
#include "mathlib/IceKey.H"
#include "filesystem.h"
#include "utlbuffer.h"
#include "gamestringpool.h"
#include "ge_shareddefs.h"
#include "ge_weapon_parse.h"

#ifdef CLIENT_DLL
	#include <vgui/VGUI.h>
	#include <vgui/ILocalize.h>
	#include <vgui/ISurface.h>
	#include "vgui_controls/Controls.h"
	#include "c_ge_gameplayresource.h"
	#include "view.h"
#undef min
#undef max
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
	_realMapCommand->CreateBase( new_name, "", FCVAR_HIDDEN );       //Tony; create and hide it from the list.
	_realMapCommand->Init();

	_overMapCommand->Shutdown();
	_overMapCommand->CreateBase( real_name, override_desc );
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
		data[len] = 0;

		out.AddToTail( data );
		cnt++;
	
		x = Q_SkipSpaces( y );
		y = Q_SkipData( x );		
	}

	return cnt;
}

#ifdef CLIENT_DLL

void GEUTIL_DrawSprite3D( IMaterial *pMaterial, Vector offset, float width, float height, int alpha = 255 )
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

	meshBuilder.Color4ub( 255, 255, 255, alpha );
	meshBuilder.TexCoord2f( 0,0,0 );
	meshBuilder.Position3fv( (vOrigin + left + top).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( 255, 255, 255, alpha );
	meshBuilder.TexCoord2f( 0,1,0 );
	meshBuilder.Position3fv( (vOrigin + right + top).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( 255, 255, 255, alpha );
	meshBuilder.TexCoord2f( 0,1,1 );
	meshBuilder.Position3fv( (vOrigin + right + bottom).Base() );
	meshBuilder.AdvanceVertex();

	meshBuilder.Color4ub( 255, 255, 255, alpha );
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

void GEUTIL_ParseInternalLocalization( wchar_t *out, int out_len, const char *in, const char *trigger = "##" )
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
	wcsncpy( out, output.c_str(), out_len );
}

void GEUTIL_ParseLocalization( wchar_t *out, int out_len, const char *input )
{
	static wchar_t* tkn_names[] = { L"%s1", L"%s2", L"%s3", L"%s4", L"%s5", L"%s6", L"%s7", L"%s8", L"%s9", L"%s10" };
	static int max_tkn_count = 10;

	// Default is direct input -> output passthrough
	Q_strtowcs( input, -1, out, out_len * sizeof(wchar_t) );

	if ( !input || Q_strlen(input) < 2 )
		return;

	// Check for non-localization or the trigger as the first portion
	if ( input[0] != '#' || ( input[0] == '#' && input[1] == '#' ) )
	{
		// Parse internal localization (GES Specific!), looking for ## triggers
		GEUTIL_ParseInternalLocalization( out, out_len, input );
		// Replace the key bindings
		GEUTIL_ReplaceKeyBindings( out, out, out_len );
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

		int tkn_count =  (entries.Count() < max_tkn_count) ? entries.Count() : max_tkn_count;
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
		wcsncpy( out, finalstr.c_str(), out_len );
	}
	else if ( found )
	{
		// We don't have any tokens, copy the localized text to the output
		wcsncpy( out, found, out_len );
	}

	// Finally parse out key bindings
	GEUTIL_ReplaceKeyBindings( out, out, out_len );
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

uint64 GEUTIL_GetUniqueSkinData(int steamhash)
{
	uint64 value = 0;
	FileHandle_t file = filesystem->Open("gesdata.txt", "r", "GAME");

	if (file)
	{
		char mixedvalue[32], writevalue[16], hashvalue[8];
		int filehash = 0;

		filesystem->Read(mixedvalue, 32, file);
		filesystem->Close(file);

		for (int i = 0; i < 32; i++)
		{
			if (i % 2 == 0)
				writevalue[(int)(i / 2)] = mixedvalue[i];
			else if ((i - 1) % 4 == 0)
				hashvalue[(int)((i - 1) / 4)] = mixedvalue[i];
		}

		for (int i = 0; i < 8; i++)
			filehash += (hashvalue[i] - i - 32) << (i * 4);

		if (filehash != steamhash)
		{
			Warning("Invalid gesdata.txt!\n");
			return 0;
		}

		for (int i = 0; i < 16; i++)
			value += (writevalue[i] - i - 32) << (i * 4);
	}

	return value;
}

// This is just to scare off the people who aren't hardcore, and make it a pain to share the skins.
// If you're not part of the project and found this on github, congratulations!  You can give yourself some cool skins.
void GEUTIL_WriteUniqueSkinData( uint64 value, int steamhash )
{
	uint64 origbits = GEUTIL_GetUniqueSkinData(steamhash);
	uint64 bits = 0;

	for (int i = 0; i < 32; i++)
	{
		if ((value & (3 << (i*2))) > (origbits & (3 << (i*2))))
			bits = bits | value & (3 << (i*2));
		else
			bits = bits | origbits & (3 << (i * 2));
	}

	FileHandle_t file = filesystem->Open("gesdata.txt", "w", "MOD");

	if (file)
	{
		char writevalue[16], hashvalue[8], garbagevalue[8], mixedvalue[32];

		for (int i = 0; i < 16; i++)
			writevalue[i] = ((bits >> (i * 4)) & 15) + i + 32;

		for (int i = 0; i < 8; i++)
			hashvalue[i] = ((steamhash >> (i * 4)) & 15) + i + 32;

		for (int i = 0; i < 8; i++)
			garbagevalue[i] = (( (steamhash*steamhash - 398 * 28) >> (i * 4) ) & 15) + 32;

		for (int i = 0; i < 32; i++)
		{
			if (i % 2 == 0)
				mixedvalue[i] = writevalue[(int)(i/2)];
			else if ((i - 1) % 4 == 0)
				mixedvalue[i] = hashvalue[(int)((i - 1) / 4)];
			else if (i == 31)
				mixedvalue[i] = '\0';
			else
				mixedvalue[i] = garbagevalue[(int)((i+1)/4)];
		}

		filesystem->Write(mixedvalue, V_strlen(mixedvalue), file);

		filesystem->Close(file);
	}
}

// There are other, more seamless ways to do this but we wouldn't want someone making a mistake and giving out invalid skins.
// Since that could result in unusuable skins if the value is higher than a usable one.
uint64 GEUTIL_EventCodeToSkin( int code )
{
	uint64 skincode = 0;

	if (code & 1)
		skincode |= 64; // Black DD44
	if (code & 2)
		skincode |= 128; // Ivory DD44
	if (code & 4)
		skincode |= 4096; // Black Magnum
	if (code & 8)
		skincode |= 16777216; // Rusty ZMG

	return skincode;
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

void GEUTIL_StripWhitespace( char *szBuffer )
{
	char *szOut = szBuffer;

	for ( char *szIn = szOut; *szIn; szIn++ )
	{
		if ( *szIn != ' ' && *szIn != '\r' && *szIn != '\n' )
			*szOut++ = *szIn;
	}
	*szOut = '\0';
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
	case LIST_CONTRIBUTORS:
		return vContributorsHash.Find(hash) != -1;
	case LIST_SKINS:
		return vSkinsHash.Find(hash) != -1;
	case LIST_BANNED:
		return vBannedHash.Find(hash) != -1;
	}

	return false;
}

// These are extern'd in shareddefs.h
CUtlVector<unsigned int> vDevsHash;
CUtlVector<unsigned int> vTestersHash;
CUtlVector<unsigned int> vContributorsHash;
CUtlVector<unsigned int> vBannedHash;

CUtlVector<unsigned int> vSkinsHash;
CUtlVector<uint64> vSkinsValues;

int iAwardEventCode = 0;
uint64 iAllowedClientSkins = 0;
int iAlertCode = 31; // bit 1 = worry about spread, bit 2 = non-srand based GERandom, bit 3 = seed srand uniquely, bit 4 = votekick, bit 5 = namechange kick, bit 6 = martial law.  May we never use this one.
int iVotekickThresh = 70;

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

extern ConVar ge_alertcodeoverride;

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
		else if ( !Q_stricmp("cont", hash->GetName()) )
		{
			if ( vContributorsHash.Find( uHash ) == -1 )
				vContributorsHash.AddToTail( uHash );
		}
		else if ( !Q_stricmp("ban", hash->GetName()) )
		{
			if ( vBannedHash.Find( uHash ) == -1 )
				vBannedHash.AddToTail( uHash );
		}
		// Format is skin_x_y where x is the weapon to be skinned and y is the skin value of that weapon.
		else if ( !Q_strnicmp("skin_", hash->GetName(), 5) )
		{
			CUtlVector <char*> skindata;
			Q_SplitString(hash->GetName(), "_", skindata);

			if (skindata.Count() > 2)
			{
				uint64 skinvalue = atoi(skindata[2]); // Have to transfer it here so we have a 64 bit value to shift.
				uint64 bits = skinvalue << (atoi(skindata[1]) * 2); //2 bits per weapon.  

				int plIndex = vSkinsHash.Find(uHash);

				if (plIndex == -1) // We have no skins registered yet.
				{
					vSkinsHash.AddToTail(uHash);
					vSkinsValues.AddToTail(bits);
				}
				else // We already have a skin so we need to adjust our bitflags.
				{
					uint64 newbits = vSkinsValues[plIndex] | bits; //Bitwise OR to combine our bitflags. 100 | 010 = 110
					vSkinsValues[plIndex] = newbits;
				}
			}
			else
				Warning("Error parsing skin auth %s, Please bother E-S\n", hash->GetName());

			skindata.PurgeAndDeleteElements();
		}
		else if (!Q_stricmp("event_code", hash->GetName()))
		{
			iAwardEventCode = uHash; //Not really a hash this time but oh well.
		}
		else if (!Q_stricmp("allowed_skins", hash->GetName()))
		{
			iAllowedClientSkins = strtoull(hash->GetString(), NULL, 0); //Need to get the long long of the hash this time.
		}
		else if (!Q_stricmp( "alert_code", hash->GetName() ))
		{
			if (ge_alertcodeoverride.GetInt() < 0)
				iAlertCode = uHash;
		}
		else if (!Q_stricmp("vote_percent", hash->GetName()))
		{
			iVotekickThresh = uHash;
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

int GetStrengthOfWeapon(int id)
{
	if ((id < 0) || (id >= WEAPON_MAX))
		return 0;

	return GEWeaponInfo[id].strength;
}


int WeaponMaxDamageFromID(int id)
{
	const CGEWeaponInfo *weap = NULL;
	
	const char *name = WeaponIDToAlias(id);
	if (name && name[0] != '\0')
	{
		int h = LookupWeaponInfoSlot(name);
		if (h == GetInvalidWeaponInfoHandle())
			return 5000;

		weap = dynamic_cast<CGEWeaponInfo*>(GetFileWeaponInfoFromHandle(h));

		if (weap)
			return weap->m_iDamageCap;
	}

	return 5000;
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

// Removes cut off multibyte characters from the end of a string.
bool GEUTIL_CleanupNameEnding( const char* pName, char* pOutputName )
{
	Q_strncpy(pOutputName, pName, MAX_PLAYER_NAME_LENGTH); // We always expect an output.

	// Source makes sure the last character is the null terminator so we don't need to check for that.  In fact that's part of the issue.
	if (Q_strlen(pName) >= MAX_PLAYER_NAME_LENGTH - 1 && pName[MAX_PLAYER_NAME_LENGTH - 2] & 0x80) // Name hits the character limit and penultimate character is not a single byte character.
	{
		// Basically the issue is that multibyte characters can get cut off by source and cause issues in python.
		// Because of this we need to find and remove any corrupted characters.  We do this with their bit encodings.
		// 0xxxxxxx is a single byte character.
		// 1xxxxxxx is a multibyte character.
		// the posistion of the first 0 after the 1 tells us the nature of this particular character.
		// 10xxxxxx is a continuation of a multibyte character
		// 110xxxxx is the first byte of a 2 byte multibyte character
		// 1110xxxx is the first byte of a 3 byte multibyte character
		// 11110xxx is the first byte of a 4 byte multibyte character

		int c;
		int zeropos = -1;

		for (c = 2; c < 5; c++) // Check the last 3 characters in the string, since UTF-8 characters can be up to 4 bytes.
		{
			for (int i = 1; i < 6; i++)
			{
				if (~pName[MAX_PLAYER_NAME_LENGTH - c] & (1 << (7 - i))) // Find the first zero
				{
					zeropos = i;
					break;
				}
			}

			if (zeropos > 1) // If the zero is at bit 6 then this is not the first byte of the character, we need to keep going.  Otherwise we found what we're looking for.
				break;
		}

		if (zeropos >= c)
		{
			pOutputName[MAX_PLAYER_NAME_LENGTH - c] = '\0'; //Replace the start of the character with the end of the string.

			return true;
		}
	}
	return false;
}
#endif

