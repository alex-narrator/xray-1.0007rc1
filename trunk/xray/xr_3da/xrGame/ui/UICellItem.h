#pragma once

#include "UIStatic.h"
#include "UIDialogWnd.h"

class CUIDragItem;
class CUIDragDropListEx;
class CUICellItem;
class CUIProgressBar;

class ICustomDrawCell
{
public:
	virtual				~ICustomDrawCell	()	{};
	virtual void		OnDraw				(CUICellItem* cell)	= 0;
};

class CUICellItem :public CUIStatic
{
private:
	typedef		CUIStatic	inherited;
protected:
	xr_vector<CUICellItem*> m_childs;

	CUIDragDropListEx*		m_pParentList;
	CUIProgressBar* 		m_pConditionState;
	Ivector2				m_grid_size;
	ICustomDrawCell*		m_custom_draw;
	Fvector2				m_cell_size;				// alpet: реальные размеры ячейки инветаря вместо 50.f
	int						m_accelerator;
	CUIStatic*				m_text; 
	virtual void			UpdateItemText			();
	void					init					();
public:
							CUICellItem				();
	virtual					~CUICellItem			();

	virtual		bool		OnKeyboard				(int dik, EUIMessages keyboard_action);
	virtual		bool		OnMouse					(float x, float y, EUIMessages mouse_action);
	virtual		void		Draw					();
	virtual		void		Update					();
				
	virtual		void		OnAfterChild			(CUIDragDropListEx* parent_list)						{};
	virtual		void		OnBeforeChild			(CUIDragDropListEx* parent_list)						{};

				u32			ChildsCount				();
				void		 PushChild				(CUICellItem*);
				CUICellItem* PopChild				();
				CUICellItem* Child					(u32 idx)				{return m_childs[idx];};
				bool		HasChild					(CUICellItem* item);
	virtual		bool		EqualTo					(CUICellItem* itm);
	virtual		Ivector2	GetGridSize				(bool);

	IC			void		SetAccelerator			(int dik)				{m_accelerator=dik;};
	IC			int			GetAccelerator			()		const			{return m_accelerator;};

	virtual		CUIDragItem* CreateDragItem			();
	static		CUICellItem* m_mouse_selected_item;

	CUIDragDropListEx*		OwnerList				()						{return m_pParentList;}
				void		SetOwnerList			(CUIDragDropListEx* p);
				void		UpdateConditionProgressBar();
				void		SetCustomDraw			(ICustomDrawCell* c);
				void*		m_pData;
				int			m_index;
				bool		m_b_already_drawn;
				bool		m_b_destroy_childs;
				//colorize
				void		ColorizeWeapon		(CUIDragDropListEx*, CUIDragDropListEx*, CUIDragDropListEx* = nullptr, CUIDragDropListEx* = nullptr);
				void		ColorizeEquipped	(CUICellItem* itm, bool b_can_trade = true);
				u32         u_ColorWeapon;
				u32         u_ColorEquipped;
				u32         u_ColorUntradeable;
};

class CUIDragItem: public CUIWindow, public pureRender, public pureFrame
{
private:
	typedef		CUIWindow	inherited;
	CUIStatic				m_static;
	CUICellItem*			m_pParent;
	Fvector2				m_pos_offset;
	CUIDragDropListEx*		m_back_list;
public:
							CUIDragItem(CUICellItem* parent);
	virtual		void		Init(const ref_shader& sh, const Frect& rect, const Frect& text_rect);
	virtual					~CUIDragItem();
			CUIStatic*		wnd						() {return &m_static;}
	virtual		bool		OnMouse					(float x, float y, EUIMessages mouse_action);
	virtual		void		Draw					();
	virtual		void		OnRender				();
	virtual		void		OnFrame					();
		CUICellItem*		ParentItem				()							{return m_pParent;}
				void		SetBackList				(CUIDragDropListEx*l);
	CUIDragDropListEx*		BackList				()							{return m_back_list;}
				Fvector2	GetPosition				();
};
