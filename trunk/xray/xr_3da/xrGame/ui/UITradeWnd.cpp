#include "stdafx.h"
#include "../xr_3da/xrGame/StdAfx.h"
#include "UITradeWnd.h"

#include "xrUIXmlParser.h"
#include "UIXmlInit.h"

#include "../Entity.h"
#include "../HUDManager.h"
#include "../WeaponAmmo.h"
#include "../WeaponMagazined.h"
#include "../Actor.h"
#include "../Trade.h"
#include "../UIGameSP.h"
#include "UIInventoryUtilities.h"
#include "../inventoryowner.h"
#include "../eatable_item.h"
#include "../inventory.h"
#include "../level.h"
#include "../string_table.h"
#include "../character_info.h"
#include "UIMultiTextStatic.h"
#include "UI3tButton.h"
#include "UIItemInfo.h"

#include "UICharacterInfo.h"
#include "UIDragDropListEx.h"
#include "UICellItem.h"
#include "UICellItemFactory.h"
//
#include "UIPropertiesBox.h"
#include "UIListBoxItem.h"
#include "../xr_3da/xr_input.h"


#define				TRADE_XML			"trade.xml"
#define				TRADE_CHARACTER_XML	"trade_character.xml"
#define				TRADE_ITEM_XML		"trade_item.xml"


struct CUITradeInternal{
	CUIStatic			UIStaticTop;
	CUIStatic			UIStaticBottom;

	CUIStatic			UIOurBagWnd;
	CUIStatic			UIOurMoneyStatic;
	CUIStatic			UIOthersBagWnd;
	CUIStatic			UIOtherMoneyStatic;
	CUIDragDropListEx	UIOurBagList;
	CUIDragDropListEx	UIOthersBagList;

	CUIStatic			UIOurTradeWnd;
	CUIStatic			UIOthersTradeWnd;
	CUIMultiTextStatic	UIOurPriceCaption;
	CUIMultiTextStatic	UIOthersPriceCaption;
	CUIDragDropListEx	UIOurTradeList;
	CUIDragDropListEx	UIOthersTradeList;

	//кнопки
	CUI3tButton			UIPerformTradeButton;
	CUI3tButton			UIToTalkButton;

	//информация о персонажах 
	CUIStatic			UIOurIcon;
	CUIStatic			UIOthersIcon;
	CUICharacterInfo	UICharacterInfoLeft;
	CUICharacterInfo	UICharacterInfoRight;

	//информация о перетаскиваемом предмете
	CUIStatic			UIDescWnd;
	CUIItemInfo			UIItemInfo;

	SDrawStaticStruct*	UIDealMsg;
};

CUITradeWnd::CUITradeWnd()
	:	m_bDealControlsVisible	(false),
		m_pTrade(NULL),
		m_pOthersTrade(NULL),
		bStarted(false)
{
	m_uidata = xr_new<CUITradeInternal>();
	Init();
	Hide();
	SetCurrentItem			(NULL);
	m_b_need_update			= false;
}

CUITradeWnd::~CUITradeWnd()
{
	m_uidata->UIOurBagList.ClearAll		(true);
	m_uidata->UIOurTradeList.ClearAll	(true);
	m_uidata->UIOthersBagList.ClearAll	(true);
	m_uidata->UIOthersTradeList.ClearAll(true);
	xr_delete							(m_uidata);
}

void CUITradeWnd::Init()
{
	CUIXml								uiXml;
	bool xml_result						= uiXml.Init(CONFIG_PATH, UI_PATH, TRADE_XML);
	R_ASSERT3							(xml_result, "xml file not found", TRADE_XML);
	CUIXmlInit							xml_init;

	xml_init.InitWindow					(uiXml, "main", 0, this);

	//статические элементы интерфейса
	AttachChild							(&m_uidata->UIStaticTop);
	xml_init.InitStatic					(uiXml, "top_background", 0, &m_uidata->UIStaticTop);
	AttachChild							(&m_uidata->UIStaticBottom);
	xml_init.InitStatic					(uiXml, "bottom_background", 0, &m_uidata->UIStaticBottom);

	//иконки с изображение нас и партнера по торговле
	AttachChild							(&m_uidata->UIOurIcon);
	xml_init.InitStatic					(uiXml, "static_icon", 0, &m_uidata->UIOurIcon);
	AttachChild							(&m_uidata->UIOthersIcon);
	xml_init.InitStatic					(uiXml, "static_icon", 1, &m_uidata->UIOthersIcon);
	m_uidata->UIOurIcon.AttachChild		(&m_uidata->UICharacterInfoLeft);
	m_uidata->UICharacterInfoLeft.Init	(0,0, m_uidata->UIOurIcon.GetWidth(), m_uidata->UIOurIcon.GetHeight(), TRADE_CHARACTER_XML);
	m_uidata->UIOthersIcon.AttachChild	(&m_uidata->UICharacterInfoRight);
	m_uidata->UICharacterInfoRight.Init	(0,0, m_uidata->UIOthersIcon.GetWidth(), m_uidata->UIOthersIcon.GetHeight(), TRADE_CHARACTER_XML);


	//Списки торговли
	AttachChild							(&m_uidata->UIOurBagWnd);
	xml_init.InitStatic					(uiXml, "our_bag_static", 0, &m_uidata->UIOurBagWnd);
	AttachChild							(&m_uidata->UIOthersBagWnd);
	xml_init.InitStatic					(uiXml, "others_bag_static", 0, &m_uidata->UIOthersBagWnd);

	m_uidata->UIOurBagWnd.AttachChild	(&m_uidata->UIOurMoneyStatic);
	xml_init.InitStatic					(uiXml, "our_money_static", 0, &m_uidata->UIOurMoneyStatic);

	m_uidata->UIOthersBagWnd.AttachChild(&m_uidata->UIOtherMoneyStatic);
	xml_init.InitStatic					(uiXml, "other_money_static", 0, &m_uidata->UIOtherMoneyStatic);

	AttachChild							(&m_uidata->UIOurTradeWnd);
	xml_init.InitStatic					(uiXml, "static", 0, &m_uidata->UIOurTradeWnd);
	AttachChild							(&m_uidata->UIOthersTradeWnd);
	xml_init.InitStatic					(uiXml, "static", 1, &m_uidata->UIOthersTradeWnd);

	m_uidata->UIOurTradeWnd.AttachChild	(&m_uidata->UIOurPriceCaption);
	xml_init.InitMultiTextStatic		(uiXml, "price_mt_static", 0, &m_uidata->UIOurPriceCaption);

	m_uidata->UIOthersTradeWnd.AttachChild(&m_uidata->UIOthersPriceCaption);
	xml_init.InitMultiTextStatic		(uiXml, "price_mt_static", 0, &m_uidata->UIOthersPriceCaption);

	//Списки Drag&Drop
	m_uidata->UIOurBagWnd.AttachChild	(&m_uidata->UIOurBagList);	
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_list", 0, &m_uidata->UIOurBagList);

	m_uidata->UIOthersBagWnd.AttachChild(&m_uidata->UIOthersBagList);	
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_list", 1, &m_uidata->UIOthersBagList);

	m_uidata->UIOurTradeWnd.AttachChild	(&m_uidata->UIOurTradeList);	
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_list", 2, &m_uidata->UIOurTradeList);

	m_uidata->UIOthersTradeWnd.AttachChild(&m_uidata->UIOthersTradeList);	
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_list", 3, &m_uidata->UIOthersTradeList);

	
	AttachChild							(&m_uidata->UIDescWnd);
	xml_init.InitStatic					(uiXml, "desc_static", 0, &m_uidata->UIDescWnd);
	m_uidata->UIDescWnd.AttachChild		(&m_uidata->UIItemInfo);
	m_uidata->UIItemInfo.Init			(0,0, m_uidata->UIDescWnd.GetWidth(), m_uidata->UIDescWnd.GetHeight(), TRADE_ITEM_XML);


	xml_init.InitAutoStatic				(uiXml, "auto_static", this);


	AttachChild							(&m_uidata->UIPerformTradeButton);
	xml_init.Init3tButton					(uiXml, "button", 0, &m_uidata->UIPerformTradeButton);

	AttachChild							(&m_uidata->UIToTalkButton);
	xml_init.Init3tButton					(uiXml, "button", 1, &m_uidata->UIToTalkButton);
	//
	m_pUIPropertiesBox                  = xr_new<CUIPropertiesBox>(); m_pUIPropertiesBox->SetAutoDelete(true);
	AttachChild                         (m_pUIPropertiesBox);
	m_pUIPropertiesBox->Init            (0, 0, 300, 300);
	m_pUIPropertiesBox->Hide            ();

	m_uidata->UIDealMsg					= NULL;

	BindDragDropListEvents				(&m_uidata->UIOurBagList);
	BindDragDropListEvents				(&m_uidata->UIOthersBagList);
	BindDragDropListEvents				(&m_uidata->UIOurTradeList);
	BindDragDropListEvents				(&m_uidata->UIOthersTradeList);

	//Load sounds
	if (uiXml.NavigateToNode("action_sounds", 0))
	{
		XML_NODE* stored_root = uiXml.GetLocalRoot();
		uiXml.SetLocalRoot(uiXml.NavigateToNode("action_sounds", 0));

		::Sound->create(sounds[eInvSndOpen],		uiXml.Read("snd_open",			0, NULL), st_Effect, sg_SourceType);
		::Sound->create(sounds[eInvProperties],		uiXml.Read("snd_properties",	0, NULL), st_Effect, sg_SourceType);
		::Sound->create(sounds[eInvDropItem],		uiXml.Read("snd_drop_item",		0, NULL), st_Effect, sg_SourceType);
		::Sound->create(sounds[eInvMoveItem],		uiXml.Read("snd_move_item",		0, NULL), st_Effect, sg_SourceType);
		::Sound->create(sounds[eInvDetachAddon],	uiXml.Read("snd_detach_addon",	0, NULL), st_Effect, sg_SourceType);
		::Sound->create(sounds[eInvItemUse],		uiXml.Read("snd_item_use",		0, NULL), st_Effect, sg_SourceType);

		uiXml.SetLocalRoot(stored_root);
	}
}

void CUITradeWnd::InitTrade(CInventoryOwner* pOur, CInventoryOwner* pOthers)
{
	VERIFY								(pOur);
	VERIFY								(pOthers);

	m_pInvOwner							= pOur;
	m_pOthersInvOwner					= pOthers;
	m_uidata->UIOthersPriceCaption.GetPhraseByIndex(0)->SetText(*CStringTable().translate("ui_st_opponent_items"));

	m_uidata->UICharacterInfoLeft.InitCharacter(m_pInvOwner->object_id());
	m_uidata->UICharacterInfoRight.InitCharacter(m_pOthersInvOwner->object_id());

	m_pInv								= &m_pInvOwner->inventory();
	m_pOthersInv						= pOur->GetTrade()->GetPartnerInventory();
		
	m_pTrade							= pOur->GetTrade();
	m_pOthersTrade						= pOur->GetTrade()->GetPartnerTrade();
	//
	m_pUIPropertiesBox->Hide();
	//

	EnableAll							();

	UpdateLists							(eBoth);

// режим бартерной торговли
	if (!/*g_actor*/m_pInvOwner->GetPDA())
	{
		m_uidata->UIOurMoneyStatic.SetText(*CStringTable().translate("ui_st_pda_account_unavailable"));   //закроем статиком кол-во денег актора, т.к. оно еще не обновилось и не ноль
		m_uidata->UIOtherMoneyStatic.SetText(*CStringTable().translate("ui_st_pda_account_unavailable")); //закроем статиком кол-во денег контрагента, т.к. оно еще не обновилось и не ---
		m_uidata->UIPerformTradeButton.SetText(*CStringTable().translate("ui_st_barter")); //напишем "бартер" на кнопке, вместо "торговать"
	}
	else
		m_uidata->UIPerformTradeButton.SetText(*CStringTable().translate("ui_st_trade")); //вернём надпись "торговать" на кнопке, вместо "бартер"
//
}  

#include "ui_af_params.h"
void CUITradeWnd::SendMessage(CUIWindow *pWnd, s16 msg, void *pData)
{
	if(pWnd == &m_uidata->UIToTalkButton && msg == BUTTON_CLICKED)
	{
		SwitchToTalk();
	}
	else if(pWnd == &m_uidata->UIPerformTradeButton && msg == BUTTON_CLICKED)
	{
		PerformTrade();
	}
	//
	else if (pWnd == m_pUIPropertiesBox && msg == PROPERTY_CLICKED)
	{
		if (m_pUIPropertiesBox->GetClickedItem())
		{
			CUICellItem *	itm		= CurrentItem();
			CWeapon*		pWeapon = smart_cast<CWeapon*>		(CurrentIItem());
			CWeaponAmmo*	pAmmo	= smart_cast<CWeaponAmmo*>	(CurrentIItem());
			//
			switch (m_pUIPropertiesBox->GetClickedItem()->GetTAG())
			{
				//
			case INVENTORY_EAT_ACTION:	//съесть объект
				EatItem();
				SetCurrentItem(NULL);
				break;
				//
			case INVENTORY_DROP_ACTION:
			{
				void* d = m_pUIPropertiesBox->GetClickedItem()->GetData();
				bool b_all = (d == (void*)33);

				DropItemsfromCell(b_all);
			}break;
			case INVENTORY_MOVE_ACTION:
			{
				void* d = m_pUIPropertiesBox->GetClickedItem()->GetData();
				bool b_all = (d == (void*)33);
				MoveItemsFromCell(itm, b_all);
			}break;
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
				for (u32 i = 0; i<itm->ChildsCount(); ++i)
				{
					CUICellItem * child_itm = itm->Child(i);
					(smart_cast<CWeaponMagazined*>((CWeapon*)child_itm->m_pData))->UnloadMagazine();
					(smart_cast<CWeaponMagazined*>((CWeapon*)child_itm->m_pData))->PullShutter();
				}
			}break;
			case INVENTORY_RELOAD_AMMO_BOX:
			{
				pAmmo->ReloadBox((LPCSTR)m_pUIPropertiesBox->GetClickedItem()->GetData());
				SetCurrentItem(NULL);
				UpdateLists(e1st);
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
			}
		}
	}

	CUIWindow::SendMessage(pWnd, msg, pData);
}

void CUITradeWnd::Draw()
{
	inherited::Draw				();
	if(m_uidata->UIDealMsg)		m_uidata->UIDealMsg->Draw();
	//Msg("Draw");
}

extern void UpdateCameraDirection(CGameObject* pTo);

void CUITradeWnd::UpdateLists_delayed()
{
	m_b_need_update = true;
}

void CUITradeWnd::Update()
{
	EListType et					= eNone;
	
	if (m_pInv->ModifyFrame() == Device.dwFrame && m_pOthersInv->ModifyFrame() == Device.dwFrame || m_b_need_update){
		et = eBoth;
	}else if(m_pInv->ModifyFrame()==Device.dwFrame){
		et = e1st;
	}else if(m_pOthersInv->ModifyFrame()==Device.dwFrame){
		et = e2nd;
	}
	if(et!=eNone)
		UpdateLists					(et);

	UpdatePrices();

	inherited::Update				();
	UpdateCameraDirection			(smart_cast<CGameObject*>(m_pOthersInvOwner));

	if(m_uidata->UIDealMsg){
		m_uidata->UIDealMsg->Update();
		if( !m_uidata->UIDealMsg->IsActual()){
			/*HUD().GetUI()->UIGame()->RemoveCustomStatic("not_enough_money_mine");
			HUD().GetUI()->UIGame()->RemoveCustomStatic("not_enough_money_other");*/
			HUD().GetUI()->UIGame()->RemoveCustomStatic("not_enough_money");
			m_uidata->UIDealMsg			= NULL;
		}
	}
}

#include "UIInventoryUtilities.h"
void CUITradeWnd::Show()
{
	InventoryUtilities::SendInfoToActor("ui_trade");
	inherited::Show					(true);
	inherited::Enable				(true);

	SetCurrentItem					(NULL);
	ResetAll						();
	m_uidata->UIDealMsg				= NULL;
	//
	CActor *pActor = smart_cast<CActor*>(Level().CurrentEntity());
	if (pActor && psActorFlags.test(AF_AMMO_FROM_BELT)) pActor->inventory().m_bRuckAmmoPlacement = true; //установим флаг перезарядки из рюкзака
	PlaySnd(eInvSndOpen);
}

void CUITradeWnd::Hide()
{
	InventoryUtilities::SendInfoToActor("ui_trade_hide");
	inherited::Show					(false);
	inherited::Enable				(false);
	if(bStarted)
		StopTrade					();

	m_uidata->UIDealMsg				= NULL;

	if(HUD().GetUI()->UIGame()){
		HUD().GetUI()->UIGame()->RemoveCustomStatic("not_enough_money");
	}

	m_uidata->UIOurBagList.ClearAll		(true);
	m_uidata->UIOurTradeList.ClearAll	(true);
	m_uidata->UIOthersBagList.ClearAll	(true);
	m_uidata->UIOthersTradeList.ClearAll(true);
	//
	CActor *pActor = smart_cast<CActor*>(Level().CurrentEntity());
	if (pActor && psActorFlags.test(AF_AMMO_FROM_BELT)) pActor->inventory().m_bRuckAmmoPlacement = false; //сбросим флаг перезарядки из рюкзака
}

void CUITradeWnd::StartTrade()
{
	if (m_pTrade)					m_pTrade->TradeCB(true);
	if (m_pOthersTrade)				m_pOthersTrade->TradeCB(true);
	bStarted						= true;
}

void CUITradeWnd::StopTrade()
{
	if (m_pTrade)					m_pTrade->TradeCB(false);
	if (m_pOthersTrade)				m_pOthersTrade->TradeCB(false);
	bStarted						= false;
}

#include "../trade_parameters.h"
bool CUITradeWnd::CanMoveToOther(PIItem pItem)
{

	float r1				= CalcItemsWeight(&m_uidata->UIOurTradeList);	// our
	float r2				= CalcItemsWeight(&m_uidata->UIOthersTradeList);	// other

	float itmWeight			= pItem->Weight();
	float otherInvWeight	= m_pOthersInv->CalcTotalWeight();
	float otherMaxWeight	= m_pOthersInv->GetMaxWeight();

	if (!m_pOthersInvOwner->trade_parameters().enabled(
			CTradeParameters::action_buy(0),
			pItem->object().cNameSect()
		))
		return				(false);

	if(otherInvWeight-r2+r1+itmWeight > otherMaxWeight)
		return				false;

	return true;
}

void move_item(CUICellItem* itm, CUIDragDropListEx* from, CUIDragDropListEx* to)
{
	CUICellItem* _itm		= from->RemoveItem	(itm, false);
	to->SetItem				(_itm);
}

bool CUITradeWnd::ToOurTrade()
{
	if (!CanMoveToOther(CurrentIItem()))	return false;

	move_item				(CurrentItem(), &m_uidata->UIOurBagList, &m_uidata->UIOurTradeList);
	UpdatePrices			();
	return					true;
}

bool CUITradeWnd::ToOthersTrade()
{
	move_item				(CurrentItem(), &m_uidata->UIOthersBagList, &m_uidata->UIOthersTradeList);
	UpdatePrices			();

	return					true;
}

bool CUITradeWnd::ToOurBag()
{
	move_item				(CurrentItem(), &m_uidata->UIOurTradeList, &m_uidata->UIOurBagList);
	UpdatePrices			();
	
	return					true;
}

bool CUITradeWnd::ToOthersBag()
{
	move_item				(CurrentItem(), &m_uidata->UIOthersTradeList, &m_uidata->UIOthersBagList);
	UpdatePrices			();

	return					true;
}

float CUITradeWnd::CalcItemsWeight(CUIDragDropListEx* pList)
{
	float res = 0.0f;

	for(u32 i=0; i<pList->ItemsCount(); ++i)
	{
		CUICellItem* itm	= pList->GetItemIdx	(i);
		PIItem	iitem		= (PIItem)itm->m_pData;
		res					+= iitem->Weight();
		for(u32 j=0; j<itm->ChildsCount(); ++j){
			PIItem	jitem		= (PIItem)itm->Child(j)->m_pData;
			res					+= jitem->Weight();
		}
	}
	return res;
}

u32 CUITradeWnd::CalcItemsPrice(CUIDragDropListEx* pList, CTrade* pTrade, bool bBuying)
{
	u32 iPrice				= 0;
	
	for(u32 i=0; i<pList->ItemsCount(); ++i)
	{
		CUICellItem* itm	= pList->GetItemIdx(i);
		PIItem iitem		= (PIItem)itm->m_pData;
		iPrice				+= pTrade->GetItemPrice(iitem, bBuying);

		for(u32 j=0; j<itm->ChildsCount(); ++j){
			PIItem jitem	= (PIItem)itm->Child(j)->m_pData;
			iPrice			+= pTrade->GetItemPrice(jitem, bBuying);
		}

	}

	return					iPrice;
}

void CUITradeWnd::PerformTrade()
{

	if (m_uidata->UIOurTradeList.ItemsCount()==0 && m_uidata->UIOthersTradeList.ItemsCount()==0) 
		return;

	int our_money			= (int)m_pInvOwner->get_money();
	int others_money		= (int)m_pOthersInvOwner->get_money();

	int delta_price			= int(m_iOurTradePrice-m_iOthersTradePrice);

	our_money				+= delta_price;
	others_money			-= delta_price;

	if(our_money>=0 && others_money>=0 && (m_iOurTradePrice>=0 || m_iOthersTradePrice>0))
	{
		m_pOthersTrade->OnPerformTrade(m_iOthersTradePrice, m_iOurTradePrice);
		
		TransferItems		(&m_uidata->UIOurTradeList,		&m_uidata->UIOthersBagList, m_pOthersTrade,	true);
		TransferItems		(&m_uidata->UIOthersTradeList,	&m_uidata->UIOurBagList,	m_pOthersTrade,	false);
	}
	else
	{
		string256				deal_refuse_text; //строка с текстом сообщения-отказа при невозмжности совершить торговую сделку
		//условия для формирования текста
		LPCSTR                  trader_name = others_money < 0 ? m_pOthersInvOwner->Name() : m_pInvOwner->Name(); //от чьего имени выдаётся сообщение
		STRING_ID               refusal_text = /*g_actor*/m_pInvOwner->GetPDA() ? "st_not_enough_money_to_trade" : "st_not_enough_money_to_barter"; //текст сообщения отказа в зависимости от торговля/бартер
		//показываем статик с текстом отказа
		m_uidata->UIDealMsg = HUD().GetUI()->UIGame()->AddCustomStatic("not_enough_money", true); //показать статик
		strconcat(sizeof(deal_refuse_text), deal_refuse_text, trader_name, ": ", *CStringTable().translate(refusal_text)); //сформировать текст
		m_uidata->UIDealMsg->wnd()->SetText(deal_refuse_text); //задать текст статику
		//
		m_uidata->UIDealMsg->m_endTime	= Device.fTimeGlobal+2.0f;// sec
	}
	PlaySnd(eInvMoveItem);
	SetCurrentItem			(NULL);
}

void CUITradeWnd::DisableAll()
{
	m_uidata->UIOurBagWnd.Enable			(false);
	m_uidata->UIOthersBagWnd.Enable			(false);
	m_uidata->UIOurTradeWnd.Enable			(false);
	m_uidata->UIOthersTradeWnd.Enable		(false);
}

void CUITradeWnd::EnableAll()
{
	m_uidata->UIOurBagWnd.Enable			(true);
	m_uidata->UIOthersBagWnd.Enable			(true);
	m_uidata->UIOurTradeWnd.Enable			(true);
	m_uidata->UIOthersTradeWnd.Enable		(true);
}

void CUITradeWnd::UpdatePrices()
{
	m_iOurTradePrice	= CalcItemsPrice	(&m_uidata->UIOurTradeList,		m_pOthersTrade, true);
	m_iOthersTradePrice = CalcItemsPrice	(&m_uidata->UIOthersTradeList,	m_pOthersTrade, false);


	string256				buf;
	sprintf_s					(buf, "%d %s", m_iOurTradePrice,*CStringTable().translate("ui_st_money_regional"));
	m_uidata->UIOurPriceCaption.GetPhraseByIndex(2)->str = buf;
	sprintf_s					(buf, "%d %s", m_iOthersTradePrice,*CStringTable().translate("ui_st_money_regional"));
	m_uidata->UIOthersPriceCaption.GetPhraseByIndex(2)->str = buf;

	sprintf_s					(buf, "%d %s", m_pInvOwner->get_money(),*CStringTable().translate("ui_st_money_regional"));
	m_uidata->UIOurMoneyStatic.SetText(buf);

	if(!m_pOthersInvOwner->InfinitiveMoney()){
		sprintf_s					(buf, "%d %s", m_pOthersInvOwner->get_money(),*CStringTable().translate("ui_st_money_regional"));
		m_uidata->UIOtherMoneyStatic.SetText(buf);
	}
	//
	/*else*/ if (m_pOthersInvOwner->InfinitiveMoney() || (!m_pOthersInvOwner->InfinitiveMoney() && !/*g_actor*/m_pInvOwner->GetPDA())) //закроем --- счетчик денег контрагента, если в режиме бартера
	{
		m_uidata->UIOtherMoneyStatic.SetText("---");
	}
}

void CUITradeWnd::TransferItems(CUIDragDropListEx* pSellList,
								CUIDragDropListEx* pBuyList,
								CTrade* pTrade,
								bool bBuying)
{
	while(pSellList->ItemsCount())
	{
		CUICellItem* itm	=	pSellList->RemoveItem(pSellList->GetItemIdx(0),false);
		PIItem	iitm		= (PIItem)itm->m_pData;
		if (!bBuying)		iitm->OnMoveOut(iitm->m_eItemPlace);
		pTrade->TransferItem	(iitm, bBuying);
		pBuyList->SetItem		(itm);
	}

	pTrade->pThis.inv_owner->set_money ( pTrade->pThis.inv_owner->get_money(), true );
	pTrade->pPartner.inv_owner->set_money( pTrade->pPartner.inv_owner->get_money(), true );
}

void CUITradeWnd::UpdateLists(EListType mode)
{
	if(mode==eBoth||mode==e1st){
		m_uidata->UIOurBagList.ClearAll(true);
		m_uidata->UIOurTradeList.ClearAll(true);
	}

	if(mode==eBoth||mode==e2nd){
		m_uidata->UIOthersBagList.ClearAll(true);
		m_uidata->UIOthersTradeList.ClearAll(true);
	}

	UpdatePrices						();


	if(mode==eBoth||mode==e1st){
		ruck_list.clear					();
   		m_pInv->AddAvailableItems		(ruck_list, true);
		std::sort						(ruck_list.begin(),ruck_list.end(),InventoryUtilities::GreaterRoomInRuck);
		FillList						(ruck_list, m_uidata->UIOurBagList, true);
	}

	if(mode==eBoth||mode==e2nd){
		ruck_list.clear					();
		m_pOthersInv->AddAvailableItems	(ruck_list, true);
		std::sort						(ruck_list.begin(),ruck_list.end(),InventoryUtilities::GreaterRoomInRuck);
		FillList						(ruck_list, m_uidata->UIOthersBagList, false);
	}

	m_b_need_update = false;
}

void CUITradeWnd::FillList	(TIItemContainer& cont, CUIDragDropListEx& dragDropList, bool do_colorize)
{
	TIItemContainer::iterator it	= cont.begin();
	TIItemContainer::iterator it_e	= cont.end();

	for(; it != it_e; ++it) 
	{
		CUICellItem* itm			= create_cell_item	(*it);
		if (do_colorize)			itm->ColorizeEquipped(itm, CanMoveToOther(*it));
		dragDropList.SetItem		(itm);
	}

}

bool CUITradeWnd::OnItemStartDrag(CUICellItem* itm)
{
	return false; //default behaviour
}

bool CUITradeWnd::OnItemSelected(CUICellItem* itm)
{
	SetCurrentItem		(itm);
	itm->ColorizeWeapon({ &m_uidata->UIOurTradeList, &m_uidata->UIOthersTradeList, &m_uidata->UIOurBagList, &m_uidata->UIOthersBagList });
	return				false;
}

#include "../xr_level_controller.h"

bool CUITradeWnd::OnKeyboard(int dik, EUIMessages keyboard_action)
{
	if (m_b_need_update)
		return true;

	if (m_pUIPropertiesBox->GetVisible())
		m_pUIPropertiesBox->OnKeyboard(dik, keyboard_action);

	if (keyboard_action == WINDOW_KEY_PRESSED)
	{
		// здійснити торгівельну операцію по натисканню kCHECKACTIVEITEM
		if (is_binded(kCHECKACTIVEITEM, dik))
		{
			PerformTrade();
			return true;
		}
		// перейти до діалогу по натисканню kQUIT
		if (is_binded(kQUIT, dik))
		{
			SwitchToTalk();
			return true;
		}
	}
	//
	if (inherited::OnKeyboard(dik, keyboard_action))return true;
	//
	b_TakeAllActionKeyHolded = keyboard_action == WINDOW_KEY_PRESSED && is_binded(kCROUCH, dik);

	return false;
}

bool CUITradeWnd::OnMouse(float x, float y, EUIMessages mouse_action)
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

#include "../Artifact.h"
#include "../Medkit.h"
#include "../Antirad.h"
#include "../BottleItem.h"
#include "../string_table.h"
void CUITradeWnd::ActivatePropertiesBox()
{
	m_pUIPropertiesBox->RemoveAll();
	//
	//CActor *pActor = smart_cast<CActor*>(Level().CurrentEntity());

	auto pWeapon		= smart_cast<CWeaponMagazined*>			(CurrentIItem());
	auto pArtefact		= smart_cast<CArtefact*>				(CurrentIItem());
	auto pAmmo			= smart_cast<CWeaponAmmo*>				(CurrentIItem());
	auto pMedkit		= smart_cast<CMedkit*>					(CurrentIItem());
	auto pAntirad		= smart_cast<CAntirad*>					(CurrentIItem());
	auto pBottleItem	= smart_cast<CBottleItem*>				(CurrentIItem());
	auto pEatableItem	= smart_cast<CEatableItem*>				(CurrentIItem());

	bool						b_show			= false;

	string1024 temp;
	//
	CUIDragDropListEx*	owner = CurrentItem()->OwnerList();

	if (owner == &m_uidata->UIOurBagList)
	{
		if (pAmmo)
		{
			LPCSTR _ammo_sect;

			if (pAmmo->IsBoxReloadable())
			{
				//unload AmmoBox
				m_pUIPropertiesBox->AddItem("st_unload", NULL, INVENTORY_UNLOAD_AMMO_BOX);
				b_show = true;
				//reload AmmoBox
				if (pAmmo->m_boxCurr < pAmmo->m_boxSize)
				{
					if (m_pInv->GetAmmo(*pAmmo->m_ammoSect, true))
					{
						strconcat(sizeof(temp), temp, *CStringTable().translate("st_load_ammo_type"), " ",
							*CStringTable().translate(pSettings->r_string(pAmmo->m_ammoSect, "inv_name_short")));
						_ammo_sect = *pAmmo->m_ammoSect;
						m_pUIPropertiesBox->AddItem(temp, (void*)_ammo_sect, INVENTORY_RELOAD_AMMO_BOX);
						b_show = true;
					}
				}
			}
			else if (pAmmo->IsBoxReloadableEmpty())
			{
				for (u8 i = 0; i < pAmmo->m_ammoTypes.size(); ++i)
				{
					if (m_pInv->GetAmmo(*pAmmo->m_ammoTypes[i], true))
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
				if (m_pInv->InSlot(pWeapon))
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

			if (m_pInv->InSlot(pWeapon))
			{
				for (u8 i = 0; i < pWeapon->m_ammoTypes.size(); ++i)
				{
					if (m_pInv->GetAmmo(pWeapon->m_ammoTypes[i].c_str(), false))
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
				for (u32 i = 0; i < itm->ChildsCount(); ++i)
				{
					pWeapon = smart_cast<CWeaponMagazined*>((CWeapon*)itm->Child(i)->m_pData);
					if (pWeapon->GetAmmoElapsed())
					{
						b = true;
						break;
					}
				}
			}

			if (b)
			{
				const char *unload_text = pWeapon->GetGrenadeMode() ? "st_unload_gl" : "st_unload";
				m_pUIPropertiesBox->AddItem(unload_text, NULL, INVENTORY_UNLOAD_MAGAZINE);
				b_show = true;
			}

		}

		LPCSTR _action = nullptr;
		if (pMedkit || pAntirad)
		{
			_action = "st_use";
			b_show = true;
		}
		else if (pEatableItem)
		{
			if (pBottleItem)
				_action = "st_drink";
			else
				_action = "st_eat";
			b_show = true;
		}
		if (_action)
			m_pUIPropertiesBox->AddItem(_action, NULL, INVENTORY_EAT_ACTION);

		m_pUIPropertiesBox->AddItem("st_drop", NULL, INVENTORY_DROP_ACTION);
		b_show = true;
		bool many_items_in_cell = CurrentItem()->ChildsCount() > 0;
		if (many_items_in_cell)
			m_pUIPropertiesBox->AddItem("st_drop_all", (void*)33, INVENTORY_DROP_ACTION);
	}

	//
	//b_show = true;
	//
	if (CanMoveToOther(CurrentIItem()))
	{
		bool many_items_in_cell = CurrentItem()->ChildsCount() > 0;

		m_pUIPropertiesBox->AddItem("st_move", NULL, INVENTORY_MOVE_ACTION); //переместить один предмет
		b_show = true;

		if (many_items_in_cell) //предметов в ячейке больше одного
			m_pUIPropertiesBox->AddItem("st_move_all", (void*)33, INVENTORY_MOVE_ACTION); //переместить стак предметов
	}
	//
	if (b_show)
	{
		m_pUIPropertiesBox->AutoUpdateSize();
		m_pUIPropertiesBox->BringAllToTop();

		Fvector2						cursor_pos;
		Frect							vis_rect;

		GetAbsoluteRect(vis_rect);
		cursor_pos = GetUICursor()->GetCursorPosition();
		cursor_pos.sub(vis_rect.lt);
		m_pUIPropertiesBox->Show(vis_rect, cursor_pos);
		PlaySnd(eInvProperties);
	}
}

bool CUITradeWnd::OnItemRButtonClick(CUICellItem* itm)
{
	SetCurrentItem				(itm);
	ActivatePropertiesBox       ();
	return						false;
}

bool CUITradeWnd::OnItemDrop(CUICellItem* itm)
{
	CUIDragDropListEx*	old_owner		= itm->OwnerList();
	CUIDragDropListEx*	new_owner		= CUIDragDropListEx::m_drag_item->BackList();
	if(old_owner==new_owner || !old_owner || !new_owner)
					return false;

	return MoveItemsFromCell(itm, false);
}

bool CUITradeWnd::OnItemDbClick(CUICellItem* itm)
{
	SetCurrentItem						(itm);
	return MoveItemsFromCell(itm, b_TakeAllActionKeyHolded);
}

bool CUITradeWnd::MoveItemsFromCell(CUICellItem* itm, bool b_all)
{
	CUIDragDropListEx* old_owner = itm->OwnerList();

	u32 cnt = itm->ChildsCount();
	for (u32 i = 0; i < cnt && b_all; ++i)
	{
		if (old_owner == &m_uidata->UIOurBagList)
			ToOurTrade();
		else if (old_owner == &m_uidata->UIOurTradeList)
			ToOurBag();
		else if (old_owner == &m_uidata->UIOthersBagList)
			ToOthersTrade();
		else if (old_owner == &m_uidata->UIOthersTradeList)
			ToOthersBag();
		else
			R_ASSERT2(false, "wrong parent for cell item");
	}
	
	if (old_owner == &m_uidata->UIOurBagList)
		ToOurTrade();
	else if (old_owner == &m_uidata->UIOurTradeList)
		ToOurBag();
	else if (old_owner == &m_uidata->UIOthersBagList)
		ToOthersTrade();
	else if (old_owner == &m_uidata->UIOthersTradeList)
		ToOthersBag();
	else
		R_ASSERT2(false, "wrong parent for cell item");
	PlaySnd(eInvMoveItem);
	return true;
}

CUICellItem* CUITradeWnd::CurrentItem()
{
	return m_pCurrentCellItem;
}

PIItem CUITradeWnd::CurrentIItem()
{
	return	(m_pCurrentCellItem)?(PIItem)m_pCurrentCellItem->m_pData : NULL;
}

void CUITradeWnd::SetCurrentItem(CUICellItem* itm)
{
	if(m_pCurrentCellItem == itm) return;
	m_pCurrentCellItem				= itm;
	m_uidata->UIItemInfo.InitItem	(CurrentIItem());
	
	if(!m_pCurrentCellItem)		return;

	CUIDragDropListEx* owner	= itm->OwnerList();
	bool bBuying				= (owner==&m_uidata->UIOurBagList) || (owner==&m_uidata->UIOurTradeList);

	if(itm && m_uidata->UIItemInfo.UICost){

		string256			str;

		sprintf_s				(str, "%d %s", m_pOthersTrade->GetItemPrice(CurrentIItem(), bBuying),*CStringTable().translate("ui_st_money_regional") );
		m_uidata->UIItemInfo.UICost->SetText (str);
	}
}

void CUITradeWnd::SwitchToTalk()
{
	GetMessageTarget()->SendMessage		(this, TRADE_WND_CLOSED);
}

void CUITradeWnd::BindDragDropListEvents(CUIDragDropListEx* lst)
{
	lst->m_f_item_drop				= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUITradeWnd::OnItemDrop);
	lst->m_f_item_start_drag		= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUITradeWnd::OnItemStartDrag);
	lst->m_f_item_db_click			= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUITradeWnd::OnItemDbClick);
	lst->m_f_item_selected			= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUITradeWnd::OnItemSelected);
	lst->m_f_item_rbutton_click		= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUITradeWnd::OnItemRButtonClick);
}

void CUITradeWnd::DropItemsfromCell(bool b_all)
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

void CUITradeWnd::SendEvent_Item_Drop(PIItem	pItem)
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

void CUITradeWnd::PlaySnd(eInventorySndAction a)
{
	if (sounds[a]._handle())
		sounds[a].play(NULL, sm_2D);
}

void CUITradeWnd::EatItem()
{
	CActor *pActor = smart_cast<CActor*>(Level().CurrentEntity());
	if (!pActor)					return;

	NET_Packet					P;
	CGameObject::u_EventGen(P, GEG_PLAYER_ITEM_EAT, Actor()->ID());
	P.w_u16(CurrentIItem()->object().ID());
	CGameObject::u_EventSend(P);

	PlaySnd(eInvItemUse);

	UpdateLists_delayed();
}