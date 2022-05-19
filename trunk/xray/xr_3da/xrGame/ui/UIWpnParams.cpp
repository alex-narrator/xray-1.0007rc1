#include "stdafx.h"
#include "../pch_script.h"
#include "UIWpnParams.h"
#include "UIXmlInit.h"
#include "../level.h"
#include "../game_base_space.h"
#include "../ai_space.h"
#include "../script_engine.h"
#include "../script_game_object.h"
#include "../clsid_game.h"
//
#include "../smart_cast.h"
#include "../string_table.h"
#include "../weapon.h"
#include "../weaponmagazined.h"

struct SLuaWpnParams{
	luabind::functor<float>		m_functorRPM;
	luabind::functor<float>		m_functorAccuracy;
	luabind::functor<float>		m_functorDamage;
	luabind::functor<float>		m_functorDamageMP;
	luabind::functor<float>		m_functorHandling;
	SLuaWpnParams();
	~SLuaWpnParams();
};

SLuaWpnParams::SLuaWpnParams()
{
	bool	functor_exists;
	functor_exists	= ai().script_engine().functor("ui_wpn_params.GetRPM" ,		m_functorRPM);		VERIFY(functor_exists);
	functor_exists	= ai().script_engine().functor("ui_wpn_params.GetDamage" ,	m_functorDamage);	VERIFY(functor_exists);
	functor_exists	= ai().script_engine().functor("ui_wpn_params.GetDamageMP" ,m_functorDamageMP);	VERIFY(functor_exists);
	functor_exists	= ai().script_engine().functor("ui_wpn_params.GetHandling" ,m_functorHandling);	VERIFY(functor_exists);
	functor_exists	= ai().script_engine().functor("ui_wpn_params.GetAccuracy" ,m_functorAccuracy);	VERIFY(functor_exists);
}

SLuaWpnParams::~SLuaWpnParams()
{
}

SLuaWpnParams* g_lua_wpn_params = NULL;

void destroy_lua_wpn_params()
{
	if(g_lua_wpn_params)
		xr_delete(g_lua_wpn_params);
}

CUIWpnParams::CUIWpnParams(){
	AttachChild(&m_textAccuracy);
	AttachChild(&m_textDamage);
	AttachChild(&m_textHandling);
	AttachChild(&m_textRPM);

	AttachChild(&m_progressAccuracy);
	AttachChild(&m_progressDamage);
	AttachChild(&m_progressHandling);
	AttachChild(&m_progressRPM);
	//
	AttachChild(&m_textCurrentAmmo);
	AttachChild(&m_textMagSizeFiremode);
}

CUIWpnParams::~CUIWpnParams()
{
}

void CUIWpnParams::InitFromXml(CUIXml& xml_doc){
	if (!xml_doc.NavigateToNode("wpn_params", 0))	return;
	CUIXmlInit::InitWindow			(xml_doc, "wpn_params", 0, this);

	CUIXmlInit::InitStatic			(xml_doc, "wpn_params:cap_accuracy",		0, &m_textAccuracy);
	CUIXmlInit::InitStatic			(xml_doc, "wpn_params:cap_damage",			0, &m_textDamage);
	CUIXmlInit::InitStatic			(xml_doc, "wpn_params:cap_handling",		0, &m_textHandling);
	CUIXmlInit::InitStatic			(xml_doc, "wpn_params:cap_rpm",				0, &m_textRPM);

	CUIXmlInit::InitProgressBar		(xml_doc, "wpn_params:progress_accuracy",	0, &m_progressAccuracy);
	CUIXmlInit::InitProgressBar		(xml_doc, "wpn_params:progress_damage",		0, &m_progressDamage);
	CUIXmlInit::InitProgressBar		(xml_doc, "wpn_params:progress_handling",	0, &m_progressHandling);
	CUIXmlInit::InitProgressBar		(xml_doc, "wpn_params:progress_rpm",		0, &m_progressRPM);

	m_progressAccuracy.SetRange		(0, 100);
	m_progressDamage.SetRange		(0, 100);
	m_progressHandling.SetRange		(0, 100);
	m_progressRPM.SetRange			(0, 100);
	//
	CUIXmlInit::InitStatic			(xml_doc, "wpn_params:info_current_ammo",	   0, &m_textCurrentAmmo);
	CUIXmlInit::InitStatic			(xml_doc, "wpn_params:info_mag_size_firemode", 0, &m_textMagSizeFiremode);

}

void CUIWpnParams::SetInfo(CInventoryItem *wpn)
{
	const shared_str &wpn_section = wpn->object().cNameSect();

	if(!g_lua_wpn_params)
		g_lua_wpn_params = xr_new<SLuaWpnParams>();

	lua_State *L = game_lua();
	VERIFY(L);
	lua_getglobal(L, "ui_wpn_params");
	if (lua_istable(L, -1))
	{
		lua_pushgameobject(L, wpn->cast_game_object());
		lua_setfield(L, -2, "wpn_object"); // alpet: позволит динамически выбрать параметры из скрипта
	}
	lua_pop(L, 1);	

	m_progressRPM.SetProgressPos		(g_lua_wpn_params->m_functorRPM(*wpn_section));
	m_progressAccuracy.SetProgressPos	(g_lua_wpn_params->m_functorAccuracy(*wpn_section));
	if (GameID() == GAME_SINGLE)
        m_progressDamage.SetProgressPos	(g_lua_wpn_params->m_functorDamage(*wpn_section));
	else
		m_progressDamage.SetProgressPos	(g_lua_wpn_params->m_functorDamageMP(*wpn_section));
	m_progressHandling.SetProgressPos	(g_lua_wpn_params->m_functorHandling(*wpn_section));
	//
	string1024 text_to_show;
	char temp_text[64];
	auto pWeapon			= smart_cast<CWeapon*>					(wpn);
	auto pWeaponMag			= smart_cast<CWeaponMagazined*>			(wpn);
	//кол-во и тип снаряженных боеприпасов
	sprintf_s(temp_text, " %d | %s", pWeapon->GetAmmoElapsed(), pWeapon->GetCurrentAmmo_ShortName());
	strconcat(sizeof(text_to_show), text_to_show, *CStringTable().translate("st_current_ammo"), pWeapon->GetAmmoElapsed() ? temp_text : *CStringTable().translate("st_not_loaded"));
	m_textCurrentAmmo.SetText(text_to_show);
	//размер магазина и текущий режим огня
	sprintf_s(temp_text, pWeapon->GetGrenadeMode() ? *CStringTable().translate("st_gl_mode") :
		" %d |%s", pWeapon->GetAmmoMagSize(), pWeaponMag->HasFireModes() ? pWeaponMag->GetCurrentFireModeStr() : " (1)");
	strconcat(sizeof(text_to_show), text_to_show, *CStringTable().translate("st_mag_size_fire_mode"), temp_text);
	m_textMagSizeFiremode.SetText(text_to_show);
}

bool CUIWpnParams::Check(CInventoryItem *wpn)
{
	if (pSettings->line_exist(wpn->object().cNameSect(), "fire_dispersion_base"))
	{
		const char* wpn_clsid_str = pSettings->r_string(wpn->object().cNameSect(), "class");
		if (strstr(wpn_clsid_str, "KNIFE")
			|| strstr(wpn_clsid_str, "SILENC")
			|| strstr(wpn_clsid_str, "BINOC")
			)
		{
			return false;
		}
        return true;		
	}
	else
		return false;
}