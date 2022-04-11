#include "stdafx.h"
#include "pch_script.h"
#include "torch.h"
#include "entity.h"
#include "actor.h"
#include "../LightAnimLibrary.h"
#include "PhysicsShell.h"
#include "xrserver_objects_alife_items.h"
#include "ai_sounds.h"

#include "HUDManager.h"
#include "level.h"
#include "../skeletoncustom.h"
#include "../camerabase.h"
#include "inventory.h"
#include "game_base_space.h"

#include "UIGameCustom.h"
#include "actorEffector.h"
#include "CustomOutfit.h"
#include "game_object_space.h"
#include "script_callback_ex.h"

static const float		TIME_2_HIDE					= 5.f;
static const float		TORCH_INERTION_CLAMP		= PI_DIV_6;
static const float		TORCH_INERTION_SPEED_MAX	= 7.5f;
static const float		TORCH_INERTION_SPEED_MIN	= 0.5f;
static const Fvector	TORCH_OFFSET				= {-0.2f,+0.1f,-0.3f};
static const Fvector	OMNI_OFFSET					= {-0.2f,+0.1f,-0.1f};
static const float		OPTIMIZATION_DISTANCE		= 100.f;

static bool stalker_use_dynamic_lights	= false;

CTorch::CTorch(void) 
{
	light_render				= ::Render->light_create();
	light_render->set_type		(IRender_Light::SPOT);
	light_render->set_virtual_size (0.1);
	light_render->set_shadow	(true);
	light_omni					= ::Render->light_create();
	light_omni->set_type		(IRender_Light::POINT);
	light_omni->set_shadow		(false);

	m_switched_on				= false;
	glow_render					= ::Render->glow_create();
	lanim						= 0;
	time2hide					= 0;
	fBrightness					= 1.f;
	b_lastState					= false;

	/*m_NightVisionRechargeTime	= 6.f;
	m_NightVisionRechargeTimeMin= 2.f;
	m_NightVisionDischargeTime	= 10.f;
	m_NightVisionChargeTime		= 0.f;*/

	m_prev_hp.set				(0,0);
	m_delta_h					= 0;
	SetSlot						(TORCH_SLOT);
	need_slot					= true;
	m_UINightVisionMask			= NULL;
}

CTorch::~CTorch(void) 
{
	light_render.destroy	();
	light_omni.destroy	();
	glow_render.destroy		();
	HUD_SOUND::DestroySound	(m_NightVisionOnSnd);
	HUD_SOUND::DestroySound	(m_NightVisionOffSnd);
	HUD_SOUND::DestroySound	(m_NightVisionIdleSnd);
	HUD_SOUND::DestroySound	(m_NightVisionBrokenSnd);
	xr_delete				(m_UINightVisionMask);
}

inline bool CTorch::can_use_dynamic_lights	()
{
	if (!H_Parent())
		return				(true);

	CInventoryOwner			*owner = smart_cast<CInventoryOwner*>(H_Parent());
	if (!owner)
		return				(true);

	return					(owner->can_use_dynamic_lights());
}

void CTorch::Load(LPCSTR section) 
{
	inherited::Load			(section);

	m_bTorchLightEnabled = !!READ_IF_EXISTS(pSettings, r_bool, section, "torch_light", TRUE);
	if (m_bTorchLightEnabled)
		light_trace_bone		= pSettings->r_string(section,"light_trace_bone");


	m_bNightVisionEnabled = !!pSettings->r_bool(section,"night_vision");
	if(m_bNightVisionEnabled)
	{
		HUD_SOUND::LoadSound(section,"snd_night_vision_on"	, m_NightVisionOnSnd	, SOUND_TYPE_ITEM_USING);
		HUD_SOUND::LoadSound(section,"snd_night_vision_off"	, m_NightVisionOffSnd	, SOUND_TYPE_ITEM_USING);
		HUD_SOUND::LoadSound(section,"snd_night_vision_idle", m_NightVisionIdleSnd	, SOUND_TYPE_ITEM_USING);
		HUD_SOUND::LoadSound(section,"snd_night_vision_broken", m_NightVisionBrokenSnd, SOUND_TYPE_ITEM_USING);

	
		/*m_NightVisionRechargeTime		= pSettings->r_float(section,"night_vision_recharge_time");
		m_NightVisionRechargeTimeMin	= pSettings->r_float(section,"night_vision_recharge_time_min");
		m_NightVisionDischargeTime		= pSettings->r_float(section,"night_vision_discharge_time");
		m_NightVisionChargeTime			= m_NightVisionRechargeTime;*/

		m_NightVisionSect				= READ_IF_EXISTS(pSettings, r_string, section, "night_vision_effector", NULL);
		m_NightVisionTexture			= READ_IF_EXISTS(pSettings, r_string, section, "night_vision_texture",	NULL);
	}
}

void CTorch::SwitchNightVision()
{
	if (OnClient()) return;
	SwitchNightVision(!m_bNightVisionOn);	
}

void CTorch::SwitchNightVision(bool vision_on)
{
	if(!m_bNightVisionEnabled) return;

	CActor *pA = smart_cast<CActor *>(H_Parent());
	if (!pA)					return;

	CTorch* pActorTorch = smart_cast<CTorch*>(pA->inventory().ItemFromSlot(TORCH_SLOT));
	if (pActorTorch && pActorTorch != this)
		return;
	
	if(vision_on /*&& (m_NightVisionChargeTime > m_NightVisionRechargeTimeMin || OnClient())*/)
	{
		//m_NightVisionChargeTime = m_NightVisionDischargeTime*m_NightVisionChargeTime/m_NightVisionRechargeTime;
		m_bNightVisionOn = true;
	}
	else
	{
		m_bNightVisionOn = false;
	}

	bool bPlaySoundFirstPerson = (pA == Level().CurrentViewEntity());

	LPCSTR disabled_names	= pSettings->r_string(cNameSect(),"disabled_maps");
	LPCSTR curr_map			= *Level().name();
	u32 cnt					= _GetItemCount(disabled_names);
	bool b_allow			= true;
	string512				tmp;
	for(u32 i=0; i<cnt;++i){
		_GetItem(disabled_names, i, tmp);
		if(0==stricmp(tmp, curr_map)){
			b_allow = false;
			break;
		}
	}

//	CCustomOutfit* pCO=pA->GetOutfit();
	if(/*pCO&&pCO->*/!!m_NightVisionSect/*.size()*/&&!b_allow){
		HUD_SOUND::PlaySound(m_NightVisionBrokenSnd, pA->Position(), pA, bPlaySoundFirstPerson);
		return;
	}

	if(m_bNightVisionOn){
		CEffectorPP* pp = pA->Cameras().GetPPEffector((EEffectorPPType)effNightvision);
		if(!pp){
			if (/*pCO&&pCO->*/!!m_NightVisionSect/*.size()*/)
			{
				AddEffector(pA,effNightvision, /*pCO->*/m_NightVisionSect);
				HUD_SOUND::PlaySound(m_NightVisionOnSnd, pA->Position(), pA, bPlaySoundFirstPerson);
				HUD_SOUND::PlaySound(m_NightVisionIdleSnd, pA->Position(), pA, bPlaySoundFirstPerson, true);
			}
		}
	}else{
 		CEffectorPP* pp = pA->Cameras().GetPPEffector((EEffectorPPType)effNightvision);
		if(pp){
			pp->Stop			(1.0f);
			HUD_SOUND::PlaySound(m_NightVisionOffSnd, pA->Position(), pA, bPlaySoundFirstPerson);
			HUD_SOUND::StopSound(m_NightVisionIdleSnd);
		}
	}
}


void CTorch::UpdateSwitchNightVision   ()
{
	if(!m_bNightVisionEnabled) return;
	if (OnClient()) return;

	/*if(m_bNightVisionOn)
	{
		m_NightVisionChargeTime			-= Device.fTimeDelta;

		if(m_NightVisionChargeTime<0.f)
			SwitchNightVision(false);
	}
	else
	{
		m_NightVisionChargeTime			+= Device.fTimeDelta;
		clamp(m_NightVisionChargeTime, 0.f, m_NightVisionRechargeTime);
	}*/

	auto* pA = smart_cast<CActor*>(H_Parent());
	if (pA && m_bNightVisionOn && !pA->Cameras().GetPPEffector((EEffectorPPType)effNightvision))
		SwitchNightVision(true);
}


void CTorch::Switch()
{
	if (OnClient() || !m_bTorchLightEnabled) return;
	bool bActive			= !m_switched_on;
	Switch					(bActive);
}

void CTorch::Switch	(bool light_on)
{
	if (!m_bTorchLightEnabled) return;

	m_switched_on			= light_on;
	if (can_use_dynamic_lights())
	{
		light_render->set_active(light_on);
		
/*		CActor *pA = smart_cast<CActor *>(H_Parent());
		if(!pA)light_omni->set_active(light_on);*/
		light_omni->set_active(light_on);
	}
	glow_render->set_active					(light_on);

	if (*light_trace_bone) 
	{
		CKinematics* pVisual				= smart_cast<CKinematics*>(Visual()); VERIFY(pVisual);
		u16 bi								= pVisual->LL_BoneID(light_trace_bone);

		pVisual->LL_SetBoneVisible			(bi,	light_on,	TRUE);
		pVisual->CalculateBones				(TRUE);

//		pVisual->LL_SetBoneVisible			(bi,	light_on,	TRUE); //hack
	}

	/************************************************** added by Ray Twitty (aka Shadows) START **************************************************/
	// Колбек на переключение фонаря
	if(b_lastState == light_on) return;
	b_lastState = light_on;
	callback(GameObject::eSwitchTorch)(light_on);
	// вызываем событие также и для актора (при использовании его фонарика)
	CActor *pA = smart_cast<CActor *>(H_Parent());
	if ((pA) ? true : false)
	{
		Actor()->callback(GameObject::eSwitchTorch)(light_on);
	}
	/*************************************************** added by Ray Twitty (aka Shadows) END ***************************************************/
}

BOOL CTorch::net_Spawn(CSE_Abstract* DC) 
{
	CSE_Abstract			*e	= (CSE_Abstract*)(DC);
	CSE_ALifeItemTorch		*torch	= smart_cast<CSE_ALifeItemTorch*>(e);
	R_ASSERT				(torch);
	cNameVisual_set			(torch->get_visual());

	R_ASSERT				(!CFORM());
	R_ASSERT				(smart_cast<CKinematics*>(Visual()));
	collidable.model		= xr_new<CCF_Skeleton>	(this);

	if (!inherited::net_Spawn(DC))
		return				(FALSE);
	
	bool b_r2				= !!psDeviceFlags.test(rsR2);

	CKinematics* K			= smart_cast<CKinematics*>(Visual());
	CInifile* pUserData		= K->LL_UserData(); 
	R_ASSERT3				(pUserData,"Empty Torch user data!",torch->get_visual());
	lanim					= LALib.FindItem(pUserData->r_string("torch_definition","color_animator"));
	guid_bone				= K->LL_BoneID	(pUserData->r_string("torch_definition","guide_bone"));	VERIFY(guid_bone!=BI_NONE);

//	Fcolor clr				= pUserData->r_fcolor				("torch_definition",(b_r2)?"color_r2":"color");
	m_color					= pUserData->r_fcolor				( "torch_definition", b_r2 ? "color_r2" : "color" );
	fBrightness				= m_color.intensity();
	float range				= pUserData->r_float				("torch_definition",(b_r2)?"range_r2":"range");
	light_render->set_color	(m_color);
	light_render->set_range	(range);

	Fcolor clr_o			= pUserData->r_fcolor				("torch_definition",(b_r2)?"omni_color_r2":"omni_color");
	float range_o			= pUserData->r_float				("torch_definition",(b_r2)?"omni_range_r2":"omni_range");
	light_omni->set_color	(clr_o);
	light_omni->set_range	(range_o);

	light_render->set_cone	(deg2rad(pUserData->r_float			("torch_definition","spot_angle")));
	light_render->set_texture(pUserData->r_string				("torch_definition","spot_texture"));

	glow_render->set_texture(pUserData->r_string				("torch_definition","glow_texture"));
	glow_render->set_color	(m_color);
	glow_render->set_radius	(pUserData->r_float					("torch_definition","glow_radius"));

	//включить/выключить фонарик
	if (m_bTorchLightEnabled)
	{
		Switch(torch->m_active);
		VERIFY(!torch->m_active || (torch->ID_Parent != 0xffff));
	}
	
//	SwitchNightVision		(false);
	if (m_bNightVisionEnabled)
		m_bNightVisionOn = torch->m_nightvision_active;

	//m_delta_h				= PI_DIV_2-atan((range*0.5f)/_abs(TORCH_OFFSET.x));
	calc_m_delta_h			(range);

	return					(TRUE);
}

void CTorch::net_Destroy() 
{
	Switch					(false);
	SwitchNightVision		(false);

	inherited::net_Destroy	();
}

void CTorch::OnH_A_Chield() 
{
	inherited::OnH_A_Chield			();
	m_focus.set						(Position());
}

void CTorch::OnH_B_Independent	(bool just_before_destroy) 
{
	inherited::OnH_B_Independent	(just_before_destroy);
	time2hide						= TIME_2_HIDE;

	Switch						(false);
	SwitchNightVision			(false);

	HUD_SOUND::StopSound		(m_NightVisionOnSnd);
	HUD_SOUND::StopSound		(m_NightVisionOffSnd);
	HUD_SOUND::StopSound		(m_NightVisionIdleSnd);

	//m_NightVisionChargeTime		= m_NightVisionRechargeTime;
}

void CTorch::UpdateCL() 
{
	inherited::UpdateCL			();
	
	UpdateSwitchNightVision		();

	if (!m_switched_on || !m_bTorchLightEnabled)			return;

	CBoneInstance			&BI = smart_cast<CKinematics*>(Visual())->LL_GetBoneInstance(guid_bone);
	Fmatrix					M;

	if (H_Parent()) 
	{
		CActor*			actor = smart_cast<CActor*>(H_Parent());
		if (actor)		smart_cast<CKinematics*>(H_Parent()->Visual())->CalculateBones_Invalidate	();

		if (H_Parent()->XFORM().c.distance_to_sqr(Device.vCameraPosition)<_sqr(OPTIMIZATION_DISTANCE) || GameID() != GAME_SINGLE) {
			// near camera
			smart_cast<CKinematics*>(H_Parent()->Visual())->CalculateBones	();
			M.mul_43				(XFORM(),BI.mTransform);
		} else {
			// approximately the same
			M		= H_Parent()->XFORM		();
			H_Parent()->Center				(M.c);
			M.c.y	+= H_Parent()->Radius	()*2.f/3.f;
		}

		if (actor) 
		{
			if (actor->active_cam() == eacLookAt)
			{
				m_prev_hp.x = angle_inertion_var(m_prev_hp.x, -actor->cam_Active()->yaw, TORCH_INERTION_SPEED_MIN, TORCH_INERTION_SPEED_MAX, TORCH_INERTION_CLAMP, Device.fTimeDelta);
				m_prev_hp.y = angle_inertion_var(m_prev_hp.y, -actor->cam_Active()->pitch, TORCH_INERTION_SPEED_MIN, TORCH_INERTION_SPEED_MAX, TORCH_INERTION_CLAMP, Device.fTimeDelta);
			}
			else
			{
				m_prev_hp.x = angle_inertion_var(m_prev_hp.x, -actor->cam_FirstEye()->yaw, TORCH_INERTION_SPEED_MIN, TORCH_INERTION_SPEED_MAX, TORCH_INERTION_CLAMP, Device.fTimeDelta);
				m_prev_hp.y = angle_inertion_var(m_prev_hp.y, -actor->cam_FirstEye()->pitch, TORCH_INERTION_SPEED_MIN, TORCH_INERTION_SPEED_MAX, TORCH_INERTION_CLAMP, Device.fTimeDelta);
			}

			Fvector			dir,right,up;	
			dir.setHP		(m_prev_hp.x+m_delta_h,m_prev_hp.y);
			Fvector::generate_orthonormal_basis_normalized(dir,up,right);


			if (true)
			{
				Fvector offset				= M.c; 
				offset.mad					(M.i,TORCH_OFFSET.x);
				offset.mad					(M.j,TORCH_OFFSET.y);
				offset.mad					(M.k,TORCH_OFFSET.z);
				light_render->set_position	(offset);

				if (true /*false*/)
				{
					offset						= M.c; 
					offset.mad					(M.i,OMNI_OFFSET.x);
					offset.mad					(M.j,OMNI_OFFSET.y);
					offset.mad					(M.k,OMNI_OFFSET.z);
					light_omni->set_position	(offset);
				}
			}//if (true)
			glow_render->set_position	(M.c);

			if (true)
			{
				light_render->set_rotation	(dir, right);
				
				if (true /*false*/)
				{
					light_omni->set_rotation	(dir, right);
				}
			}//if (true)
			glow_render->set_direction	(dir);

		}// if(actor)
		else 
		{
			if (can_use_dynamic_lights()) 
			{
				light_render->set_position	(M.c);
				light_render->set_rotation	(M.k,M.i);

				Fvector offset				= M.c; 
				offset.mad					(M.i,OMNI_OFFSET.x);
				offset.mad					(M.j,OMNI_OFFSET.y);
				offset.mad					(M.k,OMNI_OFFSET.z);
				light_omni->set_position	(M.c);
				light_omni->set_rotation	(M.k,M.i);
			}//if (can_use_dynamic_lights()) 

			glow_render->set_position	(M.c);
			glow_render->set_direction	(M.k);
		}
	}//if(HParent())
	else 
	{
		if (getVisible() && m_pPhysicsShell) 
		{
			M.mul						(XFORM(),BI.mTransform);

			//. what should we do in case when 
			// light_render is not active at this moment,
			// but m_switched_on is true?
//			light_render->set_rotation	(M.k,M.i);
//			light_render->set_position	(M.c);
//			glow_render->set_position	(M.c);
//			glow_render->set_direction	(M.k);
//
//			time2hide					-= Device.fTimeDelta;
//			if (time2hide<0)
			{
				m_switched_on			= false;
				light_render->set_active(false);
				light_omni->set_active(false);
				glow_render->set_active	(false);
			}
		}//if (getVisible() && m_pPhysicsShell)  
	}

	if (!m_switched_on)					return;

	// calc color animator
	if (!lanim)							return;

	int						frame;
	// возвращает в формате BGR
	u32 clr					= lanim->CalculateBGR(Device.fTimeGlobal,frame); 

	Fcolor					fclr;
/*	fclr.set				((float)color_get_B(clr),(float)color_get_G(clr),(float)color_get_R(clr),1.f);
	fclr.mul_rgb			(fBrightness/255.f);*/
	fclr.set				((float)color_get_B(clr) / 255.f, (float)color_get_G(clr) / 255.f, (float)color_get_R(clr) / 255.f, 1.f);
	fclr.mul_rgb			(fBrightness);
	if (can_use_dynamic_lights())
	{
		light_render->set_color	(fclr);
		light_omni->set_color	(fclr);
	}
	glow_render->set_color		(fclr);
}


void CTorch::create_physic_shell()
{
	CPhysicsShellHolder::create_physic_shell();
}

void CTorch::activate_physic_shell()
{
	CPhysicsShellHolder::activate_physic_shell();
}

void CTorch::setup_physic_shell	()
{
	CPhysicsShellHolder::setup_physic_shell();
}

void CTorch::net_Export			(NET_Packet& P)
{
	inherited::net_Export		(P);
//	P.w_u8						(m_switched_on ? 1 : 0);


	BYTE F = 0;
	F |= (m_switched_on ? eTorchActive : 0);
	F |= (m_bNightVisionOn ? eNightVisionActive : 0);
	const CActor *pA = smart_cast<const CActor *>(H_Parent());
	if (pA)
	{
		if (pA->attached(this))
			F |= eAttached;
	}
	P.w_u8(F);
//	Msg("CTorch::net_export - NV[%d]", m_bNightVisionOn);
}

void CTorch::net_Import			(NET_Packet& P)
{
	inherited::net_Import		(P);
	
	BYTE F = P.r_u8();
	bool new_m_switched_on				= !!(F & eTorchActive);
	bool new_m_bNightVisionOn			= !!(F & eNightVisionActive);

	if (new_m_switched_on != m_switched_on)			Switch						(new_m_switched_on);
	if (new_m_bNightVisionOn != m_bNightVisionOn)	
	{
//		Msg("CTorch::net_Import - NV[%d]", new_m_bNightVisionOn);

		SwitchNightVision			(new_m_bNightVisionOn);
	}
}

bool  CTorch::can_be_attached		() const
{
//	if( !inherited::can_be_attached() ) return false;

	const CActor *pA = smart_cast<const CActor *>(H_Parent());
	if (pA) 
	{
		u32 slot = GetSlot();
		PIItem item = pA->inventory().m_slots[slot].m_pIItem;
		if( (const CTorch*)smart_cast<CTorch*>(item) == this )
			return true;
		else
			return false;
	}
	return true;
}
void CTorch::afterAttach			()
{
	inherited::afterAttach	();

	if (m_UINightVisionMask)
		xr_delete(m_UINightVisionMask);
	if (m_bNightVisionEnabled && !!m_NightVisionTexture)
	{
		m_UINightVisionMask = xr_new<CUIStaticItem>();
		m_UINightVisionMask->Init(m_NightVisionTexture.c_str(), "hud\\scopes", 0, 0, alNone);	// KD: special shader that account screen resolution
	}
}
void CTorch::afterDetach			()
{
	inherited::afterDetach	();
	Switch					(false);

	SwitchNightVision		(false);
	if (m_UINightVisionMask)
		xr_delete(m_UINightVisionMask);
}
void CTorch::renderable_Render()
{
	inherited::renderable_Render();
}

void CTorch::calc_m_delta_h(float range) 
{
	m_delta_h = PI_DIV_2 - atan((range * 0.5f) / _abs(TORCH_OFFSET.x));
}

void CTorch::OnDrawUI()
{
	if (m_bNightVisionEnabled && m_bNightVisionOn && !!m_NightVisionTexture)
	{
		m_UINightVisionMask->SetPos(0, 0);
		m_UINightVisionMask->SetRect(0, 0, UI_BASE_WIDTH, UI_BASE_HEIGHT);
		m_UINightVisionMask->Render();
	}
}