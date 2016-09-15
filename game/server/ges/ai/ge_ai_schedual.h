///////////// Copyright © 2008 LodleNet. All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_ai_schedual.h
//   Description :
//      [TODO: Write the purpose of ge_ai_schedual.h.]
//
//   Created On: 3/8/2009 8:31:57 AM
//   Created By:  <mailto:admin@lodle.net>
////////////////////////////////////////////////////////////////////////////

#ifndef MC_GE_AI_SCHEDUAL_H
#define MC_GE_AI_SCHEDUAL_H
#ifdef _WIN32
#pragma once
#endif

class CGETask
{
public:
	virtual bool StartTask()=0;
	virtual bool RunTask()=0;
	virtual const char* GetDesc() = 0;
};

class CGESchedual
{
public:
	virtual bool IsRunning() = 0;
	virtual bool CanRun(int conditions) = 0;

	virtual int GetWeightFactor() = 0;
	virtual const char* GetDesc() = 0;

	virtual void Think() = 0;
	virtual void Reset() = 0;
};


#endif //MC_GE_AI_SCHEDUAL_H
