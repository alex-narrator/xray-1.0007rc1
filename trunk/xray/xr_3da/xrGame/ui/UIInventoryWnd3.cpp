#include "stdafx.h"
#include "UIInventoryWnd.h"
#include "../actor.h"
#include "../silencer.h"
#include "../scope.h"
#include "../grenadelauncher.h"
#include "../Artifact.h"
#include "../eatable_item.h"
#include "../BottleItem.h"
#include "../WeaponMagazined.h"
#include "../WeaponAmmo.h"
#include "../inventory.h"
#include "../game_base.h"
#include "../game_cl_base.h"
#include "../xr_level_controller.h"
#include "UICellItem.h"
#include "UIListBoxItem.h"
#include "../CustomOutfit.h"
#include "../string_table.h"

void CUIInventoryWnd::EatItem(PIItem itm)
{
	SetCurrentItem							(NULL);
	if(!itm->Useful())						return;

	SendEvent_Item_Eat						(itm);

	PlaySnd									(eInvItemUse);
}

#if defined(INV_NEW_SLOTS_SYSTEM)
// вернет true если слот назначения быстрый, и не занят аналогичным предметом
bool is_quick_slot(u32 slot, PIItem item, CInventory *inv)
{
	if (slot >= SLOT_QUICK_ACCESS_0 && slot <= SLOT_QUICK_ACCESS_3)
	{
		const shared_str &sect = item->object().cNameSect();
		for (u32 s = SLOT_QUICK_ACCESS_0; s <= SLOT_QUICK_ACCESS_3; s ++)
		if  ( inv->m_slots[s].m_pIItem 
			  && inv->m_slots[s].m_pIItem->object().cNameSect() == sect )
				return false;
		return true;
	}
	return false;
}
#endif

#include "../Medkit.h"
#include "../Antirad.h"
#include "../weaponmagazinedwgrenade.h"
#include "ui_af_params.h"
void CUIInventoryWnd::ActivatePropertiesBox()
{
	// Флаг-признак для невлючения пункта контекстного меню: Dreess Outfit, если костюм уже надет
	bool bAlreadyDressed = false; 

		
	UIPropertiesBox.RemoveAll();

	CMedkit*					pMedkit				= smart_cast<CMedkit*>					(CurrentIItem());
	CAntirad*					pAntirad			= smart_cast<CAntirad*>					(CurrentIItem());
	CEatableItem*				pEatableItem		= smart_cast<CEatableItem*>				(CurrentIItem());
	CCustomOutfit*				pOutfit				= smart_cast<CCustomOutfit*>			(CurrentIItem());
	CWeapon*					pWeapon				= smart_cast<CWeapon*>					(CurrentIItem());
	CWeaponMagazined*			pWeaponMag          = smart_cast<CWeaponMagazined*>			(CurrentIItem());
	CWeaponMagazinedWGrenade*	pWeaponMagWGren		= smart_cast<CWeaponMagazinedWGrenade*>	(CurrentIItem());
	CScope*						pScope				= smart_cast<CScope*>					(CurrentIItem());
	CSilencer*					pSilencer			= smart_cast<CSilencer*>				(CurrentIItem());
	CGrenadeLauncher*			pGrenadeLauncher	= smart_cast<CGrenadeLauncher*>			(CurrentIItem());
	CBottleItem*				pBottleItem			= smart_cast<CBottleItem*>				(CurrentIItem());
	CArtefact*					pArtefact           = smart_cast<CArtefact*>				(CurrentIItem());
	CWeaponAmmo*				pAmmo				= smart_cast<CWeaponAmmo*>				(CurrentIItem());

	bool	b_show			= false;


#if defined(INV_NEW_SLOTS_SYSTEM)
	// Добавим в контекстное меню выбор слота. Real Wolf.
	auto slots = CurrentIItem()->GetSlots();
	//char temp[64];
	string1024 temp;
	for(u8 i = 0; i < (u8)slots.size(); ++i) 
	{
/*#ifndef SHOW_GRENADE_SLOT
		if (slots[i] != NO_ACTIVE_SLOT && slots[i] != GRENADE_SLOT)
#else*/
		if (slots[i] != NO_ACTIVE_SLOT)
//#endif
			if (!m_pInv->m_slots[slots[i]].m_pIItem || m_pInv->m_slots[slots[i]].m_pIItem != CurrentIItem() )
			{
#ifndef INV_MOVE_ITM_INTO_QUICK_SLOTS
				CEatableItem *eat = smart_cast<CEatableItem*>(CurrentIItem() );
				// Для еды разрешены только быстрые слоты.
				if (!eat || is_quick_slot(u32(slots[i]), CurrentIItem(), m_pInv) )
#endif
				{
					sprintf_s(temp, "st_move_to_slot%d", slots[i]);
					UIPropertiesBox.AddItem(temp,  NULL, INVENTORY_TO_SLOT0_ACTION + slots[i]);
					b_show = true;
				}

			}
	};
#else
	if(!pOutfit && CurrentIItem()->GetSlot()!=NO_ACTIVE_SLOT && !m_pInv->m_slots[CurrentIItem()->GetSlot()].m_bPersistent && m_pInv->CanPutInSlot(CurrentIItem()))
	{
		UIPropertiesBox.AddItem("st_move_to_slot",  NULL, INVENTORY_TO_SLOT_ACTION);
		b_show			= true;
	}
#endif

/*#if defined(GRENADE_FROM_BELT) && !defined(SHOW_GRENADE_SLOT)
	if (CurrentIItem() != m_pInv->m_slots[GRENADE_SLOT].m_pIItem)
#endif*/
	if(CurrentIItem()->Belt() && m_pInv->CanPutInBelt(CurrentIItem()))
	{
		UIPropertiesBox.AddItem("st_move_on_belt",  NULL, INVENTORY_TO_BELT_ACTION);
		b_show			= true;
	}
/*#if defined(GRENADE_FROM_BELT) && !defined(SHOW_GRENADE_SLOT)
	if (CurrentIItem()->GetSlot() == GRENADE_SLOT && CurrentIItem()->m_eItemPlace != eItemPlaceRuck)
	{
		UIPropertiesBox.AddItem("st_move_to_bag",  NULL, INVENTORY_TO_BAG_ACTION);
		bAlreadyDressed = true;
		b_show			= true;
	}
	else
#endif*/
	if(CurrentIItem()->Ruck() && m_pInv->CanPutInRuck(CurrentIItem()) && (CurrentIItem()->GetSlot()==u32(-1) || !m_pInv->m_slots[CurrentIItem()->GetSlot()].m_bPersistent) )
	{
		if(!pOutfit)
			UIPropertiesBox.AddItem("st_move_to_bag",  NULL, INVENTORY_TO_BAG_ACTION);
		else
			UIPropertiesBox.AddItem("st_undress_outfit",  NULL, INVENTORY_TO_BAG_ACTION);
		bAlreadyDressed = true;
		b_show			= true;
	}
#if !defined(INV_NEW_SLOTS_SYSTEM)
	if(pOutfit  && !bAlreadyDressed )
	{
		UIPropertiesBox.AddItem("st_dress_outfit",  NULL, INVENTORY_TO_SLOT_ACTION);
		b_show			= true;
	}
#endif
	
	if (psActorFlags.test(AF_ARTEFACT_DETECTOR_CHECK) && pArtefact && g_actor->GetDetector() && !UIItemInfo.UIArtefactParams->IsShown())
	{
		UIPropertiesBox.AddItem("st_detector_check", NULL, INVENTORY_DETECTOR_CHECK_ACTION);
		b_show = true;
	}

	if (pAmmo)
	{
		LPCSTR _ammo_sect;

		if (pAmmo->IsBoxReloadable())
		{
			//unload AmmoBox
			UIPropertiesBox.AddItem("st_unload", NULL, INVENTORY_UNLOAD_AMMO_BOX);
			b_show = true;
			//reload AmmoBox
			if (pAmmo->m_boxCurr < pAmmo->m_boxSize)
			{
				if (Actor()->inventory().GetAmmo(*pAmmo->m_ammoSect, true))
				{
					strconcat(sizeof(temp), temp, *CStringTable().translate("st_load_ammo_type"), " ",
						*CStringTable().translate(pSettings->r_string(pAmmo->m_ammoSect, "inv_name_short")));
					_ammo_sect = *pAmmo->m_ammoSect;
					UIPropertiesBox.AddItem(temp, (void*)_ammo_sect, INVENTORY_RELOAD_AMMO_BOX);
					b_show = true;
				}
			}
		}
		else if (pAmmo->IsBoxReloadableEmpty())
		{
			for (u8 i = 0; i < pAmmo->m_ammoTypes.size(); ++i)
			{
				if (Actor()->inventory().GetAmmo(*pAmmo->m_ammoTypes[i], true))
				{
					strconcat(sizeof(temp), temp, *CStringTable().translate("st_load_ammo_type"), " ",
						*CStringTable().translate(pSettings->r_string(pAmmo->m_ammoTypes[i], "inv_name_short")));
					_ammo_sect = *pAmmo->m_ammoTypes[i];
					UIPropertiesBox.AddItem(temp, (void*)_ammo_sect, INVENTORY_RELOAD_AMMO_BOX);
					b_show = true;
				}
			}
		}
	}

	//отсоединение аддонов от вещи
	if(pWeapon)
	{
		if (pWeapon->IsGrenadeLauncherAttached())
		{
		    const char *switch_gl_text = pWeaponMagWGren->m_bGrenadeMode ? "st_deactivate_gl" : "st_activate_gl";
		    if (m_pInv->InSlot(pWeapon))
			UIPropertiesBox.AddItem(switch_gl_text, NULL, INVENTORY_SWITCH_GRENADE_LAUNCHER_MODE);
		b_show = true;
		}
		if(pWeapon->GrenadeLauncherAttachable() && pWeapon->IsGrenadeLauncherAttached())
		{
			//UIPropertiesBox.AddItem("st_detach_gl",  NULL, INVENTORY_DETACH_GRENADE_LAUNCHER_ADDON);
			strconcat(sizeof(temp), temp, *CStringTable().translate("st_detach_addon"), " ", *CStringTable().translate(pSettings->r_string(*pWeapon->GetGrenadeLauncherName(), "inv_name")));
			UIPropertiesBox.AddItem(temp, NULL, INVENTORY_DETACH_GRENADE_LAUNCHER_ADDON);
		b_show			= true;
		}
		if(pWeapon->ScopeAttachable() && pWeapon->IsScopeAttached())
		{
			//UIPropertiesBox.AddItem("st_detach_scope",  NULL, INVENTORY_DETACH_SCOPE_ADDON);
			strconcat(sizeof(temp), temp, *CStringTable().translate("st_detach_addon"), " ", *CStringTable().translate(pSettings->r_string(*pWeapon->GetScopeName(), "inv_name")));
			UIPropertiesBox.AddItem(temp, NULL, INVENTORY_DETACH_SCOPE_ADDON);
		b_show			= true;
		}
		if(pWeapon->SilencerAttachable() && pWeapon->IsSilencerAttached())
		{
			//UIPropertiesBox.AddItem("st_detach_silencer",  NULL, INVENTORY_DETACH_SILENCER_ADDON);
			strconcat(sizeof(temp), temp, *CStringTable().translate("st_detach_addon"), " ", *CStringTable().translate(pSettings->r_string(*pWeapon->GetSilencerName(), "inv_name")));
			UIPropertiesBox.AddItem(temp, NULL, INVENTORY_DETACH_SILENCER_ADDON);
		b_show			= true;
		}
		if(smart_cast<CWeaponMagazined*>(pWeapon) && IsGameTypeSingle())
		{
			bool b = (0 != pWeapon->GetAmmoElapsed() || pWeaponMag && pWeaponMag->HasDetachableMagazine() && pWeaponMag->IsMagazineAttached());

			if (m_pInv->InSlot(pWeapon))
			{
				for (u8 i = 0; i < pWeaponMag->m_ammoTypes.size(); ++i)
				{
					if (Actor()->inventory().GetAmmo(pWeaponMag->m_ammoTypes[i].c_str(), true))
					{
						strconcat(sizeof(temp), temp, *CStringTable().translate("st_load_ammo_type"), " ", 
							*CStringTable().translate(pSettings->r_string(pWeaponMag->m_ammoTypes[i].c_str(), "inv_name_short")));
						UIPropertiesBox.AddItem(temp, (void*)i, INVENTORY_RELOAD_MAGAZINE);
						b_show = true;
					}
				}
			}

			if(!b)
			{
				CUICellItem * itm = CurrentItem();
				for(u32 i=0; i<itm->ChildsCount(); ++i)
				{
					pWeapon		= smart_cast<CWeaponMagazined*>((CWeapon*)itm->Child(i)->m_pData);
					if (pWeapon->GetAmmoElapsed())
					{
						b = true;
						break;
					}
				}
			}

			if(b){
				const char *unload_text = pWeaponMagWGren ? (pWeaponMagWGren->m_bGrenadeMode ? "st_unload_gl" : "st_unload") : "st_unload";
				UIPropertiesBox.AddItem(unload_text, NULL, INVENTORY_UNLOAD_MAGAZINE);
				b_show			= true;
			}
		}
	}
	
	//присоединение аддонов к активному слоту (2 или 3)
	if(pScope)
	{
	#ifndef INV_NEW_SLOTS_SYSTEM
		if(m_pInv->m_slots[PISTOL_SLOT].m_pIItem != NULL &&
		   m_pInv->m_slots[PISTOL_SLOT].m_pIItem->CanAttach(pScope))
		 {
			PIItem tgt = m_pInv->m_slots[PISTOL_SLOT].m_pIItem;
			UIPropertiesBox.AddItem("st_attach_scope_to_pistol",  (void*)tgt, INVENTORY_ATTACH_ADDON);
			b_show			= true;
		 }
		 if(m_pInv->m_slots[RIFLE_SLOT].m_pIItem != NULL &&
			m_pInv->m_slots[RIFLE_SLOT].m_pIItem->CanAttach(pScope))
		 {
			PIItem tgt = m_pInv->m_slots[RIFLE_SLOT].m_pIItem;
			UIPropertiesBox.AddItem("st_attach_scope_to_rifle",  (void*)tgt, INVENTORY_ATTACH_ADDON);
			b_show			= true;
		 }
	#else
		for(u8 i = 0; i < OUTFIT_SLOT; ++i) 
		{
			 if(m_pInv->m_slots[i].m_pIItem && m_pInv->m_slots[i].m_pIItem->CanAttach(pScope) )
			 {
				PIItem tgt = m_pInv->m_slots[i].m_pIItem;
				//sprintf_s(temp, "st_attach_scope_to_%d", i);
				//
				strconcat(sizeof(temp), temp, /*CurrentIItem()->Name(), " ",*/ *CStringTable().translate("st_attach_addon"), " ", tgt->Name());
				UIPropertiesBox.AddItem(temp, (void*)tgt, INVENTORY_ATTACH_ADDON);
				b_show			= true;
			 }
		};	
	#endif
	}
	else if(pSilencer)
	{
	#ifndef INV_NEW_SLOTS_SYSTEM
		 if(m_pInv->m_slots[PISTOL_SLOT].m_pIItem != NULL &&
		   m_pInv->m_slots[PISTOL_SLOT].m_pIItem->CanAttach(pSilencer))
		 {
			PIItem tgt = m_pInv->m_slots[PISTOL_SLOT].m_pIItem;
			UIPropertiesBox.AddItem("st_attach_silencer_to_pistol",  (void*)tgt, INVENTORY_ATTACH_ADDON);
			b_show			= true;
		 }
		 if(m_pInv->m_slots[RIFLE_SLOT].m_pIItem != NULL &&
			m_pInv->m_slots[RIFLE_SLOT].m_pIItem->CanAttach(pSilencer))
		 {
			PIItem tgt = m_pInv->m_slots[RIFLE_SLOT].m_pIItem;
			UIPropertiesBox.AddItem("st_attach_silencer_to_rifle",  (void*)tgt, INVENTORY_ATTACH_ADDON);
			b_show			= true;
		 }
	#else
		for(u8 i = 0; i < OUTFIT_SLOT; ++i) 
		{
			 if(m_pInv->m_slots[i].m_pIItem != NULL && m_pInv->m_slots[i].m_pIItem->CanAttach(pSilencer))
			 {
				PIItem tgt = m_pInv->m_slots[i].m_pIItem;
				//sprintf_s(temp, "st_attach_silencer_to_%d", i);
				//
				strconcat(sizeof(temp), temp, /*CurrentIItem()->Name(), " ",*/ *CStringTable().translate("st_attach_addon"), " ", tgt->Name());
				//
				UIPropertiesBox.AddItem(temp,  (void*)tgt, INVENTORY_ATTACH_ADDON);
				b_show			= true;
			 }
		}	
	#endif
	}
	else if(pGrenadeLauncher)
	{
	#ifndef INV_NEW_SLOTS_SYSTEM
		 if(m_pInv->m_slots[RIFLE_SLOT].m_pIItem != NULL &&
			m_pInv->m_slots[RIFLE_SLOT].m_pIItem->CanAttach(pGrenadeLauncher))
		 {
			PIItem tgt = m_pInv->m_slots[RIFLE_SLOT].m_pIItem;
			UIPropertiesBox.AddItem("st_attach_gl_to_rifle",  (void*)tgt, INVENTORY_ATTACH_ADDON);
			b_show			= true;
		 }
	#else
	for(u8 i = 0; i < OUTFIT_SLOT; ++i) 
	{
		 if(m_pInv->m_slots[i].m_pIItem && m_pInv->m_slots[i].m_pIItem->CanAttach(pGrenadeLauncher))
		 {
			PIItem tgt = m_pInv->m_slots[i].m_pIItem;
			//sprintf_s(temp, "st_attach_gl_to_%d", i);
			//
			strconcat(sizeof(temp), temp, /*CurrentIItem()->Name(), " ",*/ *CStringTable().translate("st_attach_addon"), " ", tgt->Name());
			//
			UIPropertiesBox.AddItem(temp,  (void*)tgt, INVENTORY_ATTACH_ADDON);
			b_show			= true;
		 }
	};		
	#endif
	}
	LPCSTR _action = NULL;

	if(pMedkit || pAntirad)
	{
		_action					= "st_use";
	}
	else if(pEatableItem)
	{
		if(pBottleItem)
			_action					= "st_drink";
		else
			_action					= "st_eat";
	}

	if(_action){
		UIPropertiesBox.AddItem(_action,  NULL, INVENTORY_EAT_ACTION);
		b_show			= true;
	}

	bool disallow_drop	= (pOutfit&&bAlreadyDressed);
	disallow_drop		|= !!CurrentIItem()->IsQuestItem();

	if(!disallow_drop)
	{

		UIPropertiesBox.AddItem("st_drop", NULL, INVENTORY_DROP_ACTION);
		b_show			= true;

		if(CurrentItem()->ChildsCount())
			UIPropertiesBox.AddItem("st_drop_all", (void*)33, INVENTORY_DROP_ACTION);
	}

	if(b_show)
	{
		UIPropertiesBox.AutoUpdateSize	();
		UIPropertiesBox.BringAllToTop	();

		Fvector2						cursor_pos;
		Frect							vis_rect;
		GetAbsoluteRect					(vis_rect);
		cursor_pos						= GetUICursor()->GetCursorPosition();
		cursor_pos.sub					(vis_rect.lt);
		UIPropertiesBox.Show			(vis_rect, cursor_pos);
		PlaySnd							(eInvProperties);
	}
}

void CUIInventoryWnd::ProcessPropertiesBoxClicked	()
{
	if(UIPropertiesBox.GetClickedItem())
	{
		auto num = UIPropertiesBox.GetClickedItem()->GetTAG();
#ifdef INV_NEW_SLOTS_SYSTEM
		if (num >= INVENTORY_TO_SLOT0_ACTION && num <= INVENTORY_TO_SLOT15_ACTION)
		{
			switch(num)
			{
			case INVENTORY_TO_SLOT0_ACTION:
				CurrentIItem()->SetSlot(KNIFE_SLOT);
				break;
			case INVENTORY_TO_SLOT1_ACTION:
				CurrentIItem()->SetSlot(PISTOL_SLOT);
				break;
			case INVENTORY_TO_SLOT2_ACTION:
				CurrentIItem()->SetSlot(RIFLE_SLOT);
				break;
			case INVENTORY_TO_SLOT3_ACTION:
				CurrentIItem()->SetSlot(GRENADE_SLOT);
				break;
			case INVENTORY_TO_SLOT4_ACTION:
				CurrentIItem()->SetSlot(APPARATUS_SLOT);
				break;
			case INVENTORY_TO_SLOT5_ACTION:
				CurrentIItem()->SetSlot(BOLT_SLOT);
				break;
			case INVENTORY_TO_SLOT6_ACTION:
				CurrentIItem()->SetSlot(OUTFIT_SLOT);
				break;
			case INVENTORY_TO_SLOT7_ACTION:
				CurrentIItem()->SetSlot(PDA_SLOT);
				break;
			case INVENTORY_TO_SLOT8_ACTION:
				CurrentIItem()->SetSlot(DETECTOR_SLOT);
				break;
			case INVENTORY_TO_SLOT9_ACTION:
				CurrentIItem()->SetSlot(TORCH_SLOT);
				break;
			case INVENTORY_TO_SLOT10_ACTION:
				CurrentIItem()->SetSlot(ARTEFACT_SLOT);
				break;
			case INVENTORY_TO_SLOT11_ACTION:
				CurrentIItem()->SetSlot(HELMET_SLOT);
				break;
			case INVENTORY_TO_SLOT12_ACTION:
				CurrentIItem()->SetSlot(SLOT_QUICK_ACCESS_0);
				break;
			case INVENTORY_TO_SLOT13_ACTION:
				CurrentIItem()->SetSlot(SLOT_QUICK_ACCESS_1);
				break;
			case INVENTORY_TO_SLOT14_ACTION:
				CurrentIItem()->SetSlot(SLOT_QUICK_ACCESS_2);
				break;
			case INVENTORY_TO_SLOT15_ACTION:
				CurrentIItem()->SetSlot(SLOT_QUICK_ACCESS_3);
				break;
			}
			ToSlot(CurrentItem(), true);
			return;
		}
#endif
		CWeapon*		pWeapon = smart_cast<CWeapon*>		(CurrentIItem());
		CWeaponAmmo*	pAmmo	= smart_cast<CWeaponAmmo*>	(CurrentIItem());
		switch(num)
		{
		case INVENTORY_TO_SLOT_ACTION:	
			ToSlot(CurrentItem(), true);
			break;
		case INVENTORY_TO_BELT_ACTION:	
			ToBelt(CurrentItem(),false);
			break;
		case INVENTORY_TO_BAG_ACTION:	
			ToBag(CurrentItem(),false);
			break;
		case INVENTORY_DROP_ACTION:
			{
				void* d = UIPropertiesBox.GetClickedItem()->GetData();
				bool b_all = (d==(void*)33);

				DropCurrentItem(b_all);
			}break;
		case INVENTORY_EAT_ACTION:
			EatItem(CurrentIItem());
			break;
		case INVENTORY_DETECTOR_CHECK_ACTION:
			UIItemInfo.UIArtefactParams->Show(true);
			UIItemInfo.TryAddArtefactInfo(CurrentIItem()->object().cNameSect());
			break;
		case INVENTORY_ATTACH_ADDON:
			AttachAddon((PIItem)(UIPropertiesBox.GetClickedItem()->GetData()));
			break;
		case INVENTORY_DETACH_SCOPE_ADDON:
			DetachAddon(*pWeapon->GetScopeName());
			break;
		case INVENTORY_DETACH_SILENCER_ADDON:
			DetachAddon(*pWeapon->GetSilencerName());
			break;
		case INVENTORY_DETACH_GRENADE_LAUNCHER_ADDON:
			DetachAddon(*pWeapon->GetGrenadeLauncherName());
			break;
		case INVENTORY_RELOAD_MAGAZINE:
			pWeapon->m_set_next_ammoType_on_reload = (u8)(UIPropertiesBox.GetClickedItem()->GetData());
			pWeapon->ReloadWeapon();
			SetCurrentItem(NULL);
			break;
		case INVENTORY_SWITCH_GRENADE_LAUNCHER_MODE:
			pWeapon->Action(kWPN_FUNC, CMD_START);
			SetCurrentItem(NULL);
			break;
		case INVENTORY_UNLOAD_MAGAZINE:
			{
				CUICellItem * itm = CurrentItem();
				(smart_cast<CWeaponMagazined*>((CWeapon*)itm->m_pData))->UnloadMagazine();
				(smart_cast<CWeaponMagazined*>((CWeapon*)itm->m_pData))->PullShutter();
				for(u32 i=0; i<itm->ChildsCount(); ++i)
				{
					CUICellItem * child_itm			= itm->Child(i);
					(smart_cast<CWeaponMagazined*>((CWeapon*)child_itm->m_pData))->UnloadMagazine();
					(smart_cast<CWeaponMagazined*>((CWeapon*)child_itm->m_pData))->PullShutter();
				}
				SetCurrentItem(NULL);
			}break;
		case INVENTORY_RELOAD_AMMO_BOX:
			{
			//Msg("load %s to %s", (LPCSTR)UIPropertiesBox.GetClickedItem()->GetData(), pAmmo->cNameSect().c_str());
			pAmmo->ReloadBox((LPCSTR)UIPropertiesBox.GetClickedItem()->GetData());
			SetCurrentItem(NULL);
			m_b_need_reinit = true;
			}break;
		case INVENTORY_UNLOAD_AMMO_BOX:
			{
			pAmmo->UnloadBox();
			SetCurrentItem(NULL);
			}break;
		}
	}
}

bool CUIInventoryWnd::TryUseItem(PIItem itm)
{
	CBottleItem*		pBottleItem			= smart_cast<CBottleItem*>		(itm);
	CMedkit*			pMedkit				= smart_cast<CMedkit*>			(itm);
	CAntirad*			pAntirad			= smart_cast<CAntirad*>			(itm);
	CEatableItem*		pEatableItem		= smart_cast<CEatableItem*>		(itm);
	//
	if (!itm->IsPlaceable(SLOT_QUICK_ACCESS_0, SLOT_QUICK_ACCESS_3)) //съедаем предмет только если его нельзя поместить в быстрые слоты
	//
	if(pMedkit || pAntirad || pEatableItem || pBottleItem)
	{
		EatItem(itm);
		return true;
	}
	return false;
}

bool CUIInventoryWnd::DropItem(PIItem itm, CUIDragDropListEx* lst)
{
	if(lst==m_pUIOutfitList)
	{
		return TryUseItem			(itm);
/*
		CCustomOutfit*		pOutfit		= smart_cast<CCustomOutfit*>	(CurrentIItem());
		if(pOutfit)
			ToSlot			(CurrentItem(), true);
		else
			EatItem				(CurrentIItem());

		return				true;
*/
	}
	CUICellItem*	_citem	= lst->ItemsCount() ? lst->GetItemIdx(0) : NULL;
	PIItem _iitem	= _citem ? (PIItem)_citem->m_pData : NULL;

	if(!_iitem)						return	false;
	if(!_iitem->CanAttach(itm))		return	false;
	AttachAddon						(_iitem);

	return							true;
}
