#include "lplanes_pool.h"

LPlanesPool::LPlanesPool()
{
	Reset();

	// preallocate the vectors to a reasonable size
	for (int n=0; n<POOL_MAX; n++)
	{
		m_Planes[n].resize(32);
	}
}

void LPlanesPool::Reset()
{
	for (int n=0; n<POOL_MAX; n++)
	{
		m_ucFreeList[n] = POOL_MAX - n - 1;
	}

	m_uiNumFree = POOL_MAX;
}

unsigned int LPlanesPool::Request()
{
	if (!m_uiNumFree)
		return -1;

	m_uiNumFree--;
	return m_ucFreeList[m_uiNumFree];

}

void LPlanesPool::Free(unsigned int ui)
{
	assert (ui <= POOL_MAX);
	assert (m_uiNumFree < POOL_MAX);

	m_ucFreeList[m_uiNumFree] = ui;
	m_uiNumFree++;
}
