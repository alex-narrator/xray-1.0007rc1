#include "stdafx.h"
#include "BackPack.h"


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

	m_additional_weight = READ_IF_EXISTS(pSettings, r_float, section, "additional_inventory_weight", m_additional_weight2); //добавка к весу обездвиживания равна добавке к весу до перегруза, если не указан явно
	m_additional_weight2 = READ_IF_EXISTS(pSettings, r_float, section, "additional_inventory_weight2", 0.f);

	m_flags.set(FUsingCondition, READ_IF_EXISTS(pSettings, r_bool, section, "use_condition", TRUE));
}

void CBackPack::Hit(float hit_power, ALife::EHitType hit_type)
{
	if (!m_flags.is(FUsingCondition)) return;
	hit_power *= m_HitTypeK[hit_type];
	ChangeCondition(-hit_power);
}

float CBackPack::GetAdditionalMaxWalkWeight()
{
	return m_additional_weight * GetCondition();
}

float CBackPack::GetAdditionalMaxWeight()
{
	return m_additional_weight2 * GetCondition();
}