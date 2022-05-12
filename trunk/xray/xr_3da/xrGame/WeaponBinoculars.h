#pragma once

#include "WeaponCustomPistol.h"
#include "script_export_space.h"

class CUIFrameWindow;
class CUIStatic;
//class CBinocularsVision;

class CWeaponBinoculars: public CWeaponCustomPistol
{
private:
	typedef CWeaponCustomPistol inherited;

public:
					CWeaponBinoculars	(); 
	virtual			~CWeaponBinoculars	();

	virtual bool	Action				(s32 cmd, u32 flags);
	virtual bool	use_crosshair		()	const {return false;}
	virtual void	GetBriefInfo		(xr_string& str_name, xr_string& icon_sect_name, xr_string& str_count);

	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(CWeaponBinoculars)
#undef script_type_list
#define script_type_list save_type_list(CWeaponBinoculars)
