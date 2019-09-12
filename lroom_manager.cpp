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

LRoomManager::LRoomManager()
{
	m_room_curr = 0;
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
			m_room_IDs.push_back(pRoom->get_instance_id());
	}

	m_room_curr = 0;

	// just set current room to first room
	if (m_room_IDs.size())
	{
		m_room_curr = m_room_IDs[0];
		print_line("first room ID is " + itos(m_room_curr));
	}
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

	LCamera cam;
	cam.m_ptPos = Vector3(0, 0, 0);
	cam.m_ptDir = Vector3 (-1, 0, 0);

	Vector<Plane> planes;

	pRoom->DetermineVisibility_Recursive(0, cam, planes);



	// only do once for now
	m_room_curr = 0;
}


void LRoomManager::_notification(int p_what) {

	switch (p_what) {
	case NOTIFICATION_ENTER_TREE: {
//			bool bVisible = is_visible_in_tree();
//			ChangeFlags(SF_INVISIBLE, bVisible == false);
//			SetProcessing();
			if (!Engine::get_singleton()->is_editor_hint())
				set_process_internal(true);

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

}
