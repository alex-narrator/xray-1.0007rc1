#include "stdafx.h"
#include "BackPack.h"

#include "Inventory.h"
#include "Actor.h"

CBackPack::CBackPack()
{
	SetSlot(BACKPACK_SLOT);
}


CBackPack::~CBackPack()
{
}

void CBackPack::Load(LPCSTR section)
{
	inherited::Load(section);

	/*	m_additional_weight		= READ_IF_EXISTS(pSettings, r_float, section, "additional_inventory_weight", 0.f);
	m_additional_weight2	= READ_IF_EXISTS(pSettings, r_float, section, "additional_inventory_weight2", m_additional_weight);*/
	m_fAdditionalMaxWeight = READ_IF_EXISTS(pSettings, r_float, section, "additional_max_weight", 0.f);

	m_flags.set(FUsingCondition, READ_IF_EXISTS(pSettings, r_bool, section, "use_condition", TRUE));
}

void CBackPack::Hit(float hit_power, ALife::EHitType hit_type)
{
	if (!m_flags.is(FUsingCondition)) return;
	hit_power *= m_HitTypeK[hit_type];
	ChangeCondition(-hit_power);
}

/*float CBackPack::GetAdditionalMaxWalkWeight()
{
	return m_additional_weight * GetCondition();
}*/

float CBackPack::GetAdditionalMaxWeight()
{
	return /*m_additional_weight2*/m_fAdditionalMaxWeight * GetCondition();
}

void CBackPack::OnMoveToSlot(EItemPlace previous_place)
{
	if (m_pCurrentInventory)
	{
		CActor* pActor = smart_cast<CActor*> (m_pCurrentInventory->GetOwner());
		if (pActor && pActor->bAllItemsLoaded)
		{
			m_pCurrentInventory->DropRuckOut();
		}
	}
}

void CBackPack::OnMoveToRuck(EItemPlace previous_place)
{
	if (m_pCurrentInventory && previous_place == eItemPlaceSlot)
	{
		auto* pActor = smart_cast<CActor*> (m_pCurrentInventory->GetOwner());
		if (pActor && pActor->bAllItemsLoaded)
		{
			m_pCurrentInventory->DropRuckOut();
		}
	}
}

void CBackPack::OnMoveOut(EItemPlace previous_place)
{
	OnMoveToRuck(previous_place);
}