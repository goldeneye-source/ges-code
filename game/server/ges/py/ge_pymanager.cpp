///////////// Copyright © 2009 LodleNet. All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_pymanager.cpp
//   Description :
//      [TODO: Write the purpose of ge_pymanager.cpp.]
//
//   Created On: 8/31/2009 9:07:05 PM
//   Created By: Mark Chandler <mailto:mark@moddb.com>
////////////////////////////////////////////////////////////////////////////

#include "ge_pyprecom.h"
#include "gemp_gamerules.h"
#include "filesystem.h"
#include "script_parser.h"
#include "ge_pymanager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern "C" void RegisterPythonModules();

// Global instance of the CPythonManager
CPythonManager gPyManager;
CPythonManager* GEPy()
{
	return &gPyManager;
}

void PythonInit()
{
	gPyManager.InitDll();
}

void PythonShutdown()
{
	gPyManager.ShutdownDll();
}

////////////////////////////////////////////////////////////////
// IPythonListener Defines
////////////////////////////////////////////////////////////////

IPythonListener::IPythonListener()
{
	GEPy()->RegisterListener( this );
}

IPythonListener::~IPythonListener()
{
	GEPy()->UnregisterListener( this );
}

////////////////////////////////////////////////////////////////
// CGEPyManager Defines
////////////////////////////////////////////////////////////////

CPythonManager::CPythonManager()
{
	m_szAbsBasePath[0] = '\0';
	m_bInit = false;
}

CPythonManager::~CPythonManager()
{
	// Nothing to do, yet
}

PyAPI_DATA(int) Py_OptimizeFlag;
PyAPI_DATA(int) Py_NoSiteFlag;
void CPythonManager::InitDll()
{
	// Only init once!
	if (m_bInit)
	{
		Warning("[GESPy] Attempted to initialize python more than once!\n");
		return;
	}

	try
	{
		// Optimize code and startup
		Py_OptimizeFlag = 2;
		Py_NoSiteFlag = 1;

		// Set our base path early, before init!
		char base_path[512];
		filesystem->RelativePathToFullPath( GetRootPath(), "MOD", base_path, sizeof(base_path) );
		V_strtowcs( base_path, sizeof(base_path), m_szAbsBasePath, sizeof(m_szAbsBasePath) );
		
		wchar_t py_path[1024];
#if defined(POSIX) || defined(_LINUX)
		swprintf( py_path, 1024, L"%ls:%ls/lib", m_szAbsBasePath, m_szAbsBasePath );
#else
		_snwprintf_s( py_path, 1024, L"%ls;%ls\\lib", m_szAbsBasePath, m_szAbsBasePath );
#endif
		Py_SetPath( py_path );

		// Register modules and initialize
		RegisterPythonModules();
		Py_Initialize();
	
		// Extract __main__ and it's associated namespace [globals()]
		main_module = bp::import("__main__");
		main_namespace = main_module.attr("__dict__");

		// Push the base path into GEGlobal.PY_BASE_DIR
		bp::object ge_global = bp::import( "GEGlobal" );
		ge_global.attr("PY_BASE_DIR") = base_path;

		// Execute our initialization routines
		bp::import( "ges" );
	}
	catch ( bp::error_already_set const & )
	{
		// If we get here, something terrible happened and we cannot continue
		PyObject *ptype, *pvalue, *ptraceback;
		PyErr_Fetch(&ptype, &pvalue, &ptraceback);

		const char *errors = NULL;
		PyObject* pyValue = PyUnicode_AsEncodedString(PyObject_Repr(pvalue), "utf-8", errors);

		Warning( "\nFailed to load python with error: %s\n", PyBytes_AS_STRING(pyValue) );
		AssertFatalMsg( false, "Failed to load python" );

		Py_XDECREF( pyValue );
		Py_XDECREF( ptype );
		Py_XDECREF( pvalue );
		Py_XDECREF( ptraceback );
	}
	
	// Let everyone know we are alive
	NotifyStartup();

	m_bInit = true;
}

void CPythonManager::ShutdownDll()
{
	// We can't shutdown if we didn't init
	if (m_bInit)
	{
		Warning("[GESPy] Attempted to shutdown python before initialization!\n");
		return;
	}

	// Let everyone know we are going down
	NotifyShutdown();

	// Shutdown Python, easy 1 call :-)
	Py_Finalize();

	m_bInit = false;
}

void CPythonManager::RegisterListener( IPythonListener *l )
{
	if ( !m_vListeners.Find( l ) )
		m_vListeners.AddToTail( l );
}

void CPythonManager::UnregisterListener( IPythonListener *l )
{
	m_vListeners.FindAndRemove( l );
}

void CPythonManager::NotifyStartup( void )
{
	FOR_EACH_VEC( m_vListeners, i )
		m_vListeners[i]->OnPythonStartup();
}

void CPythonManager::NotifyShutdown( void )
{
	FOR_EACH_VEC( m_vListeners, i )
		m_vListeners[i]->OnPythonShutdown();
}

bp::object CPythonManager::Exec(const char* buff)
{
	return bp::exec( buff, Globals(), Globals() );
}

bp::object CPythonManager::ExecFile( const char* name )
{
	char file[255];

	Q_snprintf( file, 255, "%s\\ges\\%s", GEPy()->GetRootPath(), name );

	char fullPath[255];
	filesystem->RelativePathToFullPath( file, "MOD", fullPath, 255 );

	return bp::exec_file( fullPath, Globals(), Globals() );
}


////////////////////////////////////////////////////////////////
//
////////////////////////////////////////////////////////////////
CON_COMMAND(py, "Exec python")
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
	{
		Msg( "You must be a server admin to use that command\n" );
		return;
	}

	if ( args.ArgC() < 2 )
	{
		Msg( "Executes python commands in the global namespace\n" );
		return;
	}

	try
	{
		Msg( ">>> %s\n", args.ArgS() );
		GEPy()->Exec( args.ArgS() );
		Msg( "\n" );
	}
	catch ( bp::error_already_set const& )
	{
		HandlePythonException();
	}
}

// Python debugger connection. Only allowed on Windows!
#ifdef WIN32
CON_COMMAND( ge_py_connectdebugger, "Connect to a waiting PyDev debugger (Eclipse). The first argument designates the host to connect to, the second designates the port." )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
	{
		Msg( "You must be a server admin to use that command\n" );
		return;
	}

	std::stringstream call;
	call << "import ge_debugger; ge_debugger.ConnectDebugger(";
	
	// Start the debugger connection
	if ( args.ArgC() >= 3 )
		call << "host_ovr=\"" << args[1] << "\", port_ovr=\"" << args[2] << "\")";
	else if ( args.ArgC() == 2 )
		call << "host_ovr=\"" << args[1] << "\")";
	else
		call << ")";

	try {
		GEPy()->Exec( call.str().c_str() );
	} catch ( bp::error_already_set const & ) {
		HandlePythonException();
	}
}

CON_COMMAND( ge_py_listendebugger, "Listen for Python Tools for Visual Studio (PYTVS) debugger. The first argument designates the port to connect on" )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
	{
		Msg( "You must be a server admin to use that command\n" );
		return;
	}

	std::stringstream call;
	call << "import ge_debugger; ge_debugger.ListenDebugger(";

	// Start listening for the debugger
	if ( args.ArgC() >= 2 )
		call << "port_ovr=\"" << args[1] << "\")";
	else
		call << ")";

	try {
		GEPy()->Exec( call.str().c_str() );
	} catch ( bp::error_already_set const & ) {
		HandlePythonException();
	}
}
#endif
