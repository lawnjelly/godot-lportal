#include "lplanes_pool.h"

LPlanesPool::LPlanesPool()
{
	Reset();

	// preallocate the vectors to a reasonable size
	for (int n=0; n<POOL_MAX; n++)
	{
		m_Planes[n].reserve(32);
	}
}

void LPlanesPool::Reset()
{
	memset(m_ucTaken, 0, sizeof (	m_ucTaken));
	m_uiCount = 0;
}

unsigned int LPlanesPool::Request()
{
	// can't do, pool run out
	if (m_uiCount >= POOL_MAX)
		return -1;

	for (unsigned int n=0; n<POOL_MAX; n++)
	{
		if (m_ucTaken[n] == 0)
		{
			m_ucTaken[n] = 255;
			m_uiCount++;
			return n;
		}
	}

	assert (0);
}

void LPlanesPool::Free(unsigned int ui)
{
	assert (ui <= POOL_MAX);
	assert (m_ucTaken[ui]);
	assert (m_uiCount);

	m_ucTaken[ui] = 0;
	m_uiCount--;
}
