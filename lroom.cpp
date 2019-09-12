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
#include "CoBitField_Dynamic.h"
#include "lroom_manager.h"

void LRoom::print(String sz)
{
//	print_line(sz);
	LPortal::print(sz);
}

LRoom::LRoom() {
	m_LocalRoomID = -1;
}

void LRoom::DetermineVisibility_Recursive(LRoomManager &manager, int depth, const LCamera &cam, const LVector<Plane> &planes, Core::CoBitField_Dynamic &BF_visible, ObjectID portalID_from)
{
	// prevent too much depth
	if (depth >= 8)
	{
		print("\t\t\tDEPTH LIMIT REACHED");
		return;
	}

	print("DetermineVisibility_Recursive from " + get_name());

	// show this room and add to visible list of rooms
	show();
	BF_visible.SetBit(m_LocalRoomID, true);

	// clip all objects in this room to the clipping planes
	for (int n=0; n<get_child_count(); n++)
	{
		// ignore portals
		Node * pNode = get_child(n);
		LPortal * pPortal = Object::cast_to<LPortal>(pNode);
		if (pPortal)
			continue;

		VisualInstance * pObj = Object::cast_to<VisualInstance>(pNode);
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




	// go through each portal out of here
	int nPortals = m_portal_IDs.size();

//	ObjectID this_room_id = get_instance_id();

	for (int p=0; p<nPortals; p++)
	{
		ObjectID id = m_portal_IDs[p];

		// ignore if the portal we are looking in from
		if (id == portalID_from)
			continue;

		// get the portal
		Object *pObj = ObjectDB::get_instance(id);

		// assuming is a portal
		LPortal * pPortal = Object::cast_to<LPortal>(pObj);

		if (!pPortal)
		{
			WARN_PRINT_ONCE("LRoom::DetermineVisibility_Recursive : Not a portal");
			continue;
		}

		const Vector3 &portal_normal = pPortal->m_Plane.normal;
		print("\ttesting portal " + pPortal->get_name() + " normal " + portal_normal);

		// direction with the camera? (might not need to check)
		float dot = cam.m_ptDir.dot(portal_normal);
		if (dot <= 0.0f)
		{
			Variant vd = dot;
			print("\t\tportal culled (wrong direction) dot is " + String(vd));
			continue;
		}

		// is it culled by the planes?
		LPortal::eClipResult overall_res = LPortal::eClipResult::CLIP_INSIDE;

		for (int l=0; l<planes.size(); l++)
		{
			LPortal::eClipResult res = pPortal->ClipWithPlane(planes[l]);

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
			pPortal->AddPlanes(cam.m_ptPos, new_planes);

			// get the room pointed to by the portal
			LRoom * pLinkedRoom = pPortal->GetLinkedRoom();
			if (pLinkedRoom)
				pLinkedRoom->DetermineVisibility_Recursive(manager, depth + 1, cam, new_planes, BF_visible, id);

			manager.m_Pool.Free(uiPoolMem);
		}
		else
		{
			// planes pool is empty!
			WARN_PRINT_ONCE("Planes pool is empty");
		}
	}
}



// initial setup, allows importing portals as meshes from modelling program,
// which will be auto converted to LPortals with this method
void LRoom::DetectPortalMeshes()
{
	print("DetectPortalMeshes");

	bool bFoundOne = true;

	while (bFoundOne)
	{
		bFoundOne = false;

		for (int n=0; n<get_child_count(); n++)
		{
			Node * pChild = get_child(n);

			MeshInstance * pMesh = Object::cast_to<MeshInstance>(pChild);
			if (pMesh)
			{
				// name must start with portal_
				if (LPortal::NameStartsWith(pMesh, "portal_"))
				{
					String szLinkRoom = LPortal::FindNameAfter(pMesh, "portal_");
					DetectedPortalMesh(pMesh, szLinkRoom);
					bFoundOne = true;
				}
			}

			if (bFoundOne)
				break;
		}
	}

}

void LRoom::DetectedPortalMesh(MeshInstance * pMeshInstance, String szLinkRoom)
{
	print("\tDetected PortalMesh");

	Ref<Mesh> rmesh = pMeshInstance->get_mesh();

	Array arrays = rmesh->surface_get_arrays(0);
	PoolVector<Vector3> p_vertices = arrays[VS::ARRAY_VERTEX];

	LPortal * pNew = memnew(LPortal);
	pNew->set_name("lportal_");
	add_child(pNew);

	pNew->set_transform(pMeshInstance->get_transform());

	NodePath temppath = "../../" + szLinkRoom;
	pNew->AddRoom(temppath);

	// create the portal geometry
	pNew->CreateGeometry(p_vertices);

	// delete the original child
	pMeshInstance->get_parent()->remove_child(pMeshInstance);

	pMeshInstance->queue_delete();
}

void LRoom::MakePortalsTwoWay()
{
	for (int n=0; n<get_child_count(); n++)
	{
		Node * pChild = get_child(n);

		LPortal * pPortal = Object::cast_to<LPortal>(pChild);
		if (pPortal)
		{
			pPortal->Link(this);
		}
	}

}

// assuming that portals are a child of the room, detect these and make them 2 way
void LRoom::MakePortalQuickList()
{
	// this function could be called more than one time...
	m_portal_IDs.clear();

	for (int n=0; n<get_child_count(); n++)
	{
		Node * pChild = get_child(n);

		LPortal * pPortal = Object::cast_to<LPortal>(pChild);
		if (pPortal)
		{
			ObjectID id = pPortal->get_instance_id();
			m_portal_IDs.push_back(id);
		}
	}
}

// make sure there is a back (opposite) portal leading from the portal from to the roomto, from this room
void LRoom::MakeOppositePortal(LPortal * pPortalFrom, LRoom * pRoomTo)
{
	ObjectID room_to_id = pRoomTo->get_instance_id();

	// does an opposite portal exist already?
	for (int n=0; n<get_child_count(); n++)
	{
		Node * pChild = get_child(n);

		LPortal * pPortal = Object::cast_to<LPortal>(pChild);
		if (pPortal)
		{
			if (pPortal->GetLinkedRoomID() == room_to_id)
				// already linked
				return;
		}
	}

	// if we got to here it isn't linked...
	// add a new portal
	// NYI
	LPortal * pNew = memnew(LPortal);
	pNew->set_name("lportal");
	add_child(pNew);

	pNew->AddRoom(pRoomTo->get_path());
	pNew->CopyReversedGeometry(*pPortalFrom);
}


void LRoom::_bind_methods() {

}


