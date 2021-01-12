#ifndef CPM_MESH_EXTRACTOR_H_INCLUDED
#define CPM_MESH_EXTRACTOR_H_INCLUDED

#include <list>
#include <vector>
#include <maya/MIntArray.h>
#include <maya/MPointArray.h>
#include <maya/MFloatVector.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MColorArray.h>
#include <maya/MDagPath.h>
#include <maya/MFnMesh.h>


struct DVerticeComponent
{
	DVerticeComponent(	unsigned int normalId = 0,
						unsigned int tgtBinormalId = 0,
						unsigned int uvId = 0,
						unsigned int colorId = 0,
						unsigned int fVertexId = 0)
	: normalId(normalId), tgtBinormalId(tgtBinormalId), uvId(uvId), colorId(colorId), fVertexId(fVertexId) {}

	unsigned int	normalId;
	unsigned int	tgtBinormalId;
	unsigned int	uvId;
	unsigned int	colorId;

	unsigned int	fVertexId;
};

struct MATERIAL_INFO
{
	MATERIAL_INFO() :	color(1.0f, 1.0f, 1.0f, 1.0f), colorTexName(""), ambient(0.0f, 0.0f, 0.0f, 1.0f), ambientTexName(""), specularColor(0.0f, 0.0f, 0.0f, 0.0f), specularColorTexName(""),
						specularPower(255.0f), specularPowerTexName(""), transparency(0.0f, 0.0f, 0.0f, 0.0f), transparencyTexName(""), normalTexName(""), bumpTexName("") {}
	
	MColor			color;
	MString			colorTexName;

	MColor			specularColor;
	MString			specularColorTexName;

	float			specularPower;
	MString			specularPowerTexName;

	MColor			ambient;
	MString			ambientTexName;

	MColor			transparency;
	MString			transparencyTexName;

	MString			normalTexName;
	MString			bumpTexName;

	std::vector<unsigned int>		faceIds; // faces concernées par le matériau, si faceIds.length() = 0, le mesh entier est concerné
};

struct UV
{
	float u;
	float v;
};

struct TGT_BINORMAL
{
	MFloatVector	tangent;
	MFloatVector	binormal;
};

struct MESH_EXTRACTOR_INFO
{
	MESH_EXTRACTOR_INFO() : normals(NULL), tgtBinormals(NULL), UVs(NULL), colors(NULL), materials(NULL) {}

	MIntArray							triangles;

	MPointArray							points;
	MFloatVectorArray					*normals;
	std::vector<TGT_BINORMAL>			*tgtBinormals;
	std::vector<UV>						*UVs;
	MString								uvSetName;
	MColorArray							*colors;
	MString								colorSetName;

	std::list<MATERIAL_INFO>			*materials;
};

struct ADD_POINT_INFO
{
	ADD_POINT_INFO() : pointId(0), normalId(NULL), uvId(NULL), tgtBinormalId(NULL), colorId(NULL) {}
	ADD_POINT_INFO(int pointId) : pointId(pointId), normalId(NULL), uvId(NULL), tgtBinormalId(NULL), colorId(NULL) {}

	int pointId;
	int *normalId;
	int *uvId;
	int *tgtBinormalId;
	int *colorId;
};

class CPMMeshExtractor
{
	// extrait les données d'un mesh pour l'exportation
	// agence le mesh de façon à pouvoir le charger et créer un mesh valide pour le rendu le plus rapidement possible
	// assemble les données des vertices de façon à en limiter le nombre
	public:
	CPMMeshExtractor(const MDagPath &dagPath, bool objectSpace, MStatus &status);
	~CPMMeshExtractor();

	virtual MStatus extractMesh(MESH_EXTRACTOR_INFO &mesh);

	protected:
	virtual MStatus extractGeometry(MESH_EXTRACTOR_INFO &mesh, unsigned int &numVertices);
	virtual MStatus extractMaterials(MESH_EXTRACTOR_INFO &mesh);
	virtual MStatus assembleMesh(MESH_EXTRACTOR_INFO &mesh, const unsigned int &numVertices);

	virtual unsigned int addPoint(const ADD_POINT_INFO &point, unsigned int &actualNumVertices);
	MObject findShader(const MObject &setNode);

	protected:
	MDagPath			m_dagPath;
	MFnMesh				m_mesh;
	MSpace::Space		m_space;

	// Vertices
	std::vector< std::list<DVerticeComponent> >	m_dVertices; // vertices désassemblés

	// Sets
	MObjectArray						m_polygonSets;
	MObjectArray						m_polygonComponents;
};

#endif // CPM_MESH_EXTRACTOR_H_INCLUDED