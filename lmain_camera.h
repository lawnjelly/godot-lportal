#pragma once

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

//#define LMAINCAMERA_CALC_LUT

// we get the main camera clipping planes and derive the points from 3 plane equations
// in order to cull shadow casters to the camera frustum
class LMainCamera
{
public:
	// same order as godot
	enum ePlane {
		P_NEAR,
		P_FAR,
		P_LEFT,
		P_TOP,
		P_RIGHT,
		P_BOTTOM,
		P_TOTAL,
	};

	// same order as godot
	enum ePoint {
		PT_FAR_LEFT_TOP,
		PT_FAR_LEFT_BOTTOM,
		PT_FAR_RIGHT_TOP,
		PT_FAR_RIGHT_BOTTOM,
		PT_NEAR_LEFT_TOP,
		PT_NEAR_LEFT_BOTTOM,
		PT_NEAR_RIGHT_TOP,
		PT_NEAR_RIGHT_BOTTOM,
	};

	static const char * m_szPlanes[];
	static const char * m_szPoints[];

	// 6 bits, 6 planes
	enum { NUM_CAM_PLANES = 6,
		NUM_CAM_POINTS = 8,
		MAX_CULL_PLANES = 16,
		LUT_SIZE = 64,
	};

	// create the LUT
	LMainCamera();

	bool Prepare(LRoomManager &manager, Camera * pCam);

	// main use of this object, we can create a clipping volume that is a mix of the light frustum and the camera volume
	bool AddCameraLightPlanes(LRoomManager &manager, const LSource &lsource, LVector<Plane> &planes) const;

	LVector<Plane> m_Planes;
	LVector<Vector3> m_Points;

	// centre of camera frustum
	Vector3 m_ptCentre;

private:
	bool AddCameraLightPlanes_Directional(LRoomManager &manager, const LSource &lsource, LVector<Plane> &planes) const;
	String String_PlaneBF(unsigned int BF);

#ifdef LMAINCAMERA_CALC_LUT
	void GetNeighbours(ePlane p, ePlane neigh_planes[4]) const;
	void GetCornersOfPlanes( ePlane _fpPlane0, ePlane _fpPlane1, ePoint _fpRet[2] ) const;
	void CreateLUT();
	void CompactLUT_Entry(int n);
	void DebugPrintLUT();
	void DebugPrintLUT_AsTable();
	void AddLUT(int p0, int p1, ePoint pts[2]);
	void AddLUT_Entry(unsigned int n, ePoint pts[2]);
	String DebugStringLUT_Entry(const LVector<uint8_t> &entry);
	String String_LUTEntry(const LVector<uint8_t> &entry);

	// contains a list of points for each combination of plane facing directions
	LVector<uint8_t> m_LUT[LUT_SIZE];
#endif

	// precalculated LUT
	static uint8_t m_LUT_EntrySizes[LUT_SIZE];
	static uint8_t m_LUT_Entries[LUT_SIZE][8];
};

