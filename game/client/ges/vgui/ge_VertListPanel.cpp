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

#include "ge_VertListPanel.h"

#include <KeyValues.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

DECLARE_BUILD_FACTORY( GEVertListPanel );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
GEVertListPanel::GEVertListPanel( vgui::Panel *parent, char const *panelName ) : Panel( parent, panelName )
{
	m_vbar = new ScrollBar(this, "VertListPanelVScroll", true);
	m_vbar->SetVisible(false);
	m_vbar->AddActionSignalTarget( this );
	m_vbar->SetScheme( "ClientScheme" );

	m_pPanelEmbedded = new EditablePanel(this, "PanelListEmbedded");
	m_pPanelEmbedded->SetBounds(0, 0, 20, 20);
	m_pPanelEmbedded->SetPaintBackgroundEnabled(false);
	m_pPanelEmbedded->SetPaintBorderEnabled(false);

	m_iFirstColWidth = 45; // default width
	m_iScrollRate = scheme()->GetProportionalScaledValue( 5 );  // Scroll by 5 proportional pixels per frame
	m_iState = STATE_IDLE;
	m_flNextThink = gpGlobals->curtime;
	m_iScrollTo = 0;

	m_bForceHideScrollBar = false;
	m_iPanelBuffer = PANELBUFFER;
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
GEVertListPanel::~GEVertListPanel()
{
	// free data from table
	ClearItems();
}

void GEVertListPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	ForceHideScrollBar( inResourceData->GetInt("scrollbar", 0) != 0 );

	SetBufferPixels( inResourceData->GetInt("padding", PANELBUFFER) );
}

void GEVertListPanel::Reset( void )
{
	m_flNextThink = gpGlobals->curtime;
}

void GEVertListPanel::OnThink( void )
{
	BaseClass::OnThink();

	if ( gpGlobals->curtime < m_flNextThink )
		return;

	int pos = m_vbar->GetValue();

	switch( m_iState )
	{
	case STATE_STARTSCROLL:
		if ( pos > m_iScrollTo )
			m_iState = STATE_SCROLL_UP;
		else
			m_iState = STATE_SCROLL_DOWN;
		// Take into account the buffer so that we aren't chopping it off
		m_iScrollTo -= scheme()->GetProportionalScaledValue( m_iPanelBuffer );
		break;

	case STATE_SCROLL_UP:
		if ( (pos - m_iScrollRate) > m_iScrollTo ) {
			m_vbar->SetValue( pos - m_iScrollRate );
			InvalidateLayout(true);
		} else {
			m_vbar->SetValue( m_iScrollTo );
			InvalidateLayout(true);
			m_iState = STATE_IDLE;
		}
		break;

	case STATE_SCROLL_DOWN:
		if ( (pos + m_iScrollRate) < m_iScrollTo ) {
			m_vbar->SetValue( pos + m_iScrollRate );
			InvalidateLayout(true);
		} else {
			m_vbar->SetValue( m_iScrollTo );
			InvalidateLayout(true);
			m_iState = STATE_IDLE;
		}
		break;
	}

	m_flNextThink = gpGlobals->curtime + 0.01;
}

void GEVertListPanel::SetBufferPixels( int buffer )
{
	m_iPanelBuffer = buffer;
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: counts the total vertical pixels
//-----------------------------------------------------------------------------
int	GEVertListPanel::ComputeVPixelsNeeded( bool bWithScrollBar /* = false */)
{
	int buffer		= scheme()->GetProportionalScaledValue( m_iPanelBuffer ); // add in buffer before first item
	int panel_wide	= GetWide();
	int pixels		= buffer;

	for ( int i=0; i < m_SortedItems.Count(); i++ )
	{
		DATAITEM item = m_DataItems[ m_SortedItems[i] ];
		if ( !item.panel )
			continue;

		int w, h;
		item.panel->GetSize( w, h );

		if ( item.flags & LF_AUTOSIZE )
		{
			// Resize to panel width, maintain proportionality
			float ratio = (float) h / w;

			w = panel_wide - buffer - buffer;
			if ( bWithScrollBar )
				w -= m_vbar->GetWide();

			h = ratio * w;
		}

		pixels += h + buffer + buffer;
	}

	return pixels + buffer;
}

//-----------------------------------------------------------------------------
// Purpose: Figures out what we should be doing with the scroll bar (called when the list changes)
//-----------------------------------------------------------------------------
void GEVertListPanel::CalcScrollBar( void )
{
	int wide, tall;
	GetSize( wide, tall );

	// First find out how wide the content is w/o a scrollbar
	int vpixels = ComputeVPixelsNeeded();

	// If we have more content then our width can handle we need to make the scroll bar
	// unless we are forcing it to be hidden
	if ( vpixels > tall && !m_bForceHideScrollBar ) {
		m_vbar->GetButton(0)->SetText(L"<");
		m_vbar->GetButton(1)->SetText(L">");

		m_vbar->SetSize( scheme()->GetProportionalScaledValue(10), tall );
		m_vbar->SetPos( wide - m_vbar->GetWide(), 0 );
		m_vbar->InvalidateLayout();

		// Recalculate the content width with the scrollbar
		vpixels = ComputeVPixelsNeeded( true );

		m_vbar->SetVisible( true );
		m_vbar->SetRange( 0, vpixels );
		m_vbar->SetRangeWindow( tall );
		m_vbar->SetButtonPressedScrollValue( tall / 4 ); // standard width of labels/buttons etc.
	} else {
		m_vbar->SetVisible( false );
		m_vbar->SetSize( 0, tall );
		m_vbar->SetRange( 0, vpixels );
		m_vbar->SetRangeWindow( tall );
	}

	// Update the embedded panel so that it works with our scrollbar arrangement
	m_pPanelEmbedded->SetSize( wide - m_vbar->GetWide(), vpixels );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the panel to use to render a cell
//-----------------------------------------------------------------------------
Panel *GEVertListPanel::GetCellRenderer( int row )
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
int GEVertListPanel::AddItem( Panel *panel, LayoutFlags lf /*=LF_NONE*/ )
{
	return AddItem( NULL, panel, lf );
}

int GEVertListPanel::AddItem( Panel *labelPanel, Panel *panel, LayoutFlags lf /*=LF_NONE*/ )
{
	Assert(panel);

	if ( labelPanel )
		labelPanel->SetParent( m_pPanelEmbedded );

	panel->SetParent( m_pPanelEmbedded );

	int itemID = m_DataItems.AddToTail();

	DATAITEM &newitem	= m_DataItems[itemID];
	newitem.labelPanel	= labelPanel;
	newitem.panel		= panel;
	newitem.flags		= lf;

	m_SortedItems.AddToTail( itemID );

	CalcScrollBar();
	InvalidateLayout();

	return itemID;
}

//-----------------------------------------------------------------------------
// Purpose: iteration accessor
//-----------------------------------------------------------------------------
int	GEVertListPanel::GetItemCount()
{
	return m_DataItems.Count();
}

//-----------------------------------------------------------------------------
// Purpose: returns label panel for this itemID
//-----------------------------------------------------------------------------
Panel *GEVertListPanel::GetItemLabel(int itemID)
{
	if ( !m_DataItems.IsValidIndex(itemID) )
		return NULL;

	return m_DataItems[itemID].labelPanel;
}

//-----------------------------------------------------------------------------
// Purpose: returns label panel for this itemID
//-----------------------------------------------------------------------------
Panel *GEVertListPanel::GetItemPanel(int itemID)
{
	if ( !m_DataItems.IsValidIndex(itemID) )
		return NULL;

	return m_DataItems[itemID].panel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void GEVertListPanel::RemoveItem(int itemID)
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
void GEVertListPanel::ClearItems()
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

	m_vbar->SetValue( 0 );
	CalcScrollBar();
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void GEVertListPanel::OnSizeChanged(int wide, int tall)
{
	BaseClass::OnSizeChanged(wide, tall);
	CalcScrollBar();
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: relayouts the panel after any internal changes
//-----------------------------------------------------------------------------
void GEVertListPanel::PerformLayout()
{
	// Lay out the panels on the embedded panel
	int sliderPos = m_vbar->GetValue();
	int panel_wide, panel_tall;
	GetSize( panel_wide, panel_tall );

	m_pPanelEmbedded->SetPos( 0, -sliderPos );

	int buffer = scheme()->GetProportionalScaledValue( m_iPanelBuffer );
	int x, y, w, h, totalh;
	x = y = w = h = totalh = 0;

	for ( int i = 0; i < m_SortedItems.Count(); i++, y += h )
	{
		x = buffer;
		y += buffer + buffer;

		DATAITEM &item = m_DataItems[ m_SortedItems[i] ];
		item.panel->GetSize( w, h );

		if ( item.labelPanel )
		{
			item.labelPanel->SetBounds( buffer, y, m_iFirstColWidth, h );
			x += m_iFirstColWidth + buffer;
		}

		if ( item.flags & LF_AUTOSIZE )
		{
			// Resize to panel width, maintain proportionality
			float ratio = (float) h / w;
			w = panel_wide - buffer - buffer - m_vbar->GetWide();
			h = ratio * w;
		}
		else if ( item.flags & LF_CENTERED )
		{
			// Center us within the buffered zone
			x = (panel_wide / 2) - (w / 2) - m_vbar->GetWide() - buffer - buffer;
		}

		item.panel->SetBounds( x, y, w, h );

		totalh += h;
	}

	BaseClass::PerformLayout();
}

//-----------------------------------------------------------------------------
// Purpose: scheme settings
//-----------------------------------------------------------------------------
void GEVertListPanel::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetBorder(pScheme->GetBorder("ButtonDepressedBorder"));
	SetBgColor(GetSchemeColor("HorzListPanel.BgColor", GetBgColor(), pScheme));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void GEVertListPanel::OnSliderMoved( int position )
{
//	InvalidateLayout();
	m_pPanelEmbedded->SetPos( 0, -position );
	Repaint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void GEVertListPanel::MoveScrollBarUp( void )
{
	int pos = m_vbar->GetValue();
	int itemTall = m_hSelectedItem->GetTall();

	if ( (pos - itemTall) < 0 ) {
		m_iState = STATE_STARTSCROLL;
		m_iScrollTo = 0;
	} else {
		m_iState = STATE_STARTSCROLL;
		m_iScrollTo = pos - itemTall;
	}
}

void GEVertListPanel::MoveScrollBarDown( void )
{
	int pos = m_vbar->GetValue();
	int itemTall = m_hSelectedItem->GetTall();

	int min, max;
	m_vbar->GetRange(min, max);

	if ( (pos + itemTall) > max ) {
		m_iState = STATE_STARTSCROLL;
		m_iScrollTo = max;
	} else {
		m_iState = STATE_STARTSCROLL;
		m_iScrollTo = pos + itemTall;
	}
}

void GEVertListPanel::ScrollToSelectedPanel( void )
{
	int pos = m_vbar->GetValue();
	int x,y;
	int itemTall = m_hSelectedItem->GetTall();
	m_hSelectedItem->GetPos(x,y);

	if ( y < pos || (y + itemTall) > (pos + GetTall()) ) {
		m_iState = STATE_STARTSCROLL;
		m_iScrollTo = y;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void GEVertListPanel::SetFirstColWidth( int width )
{
	m_iFirstColWidth = width;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
int GEVertListPanel::GetFirstColWidth()
{
	return m_iFirstColWidth;
}

//-----------------------------------------------------------------------------
// Purpose: moves the scrollbar with the mousewheel
//-----------------------------------------------------------------------------
void GEVertListPanel::OnMouseWheeled(int delta)
{
	int val = m_vbar->GetValue();
	val -= (delta * DEFAULT_HEIGHT);
	m_vbar->SetValue(val);	
}

//-----------------------------------------------------------------------------
// Purpose: selection handler
//-----------------------------------------------------------------------------
void GEVertListPanel::SetSelectedPanel( Panel *panel )
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

		ScrollToSelectedPanel();
	}
}

void GEVertListPanel::SetSelectedPanel( int idx )
{
	if ( idx >= 0 && idx < m_SortedItems.Count() )
	{
		DATAITEM &item = m_DataItems[ m_SortedItems[idx] ];
		SetSelectedPanel( item.panel );
	}
}

int GEVertListPanel::GetSelected( void )
{
	for ( int i=0; i < m_SortedItems.Count(); i++ )
	{
		DATAITEM &item = m_DataItems[ m_SortedItems[i] ];
		if ( item.panel == m_hSelectedItem )
			return i;
	}
	return -1;
}

Panel *GEVertListPanel::GetPanel( int idx )
{
	if ( idx >= 0 && idx < m_SortedItems.Count() )
	{
		DATAITEM &item = m_DataItems[ m_SortedItems[idx] ];
		return item.panel;
	}
	return NULL;
}

int GEVertListPanel::GetPanelIndex( Panel *panel )
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
Panel *GEVertListPanel::GetSelectedPanel()
{
	return m_hSelectedItem;
}