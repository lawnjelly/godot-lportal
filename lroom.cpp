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


LRoom::LRoom() {
}

void LRoom::DetermineVisibility_Recursive(int depth, const LCamera &cam, const Vector<Plane> &planes, ObjectID portalID_from)
{
	// prevent too much depth
	if (depth >= 8)
	{
		print_line("\t\t\tDEPTH LIMIT REACHED");
		return;
	}

	print_line("DetermineVisibility_Recursive from " + get_name());

	// clip all objects in this room to the clipping planes
	// NYI

	// go through each portal out of here
	int nPortals = m_portal_IDs.size();

	ObjectID this_room_id = get_instance_id();

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
		print_line("\ttesting portal " + pPortal->get_name() + " normal " + portal_normal);

		// direction with the camera? (might not need to check)
		float dot = cam.m_ptDir.dot(portal_normal);
		if (dot <= 0.0f)
		{
			Variant vd = dot;
			print_line("\t\tportal culled (wrong direction) dot is " + String(vd));
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
			print_line("\t\tportal culled (outside planes)");
			continue;
		}

		// else recurse into that portal
		Vector<Plane> new_planes = planes;

		// add the planes for the portal
		// NYI

		// get the room pointed to by the portal
		LRoom * pLinkedRoom = pPortal->GetLinkedRoom();
		if (pLinkedRoom)
			pLinkedRoom->DetermineVisibility_Recursive(depth + 1, cam, new_planes, id);
	}
}



// initial setup, allows importing portals as meshes from modelling program,
// which will be auto converted to LPortals with this method
void LRoom::DetectPortalMeshes()
{
	print_line("DetectPortalMeshes");

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
	print_line("\tDetected PortalMesh");

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


