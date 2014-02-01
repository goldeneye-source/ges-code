///////////// Copyright © 2010 LodleNet. All rights reserved. /////////////
//
//   Project     : Client
//   File        : ge_updatechecker.h
//   Description :
//      [TODO: Write the purpose of ge_updatechecker.h.]
//
//   Created On: 3/6/2010 4:44:15 PM
//   Created By: Mark Chandler <mailto:mark@moddb.com>
////////////////////////////////////////////////////////////////////////////

#ifndef MC_GE_UPDATECHECKER_H
#define MC_GE_UPDATECHECKER_H
#ifdef _WIN32
#pragma once
#endif


#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include "ge_webrequest.h"
#include "ge_panelhelper.h"

class CGEUpdateChecker : public vgui::Frame, public CAutoGameSystemPerFrame
{
private:
	DECLARE_CLASS_SIMPLE(CGEUpdateChecker, vgui::Frame);

public:
	CGEUpdateChecker( vgui::VPANEL parent );
	~CGEUpdateChecker();

	virtual const char *GetName( void ) { return "GESUpdateTool"; }

	virtual bool NeedsUpdate() { return false; };
	virtual bool HasInputElements( void ) { return true; }

	virtual void PostInit();
	virtual void OnTick();

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
  	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }
	virtual void SetParent( vgui::Panel* parent ) { BaseClass::SetParent( parent ); }

protected:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	void processUpdate( const char* data );

private:
	CGEWebRequest *m_pWebRequest;
};

#endif //MC_GE_UPDATECHECKER_H
