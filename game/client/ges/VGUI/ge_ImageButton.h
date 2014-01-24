//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef IMAGEBUTTON_H
#define IMAGEBUTTON_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui/Dar.h>
#include <Color.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ImagePanel.h>
#include "vgui/MouseCode.h"

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class ImageButton : public Button
{
	DECLARE_CLASS_SIMPLE( ImageButton, Button );

public:
	// You can optionally pass in the panel to send the click message to and the name of the command to send to that panel.
	ImageButton(Panel *parent, const char *panelName, const char *text, Panel *pActionSignalTarget=NULL, const char *pCmd=NULL);
	ImageButton(Panel *parent, const char *panelName, const wchar_t *text, Panel *pActionSignalTarget=NULL, const char *pCmd=NULL);
	~ImageButton();
private:
	void Init();
public:

	virtual void OnCursorEntered();
	virtual void OnCursorExited();
	virtual void OnMousePressed( MouseCode code );
	virtual void OnMouseReleased( MouseCode code );

	virtual void SetImages( const char *szDefault, const char *szArmed = NULL, const char *szDepressed = NULL );
	
	virtual void ApplySettings( KeyValues *inResourceData );

	MESSAGE_FUNC_INT( SetSelectedPanel, "PanelSelected", state );

protected:
	enum ImageState
	{
		STATE_DEFAULT = 0,
		STATE_ARMED,
		STATE_DEPRESSED,
	};

	virtual void SetButtonState( ImageState state );

	virtual void OnSizeChanged( int wide, int tall );
	virtual void OnThink( void );

private:
	ImagePanel *m_pImgDefault;
	ImagePanel *m_pImgArmed;
	ImagePanel *m_pImgDepressed;

	float m_flNextThink;
};

} // namespace vgui

#endif // IMAGEBUTTON_H
