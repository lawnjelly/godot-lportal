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


bool LPortal::NameStartsWith(Node * pNode, String szSearch)
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

	print_line("\t\tNameAfter is " + szRes);
	return szRes;
}

//////////////////////////////////////////////////////////

LPortal::eClipResult LPortal::ClipWithPlane(const Plane &p) const
{
	int nOutside = 0;
	int nPoints = m_ptsWorld.size();

	for (int n=0; n<nPoints; n++)
	{
		float d = p.distance_to(m_ptsWorld[n]);

		if  (d >= 0.0)
			nOutside++;
	}

	if (nOutside == nPoints)
		return CLIP_OUTSIDE;

	if (nOutside == 0)
		return CLIP_INSIDE;

	return CLIP_PARTIAL;
}




// use the name of the portal to find a room to link to
void LPortal::Link(LRoom * pParentRoom)
{
	// should start with 'portal_'
	if (!NameStartsWith(this, "lportal_"))
	{
		WARN_PRINT("Portal name should begin with lportal_");
		return;
	}

	String szRoom = FindNameAfter(this, "lportal_");

	print_line("LPortal::Link to room " + szRoom);

	// find the room group
	Spatial * pGroup = Object::cast_to<Spatial>(pParentRoom->get_parent());
	if (!pGroup)
	{
		WARN_PRINT("Room parent is not a spatial");
		return;
	}

	// attempt to find a child of the group that has the name specified
	int nChildren = pGroup->get_child_count();

	for (int n=0; n<nChildren; n++)
	{
		Node * pChild = pGroup->get_child(n);

		String szChildName = pChild->get_name();

		// is the name correct for the desired room?
		if (szRoom != szChildName)
			continue;

		LRoom * pTargetRoom = Object::cast_to<LRoom>(pChild);

		if (!pTargetRoom)
		{
			WARN_PRINT("Portal target is not a room");
			return;
		}

		// found! link
		pTargetRoom->MakeOppositePortal(this, pParentRoom);
		return;
	}
}


void LPortal::CreateGeometry(PoolVector<Vector3> p_vertices)
{
	int nPoints = p_vertices.size();
	ERR_FAIL_COND(nPoints < 3);

	m_ptsLocal.resize(nPoints);
	m_ptsWorld.resize(nPoints);

	print_line("\tLPortal::CreateGeometry nPoints : " + itos(nPoints));

	for (int n=0; n<nPoints; n++)
	{
		m_ptsLocal.set(n, p_vertices[n]);
		Variant pt = p_vertices[n];
		print_line("\t\t" + itos(n) + "\t: " + pt);
	}

	SortVertsClockwise();

	CalculateWorldPoints();

	PlaneFromPoints();
}

// assume first 3 determine the desired normal
void LPortal::SortVertsClockwise()
{
	Vector<Vector3> &verts = m_ptsLocal;

	// find normal
	Plane plane = Plane(verts[0], verts[1], verts[2]);
	Vector3 ptNormal = plane.normal;

	// find centroid
	int nPoints = verts.size();

	Vector3 ptCentre = Vector3(0, 0, 0);

	for (int n=0; n<nPoints; n++)
	{
		ptCentre += verts[n];
	}
	ptCentre /= nPoints;


	// now algorithm
	for (int n=0; n<nPoints-2; n++)
	{
		Vector3 a = verts[n] - ptCentre;
		a.normalize();

		Plane p = Plane(verts[n], ptCentre, ptCentre + ptNormal);

		double SmallestAngle = -1;
		int Smallest = -1;

		for (unsigned int m=n+1; m<nPoints; m++)
		{
			if (p.distance_to(verts[m]) > 0.0f)
//			if (p.WhichSideNDLCompatible(m_Verts[m], 0.0f) != CoPlane::NEGATIVE_SIDE)
			{
				Vector3 b = m_ptsLocal[m] - ptCentre;
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
	Vector<Vector3> &verts = m_ptsLocal;
	Vector<Vector3> copy = verts;

	for (int n=0; n<verts.size(); n++)
	{
		verts.set(n, copy[verts.size() - n - 1]);
	}

}


// local from world and local transform
void LPortal::CalculateLocalPoints()
{
	int nPoints = m_ptsLocal.size();
	ERR_FAIL_COND(m_ptsLocal.size() != m_ptsWorld.size());

	Transform tr = get_transform();

	print_line("\tCalculateLocalPoints");
	for (int n=0; n<nPoints; n++)
	{
		m_ptsLocal.set(n, tr.xform_inv(m_ptsWorld[n]));
		Variant pt = m_ptsLocal[n];
		print_line("\t\t" + itos(n) + "\t: " + pt);
	}
}

// world from local and transform
void LPortal::CalculateWorldPoints()
{
	int nPoints = m_ptsLocal.size();
	ERR_FAIL_COND(m_ptsLocal.size() != m_ptsWorld.size());

	Transform tr = get_global_transform();

	print_line("\tCalculateWorldPoints");
	for (int n=0; n<nPoints; n++)
	{
		m_ptsWorld.set(n, tr.xform(m_ptsLocal[n]));
		Variant pt = m_ptsWorld[n];
		print_line("\t\t" + itos(n) + "\t: " + pt);
	}
}

void LPortal::CopyReversedGeometry(const LPortal &source)
{
	print_line("CopyReversedGeometry");
	// points are the same but reverse winding order
	int nPoints = source.m_ptsWorld.size();

	m_ptsLocal.resize(nPoints);
	m_ptsWorld.resize(nPoints);

	for (int n=0; n<nPoints; n++)
	{
		m_ptsWorld.set(n, source.m_ptsWorld[nPoints - n - 1]);
		Variant pt = m_ptsWorld[n];
		print_line("\t\t" + itos(n) + "\t: " + pt);
	}

	CalculateLocalPoints();
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

	print_line("Plane normal world space : " + m_Plane);

//	Plane opp = Plane(m_ptsWorld[2], m_ptsWorld[1], m_ptsWorld[0]);
//	print_line("Plane opposite : " + opp);
}


bool LPortal::AddRoom(NodePath path)
{
	print_line("LPortal::AddRoom path is " + path);

	if (has_node(path))
	{
		LRoom * pNode = Object::cast_to<LRoom>(get_node(path));
		if (pNode)
		{
			ObjectID id = pNode->get_instance_id();

			m_room_path = path;
			m_room_ID = id;

			// make the portal name correct and feature the room name
			int num_names = path.get_name_count();
			if (num_names < 1)
			{
				WARN_PRINT("LPortal::AddRoom : Path too short");
				return false;
			}
			String szRoom = path.get_name(num_names-1);

			String szPortal = "lportal_" + szRoom;
			set_name(szPortal);

			return true;
		}
		else
		{
			WARN_PRINT("not a room");
			return false;
		}
	}
	else
	{
		WARN_PRINTS("portal link room not found : " + path);
	}

	return false;
}

LPortal::LPortal() {
	// unset
	m_room_ID = 0;
}

LRoom * LPortal::GetLinkedRoom() const
{
	Object *pObj = ObjectDB::get_instance(m_room_ID);

	if (!pObj)
		return 0;

	LRoom * pRoom = Object::cast_to<LRoom>(pObj);
	if (!pRoom)
	{
		WARN_PRINT_ONCE("LRoomManager::FrameUpdate : curr room is not an LRoom");
	}

	return pRoom;
}


void LPortal::_bind_methods() {

}



