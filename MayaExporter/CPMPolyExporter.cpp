#include <maya/MFnPlugin.h>

#include "CPMPolyExporter.h"
#include "CPMPolyWriter.h"


//
//	Fonctions d'initialisation du plugin
//
MStatus initializePlugin(MObject obj)
{
	// on enregistre la classe de fenêtre utilisée pour les fenêtres d'options du PolyExporter
	if(FAILED(CPMPolyExporter::RegisterWindowClass())) return MS::kFailure;

	// on enregistre le plugin
	MStatus status;
	MFnPlugin plugin(obj, "Unknown", "1.0", "Any");

	status = plugin.registerFileTranslator(POLYEXPORTER_NAME, "", CPMPolyExporter::creator, "", "option1=1", true);
	if(!status)
	{
		status.perror("MFnPlugin::registerFileTranslator");
		return status;
	}
	
	return status;
}

MStatus uninitializePlugin(MObject obj)
{
	if(!UnregisterClass(POLYEXPORTER_OPTWNDCLASS_NAME, NULL))
	{
		MGlobal::displayError("UnregisterClass");
	}

	MStatus status;
	MFnPlugin plugin(obj);

	status = plugin.deregisterFileTranslator(POLYEXPORTER_NAME);
	if(!status)
	{
		status.perror("MFnPlugin::deregisterFileTranslator");
		return status;
	}

	return status;
}


//
//	Divers
//
const char *TruncateEndPath(const MString &path, const MString &word)
{
	const char *str = path.asChar();
	const char *wordStr = word.asChar();

	int length = path.length();
	int wordLength = word.length();
	int p = length - 1;

	for(; p >= 0; p--)
	{
		if(str[p] == wordStr[wordLength - 1])
		{
			int i = 1;
			for(; i < wordLength && i <= p && str[p - i] == wordStr[wordLength - 1 - i]; i++)
			{

			}

			p -= i;
			if( i == wordLength)
			{
				MGlobal::displayInfo("word found");
				MString str1, str2;
				str1.set(length);
				str2.set(p);

				MGlobal::displayInfo(str1 + " " + str2);

				std::string nString;
				nString.resize(p + 2);

				if(nString.empty()) return str;

				MGlobal::displayInfo("not empty");

				for(int j = 0; j < p+1; j++)
				{
					nString[j] = str[j];
				}
				return str;
			}
		}
	}

	return str;
}

const char *TruncatePath(const MString &path)
{
	std::string str = path.asChar();
	unsigned int length = path.length();
	int p = length - 1;

	for(; p >= 0 && str[p] != '/' && str[p] != '\\' && str[p] != '|'; p--)
	{

	}

	if(p == -1) return str.c_str();
	return &(str.c_str()[p + 1]);
}

//
//	Fenêtre d'options du PolyExporter
//
#define IDB_NORMALS					100
#define IDB_UV						101
#define IDB_TGT_BINORMALS			102
#define IDB_COLOR					104
#define IDB_INVERTX					105
#define IDB_INVERTY					106
#define IDB_INVERTZ					107
#define IDB_COUNTERCLOCKWISE		108
#define IDB_DOUBLE					110
#define IDB_JOIN_MESHES				111

#define IDB_INVERTU					112
#define IDB_INVERTV					113

#define IDB_MATERIALSETS			200
#define IDB_TEXTURENAMES			201
#define IDB_TRUNC_TEXTURENAMES		202

#define IDB_OK						0
#define	IDB_CANCEL					1


LRESULT CALLBACK CPMPolyExporter::WindowOptionsProc(HWND wnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HINSTANCE hInstance = GetModuleHandle(DLL_NAME);
	static unsigned int exportOptions(CPMPolyExporter::GetExportOptions());

	// Géométrie
	static HWND GeometryGB;
	static HWND ElementsGB;
	static HWND AxesGB;
	static HWND MiscGB;

	static HWND GeometryButtons[12];

	// Matériaux
	static HWND MaterialGB;
	static HWND MaterialButtons[3];

	// Choix
	static HWND OkCancel[2];


	switch(uMsg)
	{
		case(WM_CREATE):
			CPMPolyExporter::BeginMessagesLoop();
			CPMPolyExporter::SetWindowClosedWithOk(false);

			// Géométrie
			GeometryGB = CreateWindow("BUTTON", "Géométrie", BS_GROUPBOX | WS_CHILD | WS_VISIBLE, 10, 10, 570, 380, wnd, NULL, hInstance, NULL);

			ElementsGB = CreateWindow("BUTTON", "Eléments à exporter", BS_GROUPBOX | WS_CHILD | WS_VISIBLE, 10, 20, 550, 110, GeometryGB, NULL, hInstance, NULL);
			GeometryButtons[0] = CreateWindow("BUTTON", "exporter les normales", BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE, 30, 50, 400, 20, wnd, (HMENU) IDB_NORMALS, hInstance, NULL);
			GeometryButtons[1] = CreateWindow("BUTTON", "exporter les coordonnées uv", BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE, 30, 70, 400, 20, wnd, (HMENU) IDB_UV, hInstance, NULL);
			GeometryButtons[2] = CreateWindow("BUTTON", "exporter les tangentes et les binormales", BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE, 50, 60, 400, 20, ElementsGB, (HMENU) IDB_TGT_BINORMALS, hInstance, NULL);
			GeometryButtons[3] = CreateWindow("BUTTON", "exporter les données de couleur", BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE, 10, 80, 400, 20, ElementsGB, (HMENU) IDB_COLOR, hInstance, NULL);
			CheckDlgButton(wnd, IDB_NORMALS, exportOptions & CPM_EXPORT_NORMALS);
			CheckDlgButton(wnd, IDB_UV,  exportOptions & CPM_EXPORT_UVS);
			CheckDlgButton(ElementsGB, IDB_TGT_BINORMALS,  (exportOptions & CPM_EXPORT_TGT_BINORMALS) && (exportOptions & CPM_EXPORT_NORMALS) && (exportOptions & CPM_EXPORT_UVS));
			if(!((exportOptions & CPM_EXPORT_NORMALS) && (exportOptions & CPM_EXPORT_UVS))) EnableWindow(GeometryButtons[2], false);
			CheckDlgButton(ElementsGB, IDB_COLOR,  exportOptions & CPM_EXPORT_COLORS);

			AxesGB = CreateWindow("BUTTON", "Axes", BS_GROUPBOX | WS_CHILD | WS_VISIBLE, 10, 140, 550, 130, GeometryGB, NULL, hInstance, NULL);
			GeometryButtons[4] = CreateWindow("BUTTON", "inverser l'axe des x", BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE, 10, 20, 400, 20, AxesGB, (HMENU) IDB_INVERTX, hInstance, NULL);
			GeometryButtons[5] = CreateWindow("BUTTON", "inverser l'axe des y", BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE, 10, 40, 400, 20, AxesGB, (HMENU) IDB_INVERTY, hInstance, NULL);
			GeometryButtons[6] = CreateWindow("BUTTON", "inverser l'axe des z", BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE, 10, 60, 400, 20, AxesGB, (HMENU) IDB_INVERTZ, hInstance, NULL);
			//GeometryButtons[10] = CreateWindow("BUTTON", "inverser l'axe u", BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE, 30, 230, 400, 20, wnd, (HMENU) IDB_INVERTU, hInstance, NULL);
			GeometryButtons[11] = CreateWindow("BUTTON", "inverser l'axe v et déplacer l'origine du repère UV (en haut à gauche)", BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE, 30, 250, 500, 20, wnd, (HMENU) IDB_INVERTV, hInstance, NULL);
			CheckDlgButton(AxesGB, IDB_INVERTX, exportOptions & CPM_EXPORT_INVERTX);
			CheckDlgButton(AxesGB, IDB_INVERTY, exportOptions & CPM_EXPORT_INVERTY);
			CheckDlgButton(AxesGB, IDB_INVERTZ, exportOptions & CPM_EXPORT_INVERTZ);
			//CheckDlgButton(wnd, IDB_INVERTU, exportOptions & CPM_EXPORT_INVERTU);
			CheckDlgButton(wnd, IDB_INVERTV, exportOptions & CPM_EXPORT_INVERTV);
			if((exportOptions & CPM_EXPORT_UVS) == 0)
			{
				//EnableWindow(GeometryButtons[10], false);
				EnableWindow(GeometryButtons[11], false);
			}
			
			MiscGB = CreateWindow("BUTTON", "Divers", BS_GROUPBOX | WS_CHILD | WS_VISIBLE, 10, 280, 550, 90, GeometryGB, NULL, hInstance, NULL);
			GeometryButtons[7] = CreateWindow("BUTTON", "fusionner les meshes", BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE, 10, 20, 400, 20, MiscGB, (HMENU) IDB_JOIN_MESHES, hInstance, NULL);
			GeometryButtons[8] = CreateWindow("BUTTON", "exporter en double précision si possible (position des vertices)", BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE, 10, 40, 500, 20, MiscGB, (HMENU) IDB_DOUBLE, hInstance, NULL);
			GeometryButtons[9] = CreateWindow("BUTTON", "définir les faces dans le sens contraire des aiguilles d'une montre", BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE, 10, 60, 500, 20, MiscGB, (HMENU) IDB_COUNTERCLOCKWISE, hInstance, NULL);
			CheckDlgButton(MiscGB, IDB_INVERTZ, exportOptions & CPM_EXPORT_JOINMESHES);
			EnableWindow(GeometryButtons[7], false);
			CheckDlgButton(MiscGB, IDB_DOUBLE, exportOptions & CPM_EXPORT_DOUBLE);
			CheckDlgButton(MiscGB, IDB_COUNTERCLOCKWISE, exportOptions & CPM_EXPORT_COUNTERCLOCKWISE);


			// Matériaux
			MaterialGB = CreateWindow("BUTTON", "Matériaux", BS_GROUPBOX | WS_CHILD | WS_VISIBLE, 10, 400, 570, 90, wnd, NULL, hInstance, NULL);
			MaterialButtons[0] = CreateWindow("BUTTON", "exporter les sets de matériaux", BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE, 30, 420, 400, 20, wnd, (HMENU) IDB_MATERIALSETS, hInstance, NULL);
			MaterialButtons[1] = CreateWindow("BUTTON", "exporter les noms des textures associées aux matériaux", BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE, 30, 440, 500, 20, wnd, (HMENU) IDB_TEXTURENAMES, hInstance, NULL);
			MaterialButtons[2] = CreateWindow("BUTTON", "exporter le chemin complet des textures", BS_AUTOCHECKBOX | WS_CHILD | WS_VISIBLE, 80, 460, 500, 20, wnd, (HMENU) IDB_TRUNC_TEXTURENAMES, hInstance, NULL);
			CheckDlgButton(wnd, IDB_MATERIALSETS, exportOptions & CPM_EXPORT_MATERIALSETS);
			CheckDlgButton(wnd, IDB_TEXTURENAMES, exportOptions & CPM_EXPORT_TEXTURENAMES);
			CheckDlgButton(wnd, IDB_TRUNC_TEXTURENAMES, !(exportOptions & CPM_EXPORT_TRUNCATE_TEXTURENAMES));
			if(!(exportOptions & CPM_EXPORT_MATERIALSETS)) EnableWindow(MaterialButtons[1], false);
			if(!(exportOptions & CPM_EXPORT_TEXTURENAMES)) EnableWindow(MaterialButtons[2], false);


			// OK/Cancel
			OkCancel[0] = CreateWindow("BUTTON", "OK", BS_DEFPUSHBUTTON | WS_CHILD | WS_VISIBLE, 375, 500, 100, 20, wnd, (HMENU) IDB_OK, hInstance, NULL);
			OkCancel[1] = CreateWindow("BUTTON", "Annuler", BS_DEFPUSHBUTTON | WS_CHILD | WS_VISIBLE, 480, 500, 100, 20, wnd, (HMENU) IDB_CANCEL, hInstance, NULL);
			
			return 0;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDB_NORMALS:
				case IDB_UV:
					if(IsDlgButtonChecked(wnd, IDB_UV))
					{
						if(IsDlgButtonChecked(wnd, IDB_NORMALS)) EnableWindow(GeometryButtons[2], true);
						else EnableWindow(GeometryButtons[2], false);

						//EnableWindow(GeometryButtons[10], true);
						EnableWindow(GeometryButtons[11], true);
					}
					else
					{
						EnableWindow(GeometryButtons[2], false);

						//EnableWindow(GeometryButtons[10], false);
						EnableWindow(GeometryButtons[11], false);
					}
					break;

				case IDB_MATERIALSETS:
					if(IsDlgButtonChecked(wnd, IDB_MATERIALSETS))
					{
						EnableWindow(MaterialButtons[1], true);
						if(IsDlgButtonChecked(wnd, IDB_TEXTURENAMES)) EnableWindow(MaterialButtons[2], true);
					}
					else
					{
						EnableWindow(MaterialButtons[1], false);
						EnableWindow(MaterialButtons[2], false);
					}
					break;


				case IDB_TEXTURENAMES:
					if(IsDlgButtonChecked(wnd, IDB_TEXTURENAMES))
					{
						EnableWindow(MaterialButtons[2], true);
					}
					else
					{
						EnableWindow(MaterialButtons[2], false);
					}
					break;


				case IDB_OK:
					CPMPolyExporter::SetWindowClosedWithOk(true);
				case IDB_CANCEL:
					DestroyWindow(wnd);
					return 0;

				default:
					break;
			}
			return 0;

		case(WM_DESTROY):
			exportOptions = 0;
			if(IsDlgButtonChecked(wnd, IDB_NORMALS)) exportOptions |= CPM_EXPORT_NORMALS;
			if(IsDlgButtonChecked(wnd, IDB_UV))  exportOptions |= CPM_EXPORT_UVS;
			if(IsDlgButtonChecked(ElementsGB, IDB_TGT_BINORMALS) && (exportOptions & CPM_EXPORT_NORMALS) && (exportOptions & CPM_EXPORT_UVS))  exportOptions |= CPM_EXPORT_TGT_BINORMALS;
			if(IsDlgButtonChecked(ElementsGB, IDB_COLOR))  exportOptions |= CPM_EXPORT_COLORS;

			if(IsDlgButtonChecked(AxesGB, IDB_INVERTX)) exportOptions |= CPM_EXPORT_INVERTX;
			if(IsDlgButtonChecked(AxesGB, IDB_INVERTY)) exportOptions |= CPM_EXPORT_INVERTY;
			if(IsDlgButtonChecked(AxesGB, IDB_INVERTZ)) exportOptions |= CPM_EXPORT_INVERTZ;

			if((exportOptions & CPM_EXPORT_UVS) != 0) {
				//if(IsDlgButtonChecked(wnd, IDB_INVERTU)) exportOptions |= CPM_EXPORT_INVERTU;
				if(IsDlgButtonChecked(wnd, IDB_INVERTV)) exportOptions |= CPM_EXPORT_INVERTV;
			}

			if(IsDlgButtonChecked(MiscGB, IDB_INVERTZ)) exportOptions |= CPM_EXPORT_JOINMESHES;
			if(IsDlgButtonChecked(MiscGB, IDB_DOUBLE)) exportOptions |= CPM_EXPORT_DOUBLE;
			if(IsDlgButtonChecked(MiscGB, IDB_COUNTERCLOCKWISE)) exportOptions |= CPM_EXPORT_COUNTERCLOCKWISE;

			if(IsDlgButtonChecked(wnd, IDB_MATERIALSETS)) exportOptions |= CPM_EXPORT_MATERIALSETS;
			if(IsDlgButtonChecked(wnd, IDB_TEXTURENAMES) && (exportOptions & CPM_EXPORT_MATERIALSETS)) exportOptions |= CPM_EXPORT_TEXTURENAMES;
			if(!IsDlgButtonChecked(wnd, IDB_TRUNC_TEXTURENAMES) && (exportOptions & CPM_EXPORT_TEXTURENAMES)) exportOptions |= CPM_EXPORT_TRUNCATE_TEXTURENAMES;

			CPMPolyExporter::SetExportOptions(exportOptions);

			CPMPolyExporter::EndMessagesLoop();

			return 0;

		case(WM_QUIT):
			return 0;

		default:
			return DefWindowProc(wnd, uMsg, wParam, lParam);
	}
}


//
//	CPMPolyExporter
//

// m_endLoop: détermine si la boucle de gestion des messages du PolyExporter doit s'arrêter ou non
bool CPMPolyExporter::m_endLoop(false);

// m_exportOptions: détermine les options d'exportation du PolyExporter: permet de sauvegarder les options
// choisies par l'utilisateur d'une exportation à une autre et de communiquer ces options au PolyExporter
// après avoir affiché la fenêtre d'options, à travers les fonctions SetExportOptions() et GetExportOptions()
unsigned int CPMPolyExporter::m_exportOptions(CPM_EXPORT_NORMALS | CPM_EXPORT_UVS | CPM_EXPORT_MATERIALSETS | CPM_EXPORT_TEXTURENAMES | CPM_EXPORT_INVERTZ | CPM_EXPORT_INVERTV | CPM_EXPORT_OBJECT_RELATIVE);

// m_closeWindowOk: détermine si l'utilisateur a fermé la fenêtre d'options du PolyExporter avec le bouton OK (true) ou le bouton Annuler (false)
bool CPMPolyExporter::m_windowClosedWithOk(false);

HRESULT CPMPolyExporter::RegisterWindowClass()
{
	HINSTANCE hModule = GetModuleHandle(DLL_NAME);

	WNDCLASSEX wndClass;
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = 0;
	wndClass.lpfnWndProc = WindowOptionsProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = NULL;
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH) (COLOR_MENU + 1);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = POLYEXPORTER_OPTWNDCLASS_NAME;
	wndClass.hIconSm = NULL;

	if(!RegisterClassEx((WNDCLASSEXA*) &wndClass)) return E_FAIL;

	return S_OK;
}

void CPMPolyExporter::BeginMessagesLoop()
{
	m_endLoop = false;
}

void CPMPolyExporter::EndMessagesLoop()
{
	m_endLoop = true;
}

unsigned int CPMPolyExporter::GetExportOptions()
{
	return m_exportOptions;
}

void CPMPolyExporter::SetExportOptions(unsigned int options)
{
	m_exportOptions = options;
}

void CPMPolyExporter::SetWindowClosedWithOk(bool ok)
{
	m_windowClosedWithOk = ok;
}

CPMPolyExporter::CPMPolyExporter()
{

}

CPMPolyExporter::~CPMPolyExporter()
{

}

void *CPMPolyExporter::creator()
{
	return new CPMPolyExporter;
}

MString CPMPolyExporter::defaultExtension() const
{
	return POLYEXPORTER_FORMAT;
}

PolyWriter *CPMPolyExporter::createPolyWriter(const MDagPath &dagPath, MStatus &status) const
{
	return new CPMPolyWriter(dagPath, m_exportOptions, status);
}

void CPMPolyExporter::writeHeader(ostream &f)
{
	f << "CPM_FILE\n\n" << endl;
}

void CPMPolyExporter::writeFooter(ostream &f)
{
	f << "CPM_FILE_END";
}

bool CPMPolyExporter::displayExportWindow(const MFileObject &file, const MString &optionString, FileAccessMode mode)
{
	HINSTANCE hModule = GetModuleHandle(DLL_NAME);

	unsigned int screenW = GetSystemMetrics(SM_CXSCREEN);
	unsigned int screenH = GetSystemMetrics(SM_CYSCREEN);
	unsigned int w = 600, h = 570;
	HWND wnd;
	if( !(wnd = CreateWindow(POLYEXPORTER_OPTWNDCLASS_NAME, "Options d'exportation", WS_SYSMENU | WS_CAPTION, (screenW - w)/2, (screenH - h)/2, w, h, NULL, NULL, hModule, NULL)) )
	{
		MGlobal::displayError("CreateWindow");
		return false;
	}

	ShowWindow(wnd, SW_SHOW);
	UpdateWindow(wnd);

	BOOL bRet;
	MSG msg = {0};

	while((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
	{
		if(bRet == -1)
		{
			MGlobal::displayError("Fatal error");
			PostQuitMessage(0);
			return false;
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);

		if(m_endLoop == true)
		{
			break;
		}
	}

	return m_windowClosedWithOk;
}