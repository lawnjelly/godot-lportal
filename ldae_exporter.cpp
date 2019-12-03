#include "ldae_exporter.h"
#include "scene/main/node.h"
#include "scene/3d/light.h"
#include "ldebug.h"
#include "lportal.h"

#define FSL(a) m_File.store_line(a)
#define FSS(a) m_File.store_string(a)

#define FSL_T(a, b) {for (int n=0; n<a; n++) {FSS("\t");}\
FSL(b);}

#define FSS_T(a, b) {for (int n=0; n<a; n++) {FSS("\t");}\
FSS(b);}


LDAEExporter::LDAEExporter()
{
	m_pMergedMI = 0;
	m_bAddRootNode = true;
	m_Stage = ST_DATA;
	m_bRemovePortals = true;
}


bool LDAEExporter::SaveScene(Node * pNode, String szFilename, bool bRemovePortals)
{
	m_bAddRootNode = true;
	m_bRemovePortals = bRemovePortals;

	// open the output file
	Error err = m_File.open(szFilename, _File::WRITE);
	if (err != OK)
		return false;

	Save_Preamble();

	m_Stage = ST_DATA;

	// initially find all the meshes, lights etc
	SaveScene_Recursive(pNode, 0);

	// lights
	FSL("\t<library_lights>");
	for (int n=0; n<m_Lights.size(); n++)
		SaveData_Light(*m_Lights[n]);
	FSL("\t</library_lights>");

	// mesh instances
	FSL("\t<library_geometries>");

	// merged mesh?
	if (m_pMergedMI)
		SaveData_MeshInstance(*m_pMergedMI);

	for (int n=0; n<m_MeshInstances.size(); n++)
		SaveData_MeshInstance(*m_MeshInstances[n]);
	FSL("\t</library_geometries>");

	m_Stage = ST_SCENE_GRAPH;
	Save_Mid();

	// now save the scene graph
	SaveScene_Recursive(pNode, 0);

	Save_Final();

	m_File.close();
	return true;
}


bool LDAEExporter::Save_Preamble()
{
	FSL("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
	FSL("<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">");

	return true;
}

bool LDAEExporter::Save_Mid()
{
	FSL("\t<library_visual_scenes>");
	FSL("\t\t<visual_scene id=\"Scene\" name=\"Scene\">");

	if (m_bAddRootNode)
	{
		FSL_T(3, "<node id=\"l_root\" name=\"l_root\" type=\"NODE\">");

		// rotate x by 90 degrees
		FSL_T(3, "<matrix sid=\"transform\">1 0 0 0 0 0 -1 0 0 1 0 0 0 0 0 1</matrix>");
	}

	if (m_pMergedMI)
	{
		String sz = SaveScene_MeshInstance(*m_pMergedMI, 3);
		FSL(sz); // merged node close </node>

		FSL_T(3, "<node id=\"l_level\" name=\"l_level\" type=\"NODE\">");
	}

	return true;
}

void LDAEExporter::TransformToMatrix(const Transform &tr, float * m)
{
		const Basis &b = tr.basis;

		m[0] = b.elements[0][0];
		m[1] = b.elements[0][1];
		m[2] = b.elements[0][2];
		m[3] = tr.origin.x;

		m[4] = b.elements[1][0];
		m[5] = b.elements[1][1];
		m[6] = b.elements[1][2];
		m[7] = tr.origin.y;

		m[8] = b.elements[2][0];
		m[9] = b.elements[2][1];
		m[10] = b.elements[2][2];
		m[11] = tr.origin.z;

		m[12] = 0.0f;
		m[13] = 0.0f;
		m[14] = 0.0f;
		m[15] = 1.0f;
}

String LDAEExporter::SaveScene_Light(const Light &l, int depth)
{
	int tab_depth = 3 + depth;

	String name = l.get_name();
	String long_name = name + "-light";
	FSL_T(tab_depth, "<node id=\"" + name + "\" name=\"" + name + "\" type=\"NODE\">");

	WriteMatrix(l.get_transform(), tab_depth+1);

	FSL_T(tab_depth+1, "<instance_light url=\"#" + long_name + "\" />");

	String sz = "";
	for (int n=0; n<tab_depth; n++)
		sz += "\t";
	sz += "</node>";
	return sz;
}


void LDAEExporter::WriteMatrix(const Transform &tr, int tab_depth)
{
	float mat[16];
	TransformToMatrix(tr, mat);

	FSS_T(tab_depth, "<matrix sid=\"transform\">");

	for (int n=0; n<16; n++)
	{
		FSS(ftos(mat[n]) + " ");
	}

	FSS("</matrix>\n");
}


String LDAEExporter::SaveScene_MeshInstance(const MeshInstance &mi, int depth)
{
	int tab_depth = 3 + depth;

	String name = mi.get_name();
	String long_name = name + "-mesh";
	FSL_T(tab_depth, "<node id=\"" + name + "\" name=\"" + name + "\" type=\"NODE\">");


	WriteMatrix(mi.get_transform(), tab_depth+1);

	FSL_T(tab_depth+1, "<instance_geometry url=\"#" + long_name + "\" name=\"" + name + "\">");
	FSL_T(tab_depth+1, "</instance_geometry>");

	String sz = "";
	for (int n=0; n<tab_depth; n++)
		sz += "\t";
	sz += "</node>";
//	FSL_T(tab_depth, "</node>");

	return sz;
}

String LDAEExporter::SaveScene_Spatial(const Spatial &sp, int depth)
{
	int tab_depth = 3 + depth;
	String name = sp.get_name();

	FSL_T(tab_depth, "<node id=\"" + name + "\" name=\"" + name + "\" type=\"NODE\">");

	WriteMatrix(sp.get_transform(), tab_depth+1);

	String sz = "";
	for (int n=0; n<tab_depth; n++)
		sz += "\t";
	sz += "</node>";

	return sz;
}


bool LDAEExporter::Save_Final()
{
	if (m_pMergedMI)
	{
		FSL_T(3, "</node>"); // level
	}

	if (m_bAddRootNode)
	{
		FSL_T(3, "</node>"); // root
	}

	FSL("\t\t</visual_scene>");
	FSL("\t</library_visual_scenes>");
	FSL("\t<scene>");
	FSL("\t\t<instance_visual_scene url=\"#Scene\"/>");
	FSL("\t</scene>");


	FSL("</COLLADA>");
	return true;
}


bool LDAEExporter::IsPortal(const MeshInstance &mi)
{
	if (LPortal::NameStartsWith(&mi, "portal_"))
		return true;

	return false;
}


bool LDAEExporter::SaveScene_Recursive(Node * pNode, int depth)
{
	String szClose = "";

	// mesh instance?
	MeshInstance * pMI = Object::cast_to<MeshInstance>(pNode);
	if (pMI)
	{
		if (m_bRemovePortals && (IsPortal(*pMI)))
		{
			// portal .. not exporting
		}
		else
		{
			if (m_Stage == ST_DATA)
				m_MeshInstances.push_back(pMI);
			else
				szClose = SaveScene_MeshInstance(*pMI, depth);
		}
	}

	// light?
	Light * pLight = Object::cast_to<Light>(pNode);
	if (pLight)
	{
		if (m_Stage == ST_DATA)
			m_Lights.push_back(pLight);
		else
			szClose = SaveScene_Light(*pLight, depth);
	}

	// spatial? (and only a spatial)
	if (pNode->get_class() == "Spatial")
	{
		if (m_Stage == ST_SCENE_GRAPH)
		{
			szClose = SaveScene_Spatial(*Object::cast_to<Spatial>(pNode), depth);
		}
	}


	// go through the children
	for (int n=0; n<pNode->get_child_count(); n++)
	{
		SaveScene_Recursive(pNode->get_child(n), depth+1);
	}

	// a closing xml expression
	if (szClose != "")
	{
		FSL(szClose);
	}

	return true;
}




bool LDAEExporter::SaveData_Light(const Light &light)
{
	String name = light.get_name();
	String long_name = name + "-light";

	int t = 1;

	String szLightType = "point";
	//if (light.is_class("OmniLight")

	if (light.is_class("SpotLight"))
		szLightType = "spot";

	if (light.is_class("DirectionalLight"))
		szLightType = "directional";

	FSL_T(t, "<light id=\"" + long_name + "\" name=\"" + name + "\">");
	FSL_T(t+1, "<technique_common>");
	FSL_T(t+2, "<" + szLightType + ">");



	FSL_T(t+2, "</" + szLightType + ">");
	FSL_T(t+1, "</technique_common>");
	FSL_T(t, "</light>");

	return true;
}

//static bool g_SingleDAETest = false;


bool LDAEExporter::SaveData_MeshInstance(const MeshInstance &mi)
{
//	if (g_SingleDAETest)
//		return true;
//	g_SingleDAETest = true;

	Ref<Mesh> rmesh = mi.get_mesh();
	Array arrays = rmesh->surface_get_arrays(0);
	PoolVector<Vector3> verts = arrays[VS::ARRAY_VERTEX];
	PoolVector<Vector3> norms = arrays[VS::ARRAY_NORMAL];
	PoolVector<Vector2> uv1s = arrays[VS::ARRAY_TEX_UV];
	PoolVector<int> inds = arrays[VS::ARRAY_INDEX];

	int nVerts = verts.size();
	int nInds = inds.size();
	int nFaces = nInds / 3;

	String name = mi.get_name();
	String long_name = name + "-mesh";


	FSL("\t<geometry id=\"" + long_name + "\" name=\"" + name + "\">");
	FSL("\t\t<mesh>");

	// positions
	FSL("\t\t\t<source id=\"" + long_name + "-positions\">");

	FSS("\t\t\t\t<float_array id=\"" + long_name + "-positions-array\" count=\"");
	FSS(itos(nVerts*3) + "\">");
	for (int n=0; n<nVerts; n++)
	{
		Vector3 pos = verts[n];

		FSS(ftos(pos.x) + " " + ftos(pos.y) + "  " + ftos(pos.z) + " ");
	}
	FSS("</float_array>\n");

	FSL("\t\t\t\t<technique_common>");
	FSL("\t\t\t\t\t<accessor source=\"#" + long_name + "-positions-array\" count=\""
	+ itos(nVerts) + "\" stride=\"3\">");
	FSL("\t\t\t\t\t\t<param name=\"X\" type=\"float\" />");
	FSL("\t\t\t\t\t\t<param name=\"Y\" type=\"float\" />");
	FSL("\t\t\t\t\t\t<param name=\"Z\" type=\"float\" />");

	FSL("\t\t\t\t\t</accessor>");
	FSL("\t\t\t\t</technique_common>");

	FSL("\t\t\t</source>");

	// normals
	if (norms.size())
	{
		FSL("\t\t\t<source id=\"" + long_name + "-normals\">");

		FSS("\t\t\t\t<float_array id=\"" + long_name + "-normals-array\" count=\"");
		FSS(itos(nVerts*3) + "\">");
		for (int n=0; n<nVerts; n++)
		{
			Vector3 norm = norms[n];

			FSS(ftos(norm.x) + " " + ftos(norm.y) + "  " + ftos(norm.z) + " ");
		}
		FSS("</float_array>\n");

		FSL("\t\t\t\t<technique_common>");
		FSL("\t\t\t\t\t<accessor source=\"#" + long_name + "-normals-array\" count=\""
		+ itos(nVerts) + "\" stride=\"3\">");
		FSL("\t\t\t\t\t\t<param name=\"X\" type=\"float\" />");
		FSL("\t\t\t\t\t\t<param name=\"Y\" type=\"float\" />");
		FSL("\t\t\t\t\t\t<param name=\"Z\" type=\"float\" />");

		FSL("\t\t\t\t\t</accessor>");
		FSL("\t\t\t\t</technique_common>");

		FSL("\t\t\t</source>");
	}

	FSL("\t\t\t<vertices id=\"" + long_name + "-vertices\">");
	FSL("\t\t\t\t<input semantic=\"POSITION\" source=\"#" + long_name + "-positions\" />");
	FSL("\t\t\t</vertices>");

	FSL("\t\t\t<triangles count=\"" + itos(nFaces) + "\">");
	FSL("\t\t\t\t<input semantic=\"VERTEX\" source=\"#" + long_name + "-vertices\" offset=\"0\"/>");
	if (norms.size())
	{
		FSL("\t\t\t\t<input semantic=\"NORMAL\" source=\"#" + long_name + "-normals\" offset=\"1\"/>");
	}

	FSS("\t\t\t\t<p>");

	// DAE has face winding reversed compared to godot
	for (int f=0; f<nFaces; f++)
//	for (int n=0; n<nInds; n++)
	{
		int base = f * 3;

		// one index for position, one for normal
		FSS(itos(inds[base+2]) + " ");
		FSS(itos(inds[base+2]) + " ");
		FSS(itos(inds[base+1]) + " ");
		FSS(itos(inds[base+1]) + " ");
		FSS(itos(inds[base+0]) + " ");
		FSS(itos(inds[base+0]) + " ");
	}
	FSS("</p>\n");

	FSL("\t\t\t</triangles>");

	FSL("\t\t</mesh>");
	FSL("\t</geometry>");
	return true;
}


#undef FSL
#undef FSS

