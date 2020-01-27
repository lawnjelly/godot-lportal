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

/**
	@author lawnjelly <lawnjelly@gmail.com>
*/


#include "scene/3d/spatial.h"
#include "lvector.h"
#include "lportal.h"

class LRoomManager;
class LRoom;
class LArea;
class MeshInstance;

// simple min max aabb
class LAABB
{
public:
	Vector3 m_ptMins;
	Vector3 m_ptMaxs;
	void SetToMaxOpposite()
	{
		float ma = FLT_MAX;
		float mi = FLT_MIN;
		m_ptMins = Vector3(ma, ma, ma);
		m_ptMaxs = Vector3(mi, mi, mi);
	}
	void ExpandToEnclose(const AABB &bb)
	{
		if (bb.position.x < m_ptMins.x) m_ptMins.x = bb.position.x;
		if (bb.position.y < m_ptMins.y) m_ptMins.y = bb.position.y;
		if (bb.position.z < m_ptMins.z) m_ptMins.z = bb.position.z;
		if (bb.position.x + bb.size.x > m_ptMaxs.x) m_ptMaxs.x = bb.position.x + bb.size.x;
		if (bb.position.y + bb.size.y > m_ptMaxs.y) m_ptMaxs.y = bb.position.y + bb.size.y;
		if (bb.position.z + bb.size.z > m_ptMaxs.z) m_ptMaxs.z = bb.position.z + bb.size.z;
	}
	Vector3 FindCentre() const
	{
		Vector3 pt;
		pt.x = (m_ptMaxs.x - m_ptMins.x) * 0.5f;
		pt.y = (m_ptMaxs.y - m_ptMins.y) * 0.5f;
		pt.z = (m_ptMaxs.z - m_ptMins.z) * 0.5f;
		pt += m_ptMins;
		return pt;
	}
};



class LRoomConverter
{
public:
	// temp rooms are used as an intermediate during conversion, because we need to convert the original portals
	// and the mirror portals from 2 bits of code, and we want to end up with a final contiguous list of portals
	// for efficient rendering.
	class LTempRoom
	{
	public:
		LVector<LPortal> m_Portals;
	};

	// this function calls everything else in the converter
	// single room mode enables us to emulate a room list in games that do not have rooms...
	// this allows taking advantage of basic LPortal speedup without converting games / demos
	void Convert(LRoomManager &manager, bool bVerbose, bool bPreparationRun, bool bDeleteLights, bool bSingleRoomMode = false);

private:
	int CountRooms();

	void Convert_Rooms();
	int Convert_Rooms_Recursive(Node * pParent, int count, int area);
	bool Convert_Room(Spatial * pNode, int lroomID, int areaID);
	void Convert_Room_FindObjects_Recursive(Node * pParent, LRoom &lroom, LAABB &bb_room);
	void Convert_Room_SetDefaultCullMask_Recursive(Node * pParent);
	bool Convert_IsVisibleInRooms(const Node * pNode) const;

	void Convert_Portals();
	void Convert_Bounds();
	bool Convert_ManualBound(LRoom &lroom, MeshInstance * pMI);
	void GetWorldVertsFromMesh(const MeshInstance &mi, Vector<Vector3> &pts) const;
	void Bound_FindPoints_Recursive(Node * pNode, Vector<Vector3> &pts);
	bool Convert_Bound_FromPoints(LRoom &lroom, const Vector<Vector3> &points);
	void Convert_ShadowCasters();
	void Convert_Lights();
	void Convert_AreaLights();


	void LRoom_DetectPortalMeshes(LRoom &lroom, LTempRoom &troom);
	void LRoom_MakePortalsTwoWay(LRoom &lroom, LTempRoom &troom, int iRoomNum);
	void LRoom_MakePortalFinalList(LRoom &lroom, LTempRoom &troom);
	void LRoom_DetectedPortalMesh(LRoom &lroom, LTempRoom &troom, MeshInstance * pMeshInstance, String szLinkRoom);
	LPortal * LRoom_RequestNewPortal(LRoom &lroom);
	void LRoom_PushBackSOB(LRoom &lroom, const LSob &sob);

	// lights
	void LRoom_DetectedLight(LRoom &lroom, Node * pNode);
	void Light_Trace(int iLightID);
	void LRoom_DetectedArea(LRoom &lroom, Node * pNode);

	// shadows
	void LRoom_FindShadowCasters_FromLight(LRoom &lroom, const LLight &light);
	void LRoom_FindShadowCasters_Recursive(LRoom &source_lroom, int depth, LRoom &lroom, const LLight &light, const LVector<Plane> &planes);
	void LRoom_AddShadowCaster_SOB(LRoom &lroom, int sobID);


	void TRoom_MakeOppositePortal(const LPortal &port, int iRoomOrig);


	// helper
	bool Node_IsRoom(Node * pNode) const;
	bool Node_IsArea(Node * pNode) const;
	bool Node_IsPortal(Node * pNode) const;
	bool Node_IsBound(Node * pNode) const;
	bool Node_IsIgnore(Node * pNode) const;
	bool Node_IsLight(Node * pNode) const;

	int FindRoom_ByName(String szName) const;
	int Area_FindOrCreate(String szName);


	// set up on entry
	LRoomManager * m_pManager;
	Spatial * m_pRoomList; // room list pointed to by the manager nodepath

	LVector<LTempRoom> m_TempRooms;

	bool Bound_AddPlaneIfUnique(LVector<Plane> &planes, const Plane &p);


	// whether we are preparing the level, or doing a final run,
	// in which case we should delete lights and set vis flags
	bool m_bFinalRun;
	bool m_bDeleteLights;
	bool m_bSingleRoomMode;
};
