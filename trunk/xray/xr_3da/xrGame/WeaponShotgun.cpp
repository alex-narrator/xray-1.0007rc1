#include "stdafx.h"
#include "weaponshotgun.h"
#include "WeaponHUD.h"
#include "hudmanager.h"
#include "uigamecustom.h"
#include "entity.h"
#include "ParticlesObject.h"
#include "xr_level_controller.h"
#include "inventory.h"
#include "level.h"
#include "actor.h"
#include "xrServer_Objects_ALife_Items.h"

CWeaponShotgun::CWeaponShotgun(void) : CWeaponCustomPistol("TOZ34")
{
    m_eSoundShotBoth		= ESoundTypes(SOUND_TYPE_WEAPON_SHOOTING);
	m_eSoundClose			= ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING);
	m_eSoundAddCartridge	= ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING);
	SetSlot(RIFLE_SLOT); 
	//m_bLockType				= true; //запрет заряжания смешанными патронами
}

CWeaponShotgun::~CWeaponShotgun(void)
{
	// sounds
	HUD_SOUND::DestroySound(sndShotBoth);
	HUD_SOUND::DestroySound(m_sndOpen);
	HUD_SOUND::DestroySound(m_sndAddCartridge);
	HUD_SOUND::DestroySound(m_sndClose);

}

void CWeaponShotgun::net_Destroy()
{
	inherited::net_Destroy();
}

void CWeaponShotgun::Load	(LPCSTR section)
{
	inherited::Load		(section);

	// Звук и анимация для выстрела дуплетом
	HUD_SOUND::LoadSound(section, "snd_shoot_duplet", sndShotBoth, m_eSoundShotBoth);
	animGetEx(mhud_shot_boths, "anim_shoot_both");

	if(pSettings->line_exist(section, "tri_state_reload")){
		m_bTriStateReload = !!pSettings->r_bool(section, "tri_state_reload");
	};
	if(m_bTriStateReload){
		HUD_SOUND::LoadSound(section, "snd_open_weapon", m_sndOpen, m_eSoundOpen);
		animGetEx(mhud_open, "anim_open_weapon");

		HUD_SOUND::LoadSound(section, "snd_add_cartridge", m_sndAddCartridge, m_eSoundAddCartridge);
		animGetEx(mhud_add_cartridge, "anim_add_cartridge");

		HUD_SOUND::LoadSound(section, "snd_close_weapon", m_sndClose, m_eSoundClose);
		animGetEx(mhud_close, "anim_close_weapon");
		//
		if (!pSettings->line_exist(section, "snd_shutter"))
			HUD_SOUND::LoadSound(section, "snd_open_weapon", sndShutter, m_eSoundShutter);

		animGetEx(mhud.mhud_shutter, pSettings->line_exist(hud_sect.c_str(), "anim_shutter") ? "anim_shutter" : "anim_reload");
	};

}




void CWeaponShotgun::OnShot () 
{
//.?std::swap(m_pHUD->FirePoint(), m_pHUD->FirePoint2());
//.	std::swap(vLoadedFirePoint, vLoadedFirePoint2);
	inherited::OnShot();
}
// Headers included by Cribbledirge (for callbacks).
#include "pch_script.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
#include "game_object_space.h"
void CWeaponShotgun::Fire2Start () 
{
	if(m_bPending) return;

	inherited::Fire2Start();

	if (IsValid() && !IsMisfire())
	{
		if (!IsWorking())
		{
			if (GetState()==eReload)		return;
			if (GetState()==eShowing)		return;
			if (GetState()==eHiding)		return;

			if (!iAmmoElapsed)	
			{
				CWeapon::FireStart			();
				SwitchState					(eMagEmpty);
			}
			else					
			{
				CWeapon::FireStart			();
				SwitchState					((iAmmoElapsed < iMagazineSize)?eFire:eFire2);
			}
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
		if (!iAmmoElapsed)	
			SwitchState						(eMagEmpty);
	}
}

void CWeaponShotgun::Fire2End () 
{
	inherited::Fire2End();
	FireEnd();
}


void CWeaponShotgun::OnShotBoth()
{
	//если патронов меньше, чем 2 
	if(iAmmoElapsed < iMagazineSize) 
	{ 
		OnShot(); 
		return; 
	}

	//звук выстрела дуплетом
	PlaySound			(sndShotBoth,get_LastFP());
	
	// Camera
	AddShotEffector		();
	
	// анимация дуплета
	m_pHUD->animPlay			(random_anim(mhud_shot_boths),FALSE,this,GetState());
	
	// Shell Drop
	Fvector vel; 
	PHGetLinearVell		(vel);
	OnShellDrop			(get_LastSP(), vel);

	//огонь из 2х стволов
	StartFlameParticles			();
	StartFlameParticles2		();

	//дым из 2х стволов
	CParticlesObject* pSmokeParticles = NULL;
	CShootingObject::StartParticles(pSmokeParticles, *m_sSmokeParticlesCurrent, get_LastFP(),  zero_vel, true);
	pSmokeParticles = NULL;
	CShootingObject::StartParticles(pSmokeParticles, *m_sSmokeParticlesCurrent, get_LastFP2(), zero_vel, true);

}

void CWeaponShotgun::switch2_Fire	()
{
	inherited::switch2_Fire	();
	bWorking = false;
}

void CWeaponShotgun::switch2_Fire2	()
{
	VERIFY(fTimeToFire>0.f);

	if (fTime<=0)
	{
		// Fire
		Fvector						p1, d; 
		p1.set	(get_LastFP()); 
		d.set	(get_LastFD());

		CEntity*					E = smart_cast<CEntity*>(H_Parent());
		if (E){
		CInventoryOwner* io		= smart_cast<CInventoryOwner*>(H_Parent());
			if(NULL == io->inventory().ActiveItem())
			{
			Log("current_state", GetState() );
			Log("next_state", GetNextState());
			Log("state_time", m_dwStateTime);
			Log("item_sect", cNameSect().c_str());
			Log("H_Parent", H_Parent()->cNameSect().c_str());
			}
			E->g_fireParams		(this, p1,d);
		}
		
		OnShotBoth						();

		//выстрел из обоих стволов
		FireTrace					(p1,d);
		FireTrace					(p1,d);
		fTime						+= fTimeToFire*2.f;

		// Patch for "previous frame position" :)))
		dwFP_Frame					= 0xffffffff;
		dwXF_Frame					= 0xffffffff;
	}
}

void CWeaponShotgun::UpdateSounds	()
{
	inherited::UpdateSounds();
	if (sndShotBoth.playing			()) sndShotBoth.set_position		(get_LastFP());
	if (m_sndOpen.playing			()) m_sndOpen.set_position			(get_LastFP());
	if (m_sndAddCartridge.playing	()) m_sndAddCartridge.set_position	(get_LastFP());
	if (m_sndClose.playing			()) m_sndClose.set_position			(get_LastFP());
}

bool CWeaponShotgun::Action			(s32 cmd, u32 flags) 
{
	if (GetCurrentFireMode() == 2)
	{
		switch (cmd)
		{
		case kWPN_FIRE:
		{
			if (flags&CMD_START)
			{
				if (IsPending()) return false;
				Fire2Start();
			}
			else
				Fire2End();

			return true;
		}
		}
	}

	if(inherited::Action(cmd, flags)) return true;

	if(	m_bTriStateReload && GetState()==eReload &&
		(cmd == kWPN_FIRE || cmd == kWPN_NEXT || cmd == kWPN_RELOAD) && flags&CMD_START &&
		m_sub_state==eSubstateReloadInProcess		)//остановить перезагрузку
	{
		AddCartridge(1);
		m_sub_state = eSubstateReloadEnd;
		return true;
	}
	//если оружие чем-то занято, то ничего не делать
/*	if(IsPending()) return false;

	switch(cmd) 
	{
		case kWPN_ZOOM : 
			{
				if(flags&CMD_START) Fire2Start();
				else Fire2End();
			}
			return true;
	}*/
	return false;
}

void CWeaponShotgun::OnAnimationEnd(u32 state) 
{
	if(!m_bTriStateReload || state != eReload)
		return inherited::OnAnimationEnd(state);

	switch(m_sub_state){
		case eSubstateReloadBegin:{
			m_sub_state = eSubstateReloadInProcess;
			SwitchState(eReload);
		}break;

		case eSubstateReloadInProcess:{
			if( 0 != AddCartridge(1) ){
				m_sub_state = eSubstateReloadEnd;
			}
			SwitchState(eReload);
		}break;

		case eSubstateReloadEnd:{
			m_sub_state = eSubstateReloadBegin;
			SwitchState(eIdle);
		}break;
		
	};
}

void CWeaponShotgun::Reload() 
{
	if(m_bTriStateReload){
		TriStateReload();
	}else
		inherited::Reload();
}

void CWeaponShotgun::TriStateReload()
{
	if( !HaveCartridgeInInventory(1) )return;
	inherited::OnZoomOut();
	m_sub_state			= eSubstateReloadBegin;
	SwitchState			(eReload);
}

void CWeaponShotgun::OnStateSwitch	(u32 S)
{
	if(!m_bTriStateReload || S != eReload){
		inherited::OnStateSwitch(S);
		return;
	}

	CWeapon::OnStateSwitch(S);

	if( m_magazine.size() == (u32)iMagazineSize || !HaveCartridgeInInventory(1) ){
			switch2_EndReload		();
			m_sub_state = eSubstateReloadEnd;
	};

	switch (m_sub_state)
	{
	case eSubstateReloadBegin:
		if( HaveCartridgeInInventory(1) )
			switch2_StartReload	();
		break;
	case eSubstateReloadInProcess:
			if( HaveCartridgeInInventory(1) )
				switch2_AddCartgidge	();
		break;
	case eSubstateReloadEnd:
			switch2_EndReload		();
		break;
	};
}

void CWeaponShotgun::switch2_StartReload()
{
	PlaySound			(m_sndOpen,get_LastFP());
	PlayAnimOpenWeapon	();
	m_bPending = true;
}

void CWeaponShotgun::switch2_AddCartgidge	()
{
	PlaySound	(m_sndAddCartridge,get_LastFP());
	PlayAnimAddOneCartridgeWeapon();
	m_bPending = true;
}

void CWeaponShotgun::switch2_EndReload	()
{
	m_bPending = false;
	PlaySound			(m_sndClose,get_LastFP());
	PlayAnimCloseWeapon	();
}

void CWeaponShotgun::PlayAnimOpenWeapon()
{
	VERIFY(GetState()==eReload);
	m_pHUD->animPlay(random_anim(mhud_open),TRUE,this,GetState());
}
void CWeaponShotgun::PlayAnimAddOneCartridgeWeapon()
{
	VERIFY(GetState()==eReload);
	m_pHUD->animPlay(random_anim(mhud_add_cartridge),TRUE,this,GetState());
}
void CWeaponShotgun::PlayAnimCloseWeapon()
{
	VERIFY(GetState()==eReload);
	m_pHUD->animPlay(random_anim(mhud_close),TRUE,this,GetState());
}

bool CWeaponShotgun::HaveCartridgeInInventory		(u8 cnt)
{
	if (unlimited_ammo()) return true;
	m_pAmmo = NULL;
	if(m_pCurrentInventory) 
	{
		/*if (m_set_next_ammoType_on_reload != u32(-1)) 
		{
			m_ammoType = m_set_next_ammoType_on_reload;
			m_set_next_ammoType_on_reload = u32(-1);
			if (!m_magazine.empty()) UnloadMagazine(); //разрядить если меняем тип патрона
		}*/
		//попытаться найти в инвентаре патроны текущего типа 
		m_pAmmo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmo(*m_ammoTypes[m_ammoType], ParentIsActor()));

		if (!m_pAmmo /*&& (m_magazine.empty() || !m_bLockType)*/)
		{
			for(u32 i = 0; i < m_ammoTypes.size(); ++i) 
			{
				//проверить патроны всех подходящих типов
				m_pAmmo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->GetAmmo(*m_ammoTypes[i], ParentIsActor()));

				if(m_pAmmo) 
				{ 
					m_ammoType = i; 
					break; 
				}
			}
		}
	}
	return (m_pAmmo!=NULL)&&(m_pAmmo->m_boxCurr>=cnt) ;
}

u8 CWeaponShotgun::AddCartridge		(u8 cnt)
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

	if(m_set_next_ammoType_on_reload != u32(-1)){
		m_ammoType						= m_set_next_ammoType_on_reload;
		m_set_next_ammoType_on_reload	= u32(-1);

	}

	if( !HaveCartridgeInInventory(1) )
		return 0;

	VERIFY((u32)iAmmoElapsed == m_magazine.size());


	if (m_DefaultCartridge.m_LocalAmmoType != m_ammoType)
		m_DefaultCartridge.Load(*m_ammoTypes[m_ammoType], u8(m_ammoType));
	CCartridge l_cartridge = m_DefaultCartridge;
	while(cnt)// && m_pAmmo->Get(l_cartridge)) 
	{
		if (!unlimited_ammo())
		{
			if (!m_pAmmo->Get(l_cartridge)) break;
		}
		--cnt;
		++iAmmoElapsed;
		l_cartridge.m_LocalAmmoType = u8(m_ammoType);
		m_magazine.push_back(l_cartridge);
//		m_fCurrentCartirdgeDisp = l_cartridge.m_kDisp;
	}
	m_ammoName = (m_pAmmo) ? m_pAmmo->m_nameShort : NULL;

	VERIFY((u32)iAmmoElapsed == m_magazine.size());

	//выкинуть коробку патронов, если она пустая
	if(m_pAmmo && !m_pAmmo->m_boxCurr && OnServer()) 
		m_pAmmo->SetDropManual(TRUE);

	return cnt;
}

/*BOOL	CWeaponShotgun::net_Spawn(CSE_Abstract* DC)
{
	BOOL bRes = inherited::net_Spawn(DC);

	CSE_ALifeItemWeaponShotGun* _dc = smart_cast<CSE_ALifeItemWeaponShotGun*>(DC);
	xr_vector<u8> ammo_ids = _dc->m_AmmoIDs;

	for (u32 i = 0; i < ammo_ids.size(); i++)
	{
		u8 LocalAmmoType = ammo_ids[i];
		if (i >= m_magazine.size()) continue;
		CCartridge& l_cartridge = *(m_magazine.begin() + i);
		if (LocalAmmoType == l_cartridge.m_LocalAmmoType) continue;
		l_cartridge.Load(*m_ammoTypes[LocalAmmoType], LocalAmmoType);
	}

	return bRes;
}

void	CWeaponShotgun::net_Export	(NET_Packet& P)
{
	inherited::net_Export(P);	

	P.w_u8(u8(m_magazine.size()));
	for (u32 i = 0; i<m_magazine.size(); i++)
	{
		CCartridge& l_cartridge = *(m_magazine.begin() + i);
		P.w_u8(l_cartridge.m_LocalAmmoType);
	}
}

void	CWeaponShotgun::net_Import	(NET_Packet& P)
{
	inherited::net_Import(P);	

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
	}
}*/