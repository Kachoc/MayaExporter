#ifndef POLYEXPORTER_H_INCLUDED
#define POLYEXPORTER_H_INCLUDED

#include <list>
#include <maya/MPxFileTranslator.h>

class MDagPath;
class PolyWriter;

class PolyExporter : public MPxFileTranslator
{
	public:
	PolyExporter();
	virtual ~PolyExporter();

	virtual MStatus writer(const MFileObject &file, const MString &optionString, FileAccessMode mode);

	virtual bool haveReadMethod() const;
	virtual bool haveWriteMethod() const;
	virtual bool canBeOpened() const;

	virtual MString defaultExtension() const = 0;
	virtual MString filter() const;

	protected:
	virtual void clear(); // à appeler avant le retour de la fonction writer: le PolyExporter n'est pas détruit après son appel

	virtual MStatus getSceneMeshesDagPaths();
	virtual MStatus getSelectedMeshesDagPaths();

	virtual bool displayExportWindow(const MFileObject &file, const MString &optionString, FileAccessMode mode);

	virtual void writeHeader(ostream &f);
	virtual void writeFooter(ostream &f);

	virtual MStatus processPolyMesh(const MDagPath &dagPath, ostream &os);
	virtual bool isVisible(const MDagPath &dagPath, MStatus &status);

	virtual PolyWriter *createPolyWriter(const MDagPath &dagPath, MStatus &status) const = 0;

	protected:
	std::list<MDagPath>		m_polyMeshes;
};


#endif // POLYEXPORTER_H_INCLUDED