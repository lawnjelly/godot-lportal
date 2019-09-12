#pragma once

#include "lvector.h"
#include "core/math/plane.h"

class LPlanesPool
{
public:
	const static int POOL_MAX = 32;

	void Reset();

	unsigned int Request();
	void Free(unsigned int ui);

	LVector<Plane> &Get(unsigned int ui) {return m_Planes[ui];}

	LPlanesPool();
private:
	LVector<Plane> m_Planes[	POOL_MAX];
	unsigned char m_ucTaken[POOL_MAX];
	unsigned int m_uiCount;
};
