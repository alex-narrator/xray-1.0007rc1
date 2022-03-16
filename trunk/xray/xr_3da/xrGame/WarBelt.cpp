#include "stdafx.h"
#include "WarBelt.h"

#include "Inventory.h"
#include "Actor.h"


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

void CWarBelt::OnMoveToSlot(EItemPlace previous_place)
{
	if (m_pCurrentInventory)
	{
		CActor* pActor = smart_cast<CActor*> (m_pCurrentInventory->GetOwner());
		if (pActor && pActor->bAllItemsLoaded)
		{
			m_pCurrentInventory->DropBeltToRuck();
		}
	}
}

void CWarBelt::OnMoveToRuck(EItemPlace previous_place)
{
	if (m_pCurrentInventory && previous_place == eItemPlaceSlot)
	{
		auto* pActor = smart_cast<CActor*> (m_pCurrentInventory->GetOwner());
		if (pActor && pActor->bAllItemsLoaded)
		{
			m_pCurrentInventory->DropBeltToRuck();
		}
	}
}

void CWarBelt::OnMoveOut(EItemPlace previous_place)
{
	OnMoveToRuck(previous_place);
}