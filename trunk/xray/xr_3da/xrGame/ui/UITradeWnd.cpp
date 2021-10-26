#include "stdafx.h"
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

	BindDragDropListEnents				(&m_uidata->UIOurBagList);
	BindDragDropListEnents				(&m_uidata->UIOthersBagList);
	BindDragDropListEnents				(&m_uidata->UIOurTradeList);
	BindDragDropListEnents				(&m_uidata->UIOthersTradeList);
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
	if (!g_actor->GetPDA())
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
			CUICellItem * itm = CurrentItem();
			CWeapon* pWeapon = smart_cast<CWeapon*>(CurrentIItem());
			//
			switch (m_pUIPropertiesBox->GetClickedItem()->GetTAG())
			{
				//
			case INVENTORY_MOVE_ACTION:
			{
				void* d = m_pUIPropertiesBox->GetClickedItem()->GetData();
				bool b_all = (d == (void*)33);

				if (b_all)
					MoveAllFromCell(itm);	//переместить стак предметов
				else
					MoveOneFromCell(itm);	//переместить один предмет
			}break;
				//
			case INVENTORY_DETECTOR_CHECK_ACTION:
				m_uidata->UIItemInfo.UIArtefactParams->Show(true);
				m_uidata->UIItemInfo.TryAddArtefactInfo(CurrentIItem()->object().cNameSect());
				break;
			case INVENTORY_RELOAD_MAGAZINE:
				pWeapon->Action(kWPN_RELOAD, CMD_START);
				SetCurrentItem(NULL);
				break;
			case INVENTORY_SWITCH_GRENADE_LAUNCHER_MODE:
				pWeapon->Action(kWPN_FUNC, CMD_START);
				SetCurrentItem(NULL);
				break;
			case INVENTORY_NEXT_AMMO_TYPE:
				pWeapon->Action(kWPN_NEXT, CMD_START);
				break;
			case INVENTORY_UNLOAD_MAGAZINE:
			{
				//CUICellItem * itm = CurrentItem();
				(smart_cast<CWeaponMagazined*>((CWeapon*)itm->m_pData))->UnloadMagazine();
				for (u32 i = 0; i<itm->ChildsCount(); ++i)
				{
					CUICellItem * child_itm = itm->Child(i);
					(smart_cast<CWeaponMagazined*>((CWeapon*)child_itm->m_pData))->UnloadMagazine();
				}
			}break;
			case INVENTORY_DETACH_SCOPE_ADDON:
			{
				pWeapon->Detach(pWeapon->GetScopeName().c_str(), true);
			}break;
			case INVENTORY_DETACH_SILENCER_ADDON:
			{
				pWeapon->Detach(pWeapon->GetSilencerName().c_str(), true);
			}break;
			case INVENTORY_DETACH_GRENADE_LAUNCHER_ADDON:
			{
				pWeapon->Detach(pWeapon->GetGrenadeLauncherName().c_str(), true);
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

void CUITradeWnd::Update()
{
	EListType et					= eNone;

	if(m_pInv->ModifyFrame()==Device.dwFrame && m_pOthersInv->ModifyFrame()==Device.dwFrame){
		et = eBoth;
	}else if(m_pInv->ModifyFrame()==Device.dwFrame){
		et = e1st;
	}else if(m_pOthersInv->ModifyFrame()==Device.dwFrame){
		et = e2nd;
	}
	if(et!=eNone)
		UpdateLists					(et);

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
		/*HUD().GetUI()->UIGame()->RemoveCustomStatic("not_enough_money_mine");
		HUD().GetUI()->UIGame()->RemoveCustomStatic("not_enough_money_other");*/
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
	}else
	{
		/*if(others_money<0)
			m_uidata->UIDealMsg		= HUD().GetUI()->UIGame()->AddCustomStatic("not_enough_money_other", true);
		else
			m_uidata->UIDealMsg		= HUD().GetUI()->UIGame()->AddCustomStatic("not_enough_money_mine", true);*/
		//
		string256				deal_refuse_text; //строка с текстом сообщения-отказа при невозмжности совершить торговую сделку
		//условия для формирования текста
		LPCSTR                  trader_name = others_money < 0 ? m_pOthersInvOwner->Name() : m_pInvOwner->Name(); //от чьего имени выдаётся сообщение
		STRING_ID               refusal_text = g_actor->GetPDA() ? "st_not_enough_money_to_trade" : "st_not_enough_money_to_barter"; //текст сообщения отказа в зависимости от торговля/бартер
		//показываем статик с текстом отказа
		m_uidata->UIDealMsg = HUD().GetUI()->UIGame()->AddCustomStatic("not_enough_money", true); //показать статик
		strconcat(sizeof(deal_refuse_text), deal_refuse_text, trader_name, ": ", *CStringTable().translate(refusal_text)); //сформировать текст
		m_uidata->UIDealMsg->wnd()->SetText(deal_refuse_text); //задать текст статику
		//
		m_uidata->UIDealMsg->m_endTime	= Device.fTimeGlobal+2.0f;// sec
	}
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
	/*else*/ if (m_pOthersInvOwner->InfinitiveMoney() || (!m_pOthersInvOwner->InfinitiveMoney() && !g_actor->GetPDA())) //закроем --- счетчик денег контрагента, если в режиме бартера
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
		pTrade->TransferItem	((PIItem)itm->m_pData, bBuying);
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
	itm->ColorizeWeapon	(&m_uidata->UIOurTradeList, &m_uidata->UIOthersTradeList, &m_uidata->UIOurBagList, &m_uidata->UIOthersBagList);
	return				false;
}

#include "../xr_level_controller.h"

bool CUITradeWnd::OnKeyboard(int dik, EUIMessages keyboard_action)
{
	if (m_pUIPropertiesBox->GetVisible())
		m_pUIPropertiesBox->OnKeyboard(dik, keyboard_action);

	if (inherited::OnKeyboard(dik, keyboard_action))return true;
	//
	b_TakeAllActionKeyHolded = keyboard_action == WINDOW_KEY_PRESSED && is_binded(kCROUCH, dik);

	return false;
}

bool CUITradeWnd::OnMouse(float x, float y, EUIMessages mouse_action)
{
	////вызов дополнительного меню по правой кнопке
	if (mouse_action == WINDOW_RBUTTON_DOWN)
	{
		if (m_pUIPropertiesBox->IsShown())
		{
			m_pUIPropertiesBox->Hide();
			return						true;
		}
	}

	CUIWindow::OnMouse(x, y, mouse_action);

	return true; // always returns true, because ::StopAnyMove() == true;
}

#include "../weaponmagazinedwgrenade.h"
#include "../Artifact.h"
#include "../string_table.h"
void CUITradeWnd::ActivatePropertiesBox()
{
	m_pUIPropertiesBox->RemoveAll();
	//
	CActor *pActor = smart_cast<CActor*>(Level().CurrentEntity());

	CWeaponMagazined*		pWeapon = smart_cast<CWeaponMagazined*> (CurrentIItem());
	CWeaponMagazinedWGrenade*	pWeaponMagWGren = smart_cast<CWeaponMagazinedWGrenade*>(CurrentIItem());
	CArtefact*		        pArtefact = smart_cast<CArtefact*>		(CurrentIItem());
	bool					b_show = false;
	string1024 temp;
	//
	CUIDragDropListEx*	owner = CurrentItem()->OwnerList();

	if (psActorFlags.test(AF_ARTEFACT_DETECTOR_CHECK) && pArtefact && g_actor->GetDetector() && !m_uidata->UIItemInfo.UIArtefactParams->IsShown())
	{
		m_pUIPropertiesBox->AddItem("st_detector_check", NULL, INVENTORY_DETECTOR_CHECK_ACTION);
		b_show = true;
	}

	if (pWeapon && owner == &m_uidata->UIOurBagList)
	{
		if (pWeapon->IsGrenadeLauncherAttached())
		{
			const char *switch_gl_text = pWeaponMagWGren->m_bGrenadeMode ? "st_deactivate_gl" : "st_activate_gl";
			if (pActor->inventory().InSlot(pWeapon))
				m_pUIPropertiesBox->AddItem(switch_gl_text, NULL, INVENTORY_SWITCH_GRENADE_LAUNCHER_MODE);
			b_show = true;
		}
		if (pWeapon->GrenadeLauncherAttachable() && pWeapon->IsGrenadeLauncherAttached())
		{
			//m_pUIPropertiesBox->AddItem("st_detach_gl", NULL, INVENTORY_DETACH_GRENADE_LAUNCHER_ADDON);
			strconcat(sizeof(temp), temp, *CStringTable().translate("st_detach_addon"), " ", *CStringTable().translate(pSettings->r_string(*pWeapon->GetGrenadeLauncherName(), "inv_name")));
			m_pUIPropertiesBox->AddItem(temp, NULL, INVENTORY_DETACH_GRENADE_LAUNCHER_ADDON);
			b_show = true;
		}
		if (pWeapon->ScopeAttachable() && pWeapon->IsScopeAttached())
		{
			//m_pUIPropertiesBox->AddItem("st_detach_scope", NULL, INVENTORY_DETACH_SCOPE_ADDON);
			strconcat(sizeof(temp), temp, *CStringTable().translate("st_detach_addon"), " ", *CStringTable().translate(pSettings->r_string(*pWeapon->GetScopeName(), "inv_name")));
			m_pUIPropertiesBox->AddItem(temp, NULL, INVENTORY_DETACH_SCOPE_ADDON);
			b_show = true;
		}
		if (pWeapon->SilencerAttachable() && pWeapon->IsSilencerAttached())
		{
			//m_pUIPropertiesBox->AddItem("st_detach_silencer", NULL, INVENTORY_DETACH_SILENCER_ADDON);
			strconcat(sizeof(temp), temp, *CStringTable().translate("st_detach_addon"), " ", *CStringTable().translate(pSettings->r_string(*pWeapon->GetSilencerName(), "inv_name")));
			m_pUIPropertiesBox->AddItem(temp, NULL, INVENTORY_DETACH_SILENCER_ADDON);
			b_show = true;
		}

		bool b = (0 != pWeapon->GetAmmoElapsed());

		if (pActor->inventory().InSlot(pWeapon) && pWeapon->GetAmmoElapsed() < pWeapon->GetAmmoMagSize() && pWeapon->IsAmmoAvailable()) //перезарядить контекстным меню можно только оружие в слоте
		{
			const char *reload_text = pWeaponMagWGren ? (pWeaponMagWGren->m_bGrenadeMode ? "st_reload_magazine_gl" : "st_reload_magazine") : "st_reload_magazine";
			m_pUIPropertiesBox->AddItem(reload_text, NULL, INVENTORY_RELOAD_MAGAZINE);
			b_show = true;
		}


		if (pActor->inventory().InSlot(pWeapon) && pWeapon->IsAmmoAvailable()) //перезарядить контекстным меню можно только оружие в слоте
		{
			if (pWeapon->HasNextAmmoType())
			{
				m_pUIPropertiesBox->AddItem("st_next_ammo_type", NULL, INVENTORY_NEXT_AMMO_TYPE);
				b_show = true;
			}
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

		if (b) 
		{
			const char *unload_text = pWeaponMagWGren ? (pWeaponMagWGren->m_bGrenadeMode ? "st_unload_magazine_gl" : "st_unload_magazine") : "st_unload_magazine";
			m_pUIPropertiesBox->AddItem(unload_text, NULL, INVENTORY_UNLOAD_MAGAZINE);
			b_show = true;
		}
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

	/*if(old_owner==&m_uidata->UIOurBagList && new_owner==&m_uidata->UIOurTradeList)
		ToOurTrade				();
	else if(old_owner==&m_uidata->UIOurTradeList && new_owner==&m_uidata->UIOurBagList)
		ToOurBag				();
	else if(old_owner==&m_uidata->UIOthersBagList && new_owner==&m_uidata->UIOthersTradeList)
		ToOthersTrade			();
	else if(old_owner==&m_uidata->UIOthersTradeList && new_owner==&m_uidata->UIOthersBagList)
		ToOthersBag				();

	return true;*/
	return MoveOneFromCell(itm);
}

bool CUITradeWnd::OnItemDbClick(CUICellItem* itm)
{
	SetCurrentItem						(itm);
	/*CUIDragDropListEx*	old_owner		= itm->OwnerList();
	
	if(old_owner == &m_uidata->UIOurBagList)
		ToOurTrade				();
	else if(old_owner == &m_uidata->UIOurTradeList)
		ToOurBag				();
	else if(old_owner == &m_uidata->UIOthersBagList)
		ToOthersTrade			();
	else if(old_owner == &m_uidata->UIOthersTradeList)
		ToOthersBag				();
	else
		R_ASSERT2(false, "wrong parent for cell item");

	return true;*/
	//
	return b_TakeAllActionKeyHolded ? MoveAllFromCell(itm) : MoveOneFromCell(itm);
}

bool CUITradeWnd::MoveOneFromCell(CUICellItem* itm)
{
	CUIDragDropListEx*	old_owner		= itm->OwnerList();

	if(old_owner == &m_uidata->UIOurBagList)
	ToOurTrade				();
	else if(old_owner == &m_uidata->UIOurTradeList)
	ToOurBag				();
	else if(old_owner == &m_uidata->UIOthersBagList)
	ToOthersTrade			();
	else if(old_owner == &m_uidata->UIOthersTradeList)
	ToOthersBag				();
	else
	R_ASSERT2(false, "wrong parent for cell item");

	return true;
}

bool CUITradeWnd::MoveAllFromCell(CUICellItem* itm)
{
	//CUICellItem* cur_item = CurrentItem();

	//if (!cur_item) return false;
	
	//u32 cnt = cur_item->ChildsCount();
	CUIDragDropListEx* old_owner = itm->OwnerList();
	//CUIDragDropListEx* to;

	/*if (old_owner == &m_uidata->UIOurBagList)
		to = &m_uidata->UIOurTradeList;
	else if (old_owner == &m_uidata->UIOurTradeList)
		to = &m_uidata->UIOurBagList;
	else if (old_owner == &m_uidata->UIOthersBagList)
		to = &m_uidata->UIOthersTradeList;
	else if (old_owner == &m_uidata->UIOthersTradeList)
		to = &m_uidata->UIOthersBagList;*/

	u32 cnt = itm->ChildsCount();
	for (u32 i = 0; i < cnt; ++i)
	{
//		CUICellItem* itm = itm->PopChild();

		//to->SetItem(itm);
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
	
	//return MoveOneFromCell(itm);
	return MoveOneFromCell(itm);
	SetCurrentItem(NULL);
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

void CUITradeWnd::BindDragDropListEnents(CUIDragDropListEx* lst)
{
	lst->m_f_item_drop				= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUITradeWnd::OnItemDrop);
	lst->m_f_item_start_drag		= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUITradeWnd::OnItemStartDrag);
	lst->m_f_item_db_click			= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUITradeWnd::OnItemDbClick);
	lst->m_f_item_selected			= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUITradeWnd::OnItemSelected);
	lst->m_f_item_rbutton_click		= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUITradeWnd::OnItemRButtonClick);
}