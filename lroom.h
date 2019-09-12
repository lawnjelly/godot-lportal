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

/* room.h */
#ifndef LROOM_H
#define LROOM_H

/**
	@author lawnjelly <lawnjelly@gmail.com>
*/


#include "scene/3d/spatial.h"


// Smooth node allows fixed timestep interpolation without having to write any code.
// It requires a proxy node (which is moved on physics tick), e.g. a rigid body or manually moved spatial..
// and instead of having MeshInstance as a child of this, you add Smooth node to another part of the scene graph,
// make the MeshInstance a child of the smooth node, then choose the proxy as the target for the smooth node.

// Note that in the special case of manually moving the proxy to a completely new location, you should call
// 'teleport' on the smooth node after setting the proxy node transform. This will ensure that the current AND
// previous transform records are reset, so it moves instantaneously.

class LPortal;
class MeshInstance;

class LCamera
{
public:
	// all in world space, culling done in world space
	Vector3 m_ptPos;
	Vector3 m_ptDir;
};


class LRoom : public Spatial {
	GDCLASS(LRoom, Spatial);

	friend class LPortal;
private:
	// a quick list of object IDs of child portals of this room
	Vector<ObjectID> m_portal_IDs;

protected:
	static void _bind_methods();

public:
	// initial setup, allows importing portals as meshes from modelling program,
	// which will be auto converted to LPortals with this method
	void DetectPortalMeshes();
	// assuming that portals are a child of the room, detect these and make them 2 way
	void MakePortalsTwoWay();
	void MakePortalQuickList();

	// main function
	void DetermineVisibility_Recursive(int depth, const LCamera &cam, const Vector<Plane> &planes, ObjectID portalID_from = 0);

// specific
public:
	LRoom();

private:


	//	void SetupPortal(LPortal * pPortal);
	void MakeOppositePortal(LPortal * pPortalFrom, LRoom * pRoomTo);
	void DetectedPortalMesh(MeshInstance * pMeshInstance, String szLinkRoom);
};


#endif
