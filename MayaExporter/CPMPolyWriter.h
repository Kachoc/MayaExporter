#ifndef CPM_POLYWRITER_H_INCLUDED
#define CPM_POLYWRITER_H_INCLUDED

#include <vector>
#include <list>

#include <maya/MGlobal.h>
#include <maya/MPointArray.h>
#include <maya/MColorArray.h>
#include <maya/MFloatVector.h>
#include <maya/MFloatArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MStringArray.h>
#include <maya/MMatrix.h>

#include "PolyWriter.h"
#include "CPMMeshExtractor.h"

class CPMPolyWriter : public PolyWriter
{
	public:
	CPMPolyWriter(const MDagPath &dagPath, unsigned int exportOptions, MStatus &status);
	virtual ~CPMPolyWriter();

	virtual MStatus extractGeometry();
	virtual MStatus writeToFile(ostream &os);

	protected:
	virtual MStatus outputObjectProperties(ostream &os);
	virtual MStatus outputTriangles(ostream &os);
	virtual MStatus outputVertices(ostream &os);
	virtual MStatus outputNormals(ostream &os);
	virtual MStatus outputTangents(ostream &os);
	virtual MStatus outputBinormals(ostream &os);
	virtual MStatus outputUVs(ostream &os);
	virtual MStatus outputColors(ostream &os);
	virtual MStatus outputMaterialSets(ostream &os);

	private:

	protected:
	unsigned int						m_exportOptions;

	MString								m_meshName;
	MMatrix								m_transformMatrix;

	MIntArray							m_triangles;

	MPointArray							m_points;
	MFloatVectorArray					m_normals;
	std::vector<TGT_BINORMAL>			m_tgtBinormals;
	std::vector<UV>						m_UVs;
	MString								m_uvSetName;
	MColorArray							m_colors;
	MString								m_colorSetName;

	std::list<MATERIAL_INFO>			m_materials;
};

#endif // CPM_POLYWRITER_H_INCLUDED