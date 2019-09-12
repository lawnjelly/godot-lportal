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

LRoomManager::LRoomManager()
{
	m_room_curr = 0;
	m_cameraID = 0;
}


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
}

// convert empties and meshes to rooms and portals
void LRoomManager::convert()
{
	print_line("running convert");

	Convert_Rooms();
	Convert_Portals();
	Find_Rooms();
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

	m_room_curr = 0;

	// just set current room to first room
	if (m_room_IDs.size())
	{
		m_room_curr = m_room_IDs[0];
		print_line("first room ID is " + itos(m_room_curr));
	}

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

	int nChildren = pNode->get_child_count();

	for (int n=0; n<nChildren; n++)
	{
		// reverse count
		int c = nChildren - n - 1;

		Node * pChild = pNode->get_child(c);
		pNode->remove_child(pChild);

		// add the child to the new node
		pNew->add_child(pChild);
	}

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

	// if not started
	if (!m_room_curr)
		return;

	// determine visibility
	Object *pObj = ObjectDB::get_instance(m_room_curr);

	if (!pObj)
		return;

	LRoom * pRoom = Object::cast_to<LRoom>(pObj);
	if (!pRoom)
	{
		WARN_PRINT_ONCE("LRoomManager::FrameUpdate : curr room is not an LRoom");
		print_line("curr room is not an LRoom");
		m_room_curr = 0;
		return;
	}

	m_BF_visible_rooms.Blank();

	LCamera cam;
	cam.m_ptPos = Vector3(0, 0, 0);
	cam.m_ptDir = Vector3 (-1, 0, 0);

	Vector<Plane> planes;

	// get the camera desired and make into lcamera
	if (m_cameraID)
	{
		Object *pObj = ObjectDB::get_instance(m_cameraID);

		Camera * pCamera = Object::cast_to<Camera>(pObj);
		if (pCamera)
		{
			Transform tr = pCamera->get_global_transform();
			cam.m_ptPos = tr.origin;
			cam.m_ptDir = tr.basis.get_row(2); // or possibly get_axis .. z is what we want

			planes = pCamera->get_frustum();
		}
	}

	pRoom->DetermineVisibility_Recursive(0, cam, planes, m_BF_visible_rooms);


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
	ClassDB::bind_method(D_METHOD("convert"), &LRoomManager::convert);
	ClassDB::bind_method(D_METHOD("set_camera"), &LRoomManager::set_camera);

}
