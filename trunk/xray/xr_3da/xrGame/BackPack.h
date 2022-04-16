#pragma once
#include "inventory_item_object.h"
class CBackPack :
	public CInventoryItemObject
{
private:
	typedef	CInventoryItemObject inherited;
public:
	CBackPack();
	~CBackPack();

	virtual void				Load(LPCSTR section);

	virtual void				Hit(SHit* pHDS);

protected:
	float						m_fAdditionalMaxWeight;
	float						m_fAdditionalMaxVolume;
	float                       m_fAdditionalWalkAccel;
	float                       m_fAdditionalJumpSpeed;

public:
	float						GetAdditionalMaxWeight	();
	float						GetAdditionalMaxVolume	();
	float						GetAdditionalWalkAccel	();
	float						GetAdditionalJumpSpeed	();
	float						GetHitTypeProtection	(ALife::EHitType hit_type);
	//
	float						m_fHealthRestoreSpeed;
#ifndef OBJECTS_RADIOACTIVE
//	float 						m_fRadiationRestoreSpeed;
#endif
	float 						m_fSatietyRestoreSpeed;
	float						m_fPowerRestoreSpeed;
	float						m_fBleedingRestoreSpeed;
	float						m_fPsyHealthRestoreSpeed;
	float						m_fAlcoholRestoreSpeed;

	HitImmunity::HitTypeSVec	m_HitTypeProtection;

	void						HitItemsInBackPack			(SHit* pHDS, bool hit_random_item);

	virtual void				OnMoveToSlot				(EItemPlace previous_place);
	virtual void				OnMoveToRuck				(EItemPlace previous_place);
	virtual void				OnMoveOut					(EItemPlace previous_place);
};

