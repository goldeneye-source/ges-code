///////////// Copyright © 2010 LodleNet. All rights reserved. /////////////
//
//   Project     : Client
//   File        : ge_panelhelper.h
//   Description :
//      [TODO: Write the purpose of ge_panelhelper.h.]
//
//   Created On: 2/27/2010 4:06:23 PM
//   Created By: Mark Chandler <mailto:mark@moddb.com>
////////////////////////////////////////////////////////////////////////////

#ifndef MC_GE_PANELHELPER_H
#define MC_GE_PANELHELPER_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>

class IGEGameUI
{
public:
	virtual void Create( vgui::VPANEL parent ) = 0;
	virtual void Destroy( void ) = 0;
	virtual vgui::Panel *GetPanel(void) = 0;
};


template <class T>
class GameUI : public IGEGameUI
{
public:
	GameUI()
	{
		m_pPanel = NULL;
		// Register us internally to be created at VGui_CreateGlobalPanels()
		registerGEPanel(this);
	}

	void Create(vgui::VPANEL parent)
	{
		m_pPanel = new T(parent);
	}

	void Destroy( void )
	{
		if (m_pPanel)
		{
			m_pPanel->SetParent( (vgui::Panel*)NULL );
			delete m_pPanel;
		}

		m_pPanel = NULL;
	}

	T* operator->() const
	{
		return m_pPanel;
	}

	T* GetNativePanel()
	{
		return m_pPanel;
	}

	vgui::Panel *GetPanel(void)
	{
		return GetNativePanel();
	}

private:
	T* m_pPanel;
};

#define CenterThisPanelOnScreen() CenterPanelOnScreen(this);

inline void CenterPanelOnScreen(vgui::Panel* panel)
{
	if (!panel)
		return;

	int x,w,h;

	panel->GetBounds(x,x,w,h);
	panel->SetPos((ScreenWidth()-w)/2,(ScreenHeight()-h)/2);
}

inline bool IsWidescreen()
{
	if ( ((float) ScreenWidth() / (float) ScreenHeight()) > 1.4f )
		return true;
	else
		return false;
}

void StopMenuMusic( void );
void StartMenuMusic( void );

bool IsInGame( void );

void registerGEPanel( IGEGameUI* panel );
void createGEPanels( vgui::VPANEL parent );
void destroyGEPanels();


#endif //MC_GE_PANELHELPER_H
