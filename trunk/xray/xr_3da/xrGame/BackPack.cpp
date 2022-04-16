#include "stdafx.h"
#include "BackPack.h"

#include "Inventory.h"
#include "Actor.h"

CBackPack::CBackPack()
{
	SetSlot(BACKPACK_SLOT);
	m_flags.set(FUsingCondition, TRUE);

	m_HitTypeProtection.clear();
	m_HitTypeProtection.resize(ALife::eHitTypeMax);
/*	for (int i = 0; i<ALife::eHitTypeMax; i++)
		m_HitTypeProtection[i] = 0.0f;*/
}


CBackPack::~CBackPack()
{
}

void CBackPack::Load(LPCSTR section)
{
	inherited::Load(section);

	//*_restore_speed
	m_fHealthRestoreSpeed								= READ_IF_EXISTS(pSettings, r_float, section, "health_restore_speed",		0.f);
#ifndef OBJECTS_RADIOACTIVE
//	m_fRadiationRestoreSpeed							= READ_IF_EXISTS(pSettings, r_float, section, "radiation_restore_speed",	0.f);
#endif
	m_fSatietyRestoreSpeed								= READ_IF_EXISTS(pSettings, r_float, section, "satiety_restore_speed",		0.f);
	m_fPowerRestoreSpeed								= READ_IF_EXISTS(pSettings, r_float, section, "power_restore_speed",		0.f);
	m_fBleedingRestoreSpeed								= READ_IF_EXISTS(pSettings, r_float, section, "bleeding_restore_speed",		0.f);
	m_fPsyHealthRestoreSpeed							= READ_IF_EXISTS(pSettings, r_float, section, "psy_health_restore_speed",	0.f);
	m_fAlcoholRestoreSpeed								= READ_IF_EXISTS(pSettings, r_float, section, "alcohol_restore_speed",		0.f);
	//addition
	m_fAdditionalMaxWeight								= READ_IF_EXISTS(pSettings, r_float, section, "additional_max_weight",		0.f);
	m_fAdditionalMaxVolume								= READ_IF_EXISTS(pSettings, r_float, section, "additional_max_volume",		0.f);
	m_fAdditionalWalkAccel								= READ_IF_EXISTS(pSettings, r_float, section, "additional_walk_accel",		0.f);
	m_fAdditionalJumpSpeed								= READ_IF_EXISTS(pSettings, r_float, section, "additional_jump_speed",		0.f);
	//protection
	m_HitTypeProtection[ALife::eHitTypeBurn]			= READ_IF_EXISTS(pSettings, r_float, section, "burn_protection",			0.f);
	m_HitTypeProtection[ALife::eHitTypeStrike]			= READ_IF_EXISTS(pSettings, r_float, section, "strike_protection",			0.f);
	m_HitTypeProtection[ALife::eHitTypeShock]			= READ_IF_EXISTS(pSettings, r_float, section, "shock_protection",			0.f);
	m_HitTypeProtection[ALife::eHitTypeWound]			= READ_IF_EXISTS(pSettings, r_float, section, "wound_protection",			0.f);
	m_HitTypeProtection[ALife::eHitTypeRadiation]		= READ_IF_EXISTS(pSettings, r_float, section, "radiation_protection",		0.f);
	m_HitTypeProtection[ALife::eHitTypeTelepatic]		= READ_IF_EXISTS(pSettings, r_float, section, "telepatic_protection",		0.f);
	m_HitTypeProtection[ALife::eHitTypeChemicalBurn]	= READ_IF_EXISTS(pSettings, r_float, section, "chemical_burn_protection",	0.f);
	m_HitTypeProtection[ALife::eHitTypeExplosion]		= READ_IF_EXISTS(pSettings, r_float, section, "explosion_protection",		0.f);
	m_HitTypeProtection[ALife::eHitTypeFireWound]		= READ_IF_EXISTS(pSettings, r_float, section, "fire_wound_protection",		0.f);
	m_HitTypeProtection[ALife::eHitTypeWound_2]			= READ_IF_EXISTS(pSettings, r_float, section, "wound_2_protection",			0.f);
	m_HitTypeProtection[ALife::eHitTypePhysicStrike]	= READ_IF_EXISTS(pSettings, r_float, section, "physic_strike_protection",	0.f);
}

void CBackPack::Hit(SHit* pHDS)
{
	Msg("pHDS before hit: [%.2f]", pHDS->power);
	inherited::Hit(pHDS);
	Msg("pHDS after hit: [%.2f]", pHDS->power);
	bool hit_random_item = 
		(pHDS->type() == ALife::eHitTypeFireWound ||
		pHDS->type() == ALife::eHitTypeWound ||
		pHDS->type() == ALife::eHitTypeWound_2);
	HitItemsInBackPack(pHDS, hit_random_item);
}

float CBackPack::GetAdditionalMaxWeight()
{
	return m_fAdditionalMaxWeight * GetCondition();
}

float CBackPack::GetAdditionalMaxVolume()
{
	return m_fAdditionalMaxVolume * GetCondition();
}

float CBackPack::GetAdditionalWalkAccel()
{
	return m_fAdditionalWalkAccel * GetCondition();
}

float CBackPack::GetAdditionalJumpSpeed()
{
	return m_fAdditionalJumpSpeed * GetCondition();
}

float CBackPack::GetHitTypeProtection(ALife::EHitType hit_type)
{
	return m_HitTypeProtection[hit_type] * GetCondition();
}

void CBackPack::HitItemsInBackPack(SHit* pHDS, bool hit_random_item)
{
	if (m_pCurrentInventory)
	{
		CActor* pActor = smart_cast<CActor*> (m_pCurrentInventory->GetOwner());
		if (pActor && pActor->GetBackPack() == this && pActor->IsHitToBackPack(pHDS))
		{
			Msg("pHDS power: [%.2f]", pHDS->power);
			pHDS->power *= (1.0f - GetHitTypeProtection(pHDS->type()));
			Msg("new_hit power: [%.2f]", pHDS->power);

			auto ruck = m_pCurrentInventory->m_ruck;
			u32 random_item = NULL;

			for (TIItemContainer::iterator it = ruck.begin(); ruck.end() != it; ++it)
			{
				if (hit_random_item)
					random_item = ::Random.randI(0, ruck.size());

				auto pIItem = hit_random_item ? ruck[random_item] : *it;

				if (pIItem->m_flags.is(FUsingCondition) && !fis_zero(pIItem->GetCondition()))
				{
					pIItem->Hit(pHDS);
					Msg("Hit item [%s]", pIItem->Name());
					if (hit_random_item)
						break;
				}

			}
		}
	}
}

void CBackPack::OnMoveToSlot(EItemPlace previous_place)
{
	if (m_pCurrentInventory)
	{
		CActor* pActor = smart_cast<CActor*> (m_pCurrentInventory->GetOwner());
		if (pActor && pActor->bAllItemsLoaded)
		{
//			m_pCurrentInventory->DropRuckOut();
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
//			m_pCurrentInventory->DropRuckOut();
		}
	}
}

void CBackPack::OnMoveOut(EItemPlace previous_place)
{
	OnMoveToRuck(previous_place);
}