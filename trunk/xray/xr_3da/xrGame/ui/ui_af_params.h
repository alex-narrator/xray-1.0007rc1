#pragma once
#include "UIWindow.h"
#include "../GameObject.h"

class CUIXml;
class CUIStatic;

class CUIArtefactParams :public CUIWindow
{
public:
								CUIArtefactParams		();
	virtual						~CUIArtefactParams		();
	void 						InitFromXml				(CUIXml& xml_doc);
	bool 						Check					(CGameObject *obj/*const shared_str& af_section*/);
	void 						SetInfo					(CGameObject *obj);

protected:
	enum{
		_item_start						= 0,
		_item_health_restore_speed		= _item_start,
		_item_radiation_restore_speed,
		_item_satiety_restore_speed,
		_item_power_restore_speed,
		_item_bleeding_restore_speed,
		_item_psy_health_restore_speed,
		_item_alcohol_restore_speed,
//
		_item_additional_walk_accel,
		_item_additional_jump_speed,
//
		_item_additional_weight,
		_item_additional_volume,
//
		_max_item_index1,

		_item_burn_immunity				= _max_item_index1,
		_item_strike_immunity,
		_item_shock_immunity,
		_item_wound_immunity,		
		_item_radiation_immunity,
		_item_telepatic_immunity,
		_item_chemical_burn_immunity,
		_item_explosion_immunit,
		_item_fire_wound_immunity,

		_max_item_index,

	};
	CUIStatic*					m_info_items[_max_item_index];
};