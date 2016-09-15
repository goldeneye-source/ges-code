///////////// Copyright � 2011, Goldeneye: Source. All rights reserved. /////////////
// 
// ge_hudobjectives.cpp
//
// Description:
//      Display's on-screen pips to indicate gameplay objectives
//
// Created On: 11/18/11
// Created By: Jonathan White <killermonkey>
////////////////////////////////////////////////////////////////////////////////
#include "cbase.h"
#include "hud.h"
#include "vgui_controls/Panel.h"
#include "vgui/ISurface.h"
#include "vgui/ILocalize.h"
#include "hudelement.h"
#include "iclientmode.h"
#include "ge_shareddefs.h"
#include "ge_utils.h"
#include "gemp_gamerules.h"
#include "c_ge_playerresource.h"
#include "c_ge_radarresource.h"
#include "c_gemp_player.h"
#include "c_gebot_player.h"
#include "c_team.h"
#include "materialsystem/imaterial.h"
#include "view.h"
#include "engine/ivdebugoverlay.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#define OBJ_ABS_MIN_DIST 35.0f

struct GEObjective
{
	EHANDLE		hEnt;
	Vector		last_pos;
	Vector		curr_pos;
	Color		color;
	char		refText[32];
	wchar_t		text[16];
	int			txtW, txtH;
	float		min_dist;
	bool		pulse;
	float		pulse_mod;
	// Tracker variables
	float		update_time;
	bool		valid;
};

class CGEObjectives : public CHudElement, public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE( CGEObjectives, vgui::Panel );

	CGEObjectives( const char *pElementName );
	~CGEObjectives();

	virtual void Init() { }
	virtual void VidInit( void );
	virtual void Reset( void );
	virtual bool ShouldDraw( void );

	virtual void OnThink( void );
	virtual void Paint( void );

	virtual void FireGameEvent( IGameEvent *event );

	// Mainly for team icon drawing purposes
	bool IsObjective( CBaseEntity *pEnt );

protected:
	void ClearAll();
	GEObjective *FindObjective( int serial );

private:
	CHudTexture *m_pCardTexture;
	float	m_flNextThink;
	int		m_iInnerBound;

	CUtlVector<GEObjective*> m_vObjectives;

	void ThinkAdvance( float delay = 0.025f ) { m_flNextThink = gpGlobals->curtime + delay; }
};

ConVar cl_ge_showobjectives( "cl_ge_showobjectives", "1", FCVAR_ARCHIVE | FCVAR_CLIENTDLL, "Show objective cards on-screen or not" );

DECLARE_HUDELEMENT( CGEObjectives );

CGEObjectives::CGEObjectives( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "GEObjectives" )
{
	SetParent( g_pClientMode->GetViewport() );

	m_flNextThink = 0;

	ListenForGameEvent( "round_end" );

	SetHiddenBits( HIDEHUD_PLAYERDEAD | HIDEHUD_WEAPONSELECTION );
}

CGEObjectives::~CGEObjectives()
{
	m_vObjectives.PurgeAndDeleteElements();
}

void CGEObjectives::VidInit( void )
{
	m_iInnerBound = XRES(8);
	m_pCardTexture = gHUD.GetIcon( "ic_objective_card" );
	SetSize( ScreenWidth(), ScreenHeight() );
}

void CGEObjectives::Reset( void )
{
	ThinkAdvance( 0 );
}

bool CGEObjectives::ShouldDraw( void )
{
	// Forget about drawing without our resource
	if ( !g_RR )
		return false;

	// Never draw if our CVars don't allow it
	// unless we are forcing it with our gameplay
	if ( cl_ge_showobjectives.GetInt() == 0 )
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

void CGEObjectives::OnThink( void )
{
	if ( m_flNextThink > gpGlobals->curtime )
		return;

	// If we aren't drawing, don't think
	CBasePlayer *pLocalPlayer = CBasePlayer::GetLocalPlayer();
	if ( !ShouldDraw() || !pLocalPlayer )
	{
		ThinkAdvance( 0.5f );
		return;
	}

	// Clear our validity flags
	for ( int i=0; i < m_vObjectives.Count(); i++ )
		m_vObjectives[i]->valid = false;

	int plr_serial = pLocalPlayer->GetRefEHandle().ToInt();
	vgui::HFont text_font = scheme()->GetIScheme( GetScheme() )->GetFont("GETargetID", true);

	// Go through our Radar Resource to get any new entities / updates to existing ones
	for( int i=0; i < MAX_NET_RADAR_ENTS; ++i )
	{
		if ( !g_RR->IsObjective(i) || g_RR->GetState(i) != RADAR_STATE_DRAW )
			continue;

		int team = g_RR->GetObjectiveTeam(i);
		int serial = g_RR->GetEntSerial(i);
		const char *tkn = g_RR->GetObjectiveToken(i);
		bool tkn_held = true;
		if ( tkn[0] == '!' )
		{
			tkn++;
			tkn_held = false;
		}

		if ( serial != plr_serial
			&& (team == TEAM_UNASSIGNED || team == pLocalPlayer->GetTeamNumber())
			&& (!tkn[0] || (pLocalPlayer->Weapon_OwnsThisType(tkn) != NULL) == tkn_held)
			&& (!pLocalPlayer->GetObserverTarget() || pLocalPlayer->GetObserverTarget()->GetRefEHandle().ToInt() != serial))
		{
			GEObjective *obj = FindObjective( serial );
			if ( !obj )
			{
				// New objective! Initialize us
				obj = new GEObjective;
				obj->hEnt = EHANDLE::FromIndex( serial );
				obj->curr_pos = g_RR->GetOrigin(i);
				obj->pulse_mod = 0;
				obj->refText[0] = '\0';
				obj->text[0] = L'\0';
				m_vObjectives.AddToTail( obj );
			}
			
			// Ensure text is the same
			if ( Q_strcmp( obj->refText, g_RR->GetObjectiveText(i) ) )
			{
				Q_strncpy( obj->refText, g_RR->GetObjectiveText(i), 32 );
				GEUTIL_ParseLocalization( obj->text, 16, g_RR->GetObjectiveText(i) );
				GEUTIL_GetTextSize( obj->text, text_font, obj->txtW, obj->txtH );
			}
			
			obj->last_pos = obj->curr_pos;
			obj->curr_pos = g_RR->GetOrigin(i);
			obj->min_dist = max( g_RR->GetObjectiveMinDist(i), OBJ_ABS_MIN_DIST );
			obj->update_time = gpGlobals->curtime;
			obj->pulse = g_RR->GetObjectivePulse(i);
			obj->color = g_RR->GetObjectiveColor(i);
			obj->valid = true;
		}
	}

	// Delete defunct objectives
	for ( int i=0; i < m_vObjectives.Count(); i++ )
	{
		if ( !m_vObjectives[i]->valid )
		{
			GEObjective *obj = m_vObjectives[i];
			m_vObjectives.FastRemove(i);
			delete obj;
		}
	}

	ThinkAdvance( RADAR_THINK_INT * 0.75f );
}

void CGEObjectives::Paint( void )
{
	C_GEPlayer *pPlayer = ToGEPlayer( C_BasePlayer::GetLocalPlayer() );
	if ( !pPlayer || !m_pCardTexture )
		return;

	Vector myEyePos = pPlayer->EyePosition();
	Vector myAbsPos = pPlayer->GetAbsOrigin();

	int half_w = ScreenWidth()  / 2, 
		half_h = ScreenHeight() / 2,
		card_w = XRES(20),
		card_h = XRES(20),
		buffer = XRES(5),
		off_upleft_x	= m_iInnerBound,
		off_lowleft_x	= m_iInnerBound,
		off_upright_x	= ScreenWidth() - m_iInnerBound - card_w,
		off_lowright_x	= ScreenWidth() - m_iInnerBound - card_w;

	vgui::HFont text_font = scheme()->GetIScheme( GetScheme() )->GetFont("GETargetID", true);

	for ( int i=0; i < m_vObjectives.Count(); i++ )
	{
		GEObjective *obj = m_vObjectives[i];

		Vector screen;
		Vector objAbsPos;
		// Calculate the current position based off of interpolation
		VectorLerp(obj->last_pos, obj->curr_pos, (gpGlobals->curtime - obj->update_time) / RADAR_THINK_INT, objAbsPos ); 
		// This lets us mess with the objective position without losing our "real" position
		Vector objPos = objAbsPos;
		
		int	x = 0, y = 0;
		float theta = 0;

		CBaseEntity *pEnt = obj->hEnt.Get();
		if ( pEnt )
		{
			// If we aren't dormant use our known position
			if ( !pEnt->IsDormant() )
			{
				// Try to get our NPC's position if we are really a bot
				C_GEBotPlayer *pBot = ToGEBotPlayer( pEnt );
				if ( pBot && pBot->GetNPC() )
					objPos = pBot->GetNPC()->GetAbsOrigin();
				else
					objPos = pEnt->GetAbsOrigin();
			}

			// Add our collision box which (usually) surrounds the entire entity
			float height = pEnt->CollisionProp()->OBBMaxs().z;

			// Add additional z for player specific reasons
			if ( pEnt->IsPlayer() )
				objPos.z += height + ToGEPlayer(pEnt)->GetHeadOffset();
			else
				objPos.z += max( height, 40.0f );
		}

		// Calculate parameters
		Vector toEye = objPos - myEyePos;
		float dist = toEye.Length();

		// Check out min distance parameter
		float alpha_mod = 1.0f;
		if ( obj->min_dist > 0 )
			alpha_mod = RemapValClamped( dist, obj->min_dist, obj->min_dist + 80, 0, 1.0f );

		if ( alpha_mod == 0 )
			continue;

		// Find where on the screen to draw the indicator
		// Note: we don't draw it behind us if we are really close
		bool behind = ScreenTransform( objPos, screen ) && dist > OBJ_ABS_MIN_DIST;

		if ( !behind && screen.x > -1.0 && screen.x < 1.0 )
		{
			x = half_w + screen.x*half_w - card_w/2;
			y = half_h - screen.y*half_h - card_h;

			// We are off-screen vertically, flip the card
			if ( screen.y > 1.0 )
				theta = M_PI;
		}
		else
		{
			bool onLeft = screen.x < 0;

			// Calculate a new position for our object to make it "on-screen" at the same height
			Vector forward;
			pPlayer->EyeVectors( &forward );

			Vector newPos;
			VectorMA( myEyePos, dist, forward, newPos );

			newPos.z = objPos.z;
			if ( abs(toEye.z) > 100.0f )
				newPos.z += toEye.z;
			
			//debugoverlay->AddBoxOverlay( newPos, Vector(-5), Vector(5), vec3_angle, 255, 0, 0, 200, gpGlobals->frametime );

			// Find our new screen coordinates
			ScreenTransform( newPos, screen );

			if ( screen.y > 0.97f )
			{
				// The obj is above us and off-screen
				y = m_iInnerBound;

				if ( onLeft )
				{
					x = off_upleft_x;
					theta = 0.75 * M_PI;
					off_upleft_x += card_w + buffer;
				}
				else
				{
					x = off_upright_x;
					theta = 1.25 * M_PI;
					off_upright_x -= card_w + buffer;
				}
			}
			else if ( screen.y < -0.97f )
			{
				// The obj is below us and off-screen
				y = ScreenHeight() - m_iInnerBound;

				if ( onLeft )
				{
					x = off_lowleft_x;
					theta = 0.25 * M_PI;
					off_lowleft_x += card_w + buffer;
				}
				else
				{
					x = off_lowright_x;
					theta = -0.25 * M_PI;
					off_lowright_x -= card_w + buffer;
				}
			}
			else
			{
				// The obj is "on-screen" and level with us
				if ( onLeft )
				{
					x = m_iInnerBound;
					y = half_h - screen.y*half_h;
					theta = M_HALFPI;
				}
				else
				{
					x = ScreenWidth() - m_iInnerBound - card_w;
					y = half_h - screen.y*half_h;
					theta = -M_HALFPI;
				}
			}
		}

		// Default color is white
		Color c = obj->color;
		if ( c.a() == 0 )
			c = Color( 255, 255, 255, 120 );

		// Apply alpha mod
		if ( (!behind && screen.x > -1.0 && screen.x < 1.0) )
			c[3] *= alpha_mod;

		// Bound our results
		x = clamp( x, m_iInnerBound, ScreenWidth() - m_iInnerBound - card_w );
		y = clamp( y, m_iInnerBound, ScreenHeight() - m_iInnerBound - card_h );
		
		// Draw text (if defined) before applying pulse so we don't affect it's color
		if ( obj->text[0] )
		{
			// This is where we clamp the size to 16
			int txtX, txtY;
			txtX = clamp( (x + card_w/2) - obj->txtW/2, XRES(3), ScreenWidth() - obj->txtW - XRES(3) );
			txtY = y - obj->txtH - YRES(3);

			if ( txtY < 0 )
				txtY = y + card_h + YRES(3);

			surface()->DrawSetTextColor( c );
			surface()->DrawSetTextFont( text_font );
			surface()->DrawSetTextPos( txtX, txtY );
			surface()->DrawUnicodeString( obj->text );
		}

		// Apply Pulse
		if ( obj->pulse )
		{
			float blink = SmoothCurve( obj->pulse_mod );
			c[0] = c[0] + (blink * (255-c[0]) * 0.15f);
			c[1] = c[1] + (blink * (255-c[1]) * 0.15f);
			c[2] = c[2] + (blink * (255-c[2]) * 0.15f);

			// Modulate the blink
			obj->pulse_mod += 1.2f * gpGlobals->frametime;
			if ( obj->pulse_mod > 1.0f )
				obj->pulse_mod -= 1.0f;
		}

		m_pCardTexture->DrawSelfRotated( x, y, card_w, card_h, theta, c );
	}
}

void CGEObjectives::FireGameEvent( IGameEvent *event )
{
	if ( !Q_stricmp(event->GetName(), "round_end") )
		ClearAll();
}

bool CGEObjectives::IsObjective( CBaseEntity *pEnt )
{
	if ( pEnt && FindObjective( pEnt->GetRefEHandle().ToInt() ) )
		return true;

	return false;
}

void CGEObjectives::ClearAll( void )
{
	m_vObjectives.PurgeAndDeleteElements();
}

GEObjective* CGEObjectives::FindObjective( int serial )
{
	for ( int i=0; i < m_vObjectives.Count(); i++ )
	{
		if ( m_vObjectives[i]->hEnt.ToInt() == serial )
			return m_vObjectives[i];
	}

	return NULL;
}

/*
void CGEObjectives::DEBUG_PrintText( int line, Color clr, const char *fmt, ... )
{
	int x = XRES( 50 ),
		y = YRES( 50 ) + line * YRES( 15 );

	va_list vl;
	va_start( vl, fmt );
	char buffer[256];
	vsnprintf( buffer, 256, fmt, vl );
	va_end( vl );

	wchar_t wczBuffer[256];
	g_pVGuiLocalize->ConvertANSIToUnicode( buffer, wczBuffer, sizeof(wczBuffer) );

	surface()->DrawSetTextFont( scheme()->GetIScheme(GetScheme())->GetFont( "Default", true ) );
	surface()->DrawSetTextPos( x, y );
	surface()->DrawSetTextColor( clr );
	surface()->DrawPrintText( wczBuffer, wcslen(wczBuffer) );
}*/


// Function used by team icons to ensure they don't overlap objective icons
bool GE_IsLocalObjective( C_BaseEntity *pEnt )
{
	CGEObjectives *pObjectives = GET_HUDELEMENT( CGEObjectives );
	if ( pObjectives )
		return pObjectives->IsObjective( pEnt );

	return false;
}
