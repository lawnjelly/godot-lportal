#include "lhelper.h"
#include "ldebug.h"
#include "scene/3d/baked_lightmap.h"
#include "scene/3d/mesh_instance.h"
#include "thirdparty/xatlas/xatlas.h"

// for ::free
#include <stdlib.h>

LHelper::LHelper()
{
	SetUnMergeParams(0.1f, 0.95f);
}


String LHelper::LFace::ToString() const
{
	String sz;

	for (int c=0; c<3; c++)
	{
		sz += String(Variant(m_Pos[c]));
		sz += ", ";
	}

	sz += "norm : ";
	for (int c=0; c<3; c++)
	{
		sz += String(Variant(m_Norm[c]));
		sz += ", ";
	}

	return sz;
}


bool LHelper::LVert::ApproxEqual(const LVert &o) const
{
	if (m_Pos != o.m_Pos)
		return false;
	if (m_Norm != o.m_Norm)
		return false;
	if (m_UV != o.m_UV)
		return false;
	if (m_UV2 != o.m_UV2)
		return false;

	return true;
}


//////////////////////////////////////////
/*
bool LHelper::UnMergeSOBs(LRoomManager &manager, const PoolVector<Vector2> &uv2s)
{
	int uv_count = 0;

	// go through each sob mesh
	for (int n=0; n<manager.m_SOBs.size(); n++)
	{
		LSob &sob = manager.m_SOBs[n];
		GeometryInstance * pGI = sob.GetGI();
		if (!pGI)
			continue;

		MeshInstance * pMI = Object::cast_to<MeshInstance>(pGI);
		if (!pMI)
			continue;

//		if (UnMerge_SOB(*pMI, uv2s, uv_count) == false)
//			return false;
	}

	return true;
}
*/

// main function for getting merged uv2 back to sobs
bool LHelper::TransferUV2(const MeshInstance &mi_from, MeshInstance &mi_to)
{
	LMerged merged;
	if (!FillMergedFromMesh(merged, mi_from))
		return false;

	return UnMerge_SOB(mi_to, merged);
}


bool LHelper::FillMergedFromMesh(LMerged &merged, const MeshInstance &mesh)
{
	Ref<Mesh> rmesh = mesh.get_mesh();
	Array arrays = rmesh->surface_get_arrays(0);


	merged.m_Verts = arrays[VS::ARRAY_VERTEX];
	merged.m_Norms = arrays[VS::ARRAY_NORMAL];
	merged.m_UV2s = arrays[VS::ARRAY_TEX_UV2];
	merged.m_Inds = arrays[VS::ARRAY_INDEX];
	//	PoolVector<Vector2> p_UV1s = arrays[VS::ARRAY_TEX_UV];

	merged.m_nFaces = merged.m_Inds.size() / 3;

	if (merged.m_UV2s.size() == 0)
	{
		LPRINT(5, "Merged mesh has no secondary UVs, using primary UVs");

		merged.m_UV2s = arrays[VS::ARRAY_TEX_UV];

		if (merged.m_UV2s.size() == 0)
		{
			LWARN(5, "Merged mesh has no UVs");
			return false;
		}
	}

	int miCount = 0;
	for (int mf=0; mf<merged.m_nFaces; mf++)
	{
		// construct merged lface
		LFace mlf;
		for (int c=0; c<3; c++)
		{
			int ind = merged.m_Inds[miCount++];
			mlf.m_Pos[c] = merged.m_Verts[ind];
			mlf.m_Norm[c] = merged.m_Norms[ind].normalized();
			mlf.m_index[c] = ind;
		}
		merged.m_LFaces.push_back(mlf);
	}

	return true;
}


void LHelper::SetUnMergeParams(float thresh_dist, float thresh_dot)
{
	m_MergeParams.m_fThresholdDist = thresh_dist;
	m_MergeParams.m_fThresholdDist_Squared = thresh_dist * thresh_dist;
	m_MergeParams.m_fThresholdDot = thresh_dot;
}

// take the UV2 coords from the merged mesh and attach these to the SOB meshes
bool LHelper::UnMergeSOBs(LRoomManager &manager, MeshInstance * pMerged)
{
	if (!pMerged)
	{
		LWARN(5, "UnMergeSOBs : Mesh instance is NULL2");
		return false;
	}

	LMerged merged;
	if (!FillMergedFromMesh(merged, *pMerged))
		return false;

	// go through each sob mesh
//	for (int n=1; n<2; n++)
	for (int n=0; n<manager.m_SOBs.size(); n++)
	{
#ifdef LDEBUG_UNMERGE
		LPRINT(5, "Unmerge SOB " + itos(n));
#endif

		LSob &sob = manager.m_SOBs[n];
		GeometryInstance * pGI = sob.GetGI();
		if (!pGI)
			continue;

		MeshInstance * pMI = Object::cast_to<MeshInstance>(pGI);
		if (!pMI)
			continue;

		if (UnMerge_SOB(*pMI, merged) == false)
			return false;
	}


	return true;
}

int LHelper::DebugCountUVs(MeshInstance &mi)
{
	Ref<Mesh> rmesh = mi.get_mesh();
	Array arrays = rmesh->surface_get_arrays(0);
	PoolVector<Vector3> verts = arrays[VS::ARRAY_VERTEX];
	PoolVector<Vector2> uv1s = arrays[VS::ARRAY_TEX_UV];
	PoolVector<Vector2> uv2s = arrays[VS::ARRAY_TEX_UV2];

	LPRINT(5, "Lightmap num verts is " + itos (verts.size()) + "\tUV1s is " + itos (uv1s.size()) + "\tUV2s is " + itos(uv2s.size()));

	return uv2s.size();
}


unsigned int LHelper::FindMatchingVertex(const PoolVector<Vector2> &uvs, const Vector2 &uv1) const
{
	// very slow and inefficient .. thanks xatlas!
	for (int n=0; n<uvs.size(); n++)
	{
		// approx equal (float point error)
		float d = fabs(uvs[n].x - uv1.x);

		if (d < 0.2f)
			return n;
	}

	return -1;
}

bool LHelper::DoFaceVertsApproxMatch(const LFace& sob_f, const LFace &m_face, int c0, int c1, bool bDebug) const
{
	return DoPosNormsApproxMatch(sob_f.m_Pos[c0], sob_f.m_Norm[c0], m_face.m_Pos[c1], m_face.m_Norm[c1], bDebug);
}

bool LHelper::DoPosNormsApproxMatch(const Vector3 &a_pos, const Vector3 &a_norm, const Vector3 &b_pos, const Vector3 &b_norm, bool bDebug) const
{
	bDebug = false;

	float thresh_diff = m_MergeParams.m_fThresholdDist;
	float thresh_diff_squared = m_MergeParams.m_fThresholdDist_Squared;

	float x_diff = fabs (b_pos.x - a_pos.x);
	if (x_diff > thresh_diff)
	{
#ifdef LDEBUG_UNMERGE
		if (bDebug)
			LPRINT(5, "\t\t\t\tRejecting x_diff " + ftos(x_diff));
#endif
		return false;
	}

	float z_diff = fabs (b_pos.z - a_pos.z);
	if (z_diff > thresh_diff)
	{
#ifdef LDEBUG_UNMERGE
		if (bDebug)
			LPRINT(5, "\t\t\t\tRejecting z_diff " + ftos(z_diff));
#endif
		return false;
	}

	float y_diff = fabs (b_pos.y - a_pos.y);
	if (y_diff > thresh_diff)
	{
#ifdef LDEBUG_UNMERGE
		if (bDebug)
			LPRINT(5, "\t\t\t\tRejecting y_diff " + ftos(y_diff));
#endif
		return false;
	}



	Vector3 pos_diff = b_pos - a_pos;
	if (pos_diff.length_squared() > thresh_diff_squared) // 0.1
	{
#ifdef LDEBUG_UNMERGE
		if (bDebug)
			LPRINT(5, "\t\t\t\tRejecting length squared " + ftos(pos_diff.length_squared()));
#endif

		return false;
	}

	// make sure both are normalized
	Vector3 na = a_norm;//.normalized();
	Vector3 nb = b_norm;//.normalized();

	float norm_dot = na.dot(nb);
	if (norm_dot < m_MergeParams.m_fThresholdDot)
	{
#ifdef LDEBUG_UNMERGE
		if (bDebug)
			LPRINT(5, "\t\t\t\tRejecting normal " + ftos(norm_dot) + " na : " + String(na) + ", nb : " + String(nb));
#endif
		return false;
	}

	return true;
}


int LHelper::DoFacesMatch_Offset(const LFace& sob_f, const LFace &m_face, int offset) const
{
#ifdef LDEBUG_UNMERGE
	// debug
	String sz = "\t\tPOSS match sob : ";
	sz += sob_f.ToString();
	sz += "\n\t\t\tmerged : ";
	sz += m_face.ToString();
	LPRINT(2, sz);
#endif


	// does 2nd and third match?
	int offset1 = (offset + 1) % 3;
	if (!DoFaceVertsApproxMatch(sob_f, m_face, 1, offset1, true))
		return -1;

	int offset2 = (offset + 2) % 3;
	if (!DoFaceVertsApproxMatch(sob_f, m_face, 2, offset2, true))
		return -1;

	return offset;
}


// -1 for no match, or 0 for 0 offset match, 1 for +1, 2 for +2 offset match...
int LHelper::DoFacesMatch(const LFace& sob_f, const LFace &m_face) const
{
	// match one
	for (int offset = 0; offset < 3; offset++)
	{
		if (DoFaceVertsApproxMatch(sob_f, m_face, 0, offset, false))
		{
			int res = DoFacesMatch_Offset(sob_f, m_face, offset);

			if (res != -1)
				return res;
		}
	}

	return -1; // no match
}


int LHelper::FindOrAddVert(LVector<LVert> &uni_verts, const LVert &vert) const
{
	for (int n=0; n<uni_verts.size(); n++)
	{
		if (uni_verts[n].ApproxEqual(vert))
			return n;
	}

	// not found .. add to list
	uni_verts.push_back(vert);

	return uni_verts.size() - 1;
}


//bool LHelper::UnMerge_SOB(MeshInstance &mi, const PoolVector<Vector3> merged_verts, const PoolVector<Vector3> merged_norms, const PoolVector<Vector2> &merged_uv2s, const PoolVector<int> &merged_inds, int &vert_count)
bool LHelper::UnMerge_SOB(MeshInstance &mi, LMerged &merged)
{
	//LPRINT(2, "UnMerge_SOB " + mi.get_name());

	Ref<Mesh> rmesh = mi.get_mesh();
	Array arrays = rmesh->surface_get_arrays(0);
	PoolVector<Vector3> verts = arrays[VS::ARRAY_VERTEX];
	PoolVector<Vector3> norms = arrays[VS::ARRAY_NORMAL];
	PoolVector<Vector2> uv1s = arrays[VS::ARRAY_TEX_UV];
	PoolVector<int> inds = arrays[VS::ARRAY_INDEX];

	// we need to get the vert positions and normals from local space to world space to match up with the
	// world space coords in the merged mesh
	PoolVector<Vector3> world_verts;
	PoolVector<Vector3> world_norms;
	Transform trans = mi.get_global_transform();
	Transform_Verts(verts, world_verts, trans);
	Transform_Norms(norms, world_norms, trans);


	// these are the uvs to be filled in the sob
	PoolVector<Vector2> uv2s;
	uv2s.resize(verts.size());

	// for each face in the SOB, attempt to find matching face in the merged mesh
	int nFaces = inds.size() / 3;
	int iCount = 0;

	int nMergedFaces = merged.m_nFaces;


	iCount = 0;

	// the number of unique verts in the UV2 mapped mesh may be HIGHER
	// than the original mesh, because verts with same pos / norm / uv may now have
	// different UV2. So we will be recreating the entire
	// mesh data with a new set of unique verts
	LVector<LVert> UniqueVerts;
	PoolVector<int> UniqueIndices;

	for (int f=0; f<nFaces; f++)
	{
		LFace lf;
		for (int c=0; c<3; c++)
		{
			int ind = inds[iCount++];
			lf.m_Pos[c] = world_verts[ind];
			lf.m_Norm[c] = world_norms[ind];

			lf.m_index[c] = ind;
		}

#ifdef LDEBUG_UNMERGE
		LPRINT(5, "lface : " + lf.ToString());
#endif

		// find matching face
//		int miCount = 0;
		bool bMatchFound = false;

		for (int mf=0; mf<nMergedFaces; mf++)
		{
			// construct merged lface
			const LFace &mlf = merged.m_LFaces[mf];
//			for (int c=0; c<3; c++)
//			{
//				int ind = merged.m_Inds[miCount++];
//				mlf.m_Pos[c] = merged.m_Verts[ind];
//				mlf.m_Norm[c] = merged.m_Norms[ind];

//				mlf.m_index[c] = ind;
//			}

			int match = DoFacesMatch(lf, mlf);
			if (match != -1)
			{
				// we found a match in the merged mesh! transfer the UV2s
				bMatchFound = true;

				//sz += " match found offset " + itos(match);

				// find the corresponding uv2s in the merged mesh face and add them (taking into account offset)
				Vector2 found_uvs[3];
				for (int c=0; c<3; c++)
				{
					found_uvs[c] = merged.m_UV2s[mlf.m_index[c]];
				}

				// add them
				for (int c=0; c<3; c++)
				{
					int which = (c + match)	% 3;

					// index for the uv2 should be the same as the vertex index in the 'to mesh' face
					int ind = lf.m_index[c];
					uv2s.set(ind, found_uvs[which]);

					{
						// construct the unique vert
						LVert uvert;
						int orig_ind = lf.m_index[c];
						uvert.m_Pos = verts[orig_ind];
						uvert.m_Norm = norms[orig_ind];
						uvert.m_UV = uv1s[orig_ind];
						uvert.m_UV2 = found_uvs[which];

						// find it or add to the list
						int ind_uni_vert = FindOrAddVert(UniqueVerts, uvert);

						// add the index to form the triangle list
						UniqueIndices.push_back(ind_uni_vert);
					}
				}

				break;
			}
		}

		// special case
		if (!bMatchFound)
		{
			// add some dummy uv2s
//			uv2s.push_back(Vector2(0, 0));
//			uv2s.push_back(Vector2(0, 0));
//			uv2s.push_back(Vector2(0, 0));

			String sz = "\tface " + itos(f);
			sz += " no match";
			LPRINT(2, sz);
		}

	}

//	LPRINT(2, "UnMerge MI : " + mi.get_name() + "\tFirstVert : " + itos(vert_count) + "\tNumUVs : " + itos(verts.size()));

	// something gone wrong, we are out of sync between the lightmap uvs and the original SOBs
//	if ((verts.size() + vert_count) > merged_uv2s.size())
//	{
//		LWARN(5, "Lightmap and SOBs out of sync, num verts in mesh " + itos(verts.size()) + " total uvs " + itos(merged_uv2s.size()));
//		return false;
//	}

	// uv2 sub array
//	PoolVector<Vector2> uv2s;
//	for (int n=0; n<verts.size(); n++)
//	{
//		// new method, because xatlas screws up the vertex ordering, we have to
//		// resort to manually finding a matching vertex in the output!!
//		//Vector2 orig_uv1 = uv1s[n];
//		Vector2 orig_uv1 = Vector2(vert_count, vert_count);

//		//unsigned int match = FindMatchingVertex(merged_uv1s, orig_uv1);
//		unsigned int match = vert_count++;

//		if (match == -1)
//		{
//			LWARN(2, "Lightmap merged vertex not found " + String(Variant(orig_uv1)));
//		}
//		else
//		{
//			uv2s.append(merged_uv2s[match]);
//		}

//	}

	//arrays[VS::ARRAY_TEX_UV2] = uv2s;

	// rebuild the sob mesh, but now using the new uv2s

//	LPRINT(2, "\t\tOrig Verts: " + itos(verts.size()) + ", Unique Verts: " + itos(UniqueVerts.size()) + ", Num Tris: " + itos(inds.size() / 3));

	// construct unique pool vectors to pass to mesh construction
	PoolVector<Vector3> unique_poss;
	PoolVector<Vector3> unique_norms;
	PoolVector<Vector2> unique_uv1s;
	PoolVector<Vector2> unique_uv2s;

	for (int n=0; n<UniqueVerts.size(); n++)
	{
		const LVert &v = UniqueVerts[n];
		unique_poss.push_back(v.m_Pos);
		unique_norms.push_back(v.m_Norm);
		unique_uv1s.push_back(v.m_UV);
		unique_uv2s.push_back(v.m_UV2);
	}

//	PoolVector<Vector3> norms = arrays[VS::ARRAY_NORMAL];
//	PoolVector<Vector2> uv1s = arrays[VS::ARRAY_TEX_UV];
//	PoolVector<int> inds = arrays[VS::ARRAY_INDEX];


	Ref<Material> mat = mi.get_surface_material(0);


	Ref<ArrayMesh> am_new;
	am_new.instance();
	Array arr;
	arr.resize(Mesh::ARRAY_MAX);
//	arr[Mesh::ARRAY_VERTEX] = verts;
//	arr[Mesh::ARRAY_NORMAL] = norms;
//	arr[Mesh::ARRAY_INDEX] = inds;
//	arr[Mesh::ARRAY_TEX_UV] = uv1s;
//	arr[Mesh::ARRAY_TEX_UV2] = uv2s;
	arr[Mesh::ARRAY_VERTEX] = unique_poss;
	arr[Mesh::ARRAY_NORMAL] = unique_norms;
	arr[Mesh::ARRAY_INDEX] = UniqueIndices;
	arr[Mesh::ARRAY_TEX_UV] = unique_uv1s;
	arr[Mesh::ARRAY_TEX_UV2] = unique_uv2s;

	am_new->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arr);

//	am_new->surface_set_material(0, mat);

	// hopefully the old mesh will be ref count freed? ???
	mi.set_mesh(am_new);

	mi.set_surface_material(0, mat);

	return true;
}


//bool LHelper::CreateLightmapProxy(LRoomManager &manager, BakedLightmap &baked_lightmap)
MeshInstance * LHelper::CreateLightmapProxy(LRoomManager &manager)
{
	// create a temporary mesh instance to merge to, lightmap etc
	MeshInstance * pMerged = memnew(MeshInstance);
	pMerged->set_name("lightmap_proxy");

//	baked_lightmap.add_child(pMerged);

	manager.get_parent()->add_child(pMerged);

	bool res = true;

	// create uv2s is done in merge
	if (!MergeSOBs(manager, pMerged))
	{
		res = false;
		goto finish;
	}

	/*
	if (!BakeLightmap(baked_lightmap, pMerged))
	{
		res = false;
		goto finish;
	}
	*/

	// unmerge the mesh back to the sobs
	if (!UnMergeSOBs(manager, pMerged))
	{
		res = false;
		goto finish;
	}

finish:
	// finished .. remove the merged child and delete
//	pMerged->queue_delete();

	if (res)
		return pMerged;

	// failed
	pMerged->queue_delete();
	return 0;
//	return res;
}


bool LHelper::MergeSOBs(LRoomManager &manager, MeshInstance * pMerged, bool bLightmapUnwrap)
{
	PoolVector<Vector3> verts;
	PoolVector<Vector3> normals;
	PoolVector<int> inds;
//	PoolVector<Vector2> uv2s;


	// go through each sob mesh
	for (int n=0; n<manager.m_SOBs.size(); n++)
	{
		LSob &sob = manager.m_SOBs[n];
		const GeometryInstance * pGI = sob.GetGI();
		if (!pGI)
			continue;

		const MeshInstance * pMI = Object::cast_to<MeshInstance>(pGI);
		if (!pMI)
			continue;

		// to get the transform, the node has to be in the tree, so temporarily show if hidden
		//bool bShowing = sob.m_bShow;
		//sob.Show(true);

		Merge_MI(*pMI, verts, normals, inds);

		//sob.Show(bShowing);
	}


	assert (pMerged);
	//MeshInstance * pMI = memnew(MeshInstance);
	//pMI->set_name("Merged");
	//add_child(pMI);
	//Ref<Mesh> rmesh = pMI->get_mesh();

	LPRINT(5, "Merging, num verts is " + itos(verts.size()));

	// lightmap unwrap
	//LightmapUnwrap(verts, normals, inds, uv2s);

	// unmerge the original meshes (write the uvs2 back)


	Ref<ArrayMesh> am;
	am.instance();
	Array arr;
	arr.resize(Mesh::ARRAY_MAX);
	arr[Mesh::ARRAY_VERTEX] = verts;
	arr[Mesh::ARRAY_NORMAL] = normals;
	arr[Mesh::ARRAY_INDEX] = inds;
//	arr[Mesh::ARRAY_TEX_UV2] = uv2s;

	// bug fix. The lightmapping code removes duplicate vertices, which we DONT want
	// as it makes the merged mesh get out of sync with the original meshes.
	// To prevent this we will create a dummy set of UV1s that are unique.
//	PoolVector<Vector2> dummy_uvs;
//	for (int n=0; n<verts.size(); n++)
//	{
//		dummy_uvs.append(Vector2(n, n));
//	}
//	arr[Mesh::ARRAY_TEX_UV] = dummy_uvs;

	// sanity check on the indices
	for (int n=0; n<inds.size(); n++)
	{
		int i = inds[n];
		assert (i < verts.size());
	}


//	void add_surface_from_arrays(PrimitiveType p_primitive, const Array &p_arrays, const Array &p_blend_shapes = Array(), uint32_t p_flags = ARRAY_COMPRESS_DEFAULT);

	am->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arr, Array(), Mesh::ARRAY_COMPRESS_DEFAULT);

	if (bLightmapUnwrap)
		LightmapUnwrap(am, pMerged->get_global_transform());

	// duplicate the UV2 to uv1 just in case they are needed
	arr[Mesh::ARRAY_TEX_UV] = arr[Mesh::ARRAY_TEX_UV2];

	pMerged->set_mesh(am);

	//DebugCountUVs(*pMerged);


	// check the num of uvs match the number of verts
	//int numUVs = DebugCountUVs(*pMerged);

	// set mesh to use in baked lighting
	pMerged->set_flag(GeometryInstance::FLAG_USE_BAKED_LIGHT, true);

	return true;

	// bake
//	Node * pBLN = pMerged->get_parent()->find_node("BakedLightmap");
//	BakedLightmap * pBL = Object::cast_to<BakedLightmap>(pBLN);

//	if (!pBL)
//	{
//		LWARN(5, "BakedLightmap not found");
//		return false;
//	}

//	{
//		BakedLightmap::BakeError err;
//		err = pBL->bake(pMerged);

//		switch (err)
//		{
//			case BakedLightmap::BAKE_ERROR_NO_SAVE_PATH:
//				LWARN(5, "Can't determine a save path for lightmap images.\nSave your scene (for images to be saved in the same dir), or pick a save path from the BakedLightmap properties.");
//				break;
//			case BakedLightmap::BAKE_ERROR_NO_MESHES:
//				LWARN(5, "No meshes to bake. Make sure they contain an UV2 channel and that the 'Bake Light' flag is on.");
//				break;
//			case BakedLightmap::BAKE_ERROR_CANT_CREATE_IMAGE:
//				LWARN(5, "Failed creating lightmap images, make sure path is writable.");
//				break;
//			default: {
//			}
//		}
//	}

//	return true;
}

bool LHelper::LightmapUnwrap(Ref<ArrayMesh> am, const Transform &trans)
{
//	ArrayMesh
//	Ref<Mesh> rmesh = pMesh->get_mesh();

	// we can add the UV2 coords from here
	Error err = am->lightmap_unwrap(trans);
	if (err != OK) {
		LWARN(5, "UV Unwrap failed, mesh may not be manifold?");
		return false;
	}

	return true;
}


void LHelper::SetOwnerRecursive(Node * pNode, Node * pOwner)
{
	pNode->set_owner(pOwner);

	for (int n=0; n<pNode->get_child_count(); n++)
	{
		SetOwnerRecursive(pNode->get_child(n), pOwner);
	}
}


bool LHelper::BakeLightmap(BakedLightmap &baked_lightmap, MeshInstance * pMerged)
{
	// bake

	BakedLightmap::BakeError err;
//	err = baked_lightmap.bake(pMerged);
	Node * pStartNode = baked_lightmap.get_parent();

	// baked lightmap only picks up meshes and traverses if the owner is set.... (!)
	SetOwnerRecursive(pStartNode, pStartNode);

	err = baked_lightmap.bake(pStartNode);

	switch (err)
	{
		case BakedLightmap::BAKE_ERROR_NO_SAVE_PATH:
		{
			LWARN(5, "Can't determine a save path for lightmap images.\nSave your scene (for images to be saved in the same dir), or pick a save path from the BakedLightmap properties.");
			return false;
		}
			break;
		case BakedLightmap::BAKE_ERROR_NO_MESHES:
		{
			LWARN(5, "No meshes to bake. Make sure they contain an UV2 channel and that the 'Bake Light' flag is on.");
			return false;
		}
			break;
		case BakedLightmap::BAKE_ERROR_CANT_CREATE_IMAGE:
		{
			LWARN(5, "Failed creating lightmap images, make sure path is writable.");
			return false;
		}
			break;
		default: {
		}
	}

	return true;
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


void LHelper::Merge_MI(const MeshInstance &mi, PoolVector<Vector3> &verts, PoolVector<Vector3> &norms, PoolVector<int> &inds)
{
	// some godot jiggery pokery to get the mesh verts in local space
	Ref<Mesh> rmesh = mi.get_mesh();
	Array arrays = rmesh->surface_get_arrays(0);
	PoolVector<Vector3> p_vertices = arrays[VS::ARRAY_VERTEX];
	PoolVector<Vector3> p_normals = arrays[VS::ARRAY_NORMAL];
	PoolVector<int> p_indices = arrays[VS::ARRAY_INDEX];
	//PoolVector<int>::Read ir = mesh_indices.read();

	// the first index of this mesh is offset from the verts we already have stored in the merged mesh
	int first_index = verts.size();

//	LPRINT(2, "Merge MI : " + mi.get_name() + "\tFirstVert : " + itos(first_index) + "\tNumUVs : " + itos(p_vertices.size()));

	// transform verts to world space
	Transform trans = mi.get_global_transform();

	for (int n=0; n<p_vertices.size(); n++)
	{
		Vector3 ptWorld = trans.xform(p_vertices[n]);

		// hacky way for normals, we should use transpose of inverse matrix, dunno if godot supports this
		Vector3 ptNormA = Vector3(0, 0, 0);
		Vector3 ptNormWorldA = trans.xform(ptNormA);
		Vector3 ptNormWorldB = trans.xform(p_normals[n]);

		Vector3 ptNorm = ptNormWorldB - ptNormWorldA;

		verts.push_back(ptWorld);
		norms.push_back(ptNorm);
	}

	// indices
	for (int n=0; n<p_indices.size(); n++)
		inds.push_back(p_indices[n] + first_index);

}


extern bool (*array_mesh_lightmap_unwrap_callback)(float p_texel_size, const float *p_vertices, const float *p_normals, int p_vertex_count, const int *p_indices, const int *p_face_materials, int p_index_count, float **r_uv, int **r_vertex, int *r_vertex_count, int **r_index, int *r_index_count, int *r_size_hint_x, int *r_size_hint_y);

/*
bool LHelper::LightmapUnwrap(const PoolVector<Vector3> &p_verts, const PoolVector<Vector3> &p_normals, const PoolVector<int> &p_inds, PoolVector<Vector2> &r_uvs)
{
	ERR_FAIL_COND_V(!array_mesh_lightmap_unwrap_callback, ERR_UNCONFIGURED);

	assert (p_verts.size() == p_normals.size());

	Vector<Vector3> verts;
	for (int n=0; n<p_verts.size(); n++)
		verts.push_back(p_verts[n]);

	Vector<Vector3> normals;
	for (int n=0; n<p_normals.size(); n++)
		normals.push_back(p_normals[n]);

	Vector<int> inds;
	for (int n=0; n<p_inds.size(); n++)
		inds.push_back(p_inds[n]);

	int nFaces = inds.size() / 3;

	// create some dummy face_materials
	Vector<int> face_mats;
	for (int n=0; n<nFaces; n++)
		face_mats.push_back(0);


	float *gen_uvs;
	int *gen_vertices;
	int *gen_indices;
	int gen_vertex_count;
	int gen_index_count;
	int size_x;
	int size_y;

	float p_texel_size = 1.0f;

	const float * pfVerts = (float *) &verts[0];
	const float * pfNormals = (float *) &normals[0];


	bool ok = array_mesh_lightmap_unwrap_callback(p_texel_size, pfVerts, pfNormals, verts.size(), &inds[0], &face_mats[0], inds.size(), &gen_uvs, &gen_vertices, &gen_vertex_count, &gen_indices, &gen_index_count, &size_x, &size_y);

	if (!ok) {
		return false;
	}

	// write to r_uvs
	r_uvs.resize(verts.size());
	// blank
	for (int n=0; n<r_uvs.size(); n++)
		r_uvs.set(n, Vector2(0, 0));

	LPRINT(2, "unwrapped orig vert order, num unwrapped verts " + itos(gen_vertex_count));
	for (int n=0; n<gen_vertex_count; n++)
	{
		int orig_vert_id = gen_vertices[n];
		//LPRINT(2, itos(orig_vert_id));
		const Vector2 * pUV = (Vector2 *) &gen_uvs[n * 2];

		r_uvs.set(orig_vert_id, *pUV);
	}

	//free stuff
	::free(gen_vertices);
	::free(gen_indices);
	::free(gen_uvs);

	return true;
}
*/


//bool LHelper::xatlas_mesh_lightmap_unwrap(float p_texel_size, const float *p_vertices, const float *p_normals, int p_vertex_count, const int *p_indices, const int *p_face_materials, int p_index_count, float **r_uv, int **r_vertex, int *r_vertex_count, int **r_index, int *r_index_count, int *r_size_hint_x, int *r_size_hint_y)
//{

//	//set up input mesh
//	xatlas::MeshDecl input_mesh;
//	input_mesh.indexData = p_indices;
//	input_mesh.indexCount = p_index_count;
//	input_mesh.indexFormat = xatlas::IndexFormat::UInt32;

//	input_mesh.vertexCount = p_vertex_count;
//	input_mesh.vertexPositionData = p_vertices;
//	input_mesh.vertexPositionStride = sizeof(float) * 3;
//	input_mesh.vertexNormalData = p_normals;
//	input_mesh.vertexNormalStride = sizeof(uint32_t) * 3;
//	input_mesh.vertexUvData = NULL;
//	input_mesh.vertexUvStride = 0;

//	xatlas::ChartOptions chart_options;
//	xatlas::PackOptions pack_options;

//	pack_options.maxChartSize = 4096;
//	pack_options.blockAlign = true;
//	pack_options.texelsPerUnit = 1.0 / p_texel_size;

//	xatlas::Atlas *atlas = xatlas::Create();
//	printf("Adding mesh..\n");
//	xatlas::AddMeshError::Enum err = xatlas::AddMesh(atlas, input_mesh, 1);
//	ERR_EXPLAINC(xatlas::StringForEnum(err));
//	ERR_FAIL_COND_V(err != xatlas::AddMeshError::Enum::Success, false);

//	printf("Generate..\n");
//	xatlas::Generate(atlas, chart_options, NULL, pack_options);

//	*r_size_hint_x = atlas->width;
//	*r_size_hint_y = atlas->height;

//	float w = *r_size_hint_x;
//	float h = *r_size_hint_y;

//	if (w == 0 || h == 0) {
//		return false; //could not bake because there is no area
//	}

//	const xatlas::Mesh &output = atlas->meshes[0];

//	*r_vertex = (int *)malloc(sizeof(int) * output.vertexCount);
//	*r_verts = (float *)malloc(sizeof(xatlas::Vertex) * output.vertexCount);
//	*r_index = (int *)malloc(sizeof(int) * output.indexCount);

//	float max_x = 0;
//	float max_y = 0;
//	for (uint32_t i = 0; i < output.vertexCount; i++) {
//		(*r_vertex)[i] = output.vertexArray[i].xref;
//		(*r_uv)[i * 2 + 0] = output.vertexArray[i].uv[0] / w;
//		(*r_uv)[i * 2 + 1] = output.vertexArray[i].uv[1] / h;
//		max_x = MAX(max_x, output.vertexArray[i].uv[0]);
//		max_y = MAX(max_y, output.vertexArray[i].uv[1]);
//	}

//	printf("Final texture size: %f,%f - max %f,%f\n", w, h, max_x, max_y);
//	*r_vertex_count = output.vertexCount;

//	for (uint32_t i = 0; i < output.indexCount; i++) {
//		(*r_index)[i] = output.indexArray[i];
//	}

//	*r_index_count = output.indexCount;

//	xatlas::Destroy(atlas);
//	printf("Done\n");
//	return true;
//}



/*
//Error ArrayMesh::lightmap_unwrap(const Transform &p_base_transform, float p_texel_size) {
Error LHelper::lightmap_unwrap(ArrayMesh &am, const Transform &p_base_transform, float p_texel_size)
{

	ERR_FAIL_COND_V(!array_mesh_lightmap_unwrap_callback, ERR_UNCONFIGURED);
	ERR_FAIL_COND_V_MSG(am.blend_shapes.size() != 0, ERR_UNAVAILABLE, "Can't unwrap mesh with blend shapes.");

	Vector<float> vertices;
	Vector<float> normals;
	Vector<int> indices;
	Vector<int> face_materials;
	Vector<float> uv;
	Vector<Pair<int, int> > uv_index;

	Vector<ArrayMeshLightmapSurface> surfaces;
	for (int i = 0; i < get_surface_count(); i++) {
		ArrayMeshLightmapSurface s;
		s.primitive = surface_get_primitive_type(i);

		ERR_FAIL_COND_V_MSG(s.primitive != Mesh::PRIMITIVE_TRIANGLES, ERR_UNAVAILABLE, "Only triangles are supported for lightmap unwrap.");
		s.format = surface_get_format(i);
		ERR_FAIL_COND_V_MSG(!(s.format & ARRAY_FORMAT_NORMAL), ERR_UNAVAILABLE, "Normals are required for lightmap unwrap.");

		Array arrays = surface_get_arrays(i);
		s.material = surface_get_material(i);
		s.vertices = SurfaceTool::create_vertex_array_from_triangle_arrays(arrays);

		PoolVector<Vector3> rvertices = arrays[Mesh::ARRAY_VERTEX];
		int vc = rvertices.size();
		PoolVector<Vector3>::Read r = rvertices.read();

		PoolVector<Vector3> rnormals = arrays[Mesh::ARRAY_NORMAL];
		PoolVector<Vector3>::Read rn = rnormals.read();

		int vertex_ofs = vertices.size() / 3;

		vertices.resize((vertex_ofs + vc) * 3);
		normals.resize((vertex_ofs + vc) * 3);
		uv_index.resize(vertex_ofs + vc);

		for (int j = 0; j < vc; j++) {

			Vector3 v = p_base_transform.xform(r[j]);
			Vector3 n = p_base_transform.basis.xform(rn[j]).normalized();

			vertices.write[(j + vertex_ofs) * 3 + 0] = v.x;
			vertices.write[(j + vertex_ofs) * 3 + 1] = v.y;
			vertices.write[(j + vertex_ofs) * 3 + 2] = v.z;
			normals.write[(j + vertex_ofs) * 3 + 0] = n.x;
			normals.write[(j + vertex_ofs) * 3 + 1] = n.y;
			normals.write[(j + vertex_ofs) * 3 + 2] = n.z;
			uv_index.write[j + vertex_ofs] = Pair<int, int>(i, j);
		}

		PoolVector<int> rindices = arrays[Mesh::ARRAY_INDEX];
		int ic = rindices.size();

		if (ic == 0) {

			for (int j = 0; j < vc / 3; j++) {
				if (Face3(r[j * 3 + 0], r[j * 3 + 1], r[j * 3 + 2]).is_degenerate())
					continue;

				indices.push_back(vertex_ofs + j * 3 + 0);
				indices.push_back(vertex_ofs + j * 3 + 1);
				indices.push_back(vertex_ofs + j * 3 + 2);
				face_materials.push_back(i);
			}

		} else {
			PoolVector<int>::Read ri = rindices.read();

			for (int j = 0; j < ic / 3; j++) {
				if (Face3(r[ri[j * 3 + 0]], r[ri[j * 3 + 1]], r[ri[j * 3 + 2]]).is_degenerate())
					continue;
				indices.push_back(vertex_ofs + ri[j * 3 + 0]);
				indices.push_back(vertex_ofs + ri[j * 3 + 1]);
				indices.push_back(vertex_ofs + ri[j * 3 + 2]);
				face_materials.push_back(i);
			}
		}

		surfaces.push_back(s);
	}

	//unwrap

	float *gen_uvs;
	int *gen_vertices;
	int *gen_indices;
	int gen_vertex_count;
	int gen_index_count;
	int size_x;
	int size_y;

	bool ok = array_mesh_lightmap_unwrap_callback(p_texel_size, vertices.ptr(), normals.ptr(), vertices.size() / 3, indices.ptr(), face_materials.ptr(), indices.size(), &gen_uvs, &gen_vertices, &gen_vertex_count, &gen_indices, &gen_index_count, &size_x, &size_y);

	if (!ok) {
		return ERR_CANT_CREATE;
	}

	//remove surfaces
	while (get_surface_count()) {
		surface_remove(0);
	}

	//create surfacetools for each surface..
	Vector<Ref<SurfaceTool> > surfaces_tools;

	for (int i = 0; i < surfaces.size(); i++) {
		Ref<SurfaceTool> st;
		st.instance();
		st->begin(Mesh::PRIMITIVE_TRIANGLES);
		st->set_material(surfaces[i].material);
		surfaces_tools.push_back(st); //stay there
	}

	print_verbose("Mesh: Gen indices: " + itos(gen_index_count));
	//go through all indices
	for (int i = 0; i < gen_index_count; i += 3) {

		ERR_FAIL_INDEX_V(gen_vertices[gen_indices[i + 0]], uv_index.size(), ERR_BUG);
		ERR_FAIL_INDEX_V(gen_vertices[gen_indices[i + 1]], uv_index.size(), ERR_BUG);
		ERR_FAIL_INDEX_V(gen_vertices[gen_indices[i + 2]], uv_index.size(), ERR_BUG);

		ERR_FAIL_COND_V(uv_index[gen_vertices[gen_indices[i + 0]]].first != uv_index[gen_vertices[gen_indices[i + 1]]].first || uv_index[gen_vertices[gen_indices[i + 0]]].first != uv_index[gen_vertices[gen_indices[i + 2]]].first, ERR_BUG);

		int surface = uv_index[gen_vertices[gen_indices[i + 0]]].first;

		for (int j = 0; j < 3; j++) {

			SurfaceTool::Vertex v = surfaces[surface].vertices[uv_index[gen_vertices[gen_indices[i + j]]].second];

			if (surfaces[surface].format & ARRAY_FORMAT_COLOR) {
				surfaces_tools.write[surface]->add_color(v.color);
			}
			if (surfaces[surface].format & ARRAY_FORMAT_TEX_UV) {
				surfaces_tools.write[surface]->add_uv(v.uv);
			}
			if (surfaces[surface].format & ARRAY_FORMAT_NORMAL) {
				surfaces_tools.write[surface]->add_normal(v.normal);
			}
			if (surfaces[surface].format & ARRAY_FORMAT_TANGENT) {
				Plane t;
				t.normal = v.tangent;
				t.d = v.binormal.dot(v.normal.cross(v.tangent)) < 0 ? -1 : 1;
				surfaces_tools.write[surface]->add_tangent(t);
			}
			if (surfaces[surface].format & ARRAY_FORMAT_BONES) {
				surfaces_tools.write[surface]->add_bones(v.bones);
			}
			if (surfaces[surface].format & ARRAY_FORMAT_WEIGHTS) {
				surfaces_tools.write[surface]->add_weights(v.weights);
			}

			Vector2 uv2(gen_uvs[gen_indices[i + j] * 2 + 0], gen_uvs[gen_indices[i + j] * 2 + 1]);
			surfaces_tools.write[surface]->add_uv2(uv2);

			surfaces_tools.write[surface]->add_vertex(v.vertex);
		}
	}

	//free stuff
	::free(gen_vertices);
	::free(gen_indices);
	::free(gen_uvs);

	//generate surfaces

	for (int i = 0; i < surfaces_tools.size(); i++) {
		surfaces_tools.write[i]->index();
		surfaces_tools.write[i]->commit(Ref<ArrayMesh>((ArrayMesh *)this), surfaces[i].format);
	}

	set_lightmap_size_hint(Size2(size_x, size_y));

	return OK;
}
*/
