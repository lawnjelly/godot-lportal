#pragma once

#include "scene/3d/spatial.h"
#include "lvector.h"
#include "lportal.h"

class LRoomManager;
class LRoom;
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
	void Convert(LRoomManager &manager);

private:
	int CountRooms();

	void Convert_Rooms();
	bool Convert_Room(Spatial * pNode, int lroomID);
	void Convert_Room_FindObjects_Recursive(Node * pParent, LRoom &lroom, LAABB &bb_room);

	void Convert_Portals();


	void LRoom_DetectPortalMeshes(LRoom &lroom, LTempRoom &troom);
	void LRoom_MakePortalsTwoWay(LRoom &lroom, LTempRoom &troom, int iRoomNum);
	void LRoom_MakePortalFinalList(LRoom &lroom, LTempRoom &troom);
	void LRoom_DetectedPortalMesh(LRoom &lroom, LTempRoom &troom, MeshInstance * pMeshInstance, String szLinkRoom);
	LPortal * LRoom_RequestNewPortal(LRoom &lroom);

	void TRoom_MakeOppositePortal(const LPortal &port, int iRoomOrig);


	// helper
	bool Node_IsRoom(Node * pNode) const;
	int FindRoom_ByName(String szName) const;


	LRoomManager * m_pManager;
	LVector<LTempRoom> m_TempRooms;

	static void print(String sz);

};
