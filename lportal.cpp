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

#include "lportal.h"
#include "core/engine.h"
#include "lroom.h"
#include "ldebug.h"
#include "lroom_manager.h"

/////////////////////////////////////////////////////////////////////

bool LPortal::NameStartsWith(const Node * pNode, String szSearch)
{
	int sl = szSearch.length();

	String name = pNode->get_name();
	int l = name.length();

	if (l < sl)
		return false;

	String szStart = name.substr(0, sl);

	//print_line("\t\tNameStartsWith szStart is " + szStart);

	if (szStart == szSearch)
		return true;

	return false;
}


String LPortal::FindNameAfter(Node * pNode, String szStart)
{
	String szRes;
	String name = pNode->get_name();
	szRes = name.substr(szStart.length());

	// because godot doesn't support multiple nodes with the same name, we will strip e.g. a number
	// after an @ on the end of the name...
	// e.g. portal_kitchen@2
	for (int c=0; c<szRes.length(); c++)
	{
		if (szRes[c] == '*')
		{
			// remove everything after and including this character
			szRes = szRes.substr(0, c);
			break;
		}
	}

	//print("\t\tNameAfter is " + szRes);
	return szRes;
}

//////////////////////////////////////////////////////////

void LPortal::Debug_CheckPlaneValidity(const Plane &p) const
{
	assert (p.distance_to(m_ptCentre) < 0.0f);
}


// preprocess
void LPortal::AddLightPlanes(LRoomManager &manager, const LLight &light, LVector<Plane> &planes, bool bReverse) const
{
	const Vector<Vector3> &pts = m_ptsWorld;

	int nPoints = pts.size();
	ERR_FAIL_COND(nPoints < 3);

	if (light.m_Source.m_eType == LSource::ST_DIRECTIONAL)
	{
		// assuming ortho light
		const int max_points = 32;
		Vector3 pushed_pts[max_points];

		if (nPoints > max_points)
			nPoints = max_points;

		// transform pushed points
		Vector3 ptPush = light.m_Source.m_ptDir * 2.0;

		for (int n=0; n<nPoints; n++)
		{
			pushed_pts[n] = pts[n] + ptPush;
		}

		Plane p;

		for (int n=0; n<nPoints; n++)
		{
			int nPLUS = (n + 1) % nPoints;
			p = Plane(pts[n], pts[nPLUS], pushed_pts[n]);
			if (bReverse) p = -p;
			planes.push_back(p);
			Debug_CheckPlaneValidity(p);
		}

		// first and last
//		p = Plane(pts[nPoints-1], pts[0], pushed_pts[0]);
//		if (bReverse) p = -p;
//		planes.push_back(p);
//		Debug_CheckPlaneValidity(p);

		// debug
		if (manager.m_bDebugLights)
		{
			for (int n=1; n<nPoints; n++)
			{
				if (n == 2)
					continue;

				manager.m_DebugPortalLightPlanes.push_back(pts[n-1]);
				manager.m_DebugPortalLightPlanes.push_back(pts[n]);
				manager.m_DebugPortalLightPlanes.push_back(pushed_pts[n]);
				manager.m_DebugPortalLightPlanes.push_back(pushed_pts[n]);
				manager.m_DebugPortalLightPlanes.push_back(pushed_pts[n-1]);
				manager.m_DebugPortalLightPlanes.push_back(pts[n-1]);
			}
		}

		return;
	}

	// use a point for the light for omni and spotlight
	Plane p;

	for (int n=0; n<nPoints; n++)
	{
		int nPLUS = (n + 1) % nPoints;
		p = Plane(pts[nPLUS], pts[n], light.m_Source.m_ptPos);
		if (bReverse) p = -p;
		planes.push_back(p);
		Debug_CheckPlaneValidity(p);
	}

	// first and last
//	p = Plane(pts[0], pts[nPoints-1], light.m_ptPos);
//	if (bReverse) p = -p;
//	planes.push_back(p);
//	Debug_CheckPlaneValidity(p);

}


// add clipping planes to the vector formed by each portal edge and the camera
void LPortal::AddPlanes(LRoomManager &manager, const Vector3 &ptCam, LVector<Plane> &planes) const
{
	// short version
	const Vector<Vector3> &pts = m_ptsWorld;

	int nPoints = pts.size();
	ERR_FAIL_COND(nPoints < 3);

	Plane p;

	for (int n=1; n<nPoints; n++)
	{
		p = Plane(ptCam, pts[n], pts[n-1]);

		// detect null plane
//		if (p.normal.length_squared() < 0.1f)
//		{
//			print("NULL plane detected from points : ");
//			print(ptCam + pts[n] + pts[n-1]);
//		}
		planes.push_back(p);
		Debug_CheckPlaneValidity(p);
	}

	// first and last
	p = Plane(ptCam, pts[0], pts[nPoints-1]);
	planes.push_back(p);
	Debug_CheckPlaneValidity(p);

	// debug
	if (!manager.m_bDebugPlanes)
		return;

	for (int n=0; n<nPoints; n++)
	{
		manager.m_DebugPlanes.push_back(pts[n]);
	}

}


LPortal::eClipResult LPortal::ClipWithPlane(const Plane &p) const
{
	int nOutside = 0;
	int nPoints = m_ptsWorld.size();

	for (int n=0; n<nPoints; n++)
	{
		float d = p.distance_to(m_ptsWorld[n]);

		if  (d >= 0.0f)
			nOutside++;
	}

	if (nOutside == nPoints)
	{
		LPRINT_RUN(2, "\t\tOutside plane " + p);
		return CLIP_OUTSIDE;
	}

	if (nOutside == 0)
		return CLIP_INSIDE;

	return CLIP_PARTIAL;
}


void LPortal::CreateGeometry(PoolVector<Vector3> p_vertices, const Transform &trans, bool bPortalPlane_Convention)
{
	int nPoints = p_vertices.size();
	ERR_FAIL_COND(nPoints < 3);

	m_ptsWorld.clear();

	//print("\t\t\tLPortal::CreateGeometry nPoints : " + itos(nPoints));

	//Vector3 ptFirstThree[3];

	for (int n=0; n<nPoints; n++)
	{
		Vector3 ptWorld = trans.xform(p_vertices[n]);

		// store the first three to make the plane, irrespective of duplicates
//		if (n < 3)
//			ptFirstThree[n] = ptWorld;

		// new!! test for duplicates. Some geometry may contain duplicate verts in portals which will muck up
		// the winding etc...
		bool bDuplicate = false;
		for (int m=0; m<m_ptsWorld.size(); m++)
		{
			Vector3 ptDiff = ptWorld - m_ptsWorld[m];
			if (ptDiff.length() < 0.001f)
			{
				bDuplicate = true;
				break;
			}
		}

		if (!bDuplicate)
		{
			m_ptsWorld.push_back(ptWorld);
			m_ptCentre += ptWorld;
		}

		//print("\t\t\t\t" + itos(n) + "\tLocal : " + Variant(p_vertices[n]) + "\tWorld : " + ptWorld);
	}

//	Plane portal_plane = Plane(ptFirstThree[0], ptFirstThree[1], ptFirstThree[2]);

	SortVertsClockwise(bPortalPlane_Convention);
	PlaneFromPoints();
}

void LPortal::SortVertsClockwise(bool bPortalPlane_Convention)
{
	Vector<Vector3> &verts = m_ptsWorld;

	// We first assumed first 3 determine the desired normal

	// find normal
	Plane plane;
	if (bPortalPlane_Convention)
		plane = Plane(verts[0], verts[2], verts[1]);
	else
		plane = Plane(verts[0], verts[1], verts[2]);

	Vector3 ptNormal = plane.normal;

	// find centroid
	int nPoints = verts.size();

	Vector3 ptCentre = Vector3(0, 0, 0);

	for (int n=0; n<nPoints; n++)
	{
		ptCentre += verts[n];
	}
	ptCentre /= nPoints;
	m_ptCentre = ptCentre;


	// now algorithm
	for (int n=0; n<nPoints-2; n++)
	{
		Vector3 a = verts[n] - ptCentre;
		a.normalize();

		Plane p = Plane(verts[n], ptCentre, ptCentre + ptNormal);

		double SmallestAngle = -1;
		int Smallest = -1;

		for (int m=n+1; m<nPoints; m++)
		{
			if (p.distance_to(verts[m]) > 0.0f)
			{
				Vector3 b = verts[m] - ptCentre;
				b.normalize();

				double Angle = a.dot(b);

				if (Angle > SmallestAngle)
				{
					SmallestAngle = Angle;
					Smallest = m;
				}
			} // which side

		} // for m

		// swap smallest and n+1 vert
		if (Smallest != -1)
		{
			Vector3 temp = verts[Smallest];
			verts.set(Smallest, verts[n+1]);
			verts.set(n+1, temp);
		}
	} // for n


	// the vertices are now sorted, but may be in the opposite order to that wanted.
	// we detect this by calculating the normal of the poly, then flipping the order if the normal is pointing
	// the wrong way.
	plane = Plane(verts[0], verts[1], verts[2]);

	if (ptNormal.dot(plane.normal) < 0.0f)
	{
		// reverse order of verts
		ReverseWindingOrder();
	}

}

void LPortal::ReverseWindingOrder()
{
	Vector<Vector3> &verts = m_ptsWorld;
	Vector<Vector3> copy = verts;

	for (int n=0; n<verts.size(); n++)
	{
		verts.set(n, copy[verts.size() - n - 1]);
	}

}


void LPortal::CopyReversedGeometry(const LPortal &source)
{
	//print("\t\t\tCopyReversedGeometry");
	// points are the same but reverse winding order
	int nPoints = source.m_ptsWorld.size();
	m_ptCentre = source.m_ptCentre;

	m_ptsWorld.resize(nPoints);

	for (int n=0; n<nPoints; n++)
	{
		m_ptsWorld.set(n, source.m_ptsWorld[nPoints - n - 1]);
		//print("\t\t\t\t" + itos(n) + "\t: " + Variant(m_ptsWorld[n]));
	}

	PlaneFromPoints();
}

void LPortal::PlaneFromPoints()
{
	if (m_ptsWorld.size() < 3)
	{
		WARN_PRINT("Portal must have at least 3 vertices");
		return;
	}
	// create plane from points
	m_Plane = Plane(m_ptsWorld[0], m_ptsWorld[1], m_ptsWorld[2]);

	//print("\t\t\t\t\tPlane normal world space : " + m_Plane);

}


LPortal::LPortal() {
	// unset
	m_iRoomNum = -1;
	m_bMirror = false;
//	m_uiFrameTouched_Blocked = 0;
}



