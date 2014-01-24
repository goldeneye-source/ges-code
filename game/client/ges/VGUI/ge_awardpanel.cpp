/////////////  Copyright © 2008, Goldeneye: Source. All rights reserved.  /////////////
// 
// ge_awardpanel.cpp
//
// Description:
//      Singular Instance of an award for the Round Report
//
// Created On: 21 Dec 08
// Creator: Killer Monkey
////////////////////////////////////////////////////////////////////////////////

#include "cbase.h"
#include "ge_awardpanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CGEAwardPanel::CGEAwardPanel( Panel *parent, const char *name ) : vgui::EditablePanel(parent, name)
{
	SetScheme( parent->GetScheme() );
	SetProportional( true );

	LoadControlSettings("resource/ui/AwardPanel.res");

	InvalidateLayout();
}
