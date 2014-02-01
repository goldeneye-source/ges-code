///////////// Copyright © 2008, Goldeneye: Source. All rights reserved. /////////////
// 
// File: ge_CreditsText.cpp
// Description:
//      VGUI control for Credits Text (basically an enhanced RichText)
//
// Created On: 13 Dec 08
// Created By: Jonathan White <killermonkey>
/////////////////////////////////////////////////////////////////////////////

#include "cbase.h"

#include "ge_CreditsText.h"
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/ScrollBar.h>
#include <vgui_controls/Controls.h>
#include "ge_utils.h"

// memdbgon must be the last include file in a .cpp file
#include "tier0/memdbgon.h"

using namespace vgui;

enum
{
	MAX_BUFFER_SIZE = 999999,	// maximum size of text buffer
	DRAW_OFFSET_X =	3,
	DRAW_OFFSET_Y =	1,
};

namespace vgui
{

class ClickPanel : public Panel
{
	DECLARE_CLASS_SIMPLE( ClickPanel, Panel );

public:
	ClickPanel(Panel *parent)
	{
		_textIndex = 0;
		SetParent(parent);
		AddActionSignalTarget(parent);
		
		SetCursor(dc_hand);
		
		SetPaintBackgroundEnabled(false);
		SetPaintEnabled(false);
//		SetPaintAppearanceEnabled(false);

#if defined( DRAW_CLICK_PANELS )
		SetPaintEnabled(true);
#endif
	}
	
	void SetTextIndex(int index)
	{
		_textIndex = index;
	}

#if defined( DRAW_CLICK_PANELS )
	virtual void Paint()
	{
		surface()->DrawSetColor( Color( 255, 0, 0, 255 ) );
		surface()->DrawOutlinedRect( 0, 0, GetWide(), GetTall() );
	}
#endif
	
	int GetTextIndex()
	{
		return _textIndex;
	}
	
	void OnMousePressed(MouseCode code)
	{
		if (code == MOUSE_LEFT)
		{
			PostActionSignal(new KeyValues("ClickPanel", "index", _textIndex));
		}
	}

private:
	int _textIndex;
};

};	// namespace vgui


DECLARE_BUILD_FACTORY( GECreditsText );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
GECreditsText::GECreditsText(Panel *parent, const char *panelName) : BaseClass(parent, panelName)
{
	m_CurrRenderState = NULL;
	Q_strcpy( m_szFont, "Default" );
}

void GECreditsText::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetFont( pScheme->GetFont(m_szFont, true) );
}

void GECreditsText::ApplySettings( KeyValues *inResourceData )
{
	Q_strncpy( m_szFont, inResourceData->GetString( "font", "Default" ), 128 );
	BaseClass::ApplySettings( inResourceData );
}

const wchar_t *GECreditsText::ResolveLocalizedTextAndVariables( char const *pchLookup, wchar_t *outbuf, size_t outbufsizeinbytes )
{
	GEUTIL_ParseLocalization( outbuf, outbufsizeinbytes / sizeof(outbuf[0]), pchLookup );
	return outbuf;
}

//-----------------------------------------------------------------------------
// Purpose: Recalculates the formatting state from the specified index
//-----------------------------------------------------------------------------
void GECreditsText::RecalculateDefaultState(int startIndex)
{
	if (!m_TextStream.Count())
		return;

	Assert(startIndex < m_TextStream.Count());

	m_CachedRenderState.textColor = GetFgColor();
	m_CachedRenderState.textFont = _font;
	m_CachedRenderState.centerText = false;
	_pixelsIndent = 0;
	_currentTextClickable = false;
	_clickableTextIndex = GetClickableTextIndexStart(startIndex);
	
	// find where in the formatting stream we need to be
	GenerateRenderStateForTextStreamIndex(startIndex, m_CachedRenderState);
	_recalcSavedRenderState = false;
}

//-----------------------------------------------------------------------------
// Purpose: Invalidates the current linebreak stream
//-----------------------------------------------------------------------------
void GECreditsText::InvalidateLineBreakStream()
{
	// clear the buffer
	m_LineBreaks.RemoveAll();
	m_LineBreaksLength.RemoveAll();
	m_LineBreaksHeight.RemoveAll();

	m_LineBreaks.AddToTail(MAX_BUFFER_SIZE);
	m_LineBreaksLength.AddToTail( 0 );
	m_LineBreaksHeight.AddToTail( 0 );

	_recalculateBreaksIndex = 0;
	m_bRecalcLineBreaks = true;
}

//-----------------------------------------------------------------------------
// Purpose: Draws a string of characters in the panel
// Input:	iFirst - Index of the first character to draw
//			iLast - Index of the last character to draw
//			renderState - Render state to use
//			font- font to use
// Output:	returns the width of the character drawn
//-----------------------------------------------------------------------------
int GECreditsText::DrawString(int iFirst, int iLast, TRenderState &renderState, HFont font)
{
	surface()->DrawSetTextFont( font );

	// Calculate the render size
	int fontTall = surface()->GetFontTall(font);
	// BUGBUG John: This won't exactly match the rendered size
	int charWide = 0;
	for ( int i = iFirst; i <= iLast; i++ )
	{
		wchar_t ch = m_TextStream[i];
		charWide += surface()->GetCharacterWidth(font, ch);
	}

	// draw selection, if any
	int selection0 = -1, selection1 = -1;
	GetSelectedRange(selection0, selection1);
		
	if (iFirst >= selection0 && iFirst < selection1)
	{
		// draw background selection color
		surface()->DrawSetColor(_selectionColor);
		surface()->DrawFilledRect(renderState.x, renderState.y, renderState.x + charWide, renderState.y + 1 + fontTall);
		
		// reset text color
		surface()->DrawSetTextColor(_selectionTextColor);
	}
	else
	{
		surface()->DrawSetTextColor(renderState.textColor);
	}
		
	if ( renderState.textColor.a() != 0 )
	{
		surface()->DrawSetTextPos(renderState.x, renderState.y);
		surface()->DrawPrintText(&m_TextStream[iFirst], iLast - iFirst + 1);
	}
			
	return charWide;
}

//-----------------------------------------------------------------------------
// Purpose: Draws the text in the panel
//-----------------------------------------------------------------------------
void GECreditsText::Paint()
{
	int wide, tall;
	GetSize( wide, tall );
		
	// hide all the clickable panels until we know where they are to reside
	for (int j = 0; j < _clickableTextPanels.Count(); j++)
	{
		_clickableTextPanels[j]->SetVisible(false);
	}

	if (!m_TextStream.Count())
		return;
	int lineBreakIndexIndex = 0;
	int startIndex = GetStartDrawIndex(lineBreakIndexIndex);
	_currentTextClickable = false;
	
	_clickableTextIndex = GetClickableTextIndexStart(startIndex);
	
	// recalculate and cache the render state at the render start
	if (_recalcSavedRenderState)
	{
		RecalculateDefaultState(startIndex);
	}
	// copy off the cached render state
	TRenderState renderState = m_CachedRenderState;

	_pixelsIndent = m_CachedRenderState.pixelsIndent;
	_currentTextClickable = m_CachedRenderState.textClickable;

	renderState.textClickable = _currentTextClickable;
	renderState.textColor = m_FormatStream[renderState.formatStreamIndex].color;
	renderState.textFont = m_FormatStream[renderState.formatStreamIndex].textFont;
	renderState.centerText = m_FormatStream[renderState.formatStreamIndex].centerText;

	CalculateFade( renderState );

	renderState.formatStreamIndex++;

	if ( _currentTextClickable )
	{
		_clickableTextIndex = startIndex;
	}

	// where to start drawing
	renderState.x = _drawOffsetX + _pixelsIndent;
	renderState.y = _drawOffsetY;
	
	// draw the text
	int selection0 = -1, selection1 = -1;
	GetSelectedRange(selection0, selection1);

	bool isLineCentered = false;

	for (int i = startIndex; i < m_TextStream.Count() && renderState.y < tall; )
	{
		// 1.
		// Update our current render state based on the formatting and color streams
		int nXBeforeStateChange = renderState.x;
		if ( UpdateRenderState(i, renderState) )
		{
			// check for url state change
			if (renderState.textClickable != _currentTextClickable)
			{
				if (renderState.textClickable)
				{
					// entering new URL
					_clickableTextIndex++;
					surface()->DrawSetTextFont( m_hFontUnderline );
					
					// set up the panel
					ClickPanel *clickPanel = _clickableTextPanels.IsValidIndex( _clickableTextIndex ) ? _clickableTextPanels[_clickableTextIndex] : NULL;
					
					if (clickPanel)
					{
						clickPanel->SetPos(renderState.x, renderState.y);
					}
				}
				else
				{
					FinishingURL(nXBeforeStateChange, renderState.y);
					surface()->DrawSetTextFont( renderState.textFont );
				}
				_currentTextClickable = renderState.textClickable;
			}
		}
		
		// 2.
		// if we've passed a line break go to that
		if ( m_LineBreaks[lineBreakIndexIndex] == i )
		{
			if (_currentTextClickable)
			{
				FinishingURL(renderState.x, renderState.y);
			}
			
			// add another line
			//AddAnotherLine(renderState.x, renderState.y);
			renderState.x = _drawOffsetX + _pixelsIndent;
			renderState.y += _drawOffsetY + m_LineBreaksHeight[lineBreakIndexIndex];
			lineBreakIndexIndex++;

			// Don't draw a space if it is the start of the line
			if ( m_TextStream[i] == L' ' )
				i++;

			// Center this line (we ignore intra-line centering changes!)
			isLineCentered = renderState.centerText;
			
			if ( renderState.textClickable )
			{
				// move to the next URL
				_clickableTextIndex++;
				ClickPanel *clickPanel = _clickableTextPanels.IsValidIndex( _clickableTextIndex ) ? _clickableTextPanels[_clickableTextIndex] : NULL;
				if (clickPanel)
				{
					clickPanel->SetPos(renderState.x, renderState.y);
				}
			}
		}
		else if ( i == startIndex )
		{
			isLineCentered = renderState.centerText;
		}

		// 3.
		// Calculate the range of text to draw all at once
		int iLast = m_TextStream.Count() - 1;
		
		// Stop at the next line break
		if ( m_LineBreaks[lineBreakIndexIndex] <= iLast )
			iLast = m_LineBreaks[lineBreakIndexIndex] - 1;

		// Stop at the next format change
		if ( m_FormatStream.IsValidIndex(renderState.formatStreamIndex) && 
			m_FormatStream[renderState.formatStreamIndex].textStreamIndex <= iLast )
		{
			iLast = m_FormatStream[renderState.formatStreamIndex].textStreamIndex - 1;
		}

		// Stop when entering or exiting the selected range
		if ( i < selection0 && iLast >= selection0 )
			iLast = selection0 - 1;
		if ( i >= selection0 && i < selection1 && iLast >= selection1 )
			iLast = selection1 - 1;

		// Handle non-drawing characters specially
		for ( int iT = i; iT <= iLast; iT++ )
		{
			if ( iswcntrl(m_TextStream[iT]) )
			{
				iLast = iT - 1;
				break;
			}
		}

		// 4.
		// Draw the current text range
		if ( iLast < i )
		{
			if ( m_TextStream[i] == '\t' )
			{
				int dxTabWidth = 8 * surface()->GetCharacterWidth(renderState.textFont, ' ');
				dxTabWidth = max( 1, dxTabWidth );

				renderState.x = ( dxTabWidth * ( 1 + ( renderState.x / dxTabWidth ) ) );
			}
			i++;
		}
		// If we asked to center this text (line by line)
		else if ( isLineCentered )
		{
			renderState.x = wide/2 - (m_LineBreaksLength[lineBreakIndexIndex] / 2);
			renderState.x += DrawString(i, iLast, renderState, renderState.textFont);
			i = iLast + 1;
		}
		else
		{
			renderState.x += DrawString(i, iLast, renderState, renderState.textFont);
			i = iLast + 1;
		}
	}

	if (renderState.textClickable)
	{
		FinishingURL(renderState.x, renderState.y);
	}

	// Reset our render state so we don't mess with other functions
	m_CurrRenderState = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: updates a render state based on the formatting and color streams
// Output:	true if we changed the render state
//-----------------------------------------------------------------------------
bool GECreditsText::UpdateRenderState(int textStreamPos, TRenderState &renderState)
{
	// check the color stream
	if (m_FormatStream.IsValidIndex(renderState.formatStreamIndex) && 
		m_FormatStream[renderState.formatStreamIndex].textStreamIndex == textStreamPos)
	{
		// set the current formatting
		renderState.textColor = m_FormatStream[renderState.formatStreamIndex].color;
		renderState.textClickable = m_FormatStream[renderState.formatStreamIndex].textClickable;
		renderState.textFont = m_FormatStream[renderState.formatStreamIndex].textFont;
		renderState.centerText = m_FormatStream[renderState.formatStreamIndex].centerText;

		CalculateFade( renderState );

		int indentChange = m_FormatStream[renderState.formatStreamIndex].pixelsIndent - renderState.pixelsIndent;
		renderState.pixelsIndent = m_FormatStream[renderState.formatStreamIndex].pixelsIndent;

		if (indentChange)
		{
			renderState.x = renderState.pixelsIndent + _drawOffsetX;
		}

		//!! for supporting old functionality, store off state in globals
		_pixelsIndent = renderState.pixelsIndent;

		// move to the next position in the color stream
		renderState.formatStreamIndex++;
		m_CurrRenderState = &renderState;

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Generates a base renderstate given a index into the text stream
//-----------------------------------------------------------------------------
void GECreditsText::GenerateRenderStateForTextStreamIndex(int textStreamIndex, TRenderState &renderState)
{
	// find where in the format stream we need to be given the specified place in the text stream
	renderState.formatStreamIndex = FindFormatStreamIndexForTextStreamPos(textStreamIndex);
	
	// copy the state data
	renderState.centerText = m_FormatStream[renderState.formatStreamIndex].centerText;
	renderState.textFont = m_FormatStream[renderState.formatStreamIndex].textFont;
	renderState.textColor = m_FormatStream[renderState.formatStreamIndex].color;
	renderState.pixelsIndent = m_FormatStream[renderState.formatStreamIndex].pixelsIndent;
	renderState.textClickable = m_FormatStream[renderState.formatStreamIndex].textClickable;
}

//-----------------------------------------------------------------------------
// Purpose: inserts a font change into the formatting stream
//-----------------------------------------------------------------------------
void GECreditsText::InsertFontChange(HFont font)
{
	// see if color already exists in text stream
	TFormatStream &prevItem = m_FormatStream[m_FormatStream.Count() - 1];
	if (prevItem.textFont == font)
	{
		// inserting same color into stream, just ignore
	}
	else if ( m_TextStream.Count() == 1 || prevItem.textStreamIndex == m_TextStream.Count())
	{
		// this item is in the same place; update values
		prevItem.textFont = font;
	}
	else
	{
		// add to text stream, based off existing item
		TFormatStream streamItem = prevItem;
		streamItem.textFont = font;
		streamItem.textStreamIndex = m_TextStream.Count();
		m_FormatStream.AddToTail(streamItem);
	}
}

//-----------------------------------------------------------------------------
// Purpose: inserts a font change into the formatting stream
//-----------------------------------------------------------------------------
void GECreditsText::InsertCenterText( bool center )
{
	// see if color already exists in text stream
	TFormatStream &prevItem = m_FormatStream[m_FormatStream.Count() - 1];
	if (prevItem.centerText == center)
	{
		// inserting same color into stream, just ignore
	}
	else if ( m_TextStream.Count() == 1 || prevItem.textStreamIndex == m_TextStream.Count())
	{
		// this item is in the same place; update values
		prevItem.centerText = center;
	}
	else
	{
		// add to text stream, based off existing item
		TFormatStream streamItem = prevItem;
		streamItem.centerText = center;
		streamItem.textStreamIndex = m_TextStream.Count();
		m_FormatStream.AddToTail(streamItem);
	}
}

//-----------------------------------------------------------------------------
// Purpose: moves x,y to the Start of the next line of text
//-----------------------------------------------------------------------------
void GECreditsText::AddAnotherLine(int &cx, int &cy)
{
	cx = _drawOffsetX + _pixelsIndent;

	if ( m_CurrRenderState )
		cy += _drawOffsetY + surface()->GetFontTall( m_CurrRenderState->textFont );
	else
		cy += _drawOffsetY + surface()->GetFontTall( _font );
}

void GECreditsText::SetToFullHeight()
{
	ForceRecalculateLineBreaks();
	BaseClass::SetToFullHeight();
}

void GECreditsText::ForceRecalculateLineBreaks()
{
	InvalidateLineBreakStream();
	RecalculateLineBreaks();
}

//-----------------------------------------------------------------------------
// Purpose: Recalculates line breaks
//-----------------------------------------------------------------------------
void GECreditsText::RecalculateLineBreaks()
{
	m_bRecalcLineBreaks = false;
	_recalcSavedRenderState = true;
	if (!m_TextStream.Count())
		return;

	int wide = GetWide();
	
	// subtract the scrollbar width
	if (_vertScrollBar->IsVisible())
	{
		wide -= _vertScrollBar->GetWide();
	}
	
	int charWidth;
	int x = _drawOffsetX, y = _drawOffsetY;
	
	int wordStartIndex = 0;
	int wordLength = 0;
	bool hasWord = false;
	bool justStartedNewLine = true;
	bool wordStartedOnNewLine = true;
	
	int startChar = 0;
	if (_recalculateBreaksIndex <= 0)
	{
		m_LineBreaks.RemoveAll();
		m_LineBreaksLength.RemoveAll();
		m_LineBreaksHeight.RemoveAll();

		m_LineBreaksLength.AddToTail( 0 );
		m_LineBreaksHeight.AddToTail( 0 );
	}
	else
	{
		// remove the rest of the linebreaks list since its out of date.
		for (int i = _recalculateBreaksIndex + 1; i < m_LineBreaks.Count(); ++i)
		{
			m_LineBreaks.Remove(i);
			m_LineBreaksLength.Remove(i);
			m_LineBreaksHeight.Remove(i);

			--i; // removing shrinks the list!
		}
		startChar = m_LineBreaks[_recalculateBreaksIndex];
	}
	
	// handle the case where this char is a new line, in that case
	// we have already taken its break index into account above so skip it.
	if (m_TextStream[startChar] == '\r' || m_TextStream[startChar] == '\n') 
	{
		startChar++;
	}
	
	// cycle to the right url panel	for what is visible	after the startIndex.
	int clickableTextNum = GetClickableTextIndexStart(startChar);
	clickableTextNum++;

	// initialize the renderstate with the start
	TRenderState renderState;
	GenerateRenderStateForTextStreamIndex(startChar, renderState);
	_currentTextClickable = false;
	
	// loop through all the characters	
	for (int i = startChar; i < m_TextStream.Count(); ++i)
	{
		wchar_t ch = m_TextStream[i];
		renderState.x = x;
		if ( UpdateRenderState(i, renderState) )
		{
			x = renderState.x;
			int preI = i;
			
			// check for clickable text
			if (renderState.textClickable != _currentTextClickable)
			{
				if (renderState.textClickable)
				{
					// make a new clickable text panel
					if (clickableTextNum >= _clickableTextPanels.Count())
					{
						_clickableTextPanels.AddToTail(new ClickPanel(this));
					}
					
					ClickPanel *clickPanel = _clickableTextPanels[clickableTextNum++];
					clickPanel->SetTextIndex(preI);
				}
				
				// url state change
				_currentTextClickable = renderState.textClickable;
			}
		}
		
		// line break only on whitespace characters
		if (!iswspace(ch))
		{
			if (hasWord)
			{
				// append to the current word
			}
			else
			{
				// Start a new word
				wordStartIndex = i;
				hasWord = true;
				wordStartedOnNewLine = justStartedNewLine;
				wordLength = 0;
			}
		}
		else
		{
			// whitespace/punctuation character
			// end the word
			hasWord = false;
		}
		
		// get the width
		charWidth = surface()->GetCharacterWidth(renderState.textFont, ch);

		if (!iswcntrl(ch))
		{
			justStartedNewLine = false;
		}
				
		// check to see if the word is past the end of the line [wordStartIndex, i)
		if ((x + charWidth) >= wide || ch == '\r' || ch == '\n')
		{
			// add another line
			AddAnotherLine(x, y);
			
			justStartedNewLine = true;
			hasWord = false;
			
			if (ch == '\r' || ch == '\n')
			{
				// set the break at the current character
				m_LineBreaks.AddToTail(i);
				m_LineBreaksLength.AddToTail( 0 );
				m_LineBreaksHeight.AddToTail( 0 );
			}
			else if (wordStartedOnNewLine || iswspace(ch) ) // catch the "blah             " case wrapping around a line
			{
				// Remove the length of the built up word
				m_LineBreaksLength.Tail() -= wordLength - surface()->GetCharacterWidth(renderState.textFont, L' ');

				// word is longer than a line, so set the break at the current cursor
				m_LineBreaks.AddToTail(i);
				m_LineBreaksLength.AddToTail( 0 );
				m_LineBreaksHeight.AddToTail( 0 );
				
				if (renderState.textClickable)
				{
					// need to split the url into two panels
					int oldIndex = _clickableTextPanels[clickableTextNum - 1]->GetTextIndex();
					
					// make a new clickable text panel
					if (clickableTextNum >= _clickableTextPanels.Count())
					{
						_clickableTextPanels.AddToTail(new ClickPanel(this));
					}
					
					ClickPanel *clickPanel = _clickableTextPanels[clickableTextNum++];
					clickPanel->SetTextIndex(oldIndex);
				}
			}
			else
			{
				// Remove the length of the built up word
				m_LineBreaksLength.Tail() -= wordLength - surface()->GetCharacterWidth(renderState.textFont, L' ');

				// set it at the last word Start
				m_LineBreaks.AddToTail(wordStartIndex);
				m_LineBreaksLength.AddToTail( 0 );
				m_LineBreaksHeight.AddToTail( 0 );
				
				// just back to reparse the next line of text
				i = wordStartIndex;
			}
			
			// reset word length
			wordLength = 0;
		}
		
		// add to the size
		x += charWidth;
		wordLength += charWidth;

		// Increase our width and set our max height
		m_LineBreaksLength.Tail() += charWidth;
		m_LineBreaksHeight.Tail() = max( m_LineBreaksHeight.Tail(), surface()->GetFontTall(renderState.textFont) );

	}

	m_CurrRenderState = NULL;
	
	// end the list
	m_LineBreaks.AddToTail(MAX_BUFFER_SIZE);
	m_LineBreaksLength.AddToTail(0);
	m_LineBreaksHeight.AddToTail(0);
	
	// set up the scrollbar
	_invalidateVerticalScrollbarSlider = true;
}

//-----------------------------------------------------------------------------
// Purpose: Recalculate where the vertical scroll bar slider should be 
//			based on the current cursor line we are on.
//-----------------------------------------------------------------------------
void GECreditsText::LayoutVerticalScrollBarSlider()
{
	_invalidateVerticalScrollbarSlider = false;

	// set up the scrollbar
	//if (!_vertScrollBar->IsVisible())
	//	return;


	// see where the scrollbar currently is
	int previousValue = _vertScrollBar->GetValue();
	bool bCurrentlyAtEnd = false;
	int rmin, rmax;
	_vertScrollBar->GetRange(rmin, rmax);
	if (rmax && (previousValue + rmin + _vertScrollBar->GetRangeWindow() == rmax))
	{
		bCurrentlyAtEnd = true;
	}
	
	// work out position to put scrollbar, factoring in insets
	int wide, tall;
	GetSize( wide, tall );

	_vertScrollBar->SetPos( wide - _vertScrollBar->GetWide(), 0 );
	// scrollbar is inside the borders.
	_vertScrollBar->SetSize( _vertScrollBar->GetWide(), tall );
	
	// calculate how many lines we can fully display
	int numLines = m_LineBreaks.Count();
	int displayLines = 0, linesTall = _drawOffsetY;

	for ( int i=0; i < m_LineBreaksHeight.Count(); i++ )
	{
		linesTall += m_LineBreaksHeight[i];
		if ( linesTall <= tall )
			displayLines++;
		else
			break;
	}
	
	if (numLines <= displayLines)
	{
		// disable the scrollbar
		_vertScrollBar->SetEnabled(false);
		_vertScrollBar->SetRange(0, numLines);
		_vertScrollBar->SetRangeWindow(numLines);
		_vertScrollBar->SetValue(0);

		if ( m_bUnusedScrollbarInvis )
		{
			SetVerticalScrollbar( false );
		}
	}
	else
	{
		if ( m_bUnusedScrollbarInvis )
		{
			SetVerticalScrollbar( true );
		}

		// set the scrollbars range
		_vertScrollBar->SetRange(0, numLines);
		_vertScrollBar->SetRangeWindow(displayLines);
		_vertScrollBar->SetEnabled(true);
		
		// this should make it scroll one line at a time
		_vertScrollBar->SetButtonPressedScrollValue(1);
		if (bCurrentlyAtEnd)
		{
			_vertScrollBar->SetValue(numLines - displayLines);
		}
		_vertScrollBar->InvalidateLayout();
		_vertScrollBar->Repaint();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Scrolls the list 
// Input  : delta - amount to move scrollbar up
//-----------------------------------------------------------------------------
void GECreditsText::MoveScrollBar(int delta)
{
	MoveScrollBarDirect( delta * 3 );
}

//-----------------------------------------------------------------------------
// Purpose: Scrolls the list 
// Input  : delta - amount to move scrollbar up
//-----------------------------------------------------------------------------
void GECreditsText::MoveScrollBarDirect(int delta)
{
	int val = _vertScrollBar->GetValue();
	val -= delta;
	_vertScrollBar->SetValue(val);
	_recalcSavedRenderState = true;
}
