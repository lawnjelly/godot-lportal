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
#include "lvector.h"
#include "ldob.h"


namespace Lawn {class LBitField_Dynamic;}

class LPortal;
class LRoomManager;
class MeshInstance;

class LCamera
{
public:
	// all in world space, culling done in world space
	Vector3 m_ptPos;
	Vector3 m_ptDir;
};



class LRoom
{
	friend class LPortal;
	friend class LRoomManager;
	friend class LRoomConverter;
private:

	// static objects
	LVector<LSob> m_SOBs;

	// dynamic objects
	Vector<LDob> m_DOBs;

	// portals are stored in the manager in a contiguous list
	int m_iFirstPortal;
	int m_iNumPortals;

	// Just very rough, room centre for determining start rooms of dobs
	Vector3 m_ptCentre;
	AABB m_AABB; // world bound

	// ID in the Room Manager, NOT the godot object ID
	int m_RoomID;

	ObjectID m_GodotID;

	// frame counter when last touched .. prevents handling rooms multiple times
	unsigned int m_uiFrameTouched;

	String m_szName;

public:
	const String &get_name() const {return m_szName;}

protected:
	// main function
	void DetermineVisibility_Recursive(LRoomManager &manager, int depth, const LCamera &cam, const LVector<Plane> &planes, int portalID_from = -1);
	void FirstTouch(LRoomManager &manager);


	void DOB_Add(Spatial * pDOB);
	bool DOB_Remove(Node * pDOB);
	LRoom * DOB_Update(LRoomManager &manager, Spatial * pDOB);

public:
	LRoom();
	Spatial * GetGodotRoom() const;

private:
	static void print(String sz);
};


#endif
