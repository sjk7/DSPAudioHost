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
