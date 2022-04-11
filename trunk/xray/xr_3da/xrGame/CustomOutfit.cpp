#include "stdafx.h"

#include "customoutfit.h"
#include "PhysicsShell.h"
#include "inventory_space.h"
#include "Inventory.h"
#include "Actor.h"
#include "game_cl_base.h"
#include "Level.h"
#include "BoneProtections.h"
#include "UIStaticItem.h"
#include "../../build_config_defines.h"


CCustomOutfit::CCustomOutfit()
{
	SetSlot (OUTFIT_SLOT);

	m_flags.set(FUsingCondition, TRUE);

	m_HitTypeProtection.resize(ALife::eHitTypeMax);
	for(int i=0; i<ALife::eHitTypeMax; i++)
		m_HitTypeProtection[i] = 0.0f;//1.0f;

	m_boneProtection = xr_new<SBoneProtections>();
	m_bAlwaysProcessing = TRUE;

	m_UIOutfitMask		= NULL;
}

CCustomOutfit::~CCustomOutfit() 
{
	xr_delete(m_boneProtection);
	xr_delete(m_UIOutfitMask);
}

void CCustomOutfit::net_Export(NET_Packet& P)
{
	inherited::net_Export	(P);
	P.w_float_q8			(m_fCondition,0.0f,1.0f);
}

void CCustomOutfit::net_Import(NET_Packet& P)
{
	inherited::net_Import	(P);
	P.r_float_q8			(m_fCondition,0.0f,1.0f);
}

void CCustomOutfit::Load(LPCSTR section) 
{
	inherited::Load(section);

	m_fHealthRestoreSpeed		= READ_IF_EXISTS(pSettings, r_float, section, "health_restore_speed",		0.f);
#ifndef OBJECTS_RADIOACTIVE
	m_fRadiationRestoreSpeed	= READ_IF_EXISTS(pSettings, r_float, section, "radiation_restore_speed",	0.f);
#endif
	m_fSatietyRestoreSpeed		= READ_IF_EXISTS(pSettings, r_float, section, "satiety_restore_speed",		0.f);
	m_fPowerRestoreSpeed		= READ_IF_EXISTS(pSettings, r_float, section, "power_restore_speed",		0.f);
	m_fBleedingRestoreSpeed		= READ_IF_EXISTS(pSettings, r_float, section, "bleeding_restore_speed",		0.f);
	m_fPsyHealthRestoreSpeed	= READ_IF_EXISTS(pSettings, r_float, section, "psy_health_restore_speed",	0.f);
	m_fAlcoholRestoreSpeed		= READ_IF_EXISTS(pSettings, r_float, section, "alcohol_restore_speed",		0.f);

	m_HitTypeProtection[ALife::eHitTypeBurn]			= READ_IF_EXISTS(pSettings, r_float, section, "burn_protection",			0.0f);
	m_HitTypeProtection[ALife::eHitTypeStrike]			= READ_IF_EXISTS(pSettings, r_float, section, "strike_protection",			0.0f);
	m_HitTypeProtection[ALife::eHitTypeShock]			= READ_IF_EXISTS(pSettings, r_float, section, "shock_protection",			0.0f);
	m_HitTypeProtection[ALife::eHitTypeWound]			= READ_IF_EXISTS(pSettings, r_float, section, "wound_protection",			0.0f);
	m_HitTypeProtection[ALife::eHitTypeRadiation]		= READ_IF_EXISTS(pSettings, r_float, section, "radiation_protection",		0.0f);
	m_HitTypeProtection[ALife::eHitTypeTelepatic]		= READ_IF_EXISTS(pSettings, r_float, section, "telepatic_protection",		0.0f);
	m_HitTypeProtection[ALife::eHitTypeChemicalBurn]	= READ_IF_EXISTS(pSettings, r_float, section, "chemical_burn_protection",	0.0f);
	m_HitTypeProtection[ALife::eHitTypeExplosion]		= READ_IF_EXISTS(pSettings, r_float, section, "explosion_protection",		0.0f);
	m_HitTypeProtection[ALife::eHitTypeFireWound]		= READ_IF_EXISTS(pSettings, r_float, section, "fire_wound_protection",		0.0f);
	m_HitTypeProtection[ALife::eHitTypeWound_2]			= READ_IF_EXISTS(pSettings, r_float, section, "wound_2_protection",			0.0f);
	m_HitTypeProtection[ALife::eHitTypePhysicStrike]	= READ_IF_EXISTS(pSettings, r_float, section, "physic_strike_protection",	0.0f);

	if (pSettings->line_exist(section, "actor_visual"))
		m_ActorVisual = pSettings->r_string(section, "actor_visual");
	else
		m_ActorVisual = NULL;

	m_ef_equipment_type		= pSettings->r_u32(section,"ef_equipment_type");
	if (pSettings->line_exist(section, "power_loss"))
		m_fPowerLoss = pSettings->r_float(section, "power_loss");
	else
		m_fPowerLoss = 1.0f;	

	m_fAdditionalMaxWeight = READ_IF_EXISTS(pSettings, r_float, section, "additional_max_weight", 0.f);
	m_fAdditionalMaxVolume = READ_IF_EXISTS(pSettings, r_float, section, "additional_max_volume", 0.f);
	//
	m_fAdditionalWalkAccel = READ_IF_EXISTS(pSettings, r_float, section, "additional_walk_accel", 0.f);
	m_fAdditionalJumpSpeed = READ_IF_EXISTS(pSettings, r_float, section, "additional_jump_speed", 0.f);

/*	if (pSettings->line_exist(section, "nightvision_sect"))
		m_NightVisionSect = pSettings->r_string(section, "nightvision_sect");
	else
		m_NightVisionSect = NULL;*/

	m_OutfitMaskTexture = READ_IF_EXISTS(pSettings, r_string, section, "mask_texture", NULL);

	m_full_icon_name								= pSettings->r_string(section,"full_icon_name");
}

void CCustomOutfit::Hit(float hit_power, ALife::EHitType hit_type)
{
	hit_power *= m_HitTypeK[hit_type];
	ChangeCondition(-hit_power);
}

float CCustomOutfit::GetDefHitTypeProtection(ALife::EHitType hit_type)
{
	return /*1.0f -*/ m_HitTypeProtection[hit_type]*GetCondition();
}

float CCustomOutfit::GetHitTypeProtection(ALife::EHitType hit_type, s16 element)
{
	float fBase = m_HitTypeProtection[hit_type]*GetCondition();
	float bone = m_boneProtection->getBoneProtection(element);
	return 1.0f - fBase*bone;
}

float	CCustomOutfit::HitThruArmour(float hit_power, s16 element, float AP)
{
	float BoneArmour = m_boneProtection->getBoneArmour(element)*GetCondition()*(1-AP);	
	float NewHitPower = hit_power - BoneArmour;
	if (NewHitPower < hit_power*m_boneProtection->m_fHitFrac) return hit_power*m_boneProtection->m_fHitFrac;
	return NewHitPower;
};

BOOL	CCustomOutfit::BonePassBullet					(int boneID)
{
	return m_boneProtection->getBonePassBullet(s16(boneID));
};

#include "torch.h"
void	CCustomOutfit::OnMoveToSlot(EItemPlace previous_place)
{
	if (m_pCurrentInventory)
	{
		CActor* pActor = smart_cast<CActor*> (m_pCurrentInventory->GetOwner());
		if (pActor)
		{
			if (m_ActorVisual.size())
			{
				shared_str NewVisual = NULL;
				char* TeamSection = Game().getTeamSection(pActor->g_Team());
				if (TeamSection)
				{
					if (pSettings->line_exist(TeamSection, *cNameSect()))
					{
						NewVisual = pSettings->r_string(TeamSection, *cNameSect());
						string256 SkinName;
						strcpy_s(SkinName, pSettings->r_string("mp_skins_path", "skin_path"));
						strcat_s(SkinName, *NewVisual);
						strcat_s(SkinName, ".ogf");
						NewVisual._set(SkinName);
					}
				}
				
				if (!NewVisual.size())
					NewVisual = m_ActorVisual;

				pActor->ChangeVisual(NewVisual);
			}

			if (m_UIOutfitMask)
				xr_delete(m_UIOutfitMask);
			if (!!m_OutfitMaskTexture)
			{
				m_UIOutfitMask = xr_new<CUIStaticItem>();
				m_UIOutfitMask->Init(m_OutfitMaskTexture.c_str(), "hud\\scopes", 0, 0, alNone);	// KD: special shader that account screen resolution
			}

			if(pSettings->line_exist(cNameSect(),"bones_koeff_protection")){
				m_boneProtection->reload( pSettings->r_string(cNameSect(),"bones_koeff_protection"), smart_cast<CKinematics*>(pActor->Visual()) );

			};
		}
	}
};

void	CCustomOutfit::OnMoveToRuck(EItemPlace previous_place)
{
	if (m_pCurrentInventory && previous_place == eItemPlaceSlot)
	{
		CActor* pActor = smart_cast<CActor*> (m_pCurrentInventory->GetOwner());
		if (pActor/* && pActor->bAllItemsLoaded*/)
		{
/*			CTorch* pTorch = smart_cast<CTorch*>(pActor->inventory().ItemFromSlot(TORCH_SLOT));
			if(pTorch)
			{
				pTorch->SwitchNightVision(false);
			}*/
			if (m_UIOutfitMask)
				xr_delete(m_UIOutfitMask);

			if (m_ActorVisual.size())
			{
				shared_str DefVisual = pActor->GetDefaultVisualOutfit();
				if (DefVisual.size())
				{
					pActor->ChangeVisual(DefVisual);
				};
			}
		}
	}
};

void	CCustomOutfit::OnMoveOut(EItemPlace previous_place)
{
	OnMoveToRuck(previous_place);
};

u32	CCustomOutfit::ef_equipment_type	() const
{
	return		(m_ef_equipment_type);
}

float CCustomOutfit::GetPowerLoss() 
{
	if (m_fPowerLoss<1 && GetCondition() <= 0)
	{
		return 1.0f;			
	};
	return m_fPowerLoss;
};

float CCustomOutfit::GetAdditionalMaxWeight()
{
	return m_fAdditionalMaxWeight * GetCondition();
}

float CCustomOutfit::GetAdditionalMaxVolume()
{
	return m_fAdditionalMaxVolume * GetCondition();
}

float CCustomOutfit::GetAdditionalWalkAccel()
{
	return m_fAdditionalWalkAccel * GetCondition();
}

float CCustomOutfit::GetAdditionalJumpSpeed()
{
	return m_fAdditionalJumpSpeed * GetCondition();
}

void CCustomOutfit::OnDrawUI()
{
	if (!!m_OutfitMaskTexture)
	{
		m_UIOutfitMask->SetPos(0, 0);
		m_UIOutfitMask->SetRect(0, 0, UI_BASE_WIDTH, UI_BASE_HEIGHT);
		m_UIOutfitMask->Render();
	}
}