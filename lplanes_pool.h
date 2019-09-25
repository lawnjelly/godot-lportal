#pragma once

#include "lvector.h"
#include "core/math/plane.h"

// The recursive visibility function needs to allocate lists of planes each time a room is traversed.
// Instead of doing this allocation on the fly we will use a pool which should be much faster and nearer
// constant time.

// Note this simple pool isn't super optimal but should be fine for now.
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

	uint8_t m_ucFreeList[POOL_MAX];
	uint32_t m_uiNumFree;
};
