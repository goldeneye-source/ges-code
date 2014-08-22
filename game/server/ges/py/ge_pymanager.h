///////////// Copyright © 2009 LodleNet. All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_pymanager.h
//   Description :
//      Manages multiple interfaces to python modules
//
//   Created On: 8/31/2009 9:06:31 PM
//   Created By: Mark Chandler <mailto:mark@moddb.com>
////////////////////////////////////////////////////////////////////////////

#ifndef MC_GE_PYMANAGER_H
#define MC_GE_PYMANAGER_H
#ifdef _WIN32
#pragma once
#endif

#include "ge_pyfuncs.h"
#include "ge_utils.h"

class IPythonListener
{
public:
	IPythonListener();
	~IPythonListener();

	virtual void OnPythonStartup() { };
	virtual void OnPythonShutdown() { };
};

class CPythonManager
{
public:
	CPythonManager();
	~CPythonManager();

	// Initialization functions
	void InitDll();
	void ShutdownDll();

	// Listener Functions
	void RegisterListener( IPythonListener *l );
	void UnregisterListener( IPythonListener *l );

	// Globals and Paths
	const char* GetRootPath() { return "python"; }
	const wchar_t* GetAbsBasePath() { return m_szAbsBasePath; }

	bp::object Globals() { return main_namespace; }

	// Execution helpers
	bp::object Exec( const char* buff );
	bp::object ExecFile( const char* file );

protected:
	void NotifyStartup();
	void NotifyShutdown();

private:
	// __main__ and globals() namespace
	bp::object main_module;
	bp::object main_namespace;

	// Have we gone through a DLLInit?
	bool m_bInit;

	wchar_t m_szAbsBasePath[512];

	CUtlVector<IPythonListener*> m_vListeners;
};

extern CPythonManager* GEPy();

#endif //MC_GE_PYMANAGER_H
