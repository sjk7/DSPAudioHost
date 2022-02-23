
#include "pch.h"
#include "VisualManagerWindows10.h"

IMPLEMENT_DYNCREATE(CMFCVisualManagerWindows10, CMFCVisualManagerWindows7)

// ??????
void CMFCVisualManagerWindows10::OnUpdateSystemColors() {
    CMFCVisualManagerWindows7::OnUpdateSystemColors();

    if (GetGlobalData()->m_nBitsPerPixel > 8 && !GetGlobalData()->IsHighContrastMode()) {
        // ??? Windows 10 ?????????????????????
        // GetThemeColor/GetThemeSysColor ????
        m_clrHighlight = RGB(232, 239, 247);
        m_clrHighlightDn = RGB(205, 228, 252);
        m_clrHighlightChecked = RGB(201, 224, 247);
        m_clrMenuItemBorder = RGB(164, 206, 249);
        m_clrPressedButtonBorder = RGB(164, 206, 249);

        // ?????????????????
        m_brHighlight.DeleteObject();
        m_brHighlightDn.DeleteObject();
        m_brHighlightChecked.DeleteObject();

        m_brHighlight.CreateSolidBrush(m_clrHighlight);
        m_brHighlightDn.CreateSolidBrush(m_clrHighlightDn);
        m_brHighlightChecked.CreateSolidBrush(m_clrHighlightChecked);

        m_penMenuItemBorder.DeleteObject();
        m_penMenuItemBorder.CreatePen(PS_SOLID, 1, m_clrMenuItemBorder);
    }
}

// ?? RibbonBar ???
void CMFCVisualManagerWindows10::OnFillBarBackground(
    CDC* pDC, CBasePane* pBar, CRect rectClient, CRect rectClip, BOOL bNCArea) {
    // CMFCVisualManagerWindows7::OnFillBarBackground(pDC, pBar, rectClient, rectClip,
    // bNCArea);
    CMFCVisualManagerWindows::OnFillBarBackground(
        pDC, pBar, rectClient, rectClip, bNCArea);

    // ??????????????
    if (pBar->IsKindOf(RUNTIME_CLASS(CMFCRibbonBar))) {
        // ????????????? CMFCRibbonBar ? WM_PAINT ???? ApplicationButton
        if (::GetWindowLongPtr(pBar->GetSafeHwnd(), GWLP_WNDPROC)
            != (LONG_PTR)ProcPaintWin10AppButton)
            ::SetWindowLongPtr(pBar->GetSafeHwnd(), GWLP_WNDPROC,
                (LONG_PTR)ProcPaintWin10AppButton); // ??????????

        // ????????????? CMFCTabCtrl ? WM_PAINT ??????
        // CMFCTabCtrl::OnDraw() ??
        CMDIClientAreaWnd* pClientArea = (CMDIClientAreaWnd*)CWnd::FromHandle(
            ((CMDIFrameWnd*)::AfxGetMainWnd())->m_hWndMDIClient);

        if (pClientArea != NULL) {
            const CObList& lstTabbedGroups = pClientArea->GetMDITabGroups();

            for (POSITION pos = lstTabbedGroups.GetHeadPosition(); pos != 0;) {
                CMFCTabCtrl* pNextWnd
                    = DYNAMIC_DOWNCAST(CMFCTabCtrl, lstTabbedGroups.GetNext(pos));
                if (::GetWindowLongPtr(pNextWnd->GetSafeHwnd(), GWLP_WNDPROC)
                    != (LONG_PTR)ProcPaintWin10TabCtrl)
                    ::SetWindowLongPtr(pNextWnd->GetSafeHwnd(), GWLP_WNDPROC,
                        (LONG_PTR)ProcPaintWin10TabCtrl); // ??????????
            }
        }
    }
}

// ?? RibbonButton ???
COLORREF CMFCVisualManagerWindows10::OnFillRibbonButton(
    CDC* pDC, CMFCRibbonButton* pButton) {
    // return CMFCVisualManagerWindows7::OnFillRibbonButton(pDC, pButton);
    return CMFCVisualManagerWindows::OnFillRibbonButton(pDC, pButton);
}

// RibbonButton ???
void CMFCVisualManagerWindows10::OnDrawRibbonButtonBorder(
    CDC* pDC, CMFCRibbonButton* pButton) {
    // CMFCVisualManagerWindows7::OnDrawRibbonButtonBorder(pDC, pButton);
    CMFCVisualManagerWindows::OnDrawRibbonButtonBorder(pDC, pButton);
}

// ??????????
void CMFCVisualManagerWindows10::OnHighlightMenuItem(
    CDC* pDC, CMFCToolBarMenuButton* pButton, CRect rect, COLORREF& clrText) {
    // CMFCVisualManagerWindows7::OnHighlightMenuItem(pDC, pButton, rect, clrText);
    CMFCVisualManagerWindows::OnHighlightMenuItem(pDC, pButton, rect, clrText);
}

// ??????
void CMFCVisualManagerWindows10::OnFillHighlightedArea(
    CDC* pDC, CRect rect, CBrush* pBrush, CMFCToolBarButton* pButton) {
    // return CMFCVisualManagerWindows7::OnFillHighlightedArea(pDC, rect, pBrush,
    // pButton);
    return CMFCVisualManagerWindows::OnFillHighlightedArea(pDC, rect, pBrush, pButton);
}

// ???????????????
COLORREF CMFCVisualManagerWindows10::OnDrawRibbonPanel(
    CDC* pDC, CMFCRibbonPanel* pPanel, CRect rectPanel, CRect rectCaption) {
    CPen pen(PS_SOLID, 1, ::afxGlobalData.clrBarShadow);
    CPen* pOldPen = pDC->SelectObject(&pen);
    pDC->MoveTo(rectPanel.right, rectPanel.top);
    pDC->LineTo(rectPanel.right, rectPanel.bottom + 1);
    pDC->SelectObject(pOldPen);
    return ::afxGlobalData.clrBarText;
}

// ???????? Pane ???????????????
void CMFCVisualManagerWindows10::OnDrawRibbonPanelCaption(
    CDC* pDC, CMFCRibbonPanel* pPanel, CRect rectCaption) {
    COLORREF clrTextOld
        = pDC->SetTextColor(::afxGlobalData.clrCaptionText + RGB(64, 64, 64));
    pDC->DrawText(pPanel->GetName(), rectCaption,
        DT_SINGLELINE | DT_CENTER | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
    pDC->SetTextColor(clrTextOld);
}

// ?????????????? TAB ????
void CMFCVisualManagerWindows10::OnDrawRibbonCategory(
    CDC* pDC, CMFCRibbonCategory* pCategory, CRect rectCategory) {
    // ???????????
    pDC->FillSolidRect(rectCategory, ::afxGlobalData.clrBarFace + RGB(5, 6, 7));

    // ?? TAB ????
    CRect rectActiveTab = pCategory->GetTabRect();
    CPen pen(PS_SOLID, 1, ::afxGlobalData.clrBarShadow);
    CPen* pOldPen = pDC->SelectObject(&pen);

    pDC->MoveTo(rectCategory.left, rectCategory.top);
    pDC->LineTo(rectActiveTab.left, rectCategory.top);
    pDC->MoveTo(rectActiveTab.right - 2, rectCategory.top);
    pDC->LineTo(rectCategory.right, rectCategory.top);
    pDC->MoveTo(rectCategory.left, rectCategory.bottom - 1);
    pDC->LineTo(rectCategory.right, rectCategory.bottom - 1);

    pDC->SelectObject(pOldPen);
}

// ????????? TAB ??? TAB ??????
COLORREF CMFCVisualManagerWindows10::OnDrawRibbonCategoryTab(
    CDC* pDC, CMFCRibbonTab* pTab, BOOL bIsActive) {
    CMFCRibbonBar* pBar = pTab->GetParentCategory()->GetParentRibbonBar();

    bIsActive = bIsActive
        && ((pBar->GetHideFlags() & AFX_RIBBONBAR_HIDE_ELEMENTS) == 0
            || pTab->GetDroppedDown() != NULL);

    // ???????? TAB ????????
    if (bIsActive || pTab->IsHighlighted()) {
        // ?? TAB ???
        CRect rectTab = pTab->GetRect();

        rectTab.top += 3;
        rectTab.right -= 2;

        // ?? TAB ???
        if (bIsActive)
            pDC->FillSolidRect(rectTab, ::afxGlobalData.clrBarFace + RGB(5, 6, 7));

        // ?? TAB ??
        CPen pen(PS_SOLID, 1, ::afxGlobalData.clrBarShadow);
        CPen* pOldPen = pDC->SelectObject(&pen);
        pDC->MoveTo(rectTab.left, rectTab.bottom);
        pDC->LineTo(rectTab.left, rectTab.top);
        pDC->LineTo(rectTab.right, rectTab.top);
        pDC->LineTo(rectTab.right, rectTab.bottom);
        pDC->SelectObject(pOldPen);
    }

    return ::afxGlobalData.clrBarText;
}

///< summary>???????????? CMFCRibbonBar ? WM_PAINT ??</summary>
///< param name="hWnd">???????????</param>
///< param name="message">????</param>
///< param name="wParam">...</param>
///< param name="lParam">...</param>
LRESULT CALLBACK CMFCVisualManagerWindows10::ProcPaintWin10AppButton(
    HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    LRESULT lResult = (::AfxGetAfxWndProc())(hWnd, message, wParam, lParam);

    // ???? Windows 10 ????????? OnDraw() ??
    if (message != WM_PAINT
        || !CMFCVisualManager::GetInstance()->IsKindOf(
            RUNTIME_CLASS(CMFCVisualManagerWindows10)))
        return lResult;

    // ? Windows 10 ???????? Ribbonbar ???????????????? Windows 7
    // ???????
    CMFCRibbonBar* pRibbonBar = (CMFCRibbonBar*)CWnd::FromHandle(hWnd);
    CMFCRibbonApplicationButton* pMainButton = pRibbonBar->GetApplicationButton();

    if (pMainButton != NULL) {
        CDC* pDC = pRibbonBar->GetDC();
        CMFCVisualManager::GetInstance()->OnDrawRibbonApplicationButton(pDC, pMainButton);
        pRibbonBar->ReleaseDC(pDC);
    }

    return lResult;
}

///< summary>??????? CMFCTabCtrl ? WM_PAINT ??</summary>
///< param name="hWnd">???????????</param>
///< param name="message">????</param>
///< param name="wParam">...</param>
///< param name="lParam">...</param>
LRESULT CALLBACK CMFCVisualManagerWindows10::ProcPaintWin10TabCtrl(
    HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    // ???? Windows 10 ???????? OnDraw() ??
    if (message != WM_PAINT
        || !CMFCVisualManager::GetInstance()->IsKindOf(
            RUNTIME_CLASS(CMFCVisualManagerWindows10)))
        return (::AfxGetAfxWndProc())(hWnd, message, wParam, lParam);

    // ??????? CMFCTabCtrl::OnDraw()???? Windows 10 ????
    // ...

    CMFCTabCtrlEx* pTabWnd = (CMFCTabCtrlEx*)CWnd::FromHandle(hWnd);

    CPaintDC dc(pTabWnd);

    // ?????????
    CRect rectClient;
    pTabWnd->GetClientRect(&rectClient);

    // TAB ??
    CRect rectTabs;
    pTabWnd->GetTabsRect(rectTabs);
    rectTabs.left = 1;
    rectTabs.right = rectClient.right;

    CMFCVisualManager::GetInstance()->OnEraseTabsFrame(&dc, rectClient,
        pTabWnd); // ?????????(?????????)???????????
    CMFCVisualManager::GetInstance()->OnEraseTabsArea(
        &dc, rectTabs, pTabWnd); // ?? TAB ???????????

    // ?? TAB
    CFont* pOldFont = dc.SelectObject(&::afxGlobalData.fontRegular);
    int nOldBkMd = dc.SetBkMode(TRANSPARENT);
    COLORREF cOldText = dc.SetTextColor(::afxGlobalData.clrCaptionText);

    for (int nIndex = pTabWnd->GetTabsNum() - 1; nIndex >= 0; nIndex--) {
        int nIndexAc = nIndex;
        if (pTabWnd->m_arTabIndices.GetSize() == pTabWnd->GetTabsNum())
            nIndexAc = pTabWnd->m_arTabIndices[nIndex];

        pTabWnd->m_iCurTab = nIndexAc;
        dc.SelectObject(nIndexAc == pTabWnd->GetActiveTab()
                ? ::afxGlobalData.fontBold
                : ::afxGlobalData.fontRegular);
        pTabWnd->DrawFlatTab(&dc, (CMFCTabInfo*)pTabWnd->m_arTabs[nIndexAc],
            nIndexAc == pTabWnd->GetActiveTab());
    }

    dc.SetTextColor(cOldText);
    dc.SelectObject(pOldFont);
    dc.SetBkMode(nOldBkMd);

    return TRUE;
}

// ???? TAB ????? Ribbon TAB?
void CMFCVisualManagerWindows10::OnDrawTab(
    CDC* pDC, CRect rectTab, int iTab, BOOL bIsActive, const CMFCBaseTabCtrl* pTabWnd) {
    // return;

    // ?? TAB ??
    rectTab.left -= 1;
    rectTab.top -= 2;

    // ?? TAB ?????? TAB ???????
    auto br = CBrush(pTabWnd->GetTabBkColor(iTab));
    OnFillTab(pDC, rectTab, &br, iTab, bIsActive, pTabWnd);

    // ?? TAB ??
    CPen pen(PS_SOLID, 1, ::afxGlobalData.clrBarShadow);
    CPen* pOldPen = pDC->SelectObject(&pen);
    pDC->MoveTo(rectTab.right, rectTab.top);
    pDC->LineTo(rectTab.right, rectTab.bottom - 1);

    if (iTab != 0) {
        pDC->MoveTo(rectTab.left, rectTab.top);
        pDC->LineTo(rectTab.left, rectTab.bottom - 1);
    }

    if (!bIsActive) {
        pDC->MoveTo(rectTab.left, rectTab.bottom - 1);
        pDC->LineTo(rectTab.right, rectTab.bottom - 1);
    }

    pDC->SelectObject(pOldPen);

    // ?? TAB ??
    COLORREF clrTextOld = pDC->SetTextColor(pTabWnd->GetTabTextColor(iTab));
    OnDrawTabContent(pDC, rectTab, iTab, bIsActive, pTabWnd, (COLORREF)-1);
    pDC->SetTextColor(clrTextOld);
}

// ???? TAB ?????
void CMFCVisualManagerWindows10::OnDrawTabCloseButton(CDC* pDC, CRect rect,
    const CMFCBaseTabCtrl* pTabWnd, BOOL bIsHighlighted, BOOL bIsPressed,
    BOOL bIsDisabled) {
    rect.top -= 2;

    CFont* pOldFont = pDC->SelectObject(&::afxGlobalData.fontBold);

    COLORREF cOldText
        = pDC->SetTextColor(bIsHighlighted ? RGB(255, 0, 0) : RGB(100, 100, 100));
    pDC->DrawText(L"?", rect,
        DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
    pDC->SetTextColor(cOldText);
    pDC->SelectObject(pOldFont);
}

// ?????????(?????????)???????????
BOOL CMFCVisualManagerWindows10::OnEraseTabsFrame(
    CDC* pDC, CRect rect, const CMFCBaseTabCtrl* pTabWnd) {
    COLORREF clrActiveTab = pTabWnd->GetTabBkColor(pTabWnd->GetActiveTab());
    pDC->FillSolidRect(rect, clrActiveTab);

    // ?????????
    CPen pen(PS_SOLID, 1, ::afxGlobalData.clrBarShadow);
    CPen* pOldPen = pDC->SelectObject(&pen);

    pDC->MoveTo(rect.left, rect.bottom - 1);
    pDC->LineTo(rect.right - 1, rect.bottom - 1);
    pDC->LineTo(rect.right - 1, rect.top);
    pDC->MoveTo(rect.left, rect.top);
    pDC->LineTo(rect.left, rect.bottom - 1);

    pDC->SelectObject(pOldPen);

    return TRUE;
}

// ?? TAB ?????????
void CMFCVisualManagerWindows10::OnEraseTabsArea(
    CDC* pDC, CRect rect, const CMFCBaseTabCtrl* pTabWnd) {
    // ?? TAB ????
    pDC->FillSolidRect(rect, ::afxGlobalData.clrBarFace + RGB(5, 6, 7));

    // ?? TAB ??????????
    CPen pen(PS_SOLID, 1, ::afxGlobalData.clrBarShadow);
    CPen* pOldPen = pDC->SelectObject(&pen);
    pDC->MoveTo(rect.left, rect.bottom - 1);
    pDC->LineTo(rect.right, rect.bottom - 1);
    pDC->SelectObject(pOldPen);
}

// ??????
void CMFCVisualManagerWindows10::OnDrawRibbonApplicationButton(
    CDC* pDC, CMFCRibbonButton* pButton) {
    const BOOL bIsHighlighted = pButton->IsHighlighted() || pButton->IsFocused();
    const BOOL bIsPressed = pButton->IsPressed() || pButton->IsDroppedDown();

    CRect rect = pButton->GetRect();
    rect.top += 3;

    if (bIsPressed || !bIsHighlighted)
        pDC->FillSolidRect(rect, ::afxGlobalData.clrHilite);
    else
        pDC->FillSolidRect(rect, ::afxGlobalData.clrHilite + RGB(0, 16, 32));

    CFont* pOldFont = pDC->SelectObject(&::afxGlobalData.fontRegular);
    COLORREF cOldText = pDC->SetTextColor(::afxGlobalData.clrTextHilite);
    pDC->DrawText(pButton->GetParentRibbonBar()->GetMainCategory()->GetName(), rect,
        DT_SINGLELINE | DT_VCENTER | DT_CENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
    pDC->SetTextColor(cOldText);
    pDC->SelectObject(pOldFont);
}

// ?? MDI ??
BOOL CMFCVisualManagerWindows10::OnEraseMDIClientArea(CDC* pDC, CRect rectClient) {
    pDC->FillSolidRect(rectClient, RGB(205, 215, 230)); // ?? MDI ????
    return TRUE;
}
