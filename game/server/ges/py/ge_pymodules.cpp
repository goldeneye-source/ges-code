///////////// Copyright © 2009 LodleNet. All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_pymodules.cpp
//   Description :
//      [TODO: Write the purpose of ge_pymodules.cpp.]
//
//   Created On: 9/1/2009 10:19:52 PM
//   Created By: Mark Chandler <mailto:mark@moddb.com>
////////////////////////////////////////////////////////////////////////////

#include "ge_pyprecom.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define REG( Name )	extern PyObject* PyInit_##Name(); PyImport_AppendInittab( #Name , &PyInit_##Name );	

extern "C"
{
	void RegisterPythonModules()
	{
		REG( GEUtil );
		REG( GEEntity );
		REG( GEPlayer );
		REG( GEGlobal );
		REG( GEWeapon );
		REG( GEAmmoCrate );
		REG( GEGamePlay );
		REG( GEMPGameRules );
		REG( GEAi );
		REG( GEAiConst );
		REG( GEAiSched );
		REG( GEAiTasks );
		REG( GEAiCond );
	}
}
