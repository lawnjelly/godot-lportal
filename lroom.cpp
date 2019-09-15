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

#include "lroom.h"
#include "core/engine.h"
#include "scene/3d/mesh_instance.h"
#include "lportal.h"
#include "lbitfield_dynamic.h"
#include "lroom_manager.h"

void LRoom::print(String sz)
{
	LPortal::print(sz);
}

LRoom::LRoom() {
	m_RoomID = -1;
	m_uiFrameTouched = 0;
	m_iFirstPortal = 0;
	m_iNumPortals = 0;
}




Spatial * LRoom::GetGodotRoom() const
{
	Object *pObj = ObjectDB::get_instance(m_GodotID);

	// assuming is a portal
	Spatial * pSpat = Object::cast_to<Spatial>(pObj);

	return pSpat;
}



void LRoom::AddDOB(Spatial * pDOB)
{
	LDob dob;
	dob.m_ID = pDOB->get_instance_id();

	m_DOBs.push_back(dob);
}

bool LRoom::RemoveDOB(Node * pDOB)
{
	ObjectID id = pDOB->get_instance_id();

	for (int n=0; n<m_DOBs.size(); n++)
	{
		if (m_DOBs[n].m_ID == id)
		{
			m_DOBs.remove(n);
			return true;
		}
	}

	return false;
}


// returns -1 if no change, or the linked room we are moving into
LRoom * LRoom::UpdateDOB(LRoomManager &manager, Spatial * pDOB)
{
	const Vector3 &pt = pDOB->get_global_transform().origin;

	const float slop = 0.2f;

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
			print("DOB at pos " + pt + " ahead of portal " + port.get_name() + " by " + String(Variant(dist)));

			// we want to move into the adjoining room
			return &manager.Portal_GetLinkedRoom(port);
		}
	}

	return 0;
}


void LRoom::DetermineVisibility_Recursive(LRoomManager &manager, int depth, const LCamera &cam, const LVector<Plane> &planes, int portalID_from)
{
	// prevent too much depth
	if (depth >= 8)
	{
		print("\t\t\tDEPTH LIMIT REACHED");
		return;
	}

	print("DetermineVisibility_Recursive from " + get_name());

	// set the frame counter
	assert (manager.m_uiFrameCounter > m_uiFrameTouched);
	m_uiFrameTouched = manager.m_uiFrameCounter;

	// show this room and add to visible list of rooms
	GetGodotRoom()->show();
	manager.m_BF_visible_rooms.SetBit(m_RoomID, true);

	// clip all objects in this room to the clipping planes
	for (int n=0; n<m_SOBs.size(); n++)
	{
		const LSob sob = m_SOBs[n];
		Object * pNode = ObjectDB::get_instance(sob.m_ID);

		VisualInstance * pObj = Object::cast_to<VisualInstance>(pNode);

		// should always be a visual instance, only these are added as SOBs
		if (pObj)
		{
			//Vector3 pt = pObj->get_global_transform().origin;

			bool bShow = true;


			// estimate the radius .. for now
			AABB bb = pObj->get_transformed_aabb();

			print("\t\t\tculling object " + pObj->get_name());

			for (int p=0; p<planes.size(); p++)
			{
//				float dist = planes[p].distance_to(pt);
//				print("\t\t\t\t" + itos(p) + " : dist " + String(Variant(dist)));

				float r_min, r_max;
				bb.project_range_in_plane(planes[p], r_min, r_max);

				print("\t\t\t\t" + itos(p) + " : r_min " + String(Variant(r_min)) + ", r_max " + String(Variant(r_max)));


				if (r_min > 0.0f)
				{
					bShow = false;
					break;
				}
			}

			if (bShow)
				pObj->show();
			else
				pObj->hide();

		}
	}


	for (int p=0; p<m_iNumPortals; p++)
	{
		int port_id = m_iFirstPortal + p;

		// ignore if the portal we are looking in from
		if (port_id == portalID_from)
			continue;

		const LPortal &port = manager.m_Portals[port_id];

		// have we already handled the room on this frame?
		// get the room pointed to by the portal
		LRoom * pLinkedRoom = &manager.Portal_GetLinkedRoom(port);

		if (pLinkedRoom->m_uiFrameTouched == manager.m_uiFrameCounter)
			continue;

//		const Vector3 &portal_normal = pPortal->m_Plane.normal;
//		print("\ttesting portal " + pPortal->get_name() + " normal " + portal_normal);

		// direction with the camera? (might not need to check)
//		float dot = cam.m_ptDir.dot(portal_normal);
//		if (dot <= -0.0f) // 0.0
//		{
//			Variant vd = dot;
//			print("\t\tportal culled (wrong direction) dot is " + String(vd));
//			continue;
//		}

		// is it culled by the planes?
		LPortal::eClipResult overall_res = LPortal::eClipResult::CLIP_INSIDE;

		// for portals, we want to ignore the near clipping plane, as we might be right on the edge of a doorway
		// and still want to look through the portal.
		// So we are starting this loop from 1, ASSUMING that plane zero is the near clipping plane.
		// If it isn't we would need a different strategy
		for (int l=1; l<planes.size(); l++)
		{
			LPortal::eClipResult res = port.ClipWithPlane(planes[l]);

			switch (res)
			{
			case LPortal::eClipResult::CLIP_OUTSIDE:
				overall_res = res;
				break;
			case LPortal::eClipResult::CLIP_PARTIAL:
				overall_res = res;
				break;
			}

			if (overall_res == LPortal::eClipResult::CLIP_OUTSIDE)
				break;
		}

		// this portal is culled
		if (overall_res == LPortal::eClipResult::CLIP_OUTSIDE)
		{
			print("\t\tportal culled (outside planes)");
			continue;
		}

		// else recurse into that portal
		unsigned int uiPoolMem = manager.m_Pool.Request();
		if (uiPoolMem != -1)
		{
			// get a vector of planes from the pool
			LVector<Plane> &new_planes = manager.m_Pool.Get(uiPoolMem);

			// copy the existing planes
			new_planes.copy_from(planes);

			// add the planes for the portal
			port.AddPlanes(cam.m_ptPos, new_planes);

			if (pLinkedRoom)
				pLinkedRoom->DetermineVisibility_Recursive(manager, depth + 1, cam, new_planes, port_id);

			// we no longer need these planes
			manager.m_Pool.Free(uiPoolMem);
		}
		else
		{
			// planes pool is empty!
			// This will happen if the view goes through shedloads of portals
			// The solution is either to increase the plane pool size, or build levels
			// with views through multiple portals. Looking through multiple portals is likely to be
			// slow anyway because of the number of planes to test.
			WARN_PRINT_ONCE("Planes pool is empty");
		}
	}
}


