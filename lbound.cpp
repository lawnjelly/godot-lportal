//	Copyright (c) 2019 Lawnjelly

//	Permission is hereby granted, free of charge, to any person obtaining a copy
//	of this software and associated documentation files (the "Software"), to deal
//	in the Software without restriction, including without limitation the rights
//	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//	copies of the Software, and to permit persons to whom the Software is
//	furnished to do so, subject to the following conditions:

//	The above copyright notice and this permission notice shall be included in all
//	copies or substantial portions of the Software.

//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//	SOFTWARE.

/**
	@author lawnjelly <lawnjelly@gmail.com>
*/

#include "lbound.h"


// get distance behind all planes and return the smallest..
// if inside this will be negative, if outside, positive
float LBound::GetSmallestPenetrationDistance(const Vector3 &pt) const
{
	assert (m_Planes.size());

	float closest = -FLT_MAX;

	for (int n=0; n<m_Planes.size(); n++)
	{
		float d = m_Planes[n].distance_to(pt);

		// if in front of plane, outside the convex hull
		if (d > 0.1f) // some large epsilon .. we would rather classify in a nearby cell than not in any cell
		{
			// outside the convex hull, don't use
			return FLT_MAX;
		}

		if (d > closest)
			closest = d;
	}

	return closest;
}


bool LBound::IsPointWithin(const Vector3 &pt, float epsilon) const
{
	for (int n=0; n<m_Planes.size(); n++)
	{
		float d = m_Planes[n].distance_to(pt);

		// if in front of plane, outside the convex hull
		if (d > epsilon)
			return false;
	}

	return true;
}

