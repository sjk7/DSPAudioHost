// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
// DspAudioHost.cpp : Defines the class behaviors for the application.
//

#include "pch.h"
#include "framework.h"
#include "DspAudioHost.h"
#include "DspAudioHostDlg.h"

#pragma comment(lib, "winmm")

#ifdef _DEBUG
#define new DEBUG_NEW

#endif

// CDspAudioHostApp

BEGIN_MESSAGE_MAP(CDspAudioHostApp, CWinApp)
ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

// CDspAudioHostApp construction

CDspAudioHostApp::CDspAudioHostApp() {
    // support Restart Manager
    m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;

    // TODO: add construction code here,
    // Place all significant initialization in InitInstance
}

// The one and only CDspAudioHostApp object

CDspAudioHostApp theApp;

// ensure we get return value 0 from main.
// In MFC, the thing tries to return IDCANCEL and junk like that.
int CDspAudioHostApp::ExitInstance() {
    CWinApp::ExitInstance();
    return 0;
}

BOOL CDspAudioHostApp::another_instance_running(const CString& mut_name) {
    m_mutex = CreateMutex(NULL, FALSE, mut_name);
    if (m_mutex == NULL) {
        return FALSE;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        /* You only need the message box if you wish to notify the user
           that the process is running*/
        MessageBoxW(NULL,
            L"Another instance is already running.\n\nIf you want to run more than one "
            L"instance, please copy the folder containing DSPAudioHost to a different "
            L"location, and try "
            L"running it from there.",
            L"DSPAudioHost: Another instance running", MB_OK | MB_ICONSTOP);

        return TRUE;
    }

    return FALSE;
}

// CDspAudioHostApp initialization

BOOL CDspAudioHostApp::InitInstance() {
    // InitCommonControlsEx() is required on Windows XP if an application
    // manifest specifies use of ComCtl32.dll version 6 or later to enable
    // visual styles.  Otherwise, any window creation will fail.
    INITCOMMONCONTROLSEX InitCtrls;
    InitCtrls.dwSize = sizeof(InitCtrls);
    // Set this to include all the common control classes you want to use
    // in your application.
    InitCtrls.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&InitCtrls);
    // InitDarkMode();

    CWinApp::InitInstance();

    AfxEnableControlContainer();

    // Create the shell manager, in case the dialog contains
    // any shell tree view or shell list view controls.
    // CShellManager *pShellManager = new CShellManager;

    // Activate "Windows Native" visual manager for enabling themes in MFC controls
    CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

    // Standard initialization
    // If you are not using these features and wish to reduce the size
    // of your final executable, you should remove from the following
    // the specific initialization routines you do not need
    // Change the registry key under which our settings are stored
    // TODO: You should modify this string to be something appropriate
    // such as the name of your company or organization

    CString filepath;
    filepath.GetBufferSetLength(512);
    assert(filepath.GetLength() == 512);
    ::GetModuleFileName(NULL, filepath.GetBuffer(), 512);
    filepath.Replace(L"\\", L"_");
    filepath.Replace(L":", L"_");
    filepath.Replace(L".", L"_");

    if (another_instance_running(filepath)) {
        return FALSE;
    }

    SetRegistryKey(filepath);

    CDspAudioHostDlg dlg;
    m_pMainWnd = &dlg;
    INT_PTR nResponse = dlg.DoModal();
    if (nResponse == IDOK) {
        // TODO: Place code here to handle when the dialog is
        //  dismissed with OK
    } else if (nResponse == IDCANCEL) {
        // TODO: Place code here to handle when the dialog is
        //  dismissed with Cancel
    } else if (nResponse == -1) {
        TRACE(traceAppMsg, 0,
            "Warning: dialog creation failed, so application is terminating "
            "unexpectedly.\n");
        TRACE(traceAppMsg, 0,
            "Warning: if you are using MFC controls on the dialog, you cannot #define "
            "_AFX_NO_MFC_CONTROLS_IN_DIALOGS.\n");
    }

    // Delete the shell manager created above.
    // if (pShellManager != nullptr) {
    //   delete pShellManager;
    // }

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
    ControlBarCleanUp();
#endif

    // Since the dialog has been closed, return FALSE so that we exit the
    //  application, rather than start the application's message pump.
    return FALSE;
}
