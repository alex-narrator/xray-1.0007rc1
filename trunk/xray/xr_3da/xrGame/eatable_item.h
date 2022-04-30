#pragma once

#include "inventory_item.h"
#include "base_client_classes.h"

class CPhysicItem;
class CEntityAlive;


class CEatableItem : public CInventoryItem {	
	friend class CEatableItemScript;
private:
	typedef CInventoryItem	inherited;

private:
	CPhysicItem		*m_physic_item;

public:
							CEatableItem				();
	virtual					~CEatableItem				();
	virtual	DLL_Pure*		_construct					();
	virtual CEatableItem	*cast_eatable_item			()	{return this;}

	virtual void			Load						(LPCSTR section);
	virtual bool			Useful						() const;

	virtual BOOL			net_Spawn					(CSE_Abstract* DC);

	virtual void			OnH_B_Independent			(bool just_before_destroy);
	virtual	void			UseBy						(CEntityAlive* npc);
			bool			Empty						()	const				{return m_iPortionsNum==0;};
protected:	
	//влияние при поедании вещи на параметры игрока
	float					m_fHealthInfluence;
	float					m_fPowerInfluence;
	float					m_fSatietyInfluence;
	float					m_fRadiationInfluence;
	float					m_fPsyHealthInfluence;
	float					m_fMaxPowerUpInfluence;
	//заживление ран на кол-во процентов
	float					m_fWoundsHealPerc;
	//алкоголь
	float					m_fAlcoholInfluence;
	//яка доля власної радіоактивності предмета буде передана гравцеві при вживанні
	float					m_fSelfRadiationInfluence;

	//количество порций еды, 
	//-1 - порция одна и больше не бывает (чтоб не выводить надпись в меню)
	int						m_iPortionsNum;
	int						m_iStartPortionsNum;

	bool					m_bUsePortionVolume;

	// Для поддержки серверного класса. Real Wolf. 09.09.2014.
	virtual void			net_Export(NET_Packet&);
	virtual void			net_Import(NET_Packet&);
public:
	int						GetPortionsNum() const	{ return m_iPortionsNum; }
#ifdef EAT_PORTIONS_INFLUENCE
	int    GetStartPortionsNum() const { return m_iStartPortionsNum; };  // <--- // Не используется, но может пригодится. //
	float  GetOnePortionWeight();                    // <--- //
	float  GetOnePortionVolume();                    // <--- //
	u32    GetOnePortionCost();                      // <--- //
#endif
};

