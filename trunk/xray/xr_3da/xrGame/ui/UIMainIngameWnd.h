// UIMainIngameWnd.h:  окошки-информация в игре
// 
//////////////////////////////////////////////////////////////////////

#pragma once

#include "UIProgressBar.h"
#include "UIGameLog.h"

#include "../alife_space.h"

#include "UICarPanel.h"
#include "UIMotionIcon.h"
#include "../hudsound.h"
#include "../script_export_space.h"
#include "../inventory.h"
#include "../../../build_config_defines.h"

//для режима настройки HUD
extern int				g_bHudAdjustMode;
extern float			g_fHudAdjustValue;

// Енумы элементов худа
enum EHUDElement
{
	ePDA,
	eDetector,
	eActiveItem,
	eGear,
	eArmor,
};

class					CUIPdaMsgListItem;
class					CLAItem;
class					CUIZoneMap;
class					CUIArtefactPanel;
class					CUIScrollView;
struct					GAME_NEWS_DATA;
class					CActor;
class					CWeapon;
class					CMissile;
class					CInventoryItem;

#ifdef INV_QUICK_SLOT_PANEL
class CUIXml;
class CUIStatic;

class CUIQuickSlotPanel : public CUIWindow
{
private:
	typedef				CUIWindow inherited;
public:
	CUIQuickSlotPanel			();
	~CUIQuickSlotPanel			();
	virtual void 		Update			();
	virtual void		Draw			();
	virtual void		Show			();
	virtual void		Hide			();
	virtual void 		Init			();
	void				DrawItemInSlot		(const PIItem itm, CUIStatic* m_QuickSlot_Icon, Fvector2 m_QuickSlot_Icon_Size );
protected:
	//
	Fvector2					m_QuickSlot_0_Icon_Size; 
	Fvector2					m_QuickSlot_1_Icon_Size;
	Fvector2					m_QuickSlot_2_Icon_Size;
	Fvector2					m_QuickSlot_3_Icon_Size;	
	//
	CUIStatic*					m_QuickSlot_0_Icon;
	CUIStatic*					m_QuickSlot_1_Icon;
	CUIStatic*					m_QuickSlot_2_Icon;
	CUIStatic*					m_QuickSlot_3_Icon;	
	//
	CUIStatic*					m_CountItemQuickSlot_0_Text;
	CUIStatic*					m_CountItemQuickSlot_1_Text;
	CUIStatic*					m_CountItemQuickSlot_2_Text;
	CUIStatic*					m_CountItemQuickSlot_3_Text;
	//
	CUIStatic*					m_UseQuickSlot_0_Text;
	CUIStatic*					m_UseQuickSlot_1_Text;
	CUIStatic*					m_UseQuickSlot_2_Text;
	CUIStatic*					m_UseQuickSlot_3_Text;
	//
	CUIStatic*					m_QuickSlotPanelBackground;
};
#endif

class CUIMainIngameWnd: public CUIWindow  
{
public:
	CUIMainIngameWnd();
	virtual ~CUIMainIngameWnd();

	virtual void Init();
	virtual void Draw();
	virtual void Update();

	bool OnKeyboardPress(int dik);
	bool OnKeyboardHold(int cmd);
	void HudAdjustMode(int);

protected:

	CUIStatic			UIStaticDiskIO;
	CUIStatic			UIStaticHealth;
	CUIStatic			UIStaticArmor;
	CUIStatic			UIStaticQuickHelp;
	CUIProgressBar		UIHealthBar;
	CUIProgressBar		UIArmorBar;
	CUICarPanel			UICarPanel;
	CUIMotionIcon		UIMotionIcon;	
	CUIZoneMap*			UIZoneMap;

	//иконка, показывающая количество активных PDA
	CUIStatic			UIPdaOnline;
	
	//изображение оружия
	CUIStatic			UIWeaponBack;
	CUIStatic			UIWeaponSignAmmo;
	CUIStatic			UIWeaponIcon;
	Frect				UIWeaponIcon_rect;
		
public:
	CUIStatic*			GetPDAOnline					() { return &UIPdaOnline; };
	CUIZoneMap*			GetUIZoneMap					() { return UIZoneMap; }
protected:


	// 5 статиков для отображения иконок:
	// - сломанного оружия
	// - радиации
	// - ранения
	// - голода
	// - усталости
	CUIStatic			UIWeaponJammedIcon;
	CUIStatic			UIRadiaitionIcon;
	CUIStatic			UIWoundIcon;
	CUIStatic			UIStarvationIcon;
	CUIStatic			UIPsyHealthIcon;
	CUIStatic			UIInvincibleIcon;
	CUIStatic			UISleepIcon;
	CUIStatic			UIArtefactIcon;
	//
	CUIStatic			UIArmorIcon;
	CUIStatic			UIHealthIcon;
	CUIStatic			UIPowerIcon;

	CUIScrollView*		m_UIIcons;
	CUIWindow*			m_pMPChatWnd;
	CUIWindow*			m_pMPLogWnd;
public:	
	CUIArtefactPanel*    m_artefactPanel;
#ifdef INV_QUICK_SLOT_PANEL
	CUIQuickSlotPanel*	 m_quickSlotPanel;
#endif

public:
	
	// Енумы соответсвующие предупреждающим иконкам 
	enum EWarningIcons
	{
		ewiAll				= 0,
		ewiWeaponJammed,
		ewiRadiation,
		ewiWound,
		ewiStarvation,
		ewiPsyHealth,
		ewiArmor,
		ewiHealth,
		ewiPower,
		ewiInvincible,
//		ewiSleep,
		ewiArtefact,
	};

	//// Енумы элементов худа
	//enum EHUDElement
	//{
	//	ePDA,
	//	eDetector,
	//	eActiveItem,
	//	eGear,
	//	eArmor,
	//};
	bool                IsHUDElementAllowed				(EHUDElement element);
	//
	void				SetMPChatLog					(CUIWindow* pChat, CUIWindow* pLog);

	// Задаем цвет соответствующей иконке
	void				SetWarningIconColor				(EWarningIcons icon, const u32 cl);
	void				TurnOffWarningIcon				(EWarningIcons icon);

	// Пороги изменения цвета индикаторов, загружаемые из system.ltx
	typedef				xr_map<EWarningIcons, xr_vector<float> >	Thresholds;
	typedef				Thresholds::iterator						Thresholds_it;
	Thresholds			m_Thresholds;

	// Енум перечисления возможных мигающих иконок
	enum EFlashingIcons
	{
		efiPdaTask	= 0,
		efiMail
	};
	
	void				SetFlashIconState_				(EFlashingIcons type, bool enable);

	void				AnimateContacts					(bool b_snd);
	HUD_SOUND			m_contactSnd;

	void				ReceiveNews						(GAME_NEWS_DATA* news);
	
protected:
	void				SetWarningIconColor				(CUIStatic* s, const u32 cl);
	void				InitFlashingIcons				(CUIXml* node);
	void				DestroyFlashingIcons			();
	void				UpdateFlashingIcons				();
	void				UpdateActiveItemInfo			();

	void				SetAmmoIcon						(const shared_str& seсt_name);

	// first - иконка, second - анимация
	DEF_MAP				(FlashingIcons, EFlashingIcons, CUIStatic*);
	FlashingIcons		m_FlashingIcons;

	//для текущего активного актера и оружия
	CActor*				m_pActor;	
	CWeapon*			m_pWeapon;
	CMissile*			m_pGrenade;
	CInventoryItem*		m_pItem;

	// Отображение подсказок при наведении прицела на объект
	void				RenderQuickInfos();

public:
	CUICarPanel&		CarPanel							(){return UICarPanel;};
	CUIMotionIcon&		MotionIcon							(){return UIMotionIcon;}
	void				OnConnected							();
	void				reset_ui							();
protected:
	CInventoryItem*		m_pPickUpItem;
	CUIStatic			UIPickUpItemIcon;

	float				m_iPickUpItemIconX;
	float				m_iPickUpItemIconY;
	float				m_iPickUpItemIconWidth;
	float				m_iPickUpItemIconHeight;

	void				UpdatePickUpItem();
public:
	void				SetPickUpItem	(CInventoryItem* PickUpItem);

	DECLARE_SCRIPT_REGISTER_FUNCTION
#ifdef DEBUG
	void				draw_adjust_mode					();
#endif
};

add_to_type_list(CUIMainIngameWnd)
#undef script_type_list
#define script_type_list save_type_list(CUIMainIngameWnd)


