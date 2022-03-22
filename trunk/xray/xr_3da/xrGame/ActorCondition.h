// ActorCondition.h: класс состояния игрока
//

#pragma once

#include "EntityCondition.h"
#include "actor_defs.h"
#include "actor_flags.h"

template <typename _return_type>
class CScriptCallbackEx;


class CActor;
//class CUIActorSleepVideoPlayer;

class CActorCondition: public CEntityCondition {
	friend class CScriptActor;
private:
	typedef CEntityCondition inherited;
	enum {	eCriticalPowerReached			=(1<<0),
			eCriticalMaxPowerReached		=(1<<1),
			eCriticalBleedingSpeed			=(1<<2),
			eCriticalSatietyReached			=(1<<3),
			eCriticalRadiationReached		=(1<<4),
			eWeaponJammedReached			=(1<<5),
			ePhyHealthMinReached			=(1<<6),
			eCantWalkWeight					=(1<<7),
			};
	Flags16											m_condition_flags;
private:
	CActor*											m_object;
	bool											m_bFlagState;
	void				UpdateTutorialThresholds	();
	void 				UpdateSatiety				();
	void                UpdateAlcohol				();
	//виртуальные методы CEntityCondition
	virtual void		UpdateHealth				();
	virtual void		UpdatePower					();
	virtual void		UpdateRadiation				();
	virtual void		UpdatePsyHealth				();
	//
public:
						CActorCondition				(CActor *object);
	virtual				~CActorCondition			(void);

	virtual void		LoadCondition				(LPCSTR section);
	virtual void		reinit						();

	virtual CWound*		ConditionHit				(SHit* pHDS);
	/*virtual*/ void	UpdateCondition				();
	//скорость потери крови из всех открытых ран 
	virtual float		BleedingSpeed               ();
	//
	float				GetSmoothOwerweightKoef		();
	//коэфф. выносливости - на будущее для влияния на удар ножа, бросок гранаты и т.д.
	float				GetPowerKoef				() { return psActorFlags.test(AF_SURVIVAL) ? GetPower() : 1.0f; };
	//коэфф. регенерации актора - зависит от сытости и дозы облучения
	float				GetRegenKoef				() { return psActorFlags.test(AF_SURVIVAL) ? (1.0f - GetRadiation()) * GetSatiety() : 1.0f; };
	//коэффициент нагрузки актора
	float               GetStress					();
	//во сколько раз больше трясутся руки в прицеливании при полном отсутствии выносливости
	float				GetZoomEffectorKoef			() { return m_fZoomEffectorK; };

	virtual void 		ChangeAlcohol				(float value);
	virtual void 		ChangeSatiety				(float value);

	// хромание при потере сил и здоровья
	virtual	bool		IsLimping					() const;
	virtual bool		IsCantWalk					() const;
	virtual bool		IsCantWalkWeight			();
	virtual bool		IsCantSprint				() const;

			void		ConditionJump				(float weight);
			void		ConditionWalk				(float weight, bool accel, bool sprint);
			void		ConditionStand				(float weight);

			float	xr_stdcall	GetAlcohol			()	{return m_fAlcohol;}
			float	xr_stdcall	AlcoholSatiety      ()	{ return m_fAlcohol*(1.0f + m_fAlcoholSatietyIntens - GetSatiety()); }
			float	xr_stdcall	GetPsy				()	{return 1.0f-GetPsyHealth();}
			float				GetSatiety			()  {return m_fSatiety;}

public:
	IC		CActor		&object						() const
	{
		VERIFY			(m_object);
		return			(*m_object);
	}
	virtual void			save					(NET_Packet &output_packet);
	virtual void			load					(IReader &input_packet);

protected:
	float m_fAlcohol;
	float m_fV_Alcohol;
//--
	float m_fSatiety;
	float m_fV_Satiety;
	float m_fV_SatietyPower;
	float m_fV_SatietyHealth;
	float m_fSatietyCritical;
//--
	float m_fPowerLeakSpeed;

	float m_fJumpPower;
	float m_fStandPower;
	float m_fWalkPower;
	float m_fJumpWeightPower;
	float m_fWalkWeightPower;
	float m_fOverweightWalkK;
	float m_fOverweightJumpK;
	float m_fAccelK;
	float m_fSprintK;
public:
	//float m_MaxWalkWeight;
    //
	float m_fBleedingPowerDecrease;
	//
	float m_fMinPowerWalkJump;
    //
	float m_fMinHealthRadiation;
	float m_fMinHealthRadiationTreshold;
    //
	float m_fAlcoholSatietyIntens; //коэфф. для рассчета интенсивности постэффекта опьянения от голода
	//
	float m_fExerciseStressFactor; //фактор физнагрузки - множитель для коэффициента нагрузки актора при спринте и прыжке
	//
	float m_fZoomEffectorK;
	float m_fV_HardHoldPower;
protected:
	mutable bool m_bLimping;
	mutable bool m_bCantWalk;
	mutable bool m_bCantSprint;

	//порог силы и здоровья меньше которого актер начинает хромать
	float m_fLimpingPowerBegin;
	float m_fLimpingPowerEnd;
	float m_fCantWalkPowerBegin;
	float m_fCantWalkPowerEnd;

	float m_fCantSprintPowerBegin;
	float m_fCantSprintPowerEnd;

	float m_fLimpingHealthBegin;
	float m_fLimpingHealthEnd;
};
