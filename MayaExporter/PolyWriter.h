#ifndef POLYWRITER_H_INCLUDED
#define POLYWRITER_H_INCLUDED

#include <maya/MDagPath.h>
#include <maya/MFnMesh.h>

class PolyWriter
{
	public:
	PolyWriter(const MDagPath &dagPath, MStatus &status);
	virtual ~PolyWriter();

	virtual MStatus extractGeometry() = 0;
	virtual MStatus writeToFile(ostream &os) = 0;

	virtual MObject findShader(const MObject &setNode);

	protected:
	MDagPath	*m_dagPath;
	MFnMesh		*m_mesh;
};

#endif // POLYWRITER_H_INCLUDED