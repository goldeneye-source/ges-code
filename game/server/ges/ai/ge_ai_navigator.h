///////////// Copyright © 2008 LodleNet. All rights reserved. /////////////
//
//   Project     : Server
//   File        : ge_ai_navigator.h
//   Description :
//      [TODO: Write the purpose of ge_ai_navigator.h.]
//
//   Created On: 6/14/2009 10:39:03 AM
//   Created By: Mark Chandler <mailto:mark@moddb.com>
////////////////////////////////////////////////////////////////////////////

#ifndef MC_GE_AI_NAVIGATOR_H
#define MC_GE_AI_NAVIGATOR_H
#ifdef _WIN32
#pragma once
#endif

#include "ge_ai_helper.h"

class CGEAINavigator : public CGEAIHelper
{
public:
	CGEAINavigator(CGEAIHandle* baseAi);
	~CGEAINavigator();

	void Think();

	void NavigateRandom();

private:
	//CNavArea* m_pCurArea;

};


#endif //MC_GE_AI_NAVIGATOR_H
