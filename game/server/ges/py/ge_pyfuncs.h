///////////// Copyright © 2012 GoldenEye: Source, All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_pyfuncs.h
//   Description :
//      Useful functions to use in Python interfaces
//
//   Created On: 07/01/2012
//   Created By: Killermonkey <killermonkey01@gmail.com>
////////////////////////////////////////////////////////////////////////////

#include "boost/typeof/typeof.hpp"

// Explicitly handle an error thrown by python/boost
// outputs error string to the console
void print_pyerror();
void print_pystack();

#define HandlePythonException() print_pyerror();
#define HandlePythonStackTrace() print_pystack();

template <typename T> class type_name {
public:
	static const char *name;
};

#define DECLARE_TYPE_NAME(x)		template<> const char *type_name<x>::name = #x;
#define DECLARE_TYPE_NAME2(x, str)	template<> const char *type_name<x>::name = str;
#define GET_TYPE_NAME(x)			(type_name<x>::name)

// Handy macros for calling python overrides
#define TRYFUNCRET( call, def ) try { if ( !self.is_none() ) return call; }	catch ( bp::error_already_set const& ) { HandlePythonException(); } return def;
#define TRYFUNC( call )			try { if ( !self.is_none() ) call; }		catch ( bp::error_already_set const& ) { HandlePythonException(); }
#define TRYCALL( call )			try { call; }								catch ( bp::error_already_set const& ) { HandlePythonException(); }
#define TRYCALLRET( call, def ) try { return call; }						catch ( bp::error_already_set const& ) { HandlePythonException(); } return def;

// Robust extraction of data from python dictionary
template<class TO>
TO GE_Extract( bp::dict d, const char *key, TO def_value, bool required = false )
{
	try {
		return bp::extract<TO>( d[key] );
	} catch ( const bp::error_already_set &e ) {
		// We always raise an error if we were required
		if ( required )
		{
			Warning( "KeyError: Required key \"%s\" was not defined!\n", key );
			throw e;
		}

		// Otherwise, ignore key errors, but gracefully catch extract errors
		if ( PyErr_ExceptionMatches( PyExc_KeyError ) )
			PyErr_Clear();
		else {
			Warning( "TypeError: Could not convert \"%s\" to type \"%s\"\n", key, GET_TYPE_NAME(TO) );
			HandlePythonStackTrace();
		}

		return (TO) def_value;
	}
}

// Specialized extraction for strings
bool GE_Extract( bp::dict d, const char *key, char *out, int length, bool required = false );
