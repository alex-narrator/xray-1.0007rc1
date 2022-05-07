#include "pch_script.h"
#include "stdafx.h"
#include "UIInventoryWnd.h"

#include "xrUIXmlParser.h"
#include "UIXmlInit.h"
#include "../string_table.h"

#include "../actor.h"
#include "../uigamesp.h"
#include "../hudmanager.h"

#include "../CustomOutfit.h"
#include "../CustomDetector.h"

#include "../weapon.h"

#include "../script_process.h"

#include "../eatable_item.h"
#include "../inventory.h"

#include "UIInventoryUtilities.h"
using namespace InventoryUtilities;


#include "../InfoPortion.h"
#include "../level.h"
#include "../game_base_space.h"
#include "../entitycondition.h"

#include "../game_cl_base.h"
#include "../ActorCondition.h"
#include "UIDragDropListEx.h"
#include "UIOutfitSlot.h"
#include "UI3tButton.h"
#include "../../../build_config_defines.h"
#include "../smart_cast.h"

#define				INVENTORY_ITEM_XML		"inventory_item.xml"
#define				INVENTORY_XML			"inventory_new.xml"



CUIInventoryWnd*	g_pInvWnd = NULL;

CUIInventoryWnd::CUIInventoryWnd()
{
	m_iCurrentActiveSlot				= NO_ACTIVE_SLOT;
	UIRank								= NULL;
	Init								();
	SetCurrentItem						(NULL);

	g_pInvWnd							= this;	
	m_b_need_reinit						= false;
	Hide								();	
}

void CUIInventoryWnd::Init()
{
	CUIXml								uiXml;
	bool xml_result						= uiXml.Init(CONFIG_PATH, UI_PATH, INVENTORY_XML);
	R_ASSERT3							(xml_result, "file parsing error ", uiXml.m_xml_file_name);

	CUIXmlInit							xml_init;

	xml_init.InitWindow					(uiXml, "main", 0, this);

	AttachChild							(&UIBeltSlots);
	xml_init.InitStatic					(uiXml, "belt_slots", 0, &UIBeltSlots);

	AttachChild							(&UIBack);
	xml_init.InitStatic					(uiXml, "back", 0, &UIBack);

	AttachChild							(&UIStaticBottom);
	xml_init.InitStatic					(uiXml, "bottom_static", 0, &UIStaticBottom);

	AttachChild							(&UIBagWnd);
	xml_init.InitStatic					(uiXml, "bag_static", 0, &UIBagWnd);

	AttachChild							(&UIVolumeWnd);
	xml_init.InitStatic					(uiXml, "volume_static", 0, &UIVolumeWnd);
	
	AttachChild							(&UIMoneyWnd);
	xml_init.InitStatic					(uiXml, "money_static", 0, &UIMoneyWnd);

	AttachChild							(&UIDescrWnd);
	xml_init.InitStatic					(uiXml, "descr_static", 0, &UIDescrWnd);


	UIDescrWnd.AttachChild				(&UIItemInfo);
	UIItemInfo.Init						(0, 0, UIDescrWnd.GetWidth(), UIDescrWnd.GetHeight(), INVENTORY_ITEM_XML);
	
#ifdef INV_NEW_SLOTS_SYSTEM
	if (GameID() == GAME_SINGLE){
		AttachChild							(&UISleepWnd);
		UISleepWnd.Init();
		UISleepWnd.SetWindowName("sleep_wnd");  // для лучшего нахождения через GetStatic
	}
#endif
	
	AttachChild							(&UIPersonalWnd);
	xml_init.InitFrameWindow			(uiXml, "character_frame_window", 0, &UIPersonalWnd);

	AttachChild							(&UIProgressBack);
	xml_init.InitStatic					(uiXml, "progress_background", 0, &UIProgressBack);

	AttachChild							(&UIProgressBackRadiation);
	xml_init.InitStatic					(uiXml, "progress_background_radiation", 0, &UIProgressBackRadiation);

	if (GameID() != GAME_SINGLE){
		AttachChild						(&UIProgressBack_rank);
		xml_init.InitStatic				(uiXml, "progress_back_rank", 0, &UIProgressBack_rank);

		UIProgressBack_rank.AttachChild	(&UIProgressBarRank);
		xml_init.InitProgressBar		(uiXml, "progress_bar_rank", 0, &UIProgressBarRank);
		UIProgressBarRank.SetProgressPos(100);

	}
	

	UIProgressBack.AttachChild (&UIProgressBarHealth);
	xml_init.InitProgressBar (uiXml, "progress_bar_health", 0, &UIProgressBarHealth);
	
	UIProgressBack.AttachChild	(&UIProgressBarPsyHealth);
	xml_init.InitProgressBar (uiXml, "progress_bar_psy", 0, &UIProgressBarPsyHealth);

	UIProgressBackRadiation.AttachChild(&UIProgressBarRadiation);
	xml_init.InitProgressBar (uiXml, "progress_bar_radiation", 0, &UIProgressBarRadiation);

#ifdef INV_NEW_SLOTS_SYSTEM
	if (GameID() == GAME_SINGLE){
		UIProgressBack.AttachChild	(&UIProgressBarSatiety);
		xml_init.InitProgressBar (uiXml, "progress_bar_satiety", 0, &UIProgressBarSatiety);
	}
#endif

	UIPersonalWnd.AttachChild			(&UIStaticPersonal);
	xml_init.InitStatic					(uiXml, "static_personal",0, &UIStaticPersonal);
//	UIStaticPersonal.Init				(1, UIPersonalWnd.GetHeight() - 175, 260, 260);

	AttachChild							(&UIOutfitInfo);
	UIOutfitInfo.InitFromXml			(uiXml);
//.	xml_init.InitStatic					(uiXml, "outfit_info_window",0, &UIOutfitInfo);

	//Элементы автоматического добавления
	xml_init.InitAutoStatic				(uiXml, "auto_static", this);


	if (GameID() != GAME_SINGLE){
		UIRankFrame = xr_new<CUIStatic> (); UIRankFrame->SetAutoDelete(true);
		UIRank = xr_new<CUIStatic> (); UIRank->SetAutoDelete(true);

		CUIXmlInit::InitStatic(uiXml, "rank", 0, UIRankFrame);
		CUIXmlInit::InitStatic(uiXml, "rank:pic", 0, UIRank);
		AttachChild(UIRankFrame);
		UIRankFrame->AttachChild(UIRank);		
	}

	m_pUIBagList						= xr_new<CUIDragDropListEx>(); UIBagWnd.AttachChild(m_pUIBagList); m_pUIBagList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_bag", 0, m_pUIBagList);
	BindDragDropListEvents				(m_pUIBagList);

	m_pUIBeltList						= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIBeltList); m_pUIBeltList->SetAutoDelete(true);
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_belt", 0, m_pUIBeltList);
	BindDragDropListEvents				(m_pUIBeltList);

#if defined(INV_OUTFIT_FULL_ICON_HIDE)
	m_pUIOutfitList						= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIOutfitList); m_pUIOutfitList->SetAutoDelete(true);
#else
	m_pUIOutfitList						= xr_new<CUIOutfitDragDropList>(); AttachChild(m_pUIOutfitList); m_pUIOutfitList->SetAutoDelete(true);
#endif
	xml_init.InitDragDropListEx			(uiXml, "dragdrop_outfit", 0, m_pUIOutfitList);
	BindDragDropListEvents				(m_pUIOutfitList);
	ZeroMemory(&m_slots_array, sizeof(m_slots_array));

#ifdef INV_NEW_SLOTS_SYSTEM	
	m_pUIPistolList = xr_new<CUIDragDropListEx>(); AttachChild(m_pUIPistolList); m_pUIPistolList->SetAutoDelete(true);
	xml_init.InitDragDropListEx(uiXml, "dragdrop_slot_weapon_1", 0, m_pUIPistolList);
	BindDragDropListEvents				(m_pUIPistolList);

	m_pUIAutomaticList = xr_new<CUIDragDropListEx>(); AttachChild(m_pUIAutomaticList); m_pUIAutomaticList->SetAutoDelete(true);
	xml_init.InitDragDropListEx(uiXml, "dragdrop_slot_weapon_2", 0, m_pUIAutomaticList);
	BindDragDropListEvents				(m_pUIAutomaticList);

	if (GameID() == GAME_SINGLE){
		m_pUIKnifeList						= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIKnifeList); m_pUIKnifeList->SetAutoDelete(true);
		xml_init.InitDragDropListEx			(uiXml, "dragdrop_slot_weapon_0", 0, m_pUIKnifeList);
		BindDragDropListEvents				(m_pUIKnifeList);	
		
		m_pUIBinocularList					= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIBinocularList); m_pUIBinocularList->SetAutoDelete(true);
		xml_init.InitDragDropListEx			(uiXml, "dragdrop_slot_weapon_3", 0, m_pUIBinocularList);
		BindDragDropListEvents				(m_pUIBinocularList);
		
		m_pUIDetectorList					= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIDetectorList); m_pUIDetectorList->SetAutoDelete(true);
		xml_init.InitDragDropListEx			(uiXml, "dragdrop_slot_detector", 0, m_pUIDetectorList);
		BindDragDropListEvents				(m_pUIDetectorList);

		m_pUITorchList						= xr_new<CUIDragDropListEx>(); AttachChild(m_pUITorchList); m_pUITorchList->SetAutoDelete(true);
		xml_init.InitDragDropListEx			(uiXml, "dragdrop_slot_torch", 0, m_pUITorchList);
		BindDragDropListEvents				(m_pUITorchList);

		m_pUIPDAList						= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIPDAList); m_pUIPDAList->SetAutoDelete(true);
		xml_init.InitDragDropListEx			(uiXml, "dragdrop_slot_pda", 0, m_pUIPDAList);
		BindDragDropListEvents				(m_pUIPDAList);		
		
		m_pUIHelmetList						= xr_new<CUIDragDropListEx>(); AttachChild(m_pUIHelmetList); m_pUIHelmetList->SetAutoDelete(true);
		xml_init.InitDragDropListEx			(uiXml, "dragdrop_slot_helmet", 0, m_pUIHelmetList);
		BindDragDropListEvents				(m_pUIHelmetList);
		
		m_pUISlotQuickAccessList_0			= xr_new<CUIDragDropListEx>(); AttachChild(m_pUISlotQuickAccessList_0); m_pUISlotQuickAccessList_0->SetAutoDelete(true);
		xml_init.InitDragDropListEx			(uiXml, "dragdrop_slot_quick_access_0", 0, m_pUISlotQuickAccessList_0);
		BindDragDropListEvents				(m_pUISlotQuickAccessList_0);
		
		m_pUISlotQuickAccessList_1			= xr_new<CUIDragDropListEx>(); AttachChild(m_pUISlotQuickAccessList_1); m_pUISlotQuickAccessList_1->SetAutoDelete(true);
		xml_init.InitDragDropListEx			(uiXml, "dragdrop_slot_quick_access_1", 0, m_pUISlotQuickAccessList_1);
		BindDragDropListEvents				(m_pUISlotQuickAccessList_1);
		
		m_pUISlotQuickAccessList_2			= xr_new<CUIDragDropListEx>(); AttachChild(m_pUISlotQuickAccessList_2); m_pUISlotQuickAccessList_2->SetAutoDelete(true);
		xml_init.InitDragDropListEx			(uiXml, "dragdrop_slot_quick_access_2", 0, m_pUISlotQuickAccessList_2);
		BindDragDropListEvents				(m_pUISlotQuickAccessList_2);
		
		m_pUISlotQuickAccessList_3			= xr_new<CUIDragDropListEx>(); AttachChild(m_pUISlotQuickAccessList_3); m_pUISlotQuickAccessList_3->SetAutoDelete(true);
		xml_init.InitDragDropListEx			(uiXml, "dragdrop_slot_quick_access_3", 0, m_pUISlotQuickAccessList_3);
		BindDragDropListEvents				(m_pUISlotQuickAccessList_3);
		
		m_slots_array[KNIFE_SLOT]				= m_pUIKnifeList;
		m_slots_array[APPARATUS_SLOT]			= m_pUIBinocularList;
		m_slots_array[PDA_SLOT]					= m_pUIPDAList;
		m_slots_array[DETECTOR_SLOT]			= m_pUIDetectorList;
		m_slots_array[TORCH_SLOT]				= m_pUITorchList;
		m_slots_array[HELMET_SLOT]				= m_pUIHelmetList;
		m_slots_array[SLOT_QUICK_ACCESS_0]		= m_pUISlotQuickAccessList_0;
		m_slots_array[SLOT_QUICK_ACCESS_1]		= m_pUISlotQuickAccessList_1;
		m_slots_array[SLOT_QUICK_ACCESS_2]		= m_pUISlotQuickAccessList_2;
		m_slots_array[SLOT_QUICK_ACCESS_3]		= m_pUISlotQuickAccessList_3;
	}
#else
	m_pUIPistolList = xr_new<CUIDragDropListEx>(); AttachChild(m_pUIPistolList); m_pUIPistolList->SetAutoDelete(true);
	xml_init.InitDragDropListEx(uiXml, "dragdrop_pistol", 0, m_pUIPistolList);
	BindDragDropListEvents(m_pUIPistolList);

	m_pUIAutomaticList = xr_new<CUIDragDropListEx>(); AttachChild(m_pUIAutomaticList); m_pUIAutomaticList->SetAutoDelete(true);
	xml_init.InitDragDropListEx(uiXml, "dragdrop_automatic", 0, m_pUIAutomaticList);
	BindDragDropListEvents(m_pUIAutomaticList);
#endif	

	m_slots_array[PISTOL_SLOT]				= m_pUIPistolList;
	m_slots_array[RIFLE_SLOT]				= m_pUIAutomaticList;
	m_slots_array[OUTFIT_SLOT]				= m_pUIOutfitList;

	m_pUIGrenadeList = xr_new<CUIDragDropListEx>(); AttachChild(m_pUIGrenadeList); m_pUIGrenadeList->SetAutoDelete(true);
	xml_init.InitDragDropListEx(uiXml, "dragdrop_slot_grenade", 0, m_pUIGrenadeList);
	BindDragDropListEvents(m_pUIGrenadeList);
	m_slots_array[GRENADE_SLOT] = m_pUIGrenadeList;

	m_slots_array[BOLT_SLOT]				= NULL;		
#if defined(SHOW_ARTEFACT_SLOT)
	m_pUIArtefactList = xr_new<CUIDragDropListEx>(); AttachChild(m_pUIArtefactList); m_pUIArtefactList->SetAutoDelete(true);
	xml_init.InitDragDropListEx(uiXml, "dragdrop_slot_artefact", 0, m_pUIArtefactList);
	BindDragDropListEvents(m_pUIArtefactList);

	m_slots_array[ARTEFACT_SLOT] = m_pUIArtefactList;
#else
	m_slots_array[ARTEFACT_SLOT] = NULL;
#endif

	m_pUIWarBeltList = xr_new<CUIDragDropListEx>(); AttachChild(m_pUIWarBeltList); m_pUIWarBeltList->SetAutoDelete(true);
	xml_init.InitDragDropListEx(uiXml, "dragdrop_slot_warbelt", 0, m_pUIWarBeltList);
	BindDragDropListEvents(m_pUIWarBeltList);

	m_slots_array[WARBELT_SLOT] = m_pUIWarBeltList;

	m_pUIBackPackList = xr_new<CUIDragDropListEx>(); AttachChild(m_pUIBackPackList); m_pUIBackPackList->SetAutoDelete(true);
	xml_init.InitDragDropListEx(uiXml, "dragdrop_slot_backpack", 0, m_pUIBackPackList);
	BindDragDropListEvents(m_pUIBackPackList);

	m_slots_array[BACKPACK_SLOT] = m_pUIBackPackList;

	//pop-up menu
	AttachChild							(&UIPropertiesBox);
	UIPropertiesBox.Init				(0,0,300,300);
	UIPropertiesBox.Hide				();

	AttachChild							(&UIStaticTime);
	xml_init.InitStatic					(uiXml, "time_static", 0, &UIStaticTime);

	UIStaticTime.AttachChild			(&UIStaticTimeString);
	xml_init.InitStatic					(uiXml, "time_static_str", 0, &UIStaticTimeString);

	UIExitButton						= xr_new<CUI3tButton>();UIExitButton->SetAutoDelete(true);
	AttachChild							(UIExitButton);
	xml_init.Init3tButton				(uiXml, "exit_button", 0, UIExitButton);

	UIRepackAmmoButton					= xr_new<CUI3tButton>(); UIRepackAmmoButton->SetAutoDelete(true);
	AttachChild							(UIRepackAmmoButton);
	xml_init.Init3tButton				(uiXml, "repack_ammo_button", 0, UIRepackAmmoButton);

//Load sounds

	XML_NODE* stored_root				= uiXml.GetLocalRoot		();
	uiXml.SetLocalRoot					(uiXml.NavigateToNode		("action_sounds",0));
	::Sound->create						(sounds[eInvSndOpen],		uiXml.Read("snd_open",			0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvSndClose],		uiXml.Read("snd_close",			0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvItemToSlot],	uiXml.Read("snd_item_to_slot",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvItemToBelt],	uiXml.Read("snd_item_to_belt",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvItemToRuck],	uiXml.Read("snd_item_to_ruck",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvProperties],	uiXml.Read("snd_properties",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvDropItem],		uiXml.Read("snd_drop_item",		0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvAttachAddon],	uiXml.Read("snd_attach_addon",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvDetachAddon],	uiXml.Read("snd_detach_addon",	0,	NULL),st_Effect,sg_SourceType);
	::Sound->create						(sounds[eInvItemUse],		uiXml.Read("snd_item_use",		0,	NULL),st_Effect,sg_SourceType);

	uiXml.SetLocalRoot					(stored_root);
}

EListType CUIInventoryWnd::GetType(CUIDragDropListEx* l)
{
	if(l==m_pUIBagList)			return iwBag;
	if(l==m_pUIBeltList)		return iwBelt;

	for (u32 i = 0; i < SLOTS_TOTAL; i++)
		if (m_slots_array[i] == l)
			return iwSlot;

	NODEFAULT;
#ifdef DEBUG
	return iwSlot;
#endif // DEBUG
}

void CUIInventoryWnd::PlaySnd(eInventorySndAction a)
{
	if (sounds[a]._handle())
        sounds[a].play					(NULL, sm_2D);
}

CUIInventoryWnd::~CUIInventoryWnd()
{
//.	ClearDragDrop(m_vDragDropItems);
	ClearAllLists						();
}

bool CUIInventoryWnd::OnMouse(float x, float y, EUIMessages mouse_action)
{
	if (m_b_need_reinit)
		return true;

	//вызов дополнительного меню по правой кнопке
	if(mouse_action == WINDOW_RBUTTON_DOWN)
	{
		if(UIPropertiesBox.IsShown())
		{
			UIPropertiesBox.Hide		();
			return						true;
		}
	}

	if (UIPropertiesBox.IsShown())
	{
		if (mouse_action == WINDOW_MOUSE_WHEEL_DOWN || mouse_action == WINDOW_MOUSE_WHEEL_UP)
			return true;
	}

	CUIWindow::OnMouse					(x, y, mouse_action);

	return true; // always returns true, because ::StopAnyMove() == true;
}

void CUIInventoryWnd::Draw()
{
	CUIWindow::Draw						();
}

void CUIInventoryWnd::Update()
{
	if (m_b_need_reinit)
		InitInventory					();


	CEntityAlive *pEntityAlive			= smart_cast<CEntityAlive*>(Level().CurrentEntity());

	if(pEntityAlive) 
	{
		float v = pEntityAlive->conditions().GetHealth()*100.0f;
		UIProgressBarHealth.SetProgressPos		(v);

		v = pEntityAlive->conditions().GetPsyHealth()*100.0f;
		UIProgressBarPsyHealth.SetProgressPos	(v);


		v = pEntityAlive->conditions().GetRadiation()*100.0f;
		CActor *pActor = smart_cast<CActor*>(Level().CurrentEntity());
		auto DetectorInSlot = pActor->GetDetector();
		if (DetectorInSlot && DetectorInSlot->IsGeigerCounter()) //удаляем шкалу радиации для прогрессбара в инвентаре если не экипирован детектор -- NO_RAD_UI_WITHOUT_DETECTOR_IN_SLOT
		{
			UIProgressBackRadiation.Show(true);
			UIProgressBarRadiation.Show(true);
			UIProgressBarRadiation.SetProgressPos(v); 
		}
		else
		{
			UIProgressBackRadiation.Show(false);
		}

#ifdef INV_NEW_SLOTS_SYSTEM
		if (GameID() == GAME_SINGLE){
			CActor*	m_pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
			
			v =(m_pActor->conditions().GetSatiety())*100.0f;
			UIProgressBarSatiety.SetProgressPos	(v);
		}
#endif

		CInventoryOwner* pOurInvOwner	= smart_cast<CInventoryOwner*>(pEntityAlive);
		u32 _money						= 0;

		if (GameID() != GAME_SINGLE){
			game_PlayerState* ps = Game().GetPlayerByGameID(pEntityAlive->ID());
			if (ps){
				UIProgressBarRank.SetProgressPos(ps->experience_D*100);
				_money							= ps->money_for_round;
			}
		}else
		{
			_money							= pOurInvOwner->get_money();
		}
		// update money
		string64						sMoney;
		//red_virus
		sprintf_s						(sMoney,"%d %s", _money, *CStringTable().translate("ui_st_money_regional"));
		//
		if (g_actor->GetPDA())
		    UIMoneyWnd.SetText				(sMoney);
		else
			UIMoneyWnd.SetText              (*CStringTable().translate("ui_st_pda_account_unavailable"));

		// update outfit parameters
		UIOutfitInfo.Update();

	}

	InventoryUtilities::UpdateWeight(UIBagWnd, true);
	UIVolumeWnd.SetVisible(!!psActorFlags.is(AF_INVENTORY_VOLUME));
	InventoryUtilities::UpdateVolume(UIVolumeWnd, true);

	UIStaticTimeString.SetText(*InventoryUtilities::GetGameTimeAsString(InventoryUtilities::etpTimeToMinutes));
	CUIWindow::Update					();
}

void CUIInventoryWnd::Show() 
{ 	
	InitInventory			();
	inherited::Show			();

	if (!IsGameTypeSingle())
	{
		CActor *pActor = smart_cast<CActor*>(Level().CurrentEntity());
		if(!pActor) return;

		pActor->SetWeaponHideState(INV_STATE_INV_WND, true);

		//rank icon		
		int team = Game().local_player->team;
		int rank = Game().local_player->rank;
		string256 _path;		
		if (GameID() != GAME_DEATHMATCH){
			if (1==team)
		        sprintf_s(_path, "ui_hud_status_green_0%d", rank+1);
			else
				sprintf_s(_path, "ui_hud_status_blue_0%d", rank+1);
		}
		else
		{
			sprintf_s(_path, "ui_hud_status_green_0%d", rank+1);
		}
		UIRank->InitTexture(_path);
	}

	SendInfoToActor						("ui_inventory");
	
	CActor *pActor = smart_cast<CActor*>(Level().CurrentEntity());
	if (pActor)
	{
		if (g_eFreeHands == eFreeHandsManual) pActor->SetWeaponHideState(INV_STATE_INV_WND, true); //спрячем оружие в руках
		pActor->inventory().TryToHideWeapon(true);
		if (psActorFlags.test(AF_AMMO_FROM_BELT)) pActor->inventory().m_bRuckAmmoPlacement = true; //установим флаг перезарядки из рюкзака
	}
	//
	//UIRepackAmmoButton->Show(!!psActorFlags.test(AF_AMMO_BOX_AS_MAGAZINE)); //покажем кнопку перепаковки патронов, если опция включена
	//
	Update								();
	PlaySnd								(eInvSndOpen);
}

void CUIInventoryWnd::Hide()
{
	PlaySnd								(eInvSndClose);
	inherited::Hide						();

	SendInfoToActor						("ui_inventory_hide");

	ClearAllLists						();

	//достать вещь в активный слот
	CActor *pActor = smart_cast<CActor*>(Level().CurrentEntity());
	if(pActor && m_iCurrentActiveSlot != NO_ACTIVE_SLOT && 
		pActor->inventory().m_slots[m_iCurrentActiveSlot].m_pIItem)
	{
		pActor->inventory().Activate(m_iCurrentActiveSlot);
		m_iCurrentActiveSlot = NO_ACTIVE_SLOT;
	}

	if (!IsGameTypeSingle())
	{
		CActor *pActor		= smart_cast<CActor*>(Level().CurrentEntity());
		if(!pActor)			return;

		pActor->SetWeaponHideState(INV_STATE_INV_WND, false);
	}

	if (pActor)
	{
		if (g_eFreeHands == eFreeHandsManual) pActor->SetWeaponHideState(INV_STATE_INV_WND, false); //восстановим показ оружия в руках
		pActor->inventory().TryToHideWeapon(false);
		if (psActorFlags.test(AF_AMMO_FROM_BELT)) pActor->inventory().m_bRuckAmmoPlacement = false; //сбросим флаг перезарядки из рюкзака
		//
		//if (!psActorFlags.test(AF_AMMO_BOX_AS_MAGAZINE)) pActor->inventory().RepackAmmo(); //перепакуем патроны автоматически, если опция выключена
	}
}

void CUIInventoryWnd::AttachAddon(PIItem item_to_upgrade)
{
	PlaySnd										(eInvAttachAddon);
	R_ASSERT									(item_to_upgrade);
	if (OnClient())
	{
		NET_Packet								P;
		item_to_upgrade->object().u_EventGen	(P, GE_ADDON_ATTACH, item_to_upgrade->object().ID());
		P.w_u32									(CurrentIItem()->object().ID());
		item_to_upgrade->object().u_EventSend	(P);
	};

	item_to_upgrade->Attach						(CurrentIItem(), true);


	//спрятать вещь из активного слота в инвентарь на время вызова менюшки
	CActor *pActor								= smart_cast<CActor*>(Level().CurrentEntity());
	if(pActor && item_to_upgrade == pActor->inventory().ActiveItem())
	{
			m_iCurrentActiveSlot				= pActor->inventory().GetActiveSlot();
			pActor->inventory().Activate		(NO_ACTIVE_SLOT);
	}
	SetCurrentItem								(NULL);
}

void CUIInventoryWnd::DetachAddon(const char* addon_name)
{
	PlaySnd										(eInvDetachAddon);
	if (OnClient())
	{
		NET_Packet								P;
		CurrentIItem()->object().u_EventGen		(P, GE_ADDON_DETACH, CurrentIItem()->object().ID());
		P.w_stringZ								(addon_name);
		CurrentIItem()->object().u_EventSend	(P);
	};
	CurrentIItem()->Detach						(addon_name, true);

	//спрятать вещь из активного слота в инвентарь на время вызова менюшки
	CActor *pActor								= smart_cast<CActor*>(Level().CurrentEntity());
	if(pActor && CurrentIItem() == pActor->inventory().ActiveItem())
	{
			m_iCurrentActiveSlot				= pActor->inventory().GetActiveSlot();
			pActor->inventory().Activate		(NO_ACTIVE_SLOT);
	}
}

void CUIInventoryWnd::UpdateCustomDraw()
{
	CActor *pActor = smart_cast<CActor*>(Level().CurrentEntity());

	if (!pActor) return;

	u32 belt_width = pActor->inventory().BeltWidth();
	u32 belt_height = pActor->inventory().BeltHeight();

	Ivector2 belt_array = { belt_width, belt_height };

	m_pUIBeltList->SetCellsCapacity(belt_array);
}

void	CUIInventoryWnd::SendEvent_ActivateSlot	(PIItem	pItem)
{
	NET_Packet						P;
	pItem->object().u_EventGen		(P, GEG_PLAYER_ACTIVATE_SLOT, pItem->object().H_Parent()->ID());
	P.w_u32							(pItem->GetSlot());
	pItem->object().u_EventSend		(P);
}

void	CUIInventoryWnd::SendEvent_Item2Slot			(PIItem	pItem)
{
	NET_Packet						P;
	pItem->object().u_EventGen		(P, GEG_PLAYER_ITEM2SLOT, pItem->object().H_Parent()->ID());
	P.w_u16							(pItem->object().ID());
	pItem->object().u_EventSend		(P);
	g_pInvWnd->PlaySnd				(eInvItemToSlot);
};

void	CUIInventoryWnd::SendEvent_Item2Belt			(PIItem	pItem)
{
	NET_Packet						P;
	pItem->object().u_EventGen		(P, GEG_PLAYER_ITEM2BELT, pItem->object().H_Parent()->ID());
	P.w_u16							(pItem->object().ID());
	pItem->object().u_EventSend		(P);
	g_pInvWnd->PlaySnd				(eInvItemToBelt);
};

void	CUIInventoryWnd::SendEvent_Item2Ruck			(PIItem	pItem)
{
	NET_Packet						P;
	pItem->object().u_EventGen		(P, GEG_PLAYER_ITEM2RUCK, pItem->object().H_Parent()->ID());
	P.w_u16							(pItem->object().ID());
	pItem->object().u_EventSend		(P);

	g_pInvWnd->PlaySnd				(eInvItemToRuck);
};

void	CUIInventoryWnd::SendEvent_Item_Drop(PIItem	pItem)
{
	pItem->SetDropManual			(TRUE);

	if( OnClient() )
	{
		NET_Packet					P;
		pItem->object().u_EventGen	(P, GE_OWNERSHIP_REJECT, pItem->object().H_Parent()->ID());
		P.w_u16						(pItem->object().ID());
		pItem->object().u_EventSend(P);
	}
	g_pInvWnd->PlaySnd				(eInvDropItem);
};

void	CUIInventoryWnd::SendEvent_Item_Eat			(PIItem	pItem)
{
	R_ASSERT						(pItem->m_pCurrentInventory==m_pInv);
	NET_Packet						P;
	pItem->object().u_EventGen		(P, GEG_PLAYER_ITEM_EAT, pItem->object().H_Parent()->ID());
	P.w_u16							(pItem->object().ID());
	pItem->object().u_EventSend		(P);
};


void CUIInventoryWnd::BindDragDropListEvents(CUIDragDropListEx* lst)
{
	lst->m_f_item_drop				= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUIInventoryWnd::OnItemDrop);
	lst->m_f_item_start_drag		= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUIInventoryWnd::OnItemStartDrag);
	lst->m_f_item_db_click			= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUIInventoryWnd::OnItemDbClick);
	lst->m_f_item_selected			= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUIInventoryWnd::OnItemSelected);
	lst->m_f_item_rbutton_click		= CUIDragDropListEx::DRAG_DROP_EVENT(this,&CUIInventoryWnd::OnItemRButtonClick);
}


#include "../xr_level_controller.h"
#include <dinput.h>

bool CUIInventoryWnd::OnKeyboard(int dik, EUIMessages keyboard_action)
{
	if(m_b_need_reinit)
		return true;

	if (UIPropertiesBox.GetVisible())
		UIPropertiesBox.OnKeyboard(dik, keyboard_action);

	if ( is_binded(kDROP, dik) )
	{
		if(WINDOW_KEY_PRESSED==keyboard_action)
			DropCurrentItem(false);
		return true;
	}

	if (WINDOW_KEY_PRESSED == keyboard_action)
	{
#ifdef DEBUG
		if(DIK_NUMPAD7 == dik && CurrentIItem())
		{
			CurrentIItem()->ChangeCondition(-0.05f);
			UIItemInfo.InitItem(CurrentIItem());
		}
		else if(DIK_NUMPAD8 == dik && CurrentIItem())
		{
			CurrentIItem()->ChangeCondition(0.05f);
			UIItemInfo.InitItem(CurrentIItem());
		}
#endif
	}
	if( inherited::OnKeyboard(dik,keyboard_action) )return true;

	return false;
}
