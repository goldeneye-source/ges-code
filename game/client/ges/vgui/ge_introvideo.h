///////////// Copyright © 2010 LodleNet. All rights reserved. /////////////
//
//   Project     : Client
//   File        : ge_introvideo.h
//   Description :
//      [TODO: Write the purpose of ge_introvideo.h.]
//
//   Created On: 2/27/2010 4:03:59 PM
//   Created By: Mark Chandler <mailto:mark@moddb.com>
////////////////////////////////////////////////////////////////////////////

#ifndef GE_INTROVIDEO_H
#define GE_INTROVIDEO_H

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/CheckButton.h>

#include "ge_panelhelper.h"

class CGEIntroVideoPanel : public vgui::Frame, public CAutoGameSystemPerFrame
{
public:
	CGEIntroVideoPanel( vgui::VPANEL parent );
	~CGEIntroVideoPanel();
	
	DECLARE_CLASS_SIMPLE(CGEIntroVideoPanel, vgui::Frame);

	virtual void OnKeyCodePressed(vgui::KeyCode code );
	virtual void OnCursorMoved(int x,int y);
	virtual void OnCommand(const char *command);
	virtual void OnClose();

	virtual void PostInit();

	virtual void OnTick();
	virtual void Paint();
	virtual void PaintBackground();
	virtual void Activate();

	virtual void SetPlayBackgroundVids( bool state ) { m_bInBackground = state; };
	virtual void SetUserRequested( bool state ) { m_bUserRequested = state; };
	virtual void ResetBackgroundPlaytime( void );
	virtual void DisableBackgroundPlaytime( void ) { m_fNextBackgroundPlaytime = -1; };
	virtual void SetVisible( bool bShow );

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
  	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }
	virtual void SetParent( vgui::Panel* parent ) { BaseClass::SetParent( parent ); }

protected:
	bool beginPlayback( const char *pFilename = NULL );
	void stopPlayback( void );

	void LoadMovieList( void );

private:
	vgui::Button *m_pButStop;
	vgui::Button *m_pButSkip;
	vgui::Button *m_pButReplay;
	vgui::CheckButton *m_pAlwaysSkip;
	vgui::CheckButton *m_pBGSkip;

	//BIKMaterial_t	m_BIKHandle;
	IMaterial		*m_pMaterial;

	CUtlVector<char*> m_vMovies;
	int		m_iCurrMovieIdx;

	float	m_fLastMouseMove;
	float	m_fNextBackgroundPlaytime;

	// We track the mouse position to smartly reset the next background movie time
	int		m_iMouseX;
	int		m_iMouseY;

	int		m_nPlaybackHeight;
	int		m_nPlaybackWidth;
	float	m_flU;
	float	m_flV;

	bool	m_bIsCursorShown;
	bool	m_bIsPostInit;
	bool	m_bInBackground;
	bool	m_bUserRequested;
};

#endif //MC_GE_INTROVIDEO_H
