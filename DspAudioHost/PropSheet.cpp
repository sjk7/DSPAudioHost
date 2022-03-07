// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
// DspAudioHost.cpp : Defines the class behaviors for the application.
//
// PropSheet.cpp : implementation file
//

#include "pch.h"
#include "DspAudioHost.h"
#include "PropSheet.h"

// PropSheet

IMPLEMENT_DYNAMIC(PropSheet, CMFCPropertySheet)

PropSheet::PropSheet() {
    AddPage(&mypage[0]);
}

PropSheet::~PropSheet() {}

BEGIN_MESSAGE_MAP(PropSheet, CMFCPropertySheet)
ON_WM_CREATE()
ON_WM_SIZE()
ON_WM_GETMINMAXINFO()
END_MESSAGE_MAP()

// PropSheet message handlers
