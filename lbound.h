#pragma once

#include "lvector.h"

// optional convex hull around rooms, to make it easier to determine which room a point is within
class LBound
{
public:
	bool IsPointWithin(const Vector3 &pt) const;

	// get distance behind all planes and return the smallest..
	// if inside this will be negative, if outside, positive
	float GetClosestDistance(const Vector3 &pt) const;

	// the bound is optional .. not all rooms have a bound
	bool IsActive() const {return m_Planes.size() != 0;}

	LVector<Plane> m_Planes;
};
