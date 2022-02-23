// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
// DspAudioHost.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h" // main symbols

// CDspAudioHostApp:
// See DspAudioHost.cpp for the implementation of this class
//

class CDspAudioHostApp : public CWinAppEx {
    public:
    CDspAudioHostApp();
    virtual ~CDspAudioHostApp() {
        if (m_mutex) {
            ::CloseHandle(m_mutex);
            m_mutex = nullptr;
        }
    }

    // Overrides
    public:
    int ExitInstance() override;
    BOOL InitInstance() override;
    BOOL another_instance_running(const CString& mut_name);
    HANDLE m_mutex = {0};
    // Implementation

    DECLARE_MESSAGE_MAP()
};

extern CDspAudioHostApp theApp;
