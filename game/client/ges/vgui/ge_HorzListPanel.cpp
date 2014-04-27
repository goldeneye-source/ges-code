#include "cbase.h"
#include <cdll_client_int.h>

#include <vgui/MouseCode.h>
#include <vgui/IInput.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ScrollBar.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Controls.h>

#include "ge_HorzListPanel.h"

#include <KeyValues.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

DECLARE_BUILD_FACTORY( GEHorzListPanel );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
GEHorzListPanel::GEHorzListPanel( vgui::Panel *parent, char const *panelName ) : Panel( parent, panelName )
{
	m_hbar = new ScrollBar(this, "PanelListPanelHScroll", false);
	m_hbar->SetVisible(false);
	m_hbar->AddActionSignalTarget( this );
	m_hbar->SetScheme( "ClientScheme" );

	m_pPanelEmbedded = new EditablePanel(this, "PanelListEmbedded");
	m_pPanelEmbedded->SetBounds(0, 0, 20, 20);
	m_pPanelEmbedded->SetPaintBackgroundEnabled( false );
	m_pPanelEmbedded->SetPaintBorderEnabled(false);

	m_iFirstRowHeight = 45; // default height
	m_iScrollRate = scheme()->GetProportionalScaledValue( 5 );  // Scroll by 5 proportional pixels per frame
	m_iFastScrollRate = scheme()->GetProportionalScaledValue( 10 ); // For jumping to spots
	m_iState = STATE_IDLE;
	m_flNextThink = gpGlobals->curtime;
	m_iScrollTo = 0;
	m_bUseFastScroll = false;

	m_bForceHideScrollBar = false;

	if ( IsProportional() )
	{
		m_iDefaultWidth = scheme()->GetProportionalScaledValue( DEFAULT_WIDTH );
		m_iPanelBuffer = scheme()->GetProportionalScaledValue( PANELBUFFER );
	}
	else
	{
		m_iDefaultWidth = DEFAULT_WIDTH;
		m_iPanelBuffer = PANELBUFFER;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
GEHorzListPanel::~GEHorzListPanel()
{
	// free data from table
	DeleteAllItems();
}

void GEHorzListPanel::Reset( void )
{
	m_flNextThink = gpGlobals->curtime;
}

void GEHorzListPanel::OnThink( void )
{
	BaseClass::OnThink();

	if ( gpGlobals->curtime < m_flNextThink )
		return;

	int pos = m_hbar->GetValue();
	int speed = m_bUseFastScroll ? m_iFastScrollRate : m_iScrollRate;

	switch( m_iState )
	{
	case STATE_STARTSCROLL:
		if ( pos > m_iScrollTo )
			m_iState = STATE_SCROLL_LEFT;
		else
			m_iState = STATE_SCROLL_RIGHT;
		// Take into account the buffer so that we aren't chopping it off
		m_iScrollTo -= scheme()->GetProportionalScaledValue( m_iPanelBuffer );
		break;

	case STATE_SCROLL_LEFT:
		if ( (pos - speed) > m_iScrollTo ) {
			m_hbar->SetValue( pos - speed );
			InvalidateLayout(true);
		} else {
			m_hbar->SetValue( m_iScrollTo );
			InvalidateLayout(true);
			m_iState = STATE_IDLE;
		}
		break;

	case STATE_SCROLL_RIGHT:
		if ( (pos + speed) < m_iScrollTo ) {
			m_hbar->SetValue( pos + speed );
			InvalidateLayout(true);
		} else {
			m_hbar->SetValue( m_iScrollTo );
			InvalidateLayout(true);
			m_iState = STATE_IDLE;
		}
		break;
	}

	m_flNextThink = gpGlobals->curtime + 0.01;
}

void GEHorzListPanel::SetBufferPixels( int buffer )
{
	m_iPanelBuffer = buffer;
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: counts the total vertical pixels
//-----------------------------------------------------------------------------
int	GEHorzListPanel::ComputeHPixelsNeeded( bool bWithScrollBar /* = false */)
{
	int buffer = scheme()->GetProportionalScaledValue( m_iPanelBuffer ); // add in buffer before first item
	int itemtall, pixels;
	int tall = GetTall();

	pixels = buffer;

	for ( int i = 0; i < m_SortedItems.Count(); i++ )
	{
		Panel *panel = m_DataItems[ m_SortedItems[i] ].panel;
		if ( !panel )
			continue;

		int w, h;
		panel->GetSize( w, h );

		// Assume no scrollbar to give us the biggest possible scenario
		itemtall = tall - buffer - buffer;
		if ( bWithScrollBar )
			itemtall -= m_hbar->GetTall();

		w = (w * itemtall) / h;

		pixels += w + buffer;
	}

	pixels += buffer; // add in buffer after last item

	return pixels;
}

//-----------------------------------------------------------------------------
// Purpose: Figures out what we should be doing with the scroll bar (called when the list changes)
//-----------------------------------------------------------------------------
void GEHorzListPanel::CalcScrollBar( void )
{
	int wide, tall;
	GetSize( wide, tall );

	// First find out how wide the content is w/o a scrollbar
	int hpixels = ComputeHPixelsNeeded();

	// If we have more content then our width can handle we need to make the scroll bar
	// unless we are forcing it to be hidden
	if ( hpixels > wide && !m_bForceHideScrollBar ) {
		m_hbar->GetButton(0)->SetText(L"<");
		m_hbar->GetButton(1)->SetText(L">");

		m_hbar->SetSize( wide, scheme()->GetProportionalScaledValue(10) );
		m_hbar->SetPos( 0, tall - m_hbar->GetTall() );
		m_hbar->InvalidateLayout();

		// Recalculate the content width with the scrollbar
		hpixels = ComputeHPixelsNeeded( true );

		m_hbar->SetVisible( true );
		m_hbar->SetButtonPressedScrollValue( wide / 4 ); // standard width of labels/buttons etc.
	} else {
		m_hbar->SetVisible( false );
		m_hbar->SetSize( wide, 0 );
	}

	m_hbar->SetRange( 0, hpixels );
	m_hbar->SetRangeWindow( wide );

	// Update the embedded panel so that it works with our scrollbar arrangement
	m_pPanelEmbedded->SetSize( hpixels, tall - m_hbar->GetTall() );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the panel to use to render a cell
//-----------------------------------------------------------------------------
Panel *GEHorzListPanel::GetCellRenderer( int row )
{
	if ( !m_SortedItems.IsValidIndex(row) )
		return NULL;

	Panel *panel = m_DataItems[ m_SortedItems[row] ].panel;
	return panel;
}

//-----------------------------------------------------------------------------
// Purpose: adds an item to the view
//			data->GetName() is used to uniquely identify an item
//			data sub items are matched against column header name to be used in the table
//-----------------------------------------------------------------------------
int GEHorzListPanel::AddItem( Panel *labelPanel, Panel *panel)
{
	Assert(panel);

	if ( labelPanel )
	{
		labelPanel->SetParent( m_pPanelEmbedded );
	}

	panel->SetParent( m_pPanelEmbedded );

	int itemID = m_DataItems.AddToTail();
	DATAITEM &newitem = m_DataItems[itemID];
	newitem.labelPanel = labelPanel;
	newitem.panel = panel;
	m_SortedItems.AddToTail(itemID);

	CalcScrollBar();
	InvalidateLayout();
	return itemID;
}

//-----------------------------------------------------------------------------
// Purpose: iteration accessor
//-----------------------------------------------------------------------------
int	GEHorzListPanel::GetItemCount()
{
	return m_DataItems.Count();
}

//-----------------------------------------------------------------------------
// Purpose: returns label panel for this itemID
//-----------------------------------------------------------------------------
Panel *GEHorzListPanel::GetItemLabel(int itemID)
{
	if ( !m_DataItems.IsValidIndex(itemID) )
		return NULL;

	return m_DataItems[itemID].labelPanel;
}

//-----------------------------------------------------------------------------
// Purpose: returns label panel for this itemID
//-----------------------------------------------------------------------------
Panel *GEHorzListPanel::GetItemPanel(int itemID)
{
	if ( !m_DataItems.IsValidIndex(itemID) )
		return NULL;

	return m_DataItems[itemID].panel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void GEHorzListPanel::RemoveItem(int itemID)
{
	if ( !m_DataItems.IsValidIndex(itemID) )
		return;

	DATAITEM &item = m_DataItems[itemID];
	if ( item.panel )
	{
		item.panel->MarkForDeletion();
	}
	if ( item.labelPanel )
	{
		item.labelPanel->MarkForDeletion();
	}

	m_DataItems.Remove(itemID);
	m_SortedItems.FindAndRemove(itemID);

	CalcScrollBar();
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: clears and deletes all the memory used by the data items
//-----------------------------------------------------------------------------
void GEHorzListPanel::DeleteAllItems()
{
	FOR_EACH_LL( m_DataItems, i )
	{
		if ( m_DataItems[i].panel )
		{
			delete m_DataItems[i].panel;
		}
	}

	m_DataItems.RemoveAll();
	m_SortedItems.RemoveAll();

	CalcScrollBar();
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: clears and deletes all the memory used by the data items
//-----------------------------------------------------------------------------
void GEHorzListPanel::RemoveAll()
{
	m_DataItems.RemoveAll();
	m_SortedItems.RemoveAll();

	// move the scrollbar to the top of the list
	m_hbar->SetValue(0);
	CalcScrollBar();
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void GEHorzListPanel::OnSizeChanged(int wide, int tall)
{
	BaseClass::OnSizeChanged(wide, tall);
	CalcScrollBar();
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: relayouts the panel after any internal changes
//-----------------------------------------------------------------------------
void GEHorzListPanel::PerformLayout()
{
	// Lay out the panels on the embedded panel
	int x, w, h, totalw, y_inset, itemtall, ypos;
	int sliderPos = m_hbar->GetValue();
	int wide, tall;
	GetSize( wide, tall );

	m_pPanelEmbedded->SetPos( -sliderPos, 0 );

	x = w = h = totalw = y_inset = itemtall = 0;
	ypos = scheme()->GetProportionalScaledValue( m_iPanelBuffer );
	
	for ( int i = 0; i < m_SortedItems.Count(); i++, x += w )
	{
		// add in a little buffer between panels
		x += scheme()->GetProportionalScaledValue( m_iPanelBuffer );
		DATAITEM &item = m_DataItems[ m_SortedItems[i] ];

		// TODO: THIS IS NOT THE CORRECT WIDTH!
		item.panel->GetSize( w,h );

		totalw += w;
		if (totalw >= sliderPos)
		{
			if ( item.labelPanel )
			{
				item.labelPanel->SetBounds( y_inset, x, w, m_iFirstRowHeight );
			}

			itemtall = tall - ypos - ypos - m_hbar->GetTall();
			w = (w * itemtall) / h;  // Maintain proportionality
			item.panel->SetBounds( x, ypos, w, itemtall );
		}	
	}

	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: scheme settings
//-----------------------------------------------------------------------------
void GEHorzListPanel::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetBorder(pScheme->GetBorder("ButtonDepressedBorder"));
	SetBgColor(GetSchemeColor("HorzListPanel.BgColor", GetBgColor(), pScheme));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void GEHorzListPanel::OnSliderMoved( int position )
{
	InvalidateLayout();
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void GEHorzListPanel::MoveScrollBarToLeft( void )
{
	int pos = m_hbar->GetValue();
	int itemWide = m_hSelectedItem->GetWide();
	m_bUseFastScroll = false;

	if ( (pos - itemWide) < 0 ) {
		m_iState = STATE_STARTSCROLL;
		m_iScrollTo = 0;
	} else {
		m_iState = STATE_STARTSCROLL;
		m_iScrollTo = pos - itemWide;
	}
}

void GEHorzListPanel::MoveScrollBarToRight( void )
{
	int pos = m_hbar->GetValue();
	int itemWide = m_hSelectedItem->GetWide();
	m_bUseFastScroll = false;

	int min, max;
	m_hbar->GetRange(min, max);

	if ( (pos + itemWide) > max ) {
		m_iState = STATE_STARTSCROLL;
		m_iScrollTo = max;
	} else {
		m_iState = STATE_STARTSCROLL;
		m_iScrollTo = pos + itemWide;
	}
}

void GEHorzListPanel::ScrollToSelectedPanel( void )
{
	int pos = m_hbar->GetValue();
	int x,y;
	int itemWide = m_hSelectedItem->GetWide();
	m_hSelectedItem->GetPos(x,y);

	// Get there lickity split if we are going more then two widths
	if ( abs(pos - x) > (GetWide() * 2) )
		m_bUseFastScroll = true;
	else
		m_bUseFastScroll = false;

	if ( x < pos || (x + itemWide) > (pos + GetWide()) )
	{
		m_iState = STATE_STARTSCROLL;
		m_iScrollTo = x;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void GEHorzListPanel::SetFirstRowHeight( int height )
{
	m_iFirstRowHeight = height;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
int GEHorzListPanel::GetFirstRowHeight()
{
	return m_iFirstRowHeight;
}

//-----------------------------------------------------------------------------
// Purpose: moves the scrollbar with the mousewheel
//-----------------------------------------------------------------------------
void GEHorzListPanel::OnMouseWheeled(int delta)
{
	int val = m_hbar->GetValue();
	val -= (delta * DEFAULT_WIDTH);
	m_hbar->SetValue(val);	
}

//-----------------------------------------------------------------------------
// Purpose: selection handler
//-----------------------------------------------------------------------------
void GEHorzListPanel::SetSelectedPanel( Panel *panel, bool donotscroll /*= false*/ )
{
	if ( panel != m_hSelectedItem )
	{
		// notify the panels of the selection change
		if ( m_hSelectedItem )
		{
			PostMessage( m_hSelectedItem.Get(), new KeyValues("PanelSelected", "state", 0) );
		}
		if ( panel )
		{
			PostMessage( panel, new KeyValues("PanelSelected", "state", 1) );
		}
		m_hSelectedItem = panel;

		if ( donotscroll )
		{
			int x, y;
			m_hSelectedItem->GetPos( x, y );
			m_hbar->SetValue( x );
			m_iScrollTo = x;

			InvalidateLayout(true);
			m_iState = STATE_IDLE;
		}
		else
		{
			ScrollToSelectedPanel();
		}
	}
}

void GEHorzListPanel::SetSelectedPanel( int idx, bool donotscroll /*= false*/ )
{
	if ( idx >= 0 && idx < m_SortedItems.Count() )
	{
		DATAITEM &item = m_DataItems[ m_SortedItems[idx] ];
		SetSelectedPanel( item.panel, donotscroll );
	}
}

int GEHorzListPanel::GetSelected( void )
{
	for ( int i=0; i < m_SortedItems.Count(); i++ )
	{
		DATAITEM &item = m_DataItems[ m_SortedItems[i] ];
		if ( item.panel == m_hSelectedItem )
			return i;
	}
	return -1;
}

Panel *GEHorzListPanel::GetPanel( int idx )
{
	if ( idx >= 0 && idx < m_SortedItems.Count() )
	{
		DATAITEM &item = m_DataItems[ m_SortedItems[idx] ];
		return item.panel;
	}
	return NULL;
}

int GEHorzListPanel::GetPanelIndex( Panel *panel )
{
	for ( int i=0; i < m_SortedItems.Count(); i++ )
	{
		DATAITEM &item = m_DataItems[ m_SortedItems[i] ];
		if ( item.panel == panel )
			return i;
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
Panel *GEHorzListPanel::GetSelectedPanel()
{
	return m_hSelectedItem;
}