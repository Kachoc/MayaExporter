#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>
#include <maya/MFloatArray.h>
#include <maya/MFnSet.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MFnLambertShader.h>
#include <maya/MFnPhongShader.h>
#include <maya/MFnBlinnShader.h>

#include "CPMMeshExtractor.h"

CPMMeshExtractor::CPMMeshExtractor(const MDagPath &dagPath, bool objectSpace, MStatus &status) : m_dagPath(dagPath), m_mesh(dagPath, &status)
{
	m_space = MSpace::kWorld;
	if(objectSpace) m_space = MSpace::kObject;
}

CPMMeshExtractor::~CPMMeshExtractor()
{

}

MStatus CPMMeshExtractor::extractMesh(MESH_EXTRACTOR_INFO &mesh)
{
	MStatus status;
	unsigned int numVertices = 0;

	if(!extractGeometry(mesh, numVertices)) {
		MGlobal::displayError("CPMMeshExtractor::extractGeometry");
		return MS::kFailure;
	}
	if(!extractMaterials(mesh)) {
		MGlobal::displayError("CPMMeshExtractor::extractMaterials");
		return MS::kFailure;
	}
	if(!assembleMesh(mesh, numVertices)) {
		MGlobal::displayError("CPMMeshExtractor::assembleMesh");
		return MS::kFailure;
	}

	return MS::kSuccess;
}

MStatus CPMMeshExtractor::extractGeometry(MESH_EXTRACTOR_INFO &mesh, unsigned int &numVertices)
{
	MStatus status;

	if(mesh.UVs != NULL && mesh.uvSetName == "") {
		MDagPath nDagPath(m_dagPath);
		nDagPath.extendToShape();
		unsigned int instanceNum = 0;
		if(nDagPath.isInstanced()) instanceNum = nDagPath.instanceNumber();

		status = m_mesh.getCurrentUVSetName(mesh.uvSetName, instanceNum);
		if(status == MS::kFailure)
		{
			MGlobal::displayError("MFnMesh::getCurrentUVSetName");
			return MS::kFailure;
		}
	}
	if(mesh.colors != NULL && mesh.colorSetName == "") {
		status = m_mesh.getCurrentColorSetName(mesh.colorSetName, kMFnMeshInstanceUnspecified );
		if(status == MS::kFailure)
		{
			MGlobal::displayError("MFnMesh::getCurrentColorSetName");
			return MS::kFailure;
		}
		if(mesh.colorSetName == "")
		{
			MGlobal::displayError("Le mesh " + m_dagPath.fullPathName() + " n'a pas de set de couleurs");
			return MS::kFailure;
		}
	}

	//
	//	Composition de la liste de vertices désassemblés
	//

	// Nombre de points (chaque point dans l'espace peut être associé à plusieurs normales et coordonnées uv, donnant lieu à plusieurs vertices)
	m_dVertices.resize(m_mesh.numVertices());

	// Nombre actuel de vertices comptés, dont la valeur finale donnera le nombre de vertices du mesh à exporter
	unsigned int actualNumVertices = 0;

	// On récupère les triangles et on redimensionne le vector triangles (sortie)
	MIntArray triangleCounts, triangleVertices;
	if(m_mesh.getTriangles(triangleCounts, triangleVertices) == MS::kFailure)
	{
		MGlobal::displayError("MFnMesh::getTriangles");
		return MS::kFailure;
	}

	///////////////////////////////////////////
	mesh.triangles.setLength(triangleVertices.length());
	///////////////////////////////////////////


	// Nombre de polygones
	const unsigned int numPolygons = m_mesh.numPolygons();
	unsigned int numTris = 0;

	// Pour chaque polygone, on récupère les indices de vertices, de normales et de coordonnées UV et on les enregistre dans m_dVertices
	// en vue d'assembler les vertices du mesh à exporter
	MIntArray vertexList, normalList, uvList, tgtBinormalList, colorList; // indices de vertice, normale, coordonnées UV, et couleur du polygone actuel
	MIntArray nFaceVertexList; // indices des vertices finaux du polygone actuel
	unsigned int actualTriangleVertId = 0; // début des indices de vertices du polygone actuel dans le tableau triangleVertices
	for(unsigned int i = 0; i < numPolygons; i++)
	{
		// On récupère les indices de vertices, de normales, de coordonnées uv, de tangente et de couleur
		if(!m_mesh.getPolygonVertices(i, vertexList)) {
			MGlobal::displayError("MFnMesh::getPolygonVertices");
			return MS::kFailure;
		}
		if(!m_mesh.getFaceNormalIds(i, normalList)) {
			MGlobal::displayError("MFnMesh::getFaceNormalIds");
			return MS::kFailure;
		}
		if(mesh.UVs) {
			uvList.setLength(vertexList.length());
			for(unsigned int j = 0; j < uvList.length(); j++) {
					if(!m_mesh.getPolygonUVid(i, j, uvList[j], &mesh.uvSetName)) {
					MGlobal::displayError("MFnMesh::getPolygonUVid");
					return MS::kFailure;
				}
			}
		}
		if(mesh.tgtBinormals) {
			tgtBinormalList.setLength(vertexList.length());
			for(unsigned int j = 0; j < tgtBinormalList.length(); j++) {
					tgtBinormalList[j] = m_mesh.getTangentId(i, vertexList[j], &status);
					if(!status) {
					MGlobal::displayError("MFnMesh::getTangentId");
					return MS::kFailure;
				}
			}
		}
		if(mesh.colors) {
			colorList.setLength(vertexList.length());
			for(unsigned int j = 0; j < colorList.length(); j++) {
					if(!m_mesh.getColorIndex(i, j, colorList[j], &mesh.colorSetName)) {
					MGlobal::displayError("MFnMesh::getPolygonUVid");
					return MS::kFailure;
				}
			}
		}
		
		// On récupère le nouvel indice de vertice (assemblé plus tard)
		nFaceVertexList.setLength(vertexList.length());
		for(unsigned int j = 0; j < nFaceVertexList.length(); j++)
		{
			ADD_POINT_INFO nPoint(vertexList[j]);
			if(mesh.normals)		nPoint.normalId = &normalList[j];
			if(mesh.tgtBinormals)	nPoint.tgtBinormalId = &tgtBinormalList[j];
			if(mesh.UVs)			nPoint.uvId = &uvList[j];
			if(mesh.colors)			nPoint.colorId = &colorList[j];

			nFaceVertexList[j] = addPoint(nPoint, actualNumVertices);
		}

		// On entre les valeurs d'indices dans le vector triangles en faisant le lien entre la description de la face (vertexList) et la
		// description des triangles composant la face (triangleVertices)
		for(unsigned int j = actualTriangleVertId; j < actualTriangleVertId + triangleCounts[i]*3; j++)
		{
			for(unsigned int k = 0; k < vertexList.length(); k++)
			{
				if(triangleVertices[j] == vertexList[k])
				{
					mesh.triangles[j] = nFaceVertexList[k];
					break;
				}
			}
		}

		actualTriangleVertId += triangleCounts[i]*3;
	}

	numVertices = actualNumVertices;
	
	return MS::kSuccess;
}

MStatus CPMMeshExtractor::extractMaterials(MESH_EXTRACTOR_INFO &mesh)
{
	MStatus status;

	if(!mesh.materials) return MS::kSuccess;

	//
	//	On récupère les faces du mesh
	//
	MIntArray triangleCounts, triangleVertices;
	if(!m_mesh.getTriangles(triangleCounts, triangleVertices)) {
		MGlobal::displayError("MFnMesh::getTriangles");
		return MS::kFailure;
	}

	MIntArray triIndices;
	triIndices.setLength(triangleCounts.length() + 1);
	triIndices[0] = 0;
	for(unsigned int i = 0; i < triangleCounts.length(); i++)
	{
		triIndices[i + 1] = triIndices[0] + triangleCounts[i];
	}

	//
	// Polygon sets
	//
	unsigned int instanceNum = 0;
	MDagPath nDagPath(m_dagPath);
	nDagPath.extendToShape();

	if(nDagPath.isInstanced()) instanceNum = nDagPath.instanceNumber();

	if(m_mesh.getConnectedSetsAndMembers(instanceNum, m_polygonSets, m_polygonComponents, true) == MS::kFailure)
	{
		MGlobal::displayError("MFnMesh::getconnectedSetsAndMembers");
		return MS::kFailure;
	}

	//
	// Material sets
	//

	// Textures
	unsigned int setCount = m_polygonSets.length();
	if(setCount > 1) // le dernier set représente le mesh entier lorsqu'il y a plusieurs sets
	{
		setCount--;
	}

	for(unsigned int i = 0; i < setCount; i++)
	{
		MATERIAL_INFO material; // est créé en début de boucle pour être réinitialisé à chaque fois

		MObject set = m_polygonSets[i];
		MObject comp = m_polygonComponents[i];

		MFnSet fnSet(set, &status);
		if(!status)
		{
			MGlobal::displayError("MFnSet::MFnSet");
			continue;
		}

		// On récupère les faces composant le set
		MItMeshPolygon itMeshPolygon(m_dagPath, comp, &status);
		if(!status) // le set n'est pas un set de polygones
		{
			MGlobal::displayError("MItMeshPolygon::MItMeshPolygon");
			continue;
		}
		
		std::vector<unsigned int> faceIds;
		unsigned int numTris = 0;
		unsigned int j = 0;
		faceIds.resize(itMeshPolygon.count());
		for(itMeshPolygon.reset(); !itMeshPolygon.isDone(); itMeshPolygon.next())
		{
			faceIds[j] = itMeshPolygon.index();
			numTris += triangleCounts[ faceIds[j] ];
			j++;
		}
		
		unsigned int triId = 0;
		material.faceIds.resize(numTris);
		for(unsigned int k = 0; k < itMeshPolygon.count(); k++)
		{
			for(unsigned l = 0; l < (unsigned int) triangleCounts[ faceIds[k] ]; l++)
			{
				material.faceIds[triId + l] = triIndices[ faceIds[k] ] + l;
			}
			triId += triangleCounts[ faceIds[k] ];
		}

		// On récupère le shader
		MObject shaderNode = findShader(set);
		if(shaderNode == MObject::kNullObj)
		{
			continue;
		}

		MFnDependencyNode shaderFnDNode(shaderNode, &status);
		if(!status)
		{
			MGlobal::displayError("MFnDependencyNode::MFnDependencyNode");
			continue;
		}

		// On récupère les informations du shader
		MPlugArray plugArray;
		MPlug colorPlug, transparencyPlug, ambientColorPlug, bumpPlug;

		// Color
		colorPlug = shaderFnDNode.findPlug("color", &status);
		if(!status) {
			MGlobal::displayError("MFnDependencyNode::findPlug : colorPlug");
			continue;
		}
		else
		{
			MItDependencyGraph itDg(colorPlug, MFn::kFileTexture, MItDependencyGraph::kUpstream, MItDependencyGraph::kBreadthFirst, MItDependencyGraph::kNodeLevel, &status);
			if(!status) {
				MGlobal::displayError("MItDependencyGraph::MItDependencyGraph");
				continue;
			}

			itDg.disablePruningOnFilter();

			if(!itDg.isDone())
			{
				MObject textureNode = itDg.thisNode();
				MPlug fileNamePlug = MFnDependencyNode(textureNode).findPlug("fileTextureName");
				fileNamePlug.getValue(material.colorTexName);
			}
		}


		// Transparency
		transparencyPlug = shaderFnDNode.findPlug("transparency", &status);
		if(!status) {
			MGlobal::displayError("MFnDependencyNode::findPlug : transparencyPlug");
			continue;
		}
		else
		{
			MItDependencyGraph itDg(transparencyPlug, MFn::kFileTexture, MItDependencyGraph::kUpstream, MItDependencyGraph::kBreadthFirst, MItDependencyGraph::kNodeLevel, &status);
			if(!status) {
				MGlobal::displayError("MItDependencyGraph::MItDependencyGraph");
				continue;
			}

			itDg.disablePruningOnFilter();

			if(!itDg.isDone())
			{
				MObject textureNode = itDg.thisNode();
				MPlug fileNamePlug = MFnDependencyNode(textureNode).findPlug("fileTextureName");
				fileNamePlug.getValue(material.transparencyTexName);
			}
		}


		// Ambient
		ambientColorPlug = shaderFnDNode.findPlug("ambientColor", &status);
		if(!status) {
			MGlobal::displayError("MFnDependencyNode::findPlug : ambientPlug");
			continue;
		}
		else
		{
			MItDependencyGraph itDg(ambientColorPlug, MFn::kFileTexture, MItDependencyGraph::kUpstream, MItDependencyGraph::kBreadthFirst, MItDependencyGraph::kNodeLevel, &status);
			if(!status) {
				MGlobal::displayError("MItDependencyGraph::MItDependencyGraph");
				continue;
			}

			itDg.disablePruningOnFilter();

			if(!itDg.isDone())
			{
				MObject textureNode = itDg.thisNode();
				MPlug fileNamePlug = MFnDependencyNode(textureNode).findPlug("fileTextureName");
				fileNamePlug.getValue(material.ambientTexName);
			}
		}

		// Bump / normal map
		bumpPlug = shaderFnDNode.findPlug("normalCamera", &status);
		if(!status) {
			MGlobal::displayError("MFnDependencyNode::findPlug : bumpPlug");
			continue;
		}
		else
		{

			MItDependencyGraph itDg(bumpPlug, MFn::kFileTexture, MItDependencyGraph::kUpstream, MItDependencyGraph::kBreadthFirst, MItDependencyGraph::kNodeLevel, &status);
			if(!status) {
				MGlobal::displayError("MItDependencyGraph::MItDependencyGraph");
				continue;
			}

			itDg.disablePruningOnFilter();

			if(!itDg.isDone())
			{
				MObject textureNode = itDg.thisNode();
				MPlug fileNamePlug = MFnDependencyNode(textureNode).findPlug("fileTextureName");
				MString bumpFile;
				fileNamePlug.getValue(material.normalTexName);
			}
		}


		//
		//	Matériau
		//
		MFnLambertShader lambertShader(shaderNode, &status);
		if(status)
		{
			if(material.colorTexName == "") material.color = lambertShader.color()*lambertShader.diffuseCoeff();
			if(material.transparencyTexName == "") material.transparency = lambertShader.transparency();
			if(material.ambientTexName == "") material.ambient = lambertShader.ambientColor();

			MFnPhongShader phongShader(shaderNode, &status);
			if(status)
			{
				MPlug specularColorPlug, specularPowPlug;

				// Specular color
				specularColorPlug = shaderFnDNode.findPlug("specularColor", &status);
				if(!status) {
					MGlobal::displayError("MFnDependencyNode::findPlug : specularColorPlug");
					continue;
				}
				else
				{
					MItDependencyGraph itDg(specularColorPlug, MFn::kFileTexture, MItDependencyGraph::kUpstream, MItDependencyGraph::kBreadthFirst, MItDependencyGraph::kNodeLevel, &status);
					if(!status) {
						MGlobal::displayError("MItDependencyGraph::MItDependencyGraph");
						continue;
					}

					itDg.disablePruningOnFilter();

					if(!itDg.isDone())
					{
						MObject textureNode = itDg.thisNode();
						MPlug fileNamePlug = MFnDependencyNode(textureNode).findPlug("fileTextureName");
						fileNamePlug.getValue(material.specularColorTexName);
					}
				}

				// Specular power
				specularPowPlug = shaderFnDNode.findPlug("cosinePower", &status);
				if(!status) {
					MGlobal::displayError("MFnDependencyNode::findPlug : specularColorPlug");
					continue;
				}
				else
				{
					MItDependencyGraph itDg(specularPowPlug, MFn::kFileTexture, MItDependencyGraph::kUpstream, MItDependencyGraph::kBreadthFirst, MItDependencyGraph::kNodeLevel, &status);
					if(!status) {
						MGlobal::displayError("MItDependencyGraph::MItDependencyGraph");
						continue;
					}

					itDg.disablePruningOnFilter();

					if(!itDg.isDone())
					{
						MObject textureNode = itDg.thisNode();
						MPlug fileNamePlug = MFnDependencyNode(textureNode).findPlug("fileTextureName");
						fileNamePlug.getValue(material.specularPowerTexName);
					}
				}

				if(material.specularColorTexName == "") material.specularColor = phongShader.specularColor();
				if(material.specularPowerTexName == "") material.specularPower = phongShader.cosPower();
			}
			else
			{
				MFnBlinnShader blinnShader(shaderNode, &status);
				if(status)
				{
					// on ne fait rien pour le moment: il faut une méthode pour récupérer et exporter les informations de spécularité
				}
			}
		}

		mesh.materials->push_back(material);
	}

	return MS::kSuccess;
}

MStatus CPMMeshExtractor::assembleMesh(MESH_EXTRACTOR_INFO &mesh, const unsigned int &numVertices)
{
	if(numVertices == 0) {
		MGlobal::displayError("CPMMeshExtractor : le mesh n'a aucun vertice");
		return MS::kFailure;
	}
	
	MPointArray				vertexArray;
	MFloatVectorArray		normalsArray;
	MFloatArray				uArray;
	MFloatArray				vArray;
	MFloatVectorArray		tangentsArray;
	MFloatVectorArray		binormalsArray;
	MColorArray				colorsArray;

	if(m_mesh.getPoints(vertexArray, MSpace::kObject) == MS::kFailure) {
		MGlobal::displayError("MFnMesh::getPoints");
		return MS::kFailure;
	}
	if(mesh.normals) {
		if(m_mesh.getNormals(normalsArray, MSpace::kObject) == MS::kFailure) {
			MGlobal::displayError("MFnMesh::getNormals");
			return MS::kFailure;
		}
	}
	if(mesh.UVs) {
		if(m_mesh.getUVs(uArray, vArray, &mesh.uvSetName) == MS::kFailure) {
			MGlobal::displayError("MFnMesh::getUVs");
			return MS::kFailure;
		}
	}
	if(mesh.tgtBinormals) {
		if(m_mesh.getTangents(tangentsArray, MSpace::kObject, &mesh.uvSetName) == MS::kFailure) {
		MGlobal::displayError("MFnMesh::getTangents");
		return MS::kFailure;
		}
		if(m_mesh.getBinormals(binormalsArray, MSpace::kObject, &mesh.uvSetName) == MS::kFailure) {
			MGlobal::displayError("MFnMesh::getBinormals");
			return MS::kFailure;
		}
	}
	if(mesh.colors) {
		if(m_mesh.getColors(colorsArray, &mesh.colorSetName, NULL) == MS::kFailure) {
			MGlobal::displayError("MFnMesh::getColors");
			return MS::kFailure;
		}
	}

	// On redimensionne les vectors contenant les informations de position, normale...
	mesh.points.setLength(numVertices);
	if(mesh.normals)			mesh.normals->setLength(numVertices);
	if(mesh.UVs)				mesh.UVs->resize(numVertices);
	if(mesh.tgtBinormals)		mesh.tgtBinormals->resize(numVertices);
	if(mesh.colors)				mesh.colors->setLength(numVertices);

	for(unsigned int i = 0; i < m_dVertices.size(); i++)
	{
		for(std::list<DVerticeComponent>::iterator it = m_dVertices[i].begin(); it != m_dVertices[i].end(); it++)
		{			
			mesh.points[it->fVertexId] = vertexArray[i];
			if(mesh.normals) (*mesh.normals)[it->fVertexId] = normalsArray[it->normalId];
			if(mesh.UVs) {
				(*mesh.UVs)[it->fVertexId].u = uArray[it->uvId];
				(*mesh.UVs)[it->fVertexId].v = uArray[it->uvId];
			}
			if(mesh.tgtBinormals) {
				(*mesh.tgtBinormals)[it->fVertexId].tangent = tangentsArray[it->tgtBinormalId];
				(*mesh.tgtBinormals)[it->fVertexId].binormal = binormalsArray[it->tgtBinormalId];
			}
			if(mesh.colors) (*mesh.colors)[it->fVertexId] = colorsArray[it->colorId];
		}
	}

	return MS::kSuccess;
}

unsigned int CPMMeshExtractor::addPoint(const ADD_POINT_INFO &point, unsigned int &actualNumVertices)
{
	for(std::list<DVerticeComponent>::iterator it = m_dVertices[point.pointId].begin(); it != m_dVertices[point.pointId].end(); it++)
	{
		if(point.normalId)			{ if(it->normalId != *point.normalId) continue; }
		if(point.uvId)				{ if(it->uvId != *point.uvId) continue; }
		if(point.tgtBinormalId)		{ if(it->tgtBinormalId != *point.tgtBinormalId) continue; }
		if(point.colorId)			{ if(it->colorId != *point.colorId) continue; }

		return it->fVertexId;
	}
	DVerticeComponent nVertice;
	if(point.normalId)			nVertice.normalId = *(point.normalId);
	if(point.uvId)				nVertice.uvId = *(point.uvId);
	if(point.tgtBinormalId)		nVertice.tgtBinormalId = *(point.tgtBinormalId);
	if(point.colorId)			nVertice.colorId = *(point.colorId);
	nVertice.fVertexId = actualNumVertices;
	m_dVertices[point.pointId].push_back(nVertice);
	actualNumVertices++;

	return actualNumVertices - 1;
}

MObject CPMMeshExtractor::findShader(const MObject &setNode)
{
	MFnDependencyNode fnNode(setNode);
	MPlug shaderPlug = fnNode.findPlug("surfaceShader");

	if(!shaderPlug.isNull())
	{
		MStatus status;
		MPlugArray connectedPlugs;

		shaderPlug.connectedTo(connectedPlugs, true, false, &status);
		if(!status)
		{
			MGlobal::displayError("MPlug::connectedTo");
			return MObject::kNullObj;
		}

		if(connectedPlugs.length() != 1)
		{
			MGlobal::displayError("Erreur lors de l'acquisition du shader de " + m_mesh.fullPathName());
		}
		else
		{
			return connectedPlugs[0].node();
		}
	}
	return MObject::kNullObj;
}