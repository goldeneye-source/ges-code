//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef GE_CREDITSTEXT_H
#define GE_CREDITSTEXT_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include <vgui_controls/RichText.h>
#include <utlvector.h>

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: Non-editable display of a rich text control
//-----------------------------------------------------------------------------
class GECreditsText : public RichText
{
	DECLARE_CLASS_SIMPLE( GECreditsText, RichText );

public:
	GECreditsText( Panel *parent, const char *panelName );

	HFont GetFont() { return _font; }

	virtual void InsertFontChange( HFont font );
	virtual void InsertCenterText( bool center );

	virtual void SetToFullHeight();
	virtual void ForceRecalculateLineBreaks();

	virtual void LayoutVerticalScrollBarSlider();

protected:
	virtual void ApplySchemeSettings(IScheme *pScheme);
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void Paint();

	virtual const wchar_t *ResolveLocalizedTextAndVariables( char const *pchLookup, wchar_t *outbuf, size_t outbufsizeinbytes );

	MESSAGE_FUNC_INT( MoveScrollBar, "MoveScrollBar", delta );
	MESSAGE_FUNC_INT( MoveScrollBarDirect, "MoveScrollBarDirect", delta );

	virtual void AddAnotherLine( int &cx, int &cy );

	virtual void InvalidateLineBreakStream();
	virtual void RecalculateLineBreaks();

	virtual void RecalculateDefaultState( int startIndex );

	// updates a render state based on the formatting and color streams
	// returns true if any state changed
	virtual bool UpdateRenderState( int textStreamPos, TRenderState &renderState );
	virtual void GenerateRenderStateForTextStreamIndex( int textStreamIndex, TRenderState &renderState );
	
	// draws a string of characters with the same formatting using the current render state
	virtual int DrawString( int iFirst, int iLast, TRenderState &renderState, HFont font );

	// Stores the absolute length/height of each line in pixels
	CUtlVector<int>	   m_LineBreaksLength;
	CUtlVector<int>	   m_LineBreaksHeight;
	
	char m_szFont[128];

	TRenderState	*m_CurrRenderState;
};

} // namespace vgui


#endif // GE_CREDITSTEXT_H
