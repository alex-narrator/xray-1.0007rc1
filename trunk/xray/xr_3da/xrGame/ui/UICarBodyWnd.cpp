#include "stdafx.h"
#include "../pch_script.h"
#include "UICarBodyWnd.h"
#include "xrUIXmlParser.h"
#include "UIXmlInit.h"
#include "../HUDManager.h"
#include "../level.h"
#include "UICharacterInfo.h"
#include "UIDragDropListEx.h"
#include "UIFrameWindow.h"
#include "UIItemInfo.h"
#include "UIPropertiesBox.h"
#include "../ai/monsters/BaseMonster/base_monster.h"
#include "../inventory.h"
#include "UIInventoryUtilities.h"
#include "UICellItem.h"
#include "UICellItemFactory.h"
#include "../WeaponMagazined.h"
#include "../Actor.h"
#include "../eatable_item.h"
#include "../alife_registry_wrappers.h"
#include "UI3tButton.h"
#include "UIListBoxItem.h"
#include "../InventoryBox.h"
#include "../game_object_space.h"
#include "../script_callback_ex.h"
#include "../script_game_object.h"
#include "../BottleItem.h"
#include "../WeaponKnife.h"

#define				CAR_BODY_XML		"carbody_new.xml"
#define				CARBODY_ITEM_XML	"carbody_item.xml"

void move_item (u16 from_id, u16 to_id, u16 what_id);

CUICarBodyWnd::CUICarBodyWnd()
{
	m_pInventoryBox		= nullptr;
	m_pOurObject		= nullptr;
	m_pOthersObject		= nullptr;
	Init				();
	Hide				();
	m_b_need_update		= false;
}

CUICarBodyWnd::~CUICarBodyWnd()
{
	m_pUIOurBagList->ClearAll					(true);
	m_pUIOthersBagList->ClearAll				(true);
}

void CUICarBodyWnd::Init()
{
	CUIXml						uiXml;
	uiXml.Init					(CONFIG_PATH, UI_PATH, CAR_BODY_XML);
	
	CUIXmlInit					xml_init;

	xml_init.InitWindow			(uiXml, "main", 0, this);

	m_pUIStaticTop				= xr_new<CUIStatic>(); m_pUIStaticTop->SetAutoDelete(true);
	AttachChild					(m_pUIStaticTop);
	xml_init.InitStatic			(uiXml, "top_background", 0, m_pUIStaticTop);


	m_pUIStaticBottom			= xr_new<CUIStatic>(); m_pUIStaticBottom->SetAutoDelete(true);
	AttachChild					(m_pUIStaticBottom);
	xml_init.InitStatic			(uiXml, "bottom_background", 0, m_pUIStaticBottom);

	m_pUIOurIcon				= xr_new<CUIStatic>(); m_pUIOurIcon->SetAutoDelete(true);
	AttachChild					(m_pUIOurIcon);
	xml_init.InitStatic			(uiXml, "static_icon", 0, m_pUIOurIcon);

	m_pUIOthersIcon				= xr_new<CUIStatic>(); m_pUIOthersIcon->SetAutoDelete(true);
	AttachChild					(m_pUIOthersIcon);
	xml_init.InitStatic			(uiXml, "static_icon", 1, m_pUIOthersIcon);


	m_pUICharacterInfoLeft		= xr_new<CUICharacterInfo>(); m_pUICharacterInfoLeft->SetAutoDelete(true);
	m_pUIOurIcon->AttachChild	(m_pUICharacterInfoLeft);
	m_pUICharacterInfoLeft->Init(0,0, m_pUIOurIcon->GetWidth(), m_pUIOurIcon->GetHeight(), "trade_character.xml");


	m_pUICharacterInfoRight			= xr_new<CUICharacterInfo>(); m_pUICharacterInfoRight->SetAutoDelete(true);
	m_pUIOthersIcon->AttachChild	(m_pUICharacterInfoRight);
	m_pUICharacterInfoRight->Init	(0,0, m_pUIOthersIcon->GetWidth(), m_pUIOthersIcon->GetHeight(), "trade_character.xml");

	m_pUIOurBagWnd					= xr_new<CUIStatic>(); m_pUIOurBagWnd->SetAutoDelete(true);
	AttachChild						(m_pUIOurBagWnd);
	xml_init.InitStatic				(uiXml, "our_bag_static", 0, m_pUIOurBagWnd);

	m_pUIOurVolumeWnd				= xr_new<CUIStatic>(); m_pUIOurVolumeWnd->SetAutoDelete(true);
	m_pUIOurBagWnd->AttachChild		(m_pUIOurVolumeWnd);
	xml_init.InitStatic				(uiXml, "our_volume_static", 0, m_pUIOurVolumeWnd);

	m_pUIOthersBagWnd				= xr_new<CUIStatic>(); m_pUIOthersBagWnd->SetAutoDelete(true);
	AttachChild						(m_pUIOthersBagWnd);
	xml_init.InitStatic				(uiXml, "others_bag_static", 0, m_pUIOthersBagWnd);

	m_pUIOurBagList					= xr_new<CUIDragDropListEx>(); m_pUIOurBagList->SetAutoDelete(true);
	m_pUIOurBagWnd->AttachChild		(m_pUIOurBagList);	
	xml_init.InitDragDropListEx		(uiXml, "dragdrop_list_our", 0, m_pUIOurBagList);

	m_pUIOthersBagList				= xr_new<CUIDragDropListEx>(); m_pUIOthersBagList->SetAutoDelete(true);
	m_pUIOthersBagWnd->AttachChild	(m_pUIOthersBagList);	
	xml_init.InitDragDropListEx		(uiXml, "dragdrop_list_other", 0, m_pUIOthersBagList);


	//информация о предмете
	m_pUIDescWnd					= xr_new<CUIFrameWindow>(); m_pUIDescWnd->SetAutoDelete(true);
	AttachChild						(m_pUIDescWnd);
	xml_init.InitFrameWindow		(uiXml, "frame_window", 0, m_pUIDescWnd);

	m_pUIStaticDesc					= xr_new<CUIStatic>(); m_pUIStaticDesc->SetAutoDelete(true);
	m_pUIDescWnd->AttachChild		(m_pUIStaticDesc);
	xml_init.InitStatic				(uiXml, "descr_static", 0, m_pUIStaticDesc);

	m_pUIItemInfo					= xr_new<CUIItemInfo>(); m_pUIItemInfo->SetAutoDelete(true);
	m_pUIDescWnd->AttachChild		(m_pUIItemInfo);
	m_pUIItemInfo->Init				(0,0, m_pUIDescWnd->GetWidth(), m_pUIDescWnd->GetHeight(), CARBODY_ITEM_XML);


	xml_init.InitAutoStatic			(uiXml, "auto_static", this);

	m_pUIPropertiesBox				= xr_new<CUIPropertiesBox>(); m_pUIPropertiesBox->SetAutoDelete(true);
	AttachChild						(m_pUIPropertiesBox);
	m_pUIPropertiesBox->Init		(0,0,300,300);
	m_pUIPropertiesBox->Hide		();

	SetCurrentItem					(NULL);
	m_pUIStaticDesc->SetText		(NULL);

	m_pUITakeAll					= xr_new<CUI3tButton>(); m_pUITakeAll->SetAutoDelete(true);
	AttachChild						(m_pUITakeAll);
	xml_init.Init3tButton			(uiXml, "take_all_btn", 0, m_pUITakeAll);

	m_pUIExitButton					= xr_new<CUI3tButton>(); m_pUIExitButton->SetAutoDelete(true);
	AttachChild						(m_pUIExitButton);
	xml_init.Init3tButton			(uiXml, "exit_button", 0, m_pUIExitButton);

	m_pUIRepackAmmoButton			= xr_new<CUI3tButton>(); m_pUIRepackAmmoButton->SetAutoDelete(true);
	AttachChild						(m_pUIRepackAmmoButton);
	xml_init.Init3tButton			(uiXml, "repack_ammo_button", 0, m_pUIRepackAmmoButton);

	m_pUIMoveAllFromRuckButton		= xr_new<CUI3tButton>(); m_pUIMoveAllFromRuckButton->SetAutoDelete(true);
	AttachChild						(m_pUIMoveAllFromRuckButton);
	xml_init.Init3tButton			(uiXml, "move_all_from_ruck_button", 0, m_pUIMoveAllFromRuckButton);

	BindDragDropListEvents			(m_pUIOurBagList);
	BindDragDropListEvents			(m_pUIOthersBagList);

	//Load sounds
	if (uiXml.NavigateToNode("action_sounds", 0))
	{
		XML_NODE* stored_root = uiXml.GetLocalRoot();
		uiXml.SetLocalRoot(uiXml.NavigateToNode("action_sounds", 0));

		::Sound->create(sounds[eInvSndOpen],		uiXml.Read("snd_open",			0, NULL), st_Effect, sg_SourceType);
		::Sound->create(sounds[eInvSndClose],		uiXml.Read("snd_close",			0, NULL), st_Effect, sg_SourceType);
		::Sound->create(sounds[eInvProperties],		uiXml.Read("snd_properties",	0, NULL), st_Effect, sg_SourceType);
		::Sound->create(sounds[eInvDropItem],		uiXml.Read("snd_drop_item",		0, NULL), st_Effect, sg_SourceType);
		::Sound->create(sounds[eInvMoveItem],		uiXml.Read("snd_move_item",		0, NULL), st_Effect, sg_SourceType);
		::Sound->create(sounds[eInvDetachAddon],	uiXml.Read("snd_detach_addon",	0, NULL), st_Effect, sg_SourceType);
		::Sound->create(sounds[eInvItemUse],		uiXml.Read("snd_item_use",		0, NULL), st_Effect, sg_SourceType);

		uiXml.SetLocalRoot(stored_root);
	}
}

void CUICarBodyWnd::InitCarBody(CInventoryOwner* pOur, CInventoryBox* pInvBox)
{
    m_pOurObject									= pOur;
	m_pOthersObject									= NULL;
	m_pInventoryBox									= pInvBox;
	m_pInventoryBox->m_in_use						= true;

	u16 our_id										= smart_cast<CGameObject*>(m_pOurObject)->ID();
	m_pUICharacterInfoLeft->InitCharacter			(our_id);
	m_pUIOthersIcon->Show							(false);
	m_pUICharacterInfoRight->ClearInfo				();
	m_pUIPropertiesBox->Hide						();
	EnableAll										();
	UpdateLists										();

	// Real Wolf: колбек на открытие ящика. 02.08.2014.
	pInvBox->callback(GameObject::eOnInvBoxOpen)();
}

void CUICarBodyWnd::InitCarBody(CInventoryOwner* pOur, CInventoryOwner* pOthers)
{

    m_pOurObject									= pOur;
	m_pOthersObject									= pOthers;
	m_pInventoryBox									= NULL;
	
	u16 our_id										= smart_cast<CGameObject*>(m_pOurObject)->ID();
	u16 other_id									= smart_cast<CGameObject*>(m_pOthersObject)->ID();

	m_pUICharacterInfoLeft->InitCharacter			(our_id);
	m_pUIOthersIcon->Show							(true);
	
	CBaseMonster *monster = NULL;
	if(m_pOthersObject) {
		monster										= smart_cast<CBaseMonster *>(m_pOthersObject);
		if (monster || m_pOthersObject->use_simplified_visual() ) 
		{
			m_pUICharacterInfoRight->ClearInfo		();
			if(monster)
			{
				shared_str monster_tex_name = pSettings->r_string(monster->cNameSect(),"icon");
				m_pUICharacterInfoRight->UIIcon().InitTexture(monster_tex_name.c_str());
				m_pUICharacterInfoRight->UIIcon().SetStretchTexture(true);
			}
		}else 
		{
			m_pUICharacterInfoRight->InitCharacter	(other_id);
		}
	}

	m_pUIPropertiesBox->Hide						();
	EnableAll										();
	UpdateLists										();

	if(!monster){
		CInfoPortionWrapper	*known_info_registry	= xr_new<CInfoPortionWrapper>();
		known_info_registry->registry().init		(other_id);
		KNOWN_INFO_VECTOR& known_info				= known_info_registry->registry().objects();

		KNOWN_INFO_VECTOR_IT it = known_info.begin();
		for(int i=0;it!=known_info.end();++it,++i){
			(*it).info_id;	
			NET_Packet		P;
			CGameObject::u_EventGen		(P,GE_INFO_TRANSFER, our_id);
			P.w_u16						(0);//not used
			P.w_stringZ					((*it).info_id);			//сообщение
			P.w_u8						(1);						//добавление сообщения
			CGameObject::u_EventSend	(P);
		}
		known_info.clear	();
		xr_delete			(known_info_registry);
	}
}  

void CUICarBodyWnd::UpdateLists_delayed()
{
		m_b_need_update = true;
}

void CUICarBodyWnd::Hide()
{
	InventoryUtilities::SendInfoToActor			("ui_car_body_hide");
	m_pUIOurBagList->ClearAll					(true);
	m_pUIOthersBagList->ClearAll				(true);
	inherited::Hide								();
	if(m_pInventoryBox)
		m_pInventoryBox->m_in_use				= false;
    //
	CActor *pActor = smart_cast<CActor*>(Level().CurrentEntity());
	CBaseMonster* Monster = smart_cast<CBaseMonster *>(m_pOthersObject);
	if (pActor)
	{
		//ОБЯЗАТЕЛЬНО!!! одинаковая проверка для Hide() и Show(), иначе всё ломается
		if (g_eFreeHands == eFreeHandsManual && !Monster) pActor->SetWeaponHideState(INV_STATE_INV_WND, false);  //восстановим показ оружия в руках, если обыскиваем не монстра
		pActor->inventory().TryToHideWeapon(false);
		if (psActorFlags.test(AF_AMMO_FROM_BELT)) pActor->inventory().m_bRuckAmmoPlacement = false; //сбросим флаг перезарядки из рюкзака
	}
    //
	PlaySnd(eInvSndClose);
}

void CUICarBodyWnd::UpdateLists()
{
	TIItemContainer								ruck_list;
	m_pUIOurBagList->ClearAll					(true);
	int i_pos = m_pUIOthersBagList->ScrollPos();	
	m_pUIOthersBagList->ClearAll				(true);

	ruck_list.clear								();
	m_pOurObject->inventory().AddAvailableItems	(ruck_list, true);
	std::sort									(ruck_list.begin(),ruck_list.end(),InventoryUtilities::GreaterRoomInRuck);

	//Наш рюкзак
	TIItemContainer::iterator it;
	for(it =  ruck_list.begin(); ruck_list.end() != it; ++it) 
	{
		CUICellItem* itm				= create_cell_item(*it);
		m_pUIOurBagList->SetItem		(itm);
		itm->ColorizeEquipped			(itm);
	}


	ruck_list.clear									();
	if(m_pOthersObject)
		m_pOthersObject->inventory().AddAvailableItems	(ruck_list, false);
	else
		m_pInventoryBox->AddAvailableItems			(ruck_list);

	std::sort										(ruck_list.begin(),ruck_list.end(),InventoryUtilities::GreaterRoomInRuck);

	//Чужой рюкзак
	for(it =  ruck_list.begin(); ruck_list.end() != it; ++it) 
	{
		CUICellItem* itm							= create_cell_item(*it);
		m_pUIOthersBagList->SetItem					(itm);
	}

	
	m_pUIOthersBagList->SetScrollPos(i_pos);

	InventoryUtilities::UpdateWeight				(*m_pUIOurBagWnd, true);
	InventoryUtilities::UpdateVolume				(*m_pUIOurVolumeWnd, true);
	m_b_need_update									= false;
}

#include "ui_af_params.h"
void CUICarBodyWnd::SendMessage(CUIWindow *pWnd, s16 msg, void *pData)
{
	if (BUTTON_CLICKED == msg && m_pUITakeAll == pWnd)
	{
		TakeAll					();
	}
	else if (BUTTON_CLICKED == msg && m_pUIExitButton == pWnd)
	{
		GetHolder()->StartStopMenu(this, true);
	}
	else if (BUTTON_CLICKED == msg && m_pUIRepackAmmoButton == pWnd)
	{
		m_pOurObject->inventory().RepackAmmo();
		UpdateLists_delayed();
	}
	else if (BUTTON_CLICKED == msg && m_pUIMoveAllFromRuckButton == pWnd)
	{
		MoveItemWithContent(NULL, BACKPACK_SLOT);
	}
	else if(pWnd == m_pUIPropertiesBox &&	msg == PROPERTY_CLICKED)
	{
		CUICellItem *	itm		= CurrentItem();
		CWeapon*		pWeapon = smart_cast<CWeapon*>		(CurrentIItem());
		CWeaponAmmo*	pAmmo	= smart_cast<CWeaponAmmo*>	(CurrentIItem());
		//
		if(m_pUIPropertiesBox->GetClickedItem())
		{
			switch(m_pUIPropertiesBox->GetClickedItem()->GetTAG())
			{
			case INVENTORY_EAT_ACTION:	//съесть объект
				EatItem();
				SetCurrentItem(NULL);
				break;
				//
			case INVENTORY_MOVE_ACTION:  
			{
				void* d = m_pUIPropertiesBox->GetClickedItem()->GetData();
				bool b_all = (d == (void*)33);
				MoveItemsFromCell(itm, b_all);
			}
			break;
			case INVENTORY_MOVE_WITH_CONTENT:
			{
				auto iitem = (PIItem)itm->m_pData;
				u32 slot = iitem->GetSlot();
				MoveItemWithContent(itm, slot);
			}
			break;
			case INVENTORY_RELOAD_MAGAZINE:
				pWeapon->m_set_next_ammoType_on_reload = (u8)(m_pUIPropertiesBox->GetClickedItem()->GetData());
				pWeapon->ReloadWeapon();
				SetCurrentItem(NULL);
				break;
			case INVENTORY_SWITCH_GRENADE_LAUNCHER_MODE:
				pWeapon->Action(kWPN_FUNC, CMD_START);
				SetCurrentItem(NULL);
				break;
			case INVENTORY_UNLOAD_MAGAZINE:
				{
				(smart_cast<CWeaponMagazined*>((CWeapon*)itm->m_pData))->UnloadMagazine();
				(smart_cast<CWeaponMagazined*>((CWeapon*)itm->m_pData))->PullShutter();
				for(u32 i=0; i<itm->ChildsCount(); ++i)
				{
					CUICellItem * child_itm			= itm->Child(i);
					(smart_cast<CWeaponMagazined*>((CWeapon*)child_itm->m_pData))->UnloadMagazine();
					(smart_cast<CWeaponMagazined*>((CWeapon*)child_itm->m_pData))->PullShutter();
				}
				}break;
			case INVENTORY_RELOAD_AMMO_BOX:
				{
				pAmmo->ReloadBox((LPCSTR)m_pUIPropertiesBox->GetClickedItem()->GetData());
				SetCurrentItem(NULL);
				}break;
			case INVENTORY_UNLOAD_AMMO_BOX:
				{
				pAmmo->UnloadBox();
				SetCurrentItem(NULL);
				}break;
			case INVENTORY_DETACH_SCOPE_ADDON:
				{
					pWeapon->Detach(pWeapon->GetScopeName().c_str(), true);
					PlaySnd(eInvDetachAddon);
				}break;
			case INVENTORY_DETACH_SILENCER_ADDON:
				{
					pWeapon->Detach(pWeapon->GetSilencerName().c_str(), true);
					PlaySnd(eInvDetachAddon);
				}break;
			case INVENTORY_DETACH_GRENADE_LAUNCHER_ADDON:
				{
					pWeapon->Detach(pWeapon->GetGrenadeLauncherName().c_str(), true);
					PlaySnd(eInvDetachAddon);
				}break;
			case INVENTORY_DROP_ACTION:
				{
					void* d = m_pUIPropertiesBox->GetClickedItem()->GetData();
					bool b_all = (d == (void*)33);

					DropItemsfromCell(b_all);
				}break;
			}
			//в случае инвентарных ящиков нужно обновить окошко
			switch (m_pUIPropertiesBox->GetClickedItem()->GetTAG())
			{
			case INVENTORY_UNLOAD_MAGAZINE:
			case INVENTORY_RELOAD_AMMO_BOX:
			case INVENTORY_UNLOAD_AMMO_BOX:
			case INVENTORY_DETACH_SCOPE_ADDON:
			case INVENTORY_DETACH_SILENCER_ADDON:
			case INVENTORY_DETACH_GRENADE_LAUNCHER_ADDON:
			{
				if (m_pInventoryBox)
				{
					UpdateLists_delayed();
				}
			}break;
			}
		}
	}

	inherited::SendMessage			(pWnd, msg, pData);
}

void CUICarBodyWnd::Draw()
{
	inherited::Draw	();
}


void CUICarBodyWnd::Update()
{
	if (m_b_need_update ||
		m_pOurObject->inventory().ModifyFrame()==Device.dwFrame || 
		(m_pOthersObject&&m_pOthersObject->inventory().ModifyFrame()==Device.dwFrame))

		UpdateLists		();

	m_pUIOurVolumeWnd->SetVisible(!!psActorFlags.is(AF_INVENTORY_VOLUME));

	InventoryUtilities::UpdateWeight(*m_pUIOurBagWnd, true);
	InventoryUtilities::UpdateVolume(*m_pUIOurVolumeWnd, true);
	
	if(m_pOthersObject && (smart_cast<CGameObject*>(m_pOurObject))->Position().distance_to((smart_cast<CGameObject*>(m_pOthersObject))->Position()) > 3.0f)
	{
		GetHolder()->StartStopMenu(this,true);
	}
	inherited::Update();
}


void CUICarBodyWnd::Show() 
{ 
	InventoryUtilities::SendInfoToActor		("ui_car_body");
	inherited::Show							();
	SetCurrentItem							(NULL);
/*	InventoryUtilities::UpdateWeight		(*m_pUIOurBagWnd, true);
	InventoryUtilities::UpdateVolume		(*m_pUIOurVolumeWnd, true);*/
	//
	CActor *pActor = smart_cast<CActor*>(Level().CurrentEntity());
	CBaseMonster* Monster = smart_cast<CBaseMonster *>(m_pOthersObject);
	if (pActor)
	{
		//ОБЯЗАТЕЛЬНО!!! одинаковая проверка для Hide() и Show(), иначе всё ломается
		if (g_eFreeHands == eFreeHandsManual && !Monster) pActor->SetWeaponHideState(INV_STATE_INV_WND, true);  //спрячем оружие в руках, если обыскиваем не монстра 
		pActor->inventory().TryToHideWeapon(true);
		if (psActorFlags.test(AF_AMMO_FROM_BELT)) pActor->inventory().m_bRuckAmmoPlacement = true; //установим флаг перезарядки из рюкзака
	}
	//
	PlaySnd(eInvSndOpen);
}

void CUICarBodyWnd::DisableAll()
{
	m_pUIOurBagWnd->Enable			(false);
	m_pUIOthersBagWnd->Enable		(false);
}

void CUICarBodyWnd::EnableAll()
{
	m_pUIOurBagWnd->Enable			(true);
	m_pUIOthersBagWnd->Enable		(true);
}

CUICellItem* CUICarBodyWnd::CurrentItem()
{
	return m_pCurrentCellItem;
}

PIItem CUICarBodyWnd::CurrentIItem()
{
	return	(m_pCurrentCellItem)?(PIItem)m_pCurrentCellItem->m_pData : NULL;
}

void CUICarBodyWnd::SetCurrentItem(CUICellItem* itm)
{
	if(m_pCurrentCellItem == itm) return;
	m_pCurrentCellItem		= itm;
	m_pUIItemInfo->InitItem(CurrentIItem());
}

void CUICarBodyWnd::TakeAll()
{
	u32 cnt				= m_pUIOthersBagList->ItemsCount();
	u16 tmp_id = 0;
	if(m_pInventoryBox){
		tmp_id	= (smart_cast<CGameObject*>(m_pOurObject))->ID();
	}

	for(u32 i=0; i<cnt; ++i)
	{
		CUICellItem*	ci = m_pUIOthersBagList->GetItemIdx(i);
		for(u32 j=0; j<ci->ChildsCount(); ++j)
		{
			PIItem _itm		= (PIItem)(ci->Child(j)->m_pData);

			if(m_pOthersObject)
				TransferItem	(_itm, m_pOthersObject, m_pOurObject, false);
			else{
				move_item		(m_pInventoryBox->ID(), tmp_id, _itm->object().ID());
//.				Actor()->callback(GameObject::eInvBoxItemTake)( m_pInventoryBox->lua_game_object(), _itm->object().lua_game_object() );
			}
		
		}
		PIItem itm		= (PIItem)(ci->m_pData);

		if(m_pOthersObject)
			TransferItem	(itm, m_pOthersObject, m_pOurObject, false);
		else{
			move_item		(m_pInventoryBox->ID(), tmp_id, itm->object().ID());
//.			Actor()->callback(GameObject::eInvBoxItemTake)(m_pInventoryBox->lua_game_object(), itm->object().lua_game_object() );
		}
	}
	PlaySnd(eInvMoveItem);
}

void CUICarBodyWnd::SendEvent_Item_Drop(PIItem	pItem)
{
	pItem->OnMoveOut(pItem->m_eItemPlace);
	pItem->SetDropManual(TRUE);

	if (OnClient())
	{
		NET_Packet P;
		pItem->object().u_EventGen(P, GE_OWNERSHIP_REJECT, pItem->object().H_Parent()->ID());
		P.w_u16(pItem->object().ID());
		pItem->object().u_EventSend(P);
	}
}

void CUICarBodyWnd::DropItemsfromCell(bool b_all)
{
	CActor *pActor = smart_cast<CActor*>(Level().CurrentEntity());
	if (!pActor)
	{
		return;
	}

	CUICellItem* ci = CurrentItem();
	if (!ci)
	{
		return;
	}

	CUIDragDropListEx* owner_list = ci->OwnerList();

	if (b_all)
	{
		u32 cnt = ci->ChildsCount();

		for (u32 i = 0; i<cnt; ++i) 
		{
			CUICellItem*	itm = ci->PopChild();
			PIItem			iitm = (PIItem)itm->m_pData;

			SendEvent_Item_Drop(iitm);
		}
	}

	SendEvent_Item_Drop(CurrentIItem());

	owner_list->RemoveItem(ci, b_all);

	SetCurrentItem(NULL);
	PlaySnd(eInvDropItem);
}

#include "../xr_level_controller.h"

bool CUICarBodyWnd::OnKeyboard(int dik, EUIMessages keyboard_action)
{
	//восстановление контекстного меню
	if (m_b_need_update)
		return true;

	if (m_pUIPropertiesBox->GetVisible())
		m_pUIPropertiesBox->OnKeyboard(dik, keyboard_action);
	//
	if( inherited::OnKeyboard(dik,keyboard_action) )return true;

	if(keyboard_action==WINDOW_KEY_PRESSED && is_binded(kUSE, dik)) 
	{
			GetHolder()->StartStopMenu(this,true);
			return true;
	}

	// забрать всё по кнопке kSPRINT_TOGGLE (X)
	if (keyboard_action == WINDOW_KEY_PRESSED && is_binded(kSPRINT_TOGGLE, dik))
	{
		TakeAll();
		SetCurrentItem(NULL);
		return true;
	}
	//
	b_TakeAllActionKeyHolded = keyboard_action == WINDOW_KEY_PRESSED && is_binded(kCROUCH, dik);

	return false;
}
//восстановление контекстного меню
bool CUICarBodyWnd::OnMouse(float x, float y, EUIMessages mouse_action)
{
	if (m_b_need_update)
		return true;

	//вызов дополнительного меню по правой кнопке
	if (mouse_action == WINDOW_RBUTTON_DOWN)
	{
		if (m_pUIPropertiesBox->IsShown())
		{
			m_pUIPropertiesBox->Hide();
			return						true;
		}
	}

	if (m_pUIPropertiesBox->IsShown())
	{
		if (mouse_action == WINDOW_MOUSE_WHEEL_DOWN || mouse_action == WINDOW_MOUSE_WHEEL_UP)
			return true;
	}

	CUIWindow::OnMouse(x, y, mouse_action);

	return true; // always returns true, because ::StopAnyMove() == true;
}

bool CUICarBodyWnd::MoveItemsFromCell(CUICellItem* itm, bool b_all)
{
	u16 tmp_id = 0;

	CUIDragDropListEx* owner_list = itm->OwnerList();
	if (owner_list != m_pUIOthersBagList)
	{ //actor -> other
		CUICellItem* ci = CurrentItem();
		for (u32 j = 0; j<ci->ChildsCount() && b_all; ++j)
		{
			PIItem _itm = (PIItem)(ci->Child(j)->m_pData);

			if (m_pOthersObject)
				TransferItem(_itm, m_pOurObject, m_pOthersObject, false);
			else
				move_item(tmp_id, m_pInventoryBox->ID(), _itm->object().ID());
		}
		PIItem itm = (PIItem)(ci->m_pData);

		if (m_pOthersObject)
			TransferItem(itm, m_pOurObject, m_pOthersObject, false);
		else
			move_item(tmp_id, m_pInventoryBox->ID(), itm->object().ID());
	}
	else
	{ // other -> actor
		CUICellItem* ci = CurrentItem();
		for (u32 j = 0; j<ci->ChildsCount() && b_all; ++j)
		{
			PIItem _itm = (PIItem)(ci->Child(j)->m_pData);

			if (m_pOthersObject)
				TransferItem(_itm, m_pOthersObject, m_pOurObject, false);
			else
				move_item(m_pInventoryBox->ID(), tmp_id, _itm->object().ID());
		}
		PIItem itm = (PIItem)(ci->m_pData);

		if (m_pOthersObject)
			TransferItem(itm, m_pOthersObject, m_pOurObject, false);
		else
			move_item(m_pInventoryBox->ID(), tmp_id, itm->object().ID());
	}
	PlaySnd(eInvMoveItem);
	return				true;
}

#include "../Medkit.h"
#include "../Antirad.h"
#include "../Artifact.h"
#include "../WarBelt.h"
#include "../BackPack.h"
#include "../string_table.h"
void CUICarBodyWnd::ActivatePropertiesBox()
{
	//if(m_pInventoryBox)	return;
		
	m_pUIPropertiesBox->RemoveAll();

	auto pWeapon			= smart_cast<CWeaponMagazined*>			(CurrentIItem());
	auto pEatableItem		= smart_cast<CEatableItem*>				(CurrentIItem());
	auto pMedkit			= smart_cast<CMedkit*>					(CurrentIItem());
	auto pAntirad			= smart_cast<CAntirad*>					(CurrentIItem());
	auto pBottleItem		= smart_cast<CBottleItem*>				(CurrentIItem());
	auto pArtefact			= smart_cast<CArtefact*>				(CurrentIItem());
	auto pAmmo				= smart_cast<CWeaponAmmo*>				(CurrentIItem());
	auto pWarBelt			= smart_cast<CWarBelt*>					(CurrentIItem());
	auto pBackPack			= smart_cast<CBackPack*>				(CurrentIItem());

    bool						b_show			= false;
	
	LPCSTR _action				= NULL;
	string1024 temp;

	CUIDragDropListEx*	owner = CurrentItem()->OwnerList();

	if (pAmmo)
	{
		LPCSTR _ammo_sect;

		if (pAmmo->IsBoxReloadable())
		{
			//unload AmmoBox
			m_pUIPropertiesBox->AddItem("st_unload", NULL, INVENTORY_UNLOAD_AMMO_BOX);
			b_show = true;
			//reload AmmoBox
			if (pAmmo->m_boxCurr < pAmmo->m_boxSize && owner == m_pUIOurBagList)
			{
				if (m_pOurObject->inventory().GetAmmo(*pAmmo->m_ammoSect, true))
				{
					strconcat(sizeof(temp), temp, *CStringTable().translate("st_load_ammo_type"), " ",
						*CStringTable().translate(pSettings->r_string(pAmmo->m_ammoSect, "inv_name_short")));
					_ammo_sect = *pAmmo->m_ammoSect;
					m_pUIPropertiesBox->AddItem(temp, (void*)_ammo_sect, INVENTORY_RELOAD_AMMO_BOX);
					b_show = true;
				}
			}
		}
		else if (pAmmo->IsBoxReloadableEmpty() && owner == m_pUIOurBagList)
		{
			for (u8 i = 0; i < pAmmo->m_ammoTypes.size(); ++i)
			{
				if (m_pOurObject->inventory().GetAmmo(*pAmmo->m_ammoTypes[i], true))
				{
					strconcat(sizeof(temp), temp, *CStringTable().translate("st_load_ammo_type"), " ",
						*CStringTable().translate(pSettings->r_string(pAmmo->m_ammoTypes[i], "inv_name_short")));
					_ammo_sect = *pAmmo->m_ammoTypes[i];
					m_pUIPropertiesBox->AddItem(temp, (void*)_ammo_sect, INVENTORY_RELOAD_AMMO_BOX);
					b_show = true;
				}
			}
		}
	}

	if (pWeapon)
	{
		if (pWeapon->IsGrenadeLauncherAttached())
		{
			const char *switch_gl_text = pWeapon->GetGrenadeMode() ? "st_deactivate_gl" : "st_activate_gl";
			if (m_pOurObject->inventory().InSlot(pWeapon))
				m_pUIPropertiesBox->AddItem(switch_gl_text, NULL, INVENTORY_SWITCH_GRENADE_LAUNCHER_MODE);
			b_show = true;
		}
		if (pWeapon->GrenadeLauncherAttachable() && pWeapon->IsGrenadeLauncherAttached())
		{
			strconcat(sizeof(temp), temp, *CStringTable().translate("st_detach_addon"), " ", *CStringTable().translate(pSettings->r_string(*pWeapon->GetGrenadeLauncherName(), "inv_name")));
			m_pUIPropertiesBox->AddItem(temp, NULL, INVENTORY_DETACH_GRENADE_LAUNCHER_ADDON);
			b_show = true;
		}
		if (pWeapon->ScopeAttachable() && pWeapon->IsScopeAttached())
		{
			strconcat(sizeof(temp), temp, *CStringTable().translate("st_detach_addon"), " ", *CStringTable().translate(pSettings->r_string(*pWeapon->GetScopeName(), "inv_name")));
			m_pUIPropertiesBox->AddItem(temp, NULL, INVENTORY_DETACH_SCOPE_ADDON);
			b_show = true;
		}
		if (pWeapon->SilencerAttachable() && pWeapon->IsSilencerAttached())
		{
			strconcat(sizeof(temp), temp, *CStringTable().translate("st_detach_addon"), " ", *CStringTable().translate(pSettings->r_string(*pWeapon->GetSilencerName(), "inv_name")));
			m_pUIPropertiesBox->AddItem(temp, NULL, INVENTORY_DETACH_SILENCER_ADDON);
			b_show = true;
		}

		bool b = (0 != pWeapon->GetAmmoElapsed() || pWeapon->HasDetachableMagazine() && pWeapon->IsMagazineAttached());

		if (m_pOurObject->inventory().InSlot(pWeapon))
		{
			for (u8 i = 0; i < pWeapon->m_ammoTypes.size(); ++i)
			{
				if (m_pOurObject->inventory().GetAmmo(pWeapon->m_ammoTypes[i].c_str(), false))
				{
					strconcat(sizeof(temp), temp, *CStringTable().translate("st_load_ammo_type"), " ",
						*CStringTable().translate(pSettings->r_string(pWeapon->m_ammoTypes[i].c_str(), "inv_name_short")));
					m_pUIPropertiesBox->AddItem(temp, (void*)i, INVENTORY_RELOAD_MAGAZINE);
					b_show = true;
				}
			};
		}

		if (!b)
		{
			CUICellItem * itm = CurrentItem();
			for (u32 i = 0; i<itm->ChildsCount(); ++i)
			{
				pWeapon = smart_cast<CWeaponMagazined*>((CWeapon*)itm->Child(i)->m_pData);
				if (pWeapon->GetAmmoElapsed())
				{
					b = true;
					break;
				}
			}
		}

		if (b) {
			const char *unload_text = pWeapon->GetGrenadeMode() ? "st_unload_gl" : "st_unload";
			m_pUIPropertiesBox->AddItem(unload_text, NULL, INVENTORY_UNLOAD_MAGAZINE);
			b_show = true;
		}
	}

	if(pMedkit || pAntirad)
	{
		_action						= "st_use";
		b_show						= true;
	}
	else if(pEatableItem)
	{
		if(pBottleItem)
			_action					= "st_drink";
		else
			_action					= "st_eat";
		b_show						= true;
	}
	if(_action)
		m_pUIPropertiesBox->AddItem(_action,  NULL, INVENTORY_EAT_ACTION);

	//пункты меню "переместить"
	b_show = true;
	//
	bool many_items_in_cell = CurrentItem()->ChildsCount() > 0;
	//
	if ((pWarBelt || pBackPack) && owner == m_pUIOurBagList && m_pOurObject->inventory().InSlot(CurrentIItem()))
	{
		m_pUIPropertiesBox->AddItem("st_move_with_content", NULL, INVENTORY_MOVE_WITH_CONTENT);
	}
	else
	{
		m_pUIPropertiesBox->AddItem("st_move", NULL, INVENTORY_MOVE_ACTION); //переместить один предмет
		if (many_items_in_cell) //предметов в ячейке больше одного
			m_pUIPropertiesBox->AddItem("st_move_all", (void*)33, INVENTORY_MOVE_ACTION); //переместить стак предметов
	}
	//
	//if (!m_pInventoryBox)
	{
		m_pUIPropertiesBox->AddItem("st_drop", NULL, INVENTORY_DROP_ACTION);

		if (many_items_in_cell)
			m_pUIPropertiesBox->AddItem("st_drop_all", (void*)33, INVENTORY_DROP_ACTION);
	}

	if(b_show)
	{
		m_pUIPropertiesBox->AutoUpdateSize	();
		m_pUIPropertiesBox->BringAllToTop	();

		Fvector2						cursor_pos;
		Frect							vis_rect;

		GetAbsoluteRect					(vis_rect);
		cursor_pos						= GetUICursor()->GetCursorPosition();
		cursor_pos.sub					(vis_rect.lt);
		m_pUIPropertiesBox->Show		(vis_rect, cursor_pos);
		PlaySnd(eInvProperties);
	}
}

void CUICarBodyWnd::EatItem()
{
	CActor *pActor				= smart_cast<CActor*>(Level().CurrentEntity());
	if(!pActor)					return;

	NET_Packet					P;
	CGameObject::u_EventGen		(P, GEG_PLAYER_ITEM_EAT, Actor()->ID());
	P.w_u16						(CurrentIItem()->object().ID());
	CGameObject::u_EventSend	(P);

	PlaySnd(eInvItemUse);

	UpdateLists_delayed			();
}

bool CUICarBodyWnd::OnItemDrop(CUICellItem* itm)
{
	CUIDragDropListEx*	old_owner		= itm->OwnerList();
	CUIDragDropListEx*	new_owner		= CUIDragDropListEx::m_drag_item->BackList();
	
	if (old_owner == new_owner || !old_owner || !new_owner)
		return true;
	return MoveItemsFromCell(itm, false);
}

bool CUICarBodyWnd::OnItemStartDrag(CUICellItem* itm)
{
	return				false; //default behaviour
}

#include "../xr_3da/xr_input.h"
bool CUICarBodyWnd::OnItemDbClick(CUICellItem* itm)
{
	return MoveItemsFromCell(itm, b_TakeAllActionKeyHolded);
}

bool CUICarBodyWnd::OnItemSelected(CUICellItem* itm)
{
	SetCurrentItem		(itm);
	itm->ColorizeWeapon({ m_pUIOurBagList, m_pUIOthersBagList });
	return				false;
}

bool CUICarBodyWnd::OnItemRButtonClick(CUICellItem* itm)
{
	SetCurrentItem				(itm);
	ActivatePropertiesBox		();
	return						false;
}

void move_item (u16 from_id, u16 to_id, u16 what_id)
{
	NET_Packet P;
	CGameObject::u_EventGen					(	P,
												GE_OWNERSHIP_REJECT,
												from_id
											);

	P.w_u16									(what_id);
	CGameObject::u_EventSend				(P);

	//другому инвентарю - взять вещь 
	CGameObject::u_EventGen					(	P,
												GE_OWNERSHIP_TAKE,
												to_id
											);
	P.w_u16									(what_id);
	CGameObject::u_EventSend				(P);

}

bool CUICarBodyWnd::TransferItem(PIItem itm, CInventoryOwner* owner_from, CInventoryOwner* owner_to, bool b_check)
{
	VERIFY									(NULL==m_pInventoryBox);
	CGameObject* go_from					= smart_cast<CGameObject*>(owner_from);
	CGameObject* go_to						= smart_cast<CGameObject*>(owner_to);

	if(smart_cast<CBaseMonster*>(go_to))	return false;
	if(b_check)
	{
		float invWeight						= owner_to->inventory().CalcTotalWeight();
		float maxWeight						= owner_to->inventory().GetMaxWeight();
		float itmWeight						= itm->Weight();
		if(invWeight+itmWeight >=maxWeight)	return false;
	}
	//
	CWeaponKnife* pKnife = smart_cast<CWeaponKnife*>(/*pActor*/m_pOurObject->inventory().ActiveItem());
	CBaseMonster* pMonster = smart_cast<CBaseMonster*>(go_from);
	if (g_eFreeHands != eFreeHandsOff && pMonster)      //если мы забираем что-то из инвентаря монстра в режиме "свободных рук"
	{
		if (pKnife)                                                       //убедимся что оружие в активном слоте - нож
		{
			pKnife->Fire2Start();                                         //нанесём удар ножом
			itm->ChangeCondition( -(1 - pKnife->GetCondition()) );        //уменьшим Condition части монстра на величину износа ножа (1 - Knife->GetCondition())
			pKnife->ChangeCondition(-pKnife->GetCondDecPerShotOnHit() * pMonster->m_fSkinDensityK); //уменьшим Condition ножа кол-во ударов * износ за удар
		}
	}
	//
	if (owner_from == m_pOurObject)
		itm->OnMoveOut(itm->m_eItemPlace);

	move_item(go_from->ID(), go_to->ID(), itm->object().ID());

	return true;
}

void CUICarBodyWnd::BindDragDropListEvents(CUIDragDropListEx* lst)
{
	lst->m_f_item_drop				= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUICarBodyWnd::OnItemDrop);
	lst->m_f_item_start_drag		= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUICarBodyWnd::OnItemStartDrag);
	lst->m_f_item_db_click			= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUICarBodyWnd::OnItemDbClick);
	lst->m_f_item_selected			= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUICarBodyWnd::OnItemSelected);
	lst->m_f_item_rbutton_click		= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUICarBodyWnd::OnItemRButtonClick);
}

void CUICarBodyWnd::MoveItemWithContent(CUICellItem* itm, u32 slot)
{
	EItemPlace move_from = eItemPlaceUndefined;

	switch (slot)
	{
	case WARBELT_SLOT:
		move_from = eItemPlaceBelt;
		break;
	case BACKPACK_SLOT:
		move_from = eItemPlaceRuck;
		break;
	}

	auto inv = m_pOurObject->inventory();

	for (TIItemContainer::iterator it = inv.m_all.begin(); inv.m_all.end() != it; ++it)
	{
		PIItem iitem = *it;

		if (iitem->m_eItemPlace == move_from)
		{
			if (m_pOthersObject)
				TransferItem(iitem, m_pOurObject, m_pOthersObject, false);
			else
				move_item(0, m_pInventoryBox->ID(), iitem->object().ID());
		}
	}

	if(itm) MoveItemsFromCell(itm, false);		
	PlaySnd(eInvMoveItem);
}

void CUICarBodyWnd::PlaySnd(eInventorySndAction a)
{
	if (sounds[a]._handle())
		sounds[a].play(NULL, sm_2D);
}