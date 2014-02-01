//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
//
// hud.cpp
//
// implementation of CHud class
//
#include "cbase.h"
#include "hud_macros.h"
#include "history_resource.h"
#include "iinput.h"
#include "clientmode.h"
#include "in_buttons.h"
#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include "itextmessage.h"
#include "mempool.h"
#include <KeyValues.h>
#include "filesystem.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include "hud_lcd.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static 	CClassMemoryPool< CHudTexture >	 g_HudTextureMemoryPool( 128 );

//-----------------------------------------------------------------------------
// Purpose: Parses the weapon txt files to get the sprites needed.
//-----------------------------------------------------------------------------
struct HudTextureFileRef
{
	HudTextureFileRef ( const char *cszFileKey, const char *cszHudTexturePrefix )
	{
		Q_strncpy( m_cszFileKey, cszFileKey, kcszFileKeyLength );
		Q_strncpy( m_cszHudTexturePrefix, cszHudTexturePrefix, kcszHudTexturePrefix );
		m_uiPrefixLength = Q_strlen( cszHudTexturePrefix );
		m_fileKeySymbol = KeyValuesSystem()->GetSymbolForString( m_cszFileKey );
		Assert( m_fileKeySymbol != INVALID_KEY_SYMBOL );
	}

	enum { kcszFileKeyLength = 64, };
	enum { kcszHudTexturePrefix = 16, };

	char m_cszFileKey[kcszFileKeyLength];
	char m_cszHudTexturePrefix[kcszHudTexturePrefix];
	unsigned int m_uiPrefixLength;
	HKeySymbol m_fileKeySymbol;
};

void LoadHudTextures( CUtlDict< CHudTexture *, int >& list, const char *szFilenameWithoutExtension, const unsigned char *pICEKey )
{
	KeyValues *pTemp, *pTextureSection;

	KeyValues *pKeyValuesData = ReadEncryptedKVFile( filesystem, szFilenameWithoutExtension, pICEKey );
	if ( pKeyValuesData )
	{
		CUtlVector<HudTextureFileRef> hudTextureFileRefs;

		// By default, add a default entry mapping "file" to no prefix. This will allow earlier-version files
		// to work with no modification.
		hudTextureFileRefs.AddToTail( HudTextureFileRef( "file", "" ) );

		// Read "*file"-to-prefix mapping.
		KeyValues *pTextureFileRefs = pKeyValuesData->FindKey( "TextureFileRefs" );
		if ( pTextureFileRefs )
		{
			pTemp = pTextureFileRefs->GetFirstSubKey();
			while ( pTemp )
			{
				hudTextureFileRefs.AddToTail( HudTextureFileRef( pTemp->GetName(), pTemp->GetString( "prefix", "" ) ) );
				pTemp = pTemp->GetNextKey();
			}
		}

		// Read our individual HUD texture data blocks.
		pTextureSection = pKeyValuesData->FindKey( "TextureData" );
		if ( pTextureSection  )
		{
			// Read the sprite data
			pTemp = pTextureSection->GetFirstSubKey();
			while ( pTemp )
			{
				if ( pTemp->GetString( "font", NULL ) )
				{
					CHudTexture *tex = new CHudTexture();

					// Key Name is the sprite name
					Q_strncpy( tex->szShortName, pTemp->GetName(), sizeof( tex->szShortName ) );

					// it's a font-based icon
					tex->bRenderUsingFont = true;
					tex->cCharacterInFont = *(pTemp->GetString("character", ""));
					Q_strncpy( tex->szTextureFile, pTemp->GetString( "font" ), sizeof( tex->szTextureFile ) );

					list.Insert( tex->szShortName, tex );
				}
				else
				{
					int iTexLeft	= pTemp->GetInt( "x", 0 ),
						iTexTop		= pTemp->GetInt( "y", 0 ),
						iTexRight	= pTemp->GetInt( "width", 0 )	+ iTexLeft,
						iTexBottom	= pTemp->GetInt( "height", 0 )	+ iTexTop;

					for ( int i = 0; i < hudTextureFileRefs.Size(); i++ )
					{
						const char *cszFilename = pTemp->GetString( hudTextureFileRefs[i].m_fileKeySymbol, NULL );
						if ( cszFilename )
						{
							CHudTexture *tex = new CHudTexture();

							tex->bRenderUsingFont = false;
							tex->rc.left	= iTexLeft;
							tex->rc.top		= iTexTop;
							tex->rc.right	= iTexRight;
							tex->rc.bottom	= iTexBottom;

							Q_strncpy( tex->szShortName, hudTextureFileRefs[i].m_cszHudTexturePrefix, sizeof( tex->szShortName ) );
							Q_strncpy( tex->szShortName + hudTextureFileRefs[i].m_uiPrefixLength, pTemp->GetName(), sizeof( tex->szShortName ) - hudTextureFileRefs[i].m_uiPrefixLength );
							Q_strncpy( tex->szTextureFile, cszFilename, sizeof( tex->szTextureFile ) );

							list.Insert( tex->szShortName, tex );
						}
					}
				}

				pTemp = pTemp->GetNextKey();
			}
		}
	}

	// Failed for some reason. Delete the Key data and abort.
	pKeyValuesData->deleteThis();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : * - 
//			list - 
//-----------------------------------------------------------------------------
void FreeHudTextureList( CUtlDict< CHudTexture *, int >& list )
{
	int c = list.Count();
	for ( int i = 0; i < c; i++ )
	{
		CHudTexture *tex = list[ i ];
		delete tex;
	}
	list.RemoveAll();
}

// Globally-used fonts
vgui::HFont g_hFontTrebuchet24 = vgui::INVALID_FONT;


//=======================================================================================================================
// Hud Element Visibility handling
//=======================================================================================================================
typedef struct hudelement_hidden_s
{
	char	*sElementName;
	int		iHiddenBits;	// Bits in which this hud element is hidden
} hudelement_hidden_t;

ConVar hidehud( "hidehud", "0", FCVAR_CHEAT );



CHudTexture::CHudTexture()
{
	Q_memset( szShortName, 0, sizeof( szShortName ) );
	Q_memset( szTextureFile, 0, sizeof( szTextureFile ) );
	Q_memset( texCoords, 0, sizeof( texCoords ) );
	Q_memset( &rc, 0, sizeof( rc ) );
	textureId = -1;
	bRenderUsingFont = false;
	bPrecached = false;
	cCharacterInFont = 0;
	hFont = ( vgui::HFont )NULL;
}

CHudTexture& CHudTexture::operator =( const CHudTexture& src )
{
	if ( this == &src )
		return *this;

	Q_strncpy( szShortName, src.szShortName, sizeof( szShortName ) );
	Q_strncpy( szTextureFile, src.szTextureFile, sizeof( szTextureFile ) );
	Q_memcpy( texCoords, src.texCoords, sizeof( texCoords ) );

	if ( src.textureId == -1 )
	{
		// Didn't have a texture ID set
		textureId = -1;
	}
	else
	{
		// Make a new texture ID that uses the same texture
		textureId = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile( textureId, src.szTextureFile, false, false );
	}

	rc = src.rc;
	bRenderUsingFont = src.bRenderUsingFont;
	cCharacterInFont = src.cCharacterInFont;
	hFont = src.hFont;

	return *this;
}

CHudTexture::~CHudTexture()
{
	if ( vgui::surface() && textureId != -1 )
	{
		vgui::surface()->DestroyTextureID( textureId );
		textureId = -1;
	}
}


//=======================================================================================================================
//  CHudElement
//	All hud elements are derived from this class.
//=======================================================================================================================
//-----------------------------------------------------------------------------
// Purpose: Registers the hud element in a global list, in CHud
//-----------------------------------------------------------------------------
CHudElement::CHudElement( const char *pElementName )
{
	m_bActive = false;
	m_iHiddenBits = 0;
	m_pElementName = pElementName;
	SetNeedsRemove( false );
	m_bIsParentedToClientDLLRootPanel = false;

	// Make this for all hud elements, but when its a bit safer
#if defined( TF_CLIENT_DLL ) || defined( DOD_DLL )
	RegisterForRenderGroup( "global" );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Remove this hud element from the global list in CHUD
//-----------------------------------------------------------------------------
CHudElement::~CHudElement()
{
	if ( m_bNeedsRemove )
	{
		gHUD.RemoveHudElement( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudElement::SetActive( bool bActive )
{
	m_bActive = bActive;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : needsremove - 
//-----------------------------------------------------------------------------
void CHudElement::SetNeedsRemove( bool needsremove )
{
	m_bNeedsRemove = needsremove;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudElement::SetHiddenBits( int iBits )
{
	m_iHiddenBits = iBits;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudElement::ShouldDraw( void )
{
	bool bShouldDraw = ( !gHUD.IsHidden( m_iHiddenBits ) );

	if ( bShouldDraw )
	{
		// for each render group
		int iNumGroups = m_HudRenderGroups.Count();
		for ( int iGroupIndex = 0; iGroupIndex < iNumGroups; iGroupIndex++ )
		{
			if ( gHUD.IsRenderGroupLockedFor( this, m_HudRenderGroups.Element(iGroupIndex ) ) )
				return false;
		}
	}

	return bShouldDraw;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHudElement::IsParentedToClientDLLRootPanel() const
{
	return m_bIsParentedToClientDLLRootPanel;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : parented - 
//-----------------------------------------------------------------------------
void CHudElement::SetParentedToClientDLLRootPanel( bool parented )
{
	m_bIsParentedToClientDLLRootPanel = parented;
}

//-----------------------------------------------------------------------------
// Purpose: We can register to be affected by multiple hud render groups
//-----------------------------------------------------------------------------
void CHudElement::RegisterForRenderGroup( const char *pszGroupName )
{
	int iGroupIndex = gHUD.RegisterForRenderGroup( pszGroupName );

	// add group index to our list of registered groups
	if ( m_HudRenderGroups.Find( iGroupIndex ) == m_HudRenderGroups.InvalidIndex() )
	{
		m_HudRenderGroups.AddToTail( iGroupIndex );
	}
}

void CHudElement::UnregisterForRenderGroup( const char *pszGroupName )
{
	int iGroupIndex = gHUD.RegisterForRenderGroup( pszGroupName );

	m_HudRenderGroups.FindAndRemove( iGroupIndex );
}

//-----------------------------------------------------------------------------
// Purpose: We want to obscure other elements in this group
//-----------------------------------------------------------------------------
void CHudElement::HideLowerPriorityHudElementsInGroup( const char *pszGroupName )
{
	// look up the render group
	int iGroupIndex = gHUD.LookupRenderGroupIndexByName( pszGroupName );

	// lock the group
	gHUD.LockRenderGroup( iGroupIndex, this );
}

//-----------------------------------------------------------------------------
// Purpose: Stop obscuring other elements in this group
//-----------------------------------------------------------------------------
void CHudElement::UnhideLowerPriorityHudElementsInGroup( const char *pszGroupName )
{	
	// look up the render group
	int iGroupIndex = gHUD.LookupRenderGroupIndexByName( pszGroupName );

	// unlock the group
	gHUD.UnlockRenderGroup( iGroupIndex, this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CHudElement::GetRenderGroupPriority( void )
{
	return 0;
}

CHud gHUD;  // global HUD object

DECLARE_MESSAGE(gHUD, ResetHUD);

#ifdef CSTRIKE_DLL
DECLARE_MESSAGE(gHUD, SendAudio);
#endif

CHud::CHud()
{
	SetDefLessFunc( m_RenderGroups );

	m_flScreenShotTime = -1;
}

//-----------------------------------------------------------------------------
// Purpose: This is called every time the DLL is loaded
//-----------------------------------------------------------------------------
void CHud::Init( void )
{
	HOOK_HUD_MESSAGE( gHUD, ResetHUD );
	
#ifdef CSTRIKE_DLL
	HOOK_HUD_MESSAGE( gHUD, SendAudio );
#endif

	InitFonts();

	// Create all the Hud elements
	CHudElementHelper::CreateAllElements();

	gLCD.Init();

	// Initialize all created elements
	for ( int i = 0; i < m_HudList.Size(); i++ )
	{
		m_HudList[i]->Init();
	}

	m_bHudTexturesLoaded = false;

	KeyValues *kv = new KeyValues( "layout" );
	if ( kv )
	{
		if ( kv->LoadFromFile( filesystem, "scripts/HudLayout.res" ) )
		{
			int numelements = m_HudList.Size();

			for ( int i = 0; i < numelements; i++ )
			{
				CHudElement *element = m_HudList[i];

				vgui::Panel *pPanel = dynamic_cast<vgui::Panel*>(element);
				if ( !pPanel )
				{
					Msg( "Non-vgui hud element %s\n", m_HudList[i]->GetName() );
					continue;
				}

				KeyValues *key = kv->FindKey( pPanel->GetName(), false );
				if ( !key )
				{
					Msg( "Hud element '%s' doesn't have an entry '%s' in scripts/HudLayout.res\n", m_HudList[i]->GetName(), pPanel->GetName() );
				}

				// Note:  When a panel is parented to the module root, it's "parent" is returned as NULL.
				if ( !element->IsParentedToClientDLLRootPanel() && 
					 !pPanel->GetParent() )
				{
					DevMsg( "Hud element '%s'/'%s' doesn't have a parent\n", m_HudList[i]->GetName(), pPanel->GetName() );
				}
			}
		}

		kv->deleteThis();
	}

	if ( m_bHudTexturesLoaded )
		return;

	m_bHudTexturesLoaded = true;
	CUtlDict< CHudTexture *, int >	textureList;

	// check to see if we have sprites for this res; if not, step down
	LoadHudTextures( textureList, "scripts/hud_textures", NULL );
	LoadHudTextures( textureList, "scripts/mod_textures", NULL );

	int c = textureList.Count();
	for ( int index = 0; index < c; index++ )
	{
		CHudTexture* tex = textureList[ index ];
		AddSearchableHudIconToList( *tex );
	}

	FreeHudTextureList( textureList );
}

//-----------------------------------------------------------------------------
// Purpose: Init Hud global colors
// Input  : *scheme - 
//-----------------------------------------------------------------------------
void CHud::InitColors( vgui::IScheme *scheme )
{
	m_clrNormal = scheme->GetColor( "Normal", Color( 255, 208, 64 ,255 ) );
	m_clrCaution = scheme->GetColor( "Caution", Color( 255, 48, 0, 255 ) );
	m_clrYellowish = scheme->GetColor( "Yellowish", Color( 255, 160, 0, 255 ) );
}

//-----------------------------------------------------------------------------
// Initializes fonts
//-----------------------------------------------------------------------------
void CHud::InitFonts()
{
	vgui::HScheme scheme = vgui::scheme()->GetScheme( "ClientScheme" );
	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( scheme );
	g_hFontTrebuchet24 = pScheme->GetFont("CenterPrintText", true);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHud::Shutdown( void )
{
	gLCD.Shutdown();

	// Deleting hudlist items can result in them being removed from the same hudlist (m_bNeedsRemove).
	//	So go through and kill the last item until the array is empty.
	while ( m_HudList.Size() > 0 )
	{
		delete m_HudList.Tail();
	}

	m_HudList.Purge();
	m_bHudTexturesLoaded = false;
}


//-----------------------------------------------------------------------------
// Purpose: LevelInit's called whenever a new level is starting
//-----------------------------------------------------------------------------
void CHud::LevelInit( void )
{
	// Tell all the registered hud elements to LevelInit
	for ( int i = 0; i < m_HudList.Size(); i++ )
	{
		m_HudList[i]->LevelInit();
	}

	// Unhide all render groups
	int iCount = m_RenderGroups.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		CHudRenderGroup *group = m_RenderGroups[ i ];
		group->bHidden = false;
		group->m_pLockingElements.Purge();
	}
}

//-----------------------------------------------------------------------------
// Purpose: LevelShutdown's called whenever a level is finishing
//-----------------------------------------------------------------------------
void CHud::LevelShutdown( void )
{
	// Tell all the registered hud elements to LevelShutdown
	for ( int i = 0; i < m_HudList.Size(); i++ )
	{
		m_HudList[i]->LevelShutdown();
	}
}

//-----------------------------------------------------------------------------
// Purpose: cleans up memory allocated for m_rg* arrays
//-----------------------------------------------------------------------------
CHud::~CHud()
{
	int c = m_Icons.Count();
	for ( int i = c - 1; i >= 0; i-- )
	{
		CHudTexture *tex = m_Icons[ i ];
		g_HudTextureMemoryPool.Free( tex );
	}
	m_Icons.Purge();

	c = m_RenderGroups.Count();
	for ( int i = c - 1; i >= 0; i-- )
	{
		CHudRenderGroup *group = m_RenderGroups[ i ];
		m_RenderGroups.RemoveAt(i);
		delete group;
	}
}

void CHudTexture::Precache( void )
{
	// costly function, used selectively on specific hud elements to get font pages built out at load time
	if ( IsX360() && bRenderUsingFont && !bPrecached && hFont != vgui::INVALID_FONT )
	{
		wchar_t wideChars[2];
		wideChars[0] = (wchar_t)cCharacterInFont;
		wideChars[1] = 0;
		vgui::surface()->PrecacheFontCharacters( hFont, wideChars );
		bPrecached = true;
	}
}

void CHudTexture::DrawSelf( int x, int y, const Color& clr ) const
{
	DrawSelf( x, y, Width(), Height(), clr );
}

void CHudTexture::DrawSelf( int x, int y, int w, int h, const Color& clr ) const
{
	if ( bRenderUsingFont )
	{
		vgui::surface()->DrawSetTextFont( hFont );
		vgui::surface()->DrawSetTextColor( clr );
		vgui::surface()->DrawSetTextPos( x, y );
		vgui::surface()->DrawUnicodeChar( cCharacterInFont );
	}
	else
	{
		if ( textureId == -1 )
			return;

		vgui::surface()->DrawSetTexture( textureId );
		vgui::surface()->DrawSetColor( clr );
		vgui::surface()->DrawTexturedSubRect( x, y, x + w, y + h, 
			texCoords[ 0 ], texCoords[ 1 ], texCoords[ 2 ], texCoords[ 3 ] );
	}
}

#ifdef GE_DLL
void CHudTexture::DrawSelfRotated( int x, int y, float theta, Color clr /*= Color(255,255,255,255)*/ ) const
{
	DrawSelfRotated( x, y, Width(), Height(), theta, clr );
}

void CHudTexture::DrawSelfRotated( int x, int y, int w, int h, float theta, Color clr /*= Color(255,255,255,255)*/ ) const
{
	// This is only for textures
	if ( bRenderUsingFont || theta == 0 )
	{
		DrawSelf( x, y, w, h, clr );
		return;
	}

	if ( textureId == -1 )
		return;

	float half_w = w/2.0f;
	float half_h = h/2.0f;

	VMatrix pMat;
	pMat.Init( -half_w, -half_h, 0, 0,
				half_w, -half_h, 0, 0,
				half_w,  half_h, 0, 0,
			   -half_w,	 half_h, 0, 0 );

	VMatrix rotMat;
	rotMat.Init( cos(theta), sin(theta), 0, 0,
				-sin(theta), cos(theta), 0, 0,
				 0,			 0,			 0, 0,
				 0,			 0,			 0, 0 );

	VMatrix rMat;
	MatrixMultiply( pMat, rotMat, rMat );

	int x1 = x + half_w;
	int y1 = y + half_h;

	vgui::Vertex_t points[4] =
	{
		vgui::Vertex_t( Vector2D(rMat[0][0]+x1, rMat[0][1]+y1), Vector2D(texCoords[0], texCoords[1]) ),
		vgui::Vertex_t( Vector2D(rMat[1][0]+x1, rMat[1][1]+y1), Vector2D(texCoords[2], texCoords[1]) ),
		vgui::Vertex_t( Vector2D(rMat[2][0]+x1, rMat[2][1]+y1), Vector2D(texCoords[2], texCoords[3]) ),
		vgui::Vertex_t( Vector2D(rMat[3][0]+x1, rMat[3][1]+y1), Vector2D(texCoords[0], texCoords[3]) ),
	};

	vgui::surface()->DrawSetTexture( textureId );
	vgui::surface()->DrawSetColor( clr );
	vgui::surface()->DrawTexturedPolygon( 4, points );
}

void CHudTexture::DrawSelfScaled(int x, int y, float scale, Color clr /* Color(255,255,255,255) */ ) const
{
	DrawSelfScaled( x, y, Width(), Height(), scale, clr );
}

void CHudTexture::DrawSelfScaled( int x, int y, int w, int h, float scale, Color clr /* Color(255,255,255,255) */ ) const
{
	if ( bRenderUsingFont )
	{
		vgui::surface()->DrawSetTextFont( hFont );
		vgui::surface()->DrawSetTextColor( clr );
		vgui::surface()->DrawSetTextPos( x, y );
		vgui::surface()->DrawUnicodeChar( cCharacterInFont );
	}
	else
	{
		if ( textureId == -1 )
			return;

		float half_w = (w / 2.0f) * scale;
		float half_h = (h / 2.0f) * scale;

		int x1 = x + half_w;
		int y1 = y + half_h;

		VMatrix pMat;
		pMat.Init( -half_w, -half_h, 0, 0,
					half_w, -half_h, 0, 0,
					half_w,  half_h, 0, 0,
				   -half_w,	 half_h, 0, 0 );

		vgui::Vertex_t points[4] =
		{
			vgui::Vertex_t( Vector2D(pMat[0][0]+x1, pMat[0][1]+y1), Vector2D(texCoords[0], texCoords[1]) ),
			vgui::Vertex_t( Vector2D(pMat[1][0]+x1, pMat[1][1]+y1), Vector2D(texCoords[2], texCoords[1]) ),
			vgui::Vertex_t( Vector2D(pMat[2][0]+x1, pMat[2][1]+y1), Vector2D(texCoords[2], texCoords[3]) ),
			vgui::Vertex_t( Vector2D(pMat[3][0]+x1, pMat[3][1]+y1), Vector2D(texCoords[0], texCoords[3]) ),
		};

		vgui::surface()->DrawSetTexture( textureId );
		vgui::surface()->DrawSetColor( clr );
		vgui::surface()->DrawTexturedPolygon( 4, points );
	}
}

void CHudTexture::DrawScew_Left( int tex2, int width, int height, float percent, int offset, int radius ) const
{
	// ASSUMPTIONS:
	//	1. The image will be centered vertically about the midline of the screen
	//	2. The X coordinate will be the midpoint.x - offset - width
	//	3. The other half of the image will be defined in tex2 to save calculations
	//	4. The edges of the image will be the pivot points for intersections that don't hit the far side

	// Make sure the percent cut is between 0 and 1
	percent = clamp( percent, 0.0f, 1.0f );
	vgui::surface()->DrawSetTexture( textureId );
	vgui::surface()->DrawSetColor( Color(255,255,255,255) );
	
	// This defines the placement of the U,V on the image
	float scaledV1, scaledV2, scaledU;
	// These next two values define the left x,y position and the right x,y position that the bisecting
	// line cross the image at in screen space. x3 is used when we clamp to the bounds to keep a crisp line
	int x1, y1, x2, y2, x3;
	int midX, midY, origX;
	int midH = height / 2;

	vgui::surface()->GetScreenSize( midX, midY );
	midX /= 2; midY /= 2;
	// Determine the proper origin X position for a perfect circle based on our image
	origX = midX - offset + midH;

	// If we aren't doing ANY scaling then skip unnecessary calcs
	if ( percent == 1.0f )
	{
		// Only show the containing texture
		x1 = midX - offset - width;
		y1 = midY - midH;
		vgui::surface()->DrawTexturedRect( x1, y1, x1 + width, y1 + height );
		return;
	}
	else if ( percent == 0.0f )
	{
		// Only show the texture thats supposed to be behind
		x1 = midX - offset - width;
		y1 = midY - midH;
		if ( vgui::surface()->IsTextureIDValid( tex2 ) )
		{
			vgui::surface()->DrawSetTexture( tex2 );
			vgui::surface()->DrawTexturedRect( x1, y1, x1 + width, y1 + height );
		}
		return;
	}

	// Find our angle coefficient
	float ang;
	if ( percent >= 0.5f )
		percent = RemapValClamped( percent, 0.5, 1.0, 0.0, 1.0 );
	else
		percent = RemapValClamped( percent, 0.0, 0.5, -1.0, 0.0 );

	ang = tan( percent );
	
	// First let's calculate our intersection points of the scew line to the image in world space
	x1 = midX - offset - width;
	y1 = clamp( midY - ang * (radius), midY-midH, midY+midH );

	x2 = midX - offset;
	y2 = clamp( midY - ang * (radius-width), midY-midH, midY+midH );

	if ( y1 == (midY - midH) )
		x3 = x2 - (y2 - (midY - midH)) * tan( M_PI/2.0f - percent );
	else if ( y1 == (midY + midH) )
		x3 = x2 - (y2 - (midY + midH)) * tan( M_PI/2.0f - percent );
	else
		x3 = x1;

	scaledU = 1.0f - (float)(width - (x3 - x1)) / (float)width;

	// Now calculate our v1, v2 coords by taking the ratio of the difference in the intersection
	// Y points and the full image location
	scaledV1 = 1.0f - (float)(height - (y1 - (midY - midH))) / (float)height;
	scaledV2 = 1.0f - (float)(height - (y2 - (midY - midH))) / (float)height;

	if ( y1 == (midY + midH) )
	{
		// If we are clamped to the bottom of our image, we need to draw a triangle to avoid stretching UV
		vgui::Vertex_t points[3] =
		{
			vgui::Vertex_t( Vector2D(x3, y1), Vector2D(scaledU, scaledV1) ),
			vgui::Vertex_t( Vector2D(x2, y2), Vector2D(1, scaledV2) ),
			vgui::Vertex_t( Vector2D(x2, midY + midH), Vector2D(1,1) ),
		};
		// Draw it on the screen
		vgui::surface()->DrawTexturedPolygon( 3, points );
	}
	else
	{
		// Now populate our polygon with points
		vgui::Vertex_t points[5] =
		{
			vgui::Vertex_t( Vector2D(x1, y1), Vector2D(0, scaledV1) ),
			vgui::Vertex_t( Vector2D(x3, y1), Vector2D(scaledU, scaledV1) ),
			vgui::Vertex_t( Vector2D(x2, y2), Vector2D(1, scaledV2) ),
			vgui::Vertex_t( Vector2D(x2, midY + midH), Vector2D(1,1) ),
			vgui::Vertex_t( Vector2D(x1, midY + midH), Vector2D(0,1) ),
		};
		// Draw it on the screen
		vgui::surface()->DrawTexturedPolygon( 5, points );
	}

/*	// Debugging line
#ifdef _DEBUG
	vgui::surface()->DrawSetColor( 0, 255, 0, 255 );
	vgui::surface()->DrawLine( x1, y1, x3, y1 );
	vgui::surface()->DrawSetColor( 255, 0, 0, 255 );
	vgui::surface()->DrawLine( x3, y1, x2, y2 );
	vgui::surface()->DrawSetColor( 0, 0, 255, 255 );
	vgui::surface()->DrawOutlinedRect( x1, min(y1,y2), x2, midY + midH );
#endif
*/
	if ( vgui::surface()->IsTextureIDValid( tex2 ) )
	{
		// Now draw the inverse image
		vgui::Vertex_t invPoints[5] =
		{
			vgui::Vertex_t( Vector2D(x1, midY - midH), Vector2D(0, 0) ),
			vgui::Vertex_t( Vector2D(x2, midY - midH), Vector2D(1, 0) ),
			vgui::Vertex_t( Vector2D(x2, y2), Vector2D(1, scaledV2) ),
			vgui::Vertex_t( Vector2D(x3, y1), Vector2D(scaledU, scaledV1) ),
			vgui::Vertex_t( Vector2D(x1, y1), Vector2D(0, scaledV1) ),
		};
		// Draw it on the screen
		vgui::surface()->DrawSetTexture( tex2 );
		vgui::surface()->DrawSetColor( 255, 255, 255, 255 );
		vgui::surface()->DrawTexturedPolygon( 5, invPoints );
	}
}

void CHudTexture::DrawScew_Right( int tex2, int width, int height, float percent, int offset, int radius ) const
{
	// ASSUMPTIONS:
	//	1. The image will be centered vertically about the midline of the screen
	//	2. The X coordinate will be the midpoint.x + offset
	//	3. The other half of the image will be defined in tex2 to save calculations
	//	4. The edges of the image will be the pivot points for intersections that don't hit the far side

	// Make sure the percent cut is between 0 and 1
	percent = clamp( percent, 0.0f, 1.0f );
	vgui::surface()->DrawSetTexture( textureId );
	vgui::surface()->DrawSetColor( Color(255,255,255,255) );
	
	// This defines the placement of the U,V on the image
	float scaledV1, scaledV2, scaledU;
	// These next two values define the left x,y position and the right x,y position that the bisecting
	// line cross the image at in screen space. x3 is used when we clamp to the bounds to keep a crisp line
	int x1, y1, x2, y2, x3;
	int midX, midY;
	int midH = height / 2;

	vgui::surface()->GetScreenSize( midX, midY );
	midX /= 2; midY /= 2;

	// If we aren't doing ANY scaling then skip unnecessary calcs
	if ( percent == 1.0f )
	{
		// Only show the containing texture
		x1 = midX + offset;
		y1 = midY - midH;
		vgui::surface()->DrawTexturedRect( x1, y1, x1 + width, y1 + height );
		return;
	}
	else if ( percent == 0.0f )
	{
		// Only show the texture thats supposed to be behind
		x1 = midX + offset;
		y1 = midY - midH;
		if ( vgui::surface()->IsTextureIDValid( tex2 ) )
		{
			vgui::surface()->DrawSetTexture( tex2 );
			vgui::surface()->DrawTexturedRect( x1, y1, x1 + width, y1 + height );
		}
		return;
	}

	// Find our angle coefficient
	float ang;
	if ( percent >= 0.5f )
		percent = RemapValClamped( percent, 0.5, 1.0, 0.0, 1.0 );
	else
		percent = RemapValClamped( percent, 0.0, 0.5, -1.0, 0.0 );

	ang = tan( percent );
	
	// First let's calculate our intersection points of the scew line to the image in world space
	// Note: X1 is closest to middle now! (opposite of LeftScew)
	x1 = midX + offset;
	y1 = clamp( midY - ang * (radius-width), midY-midH, midY+midH );

	x2 = midX + offset + width;
	y2 = clamp( midY - ang * radius, midY-midH, midY+midH );

	if ( y2 == (midY - midH) )
		x3 = x1 + (y1 - (midY - midH)) * tan( M_PI/2 - percent );
	else if ( y2 == (midY + midH) )
		x3 = x1 + (y1 - (midY + midH)) * tan( M_PI/2 - percent );
	else
		x3 = x2;

	scaledU = 1.0f - (float)(x2 - x3) / (float)width;

	// Now calculate our v1, v2 coords by taking the ratio of the difference in the intersection
	// Y points and the full image location
	scaledV1 = 1.0f - (float)(height - (y1 - (midY - midH))) / (float)height;
	scaledV2 = 1.0f - (float)(height - (y2 - (midY - midH))) / (float)height;

	if ( y2 == (midY + midH) )
	{
		// Since we are at the bottom of the image we can ignore X2's coords
		// otherwise we get tearing in our UV mapping
		vgui::Vertex_t points[3] =
		{
			vgui::Vertex_t( Vector2D(x1, y1), Vector2D(0, scaledV1) ),
			vgui::Vertex_t( Vector2D(x3, y2), Vector2D(scaledU, scaledV2) ),
			vgui::Vertex_t( Vector2D(x1, midY + midH), Vector2D(0,1) ),
		};
		// Draw it on the screen
		vgui::surface()->DrawTexturedPolygon( 3, points );
	}
	else
	{
		// Now populate our polygon with points
		vgui::Vertex_t points[5] =
		{
			vgui::Vertex_t( Vector2D(x1, y1), Vector2D(0, scaledV1) ),
			vgui::Vertex_t( Vector2D(x3, y2), Vector2D(scaledU, scaledV2) ),
			vgui::Vertex_t( Vector2D(x2, y2), Vector2D(1, scaledV2) ),
			vgui::Vertex_t( Vector2D(x2, midY + midH), Vector2D(1,1) ),
			vgui::Vertex_t( Vector2D(x1, midY + midH), Vector2D(0,1) ),
		};
		// Draw it on the screen
		vgui::surface()->DrawTexturedPolygon( 5, points );
	}

/*	// Debugging line
#ifdef _DEBUG
	vgui::surface()->DrawSetColor( 0, 255, 0, 255 );
	vgui::surface()->DrawLine( x2, y2, x3, y2 );
	vgui::surface()->DrawSetColor( 255, 0, 0, 255 );
	vgui::surface()->DrawLine( x3, y2, x1, y1 );
	vgui::surface()->DrawSetColor( 0, 0, 255, 255 );
	vgui::surface()->DrawOutlinedRect( x1, min(y1,y2), x2, midY + midH );
#endif
*/
	if ( vgui::surface()->IsTextureIDValid( tex2 ) )
	{
		// Now draw the inverse image
		vgui::Vertex_t invPoints[5] =
		{
			vgui::Vertex_t( Vector2D(x1, midY - midH), Vector2D(0, 0) ),
			vgui::Vertex_t( Vector2D(x2, midY - midH), Vector2D(1, 0) ),
			vgui::Vertex_t( Vector2D(x2, y2), Vector2D(1, scaledV2) ),
			vgui::Vertex_t( Vector2D(x3, y2), Vector2D(scaledU, scaledV2) ),
			vgui::Vertex_t( Vector2D(x1, y1), Vector2D(0, scaledV1) ),
		};
		// Draw it on the screen
		vgui::surface()->DrawSetTexture( tex2 );
		vgui::surface()->DrawSetColor( 255, 255, 255, 255 );
		vgui::surface()->DrawTexturedPolygon( 5, invPoints );
	}
}
#endif

void CHudTexture::DrawSelfCropped( int x, int y, int cropx, int cropy, int cropw, int croph, int finalWidth, int finalHeight, Color clr ) const
{
	if ( bRenderUsingFont )
	{
		// work out how much we've been cropped
		int height = vgui::surface()->GetFontTall( hFont );
		float frac = (height - croph) / (float)height;
		y -= cropy;

		vgui::surface()->DrawSetTextFont( hFont );
		vgui::surface()->DrawSetTextColor( clr );
		vgui::surface()->DrawSetTextPos( x, y );

		vgui::CharRenderInfo info;
		if ( vgui::surface()->DrawGetUnicodeCharRenderInfo( cCharacterInFont, info ) )
		{
			if ( cropy )
			{
				info.verts[0].m_Position.y = Lerp( frac, info.verts[0].m_Position.y, info.verts[1].m_Position.y );
				info.verts[0].m_TexCoord.y = Lerp( frac, info.verts[0].m_TexCoord.y, info.verts[1].m_TexCoord.y );
			}
			else if ( croph != height )
			{
				info.verts[1].m_Position.y = Lerp( 1.0f - frac, info.verts[0].m_Position.y, info.verts[1].m_Position.y );
				info.verts[1].m_TexCoord.y = Lerp( 1.0f - frac, info.verts[0].m_TexCoord.y, info.verts[1].m_TexCoord.y );
			}
			vgui::surface()->DrawRenderCharFromInfo(info);
		}
	}
	else
	{
		if ( textureId == -1 )
			return;

		float fw = (float)Width();
		float fh = (float)Height();

		float twidth	= texCoords[ 2 ] - texCoords[ 0 ];
		float theight	= texCoords[ 3 ] - texCoords[ 1 ];

		// Interpolate coords
		float tCoords[ 4 ];
		tCoords[ 0 ] = texCoords[ 0 ] + ( (float)cropx / fw ) * twidth;
		tCoords[ 1 ] = texCoords[ 1 ] + ( (float)cropy / fh ) * theight;
		tCoords[ 2 ] = texCoords[ 0 ] + ( (float)(cropx + cropw ) / fw ) * twidth;
		tCoords[ 3 ] = texCoords[ 1 ] + ( (float)(cropy + croph ) / fh ) * theight;

		vgui::surface()->DrawSetTexture( textureId );
		vgui::surface()->DrawSetColor( clr );
		vgui::surface()->DrawTexturedSubRect( 
			x, y, 
			x + finalWidth, y + finalHeight, 
			tCoords[ 0 ], tCoords[ 1 ], 
			tCoords[ 2 ], tCoords[ 3 ] );
	}
}

void CHudTexture::DrawSelfCropped( int x, int y, int cropx, int cropy, int cropw, int croph, Color clr ) const
{
	DrawSelfCropped( x, y, cropx, cropy, cropw, croph, cropw, croph, clr );
}

//-----------------------------------------------------------------------------
// Purpose: returns width of texture with scale factor applied.  (If rendered
//			using font, scale factor is ignored.)
//-----------------------------------------------------------------------------
int CHudTexture::EffectiveWidth( float flScale ) const
{
	if ( !bRenderUsingFont )
	{
		return (int) ( Width() * flScale );
	}
	else
	{
		return vgui::surface()->GetCharacterWidth( hFont, cCharacterInFont );		
	}
}

//-----------------------------------------------------------------------------
// Purpose: returns height of texture with scale factor applied.  (If rendered
//			using font, scale factor is ignored.)
//-----------------------------------------------------------------------------
int CHudTexture::EffectiveHeight( float flScale ) const
{
	if ( !bRenderUsingFont )
	{
		return (int) ( Height() * flScale );
	}
	else
	{
		return vgui::surface()->GetFontAscent( hFont, cCharacterInFont );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Gets texture handles for the hud icon
//-----------------------------------------------------------------------------
void CHud::SetupNewHudTexture( CHudTexture *t )
{
	if ( t->bRenderUsingFont )
	{
		vgui::HScheme scheme = vgui::scheme()->GetScheme( "ClientScheme" );
		t->hFont = vgui::scheme()->GetIScheme(scheme)->GetFont( t->szTextureFile, true );
		t->rc.top = 0;
		t->rc.left = 0;
		t->rc.right = vgui::surface()->GetCharacterWidth( t->hFont, t->cCharacterInFont );
		t->rc.bottom = vgui::surface()->GetFontTall( t->hFont );
	}
	else
	{
		// Set up texture id and texture coordinates
		t->textureId = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile( t->textureId, t->szTextureFile, false, false );

		int wide, tall;
		vgui::surface()->DrawGetTextureSize( t->textureId, wide, tall );

		t->texCoords[ 0 ] = (float)(t->rc.left + 0.5f) / (float)wide;
		t->texCoords[ 1 ] = (float)(t->rc.top + 0.5f) / (float)tall;
		t->texCoords[ 2 ] = (float)(t->rc.right - 0.5f) / (float)wide;
		t->texCoords[ 3 ] = (float)(t->rc.bottom - 0.5f) / (float)tall;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTexture *CHud::AddUnsearchableHudIconToList( CHudTexture& texture )
{
	// These names are composed based on the texture file name
	char composedName[ 512 ];

	if ( texture.bRenderUsingFont )
	{
		Q_snprintf( composedName, sizeof( composedName ), "%s_c%i",
			texture.szTextureFile, texture.cCharacterInFont );
	}
	else
	{
		Q_snprintf( composedName, sizeof( composedName ), "%s_%i_%i_%i_%i",
			texture.szTextureFile, texture.rc.left, texture.rc.top, texture.rc.right, texture.rc.bottom );
	}

	CHudTexture *icon = GetIcon( composedName );
	if ( icon )
	{
		return icon;
	}

	CHudTexture *newTexture = ( CHudTexture * )g_HudTextureMemoryPool.Alloc();
	*newTexture = texture;

	SetupNewHudTexture( newTexture );

	int idx = m_Icons.Insert( composedName, newTexture );
	return m_Icons[ idx ];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudTexture *CHud::AddSearchableHudIconToList( CHudTexture& texture )
{
	CHudTexture *icon = GetIcon( texture.szShortName );
	if ( icon )
	{
		return icon;
	}

	CHudTexture *newTexture = ( CHudTexture * )g_HudTextureMemoryPool.Alloc();
	*newTexture = texture;

	SetupNewHudTexture( newTexture );

	int idx = m_Icons.Insert( texture.szShortName, newTexture );
	return m_Icons[ idx ];
}

//-----------------------------------------------------------------------------
// Purpose: returns a pointer to an icon in the list
//-----------------------------------------------------------------------------
CHudTexture *CHud::GetIcon( const char *szIcon )
{
	int i = m_Icons.Find( szIcon );
	if ( i == m_Icons.InvalidIndex() )
		return NULL;

	return m_Icons[ i ];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHud::RefreshHudTextures()
{
	if ( !m_bHudTexturesLoaded )
	{
		Assert( 0 );
		return;
	}

	CUtlDict< CHudTexture *, int >	textureList;

	// check to see if we have sprites for this res; if not, step down
	LoadHudTextures( textureList, "scripts/hud_textures", NULL );
	LoadHudTextures( textureList, "scripts/mod_textures", NULL );

	// fix up all the texture icons first
	int c = textureList.Count();
	for ( int index = 0; index < c; index++ )
	{
		CHudTexture *tex = textureList[ index ];
		Assert( tex );

		CHudTexture *icon = GetIcon( tex->szShortName );
		if ( !icon )
			continue;

		// Update file
		Q_strncpy( icon->szTextureFile, tex->szTextureFile, sizeof( icon->szTextureFile ) );

		if ( !icon->bRenderUsingFont )
		{
			// Update subrect
			icon->rc = tex->rc;

			// Keep existing texture id, but now update texture file and texture coordinates
			vgui::surface()->DrawSetTextureFile( icon->textureId, icon->szTextureFile, false, false );

			// Get new texture dimensions in case it changed
			int wide, tall;
			vgui::surface()->DrawGetTextureSize( icon->textureId, wide, tall );

			// Assign coords
			icon->texCoords[ 0 ] = (float)(icon->rc.left + 0.5f) / (float)wide;
			icon->texCoords[ 1 ] = (float)(icon->rc.top + 0.5f) / (float)tall;
			icon->texCoords[ 2 ] = (float)(icon->rc.right - 0.5f) / (float)wide;
			icon->texCoords[ 3 ] = (float)(icon->rc.bottom - 0.5f) / (float)tall;
		}
	}

	FreeHudTextureList( textureList );

	// fixup all the font icons
	vgui::HScheme scheme = vgui::scheme()->GetScheme( "ClientScheme" );
	for (int i = m_Icons.First(); m_Icons.IsValidIndex(i); i = m_Icons.Next(i))
	{
		CHudTexture *icon = m_Icons[i];
		if ( !icon )
			continue;

		// Update file
		if ( icon->bRenderUsingFont )
		{
			icon->hFont = vgui::scheme()->GetIScheme(scheme)->GetFont( icon->szTextureFile, true );
			icon->rc.top = 0;
			icon->rc.left = 0;
			icon->rc.right = vgui::surface()->GetCharacterWidth( icon->hFont, icon->cCharacterInFont );
			icon->rc.bottom = vgui::surface()->GetFontTall( icon->hFont );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHud::OnRestore()
{
	ResetHUD();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHud::VidInit( void )
{
	for ( int i = 0; i < m_HudList.Size(); i++ )
	{
		m_HudList[i]->VidInit();
	}


	ResetHUD();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudElement *CHud::FindElement( const char *pName )
{
	for ( int i = 0; i < m_HudList.Size(); i++ )
	{
		if ( V_stricmp( m_HudList[i]->GetName(), pName ) == 0 )
			return m_HudList[i];
	}

	DevWarning(1, "Could not find Hud Element: %s\n", pName );
	Assert(0);
	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Adds a member to the HUD
//-----------------------------------------------------------------------------
void CHud::AddHudElement( CHudElement *pHudElement ) 
{
	// Add the hud element to the end of the array
	m_HudList.AddToTail( pHudElement );

	pHudElement->SetNeedsRemove( true );
}

//-----------------------------------------------------------------------------
// Purpose: Remove an element from the HUD
//-----------------------------------------------------------------------------
void CHud::RemoveHudElement( CHudElement *pHudElement ) 
{
	m_HudList.FindAndRemove( pHudElement );
}

//-----------------------------------------------------------------------------
// Purpose: Returns current mouse sensitivity setting
// Output : float - the return value
//-----------------------------------------------------------------------------
float CHud::GetSensitivity( void )
{
#ifndef _X360
	return m_flMouseSensitivity;
#else
	return 1.0f;
#endif
}

float CHud::GetFOVSensitivityAdjust()
{
	return m_flFOVSensitivityAdjust;
}
//-----------------------------------------------------------------------------
// Purpose: Return true if the passed in sections of the HUD shouldn't be drawn
//-----------------------------------------------------------------------------
bool CHud::IsHidden( int iHudFlags )
{
	// Not in game?
	if ( !engine->IsInGame() )
		return true;

	// No local player yet?
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return true;

	// Get current hidden flags
	int iHideHud = pPlayer->m_Local.m_iHideHUD;
	if ( hidehud.GetInt() )
	{
		iHideHud = hidehud.GetInt();
	}

	// Everything hidden?
	if ( iHideHud & HIDEHUD_ALL )
		return true;

	// Local player dead?
	if ( ( iHudFlags & HIDEHUD_PLAYERDEAD ) && ( pPlayer->GetHealth() <= 0 && !pPlayer->IsAlive() ) )
		return true;

	// Need the HEV suit ( HL2 )
	if ( ( iHudFlags & HIDEHUD_NEEDSUIT ) && ( !pPlayer->IsSuitEquipped() ) )
		return true;

	// Hide all HUD elements during screenshot if the user's set hud_freezecamhide ( TF2 )
#if defined( TF_CLIENT_DLL )
	extern bool IsTakingAFreezecamScreenshot();
	extern ConVar hud_freezecamhide;

	if ( IsTakingAFreezecamScreenshot() && hud_freezecamhide.GetBool() )
		return true;
#endif

	return ( ( iHudFlags & iHideHud ) != 0);
}

//-----------------------------------------------------------------------------
// Purpose: Allows HUD to modify input data
//-----------------------------------------------------------------------------
void CHud::ProcessInput( bool bActive )
{
	if ( bActive )
	{
		m_iKeyBits = input->GetButtonBits( 0 );

		// Weaponbits need to be sent down as a UserMsg now.
		gHUD.Think();
	}
}

int CHud::LookupRenderGroupIndexByName( const char *pszGroupName )
{
	return m_RenderGroupNames.Find( pszGroupName );
}

//-----------------------------------------------------------------------------
// Purpose: A hud element wants to lock this render group so other panels in the
// group do not draw
//-----------------------------------------------------------------------------
bool CHud::LockRenderGroup( int iGroupIndex, CHudElement *pLocker /* = NULL */ )
{
	// does this index exist?
	if ( !DoesRenderGroupExist(iGroupIndex) )
		return false;

	int i = m_RenderGroups.Find( iGroupIndex );

	Assert( m_RenderGroups.IsValidIndex(i) );

	CHudRenderGroup *group = m_RenderGroups.Element(i);

	Assert( group );

	if ( group )
	{
		// NULL pLocker means some higher power is globally hiding this group
		if ( pLocker == NULL )
		{
			group->bHidden = true;
		}
		else
		{
			bool bFound = false;
			// See if we have it locked already
			int iNumLockers = group->m_pLockingElements.Count();
			for ( int i=0;i<iNumLockers;i++ )
			{
				if ( pLocker == group->m_pLockingElements.Element(i) )
				{
					bFound = true;
					break;
				}
			}

			// otherwise lock us
			if ( !bFound )
				group->m_pLockingElements.Insert( pLocker );
		}

		return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: A hud element wants to release the lock on this render group 
//-----------------------------------------------------------------------------
bool CHud::UnlockRenderGroup( int iGroupIndex, CHudElement *pLocker /* = NULL */ )
{
	// does this index exist?
	if ( !DoesRenderGroupExist(iGroupIndex) )
		return false;

	int i = m_RenderGroups.Find( iGroupIndex );

	Assert( m_RenderGroups.IsValidIndex(i) );

	CHudRenderGroup *group = m_RenderGroups.Element(i);

	if ( group )
	{
		// NULL pLocker means some higher power is globally hiding this group
		if ( group->bHidden && pLocker == NULL )
		{
			group->bHidden = false;
			return true;
		}

		int iNumLockers = group->m_pLockingElements.Count();
		for ( int i=0;i<iNumLockers;i++ )
		{
			if ( pLocker == group->m_pLockingElements.Element(i) )
			{
				group->m_pLockingElements.RemoveAt( i );
				return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: See if we should draw based on a hud render group
//			Return true if this group is locked, hud elem will be hidden
//-----------------------------------------------------------------------------
bool CHud::IsRenderGroupLockedFor( CHudElement *pHudElement, int iGroupIndex )
{
	// does this index exist?
	if ( !DoesRenderGroupExist(iGroupIndex) )
		return false;

	int i = m_RenderGroups.Find( iGroupIndex );

	Assert( m_RenderGroups.IsValidIndex(i) );

	CHudRenderGroup *group = m_RenderGroups.Element(i);

	if ( !group )
		return false;

	// hidden for everyone!
	if ( group->bHidden )
		return true;

	if ( group->m_pLockingElements.Count() == 0 )
		return false;

	if ( !pHudElement )
		return true;

	CHudElement *pLocker = group->m_pLockingElements.ElementAtHead();

	return ( pLocker != pHudElement && pLocker->GetRenderGroupPriority() > pHudElement->GetRenderGroupPriority() );
}

//-----------------------------------------------------------------------------
// Purpose: CHudElements can ask for the index of hud element render groups
//			returns a group index
//-----------------------------------------------------------------------------
int CHud::RegisterForRenderGroup( const char *pszGroupName )
{
	int iGroupNameIndex = m_RenderGroupNames.Find( pszGroupName );

	if ( iGroupNameIndex != m_RenderGroupNames.InvalidIndex() )
	{	
		return iGroupNameIndex;
	}

	// otherwise add the group
	return AddHudRenderGroup( pszGroupName );
}

//-----------------------------------------------------------------------------
// Purpose: Create a new hud render group
//			returns a group index
//-----------------------------------------------------------------------------
int CHud::AddHudRenderGroup( const char *pszGroupName )
{
	// we tried to register for a group but didn't find it, add a new one

	int iGroupNameIndex = m_RenderGroupNames.AddToTail( pszGroupName );

	CHudRenderGroup *group = new CHudRenderGroup();
	return m_RenderGroups.Insert( iGroupNameIndex, group );
}

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
bool CHud::DoesRenderGroupExist( int iGroupIndex )
{
	return ( m_RenderGroups.Find( iGroupIndex ) != m_RenderGroups.InvalidIndex() );
}

//-----------------------------------------------------------------------------
// Purpose: Allows HUD to Think and modify input data
// Input  : *cdata - 
//			time - 
// Output : int - 1 if there were changes, 0 otherwise
//-----------------------------------------------------------------------------
void CHud::UpdateHud( bool bActive )
{
	// clear the weapon bits.
	gHUD.m_iKeyBits &= (~(IN_WEAPON1|IN_WEAPON2));

	g_pClientMode->Update();

	gLCD.Update();
}

//-----------------------------------------------------------------------------
// Purpose: Force a Hud UI anim to play
//-----------------------------------------------------------------------------
CON_COMMAND_F( testhudanim, "Test a hud element animation.\n\tArguments: <anim name>\n", FCVAR_CHEAT )
{
	if ( args.ArgC() != 2 )
	{
		Msg("Usage:\n   testhudanim <anim name>\n");
		return;
	}

	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( args[1] );
}

