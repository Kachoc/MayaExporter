#include <maya/MFnSet.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MPlug.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MFnLambertShader.h>
#include <maya/MFnPhongShader.h>
#include <maya/MFnBlinnShader.h>

#include "CPMPolyWriter.h"
#include "CPMPolyExporter.h"

#define RET_VALUE(CONDITION, VALUE) (((CONDITION) != 0) ? (VALUE) : (0))


//
//	CPMPolyWriter
//
CPMPolyWriter::CPMPolyWriter(const MDagPath &dagPath, unsigned int exportOptions, MStatus &status) : PolyWriter(dagPath, status), m_exportOptions(exportOptions)
{

}

CPMPolyWriter::~CPMPolyWriter()
{

}

MStatus CPMPolyWriter::extractGeometry()
{
	MStatus status;	

	MFnDagNode dagNode(*m_dagPath);
	MFnDagNode parentNode(dagNode.parent(0, &status));
	if(!status) {
		m_meshName = m_dagPath->partialPathName();
	}
	else {
		m_meshName  = parentNode.partialPathName();
	}
	m_transformMatrix = m_dagPath->inclusiveMatrix(&status);
	if(!status) {
		MGlobal::displayError("MDagPath::inclusiveMatrix");
	}

	CPMMeshExtractor meshExtractor(*m_dagPath, !(m_exportOptions & CPM_EXPORT_OBJECT_RELATIVE), status);
	if(!status) {
		MGlobal::displayError("CPMPolyWriter::extractGeometry : CPMMeshExtractor::CPMMeshExtractor");
		return MS::kFailure;
	}

	MESH_EXTRACTOR_INFO extractedMesh;
	if(m_exportOptions & CPM_EXPORT_NORMALS) extractedMesh.normals = &m_normals;
	if(m_exportOptions & CPM_EXPORT_TGT_BINORMALS) extractedMesh.tgtBinormals = &m_tgtBinormals;
	if(m_exportOptions & CPM_EXPORT_UVS) extractedMesh.UVs = &m_UVs;
	if(m_exportOptions & CPM_EXPORT_COLORS) extractedMesh.colors = &m_colors;
	if(m_exportOptions & CPM_EXPORT_MATERIALSETS) extractedMesh.materials = &m_materials;

	status = meshExtractor.extractMesh(extractedMesh);
	if(!status) {
		MGlobal::displayError("CPMPolyWriter::extractGeometry : CPMMeshExtractor::extractMesh");
		return MS::kFailure;
	}

	m_triangles = extractedMesh.triangles;
	m_points = extractedMesh.points;
	m_uvSetName = extractedMesh.uvSetName;
	m_colorSetName = extractedMesh.colorSetName;

	return MS::kSuccess;
}

MStatus CPMPolyWriter::writeToFile(ostream &os)
{
	if(outputObjectProperties(os) == MS::kFailure) return MS::kFailure;
	if(outputTriangles(os) == MS::kFailure) return MS::kFailure;
	if(outputVertices(os) == MS::kFailure) return MS::kFailure;
	if(outputNormals(os) == MS::kFailure) return MS::kFailure;
	if(outputTangents(os) == MS::kFailure) return MS::kFailure;
	if(outputBinormals(os) == MS::kFailure) return MS::kFailure;
	if(outputUVs(os) == MS::kFailure) return MS::kFailure;
	if(outputColors(os) == MS::kFailure) return MS::kFailure;
	if(outputMaterialSets(os) == MS::kFailure) return MS::kFailure;

	return MS::kSuccess;
}

MStatus CPMPolyWriter::outputObjectProperties(ostream &os)
{
	os << "Object: " << m_meshName.asChar() << endl;
	os << "TransformMatrix: " << endl;
	os << m_transformMatrix[0][0] << " " << m_transformMatrix[0][1] << " " << m_transformMatrix[0][2] << " " << m_transformMatrix[0][3] << endl;
	os << m_transformMatrix[1][0] << " " << m_transformMatrix[1][1] << " " << m_transformMatrix[1][2] << " " << m_transformMatrix[1][3] << endl;
	os << m_transformMatrix[2][0] << " " << m_transformMatrix[2][1] << " " << m_transformMatrix[2][2] << " " << m_transformMatrix[2][3] << endl;
	os << m_transformMatrix[3][0] << " " << m_transformMatrix[3][1] << " " << m_transformMatrix[3][2] << " " << m_transformMatrix[3][3] << endl;
	os << endl;

	return MS::kSuccess;
}

MStatus CPMPolyWriter::outputTriangles(ostream &os)
{
	unsigned int numTriangles = m_triangles.length() / 3;

	os << "Triangles: " << numTriangles << endl;
	for(unsigned int i = 0; i < numTriangles; i++)
	{
		((m_exportOptions & CPM_EXPORT_COUNTERCLOCKWISE) != 0 ? os << m_triangles[3*i] << " " << m_triangles[3*i + 1] << " " << m_triangles[3*i + 2] << endl
		: os << m_triangles[3*i] << " " << m_triangles[3*i + 2] << " " << m_triangles[3*i + 1] << endl);
	}
	os << "\n\n";

	return MS::kSuccess;
}

MStatus CPMPolyWriter::outputVertices(ostream &os)
{
	unsigned int numVertices = m_points.length();

	os << "Vertices: " << numVertices << endl;
	for(unsigned int i = 0; i < numVertices; i++)
	{
		os << ((m_exportOptions & CPM_EXPORT_INVERTX) != 0 ? -m_points[i].x : m_points[i].x) << " "
		<< ((m_exportOptions & CPM_EXPORT_INVERTY) != 0 ? -m_points[i].y : m_points[i].y) << " " <<
		((m_exportOptions & CPM_EXPORT_INVERTZ) != 0 ? -m_points[i].z : m_points[i].z) << endl;
	}
	os << "\n\n";

	return MS::kSuccess;
}

MStatus CPMPolyWriter::outputNormals(ostream &os)
{
	if(m_exportOptions & CPM_EXPORT_NORMALS)
	{
		unsigned int numNormals = m_normals.length();

		os << "Normals: " << numNormals << endl;
		for(unsigned int i = 0; i < numNormals; i++)
		{
			os << ((m_exportOptions & CPM_EXPORT_INVERTX) != 0 ? -m_normals[i].x : m_normals[i].x) << " "
			<< ((m_exportOptions & CPM_EXPORT_INVERTY) != 0 ? -m_normals[i].y : m_normals[i].y) << " " <<
			((m_exportOptions & CPM_EXPORT_INVERTZ) != 0 ? -m_normals[i].z : m_normals[i].z) << endl;
		}
		os << "\n\n";
	}

	return MS::kSuccess;
}

MStatus CPMPolyWriter::outputTangents(ostream &os)
{
	if(m_exportOptions & CPM_EXPORT_TGT_BINORMALS)
	{
		unsigned int numTangents = (unsigned int) m_tgtBinormals.size();

		os << "Tangents: " << numTangents << endl;
		for(unsigned int i = 0; i < numTangents; i++)
		{
			os << ((m_exportOptions & CPM_EXPORT_INVERTX) != 0 ? -m_tgtBinormals[i].tangent.x : m_tgtBinormals[i].tangent.x) << " "
			<< ((m_exportOptions & CPM_EXPORT_INVERTY) != 0 ? -m_tgtBinormals[i].tangent.y : m_tgtBinormals[i].tangent.y) << " " <<
			((m_exportOptions & CPM_EXPORT_INVERTZ) != 0 ? -m_tgtBinormals[i].tangent.z : m_tgtBinormals[i].tangent.z) << endl;
		}
		os << "\n\n";
	}

	return MS::kSuccess;
}

MStatus CPMPolyWriter::outputBinormals(ostream &os)
{
	if(m_exportOptions & CPM_EXPORT_TGT_BINORMALS)
	{
		unsigned int numBitangents = (unsigned int) m_tgtBinormals.size();

		os << "Bitangents: " << numBitangents << endl;
		for(unsigned int i = 0; i < numBitangents; i++)
		{
			os << ((m_exportOptions & CPM_EXPORT_INVERTX) != 0 ? -m_tgtBinormals[i].binormal.x : m_tgtBinormals[i].binormal.x) << " "
			<< ((m_exportOptions & CPM_EXPORT_INVERTY) != 0 ? -m_tgtBinormals[i].binormal.y : m_tgtBinormals[i].binormal.y) << " " <<
			((m_exportOptions & CPM_EXPORT_INVERTZ) != 0 ? -m_tgtBinormals[i].binormal.z : m_tgtBinormals[i].binormal.z) << endl;
		}
		os << "\n\n";
	}

	return MS::kSuccess;
}

MStatus CPMPolyWriter::outputUVs(ostream &os)
{
	if(m_exportOptions & CPM_EXPORT_UVS)
	{
		unsigned int numUVs = (int) m_UVs.size();

		os << "UVs: " << numUVs << endl;
		for(unsigned int i = 0; i < numUVs; i++)
		{
			os << m_UVs[i].u << " " << ((m_exportOptions & CPM_EXPORT_INVERTV) != 0 ? -m_UVs[i].v + 1.0f : m_UVs[i].v) << endl;
		}
		os << "\n\n";
	}

	return MS::kSuccess;
}

MStatus CPMPolyWriter::outputColors(ostream &os)
{
	return MS::kSuccess;
}

MStatus CPMPolyWriter::outputMaterialSets(ostream &os)
{
	if(m_exportOptions & CPM_EXPORT_MATERIALSETS)
	{
		unsigned int numSets = (unsigned int) m_materials.size();

		os << "Materials: " << numSets << "\n" << endl;
		for(std::list<MATERIAL_INFO>::iterator it = m_materials.begin(); it != m_materials.end(); it++)
		{
			os << "material:" << endl;

			if(it->colorTexName != "" && (m_exportOptions & CPM_EXPORT_TEXTURENAMES)) {
				os << "colorTexName: " << ((m_exportOptions & CPM_EXPORT_TRUNCATE_TEXTURENAMES) != 0 ? TruncatePath(it->colorTexName) : it->colorTexName.asChar()) << endl;
			}
			else { os << "color: " << it->color.r << " " << it->color.g << " " << it->color.b << " " << it->color.a << endl; }

			if(it->specularColorTexName != "" && (m_exportOptions & CPM_EXPORT_TEXTURENAMES)) {
				os << "specularColorTexName: " << ((m_exportOptions & CPM_EXPORT_TRUNCATE_TEXTURENAMES) != 0 ? TruncatePath(it->specularColorTexName) : it->specularColorTexName.asChar()) << endl;
			}
			else { os << "specularColor: " << it->specularColor.r << " " << it->specularColor.g << " " << it->specularColor.b << " " << it->specularColor.a << endl; }

			if(it->specularPowerTexName != "" && (m_exportOptions & CPM_EXPORT_TEXTURENAMES)) {
				os << "specularPowerTexName: " << ((m_exportOptions & CPM_EXPORT_TRUNCATE_TEXTURENAMES) != 0 ? TruncatePath(it->specularPowerTexName) : it->specularPowerTexName.asChar()) << endl;
			}
			else { os << "specularPower: " << it->specularPower << endl; }

			if(it->ambientTexName != "" && (m_exportOptions & CPM_EXPORT_TEXTURENAMES)) {
				os << "ambientTexName: " << ((m_exportOptions & CPM_EXPORT_TRUNCATE_TEXTURENAMES) != 0 ? TruncatePath(it->ambientTexName) : it->ambientTexName.asChar()) << endl;
			}
			else { os << "ambient: " << it->ambient.r << " " << it->ambient.g << " " << it->ambient.b << " " << it->ambient.a << endl; }

			if(it->transparencyTexName != "" && (m_exportOptions & CPM_EXPORT_TEXTURENAMES)) {
				os << "transparencyTexName: " << ((m_exportOptions & CPM_EXPORT_TRUNCATE_TEXTURENAMES) != 0 ? TruncatePath(it->transparencyTexName) : it->transparencyTexName.asChar()) << endl;
			}
			else { os << "transparency: " << it->transparency.r << " " << it->transparency.g << " " << it->transparency.b << " " << it->transparency.a << endl; }

			if(it->normalTexName != "" && (m_exportOptions & CPM_EXPORT_TEXTURENAMES)) {
				os << "normalTexName: " << ((m_exportOptions & CPM_EXPORT_TRUNCATE_TEXTURENAMES) != 0 ? TruncatePath(it->normalTexName) : it->normalTexName.asChar()) << endl;
			}

			if(it->bumpTexName != "" && (m_exportOptions & CPM_EXPORT_TEXTURENAMES)) {
				os << "bumpTexName: " << ((m_exportOptions & CPM_EXPORT_TRUNCATE_TEXTURENAMES) != 0 ? TruncatePath(it->bumpTexName) : it->bumpTexName.asChar()) << endl;
			}

			if(m_materials.size() != 1)
			{
				unsigned int numFaces;
				numFaces = (unsigned int) it->faceIds.size();
				os << "Faces: " << numFaces << endl;
				for(unsigned int i = 0; i < numFaces; i++)
				{
					os << it->faceIds[i] << endl;
				}
			}
			else
			{
				os << "faces: " << 0 << endl;
			}

			os << "\n";
		}
		os << "\n\n";
	}

	return MS::kSuccess;
}