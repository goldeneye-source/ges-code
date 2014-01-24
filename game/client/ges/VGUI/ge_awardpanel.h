
#ifndef GE_AWARDPANEL_H
#define GE_AWARDPANEL_H

#include <vgui/VGUI.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/EditablePanel.h>

namespace vgui
{

class CGEAwardPanel : public EditablePanel
{
public:
	DECLARE_CLASS_SIMPLE( CGEAwardPanel, vgui::EditablePanel );

	CGEAwardPanel( Panel *parent, const char *name );

};

};

#endif
