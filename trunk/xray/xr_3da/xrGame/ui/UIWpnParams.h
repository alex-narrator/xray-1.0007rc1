#pragma once
#include "../GameObject.h"
#include "UIWindow.h"
#include "UIProgressBar.h"

class CUIXml;

#include "../script_export_space.h"

struct SLuaWpnParams;

class CUIWpnParams : public CUIWindow 
{
public:
								CUIWpnParams			();
	virtual						~CUIWpnParams			();

	void 						InitFromXml				(CUIXml& xml_doc);
	void 						SetInfo					(CInventoryItem *wpn);
	bool 						Check					(CInventoryItem *wpn);

protected:
	CUIProgressBar				m_progressAccuracy;
	CUIProgressBar				m_progressHandling;
	CUIProgressBar				m_progressDamage;
	CUIProgressBar				m_progressRPM;

	CUIStatic					m_textAccuracy;
	CUIStatic					m_textHandling;
	CUIStatic					m_textDamage;
	CUIStatic					m_textRPM;
	//
	CUIStatic					m_textCurrentAmmo;
	CUIStatic					m_textMagSizeFiremode;
};