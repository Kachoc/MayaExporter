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
// Résumé: écrit le fichier
// Args: file - nom du fichier et chemin d'accès à ce fichier
//	     optionString - chaîne contenant les options
//		 mode - la méthode utilisée pour exporter le fichier: tout exporter ou exporter la sélection
{
	MStatus status;

	// on cherche les meshes à exporter
	if(mode == MPxFileTranslator::kExportAccessMode)
	{
		if(getSceneMeshesDagPaths() == MS::kFailure)
		{
			MGlobal::displayError("PolyExporter::getSceneMeshesDagPaths");
			return MS::kFailure;
		}

		if(m_polyMeshes.size() == 0)
		{
			MGlobal::displayError("Aucun mesh à exporter");
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
			MGlobal::displayError("Aucun mesh sélectionné");
			return MS::kFailure;
		}
	}
	else
	{
		MGlobal::displayError("Le mode d'exportation est invalide");
		return MS::kFailure;
	}


	// on affiche la fenêtre pour paramétrer l'exportation
	if(!displayExportWindow(file, optionString, mode))
	{
		// l'exportation n'a pas lieu
		clear();
		return MS::kFailure;
	}
	

	// on crée le fichier
	const MString fileName = file.fullName();
	ofstream newFile(fileName.asChar(), ios::out);
	if(!newFile)
	{
		MGlobal::displayError(fileName + " n'a pas pu être ouvert pour l'écriture");
		clear();
		return MS::kFailure;
	}
	newFile.setf(ios::unitbuf);

	// on écrit le header
	writeHeader(newFile);

	// on exporte les meshes un à un
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
			MGlobal::displayInfo("Mesh " + meshName + " exporté");
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
// Résumé: retourne true si l'exportateur peut lire les fichiers
{
	return false;
}

bool PolyExporter::haveWriteMethod() const
// Résumé: retourne true si l'exportateur peut écrire des fichiers
{
	return true;
}

bool PolyExporter::canBeOpened() const
// Résumé: retourne true si l'exportateur peut ouvrir et importer des fichiers,
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
// Résumé:	enregistre les noms de tous les meshes visibles de la scène pour les exporter
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

		// On récupère le chemin d'accès au mesh
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
// Résumé:	enregistre les noms des meshes sélectionnés pour les exporter
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

		// On récupère le chemin d'accès au mesh
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
// Résumé: affiche une fenêtre permettant à l'utilisateur de configurer l'exportation
// Args: file - nom du fichier et chemin d'accès à ce fichier
//	     optionString - chaîne contenant les options
//		 mode - la méthode utilisée pour exporter le fichier: tout exporter ou exporter la sélection
// Sortie: retourne true si l'exportation doit être effectuée, false sinon (si la fenêtre est fermée par l'utilisateur)
{
	// Par défaut: aucune fe^nêtre ne s'affiche
	return true;
}

void PolyExporter::writeHeader(ostream &os)
// Résumé: écrit ce qui doit apparaître au tout début du fichier
// Args: os - fichier de sortie
{
	os << "";
}

void PolyExporter::writeFooter(ostream &os)
// Résumé: écrit ce qui doit apparaître à la toute fin du fichier
// Args: os - fichier de sortie
{
	os << "";
}

MStatus PolyExporter::processPolyMesh(const MDagPath &dagPath, ostream &os)
// Résumé:	exporte le mesh désigné par dagPath
// Args:	dagPath - désigne le mesh
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
// Résumé: détermine si le mesh représenté par le chemin dagPath est visible
// Args: dagPath - chemin d'accès au mesh
//		 status - détermine si la fonction a échoué on non (sortie)
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