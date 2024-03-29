#include "pch_script.h"
#include "InventoryBox.h"
#include "level.h"
#include "actor.h"
#include "game_object_space.h"
#include "uigamesp.h"
#include "hudmanager.h"

#include "script_callback_ex.h"
#include "script_game_object.h"

CInventoryBox::CInventoryBox()
{
	m_in_use = false;
}

void CInventoryBox::OnEvent(NET_Packet& P, u16 type)
{
	inherited::OnEvent	(P, type);

	switch (type)
	{
	case GE_OWNERSHIP_TAKE:
		{
			u16 id;
            P.r_u16(id);
			CObject* itm = Level().Objects.net_Find(id);  VERIFY(itm);
			m_items.push_back	(id);
			itm->H_SetParent	(this);
			itm->setVisible		(FALSE);
			itm->setEnabled		(FALSE);

			// Real Wolf: Коллбек для ящика на получение предмета. 02.08.2014.
			if (auto obj = smart_cast<CGameObject*>(itm) )
				this->callback(GameObject::eOnInvBoxItemTake)(obj->lua_game_object() );

		}break;
	case GE_OWNERSHIP_REJECT:
		{
			u16 id;
            P.r_u16(id);
			CObject* itm = Level().Objects.net_Find(id);  VERIFY(itm);
			xr_vector<u16>::iterator it;
			it = std::find(m_items.begin(),m_items.end(),id); VERIFY(it!=m_items.end());
			m_items.erase		(it);
			itm->H_SetParent	(NULL,!P.r_eof() && P.r_u8());

			// Real Wolf: Коллбек для ящика на потерю предмета. 02.08.2014.
			if (auto obj = smart_cast<CGameObject*>(itm) )
				this->callback(GameObject::eOnInvBoxItemDrop)(obj->lua_game_object() );

			if( m_in_use )
			{
				CGameObject* GO		= smart_cast<CGameObject*>(itm);
				Actor()->callback(GameObject::eInvBoxItemTake)( this->lua_game_object(), GO->lua_game_object() );

				if (Level().CurrentViewEntity() == Actor() && HUD().GetUI() && HUD().GetUI()->UIGame())
					HUD().GetUI()->UIGame()->ReInitShownUI();
			}
		}break;
	};
}

BOOL CInventoryBox::net_Spawn(CSE_Abstract* DC)
{
	inherited::net_Spawn	(DC);
	setVisible				(TRUE);
	setEnabled				(TRUE);
	set_tip_text			("inventory_box_use");

	return					TRUE;
}

void CInventoryBox::net_Relcase(CObject* O)
{
	inherited::net_Relcase(O);
}
#include "inventory_item.h"
void CInventoryBox::AddAvailableItems(TIItemContainer& items_container) const
{
	xr_vector<u16>::const_iterator it = m_items.begin();
	xr_vector<u16>::const_iterator it_e = m_items.end();

	for(;it!=it_e;++it)
	{
		PIItem itm = smart_cast<PIItem>(Level().Objects.net_Find(*it));VERIFY(itm);
		items_container.push_back	(itm);
	}
}

CScriptGameObject* CInventoryBox::GetObjectByName(LPCSTR name)
{
	for (auto it = m_items.begin(); it != m_items.end(); ++it)
		if (auto obj = smart_cast<CGameObject*>(Level().Objects.net_Find(*it) ) )
			if (obj->cName() == name)
				return obj->lua_game_object();
	return NULL;
}

CScriptGameObject* CInventoryBox::GetObjectByIndex(u32 id)
{
	if (id < m_items.size() )
	{
		u32 obj_id = u32(m_items[id]);
		if (auto obj = smart_cast<CGameObject*>(Level().Objects.net_Find(obj_id) ) )
			return obj->lua_game_object();
	}
	return NULL;
}

void CInventoryBox::UpdateCL()
{
	//Msg("UpdateCL() for InventoryBox [%s]", cName().c_str());
	UpdateDropTasks();
}

void CInventoryBox::UpdateDropTasks()
{
	xr_vector<u16>::const_iterator it = m_items.begin();
	xr_vector<u16>::const_iterator it_e = m_items.end();

	for (; it != it_e; ++it)
	{
		PIItem itm = smart_cast<PIItem>(Level().Objects.net_Find(*it)); VERIFY(itm);

		UpdateDropItem(itm);
	}
}

void CInventoryBox::UpdateDropItem(PIItem pIItem)
{
	if (pIItem->GetDropManual())
	{
		pIItem->SetDropManual(FALSE);
		if (OnServer())
		{
			NET_Packet					P;
			pIItem->object().u_EventGen(P, GE_OWNERSHIP_REJECT, pIItem->object().H_Parent()->ID());
			P.w_u16(u16(pIItem->object().ID()));
			pIItem->object().u_EventSend(P);
		}
	}// dropManual
}