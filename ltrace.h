#pragma once

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

#include "lvector.h"

class LSource;
class LRoomManager;
class LRoom;
class LLight;
namespace Lawn {class LBitField_Dynamic;}

class LTrace
{
public:
	enum eTraceFlags
	{
		CULL_SOBS = 1 << 0,
		CULL_DOBS = 1 << 1,
		TOUCH_ROOMS = 1 << 2,
		MAKE_ROOM_VISIBLE = 1 << 3,
		DONT_TRACE_PORTALS = 1 << 4,
	};

	enum eLightRun
	{
		LR_ALL, // runtime find all shadow casters
		LR_ROOMS, // find affected rooms
		LR_CONVERT, // initial conversion
	};

	void Trace_Prepare(LRoomManager &manager, const LSource &cam, Lawn::LBitField_Dynamic &BF_SOBs, Lawn::LBitField_Dynamic &BF_Rooms, LVector<int> &visible_SOBs, LVector<int> &visible_Rooms);
//	void Trace_Prepare(LRoomManager &manager, const LCamera &cam, Lawn::LBitField_Dynamic &BF_SOBs, Lawn::LBitField_Dynamic &BF_DOBs, Lawn::LBitField_Dynamic &BF_Rooms, LVector<int> &visible_SOBs, LVector<int> &visible_DOBs, LVector<int> &visible_Rooms);

	void Trace_SetFlags(unsigned int flags) {m_TraceFlags = flags;}
	void Trace_Begin(LRoom &room, LVector<Plane> &planes);

	// simpler method of doing a trace for lights, no need to call prepare and begin
	bool Trace_Light(LRoomManager &manager, const LLight &light, eLightRun eRun);

private:
	void AddSpotlightPlanes(LVector<Plane> &planes) const;
	void Trace_Recursive(int depth, LRoom &room, const LVector<Plane> &planes, int first_portal_plane);

	void CullSOBs(LRoom &room, const LVector<Plane> &planes);
	void CullDOBs(LRoom &room, const LVector<Plane> &planes);
	void FirstTouch(LRoom &room);
	void DetectFirstTouch(LRoom &room);


	LRoomManager * m_pManager;
	const LSource * m_pCamera;

	Lawn::LBitField_Dynamic * m_pBF_SOBs;
	//Lawn::LBitField_Dynamic * m_pBF_DOBs;
	Lawn::LBitField_Dynamic * m_pBF_Rooms;

	LVector<int> * m_pVisible_SOBs;
	//LVector<int> * m_pVisible_DOBs;
	LVector<int> * m_pVisible_Rooms;

	unsigned int m_TraceFlags;
};
