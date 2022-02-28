#pragma once

#include "PropPage.h"
// PropSheet

class PropSheet : public CMFCPropertySheet {
    DECLARE_DYNAMIC(PropSheet)

    public:
    PropSheet();
    virtual ~PropSheet();
    PropPage mypage[2];
    CRect save_rc; // used in OnSize
    CRect minimum_rc; // used in OnGetMinMaxInfo

    BOOL SetPageTitle(int nPage, const CStringW& title) {
        CTabCtrl* pTab = GetTabControl();
        ASSERT(pTab);

        TC_ITEM ti{};
        ti.mask = TCIF_TEXT;
        ti.pszText = (LPWSTR)(LPWSTR)title.GetString();

        VERIFY(pTab->SetItem(nPage, &ti));

        return TRUE;
    }

    int OnCreate(LPCREATESTRUCT lpCreateStruct) {
        if (CMFCPropertySheet::OnCreate(lpCreateStruct) == -1) return -1;
        ModifyStyle(NULL, WS_MINIMIZEBOX);

        return 0;
    }

    void hideButtons() {
        CWnd* pButton = GetDlgItem(ID_APPLY_NOW);
        ASSERT(pButton);
        pButton->ShowWindow(SW_HIDE);

        pButton = GetDlgItem(IDCANCEL);
        pButton->ShowWindow(SW_HIDE);
        pButton = GetDlgItem(IDOK);
        pButton->ShowWindow(SW_HIDE);

        pButton = GetDlgItem(IDHELP);
        pButton->ShowWindow(SW_HIDE);
    }

    BOOL OnInitDialog() {
        // override for modeless:
        m_bModeless = FALSE;

        m_nFlags |= WF_CONTINUEMODAL;
        BOOL bResult = CPropertySheet::OnInitDialog();
        hideButtons();

        m_bModeless = TRUE;
        m_nFlags &= ~WF_CONTINUEMODAL;
        AddPage(&mypage[1]);
        // save rectangles for resizing
        GetClientRect(&save_rc); // save the old rect for resizing
        GetClientRect(&minimum_rc); // save the original rect for OnGetMinMaxInfo
        applyFont();
        return bResult;
    }
    void OnSize(UINT nType, int cx, int cy) {
        CPropertySheet::OnSize(nType, cx, cy);

        if (nType == SIZE_MINIMIZED) return;
        auto t = GetTabControl();
        if (!GetActivePage()) return;
        if (!t) return;

        int dx = cx - save_rc.Width();
        int dy = cy - save_rc.Height();

        // count how many childs are in window
        int count = 0;
        for (CWnd* child = GetWindow(GW_CHILD); child;
             child = child->GetWindow(GW_HWNDNEXT))
            count++;

        HDWP hDWP = ::BeginDeferWindowPos(count);

        for (CWnd* child = GetWindow(GW_CHILD); child;
             child = child->GetWindow(GW_HWNDNEXT)) {
            CRect r;
            child->GetWindowRect(&r);
            ScreenToClient(&r);
            if (child->SendMessage(WM_GETDLGCODE) & DLGC_BUTTON) {
                r.left += dx;
                r.top += dy;
                ::DeferWindowPos(hDWP, child->m_hWnd, 0, r.left, r.top, 0, 0,
                    SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            } else {
                r.right += dx;
                r.bottom += dy;
                ::DeferWindowPos(hDWP, child->m_hWnd, 0, 0, 0, r.Width(), r.Height(),
                    SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
        ::EndDeferWindowPos(hDWP);
        GetClientRect(&save_rc);
    }

    void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI) {
        lpMMI->ptMinTrackSize.x = minimum_rc.Width();
        lpMMI->ptMinTrackSize.y = minimum_rc.Height();
        CPropertySheet::OnGetMinMaxInfo(lpMMI);
    }

    BOOL Create(CWnd* wnd, CFont* font) {
        m_pfont = font;
        return CPropertySheet::Create(wnd);
    }

    CFont* m_pfont{nullptr};
    void applyFont() {
        if (m_pfont && m_hWnd) {
            SetFont(m_pfont);
            SendMessageToDescendants(WM_SETFONT, (WPARAM)m_pfont->GetSafeHandle(), 0);
            for (CWnd* child = GetWindow(GW_CHILD); child;
                 child = child->GetWindow(GW_HWNDNEXT)) {
                child->SetFont(m_pfont);
            }
        }
    }
    void setFontObject(CFont* pFont) {
        if (this->m_hWnd) {
            applyFont();
        }
        m_pfont = pFont;
    }

    protected:
    DECLARE_MESSAGE_MAP()
};
