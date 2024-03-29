#include "stdafx.h"
#include "customdetector.h"
#include "customzone.h"
#include "hudmanager.h"
#include "artifact.h"
#include "inventory.h"
#include "level.h"
#include "map_manager.h"
#include "cameraEffector.h"
#include "actor.h"
#include "actorcondition.h"
#include "../xr_3da/xrGame/ai/monsters/basemonster/base_monster.h"
#include "ai_sounds.h"

ZONE_INFO::ZONE_INFO	()
{
	pParticle=NULL;
}

ZONE_INFO::~ZONE_INFO	()
{
	if(pParticle)
		CParticlesObject::Destroy(pParticle);
}

CCustomDetector::CCustomDetector(void) 
{
	m_bWorking					= false;
	radiation_snd_time			= 0;
	monster_radiation_snd_time	= 0;
}

CCustomDetector::~CCustomDetector(void) 
{
	ZONE_TYPE_MAP_IT it;
	for(it = m_ZoneTypeMap.begin(); m_ZoneTypeMap.end() != it; ++it)
		HUD_SOUND::DestroySound(it->second.detect_snds);
//		it->second.detect_snd.destroy();

	m_ZoneInfoMap.clear();
}

BOOL CCustomDetector::net_Spawn(CSE_Abstract* DC) 
{
	m_pCurrentActor		 = NULL;
	m_pCurrentInvOwner	 = NULL;

	return		(inherited::net_Spawn(DC));
}

void CCustomDetector::Load(LPCSTR section) 
{
	inherited::Load			(section);

	m_fRadius				= pSettings->r_float(section,"radius");
	
	if( pSettings->line_exist(section,"night_vision_particle") )
		m_nightvision_particle	= pSettings->r_string(section,"night_vision_particle");

	u32 i = 1;
	string256 temp;

	//загрузить звуки для обозначения различных типов зон
	do 
	{
		sprintf_s			(temp, "zone_class_%d", i);
		if(pSettings->line_exist(section,temp))
		{
			LPCSTR z_Class			= pSettings->r_string(section,temp);
			CLASS_ID zone_cls		= TEXT2CLSID(pSettings->r_string(z_Class,"class"));

			m_ZoneTypeMap.insert	(std::make_pair(zone_cls,ZONE_TYPE()));
			ZONE_TYPE& zone_type	= m_ZoneTypeMap[zone_cls];
			sprintf_s					(temp, "zone_min_freq_%d", i);
			zone_type.min_freq		= pSettings->r_float(section,temp);
			sprintf_s					(temp, "zone_max_freq_%d", i);
			zone_type.max_freq		= pSettings->r_float(section,temp);
			R_ASSERT				(zone_type.min_freq<zone_type.max_freq);
			sprintf_s					(temp, "zone_sound_%d_", i);

			HUD_SOUND::LoadSound(section, temp	,zone_type.detect_snds		, SOUND_TYPE_ITEM);

			sprintf_s					(temp, "zone_map_location_%d", i);
			
			if( pSettings->line_exist(section,temp) )
				zone_type.zone_map_location = pSettings->r_string(section,temp);

			++i;
		}
		else break;
	} while(true);

	m_ef_detector_type	= pSettings->r_u32(section,"ef_detector_type");
	m_detect_actor_radiation = !!READ_IF_EXISTS(pSettings, r_bool, section, "detect_actor_radiation", false);
}


void CCustomDetector::shedule_Update(u32 dt) 
{
	inherited::shedule_Update	(dt);
	
	if( !IsWorking() ) return;
	if( !H_Parent()  ) return;

	Position().set(H_Parent()->Position());

	if (H_Parent() && H_Parent() == Level().CurrentViewEntity())
	{
		Fvector					P; 
		P.set					(H_Parent()->Position());
		feel_touch_update		(P,m_fRadius);
		UpdateNightVisionMode();
	}
}

void CCustomDetector::StopAllSounds()
{
	ZONE_TYPE_MAP_IT it;
	for(it = m_ZoneTypeMap.begin(); m_ZoneTypeMap.end() != it; ++it) 
	{
		ZONE_TYPE& zone_type = (*it).second;
		HUD_SOUND::StopSound(zone_type.detect_snds);
//		zone_type.detect_snd.stop();
	}
}

void CCustomDetector::UpdateCL() 
{
	inherited::UpdateCL();

	if( !IsWorking() ) return;
	if( !H_Parent()  ) return;

	if(!m_pCurrentActor) return;

	ZONE_INFO_MAP_IT it;
	for(it = m_ZoneInfoMap.begin(); m_ZoneInfoMap.end() != it; ++it) 
	{
		CCustomZone *pZone = it->first;
		ZONE_INFO& zone_info = it->second;

		
		//такой тип зон не обнаруживается
		if(m_ZoneTypeMap.find(pZone->CLS_ID) == m_ZoneTypeMap.end() ||
			!pZone->VisibleByDetector())
			continue;

		ZONE_TYPE& zone_type = m_ZoneTypeMap[pZone->CLS_ID];

		CSpaceRestrictor *pSR = smart_cast<CSpaceRestrictor*>(pZone);
		float dist_to_zone = pSR->distance_to(H_Parent()->Position()); //H_Parent()->Position().distance_to(pZone->Position()) - 0.8f*pZone->Radius();
		if (dist_to_zone > m_fRadius)
			continue;
		if(dist_to_zone<0) dist_to_zone = 0;
		
		float fRelPow = 1.f - dist_to_zone / m_fRadius;
		clamp(fRelPow, 0.f, 1.f);

		//определить текущую частоту срабатывания сигнала
		zone_info.cur_freq = zone_type.min_freq + 
			(zone_type.max_freq - zone_type.min_freq) * fRelPow* fRelPow* fRelPow* fRelPow;

		float current_snd_time = 1000.f*1.f/zone_info.cur_freq;
			
		if((float)zone_info.snd_time > current_snd_time)
		{
			zone_info.snd_time	= 0;
			HUD_SOUND::PlaySound	(zone_type.detect_snds, Fvector().set(0,0,0), this, true, false);

		} 
		else 
			zone_info.snd_time += Device.dwTimeDelta;
	}

	UpdateActorRadiation();
	UpdateMonsterRadiation();
}

void CCustomDetector::feel_touch_new(CObject* O) 
{
	CCustomZone *pZone = smart_cast<CCustomZone*>(O);
	if(pZone && pZone->IsEnabled()) 
	{
		m_ZoneInfoMap[pZone].snd_time = 0;
		
		AddRemoveMapSpot(pZone,true);
	}
}

void CCustomDetector::feel_touch_delete(CObject* O)
{
	CCustomZone *pZone = smart_cast<CCustomZone*>(O);
	if(pZone)
	{
		m_ZoneInfoMap.erase(pZone);
		AddRemoveMapSpot(pZone,false);
	}
}

BOOL CCustomDetector::feel_touch_contact(CObject* O) 
{
	return (smart_cast<CCustomZone*>(O) || smart_cast<CBaseMonster*>(O));
}

void CCustomDetector::OnH_A_Chield() 
{
	m_pCurrentActor				= smart_cast<CActor*>(H_Parent());
	m_pCurrentInvOwner			= smart_cast<CInventoryOwner*>(H_Parent());
	inherited::OnH_A_Chield		();
}

void CCustomDetector::OnH_B_Independent(bool just_before_destroy) 
{
	inherited::OnH_B_Independent(just_before_destroy);
	
	m_pCurrentActor				= NULL;
	m_pCurrentInvOwner			= NULL;

	StopAllSounds				();

	m_ZoneInfoMap.clear			();
	Feel::Touch::feel_touch.clear();
}


u32	CCustomDetector::ef_detector_type	() const
{
	return	(m_ef_detector_type);
}

void CCustomDetector::OnMoveToRuck(EItemPlace previous_place)
{
	inherited::OnMoveToRuck(previous_place);
	TurnOff();
}

void CCustomDetector::OnMoveToSlot(EItemPlace previous_place)
{
	inherited::OnMoveToSlot	(previous_place);
	TurnOn					();
}

void CCustomDetector::OnMoveToBelt(EItemPlace previous_place)
{
	inherited::OnMoveToBelt	(previous_place);
	TurnOn					();
}

void CCustomDetector::TurnOn()
{
	m_bWorking				= true;
	UpdateMapLocations		();
	UpdateNightVisionMode	();
}

void CCustomDetector::TurnOff() 
{
	m_bWorking				= false;
	UpdateMapLocations		();
	UpdateNightVisionMode	();
}

void CCustomDetector::AddRemoveMapSpot(CCustomZone* pZone, bool bAdd)
{
	if(m_ZoneTypeMap.find(pZone->CLS_ID) == m_ZoneTypeMap.end() )return;
	
	if ( bAdd && !pZone->VisibleByDetector() ) return;
		

	ZONE_TYPE& zone_type = m_ZoneTypeMap[pZone->CLS_ID];
	if( xr_strlen(zone_type.zone_map_location) ){
		if( bAdd )
			Level().MapManager().AddMapLocation(*zone_type.zone_map_location,pZone->ID());
		else
			Level().MapManager().RemoveMapLocation(*zone_type.zone_map_location,pZone->ID());
	}
}

void CCustomDetector::UpdateMapLocations() // called on turn on/off only
{
	bool in_slot = m_pCurrentInventory && m_pCurrentInventory->ItemFromSlot(DETECTOR_SLOT) == this;

	ZONE_INFO_MAP_IT it;
	for(it = m_ZoneInfoMap.begin(); it != m_ZoneInfoMap.end(); ++it)
		AddRemoveMapSpot(it->first, IsWorking() && in_slot);
}

#include "clsid_game.h"
#include "game_base_space.h"
void CCustomDetector::UpdateNightVisionMode()
{
//	CObject* tmp = Level().CurrentViewEntity();	
	bool bNightVision = false;
	if (GameID() == GAME_SINGLE)
	{
		bNightVision = Actor()->Cameras().GetPPEffector(EEffectorPPType(effNightvision))!=NULL;
	}
	else
	{
		if (Level().CurrentViewEntity() && 
			Level().CurrentViewEntity()->CLS_ID == CLSID_OBJECT_ACTOR)
		{
			CActor* pActor = smart_cast<CActor*>(Level().CurrentViewEntity());
			if (pActor)
				bNightVision = pActor->Cameras().GetPPEffector(EEffectorPPType(effNightvision))!=NULL;
		}
	}

	bool bOn =	bNightVision && 
				m_pCurrentActor &&
				m_pCurrentActor==Level().CurrentViewEntity()&& 
				IsWorking() && 
				m_nightvision_particle.size();

	ZONE_INFO_MAP_IT it;
	for(it = m_ZoneInfoMap.begin(); m_ZoneInfoMap.end() != it; ++it) 
	{
		CCustomZone *pZone = it->first;
		ZONE_INFO& zone_info = it->second;

		if(bOn){
			Fvector zero_vector;
			zero_vector.set(0.f,0.f,0.f);

			if(!zone_info.pParticle)
				zone_info.pParticle = CParticlesObject::Create(*m_nightvision_particle,FALSE);
			
			zone_info.pParticle->UpdateParent(pZone->XFORM(),zero_vector);
			if(!zone_info.pParticle->IsPlaying())
				zone_info.pParticle->Play();
		}else{
			if(zone_info.pParticle){
				zone_info.pParticle->Stop			();
				CParticlesObject::Destroy(zone_info.pParticle);
			}
		}
	}
}

bool CCustomDetector::IsGeigerCounter()
{
	ZONE_TYPE_MAP_IT it;
	for (it = m_ZoneTypeMap.begin(); m_ZoneTypeMap.end() != it; ++it)
	{
		CLASS_ID zone_type = (*it).first;

		if (zone_type == TEXT2CLSID(pSettings->r_string("zone_radioactive", "class")))
			return true;
	}

	return !psActorFlags.is(AF_ARTEFACT_DETECTOR_CHECK);
}

bool CCustomDetector::IsAnomDetector()
{
	ZONE_TYPE_MAP_IT it;
	for (it = m_ZoneTypeMap.begin(); m_ZoneTypeMap.end() != it; ++it)
	{
		CLASS_ID zone_type = (*it).first;

		if (zone_type != TEXT2CLSID(pSettings->r_string("zone_radioactive", "class")))
			return true;
	}

	return !psActorFlags.is(AF_ARTEFACT_DETECTOR_CHECK);
}

void CCustomDetector::UpdateActorRadiation()
{
	if (!m_detect_actor_radiation) return;
	if (m_ZoneTypeMap.find(CLSID_Z_RADIO) == m_ZoneTypeMap.end()) return;
	ZONE_TYPE& zone_type = m_ZoneTypeMap[CLSID_Z_RADIO];

	float fRelPow = m_pCurrentActor->conditions().GetRadiation();
	if (fis_zero(fRelPow)) 
	{
		radiation_snd_time = 0;
		return;
	}

	float cur_freq = zone_type.min_freq
		+ (zone_type.max_freq - zone_type.min_freq)
		* fRelPow * fRelPow * fRelPow * fRelPow;
	float current_snd_time = 1000.f * 1.f / cur_freq;

	if (radiation_snd_time > current_snd_time) 
	{
		radiation_snd_time = 0;
		HUD_SOUND::PlaySound(zone_type.detect_snds, Fvector().set(0, 0, 0), this, true, false);
	}
	else
		radiation_snd_time += Device.dwTimeDelta;
}

void CCustomDetector::UpdateMonsterRadiation()
{
	if (m_ZoneTypeMap.find(CLSID_Z_RADIO) == m_ZoneTypeMap.end()) return;
	ZONE_TYPE& zone_type = m_ZoneTypeMap[CLSID_Z_RADIO];

	for (auto object : feel_touch)
	{
		auto const	Monster = smart_cast<CBaseMonster*>(object);
		if (!Monster)	continue;

		if (!Monster->HasAffectHit() || Monster->m_eAffectHitType != ALife::eHitTypeRadiation)
			continue;

		float distance = H_Parent()->Position().distance_to(Monster->Position());
		if (distance > m_fRadius || distance > Monster->m_fAffectDistance) 
			continue;

		if (!Monster->g_Alive() && !Monster->m_bAffectDead)
			continue;

		float fRelPow = 1.f - distance / Monster->m_fAffectDistance;//m_fRadius;

		//определить текущую частоту срабатывания сигнала
		float cur_freq = zone_type.min_freq
			+ (zone_type.max_freq - zone_type.min_freq)
			* fRelPow * fRelPow * fRelPow * fRelPow;
		float current_snd_time = 1000.f * 1.f / cur_freq;

		if (monster_radiation_snd_time > current_snd_time)
		{
			monster_radiation_snd_time = 0;
			HUD_SOUND::PlaySound(zone_type.detect_snds, Fvector().set(0, 0, 0), this, true, false);
		}
		else
			monster_radiation_snd_time += Device.dwTimeDelta;
	}
}