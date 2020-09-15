#pragma once

#include "lroom_manager.h"

class LHelper
{
public:
	LHelper();



private:
	void Transform_Verts(const PoolVector<Vector3> &ptsLocal, PoolVector<Vector3> &ptsWorld, const Transform &tr) const;
	void Transform_Norms(const PoolVector<Vector3> &normsLocal, PoolVector<Vector3> &normsWorld, const Transform &tr) const;


	void SetOwnerRecursive(Node * pNode, Node * pOwner);


};
