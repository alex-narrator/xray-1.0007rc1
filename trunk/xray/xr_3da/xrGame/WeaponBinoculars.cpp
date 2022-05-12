#include "stdafx.h"
#include "WeaponBinoculars.h"

#include "xr_level_controller.h"

CWeaponBinoculars::CWeaponBinoculars() : CWeaponCustomPistol("BINOCULARS")
{
	SetSlot (APPARATUS_SLOT);
}

CWeaponBinoculars::~CWeaponBinoculars()
{
}

bool CWeaponBinoculars::Action(s32 cmd, u32 flags) 
{
	switch(cmd) 
	{
	case kWPN_FIRE : 
		return inherited::Action(kWPN_ZOOM, flags);
	}

	return inherited::Action(cmd, flags);
}

void CWeaponBinoculars::GetBriefInfo(xr_string& str_name, xr_string& icon_sect_name, xr_string& str_count)
{
	str_name		= NameShort();
	str_count		= "";
	icon_sect_name	= *cNameSect();
}
