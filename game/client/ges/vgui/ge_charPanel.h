
#ifndef GE_CHARPANEL_H
#define GE_CHARPANEL_H

#include <vgui/VGUI.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Label.h>
#include "ge_character_data.h"

namespace vgui
{

class GECharPanel : public ImagePanel
{
public:
	DECLARE_CLASS_SIMPLE( GECharPanel, ImagePanel );

	GECharPanel( Panel *parent, const char *name );

	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void PerformLayout();

	void SetCharacter( const CGECharData *pChar );
	void SetSelected( bool state );

	virtual void OnMousePressed(MouseCode code);
	virtual void OnMouseDoublePressed(MouseCode code);

private:
//	Label  *m_pCharName;
	char	m_szCharIdent[MAX_CHAR_IDENT];
	char	m_szOnImage[128];
	char	m_szOffImage[128];
};

} // vgui namespace

#endif
