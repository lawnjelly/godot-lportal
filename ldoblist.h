#pragma once

#include "lvector.h"
#include "ldob.h"

class LRoomManager;

class LDobList
{
public:
	// getting
	LDob &GetDob(int n) {return m_List[n];}
	const LDob &GetDob(int n) const {return m_List[n];}

	// request delete
	int Request();
	void DeleteDob(int id);

	// funcs
	int UpdateDob(LRoomManager &manager, int dob_id, const Vector3 &pos);
	void UpdateVisibility(LRoomManager &manager, Spatial * pDOBSpatial, int dob_id);

private:
	bool FindDOBOldAndNewRoom(LRoomManager &manager, int dob_id, const Vector3 &pos, int &old_room_id, int &new_room_id);

	LVector<LDob> m_List;
};


/////////////////////////////////////////////////////////

inline void LDobList::DeleteDob(int id)
{
	GetDob(id).m_bSlotTaken = false;
}


inline int LDobList::Request()
{
	for (int n=0; n<m_List.size(); n++)
	{
		LDob &d = m_List[n];
		if (d.m_bSlotTaken == false)
		{
			d.m_bSlotTaken = true;
			return n;
		}
	}

	// none found, create new
	LDob * p = m_List.request();
	memset(p, 0, sizeof (LDob));

	p->m_bSlotTaken = true;
	p->m_iRoomID = -1;

	return m_List.size()-1;
}
