#include "stdafx.h"
#include "UICellItem.h"
#include "../xr_level_controller.h"
#include "../../xr_input.h"
#include "../HUDManager.h"
#include "../level.h"
#include "../object_broker.h"
#include "UIDragDropListEx.h"
#include "UIProgressBar.h"

#include "UIXmlInit.h"
#include "UIInventoryWnd.h"
#include "../Weapon.h"
#include "../CustomOutfit.h"

#include "xr_ini.h"
#include "../smart_cast.h"
#include "../WeaponMagazinedWGrenade.h"

CUICellItem::CUICellItem()
{
	m_pParentList		= NULL;
	m_text				= NULL;
	m_pData				= NULL;
	m_custom_draw		= NULL;
	m_b_already_drawn	= false;
	SetAccelerator		(0);
	m_b_destroy_childs	= true;
	m_pConditionState 	= NULL;
	init();
}

CUICellItem::~CUICellItem()
{
	if(m_b_destroy_childs)
		delete_data	(m_childs);

	delete_data		(m_custom_draw);
}


void CUICellItem::init()
{
	CUIXml uiXml;
	bool xml_result						= uiXml.Init(CONFIG_PATH, UI_PATH, "inventory_new.xml");
	R_ASSERT3							(xml_result, "file parsing error ", uiXml.m_xml_file_name);
	
	m_text					= xr_new<CUIStatic>();
	m_text->SetAutoDelete	( true );
	AttachChild				( m_text );
	CUIXmlInit::InitStatic	( uiXml, "cell_item_text", 0, m_text );
	m_text->Show			( false );
	
	m_pConditionState					= xr_new<CUIProgressBar>();
	m_pConditionState->SetAutoDelete(true);
	AttachChild(m_pConditionState);
	CUIXmlInit::InitProgressBar(uiXml, "condition_progess_bar", 0, m_pConditionState);
	m_pConditionState->Show(true);

	//colorize
	u_ColorWeapon		= READ_IF_EXISTS(pSettings, r_color, "colorize_item", "weapon",		0xffffffff);
	u_ColorEquipped		= READ_IF_EXISTS(pSettings, r_color, "colorize_item", "equipped",	0xffffffff);
	u_ColorUntradeable	= READ_IF_EXISTS(pSettings, r_color, "colorize_item", "untrade",	0xffffffff);
}

void CUICellItem::Draw()
{
	m_b_already_drawn		= true;
	inherited::Draw();
	if(m_custom_draw) 
		m_custom_draw->OnDraw(this);
};

bool CUICellItem::OnMouse(float x, float y, EUIMessages mouse_action)
{
	if(mouse_action == WINDOW_LBUTTON_DOWN){
		GetMessageTarget()->SendMessage(this, DRAG_DROP_ITEM_SELECTED, NULL);
		return false;
	}else
	if(mouse_action == WINDOW_MOUSE_MOVE && pInput->iGetAsyncBtnState(0)){
		GetMessageTarget()->SendMessage(this, DRAG_DROP_ITEM_DRAG, NULL);
		return true;
	}else
	if(mouse_action==WINDOW_LBUTTON_DB_CLICK){
		GetMessageTarget()->SendMessage(this, DRAG_DROP_ITEM_DB_CLICK, NULL);
		return true;
	}else
	if(mouse_action==WINDOW_RBUTTON_DOWN){
		GetMessageTarget()->SendMessage(this, DRAG_DROP_ITEM_RBUTTON_CLICK, NULL);
		return true;
	}
	
	return false;
};

bool CUICellItem::OnKeyboard(int dik, EUIMessages keyboard_action)
{
	if (WINDOW_KEY_PRESSED == keyboard_action)
	{
		if (GetAccelerator() == dik)
		{
			GetMessageTarget()->SendMessage(this, DRAG_DROP_ITEM_DB_CLICK, NULL);
			return		true;
		}
	}
	return inherited::OnKeyboard(dik, keyboard_action);
}

CUIDragItem* CUICellItem::CreateDragItem()
{
	CUIDragItem* tmp;
	tmp = xr_new<CUIDragItem>(this);
	Frect r;
	GetAbsoluteRect(r);
	if( m_UIStaticItem.GetFixedLTWhileHeading() )
	{
		float w, h;
		w				= r.width();
		h				= r.height();
		if (Heading())
		{   // исправление пропорций
			w = w / m_cell_size.x * m_cell_size.y;
			h = h / m_cell_size.y * m_cell_size.x;
		}

		Fvector2 cp = GetUICursor()->GetCursorPosition();
		// поворот на 90 градусов, и центрирование по курсору мыша						
		r.x1			= (cp.x - h / 2.0f);
		r.y1			= (cp.y - w / 2.0f);
		r.x2			= r.x1 + h;
		r.y2			= r.y1 + w;
	} 
	tmp->Init(GetShader(),r,GetUIStaticItem().GetOriginalRect());
	return tmp;
}

void CUICellItem::SetOwnerList(CUIDragDropListEx* p)	
{
	m_pParentList=p;
	UpdateConditionProgressBar();
}

void CUICellItem::UpdateConditionProgressBar()
{
    if(m_pParentList && m_pParentList->GetConditionProgBarVisibility())
    {
        PIItem itm = (PIItem)m_pData;
        CWeapon* pWeapon = smart_cast<CWeapon*>(itm);
        CCustomOutfit* pOutfit = smart_cast<CCustomOutfit*>(itm);

        if(pWeapon || pOutfit )
        {
            Ivector2 itm_grid_size = GetGridSize(true);
            if(m_pParentList->GetVerticalPlacement())
                std::swap(itm_grid_size.x, itm_grid_size.y);
            Ivector2 cell_size = m_pParentList->CellSize();

			m_pConditionState->SetWidth((float)cell_size.x);

			float x = 0.5f*(itm_grid_size.x * (cell_size.x)-m_pConditionState->GetWidth());
            float y = itm_grid_size.y * (cell_size.y) - m_pConditionState->GetHeight() - 1.f;

            m_pConditionState->SetWndPos(Fvector2().set(x,y));
            m_pConditionState->SetProgressPos(itm->GetCondition()*100.0f);
            m_pConditionState->Show(true);
            return;
        }
    }
    m_pConditionState->Show(false);
}


bool CUICellItem::EqualTo(CUICellItem* itm)
{
	return (m_grid_size.x==itm->GetGridSize(true).x) && (m_grid_size.y==itm->GetGridSize(true).y);
}

u32 CUICellItem::ChildsCount()
{
	return m_childs.size();
}

void CUICellItem::PushChild(CUICellItem* c)
{
	R_ASSERT(c->ChildsCount()==0);
	VERIFY				(this!=c);
	m_childs.push_back	(c);
	UpdateItemText		();
}

CUICellItem* CUICellItem::PopChild()
{
	CUICellItem* itm	= m_childs.back();
	m_childs.pop_back	();
	std::swap			(itm->m_pData, m_pData);
	UpdateItemText		();
	R_ASSERT			(itm->ChildsCount()==0);
	itm->SetOwnerList	(NULL);
	return				itm;
}

bool CUICellItem::HasChild(CUICellItem* item)
{
	return (m_childs.end() != std::find(m_childs.begin(), m_childs.end(), item) );
}

void CUICellItem::UpdateItemText()
{
	string32 str;

		if ( ChildsCount() )
		{
			sprintf_s(str,"x%d",ChildsCount()+1);
			/*m_text->SetText(str);
			m_text->Show( true );*/
		}else{
			sprintf_s(str,"");
			//m_text->Show( false );
		}

		SetText(str);
}

void CUICellItem::Update()
{
	if (m_pParentList)
		EnableHeading(m_pParentList->GetVerticalPlacement());

	if(Heading())
	{
		SetHeading			( 90.0f * (PI/180.0f) );
		SetHeadingPivot		(Fvector2().set(0.0f,0.0f), Fvector2().set(0.0f,GetWndSize().y), true);
	}else
		ResetHeadingPivot	();

	inherited::Update();
	
	m_b_already_drawn=false;
 
}

//
void CUICellItem::ColorizeEquipped(CUICellItem* itm, bool b_can_trade)
{
	PIItem iitem = (PIItem)itm->m_pData;

	if (!b_can_trade)
		itm->SetTextureColor(u_ColorUntradeable);
	else if (iitem->m_eItemPlace == eItemPlaceSlot || iitem->m_eItemPlace == eItemPlaceBelt)
		itm->SetTextureColor(u_ColorEquipped);
}
//
void CUICellItem::ColorizeWeapon(CUIDragDropListEx* List1, CUIDragDropListEx* List2, CUIDragDropListEx* List3, CUIDragDropListEx* List4)
{
	auto inventoryitem = (CInventoryItem*)this->m_pData;
	if (!inventoryitem)
		return;

	auto ClearTextureColor = [=](CUIDragDropListEx* DdListEx) 
	{
		for (u32 i = 0, item_count = DdListEx->ItemsCount(); i < item_count; ++i) 
		{
			CUICellItem* CellItem = DdListEx->GetItemIdx(i);
			if (CellItem->GetTextureColor() == u_ColorWeapon)
				CellItem->SetTextureColor(0xffffffff);
		}
	};

	ClearTextureColor(List1);
	ClearTextureColor(List2);
	if (List3)
		ClearTextureColor(List3);
	if (List4)
		ClearTextureColor(List4);

	auto Wpn = smart_cast<CWeaponMagazined*>(inventoryitem);
	auto Ammo = smart_cast<CWeaponAmmo*>(inventoryitem);
	if (!Wpn && !Ammo)
		return;

	xr_vector<shared_str> ColorizeSects;
	if (Wpn)
	{
		std::copy(Wpn->m_ammoTypes.begin(), Wpn->m_ammoTypes.end(), std::back_inserter(ColorizeSects));
		if (auto WpnGl = smart_cast<CWeaponMagazinedWGrenade*>(inventoryitem))
			std::copy(WpnGl->m_ammoTypes2.begin(), WpnGl->m_ammoTypes2.end(), std::back_inserter(ColorizeSects));
		if (Wpn->SilencerAttachable())
			ColorizeSects.push_back(Wpn->GetSilencerName());
		if (Wpn->ScopeAttachable())
			ColorizeSects.push_back(Wpn->GetScopeName());
		if (Wpn->GrenadeLauncherAttachable())
			ColorizeSects.push_back(Wpn->GetGrenadeLauncherName());
	}
	else if (Ammo)	//ammo mag
	{
		if (Ammo->IsBoxReloadableEmpty())
			std::copy(Ammo->m_ammoTypes.begin(), Ammo->m_ammoTypes.end(), std::back_inserter(ColorizeSects));
		if (Ammo->IsBoxReloadable())
			ColorizeSects.push_back(Ammo->m_ammoSect);
	}



	auto ProcessColorize = [=](CUIDragDropListEx* DdListEx) 
	{
		for (u32 i = 0, item_count = DdListEx->ItemsCount(); i < item_count; ++i) 
		{
			CUICellItem* CellItem = DdListEx->GetItemIdx(i);
			auto invitem = (CInventoryItem*)CellItem->m_pData;
			if (invitem && (std::find(ColorizeSects.begin(), ColorizeSects.end(), invitem->object().cNameSect()) != ColorizeSects.end()))
				if (CellItem->GetTextureColor() == 0xffffffff)
					CellItem->SetTextureColor(u_ColorWeapon);
		}
	};

	ProcessColorize(List1);
	ProcessColorize(List2);
	if (List3)
		ProcessColorize(List3);
	if (List4)
		ProcessColorize(List4);
}
//

void CUICellItem::SetCustomDraw			(ICustomDrawCell* c){
	if (m_custom_draw)
		xr_delete(m_custom_draw);
	m_custom_draw = c;
}

Ivector2 CUICellItem::GetGridSize(bool with_child)
{
	return m_grid_size;
}


CUIDragItem::CUIDragItem(CUICellItem* parent)
{
	m_back_list						= NULL;
	m_pParent						= parent;
	AttachChild						(&m_static);
	Device.seqRender.Add			(this, REG_PRIORITY_LOW-5000);
	Device.seqFrame.Add				(this, REG_PRIORITY_LOW-5000);
	VERIFY							(m_pParent->GetMessageTarget());
}

CUIDragItem::~CUIDragItem()
{
	Device.seqRender.Remove			(this);
	Device.seqFrame.Remove			(this);
}

void CUIDragItem::Init(const ref_shader& sh, const Frect& rect, const Frect& text_rect)
{
	SetWndRect						(rect);
	m_static.SetShader				(sh);
	m_static.SetOriginalRect		(text_rect);
	m_static.SetWndPos				(0.0f,0.0f);
	m_static.SetWndSize				(GetWndSize());
	m_static.TextureAvailable		(true);
	m_static.TextureOn				();
	m_static.SetColor				(color_rgba(255,255,255,170));
	m_static.SetStretchTexture		(true);
	m_pos_offset.sub				(rect.lt, GetUICursor()->GetCursorPosition());
}

bool CUIDragItem::OnMouse(float x, float y, EUIMessages mouse_action)
{
	if(mouse_action == WINDOW_LBUTTON_UP)
	{
		m_pParent->GetMessageTarget()->SendMessage(m_pParent,DRAG_DROP_ITEM_DROP,NULL);
		return true;
	}
	return false;
}

void CUIDragItem::OnRender()
{
	Draw			();
}

void CUIDragItem::OnFrame()
{
	Update			();
}

void CUIDragItem::Draw()
{
	Fvector2 tmp;
	tmp.sub					(GetWndPos(), GetUICursor()->GetCursorPosition());
	tmp.sub					(m_pos_offset);
	tmp.mul					(-1.0f);
	MoveWndDelta			(tmp);
	UI()->PushScissor		(UI()->ScreenRect(),true);

	inherited::Draw();

	UI()->PopScissor();
}

void CUIDragItem::SetBackList(CUIDragDropListEx*l)
{
	if(m_back_list!=l){
		m_back_list=l;
	}
}

Fvector2 CUIDragItem::GetPosition()
{
	return Fvector2().add(m_pos_offset, GetUICursor()->GetCursorPosition());
}

