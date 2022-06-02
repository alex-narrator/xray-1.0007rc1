// Weapon.cpp: implementation of the CWeapon class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "Weapon.h"
#include "ParticlesObject.h"
#include "HUDManager.h"
#include "WeaponHUD.h"
#include "entity_alive.h"
#include "inventory_item_impl.h"

#include "inventory.h"
#include "xrserver_objects_alife_items.h"
#include "Torch.h"

#include "actor.h"
#include "actoreffector.h"
#include "ActorCondition.h"
#include "level.h"

#include "xr_level_controller.h"
#include "game_cl_base.h"
#include "../skeletoncustom.h"
#include "ai_object_location.h"
#include "clsid_game.h"
#include "mathutils.h"
#include "object_broker.h"
#include "../igame_persistent.h"

#include "UIGameCustom.h"
#include "string_table.h"

// Headers included by Cribbledirge (for callbacks).
#include "pch_script.h"
#include "script_callback_ex.h"
#include "script_game_object.h"
#include "game_object_space.h"

#include "../../build_config_defines.h"

#define WEAPON_REMOVE_TIME		60000
#define ROTATION_TIME			0.25f

LPCSTR wpn_scope			= "wpn_scope";
LPCSTR wpn_silencer			= "wpn_silencer";
LPCSTR wpn_grenade_launcher = "wpn_grenade_launcher";
LPCSTR wpn_launcher			= "wpn_launcher";

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CWeapon::CWeapon(LPCSTR name)
{
	SetState(eHidden);
	SetNextState(eHidden);
	m_sub_state = eSubstateReloadBegin;
	m_idle_state = eIdle;
	m_bTriStateReload = false;
	SetDefaults();

	m_Offset.identity();
	m_StrapOffset.identity();

	iAmmoCurrent = -1;
	m_dwAmmoCurrentCalcFrame = 0;

	iAmmoElapsed = -1;
	iMagazineSize = -1;
	m_ammoType = 0;
	m_ammoName = NULL;

	//eHandDependence = hdNone;

	m_fZoomFactor			= 1.f;// g_fov;
	m_fZoomRotationFactor	= 0.f;
	//
	m_bScopeDynamicZoom		= false;
	m_fScopeZoomFactor		= 1.f;
	m_fMinScopeZoomFactor	= 1.f;
	m_uZoomStepCount		= 3;
	m_fRTZoomFactor			= 1.f;
	//

	m_pAmmo = NULL;

	m_pFlameParticles2 = NULL;
	m_sFlameParticles2 = NULL;

	m_fCurrentCartirdgeDisp = 1.f;
	m_strap_bone0 = 0;
	m_strap_bone1 = 0;
	m_StrapOffset.identity();
	m_strapped_mode = false;
	m_can_be_strapped = false;
	m_ef_main_weapon_type = u32(-1);
	m_ef_weapon_type = u32(-1);
	m_UIScope = NULL;
	m_set_next_ammoType_on_reload = u32(-1);
	m_bZoomingIn = false;
	m_class_name = get_class_name<CWeapon>(this);
	need_slot = true;
	//
	conditionDecreasePerShotSilencerKoef	= 1.f;
	conditionDecreasePerShotSilencer		= 0.f;
	//
	m_cur_scope		= 0;
	m_cur_silencer	= 0;
	m_cur_glauncher = 0;
}

CWeapon::~CWeapon()
{
	xr_delete(m_UIScope);
	delete_data(m_scopes);
	delete_data(m_silencers);
	delete_data(m_glaunchers);
}

//void CWeapon::Hit(float P, Fvector &dir,
//		    CObject* who, s16 element,
//		    Fvector position_in_object_space,
//		    float impulse,
//		    ALife::EHitType hit_type)
void CWeapon::Hit(SHit* pHDS)
{
	//	inherited::Hit(P, dir, who, element, position_in_object_space,impulse,hit_type);
	inherited::Hit(pHDS);
}

void CWeapon::UpdateXForm()
{
	if (Device.dwFrame != dwXF_Frame)
	{
		dwXF_Frame = Device.dwFrame;

		if (0 == H_Parent())	return;

		// Get access to entity and its visual
		CEntityAlive*	E = smart_cast<CEntityAlive*>(H_Parent());

		if (!E)
		{
			if (!IsGameTypeSingle())
				UpdatePosition(H_Parent()->XFORM());
			return;
		}

		const CInventoryOwner	*parent = smart_cast<const CInventoryOwner*>(E);
		if (parent && parent->use_simplified_visual())
			return;

		if (parent->attached(this))
			return;

		R_ASSERT(E);
		CKinematics*	V = smart_cast<CKinematics*>	(E->Visual());
		VERIFY(V);

		// Get matrices
		int				boneL, boneR, boneR2;
		E->g_WeaponBones(boneL, boneR, boneR2);
		if ((HandDependence() == hd1Hand) || (GetState() == eReload) || (!E->g_Alive()))
			boneL = boneR2;
#pragma todo("TO ALL: serious performance problem")
		V->CalculateBones();
		Fmatrix& mL = V->LL_GetTransform(u16(boneL));
		Fmatrix& mR = V->LL_GetTransform(u16(boneR));
		// Calculate
		Fmatrix				mRes;
		Fvector				R, D, N;
		D.sub(mL.c, mR.c);

		if (fis_zero(D.magnitude()))
		{
			mRes.set(E->XFORM());
			mRes.c.set(mR.c);
		}
		else
		{
			D.normalize();
			R.crossproduct(mR.j, D);

			N.crossproduct(D, R);
			N.normalize();

			mRes.set(R, N, D, mR.c);
			mRes.mulA_43(E->XFORM());
		}

		UpdatePosition(mRes);
	}
}

void CWeapon::UpdateFireDependencies_internal()
{
	if (Device.dwFrame != dwFP_Frame)
	{
		dwFP_Frame = Device.dwFrame;

		UpdateXForm();

		if (GetHUDmode() && (0 != H_Parent()))
		{
			// 1st person view - skeletoned
			CKinematics* V = smart_cast<CKinematics*>(m_pHUD->Visual());
			VERIFY(V);
			V->CalculateBones();

			// fire point&direction
			Fmatrix& fire_mat = V->LL_GetTransform(u16(m_pHUD->FireBone()));
			Fmatrix& parent = m_pHUD->Transform();

			const Fvector& fp = m_pHUD->FirePoint();
			const Fvector& fp2 = m_pHUD->FirePoint2();
			const Fvector& sp = m_pHUD->ShellPoint();

			fire_mat.transform_tiny(m_firedeps.vLastFP, fp);
			parent.transform_tiny(m_firedeps.vLastFP);
			fire_mat.transform_tiny(m_firedeps.vLastFP2, fp2);
			parent.transform_tiny(m_firedeps.vLastFP2);

			fire_mat.transform_tiny(m_firedeps.vLastSP, sp);
			parent.transform_tiny(m_firedeps.vLastSP);

			m_firedeps.vLastFD.set(0.f, 0.f, 1.f);
			parent.transform_dir(m_firedeps.vLastFD);

			m_firedeps.m_FireParticlesXForm.identity();
			m_firedeps.m_FireParticlesXForm.k.set(m_firedeps.vLastFD);
			Fvector::generate_orthonormal_basis_normalized(m_firedeps.m_FireParticlesXForm.k,
				m_firedeps.m_FireParticlesXForm.j, m_firedeps.m_FireParticlesXForm.i);
		}
		else {
			// 3rd person or no parent
			Fmatrix& parent = XFORM();
			Fvector& fp = vLoadedFirePoint;
			Fvector& fp2 = vLoadedFirePoint2;
			Fvector& sp = vLoadedShellPoint;

			parent.transform_tiny(m_firedeps.vLastFP, fp);
			parent.transform_tiny(m_firedeps.vLastFP2, fp2);
			parent.transform_tiny(m_firedeps.vLastSP, sp);

			m_firedeps.vLastFD.set(0.f, 0.f, 1.f);
			parent.transform_dir(m_firedeps.vLastFD);

			m_firedeps.m_FireParticlesXForm.set(parent);
		}
	}
}

void CWeapon::ForceUpdateFireParticles()
{
	if (!GetHUDmode())
	{//update particlesXFORM real bullet direction
		if (!H_Parent())		return;

		CInventoryOwner* io = smart_cast<CInventoryOwner*>(H_Parent());
		if (NULL == io->inventory().ActiveItem())
		{
			Log("current_state", GetState());
			Log("next_state", GetNextState());
			Log("state_time", m_dwStateTime);
			Log("item_sect", cNameSect().c_str());
			Log("H_Parent", H_Parent()->cNameSect().c_str());
		}

		Fvector					p, d;
		smart_cast<CEntity*>(H_Parent())->g_fireParams(this, p, d);

		Fmatrix						_pxf;
		_pxf.k = d;
		_pxf.i.crossproduct(Fvector().set(0.0f, 1.0f, 0.0f), _pxf.k);
		_pxf.j.crossproduct(_pxf.k, _pxf.i);
		_pxf.c = XFORM().c;

		m_firedeps.m_FireParticlesXForm.set(_pxf);
	}
}

void CWeapon::Load(LPCSTR section)
{
	inherited::Load(section);
	CShootingObject::Load(section);

	if (pSettings->line_exist(section, "flame_particles_2"))
		m_sFlameParticles2 = pSettings->r_string(section, "flame_particles_2");

#ifdef DEBUG
	{
		Fvector				pos, ypr;
		pos = pSettings->r_fvector3(section, "position");
		ypr = pSettings->r_fvector3(section, "orientation");
		ypr.mul(PI / 180.f);

		m_Offset.setHPB(ypr.x, ypr.y, ypr.z);
		m_Offset.translate_over(pos);
	}

	m_StrapOffset = m_Offset;
	if (pSettings->line_exist(section, "strap_position") && pSettings->line_exist(section, "strap_orientation")) {
		Fvector				pos, ypr;
		pos = pSettings->r_fvector3(section, "strap_position");
		ypr = pSettings->r_fvector3(section, "strap_orientation");
		ypr.mul(PI / 180.f);

		m_StrapOffset.setHPB(ypr.x, ypr.y, ypr.z);
		m_StrapOffset.translate_over(pos);
	}
#endif

	// load ammo classes
	m_ammoTypes.clear();
	LPCSTR				S = pSettings->r_string(section, "ammo_class");
	if (S && S[0])
	{
		string128		_ammoItem;
		int				count = _GetItemCount(S);
		for (int it = 0; it < count; ++it)
		{
			_GetItem(S, it, _ammoItem);
			m_ammoTypes.push_back(_ammoItem);
		}
		m_ammoName = pSettings->r_string(*m_ammoTypes[0], "inv_name_short");
	}
	else
		m_ammoName = 0;

	iAmmoElapsed = pSettings->r_s32(section, "ammo_elapsed");
	iMagazineSize = pSettings->r_s32(section, "ammo_mag_size");

	////////////////////////////////////////////////////
	// дисперсия стрельбы

	//подбрасывание камеры во время отдачи
	camMaxAngle = pSettings->r_float(section, "cam_max_angle");
	camMaxAngle = deg2rad(camMaxAngle);
	camRelaxSpeed = pSettings->r_float(section, "cam_relax_speed");
	camRelaxSpeed = deg2rad(camRelaxSpeed);
	if (pSettings->line_exist(section, "cam_relax_speed_ai"))
	{
		camRelaxSpeed_AI = pSettings->r_float(section, "cam_relax_speed_ai");
		camRelaxSpeed_AI = deg2rad(camRelaxSpeed_AI);
	}
	else
	{
		camRelaxSpeed_AI = camRelaxSpeed;
	}

	//	camDispersion		= pSettings->r_float		(section,"cam_dispersion"	);
	//	camDispersion		= deg2rad					(camDispersion);

	camMaxAngleHorz = pSettings->r_float(section, "cam_max_angle_horz");
	camMaxAngleHorz = deg2rad(camMaxAngleHorz);
	camStepAngleHorz = pSettings->r_float(section, "cam_step_angle_horz");
	camStepAngleHorz = deg2rad(camStepAngleHorz);
	camDispertionFrac = READ_IF_EXISTS(pSettings, r_float, section, "cam_dispertion_frac", 0.7f);
	//  [8/2/2005]
	//m_fParentDispersionModifier = READ_IF_EXISTS(pSettings, r_float, section, "parent_dispersion_modifier",1.0f);
	m_fPDM_disp_base = READ_IF_EXISTS(pSettings, r_float, section, "PDM_disp_base", 1.0f);
	m_fPDM_disp_vel_factor = READ_IF_EXISTS(pSettings, r_float, section, "PDM_disp_vel_factor", 1.0f);
	m_fPDM_disp_accel_factor = READ_IF_EXISTS(pSettings, r_float, section, "PDM_disp_accel_factor", 1.0f);
	m_fPDM_disp_crouch = READ_IF_EXISTS(pSettings, r_float, section, "PDM_disp_crouch", 1.0f);
	m_fPDM_disp_crouch_no_acc = READ_IF_EXISTS(pSettings, r_float, section, "PDM_disp_crouch_no_acc", 1.0f);
	//
	camRecoilCompensation = !!READ_IF_EXISTS(pSettings, r_bool, section, "cam_recoil_compensation", false);
	//  [8/2/2005]

	fireDispersionConditionFactor = pSettings->r_float(section, "fire_dispersion_condition_factor");
	misfireProbability = pSettings->r_float(section, "misfire_probability");
	misfireConditionK = READ_IF_EXISTS(pSettings, r_float, section, "misfire_condition_k", 1.0f);
	conditionDecreasePerShot = pSettings->r_float(section, "condition_shot_dec");
	//
	conditionDecreasePerShotOnHit = READ_IF_EXISTS(pSettings, r_float, section, "condition_shot_dec_on_hit", 0.f);

	vLoadedFirePoint = pSettings->r_fvector3(section, "fire_point");

	if (pSettings->line_exist(section, "fire_point2"))
		vLoadedFirePoint2 = pSettings->r_fvector3(section, "fire_point2");
	else
		vLoadedFirePoint2 = vLoadedFirePoint;

	// hands
	/*eHandDependence = EHandDependence(pSettings->r_s32(section, "hand_dependence"));
	m_bIsSingleHanded = true;
	if (pSettings->line_exist(section, "single_handed"))
		m_bIsSingleHanded = !!pSettings->r_bool(section, "single_handed");*/
	//
	m_fMinRadius = pSettings->r_float(section, "min_radius");
	m_fMaxRadius = pSettings->r_float(section, "max_radius");

	// информация о возможных апгрейдах и их визуализации в инвентаре
	m_eScopeStatus = (ALife::EWeaponAddonStatus)pSettings->r_s32(section, "scope_status");
	m_eSilencerStatus = (ALife::EWeaponAddonStatus)pSettings->r_s32(section, "silencer_status");
	m_eGrenadeLauncherStatus = (ALife::EWeaponAddonStatus)pSettings->r_s32(section, "grenade_launcher_status");

	m_bZoomEnabled = !!pSettings->r_bool(section, "zoom_enabled");
	m_fZoomRotateTime = ROTATION_TIME;

	if (m_bZoomEnabled && m_pHUD) LoadZoomOffset(*hud_sect, "");

	if (m_eScopeStatus == ALife::eAddonAttachable)
	{
		if (pSettings->line_exist(section, "scope_name"))
		{
			LPCSTR str = pSettings->r_string(section, "scope_name");
			for (int i = 0, count = _GetItemCount(str); i < count; ++i)
			{
				string128 scope_section;
				_GetItem(str, i, scope_section);
				m_scopes.push_back(scope_section);
			}
		}
	}

	if (m_eSilencerStatus == ALife::eAddonAttachable)
	{
		if (pSettings->line_exist(section, "silencer_name"))
		{
			LPCSTR str = pSettings->r_string(section, "silencer_name");
			for (int i = 0, count = _GetItemCount(str); i < count; ++i)
			{
				string128 silencer_section;
				_GetItem(str, i, silencer_section);
				m_silencers.push_back(silencer_section);
			}
		}
	}

	if (m_eGrenadeLauncherStatus == ALife::eAddonAttachable)
	{
		if (pSettings->line_exist(section, "grenade_launcher_name"))
		{
			LPCSTR str = pSettings->r_string(section, "grenade_launcher_name");
			for (int i = 0, count = _GetItemCount(str); i < count; ++i)
			{
				string128 glauncher_section;
				_GetItem(str, i, glauncher_section);
				m_glaunchers.push_back(glauncher_section);
			}
		}
	}

	InitAddons();

	//////////////////////////////////////
	//время убирания оружия с уровня
	if (pSettings->line_exist(section, "weapon_remove_time"))
		m_dwWeaponRemoveTime = pSettings->r_u32(section, "weapon_remove_time");
	else
		m_dwWeaponRemoveTime = WEAPON_REMOVE_TIME;
	//////////////////////////////////////
	if (pSettings->line_exist(section, "auto_spawn_ammo"))
		m_bAutoSpawnAmmo = pSettings->r_bool(section, "auto_spawn_ammo");
	else
		m_bAutoSpawnAmmo = TRUE;
	//////////////////////////////////////

	m_bHideCrosshairInZoom = true;
	if (pSettings->line_exist(hud_sect, "zoom_hide_crosshair"))
		m_bHideCrosshairInZoom = !!pSettings->r_bool(hud_sect, "zoom_hide_crosshair");

	m_bAimReloading = !!READ_IF_EXISTS(pSettings, r_bool, section, "allow_aim_reloading", false);

	//////////////////////////////////////////////////////////

	m_bHasTracers = READ_IF_EXISTS(pSettings, r_bool, section, "tracers", true);
	m_u8TracerColorID = READ_IF_EXISTS(pSettings, r_u8, section, "tracers_color_ID", u8(-1));

	string256						temp;
	for (int i = egdNovice; i < egdCount; ++i) {
		strconcat(sizeof(temp), temp, "hit_probability_", get_token_name(difficulty_type_token, i));
		m_hit_probability[i] = READ_IF_EXISTS(pSettings, r_float, section, temp, 1.f);
	}
}

void CWeapon::LoadFireParams(LPCSTR section, LPCSTR prefix)
{
	camDispersion = pSettings->r_float(section, "cam_dispersion");
	camDispersion = deg2rad(camDispersion);

	if (pSettings->line_exist(section, "cam_dispersion_inc"))
	{
		camDispersionInc = pSettings->r_float(section, "cam_dispersion_inc");
		camDispersionInc = deg2rad(camDispersionInc);
	}
	else
		camDispersionInc = 0;

	CShootingObject::LoadFireParams(section, prefix);
};

void CWeapon::LoadZoomOffset(LPCSTR section, LPCSTR prefix)
{
	string256 full_name;
	m_pHUD->SetZoomOffset(pSettings->r_fvector3(hud_sect, strconcat(sizeof(full_name), full_name, prefix, "zoom_offset")));
	m_pHUD->SetZoomRotateX(pSettings->r_float(hud_sect, strconcat(sizeof(full_name), full_name, prefix, "zoom_rotate_x")));
	m_pHUD->SetZoomRotateY(pSettings->r_float(hud_sect, strconcat(sizeof(full_name), full_name, prefix, "zoom_rotate_y")));

	if (pSettings->line_exist(hud_sect, "zoom_rotate_time"))
		m_fZoomRotateTime = pSettings->r_float(hud_sect, "zoom_rotate_time");
}
/*
void CWeapon::animGet	(MotionSVec& lst, LPCSTR prefix)
{
const MotionID		&M = m_pHUD->animGet(prefix);
if (M)				lst.push_back(M);
for (int i=0; i<MAX_ANIM_COUNT; ++i)
{
string128		sh_anim;
sprintf_s			(sh_anim,"%s%d",prefix,i);
const MotionID	&M = m_pHUD->animGet(sh_anim);
if (M)			lst.push_back(M);
}
R_ASSERT2			(!lst.empty(),prefix);
}
*/

//
void CWeapon::OnBulletHit() 
{
	if (!fis_zero(conditionDecreasePerShotOnHit))
		ChangeCondition(-conditionDecreasePerShotOnHit);
}

bool CWeapon::IsPartlyReloading() 
{
	return (GetAmmoElapsed() > 0 /*&& m_set_next_ammoType_on_reload == u32(-1) && !IsMisfire()*/);
}
//

BOOL CWeapon::net_Spawn(CSE_Abstract* DC)
{
	BOOL bResult = inherited::net_Spawn(DC);
	CSE_Abstract					*e = (CSE_Abstract*)(DC);
	CSE_ALifeItemWeapon			    *E = smart_cast<CSE_ALifeItemWeapon*>(e);

	//iAmmoCurrent					= E->a_current;
	iAmmoElapsed = E->a_elapsed;
	m_flagsAddOnState = E->m_addon_flags.get();
	if (E->ammo_type < m_ammoTypes.size())
		m_ammoType = E->ammo_type;
	SetState(E->wpn_state);
	SetNextState(E->wpn_state);
	//
	bMisfire = E->bMisfire;
	//
	m_cur_scope		= E->m_cur_scope;
	m_cur_silencer	= E->m_cur_silencer;
	m_cur_glauncher = E->m_cur_glauncher;
	//Msg("CWeapon::net_Spawn: weapon [%s] with m_ammoType [%d], ammo [%s]", Name_script(), m_ammoType, *m_ammoTypes[m_ammoType]);
	m_DefaultCartridge.Load(*m_ammoTypes[m_ammoType], u8(m_ammoType));
	if (iAmmoElapsed)
	{
		m_fCurrentCartirdgeDisp = m_DefaultCartridge.m_kDisp;
		for (int i = 0; i < iAmmoElapsed; ++i)
			m_magazine.push_back(m_DefaultCartridge);
	}

	UpdateAddonsVisibility();
	InitAddons();

	m_dwWeaponIndependencyTime = 0;

	VERIFY((u32)iAmmoElapsed == m_magazine.size());
	m_bAmmoWasSpawned = false;

	return bResult;
}

void CWeapon::net_Destroy()
{
	inherited::net_Destroy();

	//удалить объекты партиклов
	StopFlameParticles();
	StopFlameParticles2();
	StopLight();
	Light_Destroy();

	//while (m_magazine.size()) m_magazine.pop_back();
	m_magazine.clear();
	m_magazine.shrink_to_fit();
}

BOOL CWeapon::IsUpdating()
{
	bool bIsActiveItem = m_pCurrentInventory && m_pCurrentInventory->ActiveItem() == this;
	return bIsActiveItem || bWorking || m_bPending || getVisible();
}

void CWeapon::net_Export(NET_Packet& P)
{
	inherited::net_Export(P);

//	P.w_float_q8(m_fCondition, 0.0f, 1.0f);

	u8 need_upd = IsUpdating() ? 1 : 0;
	P.w_u8(need_upd);
	P.w_u16(u16(iAmmoElapsed));
	P.w_u8(m_flagsAddOnState);
	P.w_u8((u8)m_ammoType);
	P.w_u8((u8)GetState());
	P.w_u8((u8)m_bZoomMode);
	//
	P.w_u8((u8)bMisfire);
	//
	P.w_u8((u8)m_cur_scope);
	P.w_u8((u8)m_cur_silencer);
	P.w_u8((u8)m_cur_glauncher);
}

void CWeapon::net_Import(NET_Packet& P)
{
	inherited::net_Import(P);

//	P.r_float_q8(m_fCondition, 0.0f, 1.0f);

	u8 flags = 0;
	P.r_u8(flags);

	u16 ammo_elapsed = 0;
	P.r_u16(ammo_elapsed);

	u8						NewAddonState;
	P.r_u8(NewAddonState);

	m_flagsAddOnState = NewAddonState;
	UpdateAddonsVisibility();

	u8 ammoType, wstate;
	P.r_u8(ammoType);
	P.r_u8(wstate);

	u8 Zoom;
	P.r_u8((u8)Zoom);
	//
	bMisfire = !!(P.r_u8() & 0x1);
	//
	m_cur_scope		= P.r_u8();
	m_cur_silencer	= P.r_u8();
	m_cur_glauncher = P.r_u8();

	if (H_Parent() && H_Parent()->Remote())
	{
		if (Zoom) OnZoomIn();
		else OnZoomOut();
	};
	switch (wstate)
	{
	case eFire:
	case eFire2:
	case eSwitch:
	case eReload:
	{
	}break;
	default:
	{
		if (ammoType >= m_ammoTypes.size())
			Msg("!! Weapon [%d], State - [%d]", ID(), wstate);
		else
		{
			m_ammoType = ammoType;
			SetAmmoElapsed((ammo_elapsed));
		}
	}break;
	}

	VERIFY((u32)iAmmoElapsed == m_magazine.size());
}

void CWeapon::save(NET_Packet &output_packet)
{
	inherited::save(output_packet);
	save_data(iAmmoElapsed, output_packet);
	save_data(m_flagsAddOnState, output_packet);
	save_data(m_ammoType, output_packet);
	save_data(m_bZoomMode, output_packet);
}

void CWeapon::load(IReader &input_packet)
{
	inherited::load(input_packet);
	load_data(iAmmoElapsed, input_packet);
	load_data(m_flagsAddOnState, input_packet);
	UpdateAddonsVisibility();
	load_data(m_ammoType, input_packet);
	load_data(m_bZoomMode, input_packet);

	if (m_bZoomMode)	OnZoomIn();
	else			OnZoomOut();
}

void CWeapon::OnEvent(NET_Packet& P, u16 type)
{
	switch (type)
	{
	case GE_ADDON_CHANGE:
	{
		P.r_u8(m_flagsAddOnState);
		InitAddons();
		UpdateAddonsVisibility();
	}break;

	case GE_WPN_STATE_CHANGE:
	{
		u8				state;
		P.r_u8(state);
		P.r_u8(m_sub_state);
		//			u8 NewAmmoType =
		P.r_u8();
		u8 AmmoElapsed = P.r_u8();
		u8 NextAmmo = P.r_u8();
		if (NextAmmo == u8(-1))
			m_set_next_ammoType_on_reload = u32(-1);
		else
			m_set_next_ammoType_on_reload = u8(NextAmmo);

		if (OnClient()) SetAmmoElapsed(int(AmmoElapsed));
		OnStateSwitch(u32(state));
	}
		break;
	default:
	{
		inherited::OnEvent(P, type);
	}break;
	}
};

void CWeapon::shedule_Update(u32 dT)
{
	// Queue shrink
	//	u32	dwTimeCL		= Level().timeServer()-NET_Latency;
	//	while ((NET.size()>2) && (NET[1].dwTimeStamp<dwTimeCL)) NET.pop_front();

	// Inherited
	inherited::shedule_Update(dT);
}

void CWeapon::OnH_B_Independent(bool just_before_destroy)
{
	RemoveShotEffector();

	inherited::OnH_B_Independent(just_before_destroy);

	if (m_pHUD)
		m_pHUD->Hide();

	//завершить принудительно все процессы что шли
	FireEnd();
	m_bPending = false;
	SwitchState(eIdle);

	m_strapped_mode = false;
	SetHUDmode(FALSE);
	m_bZoomMode = false;
	UpdateXForm();
}

void CWeapon::OnH_A_Independent()
{
	m_dwWeaponIndependencyTime = Level().timeServer();
	inherited::OnH_A_Independent();
	Light_Destroy();
};

void CWeapon::OnH_A_Chield()
{
	inherited::OnH_A_Chield();

	UpdateAddonsVisibility();
};

void CWeapon::OnActiveItem()
{
	inherited::OnActiveItem();
	//если мы занружаемся и оружие было в руках
	SetState(eIdle);
	SetNextState(eIdle);
	if (m_pHUD) m_pHUD->Show();
}

void CWeapon::OnHiddenItem()
{
	inherited::OnHiddenItem();
	if (m_pHUD)	m_pHUD->Hide();
	SetState(eHidden);
	SetNextState(eHidden);
	m_set_next_ammoType_on_reload = u32(-1);
}

void CWeapon::OnH_B_Chield()
{
	m_dwWeaponIndependencyTime = 0;
	inherited::OnH_B_Chield();

	OnZoomOut();
	m_set_next_ammoType_on_reload = u32(-1);
}

void CWeapon::UpdateCL()
{
	inherited::UpdateCL();
	UpdateHUDAddonsVisibility();
	//подсветка от выстрела
	UpdateLight();

	//нарисовать партиклы
	UpdateFlameParticles();
	UpdateFlameParticles2();

	if (!IsGameTypeSingle())
		make_Interpolation();

	VERIFY(smart_cast<CKinematics*>(Visual()));

	if (GetState() == eIdle) {
		auto state = idle_state();
		if (m_idle_state != state) {
			m_idle_state = state;
			SwitchState(eIdle);
		}
	}
	else
		m_idle_state = eIdle;
	//
	if (ParentIsActor())
	{
		bool b_shoots = ((GetState() == eFire) || (GetState() == eFire2));
		g_actor->TryToBlockSprint(b_shoots);

		if(g_actor->conditions().IsCantWalk() && IsZoomed())
			OnZoomOut();
	}
	//
}

void CWeapon::renderable_Render()
{
	UpdateXForm();

	//нарисовать подсветку

	RenderLight();

	//если мы в режиме снайперки, то сам HUD рисовать не надо
	if (IsZoomed() && !IsRotatingToZoom() && ZoomTexture())
		m_bRenderHud = false;
	else
		m_bRenderHud = true;

	inherited::renderable_Render();
}

void CWeapon::signal_HideComplete()
{
	if (H_Parent()) setVisible(FALSE);
	m_bPending = false;
	if (m_pHUD) m_pHUD->Hide();
}

void CWeapon::SetDefaults()
{
	bWorking2 = false;
	m_bPending = false;

	m_flags.set(FUsingCondition, TRUE);
	bMisfire = false;
	m_flagsAddOnState = 0;
	m_bZoomMode = false;
}

void CWeapon::UpdatePosition(const Fmatrix& trans)
{
	Position().set(trans.c);
	XFORM().mul(trans, m_strapped_mode ? m_StrapOffset : m_Offset);
	VERIFY(!fis_zero(DET(renderable.xform)));
}

#pragma optimize("gyt", off)

bool CWeapon::Action(s32 cmd, u32 flags)
{
	if (inherited::Action(cmd, flags)) return true;

	switch (cmd)
	{
	case kWPN_FIRE:
	{
		//если оружие чем-то занято, то ничего не делать
		{
			if (flags&CMD_START)
			{
				if (IsPending())		return false;
				FireStart();
			}
			else
				FireEnd();
		};
	}
		return true;
	case kWPN_NEXT:
	{
		if (IsPending() || OnClient())
		{
			return false;
		}

		if (flags&CMD_START)
		{
			bool manually = !!psActorFlags.test(AF_NO_AUTO_RELOAD);

			if (GetNextAmmoType(manually) != u32(-1) && TryToGetAmmo(GetNextAmmoType(manually)))
			{
				m_set_next_ammoType_on_reload = GetNextAmmoType(manually);

				if (OnServer())
				{
					if (!manually) Reload();

					Msg("m_set_next_ammoType_on_reload [%d]", m_set_next_ammoType_on_reload);

					LPCSTR ammo_sect = pSettings->r_string(m_ammoTypes[m_set_next_ammoType_on_reload].c_str(), "inv_name_short");
					string1024	str;
					strconcat(sizeof(str), str, *CStringTable().translate("st_next_ammo_type"), ": ", *CStringTable().translate(ammo_sect));
					SDrawStaticStruct* _s = HUD().GetUI()->UIGame()->AddCustomStatic("next_ammo_type", true);
					_s->m_endTime = Device.fTimeGlobal + 1.0f;// 1 sec
					_s->wnd()->SetText(str);
				}
			}
		}
	}
		return true;

	case kWPN_ZOOM:
		if (IsZoomEnabled())
		{
			auto pActor = smart_cast<const CActor*>(H_Parent());
			auto pTorch = smart_cast<CTorch*>(pActor->inventory().ItemFromSlot(TORCH_SLOT));
			if (pTorch && pTorch->m_bNightVisionOn)
			{
				if (IsScopeAttached() && !GetGrenadeMode())
				{
					HUD().GetUI()->AddInfoMessage("cant_aim");
					return false;
				}
			}

			if (!pActor->conditions().IsCantWalk())
			{
				if (flags & CMD_START && !IsPending())
				{
					if (!psActorFlags.is(AF_HOLD_TO_AIM) && IsZoomed())
					{
						OnZoomOut();
					}
					else
						OnZoomIn();
				}
				else if (IsZoomed() && psActorFlags.is(AF_HOLD_TO_AIM))
				{
					OnZoomOut();
				}
				return true;
			}
			else
				HUD().GetUI()->AddInfoMessage("cant_walk");
		}
		else
			return false;

	case kWPN_ZOOM_INC:
	case kWPN_ZOOM_DEC:
		if (IsZoomEnabled() && IsZoomed() && m_bScopeDynamicZoom && IsScopeAttached())
		{
			if (cmd == kWPN_ZOOM_INC)  ZoomInc();
			else					ZoomDec();
			return true;
		}
		else
			return false;
	}
	return false;
}

float  CWeapon::GetZoomStepDelta(float scope_factor, float min_scope_factor, u32 step_count)
{
	float total_zoom_delta = scope_factor - min_scope_factor;
	return total_zoom_delta / step_count;
}

void CWeapon::ZoomInc()
{

	float currentZoomFactor = m_fZoomFactor;

	m_fZoomFactor -= GetZoomStepDelta(m_fScopeZoomFactor, m_fMinScopeZoomFactor, m_uZoomStepCount);//delta;
	clamp(m_fZoomFactor, m_fMinScopeZoomFactor, m_fScopeZoomFactor);

	if (!fsimilar(currentZoomFactor, m_fZoomFactor))
	{
		OnZoomChanged();
	}
}

void CWeapon::ZoomDec()
{
	float currentZoomFactor = m_fZoomFactor;

	m_fZoomFactor += GetZoomStepDelta(m_fScopeZoomFactor, m_fMinScopeZoomFactor, m_uZoomStepCount);//delta;
	clamp(m_fZoomFactor, m_fMinScopeZoomFactor, m_fScopeZoomFactor);

	if (!fsimilar(currentZoomFactor, m_fZoomFactor))
	{
		OnZoomChanged();
	}
}

void CWeapon::SpawnAmmo(u32 boxCurr, LPCSTR ammoSect, u32 ParentID)
{
	if (!m_ammoTypes.size())			return;
	if (OnClient())					return;
	m_bAmmoWasSpawned = true;

	if (!ammoSect) ammoSect = m_ammoTypes.front().c_str();

	CSE_Abstract *D = F_entity_Create(ammoSect);

	if (D->m_tClassID == CLSID_OBJECT_AMMO ||
		D->m_tClassID == CLSID_OBJECT_A_M209 ||
		D->m_tClassID == CLSID_OBJECT_A_VOG25 ||
		D->m_tClassID == CLSID_OBJECT_A_OG7B)
	{
		CSE_ALifeItemAmmo *l_pA = smart_cast<CSE_ALifeItemAmmo*>(D);
		R_ASSERT(l_pA);
		l_pA->m_boxSize = (u16)pSettings->r_s32(ammoSect, "box_size");
		D->s_name = ammoSect;
		D->set_name_replace("");
		D->s_gameid = u8(GameID());
		D->s_RP = 0xff;
		D->ID = 0xffff;
		if (ParentID == 0xffffffff)
			D->ID_Parent = (u16)H_Parent()->ID();
		else
			D->ID_Parent = (u16)ParentID;

		D->ID_Phantom = 0xffff;
		D->s_flags.assign(M_SPAWN_OBJECT_LOCAL);
		D->RespawnTime = 0;
		l_pA->m_tNodeID = g_dedicated_server ? u32(-1) : ai_location().level_vertex_id();
		
		if (boxCurr == 0xffffffff)
			boxCurr = l_pA->m_boxSize;

		if (boxCurr > 0)
		{
			while (boxCurr)
			{
				l_pA->a_elapsed = (u16)(boxCurr > l_pA->m_boxSize ? l_pA->m_boxSize : boxCurr);
				NET_Packet				P;
				D->Spawn_Write(P, TRUE);
				Level().Send(P, net_flags(TRUE));

				if (boxCurr > l_pA->m_boxSize)
					boxCurr -= l_pA->m_boxSize;
				else
					boxCurr = 0;
			}
		}
		else
		{
			NET_Packet				P;
			D->Spawn_Write(P, TRUE);
			Level().Send(P, net_flags(TRUE));
		}
	};
	F_entity_Destroy(D);
}

int CWeapon::GetAmmoCurrent(bool use_item_to_spawn) const
{
	int l_count = iAmmoElapsed;
	if (!m_pCurrentInventory) return l_count;

	//чтоб не делать лишних пересчетов
	if (m_pCurrentInventory->ModifyFrame() <= m_dwAmmoCurrentCalcFrame)
		if (g_actor->m_inventory != m_pCurrentInventory || !psActorFlags.test(AF_AMMO_FROM_BELT)) //очень некрасивый костыль!
		return l_count + iAmmoCurrent;

	//Msg("[%s] get ammo current [%d]:[%s] weapon [%s] ", m_pCurrentInventory->GetOwner()->Name(), l_count + iAmmoCurrent, *m_ammoTypes[m_ammoType], this->Name_script());

	m_dwAmmoCurrentCalcFrame = Device.dwFrame;
	iAmmoCurrent = 0;

	for (int i = 0; i < (int)m_ammoTypes.size(); ++i)
	{
		LPCSTR l_ammoType = *m_ammoTypes[i];

		bool search_ruck = g_actor->m_inventory != m_pCurrentInventory || !psActorFlags.test(AF_AMMO_FROM_BELT);
		TIItemContainer &list = search_ruck ? m_pCurrentInventory->m_ruck : m_pCurrentInventory->m_belt;
		for (TIItemContainer::iterator l_it = list.begin(); list.end() != l_it; ++l_it)
		{
			CWeaponAmmo *l_pAmmo = smart_cast<CWeaponAmmo*>(*l_it);
			if (l_pAmmo && !xr_strcmp(l_pAmmo->cNameSect(), l_ammoType))
			{
				iAmmoCurrent = iAmmoCurrent + l_pAmmo->m_boxCurr;
			}
		}
		//помимо пояса еще и в слотах поищем
		if (!search_ruck)
		{
			for (u32 i = 0; i < m_pCurrentInventory->m_slots.size(); ++i)
			{
				CWeaponAmmo *l_pAmmo = smart_cast<CWeaponAmmo*>(m_pCurrentInventory->m_slots[i].m_pIItem);
				if (l_pAmmo && !xr_strcmp(l_pAmmo->cNameSect(), l_ammoType))
				{
					iAmmoCurrent = iAmmoCurrent + l_pAmmo->m_boxCurr;
				}
			}
		}

		if (!use_item_to_spawn)
			continue;

		if (!inventory_owner().item_to_spawn())
			continue;

		iAmmoCurrent += inventory_owner().ammo_in_box_to_spawn();

		//Msg("[%s] get ammo current [%d]:[%s] weapon [%s] ", m_pCurrentInventory->GetOwner()->Name(), l_count + iAmmoCurrent, l_ammoType, this->Name_script());
	}
	return l_count + iAmmoCurrent;
}
//
u32 CWeapon::GetNextAmmoType(bool looped)
{
	u32 l_newType = looped ? m_set_next_ammoType_on_reload : m_ammoType;

	for (;;)
	{
		if (++l_newType >= m_ammoTypes.size())
		{
			for (l_newType = 0; l_newType < m_ammoTypes.size(); ++l_newType)
				if (unlimited_ammo() || m_pCurrentInventory->GetAmmo(m_ammoTypes[l_newType].c_str(), ParentIsActor()))
					break;
			break;
		}

		if (unlimited_ammo() || m_pCurrentInventory->GetAmmo(m_ammoTypes[l_newType].c_str(), ParentIsActor()))
			break;
	}

	if (l_newType != (looped ? m_set_next_ammoType_on_reload : m_ammoType) && l_newType < m_ammoTypes.size())
		return l_newType;
	else
		return u32(-1);
}
//
float CWeapon::GetConditionMisfireProbability() const
{
	float mis = 0.0f;

	//if (GetCondition() > 0.95f) return 0.0f;

	//float mis = misfireProbability + powf(1.f - GetCondition(), 3.f)*misfireConditionK;

	//вероятность осечки от патрона
	if (!m_magazine.empty())
	{
		const auto &l_cartridge = m_magazine.back();
		mis = l_cartridge.m_misfireProbability;
	}
	//вероятность осечки от состояния оружия
	if (GetCondition() < 0.95f)
		mis += (misfireProbability + powf(1.f - GetCondition(), 3.f)*misfireConditionK);

	clamp(mis, 0.0f, 0.99f);
	return mis;
}

BOOL CWeapon::CheckForMisfire()
{
	if (OnClient()) return FALSE;

	float rnd = ::Random.randF(0.f, 1.f);
	float mp = GetConditionMisfireProbability();
	if (rnd < mp)
	{
		FireEnd();

		bMisfire = true;
		SwitchState(eMisfire);

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL CWeapon::IsMisfire() const
{
	return bMisfire;
}
void CWeapon::Reload()
{
	if (!m_bAimReloading)
		OnZoomOut();
}

bool CWeapon::IsGrenadeLauncherAttached() const
{
	return (CSE_ALifeItemWeapon::eAddonAttachable == m_eGrenadeLauncherStatus &&
		0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher)) ||
		CSE_ALifeItemWeapon::eAddonPermanent == m_eGrenadeLauncherStatus;
}

bool CWeapon::IsScopeAttached() const
{
	return (CSE_ALifeItemWeapon::eAddonAttachable == m_eScopeStatus &&
		0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonScope)) ||
		CSE_ALifeItemWeapon::eAddonPermanent == m_eScopeStatus;
}

bool CWeapon::IsSilencerAttached() const
{
	return (CSE_ALifeItemWeapon::eAddonAttachable == m_eSilencerStatus &&
		0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonSilencer)) ||
		CSE_ALifeItemWeapon::eAddonPermanent == m_eSilencerStatus;
}

bool CWeapon::GrenadeLauncherAttachable()
{
	return (CSE_ALifeItemWeapon::eAddonAttachable == m_eGrenadeLauncherStatus);
}
bool CWeapon::ScopeAttachable()
{
	return (CSE_ALifeItemWeapon::eAddonAttachable == m_eScopeStatus);
}
bool CWeapon::SilencerAttachable()
{
	return (CSE_ALifeItemWeapon::eAddonAttachable == m_eSilencerStatus);
}

void CWeapon::UpdateHUDAddonsVisibility()
{//actor only
	if (H_Parent() != Level().CurrentEntity())				return;
	if (m_pHUD->IsHidden())									return;
	//	if(IsZoomed() && )

	CKinematics* pHudVisual = smart_cast<CKinematics*>(m_pHUD->Visual());
	VERIFY(pHudVisual);
	if (H_Parent() != Level().CurrentEntity()) pHudVisual = NULL;

	if (!pHudVisual)return;
	u16  bone_id;

	bone_id = pHudVisual->LL_BoneID(wpn_scope);
	if (ScopeAttachable())
	{
		VERIFY2(bone_id != BI_NONE, "there are no scope bone.");
		if (IsScopeAttached())
		{
			if (FALSE == pHudVisual->LL_GetBoneVisible(bone_id))
				pHudVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);
		}
		else{
			if (pHudVisual->LL_GetBoneVisible(bone_id))
				pHudVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
		}
	}
	if (m_eScopeStatus == CSE_ALifeItemWeapon::eAddonDisabled && bone_id != BI_NONE &&
		pHudVisual->LL_GetBoneVisible(bone_id))
		pHudVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
	else
		if (m_eScopeStatus == CSE_ALifeItemWeapon::eAddonPermanent && bone_id != BI_NONE &&
			!pHudVisual->LL_GetBoneVisible(bone_id))
			pHudVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);

	bone_id = pHudVisual->LL_BoneID(wpn_silencer);
	if (SilencerAttachable())
	{
		VERIFY2(bone_id != BI_NONE, "there are no silencer bone.");
		if (IsSilencerAttached())
		{
			if (FALSE == pHudVisual->LL_GetBoneVisible(bone_id))
				pHudVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);
		}
		else{
			if (pHudVisual->LL_GetBoneVisible(bone_id))
				pHudVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
		}
	}
	if (m_eSilencerStatus == CSE_ALifeItemWeapon::eAddonDisabled && bone_id != BI_NONE &&
		pHudVisual->LL_GetBoneVisible(bone_id))
		pHudVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
	else
		if (m_eSilencerStatus == CSE_ALifeItemWeapon::eAddonPermanent && bone_id != BI_NONE &&
			!pHudVisual->LL_GetBoneVisible(bone_id))
			pHudVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);

	bone_id = pHudVisual->LL_BoneID(wpn_grenade_launcher);
	if (GrenadeLauncherAttachable())
	{
		if (bone_id == BI_NONE)
			bone_id = pHudVisual->LL_BoneID(wpn_launcher);

		VERIFY2(bone_id != BI_NONE, "there are no grenade launcher bone.");
		if (IsGrenadeLauncherAttached())
		{
			if (FALSE == pHudVisual->LL_GetBoneVisible(bone_id))
				pHudVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);
		}
		else{
			if (pHudVisual->LL_GetBoneVisible(bone_id))
				pHudVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
		}
	}
	if (m_eGrenadeLauncherStatus == CSE_ALifeItemWeapon::eAddonDisabled && bone_id != BI_NONE &&
		pHudVisual->LL_GetBoneVisible(bone_id))
		pHudVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
	else
		if (m_eGrenadeLauncherStatus == CSE_ALifeItemWeapon::eAddonPermanent && bone_id != BI_NONE &&
			!pHudVisual->LL_GetBoneVisible(bone_id))
			pHudVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);
}

void CWeapon::UpdateAddonsVisibility()
{
	CKinematics* pWeaponVisual = smart_cast<CKinematics*>(Visual()); R_ASSERT(pWeaponVisual);

	u16  bone_id;
	UpdateHUDAddonsVisibility();

	bone_id = pWeaponVisual->LL_BoneID(wpn_scope);
	if (ScopeAttachable())
	{
		if (IsScopeAttached())
		{
			if (FALSE == pWeaponVisual->LL_GetBoneVisible(bone_id))
				pWeaponVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);
		}
		else{
			if (pWeaponVisual->LL_GetBoneVisible(bone_id))
				pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
		}
	}
	if (m_eScopeStatus == CSE_ALifeItemWeapon::eAddonDisabled && bone_id != BI_NONE &&
		pWeaponVisual->LL_GetBoneVisible(bone_id))

		pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);

	bone_id = pWeaponVisual->LL_BoneID(wpn_silencer);
	if (SilencerAttachable())
	{
		if (IsSilencerAttached()){
			if (FALSE == pWeaponVisual->LL_GetBoneVisible(bone_id))
				pWeaponVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);
		}
		else{
			if (pWeaponVisual->LL_GetBoneVisible(bone_id))
				pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
		}
	}
	if (m_eSilencerStatus == CSE_ALifeItemWeapon::eAddonDisabled && bone_id != BI_NONE &&
		pWeaponVisual->LL_GetBoneVisible(bone_id))

		pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);

	bone_id = pWeaponVisual->LL_BoneID(wpn_launcher);
	if (GrenadeLauncherAttachable())
	{
		if (IsGrenadeLauncherAttached())
		{
			if (FALSE == pWeaponVisual->LL_GetBoneVisible(bone_id))
				pWeaponVisual->LL_SetBoneVisible(bone_id, TRUE, TRUE);
		}
		else{
			if (pWeaponVisual->LL_GetBoneVisible(bone_id))
				pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);
		}
	}
	if (m_eGrenadeLauncherStatus == CSE_ALifeItemWeapon::eAddonDisabled && bone_id != BI_NONE &&
		pWeaponVisual->LL_GetBoneVisible(bone_id))

		pWeaponVisual->LL_SetBoneVisible(bone_id, FALSE, TRUE);

	pWeaponVisual->CalculateBones_Invalidate();
	pWeaponVisual->CalculateBones();
}

bool CWeapon::Activate()
{
	UpdateAddonsVisibility();
	return inherited::Activate();
}

void CWeapon::InitAddons()
{
}

float CWeapon::CurrentZoomFactor()
{
	float res = 1.f;

	if (IsScopeAttached())
	{
		if (m_bScopeDynamicZoom)
		{
			clamp(m_fRTZoomFactor, m_fMinScopeZoomFactor, m_fScopeZoomFactor);
			res = m_fRTZoomFactor;
//			Msg("m_fRTZoomFactor res = [%.2f]", res);
		}
		else
		{
			res = m_fScopeZoomFactor;
//			Msg("m_fScopeZoomFactor res = [%.2f]", res);
		}
	}
	else
	{
		res = m_fIronSightZoomFactor;
//		Msg("m_fIronSightZoomFactor res = [%.2f]", res);
	}

	return res;
};

void CWeapon::OnZoomIn()
{
	m_bZoomMode = true;
	m_fZoomFactor = CurrentZoomFactor();

//	Msg("OnZoomIn() m_fZoomFactor = [%.2f]", m_fZoomFactor);
	//StopHudInertion();
}

void CWeapon::OnZoomOut()
{
	if (H_Parent() && IsZoomed() && !IsRotatingToZoom() && m_bScopeDynamicZoom)
	{
		if (IsScopeAttached() && !GetGrenadeMode())
		{
			m_fRTZoomFactor = m_fZoomFactor;//store current
			clamp(m_fRTZoomFactor, m_fMinScopeZoomFactor, m_fScopeZoomFactor);
			//		Msg("OnZoomOut() m_fRTZoomFactor res = [%.2f]", m_fRTZoomFactor);
		}
	}

	m_bZoomMode = false;
	m_fZoomFactor = 1.0f;// g_fov;

	//StartHudInertion();
}

CUIStaticItem* CWeapon::ZoomTexture()
{
	if (UseScopeTexture())
		return m_UIScope;
	else
		return NULL;
}

void CWeapon::SwitchState(u32 S)
{
	if (OnClient()) return;

	SetNextState(S);	// Very-very important line of code!!! :)
	if (CHudItem::object().Local() && !CHudItem::object().getDestroy()/* && (S!=NEXT_STATE)*/
		&& m_pCurrentInventory && OnServer())
	{
		// !!! Just single entry for given state !!!
		NET_Packet		P;
		CHudItem::object().u_EventGen(P, GE_WPN_STATE_CHANGE, CHudItem::object().ID());
		P.w_u8(u8(S));
		P.w_u8(u8(m_sub_state));
		P.w_u8(u8(m_ammoType & 0xff));
		P.w_u8(u8(iAmmoElapsed & 0xff));
		P.w_u8(u8(m_set_next_ammoType_on_reload & 0xff));
		CHudItem::object().u_EventSend(P, net_flags(TRUE, TRUE, FALSE, TRUE));
	}
}

void CWeapon::OnMagazineEmpty()
{
	VERIFY((u32)iAmmoElapsed == m_magazine.size());
}

void CWeapon::reinit()
{
	CShootingObject::reinit();
	CHudItemObject::reinit();
}

void CWeapon::reload(LPCSTR section)
{
	CShootingObject::reload(section);
	CHudItemObject::reload(section);

	m_can_be_strapped = true;
	m_strapped_mode = false;

	if (pSettings->line_exist(section, "strap_bone0"))
		m_strap_bone0 = pSettings->r_string(section, "strap_bone0");
	else
		m_can_be_strapped = false;

	if (pSettings->line_exist(section, "strap_bone1"))
		m_strap_bone1 = pSettings->r_string(section, "strap_bone1");
	else
		m_can_be_strapped = false;

	if (m_eScopeStatus == ALife::eAddonAttachable) {
		m_addon_holder_range_modifier	= READ_IF_EXISTS(pSettings, r_float, GetScopeName(), "holder_range_modifier", m_holder_range_modifier);
		m_addon_holder_fov_modifier		= READ_IF_EXISTS(pSettings, r_float, GetScopeName(), "holder_fov_modifier", m_holder_fov_modifier);
	}
	else {
		m_addon_holder_range_modifier = m_holder_range_modifier;
		m_addon_holder_fov_modifier = m_holder_fov_modifier;
	}

	{
		Fvector				pos, ypr;
		pos = pSettings->r_fvector3(section, "position");
		ypr = pSettings->r_fvector3(section, "orientation");
		ypr.mul(PI / 180.f);

		m_Offset.setHPB(ypr.x, ypr.y, ypr.z);
		m_Offset.translate_over(pos);
	}

	m_StrapOffset = m_Offset;
	if (pSettings->line_exist(section, "strap_position") && pSettings->line_exist(section, "strap_orientation")) {
		Fvector				pos, ypr;
		pos = pSettings->r_fvector3(section, "strap_position");
		ypr = pSettings->r_fvector3(section, "strap_orientation");
		ypr.mul(PI / 180.f);

		m_StrapOffset.setHPB(ypr.x, ypr.y, ypr.z);
		m_StrapOffset.translate_over(pos);
	}
	else
		m_can_be_strapped = false;

	m_ef_main_weapon_type = READ_IF_EXISTS(pSettings, r_u32, section, "ef_main_weapon_type", u32(-1));
	m_ef_weapon_type = READ_IF_EXISTS(pSettings, r_u32, section, "ef_weapon_type", u32(-1));
}

void CWeapon::create_physic_shell()
{
	CPhysicsShellHolder::create_physic_shell();
}

void CWeapon::activate_physic_shell()
{
	CPhysicsShellHolder::activate_physic_shell();
}

void CWeapon::setup_physic_shell()
{
	CPhysicsShellHolder::setup_physic_shell();
}

int		g_iWeaponRemove = 1;

bool CWeapon::NeedToDestroyObject()	const
{
	if (H_Parent()) return false;
	if (g_iWeaponRemove == -1) return false;
	if (g_iWeaponRemove == 0) return true;
	return (TimePassedAfterIndependant() > m_dwWeaponRemoveTime);
}

ALife::_TIME_ID	 CWeapon::TimePassedAfterIndependant()	const
{
	if (!H_Parent() && m_dwWeaponIndependencyTime != 0)
		return Level().timeServer() - m_dwWeaponIndependencyTime;
	else
		return 0;
}

bool CWeapon::can_kill() const
{
	if (GetAmmoCurrent(true) || m_ammoTypes.empty())
		return				(true);

	return					(false);
}

CInventoryItem *CWeapon::can_kill(CInventory *inventory) const
{
	if (GetAmmoElapsed() || m_ammoTypes.empty())
		return				(const_cast<CWeapon*>(this));

	TIItemContainer::iterator I = inventory->m_all.begin();
	TIItemContainer::iterator E = inventory->m_all.end();
	for (; I != E; ++I) {
		CInventoryItem	*inventory_item = smart_cast<CInventoryItem*>(*I);
		if (!inventory_item)
			continue;

		xr_vector<shared_str>::const_iterator	i = std::find(m_ammoTypes.begin(), m_ammoTypes.end(), inventory_item->object().cNameSect());
		if (i != m_ammoTypes.end())
			return			(inventory_item);
	}

	return					(0);
}

const CInventoryItem *CWeapon::can_kill(const xr_vector<const CGameObject*> &items) const
{
	if (m_ammoTypes.empty())
		return				(this);

	xr_vector<const CGameObject*>::const_iterator I = items.begin();
	xr_vector<const CGameObject*>::const_iterator E = items.end();
	for (; I != E; ++I) {
		const CInventoryItem	*inventory_item = smart_cast<const CInventoryItem*>(*I);
		if (!inventory_item)
			continue;

		xr_vector<shared_str>::const_iterator	i = std::find(m_ammoTypes.begin(), m_ammoTypes.end(), inventory_item->object().cNameSect());
		if (i != m_ammoTypes.end())
			return			(inventory_item);
	}

	return					(0);
}

bool CWeapon::ready_to_kill() const
{
	return  (
		!IsMisfire() &&
		((GetState() == eIdle) || (GetState() == eFire) || (GetState() == eFire2)) &&
		GetAmmoElapsed()
		);
}

void CWeapon::UpdateHudAdditonal(Fmatrix& trans)
{
	auto pActor = smart_cast<const CActor*>(H_Parent());
	if (!pActor) return;

	if ((pActor->IsZoomAimingMode() && m_fZoomRotationFactor <= 1.f) ||
		(!pActor->IsZoomAimingMode() && m_fZoomRotationFactor > 0.f))
	{
		Fmatrix hud_rotation;
		hud_rotation.identity();
		hud_rotation.rotateX(m_pHUD->ZoomRotateX()*m_fZoomRotationFactor);

		Fmatrix hud_rotation_y;
		hud_rotation_y.identity();
		hud_rotation_y.rotateY(m_pHUD->ZoomRotateY()*m_fZoomRotationFactor);
		hud_rotation.mulA_43(hud_rotation_y);

		Fvector offset = m_pHUD->ZoomOffset();
		offset.mul(m_fZoomRotationFactor);
		hud_rotation.translate_over(offset);
		trans.mulB_43(hud_rotation);

#pragma todo("Cribbledirge: test these callbacks (including the one in the 'else' statmenet) to see if they work with scopes.")
		if (pActor->IsZoomAimingMode())
		{
			m_fZoomRotationFactor += Device.fTimeDelta / m_fZoomRotateTime;
			// Send callback for zoom in (Added by Cribbledirge).
			if (!m_bZoomingIn)
			{
				pActor->callback(GameObject::eOnActorWeaponZoomIn)(lua_game_object());
				m_bZoomingIn = true;
			}
		}
		else
		{
			m_fZoomRotationFactor -= Device.fTimeDelta / m_fZoomRotateTime;
			// Send callback for zoom out (Added by Cribbledirge).
			if (m_bZoomingIn)
			{
				pActor->callback(GameObject::eOnActorWeaponZoomOut)(lua_game_object());
				m_bZoomingIn = false;
			}
		}
		clamp(m_fZoomRotationFactor, 0.f, 1.f);
	}
}

void	CWeapon::SetAmmoElapsed(int ammo_count)
{
	iAmmoElapsed = ammo_count;

	u32 uAmmo = u32(iAmmoElapsed);

	if (uAmmo != m_magazine.size())
	{
		if (uAmmo > m_magazine.size())
		{
			CCartridge			l_cartridge;
			l_cartridge.Load(*m_ammoTypes[m_ammoType], u8(m_ammoType));
			while (uAmmo > m_magazine.size())
				m_magazine.push_back(l_cartridge);
		}
		else
		{
			while (uAmmo < m_magazine.size())
				m_magazine.pop_back();
		};
	};
}

u32	CWeapon::ef_main_weapon_type() const
{
	VERIFY(m_ef_main_weapon_type != u32(-1));
	return	(m_ef_main_weapon_type);
}

u32	CWeapon::ef_weapon_type() const
{
	VERIFY(m_ef_weapon_type != u32(-1));
	return	(m_ef_weapon_type);
}

bool CWeapon::IsNecessaryItem(const shared_str& item_sect)
{
	return (std::find(m_ammoTypes.begin(), m_ammoTypes.end(), item_sect) != m_ammoTypes.end());
}

void CWeapon::modify_holder_params(float &range, float &fov) const
{
	if (!IsScopeAttached()) {
		inherited::modify_holder_params(range, fov);
		return;
	}
	range *= m_addon_holder_range_modifier;
	fov *= m_addon_holder_fov_modifier;
}

void CWeapon::OnDrawUI()
{
	if (IsZoomed() && ZoomHideCrosshair()){
		if (ZoomTexture() && !IsRotatingToZoom()){
			ZoomTexture()->SetPos(0, 0);
			ZoomTexture()->SetRect(0, 0, UI_BASE_WIDTH, UI_BASE_HEIGHT);
			ZoomTexture()->Render();

			//			m_UILens.Draw();
		}
	}
}

bool CWeapon::unlimited_ammo()
{
	if (GameID() == GAME_SINGLE)
		return psActorFlags.test(AF_UNLIMITEDAMMO) &&
		m_DefaultCartridge.m_flags.test(CCartridge::cfCanBeUnlimited);

	return (GameID() != GAME_ARTEFACTHUNT) &&
		m_DefaultCartridge.m_flags.test(CCartridge::cfCanBeUnlimited);
};

LPCSTR	CWeapon::GetCurrentAmmo_ShortName()
{
	if (m_magazine.empty()) return ("");
	CCartridge &l_cartridge = m_magazine.back();
	return *(l_cartridge.m_InvShortName);
}

float CWeapon::GetAmmoInMagazineWeight(const decltype(CWeapon::m_magazine)& mag)
{
	float res = 0;
	const char* last_type = nullptr;
	float last_ammo_weight = 0;
	for (auto c : mag)
	{
		// Usually ammos in mag have same type, use this fact to improve performance
		if (last_type != c.m_ammoSect.c_str())
		{
			last_type = c.m_ammoSect.c_str();
			last_ammo_weight = c.Weight();
		}
		res += last_ammo_weight;
	}
	return res;
}

float CWeapon::Weight()
{
	float res = CInventoryItemObject::Weight();

	if (GrenadeLauncherAttachable() && IsGrenadeLauncherAttached())
		res += pSettings->r_float(GetGrenadeLauncherName(), "inv_weight");

	if (ScopeAttachable() && IsScopeAttached())
		res += pSettings->r_float(GetScopeName(), "inv_weight");

	if (SilencerAttachable() && IsSilencerAttached())
		res += pSettings->r_float(GetSilencerName(), "inv_weight");

	res += GetAmmoInMagazineWeight(m_magazine);

	return res;
}
void CWeapon::Hide()
{
	if (IsGameTypeSingle())
		SwitchState(eHiding);
	else
		SwitchState(eHidden);

	OnZoomOut();
}

void CWeapon::Show()
{
	SwitchState(eShowing);
}

bool CWeapon::show_crosshair()
{
	return !IsZoomed();
}

bool CWeapon::show_indicators()
{
	return !(IsZoomed() && ZoomTexture());
}

float CWeapon::GetConditionToShow() const
{
	return	(GetCondition());//powf(GetCondition(),4.0f));
}

BOOL CWeapon::ParentMayHaveAimBullet()
{
	CObject* O = H_Parent();
	CEntityAlive* EA = smart_cast<CEntityAlive*>(O);
	return EA->cast_actor() != 0;
}

BOOL CWeapon::ParentIsActor()
{
	CObject* O = H_Parent();
	CEntityAlive* EA = smart_cast<CEntityAlive*>(O);
	return EA && EA->cast_actor() != 0;
}

const float &CWeapon::hit_probability() const
{
	VERIFY((g_SingleGameDifficulty >= egdNovice) && (g_SingleGameDifficulty <= egdMaster));
	return					(m_hit_probability[egdNovice]);
}

u8 CWeapon::idle_state() {
	CActor *actor = smart_cast<CActor*>(H_Parent());

	if (actor)
		if (actor->get_state() & mcSprint) {
			return eSubstateIdleSprint;
		}
		else if (actor->is_actor_running())
			return eSubstateIdleMoving;

	return eIdle;
}

int	CWeapon::GetScopeX() 
{ 
	int res = pSettings->r_s32(cNameSect(), "scope_x"); 

	string1024 scope_sect_x;
	const char* scope_sect = m_scopes[m_cur_scope].c_str();
	strconcat(sizeof(scope_sect_x), scope_sect_x, scope_sect, "_x");

	if (pSettings->line_exist(cNameSect(), scope_sect_x))
		res = pSettings->r_s32(cNameSect(), scope_sect_x);

	return res;
}

int	CWeapon::GetScopeY() 
{ 
	int res = pSettings->r_s32(cNameSect(), "scope_y");

	string1024 scope_sect_y;
	const char* scope_sect = m_scopes[m_cur_scope].c_str();
	strconcat(sizeof(scope_sect_y), scope_sect_y, scope_sect, "_y");

	if (pSettings->line_exist(cNameSect(), scope_sect_y))
		res = pSettings->r_s32(cNameSect(), scope_sect_y);

	return res;
}

int	CWeapon::GetSilencerX()
{
	int res = pSettings->r_s32(cNameSect(), "silencer_x");

	string1024 silencer_sect_x;
	const char* silencer_sect = m_silencers[m_cur_silencer].c_str();
	strconcat(sizeof(silencer_sect_x), silencer_sect_x, silencer_sect, "_x");

	if (pSettings->line_exist(cNameSect(), silencer_sect_x))
		res = pSettings->r_s32(cNameSect(), silencer_sect_x);

	return res;
}

int	CWeapon::GetSilencerY()
{
	int res = pSettings->r_s32(cNameSect(), "silencer_y");

	string1024 silencer_sect_y;
	const char* silencer_sect = m_silencers[m_cur_silencer].c_str();
	strconcat(sizeof(silencer_sect_y), silencer_sect_y, silencer_sect, "_y");

	if (pSettings->line_exist(cNameSect(), silencer_sect_y))
		res = pSettings->r_s32(cNameSect(), silencer_sect_y);

	return res;
}

int	CWeapon::GetGrenadeLauncherX()
{
	int res = pSettings->r_s32(cNameSect(), "grenade_launcher_x");

	string1024 glauncher_sect_x;
	const char* glauncher_sect = m_glaunchers[m_cur_glauncher].c_str();
	strconcat(sizeof(glauncher_sect_x), glauncher_sect_x, glauncher_sect, "_x");

	if (pSettings->line_exist(cNameSect(), glauncher_sect_x))
		res = pSettings->r_s32(cNameSect(), glauncher_sect_x);

	return res;
}

int	CWeapon::GetGrenadeLauncherY()
{
	int res = pSettings->r_s32(cNameSect(), "grenade_launcher_y");

	string1024 glauncher_sect_y;
	const char* glauncher_sect = m_glaunchers[m_cur_glauncher].c_str();
	strconcat(sizeof(glauncher_sect_y), glauncher_sect_y, glauncher_sect, "_y");

	if (pSettings->line_exist(cNameSect(), glauncher_sect_y))
		res = pSettings->r_s32(cNameSect(), glauncher_sect_y);

	return res;
}