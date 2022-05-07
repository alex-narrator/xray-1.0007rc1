#pragma once
#include "stdafx.h"
#include "UIWindow.h"
#include "../inventory_space.h"

class CInventoryOwner;
class CEatableItem;
class CTrade;
struct CUITradeInternal;

class CUIDragDropListEx;
class CUICellItem;
//
class CUIPropertiesBox;

class CUITradeWnd: public CUIWindow
{
private:
	typedef CUIWindow inherited;
	bool	m_b_need_update;
public:
						CUITradeWnd					();
	virtual				~CUITradeWnd				();

	virtual void		Init						();

	virtual void		SendMessage					(CUIWindow *pWnd, s16 msg, void *pData);

	void				InitTrade					(CInventoryOwner* pOur, CInventoryOwner* pOthers);
	
	virtual void 		Draw						();
	virtual void 		Update						();
	virtual void 		Show						();
	virtual void 		Hide						();

	void 				DisableAll					();
	void 				EnableAll					();
	//контекстное меню
	void				ActivatePropertiesBox		();
	virtual bool		OnKeyboard                  (int dik, EUIMessages keyboard_action);
	virtual bool		OnMouse                     (float x, float y, EUIMessages mouse_action);

	void 				SwitchToTalk				();
	void 				StartTrade					();
	void 				StopTrade					();

	void				UpdateLists_delayed			();
protected:

	CUITradeInternal*	m_uidata;

	bool				bStarted;
	bool 				ToOurTrade					();
	bool 				ToOthersTrade				();
	bool 				ToOurBag					();
	bool 				ToOthersBag					();
	void 				SendEvent_ItemDrop			(PIItem pItem);
	
	u32					CalcItemsPrice				(CUIDragDropListEx* pList, CTrade* pTrade, bool bBuying);
	float				CalcItemsWeight				(CUIDragDropListEx* pList);

	void				TransferItems				(CUIDragDropListEx* pSellList, CUIDragDropListEx* pBuyList, CTrade* pTrade, bool bBuying);

	void				PerformTrade				();
	void				UpdatePrices				();

	enum EListType{eNone,e1st,e2nd,eBoth};
	void				UpdateLists					(EListType);

	void				FillList					(TIItemContainer& cont, CUIDragDropListEx& list, bool do_colorize);

	bool				m_bDealControlsVisible;

	bool				CanMoveToOther				(PIItem pItem);

	//указатели игрока и того с кем торгуем
	CInventory*			m_pInv;
	CInventory*			m_pOthersInv;
	CInventoryOwner*	m_pInvOwner;
	CInventoryOwner*	m_pOthersInvOwner;
	CTrade*				m_pTrade;
	CTrade*				m_pOthersTrade;
	//
	CUIPropertiesBox*	m_pUIPropertiesBox;

	u32					m_iOurTradePrice;
	u32					m_iOthersTradePrice;


	CUICellItem*		m_pCurrentCellItem;
	TIItemContainer		ruck_list;


	void				SetCurrentItem				(CUICellItem* itm);
	CUICellItem*		CurrentItem					();
	PIItem				CurrentIItem				();
	//
	bool                MoveItemsFromCell			(CUICellItem* itm, bool b_all);  //переместить предмет/стак предметов
	//
	bool                b_TakeAllActionKeyHolded;
	//
	void				DropItemsfromCell			(bool b_all);        //выбросить предмет/стак предметов

	bool		xr_stdcall		OnItemDrop			(CUICellItem* itm);
	bool		xr_stdcall		OnItemStartDrag		(CUICellItem* itm);
	bool		xr_stdcall		OnItemDbClick		(CUICellItem* itm);
	bool		xr_stdcall		OnItemSelected		(CUICellItem* itm);
	bool		xr_stdcall		OnItemRButtonClick	(CUICellItem* itm);

	void				BindDragDropListEvents		(CUIDragDropListEx* lst);

	void				SendEvent_Item_Drop			(PIItem	pItem);
};