///////////// Copyright � 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// ge_hudradar.cpp
//
// Description:
//      Ripped from valve and tweaked for Goldeneye
//		Display's a radar that is customizable via Python (server-side)!
//
// Created On: 12/2/2008
// Created By: Jonathan White <killermonkey>
////////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "hud.h"
#include "vgui_controls/Panel.h"
#include "vgui/ISurface.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "ge_shareddefs.h"
#include "c_ge_playerresource.h"
#include "c_ge_radarresource.h"
#include "gemp_gamerules.h"
#include "c_gemp_player.h"
#include "c_team.h"
#include "materialsystem/imaterial.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

struct CGERadarContact
{
	int GetEntindex( void ) {
		return m_iSerial & ((1 << MAX_EDICT_BITS) - 1);
	}

	int			m_iSerial;
	CHudTexture	*m_Icon;
	CHudTexture	*m_IconAbove;
	CHudTexture	*m_IconBelow;
	Color	m_ColorOverride;
	int		m_iType;			// What are we?
	bool	m_bAlwaysVisible;	// Used by Python if we want to ignore the max distance
	int		m_iCampingPercent;	// For player types only
	Vector	m_vScaledPos;		// World to Radar scaled position
	float	m_flAlphaMod;		// Modulation on the alpha based on position to player [0-1.0]
	float	m_flBlinkMod;		// Modulation of the camping blink [0-1.0]
};

class CGERadar : public CHudElement, public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE( CGERadar, vgui::Panel );
	
	CGERadar( const char *pElementName );
	~CGERadar();

	virtual void Init() { }
	virtual void VidInit( void );
	virtual void Reset( void );
	virtual bool ShouldDraw( void );

	virtual void OnThink( void );
	virtual void Paint( void );
	
	void UpdateContact( int iEntSerial, int type, bool bAlwaysVisible = false, Color colOverride = Color(0,0,0,0) );
	void ValidateContacts();

	int FindContact( int iEntSerial );
	
	virtual void FireGameEvent( IGameEvent *event );

	void ClearAllContacts();

	// Static colors!
	static Color	s_ColorDM;
	static Color	s_ColorJanus;
	static Color	s_ColorMI6;

private:
	void WorldToRadar( CGERadarContact *contact );

	bool IsContactCamping( int iEntSerial );
	bool IsContactInRange( const Vector location );
	bool IsContactInRange( CGERadarContact *contact );
	float GetContactRange( const Vector location );
	Vector GetContactPosition( CGERadarContact *contact );
	float GetRadarRange( void );
	void ResolveContactIcons( const char *szIcon, CGERadarContact *contact );

	void MoveContact( int oldpos, int newpos );
	void ResetContact( int pos );

	void DrawIconOnRadar( CGERadarContact *contact, Color col );

private:
	CGERadarContact	m_radarContacts[ MAX_NET_RADAR_ENTS ];
	int				m_iNumContacts;

	float			m_flNextThink;

	float			m_flScaleX;
	float			m_flScaleY;
	float			m_flRadarDiameter;
	int				m_iSideBuff;

	// Textures!
	CHudTexture		*m_Background;

	CHudTexture		*m_IconBlip;
	CHudTexture		*m_IconBlipAbove;
	CHudTexture		*m_IconBlipBelow;

	CHudTexture		*m_IconToken;
	CHudTexture		*m_IconTokenAbove;
	CHudTexture		*m_IconTokenBelow;

	void ThinkAdvance( float delay = 0.025f ) { m_flNextThink = gpGlobals->curtime + delay; }
};

// Static colors definitions!
Color CGERadar::s_ColorDM( 0, 185, 0, 255 );
Color CGERadar::s_ColorMI6( 134, 210, 255, 255 );
Color CGERadar::s_ColorJanus( 202, 47, 47, 255 );

typedef struct
{
	const char *x;
	const char *y;
} sRadarPos;

static const sRadarPos GERadarPos[] = {
	{"c-40", "378"},
	{"0", "c-40"},
	{"c-40", "0" },
	{"r80", "0" },
	{"r80", "c-40" },
};

void GERadarPos_Callback( IConVar *var, const char *pOldString, float flOldValue )
{
	static bool denyReentrant = false;
	if ( denyReentrant )
		return;

	denyReentrant = true;

	// Bound our value to 5
	ConVar *cVar = static_cast<ConVar*>(var);
	if ( cVar->GetInt() > 5 )
		cVar->SetValue( 5 );

	denyReentrant = false;

	// cVar <= 0 implies use HudLayout.res
	CGERadar *radar = GET_HUDELEMENT( CGERadar );
	if ( !radar || cVar->GetInt() <= 0 )
		return;

	KeyValues *kv = new KeyValues("settings");
	int pos = cVar->GetInt() - 1;
	kv->SetString( "xpos", GERadarPos[pos].x );
	kv->SetString( "ypos", GERadarPos[pos].y );

	radar->ApplySettings( kv );

	kv->deleteThis();
}

extern ConVar ge_allowradar;
extern ConVar ge_radar_range;
extern ConVar ge_radar_showenemyteam;

ConVar cl_ge_showradar( "cl_ge_showradar", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL, "Toggles the radar if allowed in the server" );
ConVar cl_ge_radarpos( "cl_ge_radarpos", "0", FCVAR_ARCHIVE | FCVAR_CLIENTDLL, "Positions the radar in set locations clockwise from middle-bottom [0:5]", GERadarPos_Callback );

DECLARE_HUDELEMENT( CGERadar );

//---------------------------------------------------------
//---------------------------------------------------------
CGERadar::CGERadar( const char *pElementName ) : 
	CHudElement( pElementName ), BaseClass( NULL, "GERadar" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_PLAYERDEAD | HIDEHUD_HEALTH );

	m_Background = NULL;
	m_IconBlip = NULL;
	m_IconToken = NULL;
	m_IconTokenAbove = NULL;
	m_IconTokenBelow = NULL;
	m_IconBlipAbove = NULL;
	m_IconBlipBelow = NULL;

	m_flScaleX = 1.0f;
	m_flScaleY = 1.0f;
	m_iSideBuff = 8;
	m_flRadarDiameter = 1.0f;

	ListenForGameEvent( "round_start" );
	ListenForGameEvent( "player_team" );
}

//---------------------------------------------------------
//---------------------------------------------------------
CGERadar::~CGERadar()
{
}

// From CHudElement
void CGERadar::VidInit( void )
{
	// Load up our default icons
	m_Background	 = gHUD.GetIcon("ge_radar_bg");
	m_IconBlip		 = gHUD.GetIcon("ge_radar_blip");
	m_IconToken		 = gHUD.GetIcon("ge_radar_token");
	m_IconTokenAbove = gHUD.GetIcon("ge_radar_token_up");
	m_IconTokenBelow = gHUD.GetIcon("ge_radar_token_down");
	m_IconBlipAbove	 = gHUD.GetIcon("ge_radar_up");
	m_IconBlipBelow  = gHUD.GetIcon("ge_radar_down");

	Reset();
}

void CGERadar::Reset( void )
{
	ClearAllContacts();
	// Think NOW!
	ThinkAdvance(0);

	if ( m_Background )
	{
		m_flScaleX = GetWide() / m_Background->Width();
		m_flScaleY = GetTall() / m_Background->Height();
	}

	// Curtail anyone from making the radar into a permanent crosshair by not allowing it anywhere near the center of the screen
	int x, y, screenW, screenH;
	GetPos( x, y );
	surface()->GetScreenSize( screenW, screenH );

	if ( x > (screenW * 0.4) && x < (screenW * 0.6) && y > (screenH * 0.4) && y < (screenH * 0.6) )
	{
		x = (screenW * 0.6);
		y = (screenH * 0.6);
	}

	m_iSideBuff = scheme()->GetProportionalScaledValue( 8 );

	// Hard code base size of 64 to prevent haxing of the radar
	int size = scheme()->GetProportionalScaledValue( 64 ) + m_iSideBuff + m_iSideBuff;
	SetBounds( x, y, size, size );

	m_flRadarDiameter = size - m_iSideBuff - m_iSideBuff;
}

void CGERadar::FireGameEvent( IGameEvent *event )
{
	if ( !Q_strcmp( event->GetName(), "round_start" ) )
	{
		// Purge all our contacts at the start of a round
		ClearAllContacts();
	}
	else if ( !Q_strcmp( event->GetName(), "player_team" ) )
	{
		// Figure out which background we should use
		// TODO: This should probably be in real-time code to catch observer team...
		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
		if ( pPlayer && pPlayer->GetUserID() == event->GetInt("userid") )
		{
			int team = event->GetInt( "team" );
			if ( team == TEAM_MI6 )
				m_Background = gHUD.GetIcon( "ge_radar_bg_mi6" );
			else if ( team == TEAM_JANUS )
				m_Background = gHUD.GetIcon( "ge_radar_bg_janus" );
			else
				m_Background = gHUD.GetIcon("ge_radar_bg");
		}

		Reset();
	}
}

bool CGERadar::ShouldDraw( void )
{
	// Forget about drawing without our resource
	if ( !g_RR )
		return false;

	// Never draw if our CVars don't allow it
	// unless we are forcing it with our gameplay
	if ( (ge_allowradar.GetInt() == 0 || cl_ge_showradar.GetInt() == 0) && !g_RR->ForceRadarOn() )
		return false;

	// Don't show if we are in intermission time!
	if ( GEMPRules() && GEMPRules()->IsIntermission() )
		return false;

	// Always show if we are observing and passed the tests above
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer && pPlayer->IsObserver() )
		return true;

	return CHudElement::ShouldDraw();
}

void CGERadar::OnThink( void )
{
	if ( m_flNextThink > gpGlobals->curtime )
		return;

	// Don't worry about the radar if we don't even have a resource!
	if ( !g_RR )
	{
		ThinkAdvance( 0.5f );
		return;
	}

	// Check to see if we are allowed to show the radar at all
	if ( (ge_allowradar.GetInt() == 0 || cl_ge_showradar.GetInt() == 0) && !g_RR->ForceRadarOn()  )
	{
		ClearAllContacts();
		ThinkAdvance( 0.3f );
		return;
	}

	CBasePlayer *pLocalPlayer = CBasePlayer::GetLocalPlayer();

	// Go through our Radar Resource to get any new entities / updates to existing ones
	int iEntSerial;
	int iLocalSerial = pLocalPlayer->GetRefEHandle().ToInt();
	for( int i=0; i < MAX_NET_RADAR_ENTS; ++i )
	{
		iEntSerial = g_RR->GetEntSerial(i);
		if ( iEntSerial != INVALID_EHANDLE_INDEX && g_RR->GetState(i) == RADAR_STATE_DRAW )
		{
			// Don't add enemy teams if the convar isn't set
			if ( GEMPRules()->IsTeamplay() && !ge_radar_showenemyteam.GetBool() && g_RR->GetEntTeam(i) != pLocalPlayer->GetTeamNumber() && g_RR->GetType(i) == RADAR_TYPE_PLAYER )
				continue;

			// Don't add yourself
			if ( iEntSerial == iLocalSerial )
				continue;

			// Don't add the current observed target
			if ( pLocalPlayer->IsObserver() && pLocalPlayer->GetObserverTarget() )
			{
				if ( pLocalPlayer->GetObserverTarget()->GetRefEHandle().ToInt() == iEntSerial )
					continue;
			}

			// Never add a contact that is out of range initially or set to not draw on the radar (don't include campers!)
			// Don't show contacts the gamerules say we shouldn't.



			// Never add a contact that is out of range initially or set to not draw on the radar (don't include campers!)
			if ( !g_RR->GetAlwaysVisible(i) && !IsContactCamping(iEntSerial) && !IsContactInRange(g_RR->GetOrigin(i)) )
				continue;

			UpdateContact( iEntSerial, g_RR->GetType(i), g_RR->GetAlwaysVisible(i), g_RR->GetColor(i) );
		}
	}

	ValidateContacts();

	ThinkAdvance( 0.025f );
}

//---------------------------------------------------------
// Purpose: Register a radar contact in the list of contacts
//---------------------------------------------------------
void CGERadar::UpdateContact( int iEntSerial, int type, bool bAlwaysVisible /*= false*/, Color colOverride /*= Color(0,0,0)*/ )
{
	// Never add the local player, and don't overflow the buffer
	if( m_iNumContacts == MAX_NET_RADAR_ENTS )
		return;

	// See if we already have him in the list, if so update the information
	int id = FindContact( iEntSerial );

	// If this is a new contact add it to the end of the list
	if( id == -1 )
	{
		id = m_iNumContacts;
		m_iNumContacts++;
	}
	
	ResolveContactIcons( g_RR->GetIcon( g_RR->FindEntityIndex(iEntSerial) ), &m_radarContacts[id] );

	m_radarContacts[id].m_iSerial = iEntSerial;
	m_radarContacts[id].m_iType = type;
	m_radarContacts[id].m_ColorOverride = colOverride;

	// Resolve their scaled position on the radar
	WorldToRadar( &m_radarContacts[id] );

	// For camping checks
	m_radarContacts[id].m_iCampingPercent = GEPlayerRes()->GetCampingPercent( m_radarContacts[id].GetEntindex() );
	m_radarContacts[id].m_bAlwaysVisible = (m_radarContacts[id].m_iCampingPercent >= RADAR_CAMP_ALLVIS_PERCENT) ? true : bAlwaysVisible;
}

//---------------------------------------------------------
// Purpose: Go through all client known radar targets and see if any
//			are too far away to show. If yes, remove them from the
//			list.
//---------------------------------------------------------
void CGERadar::ValidateContacts()
{
	// Nothing to do without the radar resource
	if ( !g_RR )
		return;

	bool bRemove = false;
	CBasePlayer *pLocalPlayer = CBasePlayer::GetLocalPlayer();
	CGERadarContact *pContact;
	int index = -1;

	// Loop through all our contacts and remove the ones that are out of range
	for( int i = 0 ; i < m_iNumContacts ; i++ )
	{
		bRemove = false;

		pContact = &m_radarContacts[i];
		index = g_RR->FindEntityIndex( pContact->m_iSerial );

		if ( index != -1 && pContact->m_iSerial != INVALID_EHANDLE_INDEX )
		{
			// Update state and visibility
			if( g_RR->GetState(index) != RADAR_STATE_DRAW || !IsContactInRange( pContact ) )
				bRemove = true;

			// Update team status
			if ( pContact->m_iType == RADAR_TYPE_PLAYER )
			{
				// Make sure we haven't changed teams and are tracking this sort of thing
				if ( GEMPRules()->IsTeamplay() && !ge_radar_showenemyteam.GetBool() && g_RR->GetEntTeam(index) != pLocalPlayer->GetTeamNumber() )
					bRemove = true;

				// Remove the currently observed target
				if ( pLocalPlayer->IsObserver() && pLocalPlayer->GetObserverTarget() )
				{
					if ( pLocalPlayer->GetObserverTarget()->GetRefEHandle().ToInt() == pContact->m_iSerial )
						bRemove = true;
				}
			}
		}
		else
			bRemove = true;

		if ( bRemove )
		{
			// Time for this guy to go. Easiest thing is just to copy the last element 
			// into this element's spot and then decrement the count of entities.
			// We also decrement our current index so we can check this new guy next
			MoveContact( --m_iNumContacts, i-- );
		}
	} 
}

void CGERadar::WorldToRadar( CGERadarContact *contact )
{
	// If we are out of range don't bother
	if ( !IsContactInRange( contact ) )
		return;

	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pLocalPlayer )
		return;

	Vector vContact = GetContactPosition(contact);

	float x_diff = vContact.x - pLocalPlayer->GetAbsOrigin().x;
	float y_diff = vContact.y - pLocalPlayer->GetAbsOrigin().y;

	// Supply epsilon values to avoid divide-by-zero
	if(x_diff == 0.0f )
		x_diff = 0.001f;

	if(y_diff == 0.0f )
		y_diff = 0.001f;

	float fRadarRadius = m_flRadarDiameter * 0.500f;
	float fRange = GetRadarRange();
	float fScale = fRange >= 1.000f ? fRadarRadius / fRange : 0.000f;
	float dist = min( GetContactRange( vContact ), fRange );

	float flOffset = atan(y_diff/x_diff);
	float flPlayerY;

	// If we're in first person spectate mode we should use the rotation of whoever we're spectating.
	if (pLocalPlayer->IsObserver() && pLocalPlayer->GetObserverTarget() && pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE)
		flPlayerY = (pLocalPlayer->GetLocalAngles().y * M_PI) * 0.0055555f;
	else
		flPlayerY = (pLocalPlayer->LocalEyeAngles().y * M_PI) * 0.0055555f;

	// Always add because atan will return neg angle w/ neg coeff
	if ( x_diff < 0 )
		flOffset += M_PI;
	else
		flOffset += M_TWOPI;

	flOffset = flPlayerY - flOffset;

	// Transform relative to radar source
	float xnew_diff = dist * sin(flOffset);
	float ynew_diff = -dist * cos(flOffset);

	// Scale the dot's position to match radar scale
	xnew_diff *= fScale;
	ynew_diff *= fScale;

	// Make sure we never leave our radar circle!
	contact->m_vScaledPos.x = fRadarRadius + xnew_diff + m_iSideBuff;
	contact->m_vScaledPos.y = fRadarRadius + ynew_diff + m_iSideBuff;
	contact->m_vScaledPos.z = vContact.z - pLocalPlayer->GetAbsOrigin().z;

	// Figure out our alpha modulation
	if ( !contact->m_bAlwaysVisible && dist > fRange * 0.8f )
		contact->m_flAlphaMod = RemapValClamped( dist, fRange * 0.8f, fRange, 1.0, 0 );
	else
		contact->m_flAlphaMod = 1.0f;
}

//---------------------------------------------------------
// Purpose: Draw the radar panel.
//			We're probably doing too much other work in here
//---------------------------------------------------------
void CGERadar::Paint()
{
	// Check to make sure we loaded our icons
	if ( !m_Background )
		Init();

	// Draw the radar background.
	int bgAlpha = 225;
	if ( C_BasePlayer::GetLocalPlayer() && C_BasePlayer::GetLocalPlayer()->IsObserver() )
		bgAlpha = 255;

	m_Background->DrawSelf(m_iSideBuff, m_iSideBuff, m_flRadarDiameter, m_flRadarDiameter, Color(255,255,255,bgAlpha));

	// Now go through the list of radar targets and represent them on the radar screen
	// by drawing their icons on top of the background.
	for( int i = 0 ; i < m_iNumContacts ; i++ )
	{
		int alpha = 140;
		CGERadarContact *pContact = &m_radarContacts[ i ];
		
		Color col( 255, 255, 255, 255 );
		if ( pContact->m_iType == RADAR_TYPE_PLAYER )
		{
			// Players default to DM color
			col.SetRawColor( s_ColorDM.GetRawColor() );

			if ( GEMPRules()->IsTeamplay() )
			{
				CBasePlayer *pLocalPlayer = CBasePlayer::GetLocalPlayer();
				int team = GEPlayerRes()->GetTeam( pContact->GetEntindex() );

				// Set the color to their team
				if ( team == TEAM_MI6 )
					col.SetRawColor( s_ColorMI6.GetRawColor() );
				else
					col.SetRawColor( s_ColorJanus.GetRawColor() );

				// Enemies are higher opacity so they stand out
				if ( team != pLocalPlayer->GetTeamNumber() )
					alpha = 200;
			}
			else
			{
				// Calculate our display for campers (only non-teamplay!)
				if ( pContact->m_iCampingPercent > 0 )
				{
					alpha = RemapValClamped( pContact->m_iCampingPercent, 0, RADAR_CAMP_ALLVIS_PERCENT, alpha, 255 );

					if ( pContact->m_iCampingPercent > RADAR_CAMP_ALLVIS_PERCENT )
					{
						// Make the dot more intensely colored green
						float blink = SmoothCurve( pContact->m_flBlinkMod );
						col[1] = col[1] + blink * (255-col[1]);

						// Modulate the blink
						pContact->m_flBlinkMod += 0.5f * gpGlobals->frametime;
						if ( pContact->m_flBlinkMod > 1.0f )
							pContact->m_flBlinkMod = 0;
					}
				}
				else
				{
					pContact->m_flBlinkMod = 0;
				}
			}
		}

		col[3] = alpha;

		// If we have an override color make sure we display it instead of the above
		if ( pContact->m_ColorOverride[3] != 0 )
			col = pContact->m_ColorOverride;

		DrawIconOnRadar( pContact, col );
	}
}

//---------------------------------------------------------
// Purpose: Compute the proper position on the radar screen
//			for this object's position relative to the player.
//			Then draw the icon in the proper location on the
//			radar screen.
//---------------------------------------------------------
void CGERadar::DrawIconOnRadar( CGERadarContact *contact, Color col )
{
	float x = contact->m_vScaledPos.x, 
		y = contact->m_vScaledPos.y,
		z_delta = contact->m_vScaledPos.z;

	int width, height;

	float floorheight = GEMPRules()->GetMapFloorHeight();

	// Get the correct icon for this type of contact
	CHudTexture *icon = NULL;

	if ( contact->m_Icon )
	{
		// We have a custom icon!
		if (z_delta > floorheight && contact->m_IconAbove)
			icon = contact->m_IconAbove;
		else if (z_delta < floorheight * -1 && contact->m_IconBelow)
			icon = contact->m_IconBelow;
		else
			icon = contact->m_Icon;

		width = max( icon->EffectiveWidth(m_flScaleX), 16 );
		height = max( icon->EffectiveHeight(m_flScaleY), 16 );
	}
	else if ( contact->m_iType == RADAR_TYPE_TOKEN )
	{
		if ( z_delta > floorheight )
			icon = m_IconTokenAbove;
		else if ( z_delta < floorheight * -1 )
			icon = m_IconTokenBelow;
		else
			icon = m_IconToken;

		width = max( icon->EffectiveWidth(m_flScaleX), 8 );
		height = max( icon->EffectiveHeight(m_flScaleY), 8 );
	}
	else
	{
		if ( z_delta > floorheight )
			icon = m_IconBlipAbove;
		else if ( z_delta < floorheight * -1 )
			icon = m_IconBlipBelow;
		else
			icon = m_IconBlip;

		width = max( icon->EffectiveWidth(m_flScaleX), 8 );
		height = max( icon->EffectiveHeight(m_flScaleY), 8 );
	}

	// Double check we are inside the radar bounds
	if ( x >= m_iSideBuff && y >= m_iSideBuff )
	{
		// Center the icon around its position.
		x -= (width >> 1);
		y -= (height >> 1);

		vgui::surface()->DrawSetColor( col[0], col[1], col[2], col[3]*contact->m_flAlphaMod );
		vgui::surface()->DrawSetTexture( icon->textureId );
		vgui::surface()->DrawTexturedRect(x, y, x + width, y + height);
	}
}

bool CGERadar::IsContactCamping( int iEntSerial )
{
	int index = iEntSerial & ((1 << MAX_EDICT_BITS) - 1);
	return GEPlayerRes()->GetCampingPercent( index ) >= RADAR_CAMP_ALLVIS_PERCENT;
}

bool CGERadar::IsContactInRange( const Vector location )
{
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pLocalPlayer )
		return false;

	return location.DistTo( pLocalPlayer->GetAbsOrigin() ) < GetRadarRange();
}

bool CGERadar::IsContactInRange( CGERadarContact *contact )
{
	if ( contact->m_bAlwaysVisible )
		return true;

	return IsContactInRange( GetContactPosition(contact) );
}

float CGERadar::GetContactRange( const Vector location )
{
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pLocalPlayer )
		return false;
	Vector dist = pLocalPlayer->GetAbsOrigin() - location;

	return dist.Length2D();
}

Vector CGERadar::GetContactPosition( CGERadarContact *contact )
{
	if ( !contact || !g_RR )
		return vec3_origin;

	return g_RR->GetOrigin( g_RR->FindEntityIndex( contact->m_iSerial ) );
}

float CGERadar::GetRadarRange( void )
{
	CBasePlayer *pLocalPlayer = CBasePlayer::GetLocalPlayer();
	float flMod = g_RR->GetRangeModifier( g_RR->FindEntityIndex( pLocalPlayer->GetRefEHandle().ToInt() ) );
	return ge_radar_range.GetFloat() * flMod;
}

void CGERadar::ResolveContactIcons( const char *szIcon, CGERadarContact *contact )
{
	if ( !contact ) return;

	char fn[64];
	Q_FileBase( szIcon, fn, 64 );

	if ( !szIcon || szIcon[0] == '\0' )
	{
		contact->m_Icon = contact->m_IconAbove = contact->m_IconBelow = NULL;
		return;
	}
	else if ( contact->m_Icon && !Q_stricmp( contact->m_Icon->szShortName, fn ) )
	{
		// We didn't change our icon
		return;
	}

	char shortname[64];

	// Try to find it with it's name
	CHudTexture *pIcon = gHUD.GetIcon( szIcon );
	if ( pIcon )
	{
		contact->m_Icon = pIcon;

		Q_snprintf( shortname, 64, "%s_%s", szIcon, "above" );
		contact->m_IconAbove = gHUD.GetIcon( shortname );

		Q_snprintf( shortname, 64, "%s_%s", szIcon, "below" );
		contact->m_IconBelow = gHUD.GetIcon( shortname );

		return;
	}
	
	// See if we previously registered this icon from a file input
	pIcon = gHUD.GetIcon( fn );
	if ( pIcon )
	{
		contact->m_Icon = pIcon;

		Q_snprintf( shortname, 64, "%s_%s", fn, "above" );
		contact->m_IconAbove = gHUD.GetIcon( shortname );

		Q_snprintf( shortname, 64, "%s_%s", fn, "below" );
		contact->m_IconBelow = gHUD.GetIcon( shortname );

		return;
	}

	// We failed to find it, so add it as a new icon to the system
	// Get our dimensions
	int id, wide, tall;
	id = vgui::surface()->CreateNewTextureID();
	vgui::surface()->DrawSetTextureFile( id, szIcon, false, false );
	vgui::surface()->DrawGetTextureSize( id, wide, tall );

	// We must have sent a texture path, try to make it
	CHudTexture icon;
	icon.rc.right = wide;
	icon.rc.bottom = tall;

	Q_strncpy( icon.szTextureFile, szIcon, sizeof(icon.szTextureFile) );
	Q_strncpy( icon.szShortName, fn, 64 );
	contact->m_Icon = gHUD.AddSearchableHudIconToList( icon );

	contact->m_IconAbove = contact->m_IconBelow = NULL;

	char file[128];
	Q_snprintf( file, 128, "%s_%s", szIcon, "above" );
	if ( !IsErrorMaterial( materials->FindMaterial(file, TEXTURE_GROUP_VGUI, false) ) )
	{
		Q_snprintf( shortname, 64, "%s_%s", fn, "above" );
		Q_strncpy( icon.szTextureFile, file, sizeof(icon.szTextureFile) );
		Q_strncpy( icon.szShortName, shortname, sizeof(icon.szShortName) );
		contact->m_IconAbove = gHUD.AddSearchableHudIconToList( icon );
	}

	Q_snprintf( file, 128, "%s_%s", szIcon, "below" );
	if ( !IsErrorMaterial( materials->FindMaterial(file, TEXTURE_GROUP_VGUI, false) ) )
	{
		Q_snprintf( shortname, 64, "%s_%s", fn, "below" );
		Q_strncpy( icon.szTextureFile, file, sizeof(icon.szTextureFile) );
		Q_strncpy( icon.szShortName, shortname, sizeof(icon.szShortName) );
		contact->m_IconBelow = gHUD.AddSearchableHudIconToList( icon );
	}
}

//---------------------------------------------------------
// Purpose: Search the contact list for a specific contact
//---------------------------------------------------------
int CGERadar::FindContact( int iEntSerial )
{
	for( int i = 0 ; i < m_iNumContacts ; i++ )
	{
		if( m_radarContacts[i].m_iSerial == iEntSerial )
			return i;
	}

	return -1;
}

void CGERadar::MoveContact( int oldpos, int newpos )
{
	if ( oldpos < 0 || oldpos >= MAX_NET_RADAR_ENTS )
		return;
	if ( newpos < 0 || newpos >= MAX_NET_RADAR_ENTS )
		return;

	m_radarContacts[newpos].m_iSerial = m_radarContacts[oldpos].m_iSerial;
	m_radarContacts[newpos].m_iType = m_radarContacts[oldpos].m_iType;
	m_radarContacts[newpos].m_bAlwaysVisible = m_radarContacts[oldpos].m_bAlwaysVisible;
	m_radarContacts[newpos].m_ColorOverride = m_radarContacts[oldpos].m_ColorOverride;
	m_radarContacts[newpos].m_iCampingPercent = m_radarContacts[oldpos].m_iCampingPercent;
	m_radarContacts[newpos].m_Icon = m_radarContacts[oldpos].m_Icon;
	m_radarContacts[newpos].m_IconAbove = m_radarContacts[oldpos].m_IconAbove;
	m_radarContacts[newpos].m_IconBelow = m_radarContacts[oldpos].m_IconBelow;
	m_radarContacts[newpos].m_vScaledPos = m_radarContacts[oldpos].m_vScaledPos;
	m_radarContacts[newpos].m_flAlphaMod = m_radarContacts[oldpos].m_flAlphaMod;

	ResetContact( oldpos );
}

void CGERadar::ClearAllContacts( void )
{
	for ( int i=0; i < MAX_NET_RADAR_ENTS; ++i )
		ResetContact(i);

	m_iNumContacts = 0;
}

void CGERadar::ResetContact( int pos )
{
	if ( pos < 0 || pos >= MAX_NET_RADAR_ENTS )
		return;

	m_radarContacts[pos].m_iSerial = INVALID_EHANDLE_INDEX;
	m_radarContacts[pos].m_iType = 0;
	m_radarContacts[pos].m_bAlwaysVisible = false;
	m_radarContacts[pos].m_ColorOverride = Color(0,0,0,0);
	m_radarContacts[pos].m_iCampingPercent = 0;
	m_radarContacts[pos].m_Icon = NULL;
	m_radarContacts[pos].m_IconAbove = NULL;
	m_radarContacts[pos].m_IconBelow = NULL;
	m_radarContacts[pos].m_vScaledPos = vec3_origin;
	m_radarContacts[pos].m_flAlphaMod = 1.0f;
	m_radarContacts[pos].m_flBlinkMod = 0;
}
