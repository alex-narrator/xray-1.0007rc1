#include "StdAfx.h"
#include "../pch_script.h"
#include "UIMainIngameWnd.h"
#include "UIMotionIcon.h"
#include "UIXmlInit.h"
//added for hud_use_luminosity
#include "../game_cl_base.h"
#include "../level.h"
#include "../../CustomHUD.h"
//added for MotionIcon
#include "../Actor.h"
#include "../ActorCondition.h"
#include "../hudmanager.h"
#include <functional>

const LPCSTR MOTION_ICON_XML = "motion_icon.xml";

CUIMotionIcon::CUIMotionIcon()
{
	m_curren_state	= stLast;
	m_bchanged		= false;
	m_luminosity	= 0.0f;
}

CUIMotionIcon::~CUIMotionIcon()
{

}

void CUIMotionIcon::ResetVisibility()
{
	m_npc_visibility.clear	();
	m_bchanged				= true;
}

void CUIMotionIcon::Init()
{
	CUIXml uiXml;
	bool result = uiXml.Init(CONFIG_PATH, UI_PATH, MOTION_ICON_XML);
	R_ASSERT3(result, "xml file not found", MOTION_ICON_XML);

	CUIXmlInit	xml_init;

	AttachChild(&UIStaticLuminocity);
	xml_init.InitStatic(uiXml, "static_luminosity", 0, &UIStaticLuminocity);

	UIStaticLuminocity.AttachChild(&m_luminosity_progress);
	xml_init.InitProgressBar(uiXml, "luminosity_progress", 0, &m_luminosity_progress);
	//
	AttachChild(&UIStaticNoise);
	xml_init.InitStatic(uiXml, "static_noise", 0, &UIStaticNoise);

	UIStaticNoise.AttachChild(&m_noise_progress);
	xml_init.InitProgressBar(uiXml, "noise_progress", 0, &m_noise_progress);

	//
	AttachChild                 (&UIStaticMotionBack);
	xml_init.InitStatic			(uiXml, "background", 0, &UIStaticMotionBack);	

	UIStaticMotionBack.AttachChild(&m_power_progress);
	xml_init.InitProgressBar	(uiXml, "power_progress", 0, &m_power_progress);	
	
	UIStaticMotionBack.AttachChild(&m_states[stNormal]);
	xml_init.InitStatic			(uiXml, "state_normal", 0, &m_states[stNormal]);
	m_states[stNormal].Show		(false);

	UIStaticMotionBack.AttachChild(&m_states[stCrouch]);
	xml_init.InitStatic			(uiXml, "state_crouch", 0, &m_states[stCrouch]);	
	m_states[stCrouch].Show		(false);

	UIStaticMotionBack.AttachChild(&m_states[stCreep]);
	xml_init.InitStatic			(uiXml, "state_creep", 0, &m_states[stCreep]);	
	m_states[stCreep].Show		(false);

	UIStaticMotionBack.AttachChild(&m_states[stClimb]);
	xml_init.InitStatic			(uiXml, "state_climb", 0, &m_states[stClimb]);	
	m_states[stClimb].Show		(false);

	UIStaticMotionBack.AttachChild(&m_states[stRun]);
	xml_init.InitStatic			(uiXml, "state_run", 0, &m_states[stRun]);	
	m_states[stRun].Show		(false);

	UIStaticMotionBack.AttachChild(&m_states[stSprint]);
	xml_init.InitStatic			(uiXml, "state_sprint", 0, &m_states[stSprint]);	
	m_states[stSprint].Show		(false);

	ShowState					(stNormal);
	//
	InitStateColorize();
}

void CUIMotionIcon::InitStateColorize()
{
	u_ColorDefault = READ_IF_EXISTS(pSettings, r_color, "main_ingame_indicators_thresholds", "motion_icon_color_default", 0xff31c2b5);
	// Читаем данные порогов для индикатора
	shared_str cfgRecord = pSettings->r_string("main_ingame_indicators_thresholds", "motion_icon_health");
	u32 count = _GetItemCount(*cfgRecord);

	char	singleThreshold[8];
	float	f = 0;
	for (u32 k = 0; k < count; ++k)
	{
		_GetItem(*cfgRecord, k, singleThreshold);
		sscanf(singleThreshold, "%f", &f);

		m_Thresholds.push_back(f);
	}
}

void CUIMotionIcon::ShowState(EState state)
{
	if(m_curren_state==state)			return;
	if(m_curren_state!=stLast)
	{
	
		m_states[m_curren_state].Show	(false);
		m_states[m_curren_state].Enable	(false);
	}
	m_states[state].Show				(true);
	m_states[state].Enable				(true);

	m_curren_state=state;
}

void CUIMotionIcon::SetPower(float Pos)
{
	m_power_progress.SetProgressPos(Pos);
}

void CUIMotionIcon::SetNoise(float Pos)
{
	Pos	= clampr(Pos, m_noise_progress.GetRange_min(), m_noise_progress.GetRange_max());
	m_noise_progress.SetProgressPos(Pos);
}

void CUIMotionIcon::SetLuminosity(float Pos)
{
	Pos						= clampr(Pos, m_luminosity_progress.GetRange_min(), m_luminosity_progress.GetRange_max());
	m_luminosity			= Pos;
}

void CUIMotionIcon::SetStateWarningColor(EState state)
{
	if (g_eHudOnKey != eHudOnKeyMotionIcon)
	{
		if (m_states[state].GetColor() != u_ColorDefault)
			m_states[state].SetColor(u_ColorDefault);
		return;
	}

	CActor*	m_pActor = smart_cast<CActor*>(Level().CurrentViewEntity());

	float value = m_pActor ? 1 - m_pActor->conditions().GetHealth() : 0;
	
	// Минимальное и максимальное значения границы
	float min = m_Thresholds.front();
	float max = m_Thresholds.back();

	if (m_Thresholds.size() > 1)
	{
		xr_vector<float>::reverse_iterator	rit;

		// Сначала проверяем на точное соответсвие
		rit = std::find(m_Thresholds.rbegin(), m_Thresholds.rend(), value);

		// Если его нет, то берем последнее меньшее значение ()
		if (rit == m_Thresholds.rend())
			rit = std::find_if(m_Thresholds.rbegin(), m_Thresholds.rend(), std::bind2nd(std::less<float>(), value));

		if (rit != m_Thresholds.rend())
		{
			float v = *rit;
			m_states[state].SetColor(color_argb(
				0xFF, 
				clampr<u32>(static_cast<u32>(255 * ((v - min) / (max - min) * 2)), 0, 255),
				clampr<u32>(static_cast<u32>(255 * (2.0f - (v - min) / (max - min) * 2)), 0, 255),
				0
				));
		}
		else
			m_states[state].SetColor(u_ColorDefault);
	}
	else
	{
		float val = 1 - value;
		float treshold = 1 - min;
		clamp<float>(treshold, 0.01, 1.f);

		if (val <= treshold)
		{
			float v = val / treshold;
			clamp<float>(v, 0.f, 1.f);
			m_states[state].SetColor(color_argb(
				0xFF,
				255,
				clampr<u32>(static_cast<u32>(255 * v), 0, 255),
				0
				));
		}
		else
			m_states[state].SetColor(u_ColorDefault);
	}
}

void CUIMotionIcon::Update()
{
	auto CurrentHUD			= HUD().GetUI()->UIMainIngameWnd;
	bool show_motion_icon	= g_eHudOnKey != eHudOnKeyWarningIcon;
	bool show_progress_bar	= CurrentHUD->IsHUDElementAllowed(ePDA);

	//статик положения персонажа и выносливости
	UIStaticMotionBack.Show (show_motion_icon);
	//раскраска иконки положения персонажа
	SetStateWarningColor	(m_curren_state);
	//статики прогресс-баров освещенности/заметности и шума
	UIStaticLuminocity.Show (show_progress_bar);
	UIStaticNoise.Show      (show_progress_bar);
	//
	CActor*	m_pActor = smart_cast<CActor*>(Level().CurrentViewEntity());

	SetNoise((s16)(0xffff & iFloor(m_pActor->m_snd_noise*100.0f)));
	SetPower(m_pActor->conditions().GetPower()*100.0f);
	//

	if (!psHUD_Flags.test(HUD_USE_LUMINOSITY)) //использование освещённости вместо заметности на худовой шкале
	{
		if (m_bchanged)
		{
			m_bchanged = false;
			if (m_npc_visibility.size())
			{
				std::sort(m_npc_visibility.begin(), m_npc_visibility.end());
				SetLuminosity(m_npc_visibility.back().value);
			}
			else
				SetLuminosity(m_luminosity_progress.GetRange_min());
		}
	}
	else //ранее этот код был в UIMainIngameWnd, использовался в мультиплеере
	{
		float		luminocity = smart_cast<CGameObject*>(Level().CurrentEntity())->ROS()->get_luminocity();
		float		power = log(luminocity > .001f ? luminocity : .001f)*(1.f/*luminocity_factor*/);
		luminocity = exp(power);

		static float cur_lum = luminocity;
		cur_lum = luminocity*0.01f + cur_lum*0.99f;
		SetLuminosity((s16)iFloor(cur_lum*100.0f));
	}

	inherited::Update();
	
	//m_luminosity_progress 
	{
		float len					= m_noise_progress.GetRange_max()-m_noise_progress.GetRange_min();
		float cur_pos				= m_luminosity_progress.GetProgressPos();
		if(cur_pos!=m_luminosity){
			float _diff = _abs(m_luminosity-cur_pos);
			if(m_luminosity>cur_pos){
				cur_pos				+= _min(len*Device.fTimeDelta, _diff);
			}else{
				cur_pos				-= _min(len*Device.fTimeDelta, _diff);
			}
			clamp(cur_pos, m_noise_progress.GetRange_min(), m_noise_progress.GetRange_max());
			m_luminosity_progress.SetProgressPos(cur_pos);
		}
	}
}

void CUIMotionIcon::SetActorVisibility		(u16 who_id, float value)
{
	float v		= float(m_luminosity_progress.GetRange_max() - m_luminosity_progress.GetRange_min());
	value		*= v;
	value		+= m_luminosity_progress.GetRange_min();

	xr_vector<_npc_visibility>::iterator it = std::find(m_npc_visibility.begin(), 
														m_npc_visibility.end(),
														who_id);

	if(it==m_npc_visibility.end() && value!=0)
	{
		m_npc_visibility.resize	(m_npc_visibility.size()+1);
		_npc_visibility& v		= m_npc_visibility.back();
		v.id					= who_id;
		v.value					= value;
	}
	else if( fis_zero(value) )
	{
		if (it!=m_npc_visibility.end())
			m_npc_visibility.erase	(it);
	}
	else
	{
		(*it).value				= value;
	}

	m_bchanged = true;
}
