#pragma once

#include "../../../build_config_defines.h"

//////////////////////////////////////////////////////////////////////////

enum EUIMessages
{
	// CUIWindow
	WINDOW_LBUTTON_DOWN = 0,
	WINDOW_RBUTTON_DOWN,
	WINDOW_CBUTTON_DOWN,

	WINDOW_LBUTTON_UP,
	WINDOW_RBUTTON_UP,
	WINDOW_CBUTTON_UP,

	WINDOW_MOUSE_MOVE,
	WINDOW_MOUSE_WHEEL_UP,
	WINDOW_MOUSE_WHEEL_DOWN,
	WINDOW_LBUTTON_DB_CLICK,
	WINDOW_KEY_PRESSED,
	WINDOW_KEY_RELEASED,
	WINDOW_MOUSE_CAPTURE_LOST, 
	WINDOW_KEYBOARD_CAPTURE_LOST,

	// CUIStatic
	STATIC_FOCUS_RECEIVED,
	STATIC_FOCUS_LOST,

	// CUIButton
	BUTTON_CLICKED,
	BUTTON_DOWN,

	// CUITabControl
	TAB_CHANGED,	

	// CUICheckButton
	CHECK_BUTTON_SET,
	CHECK_BUTTON_RESET, 

	// CUIRadioButton
	RADIOBUTTON_SET,

	// CUIdragDropItem
	DRAG_DROP_ITEM_DRAG,
	DRAG_DROP_ITEM_DROP, 
	DRAG_DROP_ITEM_DB_CLICK,
	DRAG_DROP_ITEM_RBUTTON_CLICK,
	DRAG_DROP_ITEM_SELECTED,
	
	// CUIScrollBox
	SCROLLBOX_MOVE,

	// CUIScrollBar
	SCROLLBAR_VSCROLL,
	SCROLLBAR_HSCROLL,
	SCROLLBAR_NEEDUPDATE,

	// CUIScrollView
	CHILD_CHANGED_SIZE,

	// CUIListWnd
	LIST_ITEM_CLICKED,
	LIST_ITEM_DB_CLICKED,
	LIST_ITEM_SELECT,
	LIST_ITEM_UNSELECT,
	LIST_ITEM_FOCUS_RECEIVED,

	// CUIInteractiveItem
	INTERACTIVE_ITEM_CLICK,

	// UIPropertiesBox
	PROPERTY_CLICKED,

	// CUIMessageBox
	MESSAGE_BOX_OK_CLICKED,
	MESSAGE_BOX_YES_CLICKED,
	MESSAGE_BOX_QUIT_WIN_CLICKED,
	MESSAGE_BOX_QUIT_GAME_CLICKED,
	MESSAGE_BOX_NO_CLICKED,
	MESSAGE_BOX_CANCEL_CLICKED,

	// CUITalkDialogWnd
	TALK_DIALOG_TRADE_BUTTON_CLICKED,
	TALK_DIALOG_QUESTION_CLICKED,

	// CUIPdaWnd
	PDA_OPEN_ENCYCLOPEDIA_ARTICLE,
	PDA_ENCYCLOPEDIA_HAS_ARTICLE,
	// CUIPdaDialogWnd
	PDA_DIALOG_WND_BACK_BUTTON_CLICKED,
	PDA_DIALOG_WND_MESSAGE_BUTTON_CLICKED,

	// CUIPdaContactsWnd
	PDA_CONTACTS_WND_CONTACT_SELECTED,

	// CUITradeWnd
	TRADE_WND_CLOSED,

	// CUISleepWnd
//	SLEEP_WND_PERFORM_BUTTON_CLICKED,

	// CUIOutfitSlot
	UNDRESS_OUTFIT,
	OUTFIT_RETURNED_BACK,

	// CUIInventroyWnd
	INVENTORY_DROP_ACTION,
	INVENTORY_EAT_ACTION,
	INVENTORY_TO_BELT_ACTION, 
	INVENTORY_TO_SLOT_ACTION,
	INVENTORY_TO_BAG_ACTION,
	INVENTORY_ATTACH_ADDON, 

	INVENTORY_ATTACH_SCOPE_ADDON,
	INVENTORY_DETACH_SCOPE_ADDON,

    INVENTORY_ATTACH_SILENCER_ADDON,	
	INVENTORY_DETACH_SILENCER_ADDON,

	INVENTORY_ATTACH_GRENADE_LAUNCHER_ADDON,
	INVENTORY_DETACH_GRENADE_LAUNCHER_ADDON,
	//
	INVENTORY_SWITCH_GRENADE_LAUNCHER_MODE,

	INVENTORY_RELOAD_MAGAZINE,
	INVENTORY_UNLOAD_MAGAZINE,
	INVENTORY_SELL_ITEM,
	//
	INVENTORY_MOVE_ACTION,
	//
	INVENTORY_DETECTOR_CHECK_ACTION,
	// AmmoBox
	INVENTORY_RELOAD_AMMO_BOX,
	INVENTORY_UNLOAD_AMMO_BOX,

	// BuyWeaponWnd
	XR_MENU_LEVEL_CHANGED,
	MP_MONEY_CHANGE,

	//CUIAnimationBase
    ANIMATION_STOPPED,
	EDIT_TEXT_CHANGED,
	EDIT_TEXT_COMMIT,
	MAP_SHOW_HINT,
	MAP_HIDE_HINT,
	MAP_SELECT_SPOT,
#if defined(INV_NEW_SLOTS_SYSTEM) || defined(INV_DOUBLE_WPN_SLOTS)
	// Для контекстного меню равнозначных слотов оружия. Real Wolf.
	INVENTORY_TO_SLOT0_ACTION,
	INVENTORY_TO_SLOT1_ACTION,
	INVENTORY_TO_SLOT2_ACTION,
	INVENTORY_TO_SLOT3_ACTION,
	INVENTORY_TO_SLOT4_ACTION,
	INVENTORY_TO_SLOT5_ACTION,
	INVENTORY_TO_SLOT6_ACTION,
	INVENTORY_TO_SLOT7_ACTION,
	INVENTORY_TO_SLOT8_ACTION,
	INVENTORY_TO_SLOT9_ACTION,
	INVENTORY_TO_SLOT10_ACTION,
	INVENTORY_TO_SLOT11_ACTION,
	INVENTORY_TO_SLOT12_ACTION,
	INVENTORY_TO_SLOT13_ACTION,
	INVENTORY_TO_SLOT14_ACTION,
	INVENTORY_TO_SLOT15_ACTION,
	INVENTORY_TO_SLOT16_ACTION,
#endif
};
