#include "pch_script.h"
#include "actorcondition.h"
#include "actor.h"
#include "actorEffector.h"
#include "inventory.h"
#include "level.h"
#include "sleepeffector.h"
#include "game_base_space.h"
#include "autosave_manager.h"
#include "xrserver.h"
#include "ai_space.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
#include "game_object_space.h"
#include "ui\UIVideoPlayerWnd.h"
#include "script_callback_ex.h"
#include "object_broker.h"
#include "weapon.h"

#define MAX_SATIETY					1.0f
#define START_SATIETY				0.5f

BOOL	GodMode	()	
{ 
	if (GameID() == GAME_SINGLE) 
		return psActorFlags.test(AF_GODMODE); 
	return FALSE;	
}

CActorCondition::CActorCondition(CActor *object) :
	inherited	(object)
{
	m_fJumpPower				= 0.f;
	m_fStandPower				= 0.f;
	m_fWalkPower				= 0.f;
	m_fJumpWeightPower			= 0.f;
	m_fWalkWeightPower			= 0.f;
	m_fOverweightWalkK			= 0.f;
	m_fOverweightJumpK			= 0.f;
	m_fAccelK					= 0.f;
	m_fSprintK					= 0.f;
	m_fAlcohol					= 0.f;
	m_fSatiety					= 1.0f;

	VERIFY						(object);
	m_object					= object;
	m_condition_flags.zero		();
	//
	m_bFlagState				= !!psActorFlags.test(AF_SURVIVAL);
}

CActorCondition::~CActorCondition(void)
{
}

void CActorCondition::LoadCondition(LPCSTR entity_section)
{
	inherited::LoadCondition(entity_section);

	LPCSTR						section = READ_IF_EXISTS(pSettings,r_string,entity_section,"condition_sect",entity_section);

	m_fJumpPower				= pSettings->r_float(section,"jump_power");
	m_fStandPower				= pSettings->r_float(section,"stand_power");
	m_fWalkPower				= pSettings->r_float(section,"walk_power");
	m_fJumpWeightPower			= pSettings->r_float(section,"jump_weight_power");
	m_fWalkWeightPower			= pSettings->r_float(section,"walk_weight_power");
	m_fOverweightWalkK			= pSettings->r_float(section,"overweight_walk_k");
	m_fOverweightJumpK			= pSettings->r_float(section,"overweight_jump_k");
	m_fAccelK					= pSettings->r_float(section,"accel_k");
	m_fSprintK					= pSettings->r_float(section,"sprint_k");

	//порог силы и здоровья меньше которого актер начинает хромать
	m_fLimpingHealthBegin		= pSettings->r_float(section,	"limping_health_begin");
	m_fLimpingHealthEnd			= pSettings->r_float(section,	"limping_health_end");
	R_ASSERT					(m_fLimpingHealthBegin<=m_fLimpingHealthEnd);

	m_fLimpingPowerBegin		= pSettings->r_float(section,	"limping_power_begin");
	m_fLimpingPowerEnd			= pSettings->r_float(section,	"limping_power_end");
	R_ASSERT					(m_fLimpingPowerBegin<=m_fLimpingPowerEnd);

	m_fCantWalkPowerBegin		= pSettings->r_float(section,	"cant_walk_power_begin");
	m_fCantWalkPowerEnd			= pSettings->r_float(section,	"cant_walk_power_end");
	R_ASSERT					(m_fCantWalkPowerBegin<=m_fCantWalkPowerEnd);

	m_fCantSprintPowerBegin		= pSettings->r_float(section,	"cant_sprint_power_begin");
	m_fCantSprintPowerEnd		= pSettings->r_float(section,	"cant_sprint_power_end");
	R_ASSERT					(m_fCantSprintPowerBegin<=m_fCantSprintPowerEnd);

	m_fPowerLeakSpeed			= pSettings->r_float(section,"max_power_leak_speed");
	
	m_fV_Alcohol				= pSettings->r_float(section,"alcohol_v");

	m_fSatietyCritical			= pSettings->r_float(section,"satiety_critical");
	m_fV_Satiety				= pSettings->r_float(section,"satiety_v");		
	m_fV_SatietyPower			= pSettings->r_float(section,"satiety_power_v");
	m_fV_SatietyHealth			= pSettings->r_float(section,"satiety_health_v");

	//m_MaxWalkWeight				= pSettings->r_float(section,"max_walk_weight");
    //
	m_fBleedingPowerDecrease		= READ_IF_EXISTS(pSettings, r_float, "actor_survival", "bleeding_power_dec",						0.f);
    //
	m_fMinPowerWalkJump				= READ_IF_EXISTS(pSettings, r_float, "actor_survival", "min_power_walk_jump",						1.0f);
	//
	m_fMinHealthRadiation			= READ_IF_EXISTS(pSettings, r_float, "actor_survival", "min_health_radiation",						1.0f);
	m_fMinHealthRadiationTreshold	= READ_IF_EXISTS(pSettings, r_float, "actor_survival", "min_health_radiation_treshold",				0.f);
    //
	m_fAlcoholSatietyIntens			= READ_IF_EXISTS(pSettings, r_float, "actor_survival", "satiety_to_alcohol_effector_intensity",		1.0f);
	//
	m_fExerciseStressFactor			= READ_IF_EXISTS(pSettings, r_float, "actor_survival", "exercise_stress_factor",					1.0f);
	//
	m_fZoomEffectorK				= READ_IF_EXISTS(pSettings, r_float, "actor_survival", "power_to_zoom_effector_k",					10.0f);
	//
	m_fV_HardHoldPower				= READ_IF_EXISTS(pSettings, r_float, section, "hard_hold_power_v",									0.f);
}


//вычисление параметров с ходом времени
#include "UI.h"
#include "HUDManager.h"
#include "CharacterPhysicsSupport.h"
void CActorCondition::UpdateCondition()
{
	if (GodMode())				return;
	if (!object().g_Alive())	return;
	if (!object().Local() && m_object != Level().CurrentViewEntity())		return;
	//
	if (IsCantWalk() && object().character_physics_support()->movement()->PHCapture())
		object().character_physics_support()->movement()->PHReleaseObject();
	//
	float weight = object().GetCarryWeight();
	float max_weight = object().inventory().GetMaxWeight();
	float weight_coef = weight / max_weight;
	//
	if ((object().mstate_real&mcAnyMove)) 
	{
		ConditionWalk(weight_coef,
			isActorAccelerated(object().mstate_real, object().IsZoomAimingMode()), 
			(object().mstate_real&mcSprint) != 0);
	}
	else 
	{
		ConditionStand(weight_coef);
	};
	//
	/*UpdateHealth();
	UpdatePower();
	UpdateRadiation();
	UpdatePsyHealth();*/
	//этого нет в EntityCondition
	UpdateAlcohol();
	UpdateSatiety();
	//
	inherited::UpdateCondition();

	if (IsGameTypeSingle())
		UpdateTutorialThresholds();
}

float CActorCondition::BleedingSpeed()
{
	return inherited::BleedingSpeed() * GetStress();
}

void CActorCondition::UpdateHealth()
{
	float bleeding_speed = BleedingSpeed() * m_fDeltaTime * m_change_v.m_fV_Bleeding;
	m_bIsBleeding = fis_zero(bleeding_speed) ? false : true;
	m_fDeltaHealth -= CanBeHarmed() ? bleeding_speed : 0;

	m_fDeltaHealth += m_fDeltaTime * m_change_v.m_fV_HealthRestore * GetRegenKoef();
	VERIFY(_valid(m_fDeltaHealth));

	ChangeBleeding(m_change_v.m_fV_WoundIncarnation * m_fDeltaTime * GetRegenKoef());

	//радиация влияет на максимальное здоровье
	if (psActorFlags.test(AF_SURVIVAL) && m_fRadiation > m_fMinHealthRadiationTreshold) //защита от потенциального деления на 0 если m_fRadiationTreshold = 1
		SetMaxHealth(m_fMinHealthRadiation + (1.0f - m_fMinHealthRadiation) * (1.0f - m_fRadiation) / (1.0f - m_fMinHealthRadiationTreshold));
	else
		SetMaxHealth(1.0f);

	//debug
	/*Msg("Max_Health [%f]", GetMaxHealth());
	Msg("regen_koef [%f]", GetRegenKoef());
	Msg("stress [%f]", GetStress());
	Msg("bleeding_speed [%f]", BleedingSpeed());
	Msg("ChangeBleeding [%f]", m_change_v.m_fV_WoundIncarnation * m_fDeltaTime * GetRegenKoef());*/
}

void CActorCondition::UpdatePower()
{
	if (IsGameTypeSingle())
	{

		float k_max_power = 1.0f;

		if (true)
		{
			//float weight = object().inventory().TotalWeight();
			float weight = object().GetCarryWeight();

			float base_w = object().MaxCarryWeight();
			/*
			CCustomOutfit* outfit	= m_object->GetOutfit();
			if(outfit)
			base_w += outfit->m_additional_weight2;
			*/

			k_max_power = 1.0f + _min(weight, base_w) / base_w + _max(0.0f, (weight - base_w) / 10.0f);

		}
		else
			k_max_power = 1.0f;

		SetMaxPower(GetMaxPower() - m_fPowerLeakSpeed*m_fDeltaTime*k_max_power); //кажется это таки "сонливость" - постоянное уменьшение максимальной выносливости
	}

	//коэффициенты уменьшения восстановления силы от сытоти и радиации
	/*float radiation_power_k = 1.f;
	float satiety_power_k = 1.f;*/
	float bleeding_power_dec = BleedingSpeed() * /*m_fDeltaTime * m_change_v.m_fV_Bleeding */ m_fBleedingPowerDecrease;

	m_fDeltaPower += m_fV_SatietyPower*
		//radiation_power_k*
		//satiety_power_k*
		m_fDeltaTime * GetRegenKoef() - bleeding_power_dec;

	//задержка дыхания
	if (object().IsHardHold() && !object().is_actor_creep() && GetPower() > m_fCantWalkPowerEnd)
	{
		float inertion_factor = object().inventory().ActiveItem()->GetControlInertionFactor();
		m_fDeltaPower -= m_fDeltaTime * m_fV_HardHoldPower * inertion_factor;
	}
	else
		object().SetHardHold(false);
}

void CActorCondition::UpdatePsyHealth()
{
	/*if (m_fPsyHealth>0)
	{
		m_fDeltaPsyHealth += m_change_v.m_fV_PsyHealth*m_fDeltaTime;
	}*/
	inherited::UpdatePsyHealth();

	if (IsGameTypeSingle())
	{
		CEffectorPP* ppe = object().Cameras().GetPPEffector((EEffectorPPType)effPsyHealth);

		string64			pp_sect_name;
		shared_str ln = Level().name();
		strconcat(sizeof(pp_sect_name), pp_sect_name, "effector_psy_health", "_", *ln);
		if (!pSettings->section_exist(pp_sect_name))
			strcpy_s(pp_sect_name, "effector_psy_health");

		if (!fsimilar(GetPsyHealth(), 1.0f, 0.05f))
		{
			if (!ppe)
			{
				AddEffector(m_object, effPsyHealth, pp_sect_name, GET_KOEFF_FUNC(this, &CActorCondition::GetPsy));
			}
		}
		else
		{
			if (ppe)
				RemoveEffector(m_object, effPsyHealth);
		}
		//смерть при нулевом пси-здоровье
		if (fis_zero(GetPsyHealth()))
			health() = 0.0f;
	};
}

void CActorCondition::UpdateRadiation()
{
	if (m_fRadiation>0)
	{
		m_fDeltaRadiation -= m_change_v.m_fV_Radiation*m_fDeltaTime;
		//радиация постоянно отнимает здоровье только если выкючена опция взаимозависимости параметров
		m_fDeltaHealth -= CanBeHarmed() && !psActorFlags.test(AF_SURVIVAL) ? m_change_v.m_fV_RadiationHealth*m_fRadiation*m_fDeltaTime : 0.0f;
		//Msg("CActorCondition m_fDeltaHealth [%f]", m_fDeltaHealth);
	}
}

void CActorCondition::UpdateAlcohol()
{
	m_fAlcohol += m_fV_Alcohol*m_fDeltaTime;
	clamp(m_fAlcohol, 0.0f, 1.0f);

	bool flag_state = !!psActorFlags.test(AF_SURVIVAL);

	if (IsGameTypeSingle())
	{
		CEffectorCam* ce = Actor()->Cameras().GetCamEffector((ECamEffectorType)effAlcohol);
		if ((m_fAlcohol > 0.0001f))
		{
			if (!ce)
			{
				AddEffector(m_object, effAlcohol, "effector_alcohol", GET_KOEFF_FUNC(this, flag_state ?
					&CActorCondition::AlcoholSatiety : 
					&CActorCondition::GetAlcohol));
			}
			else if (m_bFlagState != flag_state) //удалим эффектор для его передобавления если опцию изменили в процессе
			{
					RemoveEffector(m_object, effAlcohol);
					m_bFlagState = flag_state;
					//Msg("Restart effector");
			}
		}
		else
		{
			if (ce) RemoveEffector(m_object, effAlcohol);
		}
		//смерть при максимальном опьянении
		if (fsimilar(GetAlcohol(), 1.0f))
			health() = 0.0f;
	}
	//Msg("Alcohol [%f], Satiety [%f], AlcoSat [%f]", GetAlcohol(), GetSatiety(), AlcoholSatiety());
}

void CActorCondition::UpdateSatiety()
{
	if (!IsGameTypeSingle()) return;

	if(m_fSatiety>0)
	{
		m_fSatiety -=	m_fV_Satiety * m_fDeltaTime * GetStress();	
		clamp(m_fSatiety,		0.0f,		1.0f);
	}		
	//сытость увеличивает здоровье только если нет открытых ран
		m_fDeltaHealth += CanBeHarmed() && !m_bIsBleeding ?
			(m_fV_SatietyHealth*(m_fSatiety>m_fSatietyCritical ? 1.f : -1.f)*m_fDeltaTime*GetRegenKoef()) //по идее тут надо сравнить с m_fSatietyCritical
			: 0;
}

float CActorCondition::GetSmoothOwerweightKoef()
{
	float val = 1.0f;

	if (psActorFlags.test(AF_SMOOTH_OVERWEIGHT))
	{
		float power_k		= m_fMinPowerWalkJump + (1.0f - m_fMinPowerWalkJump) * GetPower();				//коэф влияния выносливости

		float overweight_k = object().GetCarryWeight() > object().MaxCarryWeight() ?	//считаем коэф. только если есть перегруз
			object().MaxCarryWeight() / object().GetCarryWeight() :	//коэф влияния перегруза
								1.0f;

		val = power_k * overweight_k;
	}
	//Msg("SmoothOverweightK = %f", val);
	return val;
}

float CActorCondition::GetStress()
{
	float exercise_stress = psActorFlags.test(AF_SURVIVAL) && object().get_state()&(mcSprint | mcJump) ?
								m_fExerciseStressFactor : 
								1.0f;

	float overweight_stress = psActorFlags.test(AF_SMOOTH_OVERWEIGHT) && object().GetCarryWeight() > object().MaxCarryWeight() ?
								object().GetCarryWeight() / object().MaxCarryWeight() : 
								1.0f;

	/*if (object().get_state()&mcSprint) Msg("mcSprint!");
	else
		if (object().get_state()&mcJump) Msg("Jump!");
	Msg("overweight_stress = %f | exercise_stress = %f | m_fStressK = %f", overweight_stress, exercise_stress, m_fStressK);*/

	return overweight_stress * exercise_stress;
}

CWound* CActorCondition::ConditionHit(SHit* pHDS)
{
	if (GodMode()) return NULL;
	return inherited::ConditionHit(pHDS);
}

//weight - "удельный" вес от 0..1
void CActorCondition::ConditionJump(float weight)
{
	float power			=	m_fJumpPower;
	power				+=	m_fJumpWeightPower*weight*(weight>1.f?m_fOverweightJumpK:1.f);
	m_fPower			-=	HitPowerEffect(power);
}
void CActorCondition::ConditionWalk(float weight, bool accel, bool sprint)
{	
	float power			=	m_fWalkPower;
	power				+=	m_fWalkWeightPower*weight*(weight>1.f?m_fOverweightWalkK:1.f);
	power				*=	m_fDeltaTime*(accel?(sprint?m_fSprintK:m_fAccelK):1.f);
	m_fPower			-=	HitPowerEffect(power);
}

void CActorCondition::ConditionStand(float weight)
{	
	float power = m_fStandPower * GetRegenKoef();
	power *= m_fDeltaTime;
	m_fPower -= power;
}

bool CActorCondition::IsCantWalk() const
{
	if(m_fPower< m_fCantWalkPowerBegin)
		m_bCantWalk		= true;
	else if(m_fPower > m_fCantWalkPowerEnd)
		m_bCantWalk		= false;
	return				m_bCantWalk;
}

#include "CustomOutfit.h"
#include "BackPack.h"
#include "Artifact.h"

bool CActorCondition::IsCantWalkWeight()
{
	if (IsGameTypeSingle() && !GodMode() && !psActorFlags.test(AF_SMOOTH_OVERWEIGHT)) //обездвиживание по перегрузу только если отключена опция плавного перегруза
	{
/*		float max_w				= m_MaxWalkWeight;

		auto outfit	= m_object->GetOutfit();
		if (outfit && !fis_zero(outfit->GetCondition()))
			max_w += outfit->GetAdditionalMaxWalkWeight();//m_additional_weight;

		auto backpack = m_object->GetBackPack();
		if (backpack && !fis_zero(backpack->GetCondition()))
			max_w += backpack->GetAdditionalMaxWalkWeight();

		TIItemContainer &list = psActorFlags.test(AF_ARTEFACTS_FROM_ALL) ? object().inventory().m_all : object().inventory().m_belt;
		for (TIItemContainer::iterator it = list.begin(); list.end() != it; ++it)
		{
			auto artefact = smart_cast<CArtefact*>(*it);

			if (artefact && !artefact->InContainer() && !fis_zero(artefact->GetCondition()))
			{
				max_w += artefact->GetAdditionalMaxWalkWeight();
			}
		}*/

		if(object().GetCarryWeight() > /*max_w*/object().MaxCarryWeight())
		{
			m_condition_flags.set			(eCantWalkWeight, TRUE);
			return true;
		}

	}
	m_condition_flags.set					(eCantWalkWeight, FALSE);
	return false;
}

bool CActorCondition::IsCantSprint() const
{
	if(m_fPower< m_fCantSprintPowerBegin)
		m_bCantSprint	= true;
	else if(m_fPower > m_fCantSprintPowerEnd)
		m_bCantSprint	= false;
	return				m_bCantSprint;
}

bool CActorCondition::IsLimping() const
{
	if(m_fPower< m_fLimpingPowerBegin || GetHealth() < m_fLimpingHealthBegin)
		m_bLimping = true;
	else if(m_fPower > m_fLimpingPowerEnd && GetHealth() > m_fLimpingHealthEnd)
		m_bLimping = false;
	return m_bLimping;
}
extern bool g_bShowHudInfo;

void CActorCondition::save(NET_Packet &output_packet)
{
	inherited::save		(output_packet);
	save_data			(m_fAlcohol, output_packet);
	save_data			(m_condition_flags, output_packet);
	save_data			(m_fSatiety, output_packet);
}

void CActorCondition::load(IReader &input_packet)
{
	inherited::load		(input_packet);
	load_data			(m_fAlcohol, input_packet);
	load_data			(m_condition_flags, input_packet);
	load_data			(m_fSatiety, input_packet);
}

void CActorCondition::reinit	()
{
	inherited::reinit	();
	m_bLimping					= false;
	m_fSatiety					= 1.f;
}

void CActorCondition::ChangeAlcohol	(float value)
{
	m_fAlcohol += value;
}

void CActorCondition::ChangeSatiety(float value)
{
	m_fSatiety += value;
	clamp		(m_fSatiety, 0.0f, 1.0f);
}

void CActorCondition::UpdateTutorialThresholds()
{
	string256						cb_name;
	static float _cPowerThr			= pSettings->r_float("tutorial_conditions_thresholds","power");
	static float _cPowerMaxThr		= pSettings->r_float("tutorial_conditions_thresholds","max_power");
	static float _cBleeding			= pSettings->r_float("tutorial_conditions_thresholds","bleeding");
	static float _cSatiety			= pSettings->r_float("tutorial_conditions_thresholds","satiety");
	static float _cRadiation		= pSettings->r_float("tutorial_conditions_thresholds","radiation");
	static float _cWpnCondition		= pSettings->r_float("tutorial_conditions_thresholds","weapon_jammed");
	static float _cPsyHealthThr		= pSettings->r_float("tutorial_conditions_thresholds","psy_health");



	bool b = true;
	if(b && !m_condition_flags.test(eCriticalPowerReached) && GetPower()<_cPowerThr){
		m_condition_flags.set			(eCriticalPowerReached, TRUE);
		b=false;
		strcpy_s(cb_name,"_G.on_actor_critical_power");
	}

	if(b && !m_condition_flags.test(eCriticalMaxPowerReached) && GetMaxPower()<_cPowerMaxThr){
		m_condition_flags.set			(eCriticalMaxPowerReached, TRUE);
		b=false;
		strcpy_s(cb_name,"_G.on_actor_critical_max_power");
	}

	if(b && !m_condition_flags.test(eCriticalBleedingSpeed) && BleedingSpeed()>_cBleeding){
		m_condition_flags.set			(eCriticalBleedingSpeed, TRUE);
		b=false;
		strcpy_s(cb_name,"_G.on_actor_bleeding");
	}

	if(b && !m_condition_flags.test(eCriticalSatietyReached) && GetSatiety()<_cSatiety){
		m_condition_flags.set			(eCriticalSatietyReached, TRUE);
		b=false;
		strcpy_s(cb_name,"_G.on_actor_satiety");
	}

	if(b && !m_condition_flags.test(eCriticalRadiationReached) && GetRadiation()>_cRadiation){
		m_condition_flags.set			(eCriticalRadiationReached, TRUE);
		b=false;
		strcpy_s(cb_name,"_G.on_actor_radiation");
	}

	if(b && !m_condition_flags.test(ePhyHealthMinReached) && GetPsyHealth()>_cPsyHealthThr){
		m_condition_flags.set			(ePhyHealthMinReached, TRUE);
		b=false;
		strcpy_s(cb_name,"_G.on_actor_psy");
	}

	if(b && !m_condition_flags.test(eCantWalkWeight)){
		m_condition_flags.set			(eCantWalkWeight, TRUE);
		b=false;
		strcpy_s(cb_name,"_G.on_actor_cant_walk_weight");
	}

	if(b && !m_condition_flags.test(eWeaponJammedReached)&&m_object->inventory().GetActiveSlot()!=NO_ACTIVE_SLOT){
		PIItem item							= m_object->inventory().ItemFromSlot(m_object->inventory().GetActiveSlot());
		CWeapon* pWeapon					= smart_cast<CWeapon*>(item); 
		if(pWeapon&&pWeapon->GetCondition()<_cWpnCondition){
			m_condition_flags.set			(eWeaponJammedReached, TRUE);b=false;
			strcpy_s(cb_name,"_G.on_actor_weapon_jammed");
		}
	}
	
	if(!b){
		luabind::functor<LPCSTR>			fl;
		R_ASSERT							(ai().script_engine().functor<LPCSTR>(cb_name,fl));
		fl									();
	}
}
