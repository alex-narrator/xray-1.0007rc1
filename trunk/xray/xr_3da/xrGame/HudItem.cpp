//////////////////////////////////////////////////////////////////////
// HudItem.cpp: класс родитель для всех предметов имеющих
//				собственный HUD (CWeapon, CMissile etc)
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "HudItem.h"
#include "physic_item.h"
#include "WeaponHUD.h"
#include "actor.h"
#include "actoreffector.h"
#include "Missile.h"
#include "xrmessages.h"
#include "level.h"
#include "inventory.h"
#include "../CameraBase.h"

#include "Inventory.h"
#include "Weapon.h"


CHudItem::CHudItem(void)
{
	m_pHUD				= NULL;
	SetHUDmode			(FALSE);
	m_dwStateTime		= 0;
	m_bRenderHud		= true;

	m_bInertionEnable	= true;

	m_origin_offset		= 0.;
	m_tendto_speed		= 0.;
	m_origin_offset_aim	= 0.;
	m_tendto_speed_aim	= 0.;
}

CHudItem::~CHudItem(void)
{
	xr_delete			(m_pHUD);
}

DLL_Pure *CHudItem::_construct	()
{
	m_object			= smart_cast<CPhysicItem*>(this);
	VERIFY				(m_object);

	m_item				= smart_cast<CInventoryItem*>(this);
	VERIFY				(m_item);

	return				(m_object);
}
//
static const float ORIGIN_OFFSET		= -0.05f;
static const float TENDTO_SPEED			= 5.f;

static const float ORIGIN_OFFSET_AIM	= -0.01f;
static const float TENDTO_SPEED_AIM		= 5.f;
//
void CHudItem::Load(LPCSTR section)
{
	//загрузить hud, если он нужен
	if(pSettings->line_exist(section,"hud"))
		hud_sect		= pSettings->r_string		(section,"hud");

	if(*hud_sect){
		m_pHUD			= xr_new<CWeaponHUD> (this);
		m_pHUD->Load	(*hud_sect);
		//
		m_bInertionAllow		= !!READ_IF_EXISTS(pSettings, r_bool, hud_sect, "allow_inertion",		true);
		m_bInertionAllowAim		= !!READ_IF_EXISTS(pSettings, r_bool, hud_sect, "allow_inertion_aim",	true);
		//
		m_origin_offset		= READ_IF_EXISTS(pSettings, r_float, *hud_sect, "inertion_origin_offset",		ORIGIN_OFFSET);
		m_tendto_speed		= READ_IF_EXISTS(pSettings, r_float, *hud_sect, "inertion_tendto_speed",		TENDTO_SPEED);

		m_origin_offset_aim = READ_IF_EXISTS(pSettings, r_float, *hud_sect, "inertion_origin_offset_aim",	ORIGIN_OFFSET_AIM);
		m_tendto_speed_aim	= READ_IF_EXISTS(pSettings, r_float, *hud_sect, "inertion_tendto_speed_aim",	TENDTO_SPEED_AIM);
		//
		////////////////////////////////////////////
		//--#SM+# Begin--

		// Смещение в стрейфе
		m_strafe_offset[0][0] = READ_IF_EXISTS(pSettings, r_fvector3, *hud_sect, "strafe_hud_offset_pos", (Fvector{ 0.015f, 0.f, 0.f }));
		m_strafe_offset[1][0] = READ_IF_EXISTS(pSettings, r_fvector3, *hud_sect, "strafe_hud_offset_rot", (Fvector{ 0.f, 0.f, 4.5f }));

		m_strafe_offset[0][1] = READ_IF_EXISTS(pSettings, r_fvector3, *hud_sect, "strafe_aim_hud_offset_pos", (Fvector{ 0.005f, 0.f, 0.f }));
		m_strafe_offset[1][1] = READ_IF_EXISTS(pSettings, r_fvector3, *hud_sect, "strafe_aim_hud_offset_rot", (Fvector{ 0.f, 0.f, 2.5f }));

		m_strafe_offset[2][0].set(READ_IF_EXISTS(pSettings, r_bool, *hud_sect, "strafe_enabled", true), READ_IF_EXISTS(pSettings, r_float, *hud_sect, "strafe_transition_time", 0.25f), 0.f); // normal
		m_strafe_offset[2][1].set(READ_IF_EXISTS(pSettings, r_bool, *hud_sect, "strafe_aim_enabled", true), READ_IF_EXISTS(pSettings, r_float, *hud_sect, "strafe_aim_transition_time", 0.15f), 0.f); // aim-GL

		m_lookout_offset[0][0] = READ_IF_EXISTS(pSettings, r_fvector3, *hud_sect, "lookout_hud_offset_pos", (Fvector{ 0.045f, 0.f, 0.f }));
		m_lookout_offset[1][0] = READ_IF_EXISTS(pSettings, r_fvector3, *hud_sect, "lookout_hud_offset_rot", (Fvector{ 0.f, 0.f, 10.f }));

		m_lookout_offset[0][1] = READ_IF_EXISTS(pSettings, r_fvector3, *hud_sect, "lookout_aim_hud_offset_pos", (Fvector{ 0.f, 0.f, 0.f }));
		m_lookout_offset[1][1] = READ_IF_EXISTS(pSettings, r_fvector3, *hud_sect, "lookout_aim_hud_offset_rot", (Fvector{ 0.f, 0.f, 15.f }));

		m_lookout_offset[2][0].set(READ_IF_EXISTS(pSettings, r_bool, *hud_sect, "lookout_enabled", true), READ_IF_EXISTS(pSettings, r_float, *hud_sect, "lookout_transition_time", 0.25f), 0.f); // normal
		m_lookout_offset[2][1].set(READ_IF_EXISTS(pSettings, r_bool, *hud_sect, "lookout_aim_enabled", true), READ_IF_EXISTS(pSettings, r_float, *hud_sect, "lookout_aim_transition_time", 0.15f), 0.f); // aim-GL

		// Смещение при движении вперёд/назад
		m_longitudinal_offset[0] = READ_IF_EXISTS(pSettings, r_float, *hud_sect, "longitudinal_hud_offset", 0.04f);
		m_longitudinal_offset[1] = READ_IF_EXISTS(pSettings, r_float, *hud_sect, "longitudinal_aim_hud_offset", m_longitudinal_offset[0]);

		// Параметры движения вперёд/назад
		m_longitudinal_offset[2] = READ_IF_EXISTS(pSettings, r_float, *hud_sect, "longitudinal_transition_time", 0.18f);
		m_longitudinal_offset[3] = READ_IF_EXISTS(pSettings, r_float, *hud_sect, "longitudinal_aim_transition_time", m_longitudinal_offset[2]);
		m_longitudinal_offset[4] = READ_IF_EXISTS(pSettings, r_bool, *hud_sect, "longitudinal_enabled", true);
		m_longitudinal_offset[5] = READ_IF_EXISTS(pSettings, r_bool, *hud_sect, "longitudinal_aim_enabled", m_longitudinal_offset[4]);
		//--#SM+# End--
		////////////////////////////////////////////
	}else{
		m_pHUD = NULL;
		//если hud не задан, но задан слот, то ошибка
		R_ASSERT2(item().GetSlot() == NO_ACTIVE_SLOT, "active slot is set, but hud for food item is not available");
	}

	m_animation_slot	= pSettings->r_u32(section,"animation_slot");
}

void CHudItem::net_Destroy()
{
	if(m_pHUD)
		m_pHUD->net_DestroyHud	();

	SetHUDmode			(FALSE);
	m_dwStateTime		= 0;
}

void CHudItem::PlaySound	(HUD_SOUND& hud_snd, const Fvector& position, bool overlap)
{
	//HUD_SOUND::PlaySound	(hud_snd, position, object().H_Root(), !!GetHUDmode(), false, overlap);
	Fvector pos = GetHUDmode() ? Fvector().sub(position, ::Sound->listener_position()) : position;
	HUD_SOUND::PlaySound(hud_snd, pos, object().H_Root(), !!GetHUDmode(), false, overlap);
}

BOOL  CHudItem::net_Spawn	(CSE_Abstract* DC) 
{
	return TRUE;
}

void CHudItem::renderable_Render()
{
	UpdateXForm	();
	BOOL _hud_render			= ::Render->get_HUD() && GetHUDmode();
	if(_hud_render && !m_pHUD->IsHidden() && !item().IsHidden()){ 
		// HUD render
		if(m_bRenderHud){
			::Render->set_Transform		(&m_pHUD->Transform());
			::Render->add_Visual		(m_pHUD->Visual());
		}
	}
	else {
		if (!object().H_Parent() || (!_hud_render && m_pHUD && !m_pHUD->IsHidden() && !item().IsHidden()))
			on_renderable_Render		();
		else
			if (object().H_Parent()) {
				CInventoryOwner	*owner = smart_cast<CInventoryOwner*>(object().H_Parent());
				VERIFY			(owner);
				CInventoryItem	*self = smart_cast<CInventoryItem*>(this);
				if (owner->attached(self))
					on_renderable_Render();
			}
	}
}

bool CHudItem::Action(s32 cmd, u32 flags) 
{
	return false;
}

void CHudItem::SwitchState(u32 S)
{
	if (OnClient()) return;
	SetNextState( S );	// Very-very important line of code!!! :)

	if (object().Local() && !object().getDestroy())	
	{
		// !!! Just single entry for given state !!!
		NET_Packet		P;
		object().u_EventGen		(P,GE_WPN_STATE_CHANGE,object().ID());
		P.w_u8			(u8(S));
		object().u_EventSend		(P);
	}
}

void CHudItem::OnEvent		(NET_Packet& P, u16 type)
{
	switch (type)
	{
	case GE_WPN_STATE_CHANGE:
		{
			u8				S;
			P.r_u8			(S);
			OnStateSwitch	(u32(S));
		}
		break;
	}
}

void CHudItem::OnStateSwitch	(u32 S)
{
	m_dwStateTime = 0;
	SetState( S );
	if(object().Remote()) SetNextState( S );
}


bool CHudItem::Activate() 
{
	if(m_pHUD) 
		m_pHUD->Init();

	Show();
	OnActiveItem ();
	return true;
}

void CHudItem::Deactivate() 
{
	Hide();
	//OnHiddenItem (); //для плавного сокрытия оружия
}

// Получить индекс текущих координат худа
u8 CHudItem::GetCurrentHudOffsetIdx() const
{
	auto pActor = smart_cast<const CActor*>(object().H_Parent());
	if (!pActor)
		return 0;

	bool b_aiming = pActor->IsZoomAimingMode();

	if (!b_aiming)
		return 0;
	else
		return 1;
}

void CHudItem::UpdateHudPosition	()
{
	if (m_pHUD && GetHUDmode()){
		if(item().IsHidden()) 
			SetHUDmode(FALSE);

		Fmatrix							trans;

		CActor* pActor = smart_cast<CActor*>(object().H_Parent());
		if(pActor){
			pActor->Cameras().camera_Matrix				(trans);
			UpdateHudInertion							(trans);
			UpdateHudAdditonal							(trans);
			m_pHUD->UpdatePosition						(trans);
		}
	}
}

void CHudItem::UpdateHudAdditonal		(Fmatrix& hud_trans)
{
}

void CHudItem::StartHudInertion()
{
	m_bInertionEnable = true;
}
void CHudItem::StopHudInertion()
{
	m_bInertionEnable = false;
}

static const float PITCH_OFFSET_R	= 0.017f;
static const float PITCH_OFFSET_N	= 0.012f;
static const float PITCH_OFFSET_D	= 0.02f;
//static const float ORIGIN_OFFSET	= -0.05f;
//static const float TENDTO_SPEED	= 5.f;

void CHudItem::UpdateHudInertion		(Fmatrix& hud_trans)
{
	CActor* pActor = smart_cast<CActor*>(object().H_Parent());
	bool b_hard_holded = pActor->IsHardHold();

	if (m_pHUD && m_bInertionAllow && m_bInertionEnable)
	{
		Fmatrix								xform;//,xform_orig; 
		Fvector& origin						= hud_trans.c; 
		xform								= hud_trans;

		static Fvector						m_last_dir={0,0,0};

		// calc difference
		Fvector								diff_dir;
		diff_dir.sub						(xform.k, m_last_dir);

		// clamp by PI_DIV_2
		Fvector last;						last.normalize_safe(m_last_dir);
		float dot							= last.dotproduct(xform.k);
		if (dot<EPS){
			Fvector v0;
			v0.crossproduct			(m_last_dir,xform.k);
			m_last_dir.crossproduct	(xform.k,v0);
			diff_dir.sub			(xform.k, m_last_dir);
		}

		// tend to forward
		/*m_last_dir.mad	(diff_dir,TENDTO_SPEED*Device.fTimeDelta);
		origin.mad		(diff_dir,ORIGIN_OFFSET);

		// pitch compensation
		float pitch		= angle_normalize_signed(xform.k.getP());
		origin.mad		(xform.k,	-pitch * PITCH_OFFSET_D);
		origin.mad		(xform.i,	-pitch * PITCH_OFFSET_R);
		origin.mad		(xform.j,	-pitch * PITCH_OFFSET_N);*/

		// calc moving inertion
		//CActor* pActor = smart_cast<CActor*>(object().H_Parent());
		if (!pActor->IsZoomAimingMode())
		{
			// tend to forward
			m_last_dir.mad(diff_dir, m_tendto_speed*Device.fTimeDelta);
			origin.mad(diff_dir, m_origin_offset);

			// pitch compensation
			float pitch = angle_normalize_signed(xform.k.getP());
			origin.mad(xform.k, -pitch * PITCH_OFFSET_D);
			origin.mad(xform.i, -pitch * PITCH_OFFSET_R);
			origin.mad(xform.j, -pitch * PITCH_OFFSET_N);
		}
		else if (m_bInertionAllowAim && !b_hard_holded) // в режиме прицеливания
		{
			// tend to forward
			m_last_dir.mad(diff_dir, m_tendto_speed_aim*Device.fTimeDelta);
			origin.mad(diff_dir, m_origin_offset_aim);

			// что бы не ломал прицеливание - не будем сдвигать оружие
		}
	}

	/////////////////////////////

	auto* pWeapon = smart_cast<CWeapon*>(pActor->inventory().ActiveItem());
	float m_fZoomRotationFactor = pWeapon ? pWeapon->GetZoomRotationFactor() : 0.f;

	u8 idx = GetCurrentHudOffsetIdx();
	// Боковой стрейф с оружием
	clamp(idx, 0ui8, 1ui8);

	// Рассчитываем фактор боковой ходьбы
	float fStrafeMaxTime = m_strafe_offset[2][idx].y; // Макс. время в секундах, за которое мы наклонимся из центрального положения
	if (fStrafeMaxTime <= EPS)
		fStrafeMaxTime = 0.01f;

	float fLongitudinalMaxTime = m_longitudinal_offset[2 + idx];
	if (fLongitudinalMaxTime <= EPS)
		fLongitudinalMaxTime = 0.01f;

	float fStepPerUpd = Device.fTimeDelta / fStrafeMaxTime; // Величина изменение фактора поворота
	float fStepPerUpdLongitudinal = Device.fTimeDelta / fLongitudinalMaxTime; // ^ смещения при передвижении вперёд/назад

	u32 iMovingState = pActor->get_state();
	if ((iMovingState & mcLStrafe) != 0)
	{ // Движемся влево
		float fVal = (m_fLR_MovingFactor > 0.f ? fStepPerUpd * 3 : fStepPerUpd);
		m_fLR_MovingFactor -= fVal;
	}
	else if ((iMovingState & mcRStrafe) != 0)
	{ // Движемся вправо
		float fVal = (m_fLR_MovingFactor < 0.f ? fStepPerUpd * 3 : fStepPerUpd);
		m_fLR_MovingFactor += fVal;
	}
	else
	{ // Двигаемся в любом другом направлении
		if (m_fLR_MovingFactor < 0.0f)
		{
			m_fLR_MovingFactor += fStepPerUpd;
			clamp(m_fLR_MovingFactor, -1.0f, 0.0f);
		}
		else
		{
			m_fLR_MovingFactor -= fStepPerUpd;
			clamp(m_fLR_MovingFactor, 0.0f, 1.0f);
		}
	}

	if ((iMovingState & mcBack) != 0)
	{
		// Движемся назад
		float fVal = m_fFB_MovingFactor < 0.0f ? fStepPerUpdLongitudinal * 3 : fStepPerUpdLongitudinal;
		m_fFB_MovingFactor += fVal;
	}
	else if ((iMovingState & mcFwd) != 0)
	{
		// Движемся вперёд
		float fVal = m_fFB_MovingFactor > 0.0f ? fStepPerUpdLongitudinal * 3 : fStepPerUpdLongitudinal;
		m_fFB_MovingFactor -= fVal;
	}
	else
	{
		if (m_fFB_MovingFactor < 0.0f)
		{
			m_fFB_MovingFactor += fStepPerUpdLongitudinal;
			clamp(m_fFB_MovingFactor, -1.0f, 0.0f);
		}
		else
		{
			m_fFB_MovingFactor -= fStepPerUpdLongitudinal;
			clamp(m_fFB_MovingFactor, 0.0f, 1.0f);
		}
	}

	clamp(m_fLR_MovingFactor, -1.0f, 1.0f); // Фактор боковой ходьбы не должен превышать эти лимиты
	clamp(m_fFB_MovingFactor, -1.0f, 1.0f); // ^ *вперёд/назад*

	// Производим наклон ствола для нормального режима и аима
	for (int _idx = 0; _idx <= 1; _idx++)
	{
		bool bEnabled = !!m_strafe_offset[2][_idx].x && !b_hard_holded;
		if (!bEnabled)
			continue;

		Fvector curr_offs, curr_rot;

		// Смещение позиции худа в стрейфе
		curr_offs = m_strafe_offset[0][_idx]; //pos
		curr_offs.mul(m_fLR_MovingFactor);                   // Умножаем на фактор стрейфа

		// Поворот худа в стрейфе
		curr_rot = m_strafe_offset[1][_idx]; //rot
		curr_rot.mul(-PI / 180.f);                          // Преобразуем углы в радианы
		curr_rot.mul(m_fLR_MovingFactor);                   // Умножаем на фактор стрейфа

		if (_idx == 0)
		{ // От бедра
			curr_offs.mul(1.f - m_fZoomRotationFactor);
			curr_rot.mul(1.f - m_fZoomRotationFactor);
		}
		else
		{ // Во время аима
			curr_offs.mul(m_fZoomRotationFactor);
			curr_rot.mul(m_fZoomRotationFactor);
		}

		Fmatrix hud_rotation;
		Fmatrix hud_rotation_y;

		hud_rotation.identity();
		hud_rotation.rotateX(curr_rot.x);

		hud_rotation_y.identity();
		hud_rotation_y.rotateY(curr_rot.y);
		hud_rotation.mulA_43(hud_rotation_y);

		hud_rotation_y.identity();
		hud_rotation_y.rotateZ(curr_rot.z);
		hud_rotation.mulA_43(hud_rotation_y);

		hud_rotation.translate_over(curr_offs);
		hud_trans.mulB_43(hud_rotation);
	}

	bool bEnabled = !!m_longitudinal_offset[4 + idx] && !b_hard_holded;
	if (bEnabled)
	{ // Смещение худа при движении вперёд/назад
		float curr_offs;
		curr_offs = m_longitudinal_offset[idx];
		curr_offs *= m_fFB_MovingFactor;

		Fmatrix hud_position;

		hud_position.translate(0.0f, 0.0f, curr_offs);
		hud_trans.mulB_43(hud_position);
	}

	//=============== Эффекты выглядываний ===============//
	for (int _idx = 0; _idx <= 1; _idx++)
	{
		bool bEnabled = !!m_lookout_offset[2][_idx].x;
		if (!bEnabled)
			continue;

		float fLookoutMaxTime = m_lookout_offset[2][idx].y; // Макс. время в секундах, за которое мы наклонимся из центрального положения
		if (fLookoutMaxTime <= EPS)
			fLookoutMaxTime = 0.01f;

		const float fStepPerUpdL = Device.fTimeDelta / fLookoutMaxTime; // Величина изменение фактора поворота

		if ((iMovingState & mcLLookout) && !(iMovingState & mcRLookout))
		{ // Выглядываем влево
			float fVal = (m_fLookout_MovingFactor > 0.f ? fStepPerUpdL * 3 : fStepPerUpdL);
			m_fLookout_MovingFactor -= fVal;
		}
		else if ((iMovingState & mcRLookout) && !(iMovingState & mcLLookout))
		{ // Выглядываем вправо
			float fVal = (m_fLookout_MovingFactor < 0.f ? fStepPerUpdL * 3 : fStepPerUpdL);
			m_fLookout_MovingFactor += fVal;
		}
		else
		{ // Двигаемся в любом другом направлении
			if (m_fLookout_MovingFactor < 0.0f)
			{
				m_fLookout_MovingFactor += fStepPerUpdL;
				clamp(m_fLookout_MovingFactor, -1.0f, 0.0f);
			}
			else
			{
				m_fLookout_MovingFactor -= fStepPerUpdL;
				clamp(m_fLookout_MovingFactor, 0.0f, 1.0f);
			}
		}

		clamp(m_fLookout_MovingFactor, -1.0f, 1.0f); // не должен превышать эти лимиты

		float koef{ 1.f };
		if ((iMovingState & mcCrouch) && (iMovingState & mcAccel))
			koef = 0.5; // во сколько раз менять амплитуду при полном присяде
		else if (iMovingState & mcCrouch)
			koef = 0.75; // во сколько раз менять амплитуду при присяде

		// Смещение позиции худа
		Fvector lookout_offs = m_lookout_offset[0][idx]; //pos
		lookout_offs.mul(koef);
		lookout_offs.mul(m_fLookout_MovingFactor); // Умножаем на фактор наклона

		// Поворот худа
		Fvector lookout_rot = m_lookout_offset[1][idx]; //rot
		lookout_rot.mul(koef);
		lookout_rot.mul(-PI / 180.f); // Преобразуем углы в радианы
		lookout_rot.mul(m_fLookout_MovingFactor); // Умножаем на фактор наклона

		if (idx == 0)
		{ // От бедра
			lookout_offs.mul(1.f - m_fZoomRotationFactor);
			lookout_rot.mul(1.f - m_fZoomRotationFactor);
		}
		else
		{ // Во время аима
			lookout_offs.mul(m_fZoomRotationFactor);
			lookout_rot.mul(m_fZoomRotationFactor);
		}

		Fmatrix hud_rotation;
		Fmatrix hud_rotation_y;

		hud_rotation.identity();
		hud_rotation.rotateX(lookout_rot.x);

		hud_rotation_y.identity();
		hud_rotation_y.rotateY(lookout_rot.y);
		hud_rotation.mulA_43(hud_rotation_y);

		hud_rotation_y.identity();
		hud_rotation_y.rotateZ(lookout_rot.z);
		hud_rotation.mulA_43(hud_rotation_y);

		hud_rotation.translate_over(lookout_offs);
		hud_trans.mulB_43(hud_rotation);
		
	}
	//====================================================//
}

void CHudItem::UpdateCL()
{
	m_dwStateTime += Device.dwTimeDelta;

	if(m_pHUD) m_pHUD->Update();
	UpdateHudPosition	();

	CActor* pActor = smart_cast<CActor*>(object().H_Parent());
	
	if (pActor && m_pHUD && !m_pHUD->IsHidden())
		pActor->TryToBlockSprint(IsPending());
}

void CHudItem::OnH_A_Chield		()
{
	SetHUDmode		(FALSE);

	if (m_pHUD) {
		if(Level().CurrentEntity() == object().H_Parent() && smart_cast<CActor*>(object().H_Parent()))
			m_pHUD->Visible(true);
		else
			m_pHUD->Visible(false);
	}
}

void CHudItem::OnH_B_Chield		()
{
	OnHiddenItem ();
}

void CHudItem::OnH_B_Independent	(bool just_before_destroy)
{
	SetHUDmode				(FALSE);

	if (m_pHUD)
		m_pHUD->Visible		(false);
	
	StopHUDSounds			();

	UpdateXForm				();
}

void CHudItem::OnH_A_Independent	()
{
}

void CHudItem::animGet(MotionSVec& lst, LPCSTR prefix) 
{
	const MotionID &M = m_pHUD->animGet(prefix);
	if (M)
		lst.push_back(MotionIDEx(M));
	for (int i = 0; i < MAX_ANIM_COUNT; ++i) {
		string128 sh_anim;
		sprintf_s(sh_anim, "%s%d", prefix, i);
		const MotionID &M = m_pHUD->animGet(sh_anim);
		if (M)
			lst.push_back(MotionIDEx(M));
	}
	ASSERT_FMT(!lst.empty(), "Can't find [anim_%s] in hud section [%s]", prefix, this->hud_sect.c_str());
}

void CHudItem::animGetEx(MotionSVec& lst, LPCSTR prefix, LPCSTR suffix, LPCSTR prefix2) 
{
	std::string anim_name;
	if (prefix2) {
		if (pSettings->line_exist(hud_sect.c_str(), prefix))
			anim_name = pSettings->r_string(hud_sect.c_str(), prefix);
		else
			anim_name = pSettings->r_string(hud_sect.c_str(), prefix2);
	}
	else
		anim_name = pSettings->r_string(hud_sect.c_str(), prefix);
	if (suffix)
		anim_name += suffix;
	animGet(lst, anim_name.c_str());
	std::string speed_k = prefix;
	speed_k += "_speed_k";
	if (pSettings->line_exist(hud_sect.c_str(), speed_k.c_str())) {
		float k = pSettings->r_float(hud_sect.c_str(), speed_k.c_str());
		if (!fsimilar(k, 1.f)) {
			for (const auto& M : lst) {
				auto *animated = m_pHUD->Visual()->dcast_PKinematicsAnimated();
				auto *motion_def = animated->LL_GetMotionDef(M.m_MotionID);
				motion_def->SetSpeedKoeff(k);
			}
		}
	}
	std::string stop_k = prefix;
	stop_k += "_stop_k";
	if (pSettings->line_exist(hud_sect.c_str(), stop_k.c_str())) {
		float k = pSettings->r_float(hud_sect.c_str(), stop_k.c_str());
		if (k < 1.f)
			for (auto& M : lst)
				M.stop_k = k;
	}
}