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


#include "lroom_converter.h"
#include "lroom_manager.h"
#include "lportal.h"
#include "scene/3d/mesh_instance.h"

// save typing, I am lazy
#define LMAN m_pManager

void LRoomConverter::print(String sz)
{
	// easy to turn on and off debugging
	print_line(sz);
}


void LRoomConverter::Convert(LRoomManager &manager)
{
	// This just is simply used to set how much debugging output .. more during conversion, less during running
	// except when requested by explicitly clearing this flag.
	LPortal::m_bRunning = false;
	print_line("running convert");

	LMAN = &manager;
	int count = CountRooms();

	// make sure bitfield is right size for number of rooms
	LMAN->m_BF_visible_rooms.Create(count);


	LMAN->m_Rooms.clear(true);
	LMAN->m_Rooms.resize(count);

	m_TempRooms.clear(true);
	m_TempRooms.resize(count);


	Convert_Rooms();
	Convert_Portals();
	LPortal::m_bRunning = true;

	// temp rooms no longer needed
	m_TempRooms.clear(true);
}


void LRoomConverter::Convert_Rooms()
{
	print_line("Convert_Rooms");

	// first find all room empties and convert to LRooms
	int count = 0;

	for (int n=0; n<LMAN->get_child_count(); n++)
	{
		Node * pChild = LMAN->get_child(n);

		if (!Node_IsRoom(pChild))
			continue;

		Spatial * pSpat = Object::cast_to<Spatial>(pChild);
		assert (pSpat);

		Convert_Room(pSpat, count++);
	}

}

int LRoomConverter::FindRoom_ByName(String szName) const
{
	for (int n=0; n<LMAN->m_Rooms.size(); n++)
	{
		if (LMAN->m_Rooms[n].m_szName == 	szName)
			return n;
	}

	return -1;
}

bool LRoomConverter::Convert_Room(Spatial * pNode, int lroomID)
{
	// get the room part of the name
	String szFullName = pNode->get_name();
	String szRoom = LPortal::FindNameAfter(pNode, "room_");

	print_line("Convert_Room : " + szFullName);

	// get a reference to the lroom we are writing to
	LRoom &lroom = LMAN->m_Rooms[lroomID];

	// store the godot room
	lroom.m_GodotID = pNode->get_instance_id();
	lroom.m_RoomID = lroomID;

	// create a new LRoom to exchange the children over to, and delete the original empty
	lroom.m_szName = szRoom;

	// keep a running bounding volume as we go through the visual instances
	// to determine the overall bound of the room
	LAABB bb_room;
	bb_room.SetToMaxOpposite();

	int nChildren = pNode->get_child_count();
	for (int n=0; n<nChildren; n++)
	{
		Node * pChild = pNode->get_child(n);

		VisualInstance * pVI = Object::cast_to<VisualInstance>(pChild);
		if (pVI)
		{
			print("\t\tFound VI : " + pVI->get_name());


			// update bound to find centre of room roughly
			AABB bb = pVI->get_transformed_aabb();
			bb_room.ExpandToEnclose(bb);

			// store some info about the static object for use at runtime
			LSob sob;
			sob.m_ID = pVI->get_instance_id();
			sob.m_aabb = bb;

			lroom.m_SOBs.push_back(sob);
		}
		else
		{
			// not visual instances NYI
		}
	}

	// store the lroom centre and bound
	lroom.m_ptCentre = bb_room.FindCentre();

	// bound (untested)
	lroom.m_AABB.position = bb_room.m_ptMins;
	lroom.m_AABB.size = bb_room.m_ptMaxs - bb_room.m_ptMins;

	print_line("\t" + String(lroom.m_szName) + " centre " + lroom.m_ptCentre);

	return true;
}

void LRoomConverter::Convert_Portals()
{
	for (int pass=0; pass<3; pass++)
	{
		print_line("Convert_Portals pass " + itos(pass));

		for (int n=0; n<LMAN->m_Rooms.size(); n++)
		{
			LRoom &lroom = LMAN->m_Rooms[n];
			LTempRoom &troom = m_TempRooms[n];

			switch (pass)
			{
			case 0:
				LRoom_DetectPortalMeshes(lroom, troom);
				break;
			case 1:
				LRoom_MakePortalsTwoWay(lroom, troom, n);
				break;
			case 2:
				LRoom_MakePortalFinalList(lroom, troom);
				break;
			}

		}

	}
}


int LRoomConverter::CountRooms()
{
	int nChildren = LMAN->get_child_count();
	int count = 0;

	for (int n=0; n<nChildren; n++)
	{
		if (Node_IsRoom(LMAN->get_child(n)))
			count++;
	}

	return count;
}


// go through the nodes hanging off the room looking for those that are meshes to mark portal locations
void LRoomConverter::LRoom_DetectPortalMeshes(LRoom &lroom, LTempRoom &troom)
{
	print("DetectPortalMeshes from room " + lroom.get_name());

	Spatial * pGRoom = lroom.GetGodotRoom();
	assert (pGRoom);


	for (int n=0; n<pGRoom->get_child_count(); n++)
	{
		Node * pChild = pGRoom->get_child(n);

		MeshInstance * pMesh = Object::cast_to<MeshInstance>(pChild);
		if (pMesh)
		{
			// name must start with 'portal_'
			// and ends with the name of the room we want to link to (without the 'room_')
			if (LPortal::NameStartsWith(pMesh, "portal_"))
			{
				String szLinkRoom = LPortal::FindNameAfter(pMesh, "portal_");
				LRoom_DetectedPortalMesh(lroom, troom, pMesh, szLinkRoom);
			}
		}
	}

	// we need an enclosing while loop because we might be deleting children and mucking up the iterator
	bool bDetectedOne = true;

	while (bDetectedOne)
	{
		bDetectedOne = false;

		for (int n=0; n<pGRoom->get_child_count(); n++)
		{
			Node * pChild = pGRoom->get_child(n);

			MeshInstance * pMesh = Object::cast_to<MeshInstance>(pChild);
			if (pMesh)
			{
				// name must start with 'portal_'
				// and ends with the name of the room we want to link to (without the 'room_')
				if (LPortal::NameStartsWith(pMesh, "portal_"))
				{
					// delete the original child, as it is no longer needed at runtime (except maybe for debugging .. NYI?)
					//	pMeshInstance->hide();
					pMesh->get_parent()->remove_child(pMesh);
					pMesh->queue_delete();

					bDetectedOne = true;
				}
			}

			if (bDetectedOne)
				break;
		} // for loop

	} // while

}

// handles the slight faff involved in getting a new portal in the manager contiguous list of portals
LPortal * LRoomConverter::LRoom_RequestNewPortal(LRoom &lroom)
{
	// is this the first portal?
	if (lroom.m_iNumPortals == 0)
		lroom.m_iFirstPortal = LMAN->m_Portals.size();

	lroom.m_iNumPortals++;

	return LMAN->m_Portals.request();
}

// convert the list on each room to a single contiguous list in the manager
void LRoomConverter::LRoom_MakePortalFinalList(LRoom &lroom, LTempRoom &troom)
{
	for (int n=0; n<troom.m_Portals.size(); n++)
	{
		LPortal &lport_final = *LRoom_RequestNewPortal(lroom);
		lport_final = troom.m_Portals[n];
	}
}

// found a portal mesh! create a matching LPortal
void LRoomConverter::LRoom_DetectedPortalMesh(LRoom &lroom, LTempRoom &troom, MeshInstance * pMeshInstance, String szLinkRoom)
{
	print("\tDetected PortalMesh to " + szLinkRoom);

	// which room does this portal want to link to?
	int iLinkRoom = FindRoom_ByName(szLinkRoom);
	if (iLinkRoom == -1)
	{
		print("\t\tWARNING : portal to room " + szLinkRoom + ", room not found");
		//WARN_PRINTS("portal to room " + szLinkRoom + ", room not found");
		return;
	}

	// some godot jiggery pokery to get the mesh verts in local space
	Ref<Mesh> rmesh = pMeshInstance->get_mesh();
	Array arrays = rmesh->surface_get_arrays(0);
	PoolVector<Vector3> p_vertices = arrays[VS::ARRAY_VERTEX];

	// create a new LPortal to fill with this wonderful info
	LPortal &lport = *troom.m_Portals.request();
	lport.m_szName = szLinkRoom;
	lport.m_iRoomNum = iLinkRoom;

	// create the portal geometry
	lport.CreateGeometry(p_vertices, pMeshInstance->get_global_transform());


	print("\t\t\tnum portals now " + itos(troom.m_Portals.size()));
}


// This aims to make life easier for level designers. They only need to make a portal facing one way and LPortal
// will automatically create a mirror portal the other way.
void LRoomConverter::LRoom_MakePortalsTwoWay(LRoom &lroom, LTempRoom &troom, int iRoomNum)
{
	print("LRoomConverter::LRoom_MakePortalsTwoWay from room " + lroom.get_name() + ", contains " + itos (troom.m_Portals.size()) + " portals");
	for (int n=0; n<troom.m_Portals.size(); n++)
	{
		const LPortal &portal_orig = troom.m_Portals[n];
		print("\tconsidering portal " + portal_orig.get_name());

		// only make original portals into mirror portals, to prevent infinite recursion
		if (portal_orig.m_bMirror)
		{
			print ("\t\tis MIRROR, ignoring");
			continue;
		}

		print("\t\tcreating opposite portal");

		// get the temproom this portal is linking to
		LTempRoom &nroom = m_TempRooms[portal_orig.m_iRoomNum];

		// does a portal already exist back to the orig room?
		// NOTE this doesn't cope with multiple portals between pairs of rooms yet.
//		bool bAlreadyLinked =false;

//		for (int p=0; p<nroom.m_Portals.size(); p++)
//		{
//			if (nroom.m_Portals[p].m_iRoomNum == n)
//			{
//				bAlreadyLinked = true;
//				break;
//			}
//		}

//		if (bAlreadyLinked)
//			continue;

		// needs a new reverse link if got to here
		TRoom_MakeOppositePortal(portal_orig, iRoomNum);
	}
}

// There is a need for a mirror portal, let's make one!
void LRoomConverter::TRoom_MakeOppositePortal(const LPortal &port, int iRoomOrig)
{
	LTempRoom &nroom = m_TempRooms[port.m_iRoomNum];
	const LRoom &orig_lroom = LMAN->m_Rooms[iRoomOrig];

	// the new portal should have the name of the room the original came from
	LPortal &new_port = *nroom.m_Portals.request();
	new_port.m_szName = orig_lroom.m_szName;
	new_port.m_iRoomNum = iRoomOrig;
	new_port.m_bMirror = true;

	// the portal vertices should be the same but reversed (to flip the normal)
	new_port.CopyReversedGeometry(port);
}


///////////////////////////////////////////////////

// helper
bool LRoomConverter::Node_IsRoom(Node * pNode) const
{
	Spatial * pSpat = Object::cast_to<Spatial>(pNode);
	if (!pSpat)
		return false;

	if (LPortal::NameStartsWith(pSpat, "room_"))
		return true;

	return false;
}



// keep the global namespace clean
#undef LMAN
