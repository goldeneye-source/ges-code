//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef GEVERTLISTPANEL_H
#define GEVERTLISTPANEL_H

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include "utllinkedlist.h"
#include "utlvector.h"

class KeyValues;

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: A list of variable height child panels
//  each list item consists of a label-panel pair. Height of the item is
// determined from the label.
//-----------------------------------------------------------------------------
class GEVertListPanel : public Panel
{
	DECLARE_CLASS_SIMPLE( GEVertListPanel, Panel );

public:
	GEVertListPanel( vgui::Panel *parent, char const *panelName );
	~GEVertListPanel();

	enum LayoutFlags
	{
		LF_NONE = 0,
		LF_AUTOSIZE = 0x1,	// Resize items to fit the panel width
		LF_CENTERED = 0x2,	// Center items in the panel
	};

	virtual void Reset( void );
	virtual void OnThink( void );

	virtual void ApplySettings( KeyValues *inResourceData );

	// DATA & ROW HANDLING
	// The list now owns the panel
	virtual int AddItem( Panel *panel, LayoutFlags flags = LF_NONE );
	virtual int AddItem( Panel *panel, Panel *labelPanel, LayoutFlags flags = LF_NONE );

	virtual int	GetItemCount();
	virtual int Count() { return GetItemCount(); }

	virtual Panel *GetItemLabel(int itemID); 
	virtual Panel *GetItemPanel(int itemID); 

	virtual void RemoveItem(int itemID); // removes an item from the table (changing the indices of all following items)
	virtual void ClearItems(); // clears and deletes all the memory used by the data items

	// painting
	virtual vgui::Panel *GetCellRenderer( int row );

	// layout
	void SetFirstColWidth( int width );
	int  GetFirstColWidth();
	void MoveScrollBarUp();
	void MoveScrollBarDown();
	void ScrollToSelectedPanel();
	void ForceHideScrollBar( bool state ) { m_bForceHideScrollBar = state; InvalidateLayout(); };

	// selection
	int		GetPanelIndex( Panel *panel );
	void	SetSelectedPanel( Panel *panel );
	void	SetSelectedPanel( int idx );
	int		GetSelected( void );
	Panel  *GetSelectedPanel();
	Panel  *GetPanel( int idx );
	
	/*
		On a panel being selected, a message gets sent to it
			"PanelSelected"		int "state"
		where state is 1 on selection, 0 on deselection
	*/

	void SetBufferPixels( int buffer );

protected:
	// overrides
	virtual void OnSizeChanged( int wide, int tall );
	MESSAGE_FUNC_INT( OnSliderMoved, "ScrollBarSliderMoved", position );
	virtual void PerformLayout();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnMouseWheeled( int delta );

private:
	int	ComputeVPixelsNeeded( bool bWithScrollBar = false );
	void CalcScrollBar( void );

	enum { DEFAULT_HEIGHT = 60, PANELBUFFER = 10 };
	enum { STATE_IDLE = 0, STATE_STARTSCROLL, STATE_SCROLL_UP, STATE_SCROLL_DOWN };

	typedef struct dataitem_s
	{
		// Always store a panel pointer
		Panel *panel;
		Panel *labelPanel;
		LayoutFlags flags;
	} DATAITEM;

	// list of the column headers

	CUtlLinkedList<DATAITEM, int>		m_DataItems;
	CUtlVector<int>						m_SortedItems;

	ScrollBar		*m_vbar;
	Panel			*m_pPanelEmbedded;

	PHandle			m_hSelectedItem;
	int				m_iFirstColWidth;
	int				m_iPanelBuffer;
	bool			m_bForceHideScrollBar;
	int				m_iState;
	float			m_flNextThink;

	int				m_iScrollRate;
	int				m_iScrollTo;
};

}
#endif // GEVERTLISTPANEL_H
