#include "lbound.h"


// get distance behind all planes and return the smallest..
// if inside this will be negative, if outside, positive
float LBound::GetClosestDistance(const Vector3 &pt) const
{
	assert (m_Planes.size());

	float closest = FLT_MAX;

	for (int n=0; n<m_Planes.size(); n++)
	{
		float d = m_Planes[n].distance_to(pt);

		// if in front of plane, outside the convex hull
		if (d < closest)
			closest = d;
	}

	return closest;
}


bool LBound::IsPointWithin(const Vector3 &pt) const
{
	for (int n=0; n<m_Planes.size(); n++)
	{
		float d = m_Planes[n].distance_to(pt);

		// if in front of plane, outside the convex hull
		if (d > 0.0f)
			return false;
	}

	return true;
}

