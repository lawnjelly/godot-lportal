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

#include "lmain_camera.h"
#include "lvector.h"
#include "lroom.h"
#include "ldebug.h"
#include "lroom_manager.h"
#include "ldob.h"

#include "scene/3d/camera.h"
#include "core/math/camera_matrix.h"
#include "core/math/plane.h"
#include "core/math/vector3.h"

const char * LMainCamera::m_szPlanes[] = {"NEAR", "FAR", "LEFT", "TOP", "RIGHT", "BOTTOM", };
const char * LMainCamera::m_szPoints[] = {"FAR_LEFT_TOP", "FAR_LEFT_BOTTOM", "FAR_RIGHT_TOP", "FAR_RIGHT_BOTTOM",
"NEAR_LEFT_TOP", "NEAR_LEFT_BOTTOM", "NEAR_RIGHT_TOP", "NEAR_RIGHT_BOTTOM",};


uint8_t LMainCamera::m_LUT_EntrySizes[64] = {0, 4, 4, 0, 4, 6, 6, 8, 4, 6, 6, 8, 6, 6, 6, 6, 4, 6, 6, 8, 0, 8, 8, 0, 6, 6, 6, 6, 8, 6, 6, 4, 4, 6, 6, 8, 6, 6, 6, 6, 0, 8, 8, 0, 8, 6, 6, 4, 6, 6, 6, 6, 8, 6, 6, 4, 8, 6, 6, 4, 0, 4, 4, 0, };


#ifdef LMAINCAMERA_CALC_LUT


void LMainCamera::CreateLUT()
{
	// each pair of planes that are opposite can have an edge
	for (int p0 = 0; p0<P_TOTAL; p0++)
	{
		// for each neighbour of the plane
		ePlane neighs[4];
		GetNeighbours((ePlane) p0, neighs);

		for (int n=0; n<4; n++)
		{
			int p1 = neighs[n];

			//if these are opposite we need to add the 2 points they share
			ePoint pts[2];
			GetCornersOfPlanes( (ePlane) p0, (ePlane) p1, pts );

			AddLUT(p0, p1, pts);
		}
	}

	for (int n=0; n<LUT_SIZE; n++)
	{
		CompactLUT_Entry(n);
	}

	DebugPrintLUT();
	DebugPrintLUT_AsTable();

}


// we can pre-create the entire LUT and store it hard coded as a static inside the executable!
// it is only small in size, 64 entries with max 8 bytes per entry
void LMainCamera::DebugPrintLUT_AsTable()
{
	print_line("\nLIGHT VOLUME TABLE BEGIN\n");

	String sz = "{";
	for (int n=0; n<LUT_SIZE; n++)
	{
		const LVector<uint8_t> &entry = m_LUT[n];

		sz += itos (entry.size()) + ", ";
	}
	sz += "}";
	print_line(sz);

	for (int n=0; n<LUT_SIZE; n++)
	{
		const LVector<uint8_t> &entry = m_LUT[n];

		String sz = "{";

		// first is the number of points in the entry
		int s = entry.size();
		//assert (s <= 7);

//		sz += itos(s) + ", ";

		// in the very special (and rare case of 8 points, we can infer the 8th point at runtime)
		for (int p=0; p<7; p++)
		{
			if (p < s)
				sz += itos(entry[p]);
			else
				sz += "0"; // just a spacer

			sz += ", ";
		}

		sz += "},";
		print_line(sz);
	}

	print_line("\nLIGHT VOLUME TABLE END\n");
}


void LMainCamera::DebugPrintLUT()
{
	for (int n=0; n<LUT_SIZE; n++)
	{
		String sz;
		sz = "LUT" + itos(n) + ":\t";

		sz += String_PlaneBF(n);
		print_line(sz);

		const LVector<uint8_t> &entry = m_LUT[n];

		//sz += DebugStringLUT_Entry(entry);
		sz = "\t" + String_LUTEntry(entry);

//		for (int i=0; i<entry.size(); i++)
//		{
//			int ePt = entry[i];
//			sz += itos(ePt) + ", ";
//		}

		//LPRINT(2, sz);
		print_line(sz);
	}
}


String LMainCamera::String_LUTEntry(const LVector<uint8_t> &entry)
{
	String sz;

	for (int n=0; n<entry.size(); n++)
	{
		sz += m_szPoints[entry[n]];
		sz += ", ";
	}

	return sz;
}


String LMainCamera::DebugStringLUT_Entry(const LVector<uint8_t> &entry)
{
	String sz;
//	sz = "LUT" + itos(n) + ":\t";

//	const LVector<uint8_t> &entry = m_LUT[n];

	for (int i=0; i<entry.size(); i++)
	{
		int ePt = entry[i];
		sz += itos(ePt) + ", ";
	}

	//LPRINT(2, sz);
//	print_line(sz);
	return sz;
}



void LMainCamera::AddLUT(int p0, int p1, ePoint pts[2])
{
	unsigned int bit0 = 1 << p0;
	unsigned int bit1 = 1 << p1;

	// all entries of the LUT that have plane 0 set and plane 1 not set
	for (unsigned int n=0; n<64; n++)
	{
		// if bit0 not set
		if (!(n & bit0))
			continue;

		// if bit1 set
		if (n & bit1)
			continue;

		// meets criteria
		AddLUT_Entry(n, pts);
	}
}

void LMainCamera::AddLUT_Entry(unsigned int n, ePoint pts[2])
{
	assert (n < LUT_SIZE);
	LVector<uint8_t> &entry = m_LUT[n];

	const ePoint &a = pts[0];
	const ePoint &b = pts[1];

	entry.push_back(pts[0]);
	entry.push_back(pts[1]);
}

void LMainCamera::CompactLUT_Entry(int n)
{
	assert (n < LUT_SIZE);
	LVector<uint8_t> &entry = m_LUT[n];

	int nPairs = entry.size() / 2;

	if (nPairs == 0)
		return;

	LVector<uint8_t> temp;
	temp.resize(16); // 8 max?

	String sz;
	sz = "Compact LUT" + itos(n) + ":\t";
	sz += DebugStringLUT_Entry(entry);
	print_line(sz);

	// add first pair
	temp.push_back(entry[0]);
	temp.push_back(entry[1]);
	unsigned int BFpairs = 1;

	sz = DebugStringLUT_Entry(temp) + " -> ";
	print_line(sz);

	// attempt to add a pair each time
	for (int done=1; done<nPairs; done++)
	{
		sz = "done " + itos(done) + ": ";
		// find a free pair
		for (int p=1; p<nPairs; p++)
		{
			unsigned int bit = 1 << p;
			// is it done already?
			if (BFpairs & bit)
				continue;

			// there must be at least 1 free pair
			// attempt to add
			int a = entry[p * 2];
			int b = entry[(p * 2)+1];

			sz += "[" + itos(a) + "-" + itos(b) + "], ";

			int foundA = temp.find(a);
			int foundB = temp.find(b);

			// special case, if they are both already in the list, no need to add
			// as this is a link from the tail to the head of the list
			if ((foundA != -1) && (foundB != -1))
			{
				sz += "foundAB link " + itos(foundA) + ", " + itos(foundB) + " ";
				BFpairs |= bit;
				goto found;
			}

			// find a
			if (foundA != -1)
			{
				sz += "foundA " + itos(foundA) + " ";
				temp.insert(foundA+1, b);
				BFpairs |= bit;
				goto found;
			}

			// find b
			if (foundB != -1)
			{
				sz += "foundB " + itos(foundB) + " ";
				temp.insert(foundB, a);
				BFpairs |= bit;
				goto found;
			}

		} // check each pair for adding

		// if we got here before finding a link, the whole set of planes is INVALID
		// e.g. far and near plane only, does not create continuous sillouhette of edges
		print_line("\tINVALID");
		entry.clear();
		return;

		found:;
		print_line(sz);
		sz = "\ttemp now : " + DebugStringLUT_Entry(temp);
		print_line(sz);

	}

	// temp should now be the sorted entry .. delete the old one and replace by temp
	entry.clear();
	entry.copy_from(temp);
}





void LMainCamera::GetNeighbours(ePlane p, ePlane neigh_planes[4]) const
{
	// table of neighbouring planes to each
	static const ePlane NeighTable[P_TOTAL][4] = {
	{	// LSM_FP_NEAR
	P_LEFT,
	P_RIGHT,
	P_TOP,
	P_BOTTOM
	},
	{	// LSM_FP_FAR
	P_LEFT,
	P_RIGHT,
	P_TOP,
	P_BOTTOM
	},
	{	// LSM_FP_LEFT
	P_TOP,
	P_BOTTOM,
	P_NEAR,
	P_FAR
	},
	{	// LSM_FP_TOP
	P_LEFT,
	P_RIGHT,
	P_NEAR,
	P_FAR
	},
	{	// LSM_FP_RIGHT
	P_TOP,
	P_BOTTOM,
	P_NEAR,
	P_FAR
	},
	{	// LSM_FP_BOTTOM
	P_LEFT,
	P_RIGHT,
	P_NEAR,
	P_FAR
	},
	};

	for (int n=0; n<4; n++)
	{
		neigh_planes[n] = NeighTable[p][n];
	}
}


/**
* Given two planes, returns the two points shared by those planes.  The points are always
*	returned in counter-clockwise order, assuming the first input plane is facing towards
*	the viewer.
*
* \param _fpPlane0 The plane facing towards the viewer.
* \param _fpPlane1 A plane neighboring _fpPlane0.
* \param _fpRet An array of exactly two elements to be filled with the indices of the points
*	on return.
*/
void LMainCamera::GetCornersOfPlanes( ePlane _fpPlane0, ePlane _fpPlane1,
ePoint _fpRet[2] ) const
{
	static const ePoint fpTable[P_TOTAL][P_TOTAL][2] = {
	{	// LSM_FP_NEAR
	{	// LSM_FP_NEAR
	PT_NEAR_LEFT_TOP, PT_NEAR_RIGHT_TOP,		// Invalid combination.
	},
	{	// LSM_FP_FAR
	PT_FAR_RIGHT_TOP, PT_FAR_LEFT_TOP,		// Invalid combination.
	},
	{	// LSM_FP_LEFT
	PT_NEAR_LEFT_TOP, PT_NEAR_LEFT_BOTTOM,
	},
	{	// LSM_FP_TOP
	PT_NEAR_RIGHT_TOP, PT_NEAR_LEFT_TOP,
	},
	{	// LSM_FP_RIGHT
	PT_NEAR_RIGHT_BOTTOM, PT_NEAR_RIGHT_TOP,
	},
	{	// LSM_FP_BOTTOM
	PT_NEAR_LEFT_BOTTOM, PT_NEAR_RIGHT_BOTTOM,
	},
	},

	{	// LSM_FP_FAR
	{	// LSM_FP_NEAR
	PT_FAR_LEFT_TOP, PT_FAR_RIGHT_TOP,		// Invalid combination.
	},
	{	// LSM_FP_FAR
	PT_FAR_RIGHT_TOP, PT_FAR_LEFT_TOP,		// Invalid combination.
	},
	{	// LSM_FP_LEFT
	PT_FAR_LEFT_BOTTOM, PT_FAR_LEFT_TOP,
	},
	{	// LSM_FP_TOP
	PT_FAR_LEFT_TOP, PT_FAR_RIGHT_TOP,
	},
	{	// LSM_FP_RIGHT
	PT_FAR_RIGHT_TOP, PT_FAR_RIGHT_BOTTOM,
	},
	{	// LSM_FP_BOTTOM
	PT_FAR_RIGHT_BOTTOM, PT_FAR_LEFT_BOTTOM,
	},
	},

	{	// LSM_FP_LEFT
	{	// LSM_FP_NEAR
	PT_NEAR_LEFT_BOTTOM, PT_NEAR_LEFT_TOP,
	},
	{	// LSM_FP_FAR
	PT_FAR_LEFT_TOP, PT_FAR_LEFT_BOTTOM,
	},
	{	// LSM_FP_LEFT
	PT_FAR_LEFT_BOTTOM, PT_FAR_LEFT_BOTTOM,		// Invalid combination.
	},
	{	// LSM_FP_TOP
	PT_NEAR_LEFT_TOP, PT_FAR_LEFT_TOP,
	},
	{	// LSM_FP_RIGHT
	PT_FAR_LEFT_BOTTOM, PT_FAR_LEFT_BOTTOM,		// Invalid combination.
	},
	{	// LSM_FP_BOTTOM
	PT_FAR_LEFT_BOTTOM, PT_NEAR_LEFT_BOTTOM,
	},
	},

	{	// LSM_FP_TOP
	{	// LSM_FP_NEAR
	PT_NEAR_LEFT_TOP, PT_NEAR_RIGHT_TOP,
	},
	{	// LSM_FP_FAR
	PT_FAR_RIGHT_TOP, PT_FAR_LEFT_TOP,
	},
	{	// LSM_FP_LEFT
	PT_FAR_LEFT_TOP, PT_NEAR_LEFT_TOP,
	},
	{	// LSM_FP_TOP
	PT_NEAR_LEFT_TOP, PT_FAR_LEFT_TOP,		// Invalid combination.
	},
	{	// LSM_FP_RIGHT
	PT_NEAR_RIGHT_TOP, PT_FAR_RIGHT_TOP,
	},
	{	// LSM_FP_BOTTOM
	PT_FAR_LEFT_BOTTOM, PT_NEAR_LEFT_BOTTOM,	// Invalid combination.
	},
	},

	{	// LSM_FP_RIGHT
	{	// LSM_FP_NEAR
	PT_NEAR_RIGHT_TOP, PT_NEAR_RIGHT_BOTTOM,
	},
	{	// LSM_FP_FAR
	PT_FAR_RIGHT_BOTTOM, PT_FAR_RIGHT_TOP,
	},
	{	// LSM_FP_LEFT
	PT_FAR_RIGHT_BOTTOM, PT_FAR_RIGHT_BOTTOM,	// Invalid combination.
	},
	{	// LSM_FP_TOP
	PT_FAR_RIGHT_TOP, PT_NEAR_RIGHT_TOP,
	},
	{	// LSM_FP_RIGHT
	PT_FAR_RIGHT_BOTTOM, PT_FAR_RIGHT_BOTTOM,	// Invalid combination.
	},
	{	// LSM_FP_BOTTOM
	PT_NEAR_RIGHT_BOTTOM, PT_FAR_RIGHT_BOTTOM,
	},
	},

	// ==

	//	P_NEAR,
	//	P_FAR,
	//	P_LEFT,
	//	P_TOP,
	//	P_RIGHT,
	//	P_BOTTOM,


	{	// LSM_FP_BOTTOM
	{	// LSM_FP_NEAR
	PT_NEAR_RIGHT_BOTTOM, PT_NEAR_LEFT_BOTTOM,
	},
	{	// LSM_FP_FAR
	PT_FAR_LEFT_BOTTOM, PT_FAR_RIGHT_BOTTOM,
	},
	{	// LSM_FP_LEFT
	PT_NEAR_LEFT_BOTTOM, PT_FAR_LEFT_BOTTOM,
	},
	{	// LSM_FP_TOP
	PT_NEAR_LEFT_BOTTOM, PT_FAR_LEFT_BOTTOM,	// Invalid combination.
	},
	{	// LSM_FP_RIGHT
	PT_FAR_RIGHT_BOTTOM, PT_NEAR_RIGHT_BOTTOM,
	},
	{	// LSM_FP_BOTTOM
	PT_FAR_LEFT_BOTTOM, PT_NEAR_LEFT_BOTTOM,	// Invalid combination.
	},
	},

	// ==

	};
	_fpRet[0] = fpTable[_fpPlane0][_fpPlane1][0];
	_fpRet[1] = fpTable[_fpPlane0][_fpPlane1][1];
}

#endif


LMainCamera::LMainCamera()
{
#ifdef LMAINCAMERA_CALC_LUT
	CreateLUT();
#endif
}


String LMainCamera::String_PlaneBF(unsigned int BF)
{
	String sz;

	for (int n=0; n<6; n++)
	{
		unsigned int bit = 1 << n;
		if (BF & bit)
		{
			sz += String(m_szPlanes[n]) + ", ";
		}
	}

	return sz;
}

// returns false if the light is completely culled (does not enter the camera frustum)
bool LMainCamera::AddCameraLightPlanes_Directional(LRoomManager &manager, const LSource &lsource, LVector<Plane> &planes) const
{
	uint32_t lookup = 0;

	// directional light, we will use dot against the light direction to determine back facing planes
	for (int n = 0; n < 6; n++)
	{
		float dot = m_Planes[n].normal.dot(lsource.m_ptDir);
		if (dot > 0.0f)
		{
			lookup |= 1 << n;

			// add the frustum back plane to the clipping planes
			planes.push_back(m_Planes[n]);
		}
	}

	ERR_FAIL_COND_V(lookup >= LUT_SIZE, true);

	// deal with special case... if the light is INSIDE the view frustum (i.e. all planes face away)
	// then we will add the camera frustum planes to clip the light volume .. there is no need to
	// render shadow casters outside the frustum as shadows can never re-enter the frustum.

	if (lookup == 63) // should never happen with directional light?? this may be able to be removed
	{
		/*
		num_cull_planes = 0;
		for (int n = 0; n < frustum_planes.size(); n++) {
			//planes.push_back(frustum_planes[n]);
			add_cull_plane(frustum_planes[n]);
		}
		*/

		return true;
	}

	// each edge forms a plane
#ifdef LMAINCAMERA_CALC_LUT
	const LVector<uint8_t> &entry = m_LUT[lookup];

	// each edge forms a plane
	int nEdges = entry.size()-1;
#else
	uint8_t * entry = &m_LUT_Entries[lookup][0];
	int nEdges = m_LUT_EntrySizes[lookup] - 1;
#endif


	for (int e = 0; e < nEdges; e++) {
		int i0 = entry[e];
		int i1 = entry[e + 1];
		const Vector3 &pt0 = m_Points[i0];
		const Vector3 &pt1 = m_Points[i1];

		// create a third point from the light direction
		Vector3 pt2 = pt0 - lsource.m_ptDir;

		// create plane from 3 points
		Plane p(pt0, pt1, pt2);
		planes.push_back(p);
	}

	// last to 0 edge
	if (nEdges) {
		int i0 = entry[nEdges]; // last
		int i1 = entry[0]; // first

		const Vector3 &pt0 = m_Points[i0];
		const Vector3 &pt1 = m_Points[i1];

		// create a third point from the light direction
		Vector3 pt2 = pt0 - lsource.m_ptDir;

		// create plane from 3 points
		Plane p(pt0, pt1, pt2);
		planes.push_back(p);
	}

#ifdef LIGHT_CULLER_DEBUG_LOGGING
	if (is_logging()) {
		print_line("lcam.pos is " + String(lsource.pos));
	}
#endif

	return true;
}

// returns false if the light is completely culled (does not enter the camera frustum)
bool LMainCamera::AddCameraLightPlanes(LRoomManager &manager, const LSource &lsource, LVector<Plane> &planes) const
{
	// doesn't account for directional lights yet! only points

	// find which of the camera planes are facing away from the light
	assert (m_Planes.size() == 6);

	// doesn't account for directional lights yet! only points
	switch (lsource.m_eType)
	{
		case LSource::ST_SPOTLIGHT:
		case LSource::ST_OMNI:
			break;
		case LSource::ST_DIRECTIONAL:
			return AddCameraLightPlanes_Directional(manager, lsource, planes);
			break;
		default:
			return false; // not yet supported
			break;
	}

	uint32_t lookup = 0;

	// record the original number of planes in case we need to revert
	int nPlanesBefore = planes.size();

	// DIRECTIONAL LIGHT
//	for (int n=0; n<6; n++)
//	{
//		float dot = m_Planes[n].normal.dot(lcam.m_ptDir);
//		if (dot > 0.0f)
//		{
//			lookup |= 1 << n;

//			LPRINT_RUN(2, m_szPlanes[n] + " facing away from light dot " + String(Variant(dot)));
//		}
//	}

	// POINT LIGHT (spotlight, omni)
	// BRAINWAVE!! Instead of using dot product to compare light direction to plane, we can simply
	// find out which side of the plane the camera is on!! By definition this marks the point at which the plane
	// becomes invisible. This works for portals too!

	// OMNI
	if (lsource.m_eType == LSource::ST_OMNI)
	{
		for (int n=0; n<6; n++)
		{
			float dist = m_Planes[n].distance_to(lsource.m_ptPos);
			if (dist < 0.0f)
			{
				lookup |= 1 << n;

				LPRINT_RUN(2, m_szPlanes[n] + " facing away from light dist " + String(Variant(dist)));

				// add the frustum back plane to the clipping planes
				planes.push_back(m_Planes[n]);
			}
			else
			{
				// is the light out of range?
				// This is one of the tests. If the point source is more than range distance from a frustum plane, it can't
				// be seen
				if (dist >= lsource.m_fRange)
				{
					// if the light is out of range, no need to do anything else, everything will be culled
					//out_of_range = true;
					//return false;
					goto LightCulled;
				}
			}
		}
	} // if omni
	else
	{
		// SPOTLIGHTs, more complex to cull
		Vector3 ptConeEnd = lsource.m_ptPos + (lsource.m_ptDir * lsource.m_fRange);

		// this is the radius of the cone at distance 1
		float radius_at_dist_one = Math::tan(Math::deg2rad(lsource.m_fSpread));

		// the worst case radius of the cone at the end point can be calculated
		// (the radius will scale linearly with length along the cone)
		float radius_at_end = radius_at_dist_one * lsource.m_fRange;

		for (int n = 0; n < 6; n++)
		{
			float dist = m_Planes[n].distance_to(lsource.m_ptPos);
			if (dist < 0.0f)
			{
				// either the plane is backfacing or we are inside the frustum
				lookup |= 1 << n;

				// add the frustum back plane to the clipping planes
				planes.push_back(m_Planes[n]);
			}
			else
			{
				// the light is in front of the plane

				// is the light out of range?
				if (dist >= lsource.m_fRange)
				{
					//out_of_range = true;
					//return false;
					if (manager.m_bDebugFrameString)
						manager.DebugString_Add("Light culled cone start out of range .. source room" + itos(lsource.m_RoomID) + "\n");
					goto LightCulled;
				}

				// for a spotlight, we can use an extra test
				// at this point the cone start is in front of the plane...
				// if the cone end point is further than the maximum possible distance to the plane
				// we can guarantee that the cone does not cross the plane, and hence the cone
				// is outside the frustum

				// side vector (perpendicular to light direction and plane normal)
				Vector3 ptDirSide = lsource.m_ptDir.cross(m_Planes[n].normal);

				// note this could be near zero length...this is not dealt with yet!!
				Vector3 ptDirConeClosest = lsource.m_ptDir.cross(ptDirSide);

				// see https://bartwronski.com/2017/04/13/cull-that-cone/
				ptDirConeClosest.normalize();

				// push out the cone end to the closest point
				Vector3 ptConeClosest = ptConeEnd + (ptDirConeClosest * radius_at_end);

				float dist_end = m_Planes[n].distance_to(ptConeClosest);

				if (dist_end >= 0.0f)
				{
					//out_of_range = true;
					//return false;
					if (manager.m_bDebugFrameString)
					{
						manager.DebugString_Add("Light culled cone end out of range .. source room" + itos(lsource.m_RoomID) + "\n");
						manager.DebugString_Add("cone end radius " + ftos(radius_at_end) + ", dist_end " + ftos(dist_end) + "\n");
					}
					goto LightCulled;
				}
			}
		}
	}

	goto NotCulled;

	LightCulled:

	// remove any added planes .. may not be necessary
	planes.resize(nPlanesBefore);


	return false;

	NotCulled:


	LPRINT_RUN(2, "LOOKUP " + itos(lookup));

	assert (lookup < LUT_SIZE);

	// deal with special case... if the light is INSIDE the view frustum (i.e. all planes face away)
	// then we will add the camera frustum planes to clip the light volume .. there is no need to
	// render shadow casters outside the frustum as shadows can never re-enter the frustum.
	if (lookup == 63)
	{
		/*
		for (int n=0; n<m_Planes.size(); n++)
		{
			planes.push_back(m_Planes[n]);
		}
		*/

		return true;
	}


#ifdef LMAINCAMERA_CALC_LUT
	const LVector<uint8_t> &entry = m_LUT[lookup];

	// each edge forms a plane
	int nEdges = entry.size()-1;
#else
	uint8_t * entry = &m_LUT_Entries[lookup][0];
	int nEdges = m_LUT_EntrySizes[lookup] - 1;
#endif


	for (int e=0; e<nEdges; e++)
	{
		int i0 = entry[e];
		int i1 = entry[e+1];
		const Vector3 &pt0 = m_Points[i0];
		const Vector3 &pt1 = m_Points[i1];

		// create plane from 3 points
		Plane p(pt0, pt1, lsource.m_ptPos);
		planes.push_back(p); // pushing by value, urg!!
	}
	
	// last to 0 edge, NYI!!!
	if (nEdges)
	{
		int i0 = entry[nEdges]; // last
		int i1 = entry[0]; // first

		const Vector3 &pt0 = m_Points[i0];
		const Vector3 &pt1 = m_Points[i1];

		// create plane from 3 points
		Plane p(pt0, pt1, lsource.m_ptPos);
		planes.push_back(p); // pushing by value, urg!!
	}
	
	

	if (manager.m_bDebugLightVolumes)
	{
		for (int e=0; e<nEdges; e++)
		{
			int i0 = entry[e];
			const Vector3 &pt0 = m_Points[i0];

			manager.m_DebugLightVolumes.push_back(lsource.m_ptPos);
			manager.m_DebugLightVolumes.push_back(pt0);

			//print_line(String(pt0));
		}

		for (int e=0; e<nEdges+1; e++)
		{
			int i = entry[e];
			LPRINT_RUN(2, "\t" + m_szPoints[i]);
		}
	}

	return true;
}


bool LMainCamera::Prepare(LRoomManager &manager, Camera * pCam)
{
	m_Planes.copy_from(pCam->get_frustum());

	if (m_Points.size() != 8)
		m_Points.resize(8);

//	Transform p_transform = pCam->get_global_transform();

	//	Vector<Plane> planes = get_projection_planes(Transform());
	const CameraMatrix::Planes intersections[8][3] = {
	{ CameraMatrix::PLANE_FAR, CameraMatrix::PLANE_LEFT, CameraMatrix::PLANE_TOP },
	{ CameraMatrix::PLANE_FAR, CameraMatrix::PLANE_LEFT, CameraMatrix::PLANE_BOTTOM },
	{ CameraMatrix::PLANE_FAR, CameraMatrix::PLANE_RIGHT, CameraMatrix::PLANE_TOP },
	{ CameraMatrix::PLANE_FAR, CameraMatrix::PLANE_RIGHT, CameraMatrix::PLANE_BOTTOM },
	{ CameraMatrix::PLANE_NEAR, CameraMatrix::PLANE_LEFT, CameraMatrix::PLANE_TOP },
	{ CameraMatrix::PLANE_NEAR, CameraMatrix::PLANE_LEFT, CameraMatrix::PLANE_BOTTOM },
	{ CameraMatrix::PLANE_NEAR, CameraMatrix::PLANE_RIGHT, CameraMatrix::PLANE_TOP },
	{ CameraMatrix::PLANE_NEAR, CameraMatrix::PLANE_RIGHT, CameraMatrix::PLANE_BOTTOM },
	};

	m_ptCentre.zero();

	for (int i = 0; i < 8; i++) {

		Vector3 point;
		bool res = m_Planes[intersections[i][0]].intersect_3(m_Planes[intersections[i][1]], m_Planes[intersections[i][2]], &point);
		ERR_FAIL_COND_V(!res, false);

		//m_Points[i] = p_transform.xform(point);
		m_Points[i] = point;

		m_ptCentre += point;
//		print_line("point " + itos(i) + " " + String(point) + " -> " + String(m_Points[i]));
	}

	m_ptCentre *= 1.0f / 8.0f;

#define PUSH_PT(a) manager.m_DebugFrustums.push_back(m_Points[a])

	if (manager.m_bDebugFrustums)
	{
		PUSH_PT(PT_NEAR_LEFT_TOP);
		PUSH_PT(PT_FAR_LEFT_TOP);
		PUSH_PT(PT_NEAR_RIGHT_TOP);
		PUSH_PT(PT_FAR_RIGHT_TOP);
		PUSH_PT(PT_NEAR_LEFT_BOTTOM);
		PUSH_PT(PT_FAR_LEFT_BOTTOM);
		PUSH_PT(PT_NEAR_RIGHT_BOTTOM);
		PUSH_PT(PT_FAR_RIGHT_BOTTOM);
	}


	return true;
}



uint8_t LMainCamera::m_LUT_Entries[64][8] = {
{0, 0, 0, 0, 0, 0, 0, },
{7, 6, 4, 5, 0, 0, 0, },
{1, 0, 2, 3, 0, 0, 0, },
{0, 0, 0, 0, 0, 0, 0, },
{1, 5, 4, 0, 0, 0, 0, },
{1, 5, 7, 6, 4, 0, 0, },
{4, 0, 2, 3, 1, 5, 0, },
{5, 7, 6, 4, 0, 2, 3, },
{0, 4, 6, 2, 0, 0, 0, },
{0, 4, 5, 7, 6, 2, 0, },
{6, 2, 3, 1, 0, 4, 0, },
{2, 3, 1, 0, 4, 5, 7, },
{0, 1, 5, 4, 6, 2, 0, },
{0, 1, 5, 7, 6, 2, 0, },
{6, 2, 3, 1, 5, 4, 0, },
{2, 3, 1, 5, 7, 6, 0, },
{2, 6, 7, 3, 0, 0, 0, },
{2, 6, 4, 5, 7, 3, 0, },
{7, 3, 1, 0, 2, 6, 0, },
{3, 1, 0, 2, 6, 4, 5, },
{0, 0, 0, 0, 0, 0, 0, },
{2, 6, 4, 0, 1, 5, 7, },
{7, 3, 1, 5, 4, 0, 2, },
{0, 0, 0, 0, 0, 0, 0, },
{2, 0, 4, 6, 7, 3, 0, },
{2, 0, 4, 5, 7, 3, 0, },
{7, 3, 1, 0, 4, 6, 0, },
{3, 1, 0, 4, 5, 7, 0, },
{2, 0, 1, 5, 4, 6, 7, },
{2, 0, 1, 5, 7, 3, 0, },
{7, 3, 1, 5, 4, 6, 0, },
{3, 1, 5, 7, 0, 0, 0, },
{3, 7, 5, 1, 0, 0, 0, },
{3, 7, 6, 4, 5, 1, 0, },
{5, 1, 0, 2, 3, 7, 0, },
{7, 6, 4, 5, 1, 0, 2, },
{3, 7, 5, 4, 0, 1, 0, },
{3, 7, 6, 4, 0, 1, 0, },
{5, 4, 0, 2, 3, 7, 0, },
{7, 6, 4, 0, 2, 3, 0, },
{0, 0, 0, 0, 0, 0, 0, },
{3, 7, 6, 2, 0, 4, 5, },
{5, 1, 0, 4, 6, 2, 3, },
{0, 0, 0, 0, 0, 0, 0, },
{3, 7, 5, 4, 6, 2, 0, },
{3, 7, 6, 2, 0, 1, 0, },
{5, 4, 6, 2, 3, 7, 0, },
{7, 6, 2, 3, 0, 0, 0, },
{3, 2, 6, 7, 5, 1, 0, },
{3, 2, 6, 4, 5, 1, 0, },
{5, 1, 0, 2, 6, 7, 0, },
{1, 0, 2, 6, 4, 5, 0, },
{3, 2, 6, 7, 5, 4, 0, },
{3, 2, 6, 4, 0, 1, 0, },
{5, 4, 0, 2, 6, 7, 0, },
{6, 4, 0, 2, 0, 0, 0, },
{3, 2, 0, 4, 6, 7, 5, },
{3, 2, 0, 4, 5, 1, 0, },
{5, 1, 0, 4, 6, 7, 0, },
{1, 0, 4, 5, 0, 0, 0, },
{0, 0, 0, 0, 0, 0, 0, },
{3, 2, 0, 1, 0, 0, 0, },
{5, 4, 6, 7, 0, 0, 0, },
{0, 0, 0, 0, 0, 0, 0, },
};

