#include "ldoblist.h"
#include "lroom_manager.h"
#include "ldebug.h"

// returns whether changed room
bool LDobList::FindDOBOldAndNewRoom(LRoomManager &manager, int dob_id, const Vector3 &pos, int &old_room_id, int &new_room_id)
{
	LDob &dob = GetDob(dob_id);

	bool bCamera =  (dob_id == manager.m_DOB_id_camera);
	float slop = 0.2f;
	if (bCamera)
		slop = 0.0f;
	else
	{
//		print_line("dob position : " + String(Variant(pt)));
	}

	// current room
	old_room_id = dob.m_iRoomID;
	new_room_id = old_room_id; // default

	bool bChangedRoom = false;
	LRoom * pCurrentRoom = 0;

	if (old_room_id == -1)
		bChangedRoom = true;
	else
	{
		pCurrentRoom = manager.GetRoom(old_room_id);

		// deal with the case that the DOB has moved way outside the room
		if (!pCurrentRoom->m_Bound.IsPointWithin(pos, 1.0f))
			bChangedRoom = true;
	}

	// if changed room definitely?
	if (bChangedRoom)
	{
		// revert to expensive method (may have been teleported etc)
		new_room_id = manager.FindClosestRoom(pos);

		if (new_room_id != old_room_id)
			return true;
		else
			return false;
	}


	assert (pCurrentRoom);

	// check each portal - has the object crossed it into the neighbouring room?
	for (int p=0; p<pCurrentRoom->m_iNumPortals; p++)
	{
		const LPortal &port = manager.m_Portals[pCurrentRoom->m_iFirstPortal + p];

		float dist = port.m_Plane.distance_to(pos);

		if (dist > slop)
		{
			LPRINT(0, "DOB at pos " + pos + " ahead of portal " + port.get_name() + " by " + String(Variant(dist)));

			new_room_id = manager.Portal_GetLinkedRoom(port).m_RoomID;
			return true;
		}
	}

	return false;
}

void LDobList::UpdateVisibility(LRoomManager &manager, Spatial * pDOBSpatial, int dob_id)
{
	LDob &dob = GetDob(dob_id);

	// get the room
	LRoom * pRoom = manager.GetRoom(dob.m_iRoomID);
	bool bRoomVisible = pRoom->IsVisible();

	Spatial * pDOB = pDOBSpatial;

	bool bDobVis = pDOB->is_visible_in_tree();
	if (bDobVis != bRoomVisible)
	{
		//String sz = "DOB " + pDOB->get_name() + "\t";

		if (bRoomVisible)
		{
			pDOB->show();
			//sz += "coming into view";
		}
		else
		{
			pDOB->hide();
			//sz += "exiting view";
		}

		//print_line(sz);
	}


}



int LDobList::UpdateDob(LRoomManager &manager, int dob_id, const Vector3 &pos)
{
	LDob &dob = GetDob(dob_id);

	// get the VI for showing / hiding
	Spatial * pSpat = (Spatial *) dob.GetVI();

	int old_room, new_room;

	if (FindDOBOldAndNewRoom(manager, dob_id, pos, old_room, new_room))
	{
		dob.m_iRoomID = new_room;
	}

	if (dob_id != manager.m_DOB_id_camera)
		UpdateVisibility(manager, pSpat, dob_id);

	return dob.m_iRoomID;
}

/*
	// get _global_transform DOES NOT WORK when detached from scene tree (or hidden)
	const Vector3 &pt = pDOB->get_global_transform().origin;

	// is it the camera?
	bool bCamera = pDOB->get_instance_id() == manager.m_ID_camera;
	float slop = 0.2f;
	if (bCamera)
		slop = 0.0f;
	else
	{
//		print_line("dob position : " + String(Variant(pt)));
	}

	// deal with the case that the DOB has moved way outside the room
	if (!m_Bound.IsPointWithin(pt, 1.0f))
	{
		// revert to expensive method (may have been teleported etc)
		int iRoomNum = manager.FindClosestRoom(pt);
		//print_line("dob_teleport closest room " + itos(iRoomNum));

		if (iRoomNum == -1)
			return 0;

		// could be no change of room...
		if (iRoomNum == m_RoomID)
			return 0;

		return manager.GetRoom(iRoomNum);
	}

	// the camera can't have slop because we might end up front side of a door without entering the room,
	// hence can't see into the room through the portal!
//	if (bCamera)
//		slop = 0.0f;

	// check each portal - has the object crossed it into the neighbouring room?
	for (int p=0; p<m_iNumPortals; p++)
	{
		const LPortal &port = manager.m_Portals[m_iFirstPortal + p];

		float dist = port.m_Plane.distance_to(pt);

		if (dist > slop)
		{
			LPRINT(0, "DOB at pos " + pt + " ahead of portal " + port.get_name() + " by " + String(Variant(dist)));

			// we want to move into the adjoining room
			return &manager.Portal_GetLinkedRoom(port);
		}
	}

	return 0;
	*/

