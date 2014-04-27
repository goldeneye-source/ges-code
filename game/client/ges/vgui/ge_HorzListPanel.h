//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef GEHORZLISTPANEL_H
#define GEHORZLISTPANEL_H

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include "utllinkedlist.h"

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose: A list of variable height child panels
//  each list item consists of a label-panel pair. Height of the item is
// determined from the label.
//-----------------------------------------------------------------------------
class GEHorzListPanel : public Panel
{
	DECLARE_CLASS_SIMPLE( GEHorzListPanel, Panel );

public:
	GEHorzListPanel( vgui::Panel *parent, char const *panelName );
	~GEHorzListPanel();

	virtual void Reset( void );
	virtual void OnThink( void );

	// DATA & ROW HANDLING
	// The list now owns the panel
	virtual int AddItem( Panel *labelPanel, Panel *panel );
	virtual int	GetItemCount();
	virtual int Count() { return GetItemCount(); }

	virtual Panel *GetItemLabel(int itemID); 
	virtual Panel *GetItemPanel(int itemID); 

	virtual void RemoveItem(int itemID); // removes an item from the table (changing the indices of all following items)
	virtual void DeleteAllItems(); // clears and deletes all the memory used by the data items
	void RemoveAll();

	// painting
	virtual vgui::Panel *GetCellRenderer( int row );

	// layout
	void SetFirstRowHeight( int height );
	int GetFirstRowHeight();
	void MoveScrollBarToLeft();
	void MoveScrollBarToRight();
	void ScrollToSelectedPanel();
	void ForceHideScrollBar( bool state ) { m_bForceHideScrollBar = state; InvalidateLayout(); };

	// selection
	void SetSelectedPanel( Panel *panel, bool donotscroll = false );
	void SetSelectedPanel( int idx, bool donotscroll = false );
	int GetSelected( void );
	Panel *GetSelectedPanel();
	Panel *GetPanel( int idx );
	int GetPanelIndex( Panel *panel );
	/*
		On a panel being selected, a message gets sent to it
			"PanelSelected"		int "state"
		where state is 1 on selection, 0 on deselection
	*/

	void		SetBufferPixels( int buffer );

protected:
	// overrides
	virtual void OnSizeChanged(int wide, int tall);
	MESSAGE_FUNC_INT( OnSliderMoved, "ScrollBarSliderMoved", position );
	virtual void PerformLayout();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void OnMouseWheeled(int delta);

private:
	int	ComputeHPixelsNeeded( bool bWithScrollBar = false );
	void CalcScrollBar( void );

	enum { DEFAULT_WIDTH = 120, PANELBUFFER = 10 };
	enum { STATE_IDLE = 0, STATE_STARTSCROLL, STATE_SCROLL_LEFT, STATE_SCROLL_RIGHT };

	typedef struct dataitem_s
	{
		// Always store a panel pointer
		Panel *panel;
		Panel *labelPanel;
	} DATAITEM;

	// list of the column headers

	CUtlLinkedList<DATAITEM, int>		m_DataItems;
	CUtlVector<int>						m_SortedItems;

	ScrollBar		*m_hbar;
	Panel			*m_pPanelEmbedded;

	PHandle			m_hSelectedItem;
	int				m_iFirstRowHeight;
	int				m_iDefaultWidth;
	int				m_iPanelBuffer;
	bool			m_bForceHideScrollBar;
	int				m_iState;
	float			m_flNextThink;

	int				m_iScrollRate;
	int				m_iFastScrollRate;
	bool			m_bUseFastScroll;
	int				m_iScrollTo;
};

}
#endif // GEHORZLISTPANEL_H
