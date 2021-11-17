#include "stdafx.h"
#include "hudmanager.h"
#include "WeaponHUD.h"
#include "WeaponMagazined.h"
#include "entity.h"
#include "actor.h"
#include "ParticlesObject.h"
#include "scope.h"
#include "silencer.h"
#include "GrenadeLauncher.h"
#include "inventory.h"
#include "xrserver_objects_alife_items.h"
#include "ActorEffector.h"
#include "EffectorZoomInertion.h"
#include "xr_level_controller.h"
#include "level.h"
#include "object_broker.h"
#include "string_table.h"

// Headers included by Cribbledirge (for callbacks).
#include "pch_script.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
#include "game_object_space.h"

#include "uigamecustom.h"

#include "../../build_config_defines.h"

CWeaponMagazined::CWeaponMagazined(LPCSTR name, ESoundTypes eSoundType) : CWeapon(name)
{
	m_eSoundShow		= ESoundTypes(SOUND_TYPE_ITEM_TAKING | eSoundType);
	m_eSoundHide		= ESoundTypes(SOUND_TYPE_ITEM_HIDING | eSoundType);
	m_eSoundShot		= ESoundTypes(SOUND_TYPE_WEAPON_SHOOTING | eSoundType);
	m_eSoundEmptyClick	= ESoundTypes(SOUND_TYPE_WEAPON_EMPTY_CLICKING | eSoundType);
	m_eSoundReload		= ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING | eSoundType);
	m_eSoundSightsUp	= ESoundTypes(SOUND_TYPE_WORLD_AMBIENT | eSoundType);	//added by Daemonion for iron sight audio in weapon parameters - sights being raised
	m_eSoundSightsDown	= ESoundTypes(SOUND_TYPE_WORLD_AMBIENT | eSoundType);	//added by Daemonion for iron sight audio in weapon parameters - sights being lowered
	//
	m_eSoundShutter		= ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING | eSoundType);

	m_pSndShotCurrent = NULL;
	m_sSilencerFlameParticles = m_sSilencerSmokeParticles = NULL;

	m_bFireSingleShot = false;
	m_iShotNum = 0;
	m_iQueueSize = WEAPON_ININITE_QUEUE;
	m_bLockType = false;
	m_class_name = get_class_name<CWeaponMagazined>(this);
	//
	m_bHasDetachableMagazine	= true;
	m_bHasChamber				= true;
}

CWeaponMagazined::~CWeaponMagazined()
{
	// sounds
	HUD_SOUND::DestroySound(sndShow);
	HUD_SOUND::DestroySound(sndHide);
	HUD_SOUND::DestroySound(sndShot);
	HUD_SOUND::DestroySound(sndSilencerShot);
	HUD_SOUND::DestroySound(sndEmptyClick);
	HUD_SOUND::DestroySound(sndReload);
	HUD_SOUND::DestroySound(sndSightsUp);		//added by Daemonion for ironsight audio in weapon parameters - sights being raised
	HUD_SOUND::DestroySound(sndSightsDown);		//added by Daemonion for ironsight audio in weapon parameters - sights being lowered
	//
	HUD_SOUND::DestroySound(sndShutter);
}

void CWeaponMagazined::StopHUDSounds()
{
	HUD_SOUND::StopSound(sndShow);
	HUD_SOUND::StopSound(sndHide);

	HUD_SOUND::StopSound(sndEmptyClick);
	HUD_SOUND::StopSound(sndReload);

	HUD_SOUND::StopSound(sndShot);
	HUD_SOUND::StopSound(sndSilencerShot);
	HUD_SOUND::StopSound(sndSightsUp);			//added by Daemonion for ironsight audio in weapon parameters - sights being raised
	HUD_SOUND::StopSound(sndSightsDown);		//added by Daemonion for ironsight audio in weapon parameters - sights being lowered
	//
	HUD_SOUND::StopSound(sndShutter);
	//.	if(sndShot.enable && sndShot.snd.feedback)
	//.		sndShot.snd.feedback->switch_to_3D();

	inherited::StopHUDSounds();
}

void CWeaponMagazined::net_Destroy()
{
	inherited::net_Destroy();
}

void CWeaponMagazined::Load(LPCSTR section)
{
	inherited::Load(section);

	// Sounds
	HUD_SOUND::LoadSound(section, "snd_draw", sndShow, m_eSoundShow);
	HUD_SOUND::LoadSound(section, "snd_holster", sndHide, m_eSoundHide);
	HUD_SOUND::LoadSound(section, "snd_shoot", sndShot, m_eSoundShot);
	HUD_SOUND::LoadSound(section, "snd_empty", sndEmptyClick, m_eSoundEmptyClick);
	HUD_SOUND::LoadSound(section, "snd_reload", sndReload, m_eSoundReload);
	HUD_SOUND::LoadSound(section, "snd_SightsUp", sndSightsUp, m_eSoundSightsUp);		//added by Daemonion for ironsight audio in weapon parameters - sights being raised
	HUD_SOUND::LoadSound(section, "snd_SightsDown", sndSightsDown, m_eSoundSightsDown);	//added by Daemonion for ironsight audio in weapon parameters - sights being lowered
	//
	bool b_shutter_sound = !!pSettings->line_exist(section, "snd_shutter");
	HUD_SOUND::LoadSound(section, b_shutter_sound ? "snd_shutter" : "snd_draw", sndShutter, m_eSoundShutter);


	m_pSndShotCurrent = &sndShot;

	// HUD :: Anims
	R_ASSERT(m_pHUD);
	animGetEx(mhud.mhud_idle, "anim_idle");
	animGetEx(mhud.mhud_idle_moving, pSettings->line_exist(hud_sect.c_str(), "anim_idle_moving") ? "anim_idle_moving" : "anim_idle");
	animGetEx(mhud.mhud_idle_sprint, pSettings->line_exist(hud_sect.c_str(), "anim_idle_sprint") ? "anim_idle_sprint" : "anim_idle");
	animGetEx(mhud.mhud_reload, "anim_reload");
	animGetEx(mhud.mhud_show, "anim_draw");
	animGetEx(mhud.mhud_hide, "anim_holster");
	animGetEx(mhud.mhud_shots, "anim_shoot");

	if (IsZoomEnabled())
		animGetEx(mhud.mhud_idle_aim, "anim_idle_aim");

	animGetEx(mhud.mhud_reload_partly, "anim_reload_partly", nullptr, "anim_reload");
	//
	animGetEx(mhud.mhud_shutter, pSettings->line_exist(hud_sect.c_str(), "anim_shutter") ? "anim_shutter" : "anim_draw");

	//звуки и партиклы глушителя, еслит такой есть
	if (m_eSilencerStatus == ALife::eAddonAttachable)
	{
		if (pSettings->line_exist(section, "silencer_flame_particles"))
			m_sSilencerFlameParticles = pSettings->r_string(section, "silencer_flame_particles");
		if (pSettings->line_exist(section, "silencer_smoke_particles"))
			m_sSilencerSmokeParticles = pSettings->r_string(section, "silencer_smoke_particles");
		HUD_SOUND::LoadSound(section, "snd_silncer_shot", sndSilencerShot, m_eSoundShot);
	}
	//  [7/20/2005]
	if (pSettings->line_exist(section, "dispersion_start"))
		m_iShootEffectorStart = pSettings->r_u8(section, "dispersion_start");
	else
		m_iShootEffectorStart = 0;
	//  [7/20/2005]
	//  [7/21/2005]
	if (pSettings->line_exist(section, "fire_modes"))
	{
		m_bHasDifferentFireModes = true;
		shared_str FireModesList = pSettings->r_string(section, "fire_modes");
		int ModesCount = _GetItemCount(FireModesList.c_str());
		m_aFireModes.clear();
		for (int i = 0; i < ModesCount; i++)
		{
			string16 sItem;
			_GetItem(FireModesList.c_str(), i, sItem);
			int FireMode = atoi(sItem);
			m_aFireModes.push_back(FireMode);
		}
		m_iCurFireMode = ModesCount - 1;
		m_iPrefferedFireMode = READ_IF_EXISTS(pSettings, r_s16, section, "preffered_fire_mode", -1);
		//
		if (pSettings->line_exist(section, "preffered_fire_mode"))
		{
			fTimeToFirePreffered = READ_IF_EXISTS(pSettings, r_float, section, "preffered_fire_mode_rpm", fTimeToFire); //скорострельность привилегированного режима стрельбы
			VERIFY(fTimeToFirePreffered>0.f);
			fTimeToFirePreffered = 60.f / fTimeToFirePreffered;
		}
	}
	else
		m_bHasDifferentFireModes = false;
	//  [7/21/2005]
	if (pSettings->line_exist(section, "has_detachable_magazine"))
		m_bHasDetachableMagazine = !!pSettings->r_bool(section, "has_detachable_magazine");
	if (pSettings->line_exist(section, "has_chamber"))
		m_bHasChamber = !!pSettings->r_bool(section, "has_chamber");
}

void CWeaponMagazined::FireStart()
{
	if (IsValid() && !IsMisfire())
	{
		if (!IsWorking() || AllowFireWhileWorking())
		{
			if (GetState() == eReload) return;
			if (GetState() == eShowing) return;
			if (GetState() == eHiding) return;
			if (GetState() == eMisfire) return;

			inherited::FireStart();

			if (iAmmoElapsed == 0)
				OnMagazineEmpty();
			else
				SwitchState(eFire);
		}
	}
	else if (IsMisfire())
	{
		if (smart_cast<CActor*>(this->H_Parent()) && (Level().CurrentViewEntity() == H_Parent()))
		{
			HUD().GetUI()->UIGame()->RemoveCustomStatic("gun_not_jammed");
			HUD().GetUI()->AddInfoMessage("gun_jammed");
		}
		// Callbacks added by Cribbledirge.
		StateSwitchCallback(GameObject::eOnActorWeaponJammed, GameObject::eOnNPCWeaponJammed);
	}
	else
	{
		if (eReload != GetState() && eMisfire != GetState()) OnMagazineEmpty();
	}
}

void CWeaponMagazined::FireEnd()
{
	inherited::FireEnd();

	CActor	*actor = smart_cast<CActor*>(H_Parent());
	if (!iAmmoElapsed && actor && GetState() != eReload && !psActorFlags.test(AF_NO_AUTO_RELOAD))
		Reload();
}

void CWeaponMagazined::Reload()
{
	inherited::Reload();

	TryReload();
}

// Real Wolf: Одна реализация на все участки кода.20.01.15
bool CWeaponMagazined::TryToGetAmmo(u32 id)
{
	m_pAmmo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmo(*m_ammoTypes[id], ParentIsActor()));

	return m_pAmmo != NULL;
}

bool CWeaponMagazined::TryReload()
{
	if (m_pCurrentInventory)
	{
		if (TryToGetAmmo(m_ammoType) || unlimited_ammo() || (IsMisfire() && iAmmoElapsed))
		{
			m_bPending	= true;
			SwitchState(eReload);
			return true;
		}

		for (u32 i = 0; i < m_ammoTypes.size(); ++i)
		{
			if (TryToGetAmmo(i))
			{
				m_ammoType	= i;
				m_bPending	= true;
				SwitchState(eReload);
				return true;
			}
		}
	}

	SwitchState(eIdle);

	return false;
}

bool CWeaponMagazined::IsAmmoAvailable()
{
	if (TryToGetAmmo(m_ammoType))
		return true;
	
	for (u32 i = 0; i < m_ammoTypes.size(); ++i)
	{
		if (TryToGetAmmo(i))
			return true;
	}

	return false;
	
}

void CWeaponMagazined::OnMagazineEmpty()
{
	//попытка стрелять когда нет патронов
	if (GetState() == eIdle)
	{
		OnEmptyClick();
		return;
	}

	if (GetNextState() != eMagEmpty && GetNextState() != eReload)
	{
		SwitchState(eMagEmpty);
	}

	inherited::OnMagazineEmpty();
}

void CWeaponMagazined::UnloadAmmo(int unload_count, bool spawn_ammo)
{
	xr_map<LPCSTR, u16> l_ammo;
	for (int i = 0; i < unload_count; ++i)
	{
		CCartridge &l_cartridge = m_magazine.back();

			if (!l_ammo[*l_cartridge.m_ammoSect])
				l_ammo[*l_cartridge.m_ammoSect] = 1;
			else
				l_ammo[*l_cartridge.m_ammoSect]++;

			m_magazine.pop_back();
			--iAmmoElapsed;
	}


	VERIFY((u32)iAmmoElapsed == m_magazine.size());

	if (!spawn_ammo)
		return;

	xr_map<LPCSTR, u16>::iterator l_it;
	for (l_it = l_ammo.begin(); l_ammo.end() != l_it; ++l_it)
	{
		if (m_pCurrentInventory && !HasDetachableMagazine()/*(!psActorFlags.test(AF_AMMO_BOX_AS_MAGAZINE) || !HasDetachableMagazine())*/) //упаковать разряжаемые патроны в неполную пачку
		{
			CWeaponAmmo *l_pA = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmo(l_it->first, ParentIsActor()));

			if (l_pA)
			{
				u16 l_free = l_pA->m_boxSize - l_pA->m_boxCurr;
				l_pA->m_boxCurr = l_pA->m_boxCurr + (l_free < l_it->second ? l_free : l_it->second);
				l_it->second = l_it->second - (l_free < l_it->second ? l_free : l_it->second);
			}
		}
		if (l_it->second && !unlimited_ammo()) SpawnAmmo(l_it->second, l_it->first);
	}
}

void CWeaponMagazined::UnloadMagazine(bool spawn_ammo)
{
	if (HasChamber() && HasDetachableMagazine())
	{
		HandleCartridgeInChamber();
		UnloadAmmo(iAmmoElapsed - 1, spawn_ammo);
		/*if (!m_bLockType &&	(!m_pAmmo || xr_strcmp(m_pAmmo->cNameSect(), *m_magazine.back().m_ammoSect)))
		{
			ShutterAction();
			Msg("[Unload ammo on type change]");
		}*/
	}
	else
		UnloadAmmo(iAmmoElapsed, spawn_ammo);
}

void CWeaponMagazined::HandleCartridgeInChamber()
{
	if (!HasChamber() || !HasDetachableMagazine())
		return;
	//отстрел и заряжание нового патрона идёт от конца вектора m_magazine.back() - первым подаётся последний добавленный патрон
	if (*m_magazine.back().m_ammoSect != *m_magazine.front().m_ammoSect) //первый и последний патрон различны, значит зарядка смешанная
	{
		if (*m_magazine[m_magazine.size() - 2].m_ammoSect == *m_magazine.front().m_ammoSect) //предпоследний патрон аналогичен первому
		{//начало разряжания магазина
			//перекладываем патрон отличного типа (последний заряженный, он же первый на отстрел) из конца вектора (реверсного начала) в начало (реверсный конец)
			Msg("weapon:[%s]|back:[%s]|front:[%s]|[size() - 2]:[%s] on unloading", Name_script(), *m_magazine.back().m_ammoSect, *m_magazine.front().m_ammoSect, *m_magazine[m_magazine.size() - 1].m_ammoSect);
			rotate(m_magazine.rbegin(), m_magazine.rbegin() + 1, m_magazine.rend());
			Msg("weapon:[%s]|back:[%s]|front:[%s]|[size() - 2]:[%s] after rotate on unloading", Name_script(), *m_magazine.back().m_ammoSect, *m_magazine.front().m_ammoSect, *m_magazine[m_magazine.size() - 1].m_ammoSect);
		}
		else if (*m_magazine[1].m_ammoSect == *m_magazine.back().m_ammoSect) //второй патрон аналогичен последнему патрону
		{//конец заряжания магазина
			//перекладываем патрон отличного типа (первый заряженный, он же последний на отстрел) из начала вектора в конец
			Msg("weapon:[%s]|back:[%s]|front:[%s]|[1]:[%s] on reloading", Name_script(), *m_magazine.back().m_ammoSect, *m_magazine.front().m_ammoSect, *m_magazine[1].m_ammoSect);
			rotate(m_magazine.begin(), m_magazine.begin() + 1, m_magazine.end());
			Msg("weapon:[%s]|back:[%s]|front:[%s]|[1]:[%s] after rotate on reloading", Name_script(), *m_magazine.back().m_ammoSect, *m_magazine.front().m_ammoSect, *m_magazine[1].m_ammoSect);
		}
	}
}

void CWeaponMagazined::UpdateExpansionToChamber()
{
	if (!HasChamber() || !HasDetachableMagazine())
		return;

	int base_iMagazineSize = pSettings->r_s32(cNameSect(), "ammo_mag_size");
	bool b_base_magsize = iMagazineSize == base_iMagazineSize;

	if (!b_base_magsize && m_magazine.empty()/* || m_set_next_ammoType_on_reload != u32(-1)*/)
	{
		iMagazineSize = base_iMagazineSize;
		Msg("[Set original iMagazineSize]");
	}
	else
	if (!m_magazine.empty() && b_base_magsize)
	{
		++iMagazineSize;
		Msg("[++iMagazineSize]");
	}
}

void CWeaponMagazined::ReloadMagazine()
{
	m_dwAmmoCurrentCalcFrame = 0;

	//устранить осечку при перезарядке
	if (IsMisfire() && (!HasChamber() || m_magazine.empty()))
	{
		bMisfire = false;
		if (smart_cast<CActor*>(this->H_Parent()) && (Level().CurrentViewEntity() == H_Parent()))
		{
			HUD().GetUI()->UIGame()->RemoveCustomStatic("gun_jammed");
			HUD().GetUI()->AddInfoMessage("gun_not_jammed");
		}
	}

	//переменная блокирует использование
	//только разных типов патронов
	//	static bool l_lockType = false;
	if (!m_bLockType) {
		m_ammoName = NULL;
		m_pAmmo = NULL;
	}

	if (!m_pCurrentInventory) return;

	if (m_set_next_ammoType_on_reload != u32(-1)){
		m_ammoType = m_set_next_ammoType_on_reload;
		m_set_next_ammoType_on_reload = u32(-1);
	}

	if (!unlimited_ammo())
	{
		//попытаться найти в инвентаре патроны текущего типа
		m_pAmmo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmo(*m_ammoTypes[m_ammoType], ParentIsActor()));
		
		if (!m_pAmmo && !m_bLockType)
		{
			for (u32 i = 0; i < m_ammoTypes.size(); ++i)
			{
				//проверить патроны всех подходящих типов
				m_pAmmo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmo(*m_ammoTypes[i], ParentIsActor()));
		
				if (m_pAmmo)
				{
					m_ammoType = i;
					break;
				}
			}
		}
	}
	else
		m_ammoType = m_ammoType;

	//нет патронов для перезарядки
	if (!m_pAmmo && !unlimited_ammo()) return;

	//разрядить магазин, если загружаем патронами другого типа
	if (!m_bLockType && !m_magazine.empty() &&
		(!m_pAmmo || xr_strcmp(m_pAmmo->cNameSect(), *m_magazine.back().m_ammoSect) ||
		psActorFlags.test(AF_AMMO_BOX_AS_MAGAZINE) && !unlimited_ammo())) //разряжать магазин и при перезарядке, если включена опция
		UnloadMagazine();

	VERIFY((u32)iAmmoElapsed == m_magazine.size());

	if (m_DefaultCartridge.m_LocalAmmoType != m_ammoType)
		m_DefaultCartridge.Load(*m_ammoTypes[m_ammoType], u8(m_ammoType));
	CCartridge l_cartridge = m_DefaultCartridge;
	while (iAmmoElapsed < iMagazineSize)
	{
		if (!unlimited_ammo())
		{
			if (!m_pAmmo->Get(l_cartridge)) break;
		}
		++iAmmoElapsed;
		l_cartridge.m_LocalAmmoType = u8(m_ammoType);
		m_magazine.push_back(l_cartridge);
	}
	m_ammoName = (m_pAmmo) ? m_pAmmo->m_nameShort : NULL;

	VERIFY((u32)iAmmoElapsed == m_magazine.size());

	//выкинуть коробку патронов, если она пустая
	if (m_pAmmo && !m_pAmmo->m_boxCurr && OnServer())
		m_pAmmo->SetDropManual(TRUE);

	if (iMagazineSize > iAmmoElapsed && !psActorFlags.test(AF_AMMO_BOX_AS_MAGAZINE)) //дозарядить оружие до полного магазина, если опция выключена
	{
		m_bLockType = true;
		ReloadMagazine();
		m_bLockType = false;
	}

	VERIFY((u32)iAmmoElapsed == m_magazine.size());
}

// Function for callbacks added by Cribbledirge.
void CWeaponMagazined::StateSwitchCallback(GameObject::ECallbackType actor_type, GameObject::ECallbackType npc_type)
{
	xr_string ammo_type;
	if (GetAmmoElapsed() == 0 || m_magazine.empty())
	{
		ammo_type = *m_ammoTypes[m_ammoType];
	}
	else
	{
		ammo_type = *m_ammoTypes[m_magazine.back().m_LocalAmmoType];
	}

	if (g_actor)
	{
		if (smart_cast<CActor*>(H_Parent()))  // This is an actor.
		{
			Actor()->callback(actor_type)(
				lua_game_object(),  // The weapon as a game object.
				ammo_type.c_str()   // The caliber of the weapon.
			);
		}
		else if (smart_cast<CEntityAlive*>(H_Parent()))  // This is an NPC.
		{
			Actor()->callback(npc_type)(
				smart_cast<CEntityAlive*>(H_Parent())->lua_game_object(),       // The owner of the weapon.
				lua_game_object(),                                              // The weapon itself.
                                ammo_type.c_str()                               // The caliber of the weapon.
			);
		}
	}
}

void CWeaponMagazined::OnStateSwitch(u32 S)
{
	inherited::OnStateSwitch(S);
	switch (S)
	{
	case eIdle:
		switch2_Idle();
		break;
	case eFire:
		switch2_Fire();
		break;
	case eFire2:
		switch2_Fire2();
		break;
	case eMisfire:
		/*if (smart_cast<CActor*>(this->H_Parent()) && (Level().CurrentViewEntity() == H_Parent()))
		{
			HUD().GetUI()->UIGame()->RemoveCustomStatic("gun_not_jammed");
			HUD().GetUI()->AddInfoMessage("gun_jammed");
		}*/
		// Callbacks added by Cribbledirge.
		StateSwitchCallback(GameObject::eOnActorWeaponJammed, GameObject::eOnNPCWeaponJammed);
		break;
	case eMagEmpty:
		switch2_Empty();
		// Callbacks added by Cribbledirge.
		StateSwitchCallback(GameObject::eOnActorWeaponEmpty, GameObject::eOnNPCWeaponEmpty);
		break;
	case eReload:
		switch2_Reload();
		// Callbacks added by Cribbledirge.
		StateSwitchCallback(GameObject::eOnActorWeaponReload, GameObject::eOnNPCWeaponReload);
		break;
	case eShowing:
		switch2_Showing();
		break;
	case eHiding:
		switch2_Hiding();
		break;
	case eHidden:
		switch2_Hidden();
		break;
	case eShutter:
		switch2_Shutter();
		break;
	}
}

void CWeaponMagazined::UpdateCL()
{
	inherited::UpdateCL();
	float dt = Device.fTimeDelta;

	//когда происходит апдейт состояния оружия
	//ничего другого не делать
	if (GetNextState() == GetState())
	{
		switch (GetState())
		{
		case eShowing:
		case eHiding:
		case eReload:
		case eIdle:
			fTime -= dt;
			if (fTime < 0)
				fTime = 0;
			break;
		case eFire:
			if (iAmmoElapsed>0)
				state_Fire(dt);

			if (fTime <= 0)
			{
				if (iAmmoElapsed == 0)
					OnMagazineEmpty();
				StopShooting();
			}
			else
			{
				fTime -= dt;
			}

			break;
		case eMisfire:		state_Misfire(dt);	break;
		case eMagEmpty:		state_MagEmpty(dt);	break;
		case eHidden:		break;
		}
	}

	UpdateSounds();
}

void CWeaponMagazined::UpdateSounds()
{
	if (Device.dwFrame == dwUpdateSounds_Frame)
		return;

	dwUpdateSounds_Frame = Device.dwFrame;

	// ref_sound positions
	if (sndShow.playing			())	sndShow.set_position			(get_LastFP());
	if (sndHide.playing			())	sndHide.set_position			(get_LastFP());
	if (sndShot.playing			()) sndShot.set_position			(get_LastFP());
	if (sndSilencerShot.playing	()) sndSilencerShot.set_position	(get_LastFP());
	if (sndReload.playing		()) sndReload.set_position			(get_LastFP());
	if (sndEmptyClick.playing	())	sndEmptyClick.set_position		(get_LastFP());
	if (sndSightsUp.playing		())	sndSightsUp.set_position		(get_LastFP());			//Daemonion - iron sight audio - sights being raised
	if (sndSightsDown.playing	())	sndSightsDown.set_position		(get_LastFP());			//Daemonion - iron sight audio - sights being lowered
	//
	if (sndShutter.playing		()) sndShutter.set_position			(get_LastFP());
}

void CWeaponMagazined::state_Fire(float dt)
{
	VERIFY(fTimeToFire > 0.f);

	Fvector					p1, d;
	p1.set(get_LastFP());
	d.set(get_LastFD());

	if (!H_Parent()) return;

	CInventoryOwner* io = smart_cast<CInventoryOwner*>(H_Parent());
	if (NULL == io->inventory().ActiveItem())
	{
		Log("current_state", GetState());
		Log("next_state", GetNextState());
		Log("state_time", m_dwStateTime);
		Log("item_sect", cNameSect().c_str());
		Log("H_Parent", H_Parent()->cNameSect().c_str());
	}

	smart_cast<CEntity*>	(H_Parent())->g_fireParams(this, p1, d);
	if (m_iShotNum == 0)
	{
		m_vStartPos = p1;
		m_vStartDir = d;
	};

	VERIFY(!m_magazine.empty());
	//	Msg("%d && %d && (%d || %d) && (%d || %d)", !m_magazine.empty(), fTime<=0, IsWorking(), m_bFireSingleShot, m_iQueueSize < 0, m_iShotNum < m_iQueueSize);
	while (!m_magazine.empty() && fTime <= 0 && (IsWorking() || m_bFireSingleShot) && (m_iQueueSize < 0 || m_iShotNum < m_iQueueSize))
	{
		m_bFireSingleShot = false;
		//если у оружия есть разные размеры очереди
		//привилегированный режим очереди не полный автомат
		//текущий режим очереди является привилегированным
		//или кол-во выстрелов попадает в предел привилегированного режима
		if (m_bHasDifferentFireModes && m_iPrefferedFireMode != -1 && (GetCurrentFireMode() == m_iPrefferedFireMode || m_iShotNum < m_iPrefferedFireMode))
		{
			VERIFY(fTimeToFirePreffered > 0.f);
			fTime += fTimeToFirePreffered; //установим скорострельность привилегированного режима
			//Msg("fTimeToFirePreffered = %.6f", fTimeToFirePreffered);
		}
		else
		{
			VERIFY(fTimeToFire > 0.f);
			fTime += fTimeToFire;
			//Msg("fTimeToFire = %.6f", fTimeToFire);
		}
		//
		++m_iShotNum;

		OnShot();
		static int i = 0;
		if (i || m_iShotNum > m_iShootEffectorStart)
		{
			// Do Weapon Callback.  (Cribbledirge)
			StateSwitchCallback(GameObject::eOnActorWeaponFire, GameObject::eOnNPCWeaponFire);

			FireTrace(p1, d);
		}
		else
		{
			FireTrace(m_vStartPos, m_vStartDir);
		}
	}

	if (m_iShotNum == m_iQueueSize)
		m_bStopedAfterQueueFired = true;

	UpdateSounds();
}

void CWeaponMagazined::state_Misfire(float /**dt/**/)
{
	OnEmptyClick();
	SwitchState(eIdle);

	bMisfire = true;

	UpdateSounds();
}

void CWeaponMagazined::state_MagEmpty(float dt)
{
}

void CWeaponMagazined::SetDefaults()
{
	CWeapon::SetDefaults();
}

void CWeaponMagazined::OnShot()
{
	// Sound
	PlaySound(*m_pSndShotCurrent, get_LastFP());

	// Camera
	AddShotEffector();

	// Animation
	PlayAnimShoot();

	// Shell Drop
	Fvector vel;
	PHGetLinearVell(vel);
	OnShellDrop(get_LastSP(), vel);

	// Огонь из ствола
	StartFlameParticles();

	//дым из ствола
	ForceUpdateFireParticles();
	StartSmokeParticles(get_LastFP(), vel);
}

void CWeaponMagazined::OnEmptyClick()
{
	PlaySound(sndEmptyClick, get_LastFP());
}

void CWeaponMagazined::OnAnimationEnd(u32 state)
{
	switch (state)
	{
	case eReload:{													// End of reload animation
		UpdateExpansionToChamber();
		ReloadMagazine			();
		HandleCartridgeInChamber();
		SwitchState				(eIdle);
	}break;
	case eHiding:	SwitchState(eHidden);					break;	// End of Hide
	case eShowing:	SwitchState(eIdle);						break;	// End of Show
	case eIdle:		switch2_Idle();							break;  // Keep showing idle
	//
	case eShutter:	ShutterAction();	SwitchState(eIdle);	break;	// End of Shutter animation
	}
}
void CWeaponMagazined::switch2_Idle()
{
	HUD_SOUND::StopSound(sndShow);
	HUD_SOUND::StopSound(sndReload);
	HUD_SOUND::StopSound(sndShutter);

	m_bAmmoWasSpawned = false;

	m_bPending = false;
	PlayAnimIdle(m_idle_state);
}

#ifdef DEBUG
#include "ai\stalker\ai_stalker.h"
#include "object_handler_planner.h"
#endif
void CWeaponMagazined::switch2_Fire()
{
	CInventoryOwner* io = smart_cast<CInventoryOwner*>(H_Parent());
	CInventoryItem* ii = smart_cast<CInventoryItem*>(this);
#ifdef DEBUG
	VERIFY2(io, make_string("no inventory owner, item %s", *cName()));

	if (ii != io->inventory().ActiveItem())
		Msg("! not an active item, item %s, owner %s, active item %s", *cName(), *H_Parent()->cName(), io->inventory().ActiveItem() ? *io->inventory().ActiveItem()->object().cName() : "no_active_item");

	if (!(io && (ii == io->inventory().ActiveItem())))
	{
		CAI_Stalker			*stalker = smart_cast<CAI_Stalker*>(H_Parent());
		if (stalker) {
			stalker->planner().show();
			stalker->planner().show_current_world_state();
			stalker->planner().show_target_world_state();
		}
	}
#else
	if (!io)
		return;
#endif // DEBUG

	//
	//	VERIFY2(
	//		io && (ii == io->inventory().ActiveItem()),
	//		make_string(
	//			"item[%s], parent[%s]",
	//			*cName(),
	//			H_Parent() ? *H_Parent()->cName() : "no_parent"
	//		)
	//	);

	m_bStopedAfterQueueFired = false;
	m_bFireSingleShot = true;
	m_iShotNum = 0;

	if ((OnClient() || Level().IsDemoPlay()) && !IsWorking())
		FireStart();

	/*	if(SingleShotMode())
		{
		m_bFireSingleShot = true;
		bWorking = false;
		}*/
}
void CWeaponMagazined::switch2_Empty()
{
	if (smart_cast<CActor*>(H_Parent()) != NULL && psActorFlags.test(AF_NO_AUTO_RELOAD))
	{
		OnEmptyClick();
		return;
	}

	OnZoomOut();

	if (!TryReload())
	{
		OnEmptyClick();
	}
	else
	{
		inherited::FireEnd();
	}
}
void CWeaponMagazined::PlayReloadSound()
{
	PlaySound(sndReload, get_LastFP());
}

void CWeaponMagazined::switch2_Reload()
{
	CWeapon::FireEnd();

	PlayReloadSound();
	PlayAnimReload();
	m_bPending = true;
}
void CWeaponMagazined::switch2_Hiding()
{
	CWeapon::FireEnd();

	HUD_SOUND::StopSound(sndReload);
	HUD_SOUND::StopSound(sndShutter);

	PlaySound(sndHide, get_LastFP());

	PlayAnimHide();
	m_bPending = true;
}

void CWeaponMagazined::switch2_Hidden()
{
	CWeapon::FireEnd();

	HUD_SOUND::StopSound(sndReload);
	HUD_SOUND::StopSound(sndShutter);

	if (m_pHUD) m_pHUD->StopCurrentAnimWithoutCallback();

	signal_HideComplete();
	RemoveShotEffector();
}
void CWeaponMagazined::switch2_Showing()
{
	HUD_SOUND::StopSound(sndReload);
	HUD_SOUND::StopSound(sndShutter);

	PlaySound(sndShow, get_LastFP());

	m_bPending = true;
	PlayAnimShow();
}

bool CWeaponMagazined::Action(s32 cmd, u32 flags)
{
	if (inherited::Action(cmd, flags)) return true;

	//если оружие чем-то занято, то ничего не делать
	if (IsPending()) return false;

	switch (cmd)
	{
	case kWPN_RELOAD:
	{
/*#if defined(LOCK_RELOAD_IN_SPRINT)
	if (!ParentIsActor() || !(g_actor->get_state() & mcSprint) )
#endif*/
		if (ParentIsActor() && psActorFlags.test(AF_WPN_ACTIONS_RESET_SPRINT)) 
			g_actor->set_state_wishful(g_actor->get_state_wishful() & (~mcSprint));

		if (flags&CMD_START)
		{
			if (Level().IR_GetKeyState(get_action_dik(kSPRINT_TOGGLE)))
			{
				OnShutter();
				return true;
			}
			else if (iAmmoElapsed < iMagazineSize || IsMisfire() || /*psActorFlags.test(AF_AMMO_BOX_AS_MAGAZINE) &&*/ HasDetachableMagazine())
			{
				Reload();
				return true;
			}
		}
	}break;		
	case kWPN_FIREMODE_PREV:
	{
		if (flags&CMD_START)
		{
			OnPrevFireMode();
			return true;
		};
	}break;
	case kWPN_FIREMODE_NEXT:
	{
		if (flags&CMD_START)
		{
			OnNextFireMode();
			return true;
		};
	}break;
	}
	return false;
}

bool CWeaponMagazined::CanAttach(PIItem pIItem)
{
	CScope*				pScope = smart_cast<CScope*>(pIItem);
	CSilencer*			pSilencer = smart_cast<CSilencer*>(pIItem);
	CGrenadeLauncher*	pGrenadeLauncher = smart_cast<CGrenadeLauncher*>(pIItem);

	if (pScope &&
		m_eScopeStatus == ALife::eAddonAttachable &&
		(m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonScope) == 0 &&
		(m_sScopeName == pIItem->object().cNameSect()))
		return true;
	else if (pSilencer &&
		m_eSilencerStatus == ALife::eAddonAttachable &&
		(m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonSilencer) == 0 &&
		(m_sSilencerName == pIItem->object().cNameSect()))
		return true;
	else if (pGrenadeLauncher &&
		m_eGrenadeLauncherStatus == ALife::eAddonAttachable &&
		(m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) == 0 &&
		(m_sGrenadeLauncherName == pIItem->object().cNameSect()))
		return true;
	else
		return inherited::CanAttach(pIItem);
}

bool CWeaponMagazined::CanDetach(const char* item_section_name)
{
	if (m_eScopeStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
		0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonScope) &&
		(m_sScopeName == item_section_name))
		return true;
	else if (m_eSilencerStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
		0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonSilencer) &&
		(m_sSilencerName == item_section_name))
		return true;
	else if (m_eGrenadeLauncherStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
		0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) &&
		(m_sGrenadeLauncherName == item_section_name))
		return true;
	else
		return inherited::CanDetach(item_section_name);
}

bool CWeaponMagazined::Attach(PIItem pIItem, bool b_send_event)
{
	bool result = false;

	CScope*				pScope = smart_cast<CScope*>(pIItem);
	CSilencer*			pSilencer = smart_cast<CSilencer*>(pIItem);
	CGrenadeLauncher*	pGrenadeLauncher = smart_cast<CGrenadeLauncher*>(pIItem);

	if (pScope &&
		m_eScopeStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
		(m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonScope) == 0 &&
		(m_sScopeName == pIItem->object().cNameSect()))
	{
		m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonScope;
		result = true;
	}
	else if (pSilencer &&
		m_eSilencerStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
		(m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonSilencer) == 0 &&
		(m_sSilencerName == pIItem->object().cNameSect()))
	{
		m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonSilencer;
		result = true;
	}
	else if (pGrenadeLauncher &&
		m_eGrenadeLauncherStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
		(m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) == 0 &&
		(m_sGrenadeLauncherName == pIItem->object().cNameSect()))
	{
		m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher;
		result = true;
	}

	if (result)
	{
		if (b_send_event && OnServer())
		{
			//уничтожить подсоединенную вещь из инвентаря
			//.			pIItem->Drop					();
			pIItem->object().DestroyObject();
		};

		UpdateAddonsVisibility();
		InitAddons();

		return true;
	}
	else
		return inherited::Attach(pIItem, b_send_event);
}

bool CWeaponMagazined::Detach(const char* item_section_name, bool b_spawn_item)
{
	if (m_eScopeStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
		0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonScope) &&
		(m_sScopeName == item_section_name))
	{
		m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonScope;

		UpdateAddonsVisibility();
		InitAddons();

		return CInventoryItemObject::Detach(item_section_name, b_spawn_item);
	}
	else if (m_eSilencerStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
		0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonSilencer) &&
		(m_sSilencerName == item_section_name))
	{
		m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonSilencer;

		UpdateAddonsVisibility();
		InitAddons();
		return CInventoryItemObject::Detach(item_section_name, b_spawn_item);
	}
	else if (m_eGrenadeLauncherStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
		0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) &&
		(m_sGrenadeLauncherName == item_section_name))
	{
		m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher;

		UpdateAddonsVisibility();
		InitAddons();
		return CInventoryItemObject::Detach(item_section_name, b_spawn_item);
	}
	else
		return inherited::Detach(item_section_name, b_spawn_item);;
}

void CWeaponMagazined::InitAddons()
{
	//////////////////////////////////////////////////////////////////////////
	// Прицел
#ifndef SIMPLE_ZOOM_SETTINGS
	m_fIronSightZoomFactor = READ_IF_EXISTS(pSettings, r_float, cNameSect(), "ironsight_zoom_factor", 50.0f);
#else
	m_fIronSightZoomFactor = g_fov / READ_IF_EXISTS(pSettings, r_float, cNameSect(), "ironsight_zoom_factor", 1.0f);
#endif
	if (IsScopeAttached())
	{
		if (m_eScopeStatus == ALife::eAddonAttachable)
		{
			m_sScopeName = pSettings->r_string(cNameSect(), "scope_name");
			m_iScopeX = pSettings->r_s32(cNameSect(), "scope_x");
			m_iScopeY = pSettings->r_s32(cNameSect(), "scope_y");

			shared_str scope_tex_name;
			scope_tex_name = pSettings->r_string(*m_sScopeName, "scope_texture");
#ifndef SIMPLE_ZOOM_SETTINGS
			m_fScopeZoomFactor = pSettings->r_float(*m_sScopeName, "scope_zoom_factor");
#else
			m_fScopeZoomFactor = g_fov / pSettings->r_float(*m_sScopeName, "scope_zoom_factor");
#endif
			if (m_UIScope) xr_delete(m_UIScope);
			m_UIScope = xr_new<CUIStaticItem>();

			//			m_UIScope->Init(*scope_tex_name, "hud\\default", 0, 0, alNone);
			m_UIScope->Init(*scope_tex_name, "hud\\scopes", 0, 0, alNone);	// KD: special shader that account screen resolution
		}
		else if (m_eScopeStatus == ALife::eAddonPermanent)
		{
#ifndef SIMPLE_ZOOM_SETTINGS
			m_fScopeZoomFactor = pSettings->r_float(cNameSect(), "scope_zoom_factor");
#else
			m_fScopeZoomFactor = g_fov / pSettings->r_float(cNameSect(), "scope_zoom_factor");
#endif
			shared_str scope_tex_name;
			scope_tex_name = pSettings->r_string(cNameSect(), "scope_texture");

			if (m_UIScope) xr_delete(m_UIScope);
			m_UIScope = xr_new<CUIStaticItem>();
			//			m_UIScope->Init(*scope_tex_name, "hud\\default", 0, 0, alNone);
			m_UIScope->Init(*scope_tex_name, "hud\\scopes", 0, 0, alNone);	// KD: special shader that account screen resolution
		}
	}
	else
	{
		if (m_UIScope) xr_delete(m_UIScope);

		if (IsZoomEnabled())
#ifndef SIMPLE_ZOOM_SETTINGS
			m_fIronSightZoomFactor = pSettings->r_float(cNameSect(), "scope_zoom_factor");
#else
			m_fIronSightZoomFactor = g_fov / READ_IF_EXISTS(pSettings, r_float, cNameSect(), "ironsight_zoom_factor", 1.0f);
#endif
	}

	if (IsSilencerAttached() && SilencerAttachable())
	{
		m_sFlameParticlesCurrent = m_sSilencerFlameParticles;
		m_sSmokeParticlesCurrent = m_sSilencerSmokeParticles;
		m_pSndShotCurrent = &sndSilencerShot;

		//сила выстрела
		LoadFireParams(*cNameSect(), "");

		//подсветка от выстрела
		LoadLights(*cNameSect(), "silencer_");
		ApplySilencerKoeffs();
	}
	else
	{
		m_sFlameParticlesCurrent = m_sFlameParticles;
		m_sSmokeParticlesCurrent = m_sSmokeParticles;
		m_pSndShotCurrent = &sndShot;

		//сила выстрела
		LoadFireParams(*cNameSect(), "");
		//подсветка от выстрела
		LoadLights(*cNameSect(), "");
	}

	inherited::InitAddons();
}

void CWeaponMagazined::ApplySilencerKoeffs()
{
	float BHPk = 1.0f, BSk = 1.0f;
	float FDB_k = 1.0f, CD_k = 1.0f;

	if (pSettings->line_exist(m_sSilencerName, "bullet_hit_power_k"))
	{
		BHPk = pSettings->r_float(m_sSilencerName, "bullet_hit_power_k");
		clamp(BHPk, 0.0f, 1.0f);
	};
	if (pSettings->line_exist(m_sSilencerName, "bullet_speed_k"))
	{
		BSk = pSettings->r_float(m_sSilencerName, "bullet_speed_k");
		clamp(BSk, 0.0f, 1.0f);
	};
	if (pSettings->line_exist(m_sSilencerName, "fire_dispersion_base_k"))
	{
		FDB_k = pSettings->r_float(m_sSilencerName, "fire_dispersion_base_k");
		//		clamp(FDB_k, 0.0f, 1.0f);
	};
	if (pSettings->line_exist(m_sSilencerName, "cam_dispersion_k"))
	{
		CD_k = pSettings->r_float(m_sSilencerName, "cam_dispersion_k");
		clamp(CD_k, 0.0f, 1.0f);
	};

	//fHitPower			= fHitPower*BHPk;
	fvHitPower.mul(BHPk);
	fHitImpulse *= BSk;
	m_fStartBulletSpeed *= BSk;
	fireDispersionBase *= FDB_k;
	camDispersion *= CD_k;
	camDispersionInc *= CD_k;
}

//виртуальные функции для проигрывания анимации HUD
void CWeaponMagazined::PlayAnimShow()
{
	VERIFY(GetState() == eShowing);
	m_pHUD->animPlay(random_anim(mhud.mhud_show), FALSE, this, GetState());
}

void CWeaponMagazined::PlayAnimHide()
{
	VERIFY(GetState() == eHiding);
	m_pHUD->animPlay(random_anim(mhud.mhud_hide), TRUE, this, GetState());
}

void CWeaponMagazined::PlayAnimReload() 
{
	VERIFY(GetState() == eReload);
	if (IsPartlyReloading())
		m_pHUD->animPlay(random_anim(mhud.mhud_reload_partly), TRUE, this, GetState());
	else
		m_pHUD->animPlay(random_anim(mhud.mhud_reload), TRUE, this, GetState());
}

bool CWeaponMagazined::TryPlayAnimIdle(u8 state = eIdle) 
{
	VERIFY(GetState() == eIdle);
	if (!IsZoomed()) 
	{
		switch (state) 
		{
		case eSubstateIdleMoving:
			m_pHUD->animPlay(random_anim(mhud.mhud_idle_moving), TRUE, NULL, GetState());
			return true;
		case eSubstateIdleSprint:
			m_pHUD->animPlay(random_anim(mhud.mhud_idle_sprint), TRUE, NULL, GetState());
			return true;
		default:
			return false;
		}
	}
	return false;
}

void CWeaponMagazined::PlayAnimIdle(u8 state = eIdle)
{
	MotionSVec* m = NULL;
	if (IsZoomed())
	{
		m = &mhud.mhud_idle_aim;
	}
	else{
		m = &mhud.mhud_idle;
		if (TryPlayAnimIdle(state)) return;
	}

	VERIFY(GetState() == eIdle);
	m_pHUD->animPlay(random_anim(*m), TRUE, NULL, GetState());
}

void CWeaponMagazined::PlayAnimShoot()
{
	VERIFY(GetState() == eFire || GetState() == eFire2);
	m_pHUD->animPlay(random_anim(mhud.mhud_shots), TRUE, this, GetState());
}

void CWeaponMagazined::OnZoomIn()
{
	inherited::OnZoomIn();

	if (GetState() == eIdle)
		PlayAnimIdle(m_idle_state);

	CActor* pActor = smart_cast<CActor*>(H_Parent());
	if (pActor)
	{
																					
		HUD_SOUND::StopSound(sndSightsUp);													//daemonion - iron sight audio - sights being raised
		HUD_SOUND::StopSound(sndSightsDown);												//									
		bool b_hud_mode = (Level().CurrentEntity() == H_Parent());							//
		HUD_SOUND::PlaySound(sndSightsUp, H_Parent()->Position(), H_Parent(), b_hud_mode);	//--END	

		CEffectorZoomInertion* S = smart_cast<CEffectorZoomInertion*>	(pActor->Cameras().GetCamEffector(eCEZoom));
		if (!S)
		{
			S = (CEffectorZoomInertion*)pActor->Cameras().AddCamEffector(xr_new<CEffectorZoomInertion>());
			S->Init(this);
		};
		S->SetRndSeed(pActor->GetZoomRndSeed());
		R_ASSERT(S);
	}
}
void CWeaponMagazined::OnZoomOut()
{
	if (!m_bZoomMode) return;

	inherited::OnZoomOut();

	if (GetState() == eIdle)
		PlayAnimIdle(m_idle_state);

	CActor* pActor = smart_cast<CActor*>(H_Parent());
																						
	HUD_SOUND::StopSound(sndSightsUp);													//daemonion - iron sight audio - sights being lowered
	HUD_SOUND::StopSound(sndSightsDown);												//									
	bool b_hud_mode = (Level().CurrentEntity() == H_Parent());							//
	HUD_SOUND::PlaySound(sndSightsDown, H_Parent()->Position(), H_Parent(), b_hud_mode);//--END

	if(pActor)
		pActor->Cameras().RemoveCamEffector(eCEZoom);
}

//переключение режимов стрельбы одиночными и очередями
bool CWeaponMagazined::SwitchMode()
{
	if (eIdle != GetState() || IsPending()) return false;

	if (SingleShotMode())
		m_iQueueSize = WEAPON_ININITE_QUEUE;
	else
		m_iQueueSize = 1;

	PlaySound(sndEmptyClick, get_LastFP());

	return true;
}

void CWeaponMagazined::StartIdleAnim()
{
	if (IsZoomed())	m_pHUD->animDisplay(mhud.mhud_idle_aim[Random.randI(mhud.mhud_idle_aim.size())], TRUE);
	else			m_pHUD->animDisplay(mhud.mhud_idle[Random.randI(mhud.mhud_idle.size())], TRUE);
}

void CWeaponMagazined::onMovementChanged( ACTOR_DEFS::EMoveCommand cmd ) 
{
  if ( cmd == ACTOR_DEFS::mcSprint && GetState() == eIdle ) 
  {
    m_idle_state = eSubstateIdleSprint;
    PlayAnimIdle( m_idle_state );
  }
}

void	CWeaponMagazined::OnNextFireMode()
{
	if (!m_bHasDifferentFireModes) return;
	if (GetState() != eIdle) return;
	m_iCurFireMode = (m_iCurFireMode + 1 + m_aFireModes.size()) % m_aFireModes.size();
	SetQueueSize(GetCurrentFireMode());
};

void	CWeaponMagazined::OnPrevFireMode()
{
	if (!m_bHasDifferentFireModes) return;
	if (GetState() != eIdle) return;
	m_iCurFireMode = (m_iCurFireMode - 1 + m_aFireModes.size()) % m_aFireModes.size();
	SetQueueSize(GetCurrentFireMode());
};

void	CWeaponMagazined::OnH_A_Chield()
{
	if (m_bHasDifferentFireModes)
	{
		CActor	*actor = smart_cast<CActor*>(H_Parent());
		if (!actor) SetQueueSize(-1);
		else SetQueueSize(GetCurrentFireMode());
	};
	inherited::OnH_A_Chield();
};

void	CWeaponMagazined::SetQueueSize(int size)
{
	m_iQueueSize = size;
	if (m_iQueueSize == -1)
		strcpy_s(m_sCurFireMode, " (A)");
	else
		sprintf_s(m_sCurFireMode, " (%d)", m_iQueueSize);
};

float	CWeaponMagazined::GetWeaponDeterioration()
{
	if (!m_bHasDifferentFireModes || m_iPrefferedFireMode == -1 || u32(GetCurrentFireMode()) <= u32(m_iPrefferedFireMode))
		return inherited::GetWeaponDeterioration();
	//
	float silencer_dec_k = IsSilencerAttached() && SilencerAttachable() ? conditionDecreasePerShotSilencerKoef : 1.0f;
	//
	return m_iShotNum*conditionDecreasePerShot * silencer_dec_k;
};

void CWeaponMagazined::save(NET_Packet &output_packet)
{
	inherited::save(output_packet);
	save_data(m_iQueueSize, output_packet);
	save_data(m_iShotNum, output_packet);
	save_data(m_iCurFireMode, output_packet);
}

void CWeaponMagazined::load(IReader &input_packet)
{
	inherited::load(input_packet);
	load_data(m_iQueueSize, input_packet); SetQueueSize(m_iQueueSize);
	load_data(m_iShotNum, input_packet);
	load_data(m_iCurFireMode, input_packet);
}

BOOL CWeaponMagazined::net_Spawn(CSE_Abstract* DC)
{
	BOOL bRes = inherited::net_Spawn(DC);
	CSE_ALifeItemWeaponMagazined* const wpn = smart_cast<CSE_ALifeItemWeaponMagazined*>(DC);
	m_iCurFireMode = wpn->m_u8CurFireMode;
	SetQueueSize(GetCurrentFireMode());
	//
	if (wpn->m_AmmoIDs.size()>0)
	{
		m_magazine.clear();
		std::for_each(wpn->m_AmmoIDs.begin(), wpn->m_AmmoIDs.end(), [&](u8 at)
		{
			if (at > m_ammoTypes.size())
				at = 0;
			CCartridge l_cartridge;
			l_cartridge.Load(*m_ammoTypes[at], at);
			m_magazine.push_back(l_cartridge);
		});
	}
	return bRes;
}

void CWeaponMagazined::net_Export(NET_Packet& P)
{
	inherited::net_Export(P);

	P.w_u8(u8(m_iCurFireMode & 0x00ff));
	//
	P.w_u8(u8(m_magazine.size()));
	for (u32 i = 0; i<m_magazine.size(); i++)
	{
		CCartridge& l_cartridge = *(m_magazine.begin() + i);
		P.w_u8(l_cartridge.m_LocalAmmoType);
	}
}

void CWeaponMagazined::net_Import(NET_Packet& P)
{
	//	if (Level().IsDemoPlay())
	//		Msg("CWeapon::net_Import [%d]", ID());

	inherited::net_Import(P);

	m_iCurFireMode = P.r_u8();
	SetQueueSize(GetCurrentFireMode());
	//
	u8 AmmoCount = P.r_u8();
	for (u32 i = 0; i<AmmoCount; i++)
	{
		u8 LocalAmmoType = P.r_u8();
		if (i >= m_magazine.size()) continue;
		CCartridge& l_cartridge = *(m_magazine.begin() + i);
		if (LocalAmmoType == l_cartridge.m_LocalAmmoType) continue;
#ifdef DEBUG
		Msg("! %s reload to %s", *l_cartridge.m_ammoSect, *(m_ammoTypes[LocalAmmoType]));
#endif
		l_cartridge.Load(*(m_ammoTypes[LocalAmmoType]), LocalAmmoType);
		//		m_fCurrentCartirdgeDisp = m_DefaultCartridge.m_kDisp;		
	}
}
#include "string_table.h"
#include "ui/UIMainIngameWnd.h"
void CWeaponMagazined::GetBriefInfo(xr_string& str_name, xr_string& icon_sect_name, xr_string& str_count)
{
	auto CurrentHUD		= HUD().GetUI()->UIMainIngameWnd;
	bool b_wpn_info		= CurrentHUD->IsHUDElementAllowed(eActiveItem);
	bool b_gear_info	= CurrentHUD->IsHUDElementAllowed(eGear);

	int	AE = GetAmmoElapsed();
	int	AC = GetAmmoCurrent();

	if (AE == 0 || 0 == m_magazine.size())
		icon_sect_name = *m_ammoTypes[m_ammoType];
	else
		icon_sect_name = *m_ammoTypes[m_magazine.back().m_LocalAmmoType];

	string256		sItemName;
	strcpy_s(sItemName, *CStringTable().translate(pSettings->r_string(icon_sect_name.c_str(), "inv_name_short")));
	//
	if (iMagazineSize > 1 && !m_magazine.empty())
		sprintf_s(sItemName, "%s | %s", *m_magazine.back().m_InvShortName, iAmmoElapsed > 1 ? * m_magazine[m_magazine.size() - 2].m_InvShortName : "");

	if (HasFireModes() && b_wpn_info)
		strcat_s(sItemName, GetCurrentFireModeStr());

	str_name = sItemName;

	{
		/*if (!unlimited_ammo())
			sprintf_s(sItemName, "%d/%d", AE, AC - AE);
		else
			sprintf_s(sItemName, "%d/--", AE);*/

		if (b_wpn_info && b_gear_info)
			sprintf_s(sItemName, "%d|%d", AE, AC - AE);
		else if (b_wpn_info)
			sprintf_s(sItemName, "[%d]", AE);
		else if (b_gear_info)
			sprintf_s(sItemName, "%d", AC - AE);
		else if (unlimited_ammo())
			sprintf_s(sItemName, "%d|--", AE);

		str_count = sItemName;
	}
}
//работа затвора
void CWeaponMagazined::OnShutter()
{
	SwitchState(eShutter);
}
//
void CWeaponMagazined::switch2_Shutter()
{
	PlaySound(sndShutter, get_LastFP());
	PlayAnimShutter();
	m_bPending = true;
}
//
void CWeaponMagazined::PlayAnimShutter()
{
	VERIFY(GetState() == eShutter);
	m_pHUD->animPlay(random_anim(mhud.mhud_shutter), TRUE, this, GetState());
}
//
void CWeaponMagazined::ShutterAction() //передёргивание затвора
{
	if (IsMisfire())
	{
		bMisfire = false;
		if (smart_cast<CActor*>(this->H_Parent()) && (Level().CurrentViewEntity() == H_Parent()))
		{
			HUD().GetUI()->UIGame()->RemoveCustomStatic("gun_jammed");
			HUD().GetUI()->AddInfoMessage("gun_not_jammed");
		}
	}
	else if (HasChamber() && !m_magazine.empty())
	{
		UnloadAmmo(1);
	}
}