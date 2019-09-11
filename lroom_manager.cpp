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

LRoomManager::LRoomManager()
{

}


// convert empties and meshes to rooms and portals
void LRoomManager::Convert()
{
	Convert_Rooms();
	Convert_Portals();
}

void LRoomManager::Convert_Rooms()
{
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

			if (LPortal::NameStartsWith(pChild, "room_"))
			{
				if (Convert_Room(pChild))
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


bool LRoomManager::Convert_Room(Node * pNode)
{
	// get the room part of the name
	String szFullName = pNode->get_name();
	String szRoom = LPortal::FindNameAfter(pNode, 6);

	// create a new LRoom to exchange the children over to, and delete the original empty
	LRoom * pNew = memnew(LRoom);
	pNew->set_name(szRoom);
	add_child(pNew);

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


void LRoomManager::_bind_methods()
{

}
