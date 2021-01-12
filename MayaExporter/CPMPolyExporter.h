#ifndef CPM_POLYEXPORTER_H_INCLUDED
#define CPM_POLYEXPORTER_H_INCLUDED

#include <Windows.h>

#include "PolyExporter.h"

#define DLL_NAME	"CrowExporter"

MStatus initializePlugin(MObject obj);
MStatus uninitializePlugin(MObject obj);

#define POLYEXPORTER_NAME				"CrowdPolyMesh"
#define POLYEXPORTER_FORMAT				"cpm"
#define POLYEXPORTER_OPTWNDCLASS_NAME	"CPMPolyExporterOptionsWindowClass"

enum CPM_POLYEXPORT_OPTION
{
	CPM_EXPORT_NORMALS					= 0x1,
	CPM_EXPORT_UVS						= 0x2,
	CPM_EXPORT_TGT_BINORMALS			= 0x4,
	CPM_EXPORT_COLORS					= 0x8,
	CPM_EXPORT_INVERTX					= 0x10,
	CPM_EXPORT_INVERTY					= 0x20,
	CPM_EXPORT_INVERTZ					= 0x40,
	CPM_EXPORT_JOINMESHES				= 0x80,
	CPM_EXPORT_DOUBLE					= 0x100,
	CPM_EXPORT_COUNTERCLOCKWISE			= 0x200,
	CPM_EXPORT_MATERIALSETS				= 0x400,
	CPM_EXPORT_TEXTURENAMES				= 0x800,
	CPM_EXPORT_TRUNCATE_TEXTURENAMES	= 0x1000,
	CPM_EXPORT_INVERTU					= 0x2000,
	CPM_EXPORT_INVERTV					= 0x4000,
	CPM_EXPORT_OBJECT_RELATIVE			= 0x8000,
};

const char *TruncateEndPath(const MString &path, const MString &word);
const char *TruncatePath(const MString &path);

class CPMPolyExporter : public PolyExporter
{
	public:
	CPMPolyExporter();
	virtual ~CPMPolyExporter();

	static void				*creator();

	virtual MString			defaultExtension() const;

	static HRESULT			RegisterWindowClass();
	static void				BeginMessagesLoop();
	static void				EndMessagesLoop();
	
	static LRESULT CALLBACK WindowOptionsProc(HWND wnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static unsigned int		GetExportOptions();
	static void				SetExportOptions(unsigned int options);
	static void				SetWindowClosedWithOk(bool ok);

	protected:
	virtual PolyWriter		*createPolyWriter(const MDagPath &dagPath, MStatus &status) const;

	virtual void			writeHeader(ostream &f);
	virtual void			writeFooter(ostream &f);

	virtual bool			displayExportWindow(const MFileObject &file, const MString &optionString, FileAccessMode mode);

	protected:
	static bool				m_endLoop;
	static unsigned int		m_exportOptions;
	static bool				m_windowClosedWithOk;
};

#endif // CPM_POLYEXPORTER_H_INCLUDED