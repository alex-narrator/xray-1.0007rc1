#include "stdafx.h"
#include "WarBelt.h"


CWarBelt::CWarBelt()
{
	SetSlot(WARBELT_SLOT);

	m_iMaxBeltWidth		= 0;
	m_iMaxBeltHeight	= 0;
	m_bDropPouch		= false;
}


CWarBelt::~CWarBelt()
{
}

void CWarBelt::Load(LPCSTR section)
{
	inherited::Load(section);

	m_iMaxBeltWidth		= READ_IF_EXISTS(pSettings, r_u32, section, "max_belt_width", 0);
	m_iMaxBeltHeight	= READ_IF_EXISTS(pSettings, r_u32, section, "max_belt_height", 1);
	m_bDropPouch		= !!READ_IF_EXISTS(pSettings, r_bool, section, "drop_pouch", false);
}
