///////////// Copyright © 2012 GoldenEye: Source, All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_pyfuncs.cpp
//   Description :
//      Useful functions to use in Python interfaces
//
//   Created On: 07/01/2012
//   Created By: Killermonkey <killermonkey01@gmail.com>
////////////////////////////////////////////////////////////////////////////
#include "ge_pyprecom.h"
#include "ge_pyfuncs.h"
#include "ge_weapon.h"
#include "ge_ammocrate.h"
#include "gemp_player.h"

// Type name definitions for friendly errors
DECLARE_TYPE_NAME( int );
DECLARE_TYPE_NAME( float );
DECLARE_TYPE_NAME2( bool, "boolean" );
DECLARE_TYPE_NAME2( char*, "string" );
DECLARE_TYPE_NAME2( Color, "GEUtil.Color" );
DECLARE_TYPE_NAME2( Vector, "GEUtil.Vector" );
DECLARE_TYPE_NAME2( CBaseEntity*, "GEEntity.CBaseEntity" );
DECLARE_TYPE_NAME2( CGEWeapon*, "GEWeapon.CGEWeapon" );
DECLARE_TYPE_NAME2( CGEAmmoCrate*, "GEAmmoCrate.CGEAmmoCrate" );
DECLARE_TYPE_NAME2( CGEPlayer*, "GEPlayer.CGEPlayer" );
DECLARE_TYPE_NAME2( CGEMPPlayer*, "GEPlayer.CGEMPPlayer" );

// Keep this here
namespace boost
{
	void throw_exception(std::exception const & e)
	{
		throw e;
	}
}

void print_pyerror()
{
	if ( PyErr_Occurred() )
		PyErr_Print();
	
	bp::handle_exception();
	PyErr_Clear();
}

void print_pystack()
{
	bp::object traceback( bp::import("traceback") );
	traceback.attr( "print_stack" )();

	bp::handle_exception();
	PyErr_Clear();
}

bool GE_Extract( bp::dict d, const char *key, char *out, int length, bool required /*=false*/ )
{
	const char *tmp = GE_Extract<char*>( d, key, NULL, required );
	if ( tmp )
	{
		Q_strncpy( out, tmp, length );
		return true;
	}

	return false;
}
