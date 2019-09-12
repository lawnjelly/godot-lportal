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

/* portal.h */
#ifndef LPORTAL_H
#define LPORTAL_H

/**
	@author lawnjelly <lawnjelly@gmail.com>
*/


#include "scene/3d/spatial.h"
#include "lvector.h"

class LRoom;

class LPortal : public Spatial {
	GDCLASS(LPortal, Spatial);

	friend class LRoom;
private:

	enum eClipResult
	{
		CLIP_OUTSIDE,
		CLIP_PARTIAL,
		CLIP_INSIDE,
	};


	ObjectID m_room_ID;
	NodePath m_room_path;

protected:
	static void _bind_methods();

	LPortal::eClipResult ClipWithPlane(const Plane &p) const;
	void AddPlanes(const Vector3 &ptCam, LVector<Plane> &planes) const;

public:
	// normal determined by winding order
	Vector<Vector3> m_ptsWorld;
	Vector<Vector3> m_ptsLocal;
	Plane m_Plane;


	ObjectID GetLinkedRoomID() const {return m_room_ID;}
	LRoom * GetLinkedRoom() const;

	LPortal();

private:
	// use the name of the portal to find a room to link to
	void Link(LRoom * pParentRoom);
	bool AddRoom(NodePath path);
	void CopyReversedGeometry(const LPortal &source);
	void CreateGeometry(PoolVector<Vector3> p_vertices);
	void PlaneFromPoints();

	void CalculateWorldPoints();
	void CalculateLocalPoints();

	void SortVertsClockwise();
	void ReverseWindingOrder();

// useful funcs
public:
	static bool NameStartsWith(Node * pNode, String szSearch);
	static String FindNameAfter(Node * pNode, String szStart);
	static void print(String sz);
};


#endif
