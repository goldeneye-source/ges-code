///////////// Copyright © 2009 LodleNet. All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_pyutill.cpp
//   Description :
//      [TODO: Write the purpose of ge_pyutill.cpp.]
//
//   Created On: 8/24/2009 3:51:01 PM
//   Created By: Mark Chandler <mailto:mark@moddb.com>
////////////////////////////////////////////////////////////////////////////

#include "ge_pyprecom.h"
#include "ge_pyfuncs.h"
#include "boost/python/raw_function.hpp"
#include "boost/format.hpp"

#include "ge_shareddefs.h"
#include "ge_recipientfilter.h"
#include "gemp_gamerules.h"
#include "gemp_player.h"
#include "ge_gameplay.h"
#include "ge_tokenmanager.h"

#include "particle_parse.h"
#include "networkstringtable_gamedll.h"
#include "soundent.h"
#include "team.h"
#include "te.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void pyClientPrint( bp::object plr_dest, int msg_dest, std::string msg, std::string param1, std::string param2, std::string param3, std::string param4 )
{
	bp::extract<CGEPlayer*> to_player( plr_dest );
	bp::extract<int> to_team( plr_dest );

	CRecipientFilter *filter = NULL;

	if ( plr_dest.is_none() )
		filter = new CReliableBroadcastRecipientFilter;
	else if ( to_player.check() )
		filter = new CSingleUserRecipientFilter( to_player() );
	else if ( to_team.check() )
		filter = new CGETeamRecipientFilter( to_team() );
	else
		to_player(); // Issues verbose error message
	
	if ( filter )
	{
		filter->MakeReliable();
		UTIL_ClientPrintFilter( *filter, msg_dest, msg.c_str(), param1.c_str(), param2.c_str(), param3.c_str(), param4.c_str() );
		delete filter;
	}
}

#define MAX_NETCHANNEL 10
int g_iMsgChannel = 5;
void pyHudMessage( bp::object dest, const char *msg, float x, float y, Color col, float holdtime, int chan = -1 )
{
	bp::extract<CGEPlayer*> to_player( dest );
	bp::extract<int> to_team( dest );

	CRecipientFilter *filter = NULL;

	// Figure out where to send us
	if ( dest.is_none() )
		filter = new CBroadcastRecipientFilter;
	else if ( to_player.check() )
		filter = new CSingleUserRecipientFilter( to_player() );
	else if ( to_team.check() )
		filter = new CGETeamRecipientFilter( to_team() );
	else
		// Ignore this if we gave bad params
		return;

	// Make us reliable
	filter->MakeReliable();

	// Figure out which channel to use
	if ( chan < 0 || chan >= MAX_NETCHANNEL )
	{
		g_iMsgChannel = (g_iMsgChannel >= MAX_NETCHANNEL) ? 5 : (g_iMsgChannel + 1);
		chan = g_iMsgChannel;
	}

	// Shoot out the message
	UserMessageBegin( *filter, "HudMsg" );
		WRITE_BYTE ( chan & 0xFF );
		WRITE_FLOAT( x );
		WRITE_FLOAT( y );
		WRITE_BYTE ( col.r() );
		WRITE_BYTE ( col.g() );
		WRITE_BYTE ( col.b() );
		WRITE_BYTE ( col.a() );
		WRITE_BYTE ( col.r() );
		WRITE_BYTE ( col.g() );
		WRITE_BYTE ( col.b() );
		WRITE_BYTE ( col.a() );
		WRITE_BYTE ( 0 );		 // effect
		WRITE_FLOAT( 0.25f );	 // fade in
		WRITE_FLOAT( 0.25f );	 // fade out
		WRITE_FLOAT( holdtime ); // hold time
		WRITE_FLOAT( 0 );		 // fxtime
		WRITE_STRING( msg );
	MessageEnd();

	// Cleanup
	delete filter;
}

void pyPopupMessage( bp::object dest, std::string title, std::string msg, std::string img )
{
	bp::extract<CGEPlayer*> to_player( dest );
	bp::extract<int> to_team( dest );

	CRecipientFilter *filter = NULL;
	
	if ( dest.is_none() )
		filter = new CReliableBroadcastRecipientFilter;
	else if ( to_player.check() )
		filter = new CSingleUserRecipientFilter( to_player() );
	else if ( to_team.check() )
		filter = new CGETeamRecipientFilter( to_team() );
	else
		to_player(); // Issues verbose error message

	if ( filter )
	{
		filter->MakeReliable();

		int nMsg = g_pStringTableGameplay->FindStringIndex( msg.c_str() );
		if ( nMsg == INVALID_STRING_INDEX )
		{
			bool save = engine->LockNetworkStringTables( false );
			nMsg = g_pStringTableGameplay->AddString( true, msg.c_str() );
			engine->LockNetworkStringTables( save );
		}

		UserMessageBegin( *filter, "PopupMessage" );
		WRITE_STRING( title.c_str() );
		WRITE_STRING( img.c_str() );
		WRITE_SHORT( nMsg );
		MessageEnd();

		delete filter;
	}
}

void pyEmitGameplayEvent( std::string name, std::string value1, std::string value2, std::string value3, std::string value4, bool sendtoclients )
{
	IGameEvent *event = gameeventmanager->CreateEvent( "gameplay_event" );
	if ( event )
	{
		event->SetString( "ident", GEGameplay()->GetScenario()->GetIdent() );
		event->SetString( "name", name.c_str() );
		event->SetString( "value1", value1.c_str() );
		event->SetString( "value2", value2.c_str() );
		event->SetString( "value3", value3.c_str() );
		event->SetString( "value4", value4.c_str() );

		// Fire the event to plugins and achievements
		gameeventmanager->FireEvent( event, !sendtoclients );
	}
}

void pyPostDeathMessage( const char *msg )
{
	IGameEvent *event = gameeventmanager->CreateEvent( "death_message" );
	if( event )
	{
		char tmp[128];
		Q_strncpy( tmp, msg, 128 );

		event->SetString( "message", tmp );
		gameeventmanager->FireEvent( event );
	}
}

enum TEMPENT_TYPE
{
	RING,
	BEAM,
	BEAM_FOLLOW,
	DUST,
	SMOKE,
	SPARK,
};

extern short g_sModelIndexLaser;
extern short g_sModelIndexSmoke;
bp::object pyCreateTempEnt( bp::tuple args, bp::dict kw )
{
	CBroadcastRecipientFilter filter;
	int type = bp::extract<int>( args[0] );

	const CBaseEntity *pSuppress = te->GetSuppressHost();
	te->SetSuppressHost( NULL );

	Vector origin		= GE_Extract<Vector>( kw, "origin", vec3_origin, true );
	float delay			= GE_Extract<float>( kw, "delay", 0 );
	int framerate       = GE_Extract<int>( kw, "framerate", 24 );

	if ( type == BEAM || type == BEAM_FOLLOW || type == RING )
	{
		float radius_start	= GE_Extract<float>( kw, "radius_start", 200.0f );
		float radius_end	= GE_Extract<float>( kw, "radius_end", 256.0f );
		float duration		= GE_Extract<float>( kw, "duration", 10.0f );
		float width			= GE_Extract<float>( kw, "width", 3.0f );
		float width_end		= GE_Extract<float>( kw, "width_end", width );
		float fade_length	= GE_Extract<float>( kw, "fade_length", 0.3f );
		float amplitude		= GE_Extract<float>( kw, "amplitude", 0.8f );
		Color col			= GE_Extract<Color>( kw, "color", Color(255,255,255,120) );
		int speed			= GE_Extract<int>( kw, "speed", 8 );

		if ( type == RING )
		{
			// filter, delay, center, start_radius, end_radius, modelindex, haloindex, startframe, 
			// framerate, duration, width, spread, amplitude, r, g, b, a, speed, flags
			te->BeamRingPoint( filter, delay, origin, radius_start, radius_end, g_sModelIndexLaser, 0, 0, 
				framerate, duration, width, 0, amplitude, col.r(), col.g(), col.b(), col.a(), speed, 0 );
		}
		else if ( type == BEAM )
		{
			// Requires an end position
			Vector end_pos	= GE_Extract<Vector>( kw, "end", vec3_origin, true );

			// filter, delay, start_pos, end_pos, modelindex, haloindex, startframe, framerate, duration
			// width, end_width, fadeLength, amplitude, r, g, b, a, speed
			te->BeamPoints( filter, delay, &origin, &end_pos, g_sModelIndexLaser, 0, 0, framerate, duration,
				width, width_end, fade_length, amplitude, col.r(), col.g(), col.b(), col.a(), speed );
		}
		else
		{
			// Requires an entity to follow
			const CBaseEntity *pEnt = GE_Extract<CBaseEntity*>( kw, "entity", NULL, true );

			if ( pEnt )
			{
				// filter, delay, entindex, modelindex, haloindex, life, width, 
				// end_width, fadeLength, r, g, b, a
				te->BeamFollow( filter, delay, pEnt->entindex(), g_sModelIndexLaser, 0, duration, width, 
					width_end, fade_length, col.r(), col.g(), col.b(), col.a() );
			}
		}
	}
	else if ( type == DUST )
	{
		Vector direction = GE_Extract<Vector>( kw, "direction", vec3_origin );
		float size		 = GE_Extract<float>( kw, "size", 16.0f );
		float speed		 = GE_Extract<float>( kw, "speed", 0 );

		// filter, delay, position, direction, size, speed
		te->Dust( filter, delay, origin, direction, size, speed );
	}
	else if ( type == SMOKE )
	{
		float size = GE_Extract<float>( kw, "size", 16.0f );

		// filter, delay, position, modelindex, scale, framerate
		te->Smoke( filter, delay, &origin, g_sModelIndexSmoke, size, framerate );
	}
	else if ( type == SPARK )
	{
		Vector direction	= GE_Extract<Vector>( kw, "direction", vec3_origin );
		float magnitude		= GE_Extract<float>( kw, "magnitude", 1.0f );
		float trail_length	= GE_Extract<float>( kw, "trail_length", 8.0f );

		// filter, delay, position, magnitude, trail_length, direction
		te->Sparks( filter, delay, &origin, magnitude, trail_length, &direction );
	}
	
	te->SetSuppressHost( (CBaseEntity*) pSuppress );
	return bp::object();
}

float pyServerTime()
{
	return gpGlobals->curtime;
}

void pyAddDownloadable( const char *file )
{
	// TODO: What about mid-game downloadables?? Need a client side pull
	INetworkStringTable *table = networkstringtable->FindTable( "downloadables" );
	if ( table )
	{
		bool save = engine->LockNetworkStringTables( false );
		table->AddString( true, file );
		engine->LockNetworkStringTables( save );
	}
}

void pyPlaySoundTo( bp::object dest, const char* soundname, bool bDimMusic = false )
{
	if ( !soundname )
		return;

	bp::extract<CGEPlayer*> to_player( dest );
	bp::extract<int> to_team( dest );

	int player_id = -1;
	int team_id = -1;

	if ( dest.is_none() ) {
	  	CReliableBroadcastRecipientFilter filter;
		CBaseEntity::EmitSound( filter, -1, soundname );
	} else if ( to_player.check() ) {
		CSingleUserRecipientFilter filter( to_player() );
		filter.MakeReliable();
		CBaseEntity::EmitSound( filter, to_player()->entindex(), soundname );

		player_id = to_player()->GetUserID();
	} else if ( to_team.check() ) {
		CGETeamRecipientFilter filter( to_team(), true );
		CBaseEntity::EmitSound( filter, -1, soundname );
		
		team_id = filter.iTeam;
	} else {
		// Issues verbose error message intentionally
		to_player();
	}

	if ( bDimMusic )
	{
		IGameEvent *event = gameeventmanager->CreateEvent( "specialmusic_setup" );
		if ( event )
		{
			event->SetInt( "playerid", player_id );
			event->SetInt( "teamid", team_id );
			event->SetString( "soundfile", soundname );
			gameeventmanager->FireEvent( event );
		}
	}
}

void pyPlaySoundFrom( bp::object dest, const char* soundname, float attenuation=ATTN_NORM )
{
	if ( !soundname )
		return;

	bp::extract<CGEPlayer*> to_player( dest );
	bp::extract<Vector> to_vector( dest );

	Vector origin = vec3_origin;
	int entindex = 0;

	if ( !dest.is_none() && to_player.check() ) {
		origin = to_player()->GetAbsOrigin();
		entindex = to_player()->entindex();
	} else if ( to_vector.check() ) {
		origin = to_vector();
	}

	CPASAttenuationFilter filter( origin, attenuation );
	filter.MakeReliable();
	CBaseEntity::EmitSound( filter, entindex, soundname, &origin );
}

void pyStopSound( bp::object dest, const char* soundname )
{
	int entindex = 0;
	bp::extract<CGEPlayer*> to_player( dest );

	if ( !dest.is_none() && to_player.check() )
		entindex = to_player()->entindex();

	CBaseEntity::StopSound( entindex, soundname );
}

// DEPRECATED FUNCTION!
void pyPlaySound( const char* soundname, bool bDimMusic = false )
{
	// Pass it through... plays to everyone
	pyPlaySoundTo( bp::object(), soundname, bDimMusic );
}

void pyInitHudProgressBar( bp::object dest, int idx, std::string title, int flags, float maxValue, float x, float y, int w, int h, Color col, float currValue )
{
	bp::extract<CGEPlayer*> to_player( dest );
	bp::extract<int> to_team( dest );

	CRecipientFilter *filter = NULL;
	if ( dest.is_none() )
		filter = new CReliableBroadcastRecipientFilter;
	else if ( to_player.check() )
		filter = new CSingleUserRecipientFilter( to_player() );
	else if ( to_team.check() )
		filter = new CGETeamRecipientFilter( to_team() );
	else
		to_player(); // Issues verbose error message

	if ( filter )
	{
		filter->MakeReliable();

		int xPer = clamp(x,-1.0f,1.0f) * 100;
		int yPer = clamp(y,-1.0f,1.0f) * 100;
		int iColor = col.GetRawColor();

		UserMessageBegin( *filter, "AddProgressBar" );
		WRITE_BYTE(idx);
		WRITE_BITS(&flags, 3);
		WRITE_FLOAT(maxValue);
		WRITE_FLOAT(currValue);
		WRITE_CHAR(xPer);
		WRITE_CHAR(yPer);
		WRITE_SHORT(w);
		WRITE_SHORT(h);
		WRITE_BITS(&iColor, 32);
		WRITE_STRING(title.c_str());
		MessageEnd();

		delete filter;
	}
}

void pyUpdateHudProgressBar( bp::object dest, int idx, float value, std::string title, Color color )
{
	bp::extract<CGEPlayer*> to_player( dest );
	bp::extract<int> to_team( dest );

	CRecipientFilter *filter = NULL;
	if ( dest.is_none() )
		filter = new CReliableBroadcastRecipientFilter;
	else if ( to_player.check() )
		filter = new CSingleUserRecipientFilter( to_player() );
	else if ( to_team.check() )
		filter = new CGETeamRecipientFilter( to_team() );
	else
		to_player(); // Issues verbose error message

	if ( filter )
	{
		filter->MakeReliable();

		// Send the Update message if we need to
		if ( value != INT_MAX )
		{
			UserMessageBegin( *filter, "UpdateProgressBar" );
			WRITE_BYTE(idx);
			WRITE_FLOAT(value);
			MessageEnd();
		}

		// Send the config message if we need to
		int iColor = color.GetRawColor();
		if ( title != "\r" || iColor != 0 )
		{
			UserMessageBegin( *filter, "ConfigProgressBar" );
			WRITE_BYTE(idx);
			WRITE_STRING(title.c_str());
			WRITE_BITS(&iColor, 32);
			MessageEnd();
		}

		delete filter;
	}
}

// DEPRECATED FUNCTION
void pyConfigHudProgressBar( bp::object dest, int idx, std::string title, Color color )
{
	bp::extract<CGEPlayer*> to_player( dest );
	bp::extract<int> to_team( dest );

	CRecipientFilter *filter = NULL;
	if ( dest.is_none() )
		filter = new CReliableBroadcastRecipientFilter;
	else if ( to_player.check() )
		filter = new CSingleUserRecipientFilter( to_player() );
	else if ( to_team.check() )
		filter = new CGETeamRecipientFilter( to_team() );
	else
		to_player(); // Issues verbose error message

	if ( filter )
	{
		filter->MakeReliable();

		int iColor = color.GetRawColor();

		UserMessageBegin( *filter, "ConfigProgressBar" );
		WRITE_BYTE(idx);
		WRITE_STRING(title.c_str());
		WRITE_BITS(&iColor, 32);
		MessageEnd();

		delete filter;
	}
}

void pyRemoveHudProgressBar( bp::object dest, int idx )
{
	bp::extract<CGEPlayer*> to_player( dest );
	bp::extract<int> to_team( dest );

	CRecipientFilter *filter = NULL;
	if ( dest.is_none() )
		filter = new CReliableBroadcastRecipientFilter;
	else if ( to_player.check() )
		filter = new CSingleUserRecipientFilter( to_player() );
	else if ( to_team.check() )
		filter = new CGETeamRecipientFilter( to_team() );
	else
		to_player(); // Issues verbose error message

	if ( filter )
	{
		filter->MakeReliable();

		UserMessageBegin( *filter, "RemoveProgressBar" );
		WRITE_BYTE(idx);
		MessageEnd();

		delete filter;
	}
}

void pyParticleEffect( CBasePlayer *pPlayer, const char* attachment, const char* effect, bool bFollow )
{
	if ( !pPlayer || !effect || !attachment )
		return;

	if ( bFollow )
	{
		DispatchParticleEffect( effect, PATTACH_POINT_FOLLOW, pPlayer, attachment );
	}
	else
	{
		// Find out where the attachment point is and pass it on (otherwise the entity takes control)
		Vector origin;
		pPlayer->GetBaseAnimating()->GetAttachment( attachment, origin, NULL );
		DispatchParticleEffect( effect, origin, vec3_angle  );
	}
}

void pyParticleEffectBeam( CBasePlayer *pPlayer, const char* attachment, Vector end, const char* effect )
{
	if ( !pPlayer || !effect || !attachment )
		return;

	Vector origin;
	pPlayer->GetBaseAnimating()->GetAttachment( attachment, origin, NULL );
	DispatchParticleEffect( effect, origin, end, vec3_angle );
}

const char* pyGetCVarValue(const char* name)
{
	const ConVar *cvar = g_pCVar->FindVar( name );
	if ( !cvar )
		return NULL;

	return cvar->GetString();
}

float pyDistanceBetween(CBaseEntity *pEntOne, CBaseEntity *pEntTwo)
{
	if (pEntOne && pEntTwo)
		return pEntOne->GetAbsOrigin().DistTo(pEntTwo->GetAbsOrigin());

	return 0;
}

void pyMsg( const char* msg )
{
	Msg( msg );
}

void pyDevMsg( const char *msg )
{
	DevMsg( msg );
}

void pyWarning( const char* msg )
{
	Warning(msg);
}

void pyDevWarning( const char *msg )
{
	DevWarning( msg );
}

void pyServerCmd( std::string cmd )
{
	if ( cmd.compare( cmd.length() - 1, 1, "\n") != 0 )
		cmd.append( "\n" );

	engine->ServerCommand( cmd.c_str() );
}

void pyClientCmd( CGEPlayer *pPlayer, std::string cmd )
{
	if ( cmd.compare( cmd.length() - 1, 1, "\n") != 0 )
		cmd.append( "\n" );

	engine->ClientCommand( pPlayer->edict(), cmd.c_str() );
}

bool pyIsDedicatedServer()
{
	return engine->IsDedicatedServer();
}

bool pyIsLAN()
{
	ConVar *sv_lan = g_pCVar->FindVar( "sv_lan" );
	if ( sv_lan )
		return sv_lan->GetBool();
	else
		return false;
}

// Define trace constants for python tracing
enum TRACE_TYPE
{
	TRACE_PLAYER	= 0x1,
	TRACE_WORLD		= 0x2,
	TRACE_WEAPON	= 0x4,
	TRACE_TOKEN		= 0x8,
	TRACE_CAPAREA	= 0x10,
};

CBaseEntity* pyTrace( Vector start, Vector end, int opts, CBaseEntity *ignore = NULL )
{
	trace_t tr;
	int mask = (opts & TRACE_PLAYER) ? MASK_SHOT : MASK_SHOT_HULL;

	UTIL_TraceLine( start, end, mask, ignore, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction < 1 && !tr.startsolid && tr.m_pEnt )
	{
		if ( (opts & TRACE_PLAYER) && ToGEPlayer( tr.m_pEnt ) )
			return tr.m_pEnt;

		const char *cname = tr.m_pEnt->GetClassname();
		if ( Q_stristr( cname, "weapon_" ) || Q_stristr( cname, "token_" ) )
		{
			// If we are looking for a token, see if the classname is registered
			if ( (opts & TRACE_TOKEN) && GEMPRules()->GetTokenManager()->IsValidToken( cname ) )
				return tr.m_pEnt;

			// Otherwise if we are just looking for a weapon, we found one!
			if ( opts & TRACE_WEAPON )
				return tr.m_pEnt;
		}

		if ( (opts & TRACE_CAPAREA) && !Q_stricmp( cname, "ge_capturearea" ) )
			return tr.m_pEnt;

		if ( (opts & TRACE_WORLD) && tr.m_pEnt->IsWorld() )
			return tr.m_pEnt;
	}

	return NULL;
}

Vector pyVectorMA( Vector start, Vector direction, float dist )
{
	Vector end;
	VectorMA( start, dist, direction, end );
	return end;
}

void Color_setitem(Color &c, int index, float value)
{
	if ( index >= 0 && index < 4 )
	{
		c[index] = value;
	}
	else
	{
		PyErr_SetString(PyExc_IndexError, "index out of range");
		bp::throw_error_already_set();
	}
}

float Color_getitem(Color &c, int index)
{
	if ( index >= 0 && index < 4 )
	{
		return c[index];
	}
	else
	{
		PyErr_SetString(PyExc_IndexError, "index out of range");
		bp::throw_error_already_set();
		return 0;
	}
}

static std::string Color_str(Color &c)
{
	std::stringstream s;
	s << boost::format( "R: %i, G: %i, B: %i, A: %i") % c.r() % c.g() % c.b() % c.a();
	return s.str();
}

void QAngle_setitem(QAngle &a, int index, float value)
{
    if (index >= 0 && index < 3) 
	{
        a[index] = value;
    }
    else 
	{
        PyErr_SetString(PyExc_IndexError, "index out of range");
		bp::throw_error_already_set();
    }
}

float QAngle_getitem(QAngle &a, int index)
{
    if (index >= 0 && index < 3) 
	{
        return a[index];
    }
    else 
	{
        PyErr_SetString(PyExc_IndexError, "index out of range");
		bp::throw_error_already_set();
		return 0;
    }
}

static std::string QAngle_str( QAngle &a )
{
	std::stringstream s;
	s << boost::format( "%0.2f, %0.2f, %0.2f") % a.x % a.y % a.z;
	return s.str();
}

void Vector_setitem(Vector &v, int index, float value)
{
    if (index >= 0 && index < 3) 
	{
        v[index] = value;
    }
    else 
	{
        PyErr_SetString(PyExc_IndexError, "index out of range");
        bp::throw_error_already_set();
    }
}

float Vector_getitem(Vector &v, int index)
{
    if (index >= 0 && index < 3) 
	{
        return v[index];
    }
    else 
	{
        PyErr_SetString(PyExc_IndexError, "index out of range");
        bp::throw_error_already_set();
		return 0;
    }
}

static std::string Vector_str( Vector &v )
{
	std::stringstream s;
	s << boost::format( "%0.2f, %0.2f, %0.2f") % v.x % v.y % v.z;
	return s.str();
}

class CColor : public Color
{
public:
	CColor() : Color() { }
	CColor( int r, int g, int b, int a ) : Color( r, g, b, a ) { }
	CColor( int r, int g, int b ) : Color( r, g, b ) { }
};

BOOST_PYTHON_FUNCTION_OVERLOADS(Trace_overloads, pyTrace, 3, 4);

BOOST_PYTHON_MODULE(GEUtil)
{
	using namespace boost::python;

	// Define this early
	class_<Color>("Color", init<>())
		.def(init<Color&>())
		.def(init<int>( arg("raw") ))
		.def(init<int, int, int, int>( ("r", "g", "b", arg("a")=255) ))
		.def("__getitem__", &Color_getitem)
		.def("__setitem__", &Color_setitem)
		.def("__str__", &Color_str)
		.def("SetColor", &Color::SetColor )
		.def("SetRawColor", &Color::SetRawColor )
		.def("GetRawColor", &Color::GetRawColor )
		.def("r", &Color::r )
		.def("g", &Color::g )
		.def("b", &Color::b )
		.def("a", &Color::a );

	def("ClientPrint", &pyClientPrint, ("plr_dest", "msg_dest", "msg", arg("param1")="", arg("param2")="", arg("param3")="", arg("param4")=""));

	def("PopupMessage", &pyPopupMessage, ("dest", "title", "msg", arg("image")=""));

	def("HudMessage", &pyHudMessage, ("dest", "msg", "x", "y", arg("color")=Color(255,255,255,255), arg("hold_time")=5.0f, arg("channel")=-1));

	def("Msg", pyMsg);
	def("DevMsg", pyDevMsg);
	def("Warning", pyWarning);
	def("DevWarning", pyDevWarning);

	def("ServerCommand", pyServerCmd);
	def("ClientCommand", pyClientCmd);

	def("IsDedicatedServer", pyIsDedicatedServer);
	def("IsLAN", pyIsLAN);

	def("PostDeathMessage", pyPostDeathMessage);
	def("EmitGameplayEvent", pyEmitGameplayEvent, ("name", arg("value1")="", arg("value2")="", arg("value3")="", arg("value4")="", arg("send_to_clients")=false));
	
	// Temporary Entities
	enum_< TEMPENT_TYPE >("TempEnt")
		.value("RING", RING)
		.value("BEAM", BEAM)
		.value("BEAM_FOLLOW", BEAM_FOLLOW)
		.value("DUST", DUST)
		.value("SMOKE", SMOKE)
		.value("SPARK", SPARK);

	def("CreateTempEnt", raw_function(pyCreateTempEnt));

	def("GetTime", pyServerTime);
	def("GetCVarValue", pyGetCVarValue);

	def("VectorMA", pyVectorMA);
	def("DistanceBetween", pyDistanceBetween);
	def("DebugDrawLine", DebugDrawLine);

	// Trace types
	enum_< TRACE_TYPE >("TraceOpt")
		.value("PLAYER", TRACE_PLAYER)
		.value("WORLD", TRACE_WORLD)
		.value("WEAPON", TRACE_WEAPON)
		.value("TOKEN", TRACE_TOKEN)
		.value("CAPAREA", TRACE_CAPAREA);

	def("Trace", pyTrace, return_value_policy<reference_existing_object>(), Trace_overloads());
	
	def("AddDownloadable", pyAddDownloadable);
	def("PrecacheSound", CBaseEntity::PrecacheScriptSound);
	def("PrecacheModel", CBaseEntity::PrecacheModel);
	def("PrecacheParticleEffect", PrecacheParticleSystem);

	def("PlaySoundTo", &pyPlaySoundTo, ("dest", "sound", arg("dim_music")=false));
	def("PlaySoundFrom", &pyPlaySoundFrom, ("dest", "sound", arg("attenuation")=ATTN_NORM));
	def("StopSound", &pyStopSound, ("dest", "sound"));

	// Depricated function names
	def("PlaySound", pyPlaySound, ("sound", arg("dim_music")=false));
	def("PlaySoundToTeam", &pyPlaySoundTo, ("dest", "sound", arg("dim_music")=false));
	def("PlaySoundToPlayer", &pyPlaySoundTo, ("dest", "sound", arg("dim_music")=false));
	def("PlaySoundFromPlayer", &pyPlaySoundFrom, ("dest", "sound", arg("attenuation")=ATTN_NORM));
	// End deprication

	def("ParticleEffect", pyParticleEffect);
	def("ParticleEffectBeam", pyParticleEffectBeam);

	def("InitHudProgressBar", pyInitHudProgressBar, ("dest", "index", arg("title")="", arg("flags")=0, 
		arg("max_value")=0, arg("x")=-1, arg("y")=-1, arg("w")=120, arg("h")=60, arg("color")=Color(255,255,255,255), arg("curr_value")=0));
	def("UpdateHudProgressBar", pyUpdateHudProgressBar, ("dest", "index", arg("value")=FLT_MAX, arg("title")="\r", arg("color")=Color()));
	def("RemoveHudProgressBar", pyRemoveHudProgressBar, ("dest", "index"));
	// DEPRECATED
	def("ConfigHudProgressBar", pyConfigHudProgressBar, ("dest", "index", arg("title")="\r", arg("color")=Color()));

	// This is deprecated but ubiquitous so keeping it in for backwards compat
	class_<CColor, bases<Color> >("CColor", init<>())
		.def(init<CColor&>())
		.def(init<int, int, int, int>( ("r", "g", "b", arg("a")=255) ));

	class_<QAngle>("QAngle", init<>())
		.def(init<QAngle&>())
		.def(init<float, float, float>())
		.def("__getitem__", &QAngle_getitem)
		.def("__setitem__", &QAngle_setitem)
		.def("__str__", &QAngle_str)
		.def("Random", &QAngle::Random)
		.def("IsValid", &QAngle::IsValid)
		.def("Invalidate", &QAngle::Invalidate)
		.def("Length", &QAngle::Length)
		.def("LengthSqr", &QAngle::LengthSqr)
		.def(self += other<QAngle>())
		.def(self -= other<QAngle>())
		.def(self /= other<float>())
		.def(self *= other<float>());

	class_<Vector>("Vector", init<>())
		.def(init<Vector&>())
		.def(init<float, float, float>())
		.def("__getitem__", &Vector_getitem)
		.def("__setitem__", &Vector_setitem)
		.def("__str__", &Vector_str)
		.def("Random", &Vector::Random)
		.def("IsValid", &Vector::IsValid)
		.def("Invalidate", &Vector::Invalidate)
		.def("Length", &Vector::Length)
		.def("LengthSqr", &Vector::LengthSqr)
		.def("Zero", &Vector::Zero)
		.def("Negate", &Vector::Negate)
		.def("IsZero", &Vector::IsZero)
		.def("NormalizeInPlace", &Vector::NormalizeInPlace)
		.def("IsLengthGreaterThan", &Vector::IsLengthGreaterThan)
		.def("IsLengthLessThan", &Vector::IsLengthLessThan)
		.def("DistTo", &Vector::DistTo)
		.def("DistToSqr", &Vector::DistToSqr)
		.def("MulAdd", &Vector::MulAdd)
		.def("Dot", &Vector::Dot)
		.def("Length2D", &Vector::Length2D)
		.def("Length2DSqr", &Vector::Length2DSqr)
		.def(self += other<float>())
		.def(self -= other<float>())
		.def(self /= other<float>())
		.def(self *= other<float>())
		.def(self == other<Vector>())
		.def(self += other<Vector>())
		.def(self -= other<Vector>())
		.def(self /= other<Vector>())
		.def(self *= other<Vector>())
		.def(self - other<Vector>());

	class_<CSound, boost::noncopyable>("CSound", no_init)
		.def("DoesSoundExpire", &CSound::DoesSoundExpire)
		.def("SoundExpirationTime", &CSound::SoundExpirationTime)
		.def("GetSoundOrigin", &CSound::GetSoundOrigin, return_value_policy<reference_existing_object>())
		.def("GetSoundReactOrigin", &CSound::GetSoundReactOrigin, return_value_policy<reference_existing_object>())
		.def("FIsSound", &CSound::FIsSound)
		.def("FIsScent", &CSound::FIsScent)
		.def("IsSoundType", &CSound::IsSoundType)
		.def("SoundType", &CSound::SoundType)
		.def("SoundContext", &CSound::SoundContext)
		.def("SoundTypeNoContext", &CSound::SoundTypeNoContext)
		.def("Volume", &CSound::Volume)
		.def("OccludedVolume", &CSound::OccludedVolume);
}
