#include "stdafx.h"
#include "weaponBM16.h"
#include "WeaponHUD.h"

CWeaponBM16::~CWeaponBM16()
{
	HUD_SOUND::DestroySound(m_sndReload1);
}

void CWeaponBM16::Load	(LPCSTR section)
{
	inherited::Load		(section);

	animGetEx(mhud_reload1, "anim_reload_1");
	animGetEx(mhud_shot1, "anim_shoot_1");
	animGetEx(mhud_idle1, "anim_idle_1");
	animGetEx(mhud_idle2, "anim_idle_2");
	animGetEx(mhud_zoomed_idle1, "anim_zoomed_idle_1");
	animGetEx(mhud_zoomed_idle2, "anim_zoomedidle_2");

	animGetEx(mhud_idle_sprint_1, pSettings->line_exist(hud_sect.c_str(), "anim_idle_sprint_1") ? "anim_idle_sprint_1" : pSettings->line_exist(hud_sect.c_str(), "anim_idle_sprint") ? "anim_idle_sprint" : "anim_idle");
	animGetEx(mhud_idle_sprint_2, pSettings->line_exist(hud_sect.c_str(), "anim_idle_sprint_2") ? "anim_idle_sprint_2" : pSettings->line_exist(hud_sect.c_str(), "anim_idle_sprint") ? "anim_idle_sprint" : "anim_idle");
	animGetEx(mhud_idle_moving_1, pSettings->line_exist(hud_sect.c_str(), "anim_idle_moving_1") ? "anim_idle_moving_1" : pSettings->line_exist(hud_sect.c_str(), "anim_idle_moving") ? "anim_idle_moving" : "anim_idle");
	animGetEx(mhud_idle_moving_2, pSettings->line_exist(hud_sect.c_str(), "anim_idle_moving_2") ? "anim_idle_moving_2" : pSettings->line_exist(hud_sect.c_str(), "anim_idle_moving") ? "anim_idle_moving" : "anim_idle");

// Real Wolf. 03.08.2014.
#if defined(BM16_ANIMS_FIX)
	animGetEx(mhud_draw_empty_both, pSettings->line_exist(hud_sect.c_str(), "anim_draw_empty_both") ? "anim_draw_empty_both" : "anim_draw");
	animGetEx(mhud_draw_empty_right, pSettings->line_exist(hud_sect.c_str(), "anim_draw_empty_right") ? "anim_draw_empty_right" : "anim_draw");
#endif
	HUD_SOUND::LoadSound(section, "snd_reload_1", m_sndReload1, m_eSoundReload);
	//
	m_bHasChamber = !!READ_IF_EXISTS(pSettings, r_bool, section, "has_chamber", false);
}

void CWeaponBM16::PlayReloadSound()
{
	if(m_magazine.size()==1)	PlaySound	(m_sndReload1,	get_LastFP());
	else						PlaySound	(sndReload,		get_LastFP());
}

void CWeaponBM16::UpdateSounds()
{
	inherited::UpdateSounds();

	if (m_sndReload1.playing())	m_sndReload1.set_position(get_LastFP());
}

void CWeaponBM16::PlayAnimShoot()
{
	if(m_magazine.size()==1)
		m_pHUD->animPlay(random_anim(mhud_shot1),TRUE,this,GetState());
	else
		m_pHUD->animPlay(random_anim(mhud.mhud_shots),TRUE,this,GetState());
}

void CWeaponBM16::PlayAnimReload()
{
	bool b_both = HaveCartridgeInInventory(2);

	VERIFY(GetState()==eReload);
	if(m_magazine.size()==1 || !b_both)
		m_pHUD->animPlay(random_anim(mhud_reload1),TRUE,this,GetState());
	else
		m_pHUD->animPlay(random_anim(mhud.mhud_reload),TRUE,this,GetState());

}

void CWeaponBM16::PlayAnimIdle(u8 state = eIdle) 
{
	if (TryPlayAnimIdle(state)) return;

	if(IsZoomed())
	{
		switch (m_magazine.size())
		{
		case 0:{
			m_pHUD->animPlay(random_anim(mhud.mhud_idle_aim), TRUE, NULL, GetState());
		}break;
		case 1:{
			m_pHUD->animPlay(random_anim(mhud_zoomed_idle1), TRUE, NULL, GetState());
		}break;
		case 2:{
			m_pHUD->animPlay(random_anim(mhud_zoomed_idle2), TRUE, NULL, GetState());
		}break;
		};
	}else{
		switch (m_magazine.size())
		{
		case 0:{
			m_pHUD->animPlay(random_anim(mhud.mhud_idle), TRUE, NULL, GetState());
		}break;
		case 1:{
			m_pHUD->animPlay(random_anim(mhud_idle1), TRUE, NULL, GetState());
		}break;
		case 2:{
			m_pHUD->animPlay(random_anim(mhud_idle2), TRUE, NULL, GetState());
		}break;
		};
	}
}

// Real Wolf. 03.08.2014.
#if defined(BM16_ANIMS_FIX)
void CWeaponBM16::switch2_Showing()
{
	HUD_SOUND::StopSound(sndReload);

	PlaySound(sndShow, get_LastFP());
	m_bPending = true;

	switch (this->GetAmmoElapsed() )
	{
	case 1:
		m_pHUD->animPlay(random_anim(mhud_draw_empty_right), FALSE, this, GetState());
		break;
	case 0:
		m_pHUD->animPlay(random_anim(mhud_draw_empty_both), FALSE, this, GetState());
		break;
	default:
		m_pHUD->animPlay(random_anim(mhud.mhud_show), FALSE, this, GetState());
	}	
}
#endif