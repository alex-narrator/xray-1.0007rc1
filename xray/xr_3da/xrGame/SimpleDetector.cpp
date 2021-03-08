#include "stdafx.h"
#include "simpledetector.h"

CSimpleDetector::CSimpleDetector(void)
{
	m_weight = .5f;
	//m_belt = true;
	SetSlot(DETECTOR_SLOT);
}

CSimpleDetector::~CSimpleDetector(void)
{
}
