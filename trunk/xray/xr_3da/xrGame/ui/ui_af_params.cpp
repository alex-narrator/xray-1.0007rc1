#include "stdafx.h"
#include "../pch_script.h"
#include "../smart_cast.h"
#include "ui_af_params.h"
#include "UIStatic.h"
#include "../object_broker.h"
#include "../Artifact.h"
#include "../CustomOutfit.h"
#include "../BackPack.h"
#include "../Actor.h"
#include "../ActorCondition.h"
#include "../CustomDetector.h"
#include "UIXmlInit.h"

CUIArtefactParams::CUIArtefactParams()
{
	Memory.mem_fill			(m_info_items, 0, sizeof(m_info_items));
}

CUIArtefactParams::~CUIArtefactParams()
{
	for(u32 i=_item_start; i<_max_item_index; ++i)
	{
		CUIStatic* _s			= m_info_items[i];
		xr_delete				(_s);
	}
}

LPCSTR af_item_sect_names[] = {
	"health_restore_speed",
	"radiation_restore_speed",
	"satiety_restore_speed",
	"power_restore_speed",
	"bleeding_restore_speed",
	"psy_health_restore_speed",
	"alcohol_restore_speed",
//
	"additional_walk_accel",
	"additional_jump_speed",
//
	"additional_max_weight",
	"additional_max_volume",
//
	"burn_immunity",
	"shock_immunity",
	"strike_immunity",
	"wound_immunity",		
	"radiation_immunity",
	"telepatic_immunity",
	"chemical_burn_immunity",
	"explosion_immunity",
	"fire_wound_immunity",
};

LPCSTR af_item_param_names[] = {
	"ui_inv_health",
	"ui_inv_radiation",
	"ui_inv_satiety",
	"ui_inv_power",
	"ui_inv_bleeding",
	"ui_inv_psy_health",
	"ui_inv_alcohol",
//
	"ui_inv_walk_accel",
	"ui_inv_jump_speed",
//
	"ui_inv_weight",
	"ui_inv_volume",
//
	"ui_inv_outfit_burn_protection",			// "(burn_imm)",
	"ui_inv_outfit_shock_protection",			// "(shock_imm)",
	"ui_inv_outfit_strike_protection",			// "(strike_imm)",
	"ui_inv_outfit_wound_protection",			// "(wound_imm)",
	"ui_inv_outfit_radiation_protection",		// "(radiation_imm)",
	"ui_inv_outfit_telepatic_protection",		// "(telepatic_imm)",
	"ui_inv_outfit_chemical_burn_protection",	// "(chemical_burn_imm)",
	"ui_inv_outfit_explosion_protection",		// "(explosion_imm)",
	"ui_inv_outfit_fire_wound_protection",		// "(fire_wound_imm)",
};

LPCSTR af_actor_param_names[]={
	"satiety_health_v",
	"radiation_v",
	"satiety_v",
	"satiety_power_v",
	"wound_incarnation_v",
	"psy_health_v",
	"alcohol_v",
//
	"walk_accel",
	"jump_speed",
	"max_weight",
	"max_volume",
//
};

#ifdef AF_SHOW_DYNAMIC_PARAMS
float CArtefact::* af_prop_offsets[] = {
	&CArtefact::m_fHealthRestoreSpeed,
	&CArtefact::m_fRadiationRestoreSpeed,
	&CArtefact::m_fSatietyRestoreSpeed,
	&CArtefact::m_fPowerRestoreSpeed,
	&CArtefact::m_fBleedingRestoreSpeed
};
#endif



void CUIArtefactParams::InitFromXml(CUIXml& xml_doc)
{
	LPCSTR _base				= "af_params";
	if (!xml_doc.NavigateToNode(_base, 0))	return;

	string256					_buff;
	CUIXmlInit::InitWindow		(xml_doc, _base, 0, this);

	for(u32 i=_item_start; i<_max_item_index; ++i)
	{
		m_info_items[i]			= xr_new<CUIStatic>();
		CUIStatic* _s			= m_info_items[i];
		_s->SetAutoDelete		(false);
		strconcat				(sizeof(_buff),_buff, _base, ":static_", af_item_sect_names[i]);
		CUIXmlInit::InitStatic	(xml_doc, _buff,	0, _s);
	}
}

bool CUIArtefactParams::Check(CGameObject *obj/*const shared_str& af_section*/)
{
	//return !!pSettings->line_exist(af_section, "af_actor_properties");
	return (smart_cast<CArtefact*>(obj) || smart_cast<CCustomOutfit*>(obj) || smart_cast<CBackPack*>(obj));
}
#include "../string_table.h"
void CUIArtefactParams::SetInfo(CGameObject *obj)
{	
	auto artefact	= smart_cast<CArtefact*>		(obj);
	auto outfit		= smart_cast<CCustomOutfit*>	(obj);
	auto backpack	= smart_cast<CBackPack*>		(obj);

//	R_ASSERT2(art, "object is not CArtefact");
	const shared_str& item_section = obj->cNameSect();
	CActor *pActor = Actor();
	if (!pActor) return;
	//
	auto pDetector = pActor->GetDetector();

	Show(!!pDetector);

	string128					_buff;
	float						_h = 0.0f;
	DetachAll					();

	for(u32 i=_item_start; i<_max_item_index; ++i)
	{
		CUIStatic* _s			= m_info_items[i];

		float					_val = 0.f;

		if (artefact)
		{
			if (pDetector && !pDetector->IsGeigerCounter() && (i == _item_radiation_restore_speed || i == _item_radiation_immunity)) continue;
			if (pDetector && !pDetector->IsAnomDetector() && (i != _item_radiation_restore_speed && i != _item_radiation_immunity)) continue;
		}
//
		if(i<_max_item_index1)
		{
			if (i == _item_additional_walk_accel)
			{
				if (artefact)		_val = artefact->GetAdditionalWalkAccel();
				else if (outfit)	_val = outfit->GetAdditionalWalkAccel();
				else if (backpack)	_val = backpack->GetAdditionalWalkAccel();
			}
			else if (i == _item_additional_jump_speed)
			{
				if (artefact)		_val = artefact->GetAdditionalJumpSpeed();
				else if (outfit)	_val = outfit->GetAdditionalJumpSpeed();
				else if (backpack)	_val = backpack->GetAdditionalJumpSpeed();
			}
			else if (i == _item_additional_weight)
			{
				if (artefact)		_val = artefact->GetAdditionalMaxWeight();
				else if (outfit)	_val = outfit->GetAdditionalMaxWeight();
				else if (backpack)	_val = backpack->GetAdditionalMaxWeight();
			}
			else if (i == _item_additional_volume)
			{
				if (artefact)		_val = artefact->GetAdditionalMaxVolume();
				else if (outfit)	_val = outfit->GetAdditionalMaxVolume();
				else if (backpack)	_val = backpack->GetAdditionalMaxVolume();
			}
			else
			//
			{// *_restore_speed
#ifdef AF_SHOW_DYNAMIC_PARAMS
				float _actor_val = pActor->conditions().GetParamByName(af_actor_param_names[i]);
				float CArtefact::* pRestoreSpeed = af_prop_offsets[i];
				_val = (art->*pRestoreSpeed); // alpet: используется указатель на данные класса
#else
				_val = READ_IF_EXISTS(pSettings, r_float, item_section, af_item_sect_names[i], 0.f);
#endif
				if (artefact) _val *= artefact->GetRandomKoef();
				_val *= obj->cast_inventory_item()->GetCondition();
			}
		}
		else
		{
#ifdef AF_SHOW_DYNAMIC_PARAMS			
			u32 idx = i - _max_item_index1;  // absorbation index			 
			_val = art->m_ArtefactHitImmunities.immunities()[idx]; // real absorbation values			
#else
			u32 idx = i - _max_item_index1;						// absorbation index		
			//_val = artefact ? artefact->GetHitImmunities(ALife::EHitType(idx)) : outfit->GetDefHitTypeProtection(ALife::EHitType(idx)); // real absorbation values
			if (artefact)		_val = artefact->GetHitImmunities(ALife::EHitType(idx));
			else if (outfit)	_val = outfit->GetDefHitTypeProtection(ALife::EHitType(idx));
			else if (backpack)	_val = backpack->GetHitImmunities(ALife::EHitType(idx));
#endif
		}

		if (fis_zero(_val))				continue;
		if (i != _item_additional_weight && i != _item_additional_volume)
			_val *= 100.0f;

		LPCSTR _sn = "%";

		if (i == _item_radiation_restore_speed)
		{
			_val /= 100.0f;
			_sn = *CStringTable().translate("st_rad");
		}
		//
		else if (i == _item_additional_weight || i == _item_additional_volume)
		{
			_sn = i == _item_additional_weight ? *CStringTable().translate("st_kg") : *CStringTable().translate("st_l");
		}

		LPCSTR _color = (_val>0)?"%c[green]":"%c[red]";
		
		if (i == _item_bleeding_restore_speed || i == _item_alcohol_restore_speed)
			_val		*=	-1.0f;

		if (i == _item_bleeding_restore_speed || i == _item_radiation_restore_speed || i == _item_alcohol_restore_speed)
			_color = (_val>0)?"%c[red]":"%c[green]";



		sprintf_s				(_buff, "%s %s %+.1f %s", 
									CStringTable().translate(af_item_param_names[i]).c_str(), 
									_color, 
									_val, 
									_sn);
		_s->SetText				(_buff);
		_s->SetWndPos			(_s->GetWndPos().x, _h);
		_h						+= _s->GetWndSize().y;
		AttachChild				(_s);
	}
	SetHeight					(pDetector ? _h : 0.f);
}
