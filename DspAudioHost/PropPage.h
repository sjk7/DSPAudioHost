#pragma once
#include "afxdialogex.h"

// PropPage dialog

class PropPage : public CMFCPropertyPage {
    DECLARE_DYNAMIC(PropPage)

    public:
    PropPage(CWnd* pParent = nullptr); // standard constructor
    virtual ~PropPage();

// Dialog Data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_MYPROPPAGE };
#endif

    protected:
    virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV support

    DECLARE_MESSAGE_MAP()
    public:
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
};
