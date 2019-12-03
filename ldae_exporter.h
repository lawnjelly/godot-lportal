#pragma once

#include "core/bind/core_bind.h"
#include "scene/3d/mesh_instance.h"

class Light;

class LDAEExporter
{
	enum eStage
	{
		ST_DATA,
		ST_SCENE_GRAPH,
	};

public:
	LDAEExporter();
	bool SaveScene(Node * pNode, String szFilename, bool bRemovePortals = false);
	void SetMergedMeshInstance(MeshInstance * pMI) {m_pMergedMI = pMI;}


private:

	bool SaveScene_Recursive(Node * pNode, int depth);



	String SaveScene_MeshInstance(const MeshInstance &mi, int depth);
	String SaveScene_Light(const Light &l, int depth);
	String SaveScene_Spatial(const Spatial &sp, int depth);

	bool SaveData_Light(const Light &light);
	bool SaveData_MeshInstance(const MeshInstance &mi);


	bool Save_Preamble();
	bool Save_Mid();
	bool Save_Final();

	void WriteMatrix(const Transform &tr, int tab_depth);
	void TransformToMatrix(const Transform &tr, float * m);

	bool IsPortal(const MeshInstance &mi);

	Vector<const MeshInstance *> m_MeshInstances;
	Vector<const Light *> m_Lights;

	_File m_File;
	eStage m_Stage;

	// export the DAE with everything under a root node .. this is sometimes more convenient
	bool m_bAddRootNode;

	// remove portals
	bool m_bRemovePortals;

	// if we are exporting the level with a merged mesh too
	MeshInstance * m_pMergedMI;
};
