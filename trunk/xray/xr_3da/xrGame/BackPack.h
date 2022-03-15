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

	virtual void				Hit(float P, ALife::EHitType hit_type);

protected:
	float						m_additional_weight;
	float						m_additional_weight2;

public:
	float						GetAdditionalMaxWalkWeight	();
	float						GetAdditionalMaxWeight		();
};

