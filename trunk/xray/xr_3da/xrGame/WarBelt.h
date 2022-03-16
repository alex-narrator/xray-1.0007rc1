#pragma once
#include "inventory_item_object.h"
class CWarBelt :
	public CInventoryItemObject
{
private:
	typedef	CInventoryItemObject inherited;
public:
								CWarBelt();
	virtual						~CWarBelt();

	virtual void				Load(LPCSTR section);

protected:
	u32							m_iMaxBeltWidth;
	u32							m_iMaxBeltHeight;
	bool						m_bDropPouch;

public:
	u32							GetBeltWidth			() const	{ return m_iMaxBeltWidth; }
	u32							GetBeltHeight			() const	{ return m_iMaxBeltHeight; }
	bool						HasDropPouch			() const	{ return m_bDropPouch; }

	virtual void				OnMoveToSlot			(EItemPlace previous_place);
	virtual void				OnMoveToRuck			(EItemPlace previous_place);
	virtual void				OnMoveOut				(EItemPlace previous_place);
};

