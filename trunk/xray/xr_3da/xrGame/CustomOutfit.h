#pragma once

#include "inventory_item_object.h"

struct SBoneProtections;

class CCustomOutfit: public CInventoryItemObject {
	friend class COutfitScript;
private:
    typedef	CInventoryItemObject inherited;
public:
									CCustomOutfit		(void);
	virtual							~CCustomOutfit		(void);

	virtual void					Load				(LPCSTR section);
	
	//уменьшенная версия хита, для вызова, когда костюм надет на персонажа
	virtual void					Hit					(float P, ALife::EHitType hit_type);

	//коэффициенты на которые домножается хит
	//при соответствующем типе воздействия
	//если на персонаже надет костюм
	float							GetHitTypeProtection(ALife::EHitType hit_type, s16 element);
	float							GetDefHitTypeProtection(ALife::EHitType hit_type);

	float							HitThruArmour		(float hit_power, s16 element, float AP);
	//коэффициент на который домножается потеря силы
	//если на персонаже надет костюм
	float							GetPowerLoss		();


	virtual void					OnMoveToSlot		(EItemPlace previous_place);
	virtual void					OnMoveToRuck		(EItemPlace previous_place);
	virtual void					OnMoveOut			(EItemPlace previous_place);

protected:
	HitImmunity::HitTypeSVec		m_HitTypeProtection;
	float							m_fPowerLoss;

	shared_str						m_ActorVisual;
	shared_str						m_full_icon_name;
	SBoneProtections*				m_boneProtection;	
protected:
	u32								m_ef_equipment_type;
	/*	float							m_additional_weight;
	float							m_additional_weight2;*/
	float							m_fAdditionalMaxWeight;
	float							m_fAdditionalMaxVolume;
public:
/*	float							m_additional_weight;
	float							m_additional_weight2;
	float							GetAdditionalMaxWalkWeight();*/
	float							GetAdditionalMaxWeight();
	float							GetAdditionalMaxVolume();

//	shared_str						m_NightVisionSect;
	virtual u32						ef_equipment_type		() const;
	virtual	BOOL					BonePassBullet			(int boneID);
	const shared_str&				GetFullIconName			() const	{return m_full_icon_name;};

	virtual void			net_Export			(NET_Packet& P);
	virtual void			net_Import			(NET_Packet& P);
};
