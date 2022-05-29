// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

#include "Controller 1.1.h"
#include <filesystem>
#include <fstream>

#pragma warning(disable : 4996)

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        CloseDLL();
        break;
    }
    return TRUE;
}

static HMODULE gJabo = nullptr;

typedef EXPORT void (*CALL CloseDLLFn)           (void);
typedef EXPORT void (*CALL ControllerCommandFn)  (int Control, BYTE* Command);
typedef EXPORT void (*CALL DllAboutFn)           (HWND hParent);
typedef EXPORT void (*CALL DllConfigFn)          (HWND hParent);
typedef EXPORT void (*CALL DllTestFn)            (HWND hParent);
typedef EXPORT void (*CALL GetDllInfoFn)         (PLUGIN_INFO* PluginInfo);
typedef EXPORT void (*CALL GetKeysFn)            (int Control, BUTTONS* Keys);
typedef EXPORT void (*CALL InitiateControllersFn)(HWND hMainWindow, CONTROL Controls[4]);
typedef EXPORT void (*CALL ReadControllerFn)     (int Control, BYTE* Command);
typedef EXPORT void (*CALL RomClosedFn)          (void);
typedef EXPORT void (*CALL RomOpenFn)            (void);
typedef EXPORT void (*CALL WM_KeyDownFn)         (WPARAM wParam, LPARAM lParam);
typedef EXPORT void (*CALL WM_KeyUpFn)           (WPARAM wParam, LPARAM lParam);

static CloseDLLFn            gJaboCloseDLL;
static ControllerCommandFn   gJaboControllerCommand;
static DllAboutFn            gJaboDllAbout;
static DllConfigFn           gJaboDllConfig;
static DllTestFn             gJaboDllTest;
static GetDllInfoFn          gJaboGetDllInfo;
static GetKeysFn             gJaboGetKeys;
static InitiateControllersFn gJaboInitiateControllers;
static ReadControllerFn      gJaboReadController;
static RomClosedFn           gJaboRomClosed;
static RomOpenFn             gJaboRomOpen;
static WM_KeyDownFn          gJaboWM_KeyDown;
static WM_KeyUpFn            gJaboWM_KeyUp;

static int  gMultipliersTable[255]{};

static bool gActiveKeys[255]{};
static int gDivisor = 1;

static void loadJabo()
{
    if (gJabo)
        return;

    char path[MAX_PATH];
    HMODULE hm = NULL;

    if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&loadJabo, &hm) == 0)
    {
        return;
    }
    if (GetModuleFileName(hm, path, sizeof(path)) == 0)
    {
        return;
    }

    std::filesystem::path dllpath{ path };
    auto dir = dllpath.remove_filename();
    {
        auto jabopath = dir / "JABO.dat";
        gJabo = LoadLibraryW(jabopath.c_str());

        gJaboCloseDLL = (CloseDLLFn)GetProcAddress(gJabo, "CloseDLL");
        gJaboControllerCommand = (ControllerCommandFn)GetProcAddress(gJabo, "ControllerCommand");
        gJaboDllAbout = (DllAboutFn)GetProcAddress(gJabo, "DllAbout");
        gJaboDllConfig = (DllConfigFn)GetProcAddress(gJabo, "DllConfig");
        gJaboDllTest = (DllTestFn)GetProcAddress(gJabo, "DllTest");
        gJaboGetDllInfo = (GetDllInfoFn)GetProcAddress(gJabo, "GetDllInfo");
        gJaboGetKeys = (GetKeysFn)GetProcAddress(gJabo, "GetKeys");
        gJaboInitiateControllers = (InitiateControllersFn)GetProcAddress(gJabo, "InitiateControllers");
        gJaboReadController = (ReadControllerFn)GetProcAddress(gJabo, "ReadController");
        gJaboRomClosed = (RomClosedFn)GetProcAddress(gJabo, "RomClosed");
        gJaboRomOpen = (RomOpenFn)GetProcAddress(gJabo, "RomOpen");
        gJaboWM_KeyDown = (WM_KeyDownFn)GetProcAddress(gJabo, "WM_KeyDown");
        gJaboWM_KeyUp = (WM_KeyUpFn)GetProcAddress(gJabo, "WM_KeyUp");
    }

    {
        memset(gMultipliersTable, 0, sizeof(gMultipliersTable));
        auto cfgpath = dir / "KeyboardInputConfig.txt";
        FILE* f = _wfopen(cfgpath.c_str(), L"r");
        int divisor, code;
        while (2 == fscanf(f, "%d %d", &divisor, &code))
        {
            if (0 <= code && code <= sizeof(gMultipliersTable) / sizeof(*gMultipliersTable))
            gMultipliersTable[code] = divisor;
        }
        fclose(f);
    }
}

/******************************************************************
  Function: CloseDLL
  Purpose:  This function is called when the emulator is closing
            down allowing the dll to de-initialise.
  input:    none
  output:   none
*******************************************************************/
EXPORT void CALL CloseDLL(void)
{
    if (gJabo)
    {
        CloseHandle(gJabo);
        gJabo = nullptr;
    }
}

/******************************************************************
  Function: ControllerCommand
  Purpose:  To process the raw data that has just been sent to a
            specific controller.
  input:    - Controller Number (0 to 3) and -1 signalling end of
              processing the pif ram.
            - Pointer of data to be processed.
  output:   none
  note:     This function is only needed if the DLL is allowing raw
            data, or the plugin is set to raw
            the data that is being processed looks like this:
            initilize controller: 01 03 00 FF FF FF
            read controller:      01 04 01 FF FF FF FF
*******************************************************************/
EXPORT void CALL ControllerCommand(int Control, BYTE* Command)
{
    loadJabo();
    gJaboControllerCommand(Control, Command);
}

/******************************************************************
  Function: DllAbout
  Purpose:  This function is optional function that is provided
            to give further information about the DLL.
  input:    a handle to the window that calls this function
  output:   none
*******************************************************************/
EXPORT void CALL DllAbout(HWND hParent)
{
    loadJabo();
    gJaboDllAbout(hParent);
}

/******************************************************************
  Function: DllConfig
  Purpose:  This function is optional function that is provided
            to allow the user to configure the dll
  input:    a handle to the window that calls this function
  output:   none
*******************************************************************/
EXPORT void CALL DllConfig(HWND hParent)
{
    loadJabo();
    gJaboDllConfig(hParent);
}

/******************************************************************
  Function: DllTest
  Purpose:  This function is optional function that is provided
            to allow the user to test the dll
  input:    a handle to the window that calls this function
  output:   none
*******************************************************************/
EXPORT void CALL DllTest(HWND hParent)
{
    loadJabo();
    gJaboDllTest(hParent);
}

/******************************************************************
  Function: GetDllInfo
  Purpose:  This function allows the emulator to gather information
            about the dll by filling in the PluginInfo structure.
  input:    a pointer to a PLUGIN_INFO stucture that needs to be
            filled by the function. (see def above)
  output:   none
*******************************************************************/
EXPORT void CALL GetDllInfo(PLUGIN_INFO* PluginInfo)
{
    loadJabo();
    gJaboGetDllInfo(PluginInfo);
    PluginInfo->Name[0] = 'L';
    PluginInfo->Name[1] = 'I';
    PluginInfo->Name[2] = 'N';
    PluginInfo->Name[3] = 'K';
}

/******************************************************************
  Function: GetKeys
  Purpose:  To get the current state of the controllers buttons.
  input:    - Controller Number (0 to 3)
            - A pointer to a BUTTONS structure to be filled with
            the controller state.
  output:   none
*******************************************************************/
EXPORT void CALL GetKeys(int Control, BUTTONS* Keys)
{
    loadJabo();
    gJaboGetKeys(Control, Keys);
    Keys->X_AXIS /= gDivisor;
    Keys->Y_AXIS /= gDivisor;
}

/******************************************************************
  Function: InitiateControllers
  Purpose:  This function initialises how each of the controllers
            should be handled.
  input:    - The handle to the main window.
            - A controller structure that needs to be filled for
              the emulator to know how to handle each controller.
  output:   none
*******************************************************************/
EXPORT void CALL InitiateControllers(HWND hMainWindow, CONTROL Controls[4])
{
    loadJabo();
    gJaboInitiateControllers(hMainWindow, Controls);
}

/******************************************************************
  Function: ReadController
  Purpose:  To process the raw data in the pif ram that is about to
            be read.
  input:    - Controller Number (0 to 3) and -1 signalling end of
              processing the pif ram.
            - Pointer of data to be processed.
  output:   none
  note:     This function is only needed if the DLL is allowing raw
            data.
*******************************************************************/
EXPORT void CALL ReadController(int Control, BYTE* Command)
{
    loadJabo();
    gJaboReadController(Control, Command);
}

/******************************************************************
  Function: RomClosed
  Purpose:  This function is called when a rom is closed.
  input:    none
  output:   none
*******************************************************************/
EXPORT void CALL RomClosed(void)
{
    loadJabo();
    gJaboRomClosed();
}

/******************************************************************
  Function: RomOpen
  Purpose:  This function is called when a rom is open. (from the
            emulation thread)
  input:    none
  output:   none
*******************************************************************/
EXPORT void CALL RomOpen(void)
{
    loadJabo();
    gJaboRomOpen();
    gDivisor = 1;
    memset(gActiveKeys, 0, sizeof(gActiveKeys));
}

/******************************************************************
  Function: WM_KeyDown
  Purpose:  To pass the WM_KeyDown message from the emulator to the
            plugin.
  input:    wParam and lParam of the WM_KEYDOWN message.
  output:   none
*******************************************************************/
EXPORT void CALL WM_KeyDown(WPARAM wParam, LPARAM lParam)
{
    loadJabo();
    gJaboWM_KeyDown(wParam, lParam);
    if (0 <= wParam && wParam <= sizeof(gActiveKeys) / sizeof(*gActiveKeys))
    {
        auto& active = gActiveKeys[wParam];
        auto& mult   = gMultipliersTable[wParam];
        if (mult)
        {
            if (!active)
            {
                gDivisor *= mult;
            }

            active = true;
        }
    }
}

/******************************************************************
  Function: WM_KeyUp
  Purpose:  To pass the WM_KEYUP message from the emulator to the
            plugin.
  input:    wParam and lParam of the WM_KEYDOWN message.
  output:   none
*******************************************************************/
EXPORT void CALL WM_KeyUp(WPARAM wParam, LPARAM lParam)
{
    loadJabo();
    gJaboWM_KeyUp(wParam, lParam);
    if (0 <= wParam && wParam <= sizeof(gActiveKeys) / sizeof(*gActiveKeys))
    {
        auto& active = gActiveKeys[wParam];
        auto& mult   = gMultipliersTable[wParam];
        if (mult)
        {
            if (active)
            {
                gDivisor /= mult;
            }

            active = false;
        }
    }
}
