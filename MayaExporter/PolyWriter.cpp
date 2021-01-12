#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>

#include "PolyWriter.h"

PolyWriter::PolyWriter(const MDagPath &dagPath, MStatus &status)
{
	m_dagPath = new MDagPath(dagPath);
	m_mesh = new MFnMesh(dagPath, &status);
}

PolyWriter::~PolyWriter()
{
	if(m_dagPath) delete m_dagPath;
	if(m_mesh) delete m_mesh;
}

MObject PolyWriter::findShader(const MObject &setNode)
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
			MGlobal::displayError("Erreur lors de l'acquisition du shader de " + m_mesh->fullPathName());
		}
		else
		{
			return connectedPlugs[0].node();
		}
	}
	return MObject::kNullObj;
}