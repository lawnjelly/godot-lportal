#include "lhelper.h"
#include "ldebug.h"
#include "scene/3d/baked_lightmap.h"
#include "scene/3d/mesh_instance.h"

#ifdef TOOLS_ENABLED
#include "thirdparty/xatlas/xatlas.h"
#endif

// for ::free
#include <stdlib.h>

LHelper::LHelper()
{
}




void LHelper::SetOwnerRecursive(Node * pNode, Node * pOwner)
{
	pNode->set_owner(pOwner);

	for (int n=0; n<pNode->get_child_count(); n++)
	{
		SetOwnerRecursive(pNode->get_child(n), pOwner);
	}
}




void LHelper::Transform_Verts(const PoolVector<Vector3> &ptsLocal, PoolVector<Vector3> &ptsWorld, const Transform &tr) const
{
	for (int n=0; n<ptsLocal.size(); n++)
	{
		Vector3 ptWorld = tr.xform(ptsLocal[n]);
		ptsWorld.push_back(ptWorld);
	}
}

void LHelper::Transform_Norms(const PoolVector<Vector3> &normsLocal, PoolVector<Vector3> &normsWorld, const Transform &tr) const
{
	for (int n=0; n<normsLocal.size(); n++)
	{
		// hacky way for normals, we should use transpose of inverse matrix, dunno if godot supports this
		Vector3 ptNormA = Vector3(0, 0, 0);
		Vector3 ptNormWorldA = tr.xform(ptNormA);
		Vector3 ptNormWorldB = tr.xform(normsLocal[n]);

		Vector3 ptNorm = ptNormWorldB - ptNormWorldA;

		ptNorm = ptNorm.normalized();

		normsWorld.push_back(ptNorm);
	}
}



