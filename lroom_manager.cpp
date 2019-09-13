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

#include "lroom_manager.h"
#include "lportal.h"
#include "lroom.h"
#include "core/engine.h"
#include "scene/3d/camera.h"
#include "scene/3d/mesh_instance.h"

LRoomManager::LRoomManager()
{
//	m_room_curr = 0;
	m_cameraID = 0;
	m_uiFrameCounter = 0;
}

int LRoomManager::FindClosestRoom(const Vector3 &pt) const
{
	//print_line("FindClosestRoom");
	int closest = -1;
	float closest_dist = FLT_MAX;

	for (int n=0; n<m_room_IDs.size(); n++)
	{
		LRoom * pRoom = GetRoomNum(n);
		if (!pRoom)
			continue;

		float d = pt.distance_squared_to(pRoom->m_ptCentre);
//		print_line("\troom " + itos(n) + " dist " + String(Variant(d)));

		if (d < closest_dist)
		{
			closest = n;
			closest_dist = d;
		}
	}

	return closest;
}


LRoom * LRoomManager::GetRoomNum(int i) const
{
	assert (i < m_room_IDs.size());
	Object *pObj = ObjectDB::get_instance(m_room_IDs[i]);
	if (!pObj)
		return 0;

	LRoom * pRoom = Object::cast_to<LRoom>(pObj);
	if (!pRoom)
		return 0;

	return pRoom;
}

int LRoomManager::GetRoomNumFromLRoom(LRoom * pRoom) const
{
	// slow .. use metadata for this
	int search_id = pRoom->get_instance_id();

	for (int n=0; n<m_room_IDs.size(); n++)
	{
		if (m_room_IDs[n] == search_id)
			return n;
	}

	return -1;
}


void LRoomManager::Obj_SetRoomNum(Node * pNode, int num)
{
	pNode->set_meta("_lroom", num);

	assert (Obj_GetRoomNum(pNode) == num);
}

int LRoomManager::Obj_GetRoomNum(Node * pNode) const
{
	//assert (pNode->has_meta("_lroom"));
	Variant v = pNode->get_meta("_lroom");
	if (v.get_type() == Variant::NIL)
		return -1;

	return v;
}

LRoom * LRoomManager::GetRoomFromDOB(Node * pNode) const
{
	int iRoom = Obj_GetRoomNum(pNode);
	if (iRoom == -1)
	{
		WARN_PRINT_ONCE("LRoomManager::GetRoomFromDOB : metadata is empty");
		return 0;
	}

	LRoom * pRoom = GetRoomNum(iRoom);
	if (pRoom == 0)
	{
		WARN_PRINT_ONCE("LRoomManager::GetRoomFromDOB : pRoom is NULL");
	}
	return pRoom;
}


void LRoomManager::register_dob(Node * pDOB)
{
	print_line("register_dob " + pDOB->get_name());

	Spatial * pSpat = Object::cast_to<Spatial>(pDOB);
	if (!pSpat)
		return;

	Vector3 pt = pSpat->get_global_transform().origin;

	int iRoomNum = FindClosestRoom(pt);
	print_line("register_dob closest room " + itos(iRoomNum));

	if (iRoomNum == -1)
		return;

	LRoom * pRoom = GetRoomNum(iRoomNum);
	if (!pRoom)
		return;

	pRoom->AddDOB(pSpat);

	// save the room ID on the dob metadata
	Obj_SetRoomNum(pSpat, iRoomNum);
}


bool LRoomManager::update_dob(Node * pDOB)
{
	// find the room the object is attached to
	LRoom * pRoom = GetRoomFromDOB(pDOB);
	if (!pRoom)
		return false;

	Spatial * pSpat = Object::cast_to<Spatial>(pDOB);
	if (!pSpat)
		return false;

	// is it the camera?
	//bool bCamera = pDOB->get_instance_id() == m_cameraID;

	LRoom * pNewRoom = pRoom->UpdateDOB(pSpat);

	if (pNewRoom)
	{
		// remove from the list in old room and add to list in new room, and change the metadata
		int iRoomNum = GetRoomNumFromLRoom(pNewRoom);

		pRoom->RemoveDOB(pDOB);
		pNewRoom->AddDOB(pSpat);

		// save the room ID on the dob metadata
		Obj_SetRoomNum(pSpat, iRoomNum);
	}

	return false;
}

bool LRoomManager::teleport_dob(Node * pDOB)
{
	return true;
}



void LRoomManager::unregister_dob(Node * pDOB)
{
	LRoom * pRoom = GetRoomFromDOB(pDOB);
	pRoom->RemoveDOB(pDOB);
}


/*
bool LRoomManager::update_object(Node * pObj)
{
	// find the room the object is attached to
	Node * pParent = pObj->get_parent();
	LRoom * pRoom = Object::cast_to<LRoom>(pParent);
	if (!pRoom)
	{
		WARN_PRINT_ONCE("LRoomManager::update_object : object parent is not an LRoom");
		return false;
	}

	bool bChanged = pRoom->UpdateDynamicObject(pObj);

	// special .. for camera keep the camera room ID up to date
	// could alternatively just use the parent of the camera?
//	if (bChanged)
//	{
//		if (pObj->get_instance_id() == m_cameraID)
//		{
//			m_room_curr = pObj->get_parent()->get_instance_id();
//		}
//	}

	return bChanged;
}
*/

void LRoomManager::set_camera(Node * pCam)
{
	m_cameraID = 0;

	if (!pCam)
		return;

	Camera * pCamera = Object::cast_to<Camera>(pCam);
	if (!pCamera)
	{
		WARN_PRINT("Not a camera");
		return;
	}

	m_cameraID = pCam->get_instance_id();

	// use this temporarily to force debug
	LPortal::m_bRunning = false;
}

// convert empties and meshes to rooms and portals
void LRoomManager::convert()
{
	LPortal::m_bRunning = false;
	print_line("running convert");

	Convert_Rooms();
	Convert_Portals();
	Find_Rooms();
	LPortal::m_bRunning = true;
}

void LRoomManager::Find_Rooms()
{
	print_line ("Find_Rooms");
	m_room_IDs.clear();

	// first find all room empties and convert to LRooms
	for (int n=0; n<get_child_count(); n++)
	{
		Node * pChild = get_child(n);

		// don't want to handle already converted rooms
		LRoom * pRoom = Object::cast_to<LRoom>(pChild);
		if (pRoom)
		{
			pRoom->m_LocalRoomID = m_room_IDs.size();
			m_room_IDs.push_back(pRoom->get_instance_id());
		}
	}

	/*
	m_room_curr = 0;

	// just set current room to first room
	if (m_room_IDs.size())
	{
		m_room_curr = m_room_IDs[0];
		print_line("first room ID is " + itos(m_room_curr));
	}
	*/

	// make sure bitfield is right size for number of rooms
	m_BF_visible_rooms.Create(m_room_IDs.size());
}

void LRoomManager::Convert_Rooms()
{
	print_line("Convert_Rooms");

	bool bConvertedOne = true;

	// instead of recursive routine
	while (bConvertedOne)
	{
		bConvertedOne = false;

		// first find all room empties and convert to LRooms
		for (int n=0; n<get_child_count(); n++)
		{
			Node * pChild = get_child(n);

			// don't want to handle already converted rooms
			LRoom * pRoom = Object::cast_to<LRoom>(pChild);
			if (pRoom)
				continue;

			Spatial * pSpatialChild = Object::cast_to<Spatial>(pChild);
			if (!pSpatialChild)
				continue;

			if (LPortal::NameStartsWith(pSpatialChild, "room_"))
			{
				if (Convert_Room(pSpatialChild))
					bConvertedOne = true;
			}

			if (bConvertedOne)
				break;
		}
	}

}

void LRoomManager::Convert_Portals()
{
	for (int pass=0; pass<3; pass++)
	{
		print_line("Convert_Portals pass " + itos(pass));

		// first find all room empties and convert to LRooms
		for (int n=0; n<get_child_count(); n++)
		{
			Node * pChild = get_child(n);

			// don't want to handle already converted rooms
			LRoom * pRoom = Object::cast_to<LRoom>(pChild);
			if (pRoom)
			{
				switch (pass)
				{
				case 0:
					pRoom->DetectPortalMeshes();
					break;
				case 1:
					pRoom->MakePortalsTwoWay();
					break;
				case 2:
					pRoom->MakePortalQuickList();
					break;
				}
			}
		}

	}
}


bool LRoomManager::Convert_Room(Spatial * pNode)
{
	// get the room part of the name
	String szFullName = pNode->get_name();
	String szRoom = LPortal::FindNameAfter(pNode, "room_");

	print_line("Convert_Room : " + szFullName);

	// create a new LRoom to exchange the children over to, and delete the original empty
	LRoom * pNew = memnew(LRoom);
	pNew->set_name(szRoom);
	add_child(pNew);

	// make the transform of the L room match the original spatial
	pNew->set_transform(pNode->get_transform());

	// New .. room is at origin, all the child nodes are now transformed
	// so everything is in world space ... makes dynamic objects changing rooms easier
	//Transform tr_orig = pNode->get_transform();

	int nChildren = pNode->get_child_count();

	LAABB bb_room;
	bb_room.SetToMaxOpposite();

	for (int n=0; n<nChildren; n++)
	{
		// reverse count
		int c = nChildren - n - 1;

		Node * pChild = pNode->get_child(c);

		// change the transform of the child to take away the room transform
//		Spatial * pSChild = Object::cast_to<Spatial>(pChild);
//		Transform tr_world;
//		if (pSChild)
//		{
//			tr_world = pSChild->get_global_transform();
//		}

		// update bound to find centre of room roughly
		VisualInstance * pVI = Object::cast_to<VisualInstance>(pChild);
		if (pVI)
		{
			AABB bb = pVI->get_transformed_aabb();
			bb_room.ExpandToEnclose(bb);
		}


		pNode->remove_child(pChild);

		// add the child to the new node
		pNew->add_child(pChild);


//		if (pSChild)
//		{
//			pSChild->set_transform(tr_world);
//		}

	}

	pNew->m_ptCentre = bb_room.FindCentre();
	print_line(String(pNew->get_name()) + " centre " + pNew->m_ptCentre);

	// all moved .. now finally delete the empty
	remove_child(pNode);
	pNode->queue_delete();

	return true;
}

void LRoomManager::FrameUpdate()
{
	if (Engine::get_singleton()->is_editor_hint())
	{
		WARN_PRINT_ONCE("LRoomManager::FrameUpdate should not be called in editor");
		return;
	}

	m_uiFrameCounter++;

	// get the camera desired and make into lcamera
	Camera * pCamera = 0;
	if (m_cameraID)
	{
		Object *pObj = ObjectDB::get_instance(m_cameraID);
		pCamera = Object::cast_to<Camera>(pObj);
	}
	else
		// camera not set
		return;

	// camera not a camera??
	if (!pCamera)
		return;

	// if not started
//	if (!m_room_curr)
//		return;

	// determine visibility
//	Object *pObj = ObjectDB::get_instance(m_room_curr);
	LRoom * pRoom = GetRoomFromDOB(pCamera);
//	Node * pObj = pCamera->get_parent();

//	if (!pObj)
//		return;

//	LRoom * pRoom = Object::cast_to<LRoom>(pObj);
	if (!pRoom)
	{
		WARN_PRINT_ONCE("LRoomManager::FrameUpdate : curr room is not an LRoom");
		//print_line("LRoomManager::FrameUpdate : curr room is not an LRoom");
//		m_room_curr = 0;
		return;
	}

	m_BF_visible_rooms.Blank();

	LCamera cam;
	cam.m_ptPos = Vector3(0, 0, 0);
	cam.m_ptDir = Vector3 (-1, 0, 0);

	// reset the pool for another frame
	m_Pool.Reset();
	unsigned int pool_member = m_Pool.Request();
	assert (pool_member != -1);

	LVector<Plane> &planes = m_Pool.Get(pool_member);
	planes.clear();

	// get the camera desired and make into lcamera
	assert (pCamera);
//	if (pCamera)
	{
		Transform tr = pCamera->get_global_transform();
		cam.m_ptPos = tr.origin;
		cam.m_ptDir = tr.basis.get_row(2); // or possibly get_axis .. z is what we want

		planes.copy_from(pCamera->get_frustum());
	}

	pRoom->DetermineVisibility_Recursive(*this, 0, cam, planes, m_BF_visible_rooms);


	// finally hide all the rooms that are currently visible but not in the visible bitfield as having been hit
	// NOTE this could be more efficient
	for (int n=0; n<m_room_IDs.size(); n++)
	{
		Object *pObj = ObjectDB::get_instance(m_room_IDs[n]);

		LRoom * pRoom = Object::cast_to<LRoom>(pObj);
		if (pRoom)
		{
			if (!m_BF_visible_rooms.GetBit(n))
			{
				pRoom->hide();
			}
		}
	}

	LPortal::m_bRunning = true;


	// only do once for now
//	m_room_curr = 0;
}


void LRoomManager::_notification(int p_what) {

	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
//			bool bVisible = is_visible_in_tree();
//			ChangeFlags(SF_INVISIBLE, bVisible == false);
//			SetProcessing();
			if (!Engine::get_singleton()->is_editor_hint())
				set_process_internal(true);
			else
				set_process_internal(false);

//			// we can't translate string name of Target to a node until we are in the tree
//			ResolveTargetPath();
		} break;
//	case NOTIFICATION_INTERNAL_PHYSICS_PROCESS: {
//			FixedUpdate();
//		} break;
	case NOTIFICATION_INTERNAL_PROCESS: {
			FrameUpdate();
		} break;
//	case NOTIFICATION_VISIBILITY_CHANGED: {
//			bool bVisible = is_visible_in_tree();
//			ChangeFlags(SF_INVISIBLE, bVisible == false);
//			SetProcessing();
////			if (bVisible)
////				print_line("now visible");
////			else
////				print_line("now hidden");
//		} break;
	}
}


void LRoomManager::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("rooms_convert"), &LRoomManager::convert);
	ClassDB::bind_method(D_METHOD("rooms_set_camera"), &LRoomManager::set_camera);
//	ClassDB::bind_method(D_METHOD("update_object"), &LRoomManager::update_object);


	ClassDB::bind_method(D_METHOD("dob_register"), &LRoomManager::register_dob);
	ClassDB::bind_method(D_METHOD("dob_unregister"), &LRoomManager::unregister_dob);
	ClassDB::bind_method(D_METHOD("dob_update"), &LRoomManager::update_dob);
	ClassDB::bind_method(D_METHOD("dob_teleport"), &LRoomManager::teleport_dob);

}
