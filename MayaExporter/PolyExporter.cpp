#include <maya/MGlobal.h>
#include <maya/MString.h>
#include <maya/MItDag.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MPlug.h>

#include <maya/MIOStream.h>
#include <maya/MFStream.h>

#include "PolyExporter.h"
#include "PolyWriter.h"


PolyExporter::PolyExporter()
{

}

PolyExporter::~PolyExporter()
{

}

MStatus PolyExporter::writer(const MFileObject &file, const MString &optionString, FileAccessMode mode)
// R�sum�: �crit le fichier
// Args: file - nom du fichier et chemin d'acc�s � ce fichier
//	     optionString - cha�ne contenant les options
//		 mode - la m�thode utilis�e pour exporter le fichier: tout exporter ou exporter la s�lection
{
	MStatus status;

	// on cherche les meshes � exporter
	if(mode == MPxFileTranslator::kExportAccessMode)
	{
		if(getSceneMeshesDagPaths() == MS::kFailure)
		{
			MGlobal::displayError("PolyExporter::getSceneMeshesDagPaths");
			return MS::kFailure;
		}

		if(m_polyMeshes.size() == 0)
		{
			MGlobal::displayError("Aucun mesh � exporter");
			return MS::kFailure;
		}
	}
	else if(mode == MPxFileTranslator::kExportActiveAccessMode)
	{
		if(getSelectedMeshesDagPaths() == MS::kFailure)
		{
			MGlobal::displayError("PolyExporter::getSelectedMeshesDagPaths");
			return MS::kFailure;
		}

		if(m_polyMeshes.size() == 0)
		{
			MGlobal::displayError("Aucun mesh s�lectionn�");
			return MS::kFailure;
		}
	}
	else
	{
		MGlobal::displayError("Le mode d'exportation est invalide");
		return MS::kFailure;
	}


	// on affiche la fen�tre pour param�trer l'exportation
	if(!displayExportWindow(file, optionString, mode))
	{
		// l'exportation n'a pas lieu
		clear();
		return MS::kFailure;
	}
	

	// on cr�e le fichier
	const MString fileName = file.fullName();
	ofstream newFile(fileName.asChar(), ios::out);
	if(!newFile)
	{
		MGlobal::displayError(fileName + " n'a pas pu �tre ouvert pour l'�criture");
		clear();
		return MS::kFailure;
	}
	newFile.setf(ios::unitbuf);

	// on �crit le header
	writeHeader(newFile);

	// on exporte les meshes un � un
	for(std::list<MDagPath>::iterator it = m_polyMeshes.begin(); it != m_polyMeshes.end(); it++)
	{
		if(processPolyMesh(*it, newFile) == MS::kFailure)
		{
			MString meshName = it->fullPathName(&status);
			MGlobal::displayError("Echec lors de l'exportation du mesh " + meshName);

			newFile.flush();
			newFile.close();

			remove(fileName.asChar());
			
			clear();
			return MS::kFailure;
		}
		else
		{
			MString meshName = it->fullPathName(&status);
			MGlobal::displayInfo("Mesh " + meshName + " export�");
		}
	}

	// on ecrit le footer et on ferme le fichier
	writeFooter(newFile);

	newFile.flush();
	newFile.close();

	clear();

	return MS::kSuccess;
}

bool PolyExporter::haveReadMethod() const
// R�sum�: retourne true si l'exportateur peut lire les fichiers
{
	return false;
}

bool PolyExporter::haveWriteMethod() const
// R�sum�: retourne true si l'exportateur peut �crire des fichiers
{
	return true;
}

bool PolyExporter::canBeOpened() const
// R�sum�: retourne true si l'exportateur peut ouvrir et importer des fichiers,
// false s'il peut seulement les importer
{
	return true;
}

MString PolyExporter::filter() const
{
	return "." + defaultExtension();
}

void PolyExporter::clear()
{
	m_polyMeshes.clear();
}

MStatus PolyExporter::getSceneMeshesDagPaths()
// R�sum�:	enregistre les noms de tous les meshes visibles de la sc�ne pour les exporter
{
	MStatus status;

	MItDag itDag(MItDag::kDepthFirst, MFn::kMesh, &status);
	if(status == MS::kFailure)
	{
		MGlobal::displayError("itDag");
		return MS::kFailure;
	}

	for(; !itDag.isDone(); itDag.next())
	{
		MDagPath dagPath;

		// On r�cup�re le chemin d'acc�s au mesh
		if(itDag.getPath(dagPath) == MS::kFailure)
		{
			MGlobal::displayError("MItDag::getPath");
			return MS::kFailure;
		}

		if(isVisible(dagPath, status) == true && status == MS::kSuccess)
		{
			m_polyMeshes.push_back(dagPath);
		}
	}

	return MS::kSuccess;
}

MStatus PolyExporter::getSelectedMeshesDagPaths()
// R�sum�:	enregistre les noms des meshes s�lectionn�s pour les exporter
{
	MStatus status;

	MSelectionList selectionList;
	if(MGlobal::getActiveSelectionList(selectionList) == MS::kFailure)
	{
		MGlobal::displayError("MGlobal::getActiveSelectionList");
		return MS::kFailure;
	}

	MItSelectionList itSelectionList(selectionList, MFn::kMesh, &status);
	if(status == MS::kFailure)
	{
		MGlobal::displayError("MItSelectionList");
		return MS::kFailure;
	}

	for(itSelectionList.reset(); !itSelectionList.isDone(); itSelectionList.next())
	{
		MDagPath dagPath;

		// On r�cup�re le chemin d'acc�s au mesh
		if(itSelectionList.getDagPath(dagPath) == MS::kFailure)
		{
			MGlobal::displayError("MItSelectionList::getDagPath");
			return MS::kFailure;
		}

		m_polyMeshes.push_back(dagPath);
	}

	return MS::kSuccess;
}

bool PolyExporter::displayExportWindow(const MFileObject &file, const MString &optionString, FileAccessMode mode)
// R�sum�: affiche une fen�tre permettant � l'utilisateur de configurer l'exportation
// Args: file - nom du fichier et chemin d'acc�s � ce fichier
//	     optionString - cha�ne contenant les options
//		 mode - la m�thode utilis�e pour exporter le fichier: tout exporter ou exporter la s�lection
// Sortie: retourne true si l'exportation doit �tre effectu�e, false sinon (si la fen�tre est ferm�e par l'utilisateur)
{
	// Par d�faut: aucune fe^n�tre ne s'affiche
	return true;
}

void PolyExporter::writeHeader(ostream &os)
// R�sum�: �crit ce qui doit appara�tre au tout d�but du fichier
// Args: os - fichier de sortie
{
	os << "";
}

void PolyExporter::writeFooter(ostream &os)
// R�sum�: �crit ce qui doit appara�tre � la toute fin du fichier
// Args: os - fichier de sortie
{
	os << "";
}

MStatus PolyExporter::processPolyMesh(const MDagPath &dagPath, ostream &os)
// R�sum�:	exporte le mesh d�sign� par dagPath
// Args:	dagPath - d�signe le mesh
//			os - sortie
{
	MStatus status;

	PolyWriter *writer = createPolyWriter(dagPath, status);
	if(status == MS::kFailure)
	{
		delete writer;
		return MS::kFailure;
	}

	if(writer->extractGeometry() == MS::kFailure)
	{
		delete writer;
		return MS::kFailure;
	}

	if(writer->writeToFile(os) == MS::kFailure)
	{
		delete writer;
		return MS::kFailure;
	}

	if(writer) delete writer;

	return MS::kSuccess;
}

bool PolyExporter::isVisible(const MDagPath &dagPath, MStatus &status)
// R�sum�: d�termine si le mesh repr�sent� par le chemin dagPath est visible
// Args: dagPath - chemin d'acc�s au mesh
//		 status - d�termine si la fonction a �chou� on non (sortie)
// Sortie: status
//		   true si le mesh est visible
//		   false sinon
{
	MFnDagNode dagNode(dagPath);

	if(dagNode.isIntermediateObject()) return true;

	MPlug visPlug = dagNode.findPlug("visibility", &status);
	if(status == MS::kFailure)
	{
		MGlobal::displayError("MFnDagNode::findPlug");
		return false;
	}

	bool visible;
	status = visPlug.getValue(visible);
	if(status == MS::kFailure)
	{
		MGlobal::displayError("MPlug::getValue");
		return false;
	}

	return visible;
}