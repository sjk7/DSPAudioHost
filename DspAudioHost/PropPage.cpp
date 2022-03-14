// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
// DspAudioHost.cpp : Defines the class behaviors for the application.
//
// PropPage.cpp : implementation file
//

#include "pch.h"
#include "DspAudioHost.h"
#include "afxdialogex.h"
#include "PropPage.h"

// PropPage dialog

IMPLEMENT_DYNAMIC(PropPage, CMFCPropertyPage)

PropPage::PropPage(CWnd* /*=nullptr*/) : CMFCPropertyPage(IDD_MYPROPPAGE, 0) {}

PropPage::~PropPage() {}

void PropPage::DoDataExchange(CDataExchange* pDX) {
    CMFCPropertyPage::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(PropPage, CMFCPropertyPage)
ON_WM_CREATE()
END_MESSAGE_MAP()

// PropPage message handlers

int PropPage::OnCreate(LPCREATESTRUCT lpCreateStruct) {
    if (CMFCPropertyPage::OnCreate(lpCreateStruct) == -1) return -1;
    ModifyStyle(NULL, WS_MINIMIZEBOX);

    return 0;
}
