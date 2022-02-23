#include "pch.h"
#pragma once

class CMFCVisualManagerWindows10 : public CMFCVisualManagerWindows7 {
    class CMFCTabCtrlEx : public CMFCTabCtrl {
        friend class CMFCVisualManagerWindows10;
    }; // 继承类以暴露保护成员

    DECLARE_DYNCREATE(CMFCVisualManagerWindows10)

    public:
    CMFCVisualManagerWindows10() { TRACE("Windows 10 Visual Manager engaged\n"); }
    virtual void OnUpdateSystemColors(); // 更新主题颜色

    // 跳过 Windows 7
    virtual void OnFillBarBackground(CDC* pDC, CBasePane* pBar, CRect rectClient,
        CRect rectClip, BOOL bNCArea = FALSE); // 填充 RibbonBar 的背景
    virtual COLORREF OnFillRibbonButton(
        CDC* pDC, CMFCRibbonButton* pButton); // 填充 RibbonButton 的背景
    virtual void OnDrawRibbonButtonBorder(
        CDC* pDC, CMFCRibbonButton* pButton); // RibbonButton 的边框
    virtual void OnHighlightMenuItem(CDC* pDC, CMFCToolBarMenuButton* pButton, CRect rect,
        COLORREF& clrText); // 填充高亮菜单项的背景
    virtual void OnFillHighlightedArea(
        CDC* pDC, CRect rect, CBrush* pBrush, CMFCToolBarButton* pButton); // 填充高亮区域

    // Tab control:
    virtual void OnDrawTab(CDC* pDC, CRect rectTab, int iTab, BOOL bIsActive,
        const CMFCBaseTabCtrl* pTabWnd); // 绘出文档 TAB
    virtual BOOL OnEraseTabsFrame(CDC* pDC, CRect rect,
        const CMFCBaseTabCtrl* pTabWnd); // 文档框架的边缘区域(原来不区分文档颜色)
    virtual void OnEraseTabsArea(CDC* pDC, CRect rect,
        const CMFCBaseTabCtrl* pTabWnd); // 文档 TAB 空白区域及下方的线
    virtual void OnDrawTabCloseButton(CDC* pDC, CRect rect,
        const CMFCBaseTabCtrl* pTabWnd, BOOL bIsHighlighted, BOOL bIsPressed,
        BOOL bIsDisabled); // 绘出关闭按钮

    // Ribbon control:
    virtual COLORREF OnDrawRibbonPanel(CDC* pDC, CMFCRibbonPanel* pPanel, CRect rectPanel,
        CRect rectCaption); // 重载：去除各区域矩形，以分隔条代替
    virtual void OnDrawRibbonPanelCaption(CDC* pDC, CMFCRibbonPanel* pPanel,
        CRect rectCaption); // 重载：取消各区域标题文本的背景
    virtual void OnDrawRibbonCategory(CDC* pDC, CMFCRibbonCategory* pCategory,
        CRect rectCategory); // 重载：填充功能区类别的背景色、绘出 TAB 下方的线
    virtual COLORREF OnDrawRibbonCategoryTab(CDC* pDC, CMFCRibbonTab* pTab,
        BOOL bIsActive); // 重载：绘出功能区当前活动 TAB 或高亮 TAB 的背景和线条
    virtual void OnDrawRibbonApplicationButton(
        CDC* pDC, CMFCRibbonButton* pButton); // 重载：绘出程序按钮

    // MDI Client area
    virtual BOOL OnEraseMDIClientArea(CDC* pDC, CRect rectClient);

    // Window 10 风格是求简，以下去除开始菜单的边框和其它效果 ...
    virtual void OnDrawRibbonMainPanelFrame(
        CDC* pDC, CMFCRibbonMainPanel* pPanel, CRect rect){}; // 不绘出功能区边框
    virtual void OnFillRibbonMenuFrame(
        CDC* pDC, CMFCRibbonMainPanel* pPanel, CRect rect){}; // 不绘出菜单边框
    virtual void OnDrawRibbonGalleryBorder(
        CDC* pDC, CMFCRibbonGallery* pButton, CRect rectBorder){}; // 不绘出功能区右下方的阴影
    virtual COLORREF OnDrawRibbonTabsFrame(
        CDC* pDC, CMFCRibbonBar* pWndRibbonBar, CRect rectTab) {
        return (COLORREF)-1;
    }; // 不绘出快速访问工具栏下方的横线

    protected:
    static LRESULT CALLBACK ProcPaintWin10TabCtrl(HWND hWnd, UINT message, WPARAM wParam,
        LPARAM lParam); // 回调函数：处理 CMFCTabCtrl 的 WM_PAINT 消息
    static LRESULT CALLBACK ProcPaintWin10AppButton(HWND hWnd, UINT message,
        WPARAM wParam, LPARAM lParam); // 回调函数：处理 CMFCRibbonBar 的 WM_PAINT 消息
};
