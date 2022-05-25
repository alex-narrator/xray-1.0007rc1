////////////////////////////////////////////////////////////////////////////
//	Module 		: purchase_list.cpp
//	Created 	: 12.01.2006
//  Modified 	: 12.01.2006
//	Author		: Dmitriy Iassenev
//	Description : purchase list class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "purchase_list.h"
#include "inventoryowner.h"
#include "gameobject.h"
#include "ai_object_location.h"
#include "level.h"

void CPurchaseList::process	(CInifile &ini_file, LPCSTR section, CInventoryOwner &owner)
{
	owner.sell_useless_items();

	m_deficits.clear		();

	const CGameObject		&game_object = smart_cast<const CGameObject &>(owner);
	CInifile::Sect			&S = ini_file.r_section(section);
	CInifile::SectCIt		I = S.Data.begin();
	CInifile::SectCIt		E = S.Data.end();

	auto trade_ini = &ini_file;

	min_deficit = READ_IF_EXISTS(trade_ini, r_float, "trader", "min_deficit_factor", READ_IF_EXISTS(pSettings, r_float, "trade", "min_deficit_factor", 1.f));
	max_deficit = READ_IF_EXISTS(trade_ini, r_float, "trader", "max_deficit_factor", READ_IF_EXISTS(pSettings, r_float, "trade", "max_deficit_factor", 1.f));

	for ( ; I != E; ++I) {
		VERIFY3				((*I).second.size(),"PurchaseList : cannot handle lines in section without values",section);

		string256			temp0, temp1;
		THROW3				(_GetItemCount(*(*I).second) == 2,"Invalid parameters in section",section);
		process				(
			game_object,
			(*I).first,
			atoi(_GetItem(*(*I).second,0,temp0)),
			(float)atof(_GetItem(*(*I).second,1,temp1))
		);
	}
}

void CPurchaseList::process	(const CGameObject &owner, const shared_str &name, const u32 &count, const float &probability)
{
	VERIFY3					(count,"Invalid count for section in the purchase list",*name);
	VERIFY3					(!fis_zero(probability,EPS_S),"Invalid probability for section in the purchase list",*name);

	const Fvector			&position = owner.Position();
	const u32				&level_vertex_id = owner.ai_location().level_vertex_id();
	const ALife::_OBJECT_ID	&id = owner.ID();
	CRandom					random((u32)(CPU::QPC() & u32(-1)));
	for (u32 i=0, j=0; i<count; ++i) {
		if (random.randF() > probability)
			continue;

		++j;
		Level().spawn_item		(*name,position,level_vertex_id,id,false);
	}

	DEFICITS::const_iterator	I = m_deficits.find(name);
	VERIFY3						(I == m_deficits.end(),"Duplicate section in the purchase list",*name);
	float deficit = (float)count*probability / (float)j;
	clamp(deficit,
		min_deficit,
		max_deficit
		);
	m_deficits.insert(std::make_pair(name, deficit));

	Msg("~~ deficit [%.4f](min[%.4f]|max[%.4f]) for item [%s] (count [%d]|prob [%.4f]|item spawned [%d])", deficit, min_deficit, max_deficit, name.c_str(), count, probability, j);
}
