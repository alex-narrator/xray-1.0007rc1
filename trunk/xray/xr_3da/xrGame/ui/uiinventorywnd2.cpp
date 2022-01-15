#include "stdafx.h"
#include "UIInventoryWnd.h"
#include "UISleepWnd.h"
#include "../level.h"
#include "../actor.h"
#include "../ActorCondition.h"
#include "../hudmanager.h"
#include "../inventory.h"
#include "UIInventoryUtilities.h"


#include "UICellItem.h"
#include "UICellItemFactory.h"
#include "UIDragDropListEx.h"
#include "UI3tButton.h"

#include "../../../build_config_defines.h"

#include "../WeaponMagazined.h"

CUICellItem* CUIInventoryWnd::CurrentItem()
{
	return m_pCurrentCellItem;
}

PIItem CUIInventoryWnd::CurrentIItem()
{
	return	(m_pCurrentCellItem)?(PIItem)m_pCurrentCellItem->m_pData : NULL;
}

void CUIInventoryWnd::SetCurrentItem(CUICellItem* itm)
{
	if(m_pCurrentCellItem == itm) return;
	m_pCurrentCellItem				= itm;
	UIItemInfo.InitItem			(CurrentIItem());
}

void CUIInventoryWnd::SendMessage(CUIWindow *pWnd, s16 msg, void *pData)
{
	if(pWnd == &UIPropertiesBox &&	msg==PROPERTY_CLICKED)
	{
		ProcessPropertiesBoxClicked	();
	}
	else 
	if (UIExitButton == pWnd && BUTTON_CLICKED == msg)
	{
		GetHolder()->StartStopMenu			(this,true);
	}
	else
	if (UIRepackAmmoButton == pWnd && BUTTON_CLICKED == msg)
	{
		g_actor->inventory().RepackAmmo();
	}

	CUIWindow::SendMessage(pWnd, msg, pData);
}


void CUIInventoryWnd::InitInventory_delayed()
{
	m_b_need_reinit = true;
}

void CUIInventoryWnd::InitInventory() 
{
	CInventoryOwner *pInvOwner	= smart_cast<CInventoryOwner*>(Level().CurrentEntity());
	if(!pInvOwner)				return;

	m_pInv						= &pInvOwner->inventory();

	UIPropertiesBox.Hide		();
	ClearAllLists				();
	m_pMouseCapturer			= NULL;
	SetCurrentItem				(NULL);

	//Slots

	PIItem  _itm							= m_pInv->m_slots[PISTOL_SLOT].m_pIItem;
	if(_itm)
	{
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIPistolList->SetItem		(itm);
	}
	_itm								= m_pInv->m_slots[RIFLE_SLOT].m_pIItem;
	if(_itm)
	{
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIAutomaticList->SetItem		(itm);
	}
	
#ifdef INV_NEW_SLOTS_SYSTEM
	if (GameID() == GAME_SINGLE){
			_itm								= m_pInv->m_slots[KNIFE_SLOT].m_pIItem;
			if(_itm)
			{
				CUICellItem* itm				= create_cell_item(_itm);
				m_pUIKnifeList->SetItem		(itm);
			}
			_itm								= m_pInv->m_slots[APPARATUS_SLOT].m_pIItem;
			if(_itm)
			{
				CUICellItem* itm				= create_cell_item(_itm);
				m_pUIBinocularList->SetItem		(itm);
			}
			_itm								= m_pInv->m_slots[DETECTOR_SLOT].m_pIItem;
			if(_itm)
			{
				CUICellItem* itm				= create_cell_item(_itm);
				m_pUIDetectorList->SetItem		(itm);
			}
			_itm								= m_pInv->m_slots[TORCH_SLOT].m_pIItem;
			if(_itm)
			{
				CUICellItem* itm				= create_cell_item(_itm);
				m_pUITorchList->SetItem		(itm);
			}
			_itm								= m_pInv->m_slots[PDA_SLOT].m_pIItem;
			if(_itm)
			{
				CUICellItem* itm				= create_cell_item(_itm);
				m_pUIPDAList->SetItem		(itm);
			}
			_itm								= m_pInv->m_slots[HELMET_SLOT].m_pIItem;
			if(_itm)
			{
				CUICellItem* itm				= create_cell_item(_itm);
				m_pUIHelmetList->SetItem		(itm);
			}
			_itm								= m_pInv->m_slots[SLOT_QUICK_ACCESS_0].m_pIItem;
			if(_itm)
			{
				CUICellItem* itm				= create_cell_item(_itm);
				m_pUISlotQuickAccessList_0->SetItem		(itm);
			}
			_itm								= m_pInv->m_slots[SLOT_QUICK_ACCESS_1].m_pIItem;
			if(_itm)
			{
				CUICellItem* itm				= create_cell_item(_itm);
				m_pUISlotQuickAccessList_1->SetItem		(itm);
			}
			_itm								= m_pInv->m_slots[SLOT_QUICK_ACCESS_2].m_pIItem;
			if(_itm)
			{
				CUICellItem* itm				= create_cell_item(_itm);
				m_pUISlotQuickAccessList_2->SetItem		(itm);
			}
			_itm								= m_pInv->m_slots[SLOT_QUICK_ACCESS_3].m_pIItem;
			if(_itm)
			{
				CUICellItem* itm				= create_cell_item(_itm);
				m_pUISlotQuickAccessList_3->SetItem		(itm);
			}
	}
#endif

#if defined(SHOW_ARTEFACT_SLOT)
			_itm								= m_pInv->m_slots[ARTEFACT_SLOT].m_pIItem;
			if(_itm)
			{
				CUICellItem* itm				= create_cell_item(_itm);
				m_pUIArtefactList->SetItem		(itm);
			}
#endif

	PIItem _outfit						= m_pInv->m_slots[OUTFIT_SLOT].m_pIItem;
	CUICellItem* outfit					= (_outfit)?create_cell_item(_outfit):NULL;
#if defined(INV_OUTFIT_FULL_ICON_HIDE)
	if (outfit)
#endif
		m_pUIOutfitList->SetItem			(outfit);

	TIItemContainer::iterator it, it_e;
	for(it=m_pInv->m_belt.begin(),it_e=m_pInv->m_belt.end(); it!=it_e; ++it) 
	{
		CUICellItem* itm			= create_cell_item(*it);
		m_pUIBeltList->SetItem		(itm);
	}
	
	ruck_list		= m_pInv->m_ruck;
	std::sort		(ruck_list.begin(),ruck_list.end(),InventoryUtilities::GreaterRoomInRuck);

	int i=1;
	for(it=ruck_list.begin(),it_e=ruck_list.end(); it!=it_e; ++it,++i) 
	{
		CUICellItem* itm			= create_cell_item(*it);
		m_pUIBagList->SetItem		(itm);
	}
	//fake
	_itm								= m_pInv->m_slots[GRENADE_SLOT].m_pIItem;
	if(_itm)
	{
		CUICellItem* itm				= create_cell_item(_itm);
		m_pUIGrenadeList->SetItem(itm);
	}

	InventoryUtilities::UpdateWeight					(UIBagWnd, true);
	
	UpdateCustomDraw();

	m_b_need_reinit					= false;
}

void CUIInventoryWnd::DropCurrentItem(bool b_all)
{

	CActor *pActor			= smart_cast<CActor*>(Level().CurrentEntity());
	if(!pActor)				return;

	if(!b_all && CurrentIItem() && !CurrentIItem()->IsQuestItem())
	{
		SendEvent_Item_Drop		(CurrentIItem());
		SetCurrentItem			(NULL);
		InventoryUtilities::UpdateWeight			(UIBagWnd, true);
		return;
	}

	if(b_all && CurrentIItem() && !CurrentIItem()->IsQuestItem())
	{
		u32 cnt = CurrentItem()->ChildsCount();

		for(u32 i=0; i<cnt; ++i){
			CUICellItem*	itm				= CurrentItem()->PopChild();
			PIItem			iitm			= (PIItem)itm->m_pData;
			SendEvent_Item_Drop				(iitm);
		}

		SendEvent_Item_Drop					(CurrentIItem());
		SetCurrentItem						(NULL);
		InventoryUtilities::UpdateWeight	(UIBagWnd, true);
		return;
	}
}

//------------------------------------------

bool CUIInventoryWnd::ToSlot(CUICellItem* itm, bool force_place)
{
	auto	old_owner	= itm->OwnerList();
	auto	iitem		= (PIItem)itm->m_pData;
	auto	_slot		= iitem->GetSlot();
		
	if(GetInventory()->CanPutInSlot(iitem))
	{		
		
		auto new_owner	= GetSlotList(_slot);
		
		if (!new_owner)
		{
			Msg("!ERROR: Bad slot %d", _slot);
			GetSlotList(_slot);  // for tracing
			return false;
		}

		GetInventory()->UpdateItemsPlace(iitem, true); //проверим не надо ли сбросить предметы в рюкзак

#ifdef DEBUG_SLOTS
		Msg("# inventory wnd ToSlot (0x%p) from old_owner = 0x%p ", itm, old_owner);
#endif
		auto i	= old_owner->RemoveItem(itm, (old_owner == new_owner) );	
		new_owner->SetItem(i);

		SendEvent_Item2Slot(iitem);

#if defined(INV_NO_ACTIVATE_APPARATUS_SLOT)
		if (activate_slot(_slot))
			SendEvent_ActivateSlot(iitem);
#else
		SendEvent_ActivateSlot(iitem);
#endif

		/************************************************** added by Ray Twitty (aka Shadows) START **************************************************/
		// обновляем статик веса в инвентаре
		InventoryUtilities::UpdateWeight	(UIBagWnd, true);
		/*************************************************** added by Ray Twitty (aka Shadows) END ***************************************************/
		m_b_need_reinit = true;
		
		return	true;
	}
	else
	{ 
		// in case slot is busy
		if(!force_place ||  _slot == NO_ACTIVE_SLOT || GetInventory()->m_slots[_slot].m_bPersistent) 
			return false;

		auto	_iitem		= GetInventory()->m_slots[_slot].m_pIItem;
		auto	slot_list	= GetSlotList(_slot);

		if (0 == slot_list->ItemsCount())
		{
			Msg("!ERROR: slot(%d).ItemsCount = 0 (no cells) ", _slot);
			return false;
		}

		ToBag(slot_list->GetItemIdx(0), false);

		return ToSlot(itm, false);
	}
}

bool CUIInventoryWnd::ToBag(CUICellItem* itm, bool b_use_cursor_pos)
{
	PIItem	iitem						= (PIItem)itm->m_pData;

	if(GetInventory()->CanPutInRuck(iitem))
	{
#ifdef DEBUG_SLOTS
		Msg("# inventory wnd ToBag (0x%p) ", itm);
#endif

		CUIDragDropListEx*	old_owner		= itm->OwnerList();
		CUIDragDropListEx*	new_owner		= NULL;
		if(b_use_cursor_pos){
				new_owner					= CUIDragDropListEx::m_drag_item->BackList();
				VERIFY						(new_owner==m_pUIBagList);
		}else
				new_owner					= m_pUIBagList;

		GetInventory()->UpdateItemsPlace(iitem); //проверим не надо ли сбросить предметы в рюкзак

		bool result							= GetInventory()->Ruck(iitem);
		VERIFY								(result);
		CUICellItem* i						= old_owner->RemoveItem(itm, (old_owner==new_owner) );

		/************************************************** added by Ray Twitty (aka Shadows) START **************************************************/
		// обновляем статик веса в инвентаре
		InventoryUtilities::UpdateWeight	(UIBagWnd, true);
		/*************************************************** added by Ray Twitty (aka Shadows) END ***************************************************/

#ifdef INV_RUCK_UNLIMITED_FIX
		result = new_owner->CanSetItem(i);
		if (result || new_owner->IsAutoGrow())
		{
#endif
			if (b_use_cursor_pos)
			{
				new_owner->SetItem(i, old_owner->GetDragItemPosition());
			}
			else
			{
				new_owner->SetItem(i);
			}
			SendEvent_Item2Ruck					(iitem);
#ifdef INV_RUCK_UNLIMITED_FIX
		}
		else
		{
			NET_Packet					P;
			iitem->object().u_EventGen	(P, GE_OWNERSHIP_REJECT, iitem->object().H_Parent()->ID());
			P.w_u16						(u16(iitem->object().ID()));
			iitem->object().u_EventSend(P);
		}

		m_b_need_reinit = true;
		
		return result;
#else
		m_b_need_reinit = true;
		
		return true;
#endif
	}
	return false;
}

bool CUIInventoryWnd::ToBelt(CUICellItem* itm, bool b_use_cursor_pos)
{
	PIItem	iitem						= (PIItem)itm->m_pData;

	if(GetInventory()->CanPutInBelt(iitem))
	{
		CUIDragDropListEx*	old_owner		= itm->OwnerList();
		CUIDragDropListEx*	new_owner		= NULL;
		if(b_use_cursor_pos){
				new_owner					= CUIDragDropListEx::m_drag_item->BackList();
				VERIFY						(new_owner==m_pUIBeltList);
		}else
				new_owner					= m_pUIBeltList;

		bool result							= GetInventory()->Belt(iitem);
		VERIFY								(result);
		CUICellItem* i						= old_owner->RemoveItem(itm, (old_owner==new_owner) );

		if(b_use_cursor_pos)
			new_owner->SetItem				(i,old_owner->GetDragItemPosition());
		else
			new_owner->SetItem				(i);

		SendEvent_Item2Belt					(iitem);

		/************************************************** added by Ray Twitty (aka Shadows) START **************************************************/
		// обновляем статик веса в инвентаре
		InventoryUtilities::UpdateWeight	(UIBagWnd, true);
		/*************************************************** added by Ray Twitty (aka Shadows) END ***************************************************/

		m_b_need_reinit = true;
		
		return true;
	}
	return	false;
}

void CUIInventoryWnd::AddItemToBag(PIItem pItem)
{
	CUICellItem* itm						= create_cell_item(pItem);
	m_pUIBagList->SetItem					(itm);
}

bool CUIInventoryWnd::OnItemStartDrag(CUICellItem* itm)
{
	return false; //default behaviour
}

bool CUIInventoryWnd::OnItemSelected(CUICellItem* itm)
{
	SetCurrentItem		(itm);
	itm->ColorizeWeapon	(m_pUIBagList, m_pUIBeltList);
	return				false;
}

bool CUIInventoryWnd::OnItemDrop(CUICellItem* itm)
{
	CUIDragDropListEx*	old_owner		= itm->OwnerList();
	CUIDragDropListEx*	new_owner		= CUIDragDropListEx::m_drag_item->BackList();
	if(old_owner==new_owner || !old_owner || !new_owner)
					return false;

	EListType t_new		= GetType(new_owner);
	EListType t_old		= GetType(old_owner);

#if defined(INV_NEW_SLOTS_SYSTEM)
	// Для слотов проверим ниже. Real Wolf.
	if(t_new == t_old && t_new != iwSlot) return true;
#else
	if(t_new == t_old) return true;
#endif
	switch(t_new){
	case iwSlot:
	{
		PIItem item = CurrentIItem();
#ifdef INV_NEW_SLOTS_SYSTEM
		//#if !defined(INV_MOVE_ITM_INTO_QUICK_SLOTS)
		//		u32 old_slot = CurrentIItem()->GetSlot();
		//#endif
		bool can_put = false;
		Ivector2 max_size = new_owner->CellSize();

		LPCSTR name = item->object().Name_script();
		int item_w = item->GetGridWidth();
		int item_h = item->GetGridHeight();

		if (new_owner->GetVerticalPlacement())
			std::swap(max_size.x, max_size.y); 

		if (item_w <= max_size.x && item_h <= max_size.y)
		{
			for (u32 i = 0; i < SLOTS_TOTAL; i++)
				if (new_owner == m_slots_array[i])
				{				
					if (item->IsPlaceable(i, i))
					{
						item->SetSlot(i);
						can_put = true;
						Msg("# Drag-drop item %s to slot %d accepted ", name, i);
					}
					else
					{
						string256 tmp = { 0 };
						for (u32 ii = 0; ii < item->GetSlotsCount(); ii++)
						{
							u32 len = xr_strlen(tmp);
							u32 slot = item->GetSlots()[ii];
							sprintf_s(&tmp[len], 256 - len, "%d ", slot);
						}
						Msg("!WARN: cannot put item %s into slot %d, allowed slots { %s}", name, i, tmp);
					}
					break;
				}   // for-if 
		}
		else
			Msg("!#ERROR: item %s to large for slot: (%d x %d) vs (%d x %d) ", name, item_w, item_h, max_size.x, max_size.y );

			
		if( !can_put // при невозможности поместить в выбранный слот
		#if defined(INV_MOVE_ITM_INTO_QUICK_SLOTS)
			) 
			break; // не помещать в слот, просто дропнуть из слота источника?
		#else
			// ||	!is_quick_slot(item->GetSlot(), item, m_pInv) 
			) 
			{
				// восстановление не требуется, слот не был назначен
				// item->SetSlot(old_slot); 
				return true;
			}
		#endif
#endif				
			if(GetSlotList(item->GetSlot()) == new_owner)
				ToSlot	(itm, true);
		}break;
		case iwBag:{
			ToBag	(itm, true);
		}break;
		case iwBelt:{
			ToBelt	(itm, true);
		}break;
	};

	DropItem				(CurrentIItem(), new_owner);

	return true;
}

#include "../eatable_item.h"
bool CUIInventoryWnd::OnItemDbClick(CUICellItem* itm)
{
	PIItem	__item		= (PIItem)itm->m_pData;
	u32		__slot		= __item->GetSlot();
	auto	old_owner	= itm->OwnerList();
/*#if  defined(INV_NEW_SLOTS_SYSTEM)
	if (__slot < SLOT_QUICK_ACCESS_0 || __slot > SLOT_QUICK_ACCESS_3)
	{
		if(TryUseItem(__item))
		return true;
	}
	else
	{
		PIItem iitem = GetInventory()->Same(__item, true);

		if(iitem)
		{
			if(TryUseItem(iitem))
			return true;
		}
		else
		{
			if(TryUseItem(__item))
			return true;
		}	
	}
#else*/
	if(TryUseItem(__item))
		return true;
//#endif
	
	EListType t_old						= GetType(old_owner);

	switch(t_old){
		case iwSlot:{
			ToBag	(itm, false);
		}break;

		case iwBag:
		{
#ifdef INV_NEW_SLOTS_SYSTEM
			// Пытаемся найти свободный слот из списка разрешенных.
			// Если его нету, то принудительно займет первый слот, указанный в списке.
			auto slots = __item->GetSlots();
			#if !defined(INV_MOVE_ITM_INTO_QUICK_SLOTS)
			bool is_eat = __item->cast_eatable_item() != NULL;
			#endif
			for (u8 i = 0; i < (u8)slots.size(); ++i)
			{
				#if !defined(INV_MOVE_ITM_INTO_QUICK_SLOTS)
				if (!is_eat || is_quick_slot(slots[i], __item, m_pInv) )
				{
					__item->SetSlot(slots[i]);
					if (ToSlot(itm, false) )
						return true;
				}
				#else
					__item->SetSlot(slots[i]);
					if (ToSlot(itm, false) )
						return true;		
				#endif
			}			
			__item->SetSlot(slots.size()? slots[0]: NO_ACTIVE_SLOT);
#endif
			if(!ToSlot(itm, false))
			{
				if( !ToBelt(itm, false) )
					ToSlot	(itm, true);
			}
		}break;

		case iwBelt:{
			ToBag	(itm, false);
		}break;
	};

	return true;
}


bool CUIInventoryWnd::OnItemRButtonClick(CUICellItem* itm)
{
	SetCurrentItem				(itm);
	ActivatePropertiesBox		();
	return						false;
}

CUIDragDropListEx* CUIInventoryWnd::GetSlotList(u32 slot_idx)
{	
	if(slot_idx == NO_ACTIVE_SLOT || GetInventory()->m_slots[slot_idx].m_bPersistent)	return NULL;
	return m_slots_array[slot_idx];	
}



void CUIInventoryWnd::ClearAllLists()
{
	m_pUIBagList->ClearAll					(true);
	m_pUIBeltList->ClearAll					(true);
	m_pUIOutfitList->ClearAll				(true);
	m_pUIPistolList->ClearAll				(true);
	m_pUIAutomaticList->ClearAll			(true);

#ifdef INV_NEW_SLOTS_SYSTEM
if (IsGameTypeSingle()) {
	m_pUIKnifeList->ClearAll				(true);
	m_pUIBinocularList->ClearAll			(true);
	m_pUIDetectorList->ClearAll				(true);
	m_pUITorchList->ClearAll				(true);
	m_pUIPDAList->ClearAll					(true);
	m_pUIHelmetList->ClearAll				(true);
	m_pUISlotQuickAccessList_0->ClearAll	(true);
	m_pUISlotQuickAccessList_1->ClearAll	(true);
	m_pUISlotQuickAccessList_2->ClearAll	(true);
	m_pUISlotQuickAccessList_3->ClearAll	(true);
}
#endif
//#ifdef SHOW_GRENADE_SLOT
    m_pUIGrenadeList->ClearAll              (true);
//#endif
#if defined(SHOW_ARTEFACT_SLOT)
    m_pUIArtefactList->ClearAll				(true);
#endif
}
