#include "stdafx.h"
#include "grenade.h"
#include "PhysicsShell.h"
#include "WeaponHUD.h"
#include "entity.h"
#include "ParticlesObject.h"
#include "actor.h"
#include "inventory.h"
#include "level.h"
#include "xrmessages.h"
#include "xr_level_controller.h"
#include "game_cl_base.h"
#include "xrserver_objects_alife.h"
#include "../../build_config_defines.h"
#include "pch_script.h"
#include "game_object_space.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
#include "xrServer_Objects_ALife_Items.h"

#define GRENADE_REMOVE_TIME		30000
const float default_grenade_detonation_threshold_hit=100;
CGrenade::CGrenade(void) 
{
	//
	m_flags.set(Fbelt, TRUE);
	m_weight = .1f;
	SetSlot(GRENADE_SLOT);
	//
	m_eSoundCheckout = ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING);
}

CGrenade::~CGrenade(void) 
{
	HUD_SOUND::DestroySound(sndCheckout);
}

void CGrenade::Load(LPCSTR section) 
{
	inherited::Load(section);
	CExplosive::Load(section);

	HUD_SOUND::LoadSound(section,"snd_checkout",sndCheckout,m_eSoundCheckout);

	//////////////////////////////////////
	//время убирания оружия с уровня
	if(pSettings->line_exist(section,"grenade_remove_time"))
		m_dwGrenadeRemoveTime = pSettings->r_u32(section,"grenade_remove_time");
	else
		m_dwGrenadeRemoveTime = GRENADE_REMOVE_TIME;
	m_grenade_detonation_threshold_hit=READ_IF_EXISTS(pSettings,r_float,section,"detonation_threshold_hit",default_grenade_detonation_threshold_hit);
	//debug
	//Msg("Load [%s] (id [%d]) with time to destroy [%d]", cNameSect().c_str(), ID(), m_dwDestroyTimeMax);
}

void CGrenade::Hit					(SHit* pHDS)
{
	if( ALife::eHitTypeExplosion==pHDS->hit_type && m_grenade_detonation_threshold_hit<pHDS->damage()&&CExplosive::Initiator()==u16(-1)) 
	{
		CExplosive::SetCurrentParentID(pHDS->who->ID());
		Destroy();
	}
	inherited::Hit(pHDS);
}

BOOL CGrenade::net_Spawn(CSE_Abstract* DC) 
{
	m_dwGrenadeIndependencyTime			= 0;
	BOOL ret= inherited::net_Spawn		(DC);
	Fvector box;BoundingBox().getsize	(box);
	float max_size						= _max(_max(box.x,box.y),box.z);
	box.set								(max_size,max_size,max_size);
	box.mul								(3.f);
	CExplosive::SetExplosionSize		(box);
	m_thrown							= false;
	//
	if (auto se_grenade = smart_cast<CSE_ALifeItemGrenade*>(DC))
		if (se_grenade->m_dwDestroyTimeMax != NULL) //загружаем значение задержки из серверного объекта
			m_dwDestroyTimeMax = se_grenade->m_dwDestroyTimeMax;
		else										//попытаемся сгенерировать задержку
		{
			LPCSTR str = pSettings->r_string(cNameSect(), "destroy_time");
			int cnt = _GetItemCount(str);
			if (cnt > 1)							//заданы границы рандомной задержки до взрыва
			{
				Ivector2 m = pSettings->r_ivector2(cNameSect(), "destroy_time");
				m_dwDestroyTimeMax = ::Random.randI(m.x, m.y);
			}
		}
	//debug
	//Msg("net_Spawn [%s] (id [%d]) with time to destroy [%d]", cNameSect().c_str(), ID(), m_dwDestroyTimeMax);
	return								ret;
}

//
void CGrenade::net_Export(NET_Packet& P)
{
	inherited::net_Export(P);
	
	P.w_u32(m_dwDestroyTimeMax);

	//Msg("net_Export [%s] (id [%d]) with time to destroy [%d]", cNameSect().c_str(), ID(), m_dwDestroyTimeMax);
}

void CGrenade::net_Import(NET_Packet& P)
{
	inherited::net_Import(P);

	m_dwDestroyTimeMax = P.r_u32();

	//Msg("net_Import [%s] (id [%d]) with time to destroy [%d]", cNameSect().c_str(), ID(), m_dwDestroyTimeMax);
}

void CGrenade::net_Destroy() 
{
	inherited::net_Destroy				();
	CExplosive::net_Destroy				();
}

void CGrenade::OnH_B_Independent(bool just_before_destroy) 
{
	inherited::OnH_B_Independent(just_before_destroy);
}

void CGrenade::OnH_A_Independent() 
{
	m_dwGrenadeIndependencyTime			= Level().timeServer();
	inherited::OnH_A_Independent		();	
}

void CGrenade::OnH_A_Chield()
{
	m_dwGrenadeIndependencyTime			= 0;
	m_dwDestroyTime						= 0xffffffff;
	inherited::OnH_A_Chield				();
}

void CGrenade::State(u32 state) 
{
	switch (state)
	{
	case MS_THREATEN:
		{
			Fvector						C;
			Center						(C);
			PlaySound					(sndCheckout,C);
		}break;
	case MS_HIDDEN:
		{
			if(m_thrown)
			{
				if (m_pPhysicsShell)	m_pPhysicsShell->Deactivate();
				xr_delete				(m_pPhysicsShell);
				m_dwDestroyTime			= 0xffffffff;
				
				if(H_Parent())
					PutNextToSlot		();

				if (Local())
				{
					//#ifdef DEBUG
					//Msg("Destroying local grenade[%d][%d] with destroy time [%d]", ID(), Device.dwFrame, m_dwDestroyTimeMax);
					//#endif
					DestroyObject		();
				}
				
			};
		}break;
	};
	inherited::State(state);
}


void CGrenade::Throw() 
{
	if (!m_fake_missile || m_thrown)
		return;

	CGrenade					*pGrenade = smart_cast<CGrenade*>(m_fake_missile);
	VERIFY						(pGrenade);
	
	if (pGrenade) {
		pGrenade->set_destroy_time(m_dwDestroyTimeMax);
		//debug
		//Msg("Throw: [%s] (id [%d]) with time to destroy [%d]", cNameSect().c_str(), ID(), m_dwDestroyTimeMax);
		//установить ID того кто кинул гранату
		pGrenade->SetInitiator( H_Parent()->ID() );
	}
	inherited::Throw			();
	m_fake_missile->processing_activate();//@sliph
	m_thrown = true;
	
	// Real Wolf.Start.18.12.14
	auto parent = smart_cast<CGameObject*>(H_Parent());
	auto obj	= smart_cast<CGameObject*>(m_fake_missile);
	if (parent && obj)
	{
		parent->callback(GameObject::eOnThrowGrenade)(obj->lua_game_object());
	}
	// Real Wolf.End.18.12.14
}

void CGrenade::Destroy() 
{
	//Generate Expode event
	Fvector						normal;
	FindNormal					(normal);
	CExplosive::GenExplodeEvent	(Position(), normal);
}



bool CGrenade::Useful() const
{

	bool res = (/* !m_throw && */ m_dwDestroyTime == 0xffffffff && CExplosive::Useful() && TestServerFlag(CSE_ALifeObject::flCanSave));

	return res;
}

void CGrenade::OnEvent(NET_Packet& P, u16 type) 
{
	inherited::OnEvent			(P,type);
	CExplosive::OnEvent			(P,type);
}

void CGrenade::PutNextToSlot()
{
    if (OnClient()) return;
    VERIFY                                    (!getDestroy());

    //выкинуть гранату из инвентаря
    if (m_pCurrentInventory)
    {
        NET_Packet                        P;
        m_pCurrentInventory->Ruck        (this);
        //GRENADE_FROM_BELT
        this->u_EventGen                (P, (/*Belt()*/psActorFlags.test(AF_AMMO_FROM_BELT) ? GEG_PLAYER_ITEM2BELT : GEG_PLAYER_ITEM2RUCK), this->H_Parent()->ID());
        
        P.w_u16                            (this->ID());
        this->u_EventSend                (P);
        //GRENADE_FROM_BELT
        CGrenade *pNext                = smart_cast<CGrenade*>(    m_pCurrentInventory->Same(this, !/*Belt()*/psActorFlags.test(AF_AMMO_FROM_BELT))    );
        if(!pNext)
            pNext                    = smart_cast<CGrenade*>(    m_pCurrentInventory->SameSlot(GRENADE_SLOT, this, !/*Belt()*/psActorFlags.test(AF_AMMO_FROM_BELT)));

        VERIFY                            (pNext != this);

        if(pNext && m_pCurrentInventory->Slot(pNext) )
        {
            pNext->u_EventGen            (P, GEG_PLAYER_ITEM2SLOT, pNext->H_Parent()->ID());
            P.w_u16                        (pNext->ID());
            pNext->u_EventSend            (P);
            m_pCurrentInventory->SetActiveSlot(pNext->GetSlot());
        }
/////    m_thrown                = false;
    }
}

void CGrenade::OnAnimationEnd(u32 state) 
{
	switch(state){
	case MS_END: SwitchState(MS_HIDDEN);	break;
//.	case MS_END: SwitchState(MS_RESTORE);	break;
		default : inherited::OnAnimationEnd(state);
	}
}



void CGrenade::UpdateCL() 
{
	inherited::UpdateCL			();
	CExplosive::UpdateCL		();

	if(!IsGameTypeSingle())	make_Interpolation();
}


bool CGrenade::Action(s32 cmd, u32 flags)
{
	if (inherited::Action(cmd, flags)) return true;

	switch (cmd)
	{
		//переключение типа гранаты
	case kWPN_NEXT:
	{
		if (flags&CMD_START)
		{
			if (m_pCurrentInventory)
			{
				// (c) NanoBot
				xr_vector<shared_str>    types_sect_grn;        // текущий список секций гранат
				// Находим список секций гранат разных типов в активе
				// в m_belt или m_ruck нет гранаты которую актор держит в руках, т.е. this
				types_sect_grn.push_back(this->cNameSect());
				int        count_types = 1;    // текущие количество типов гранат в активе
				//GRENADE_FROM_BELT
				TIItemContainer::iterator    it, it_e;
				if (/*Belt()*/psActorFlags.test(AF_AMMO_FROM_BELT))
				{
					it = m_pCurrentInventory->m_belt.begin();
					it_e = m_pCurrentInventory->m_belt.end();
				}
				else
				{
					it = m_pCurrentInventory->m_ruck.begin();
					it_e = m_pCurrentInventory->m_ruck.end();
				}
				for (; it != it_e; ++it)
				{
					CGrenade *pGrenade = smart_cast<CGrenade*>(*it);
					if (pGrenade)
					{
						// составляем список типов гранат (с) НаноБот
						xr_vector<shared_str>::const_iterator    I = types_sect_grn.begin();
						xr_vector<shared_str>::const_iterator    E = types_sect_grn.end();
						bool    new_type = true;
						for (; I != E; ++I)
						{
							if (!xr_strcmp(pGrenade->cNameSect(), *I)) // если совпадают
								new_type = false;
						}
						if (new_type)    // новый тип гранаты?, добавляем
						{
							types_sect_grn.push_back(pGrenade->cNameSect());
							count_types++;
						}
					}
				}
				// Если типов больше 1 то, сортируем список по алфавиту
				// и находим номер текущей гранаты в списке.
				if (count_types>1)
				{
					int        curr_num = 0;        // номер типа текущей гранаты
					std::sort(types_sect_grn.begin(), types_sect_grn.end());
					xr_vector<shared_str>::const_iterator    I = types_sect_grn.begin();
					xr_vector<shared_str>::const_iterator    E = types_sect_grn.end();
					for (; I != E; ++I)
					{
						if (!xr_strcmp(this->cNameSect(), *I)) // если совпадают
							break;
						curr_num++;
					}
					int        next_num = curr_num + 1;    // номер секции следующей гранаты
					if (next_num >= count_types)    next_num = 0;
					shared_str    sect_next_grn = types_sect_grn[next_num];    // секция следущей гранаты
					// Ищем в активе гранату с секцией следущего типа
					//GRENADE_FROM_BELT
					TIItemContainer::iterator    it, it_e;
					if (/*Belt()*/psActorFlags.test(AF_AMMO_FROM_BELT))
					{
						it = m_pCurrentInventory->m_belt.begin();
						it_e = m_pCurrentInventory->m_belt.end();
					}
					else
					{
						it = m_pCurrentInventory->m_ruck.begin();
						it_e = m_pCurrentInventory->m_ruck.end();
					}
					for (; it != it_e; ++it)
					{
						CGrenade *pGrenade = smart_cast<CGrenade*>(*it);
						if (pGrenade && !xr_strcmp(pGrenade->cNameSect(), sect_next_grn))
						{
							m_pCurrentInventory->Ruck(this);
							m_pCurrentInventory->SetActiveSlot(NO_ACTIVE_SLOT);
							m_pCurrentInventory->Slot(pGrenade);
							//GRENADE_FROM_BELT
							if (/*Belt()*/psActorFlags.test(AF_AMMO_FROM_BELT))
								m_pCurrentInventory->Belt(this);                    // текущую гранату, обратно в пояс.
							return true;
						}
					}
				}
				return true;
			}
		}
		return true;
	};
	}
	return false;
}


bool CGrenade::NeedToDestroyObject()	const
{
	return ( TimePassedAfterIndependant() > m_dwGrenadeRemoveTime);
}

ALife::_TIME_ID	 CGrenade::TimePassedAfterIndependant()	const
{
	if(!H_Parent() && m_dwGrenadeIndependencyTime != 0)
		return Level().timeServer() - m_dwGrenadeIndependencyTime;
	else
		return 0;
}

BOOL CGrenade::UsedAI_Locations		()
{
#pragma todo("Dima to Yura : It crashes, because on net_Spawn object doesn't use AI locations, but on net_Destroy it does use them")
	return TRUE;//m_dwDestroyTime == 0xffffffff;
}

void CGrenade::net_Relcase(CObject* O )
{
	CExplosive::net_Relcase(O);
	inherited::net_Relcase(O);
}

void CGrenade::Deactivate()
{
	//Drop grenade if primed
	m_pHUD->StopCurrentAnimWithoutCallback();
	if (!GetTmpPreDestroy() && Local() && (GetState() == MS_THREATEN || GetState() == MS_READY || GetState() == MS_THROW))
	{
		if (m_fake_missile)
		{
			CGrenade*		pGrenade	= smart_cast<CGrenade*>(m_fake_missile);
			if (pGrenade)
			{
				if (m_pCurrentInventory->GetOwner())
				{
					CActor* pActor = smart_cast<CActor*>(m_pCurrentInventory->GetOwner());
					if (pActor)
					{
						if (!pActor->g_Alive())
						{
							m_constpower			= false;
							m_fThrowForce			= 0;
						}
					}
				}				
				Throw					();
			};
		};
	};

	inherited::Deactivate();
}
#include "hudmanager.h"
#include "ui/UIMainIngameWnd.h"
void CGrenade::GetBriefInfo(xr_string& str_name, xr_string& icon_sect_name, xr_string& str_count)
{
	str_name				= NameShort();
	bool SearchRuck = !psActorFlags.test(AF_AMMO_FROM_BELT);
	u32 ThisGrenadeCount = m_pCurrentInventory->GetSameItemCount(*cNameSect(), SearchRuck);
	string16				stmp;
	auto CurrentHUD = HUD().GetUI()->UIMainIngameWnd;
	
	if (CurrentHUD->IsHUDElementAllowed(eGear))
       sprintf_s			(stmp, "%d", ThisGrenadeCount);
	else
	if (CurrentHUD->IsHUDElementAllowed(eActiveItem))
	   sprintf_s			(stmp, "");
	str_count				= stmp;
	icon_sect_name			= *cNameSect();
}
