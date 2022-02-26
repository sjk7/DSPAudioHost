// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//

#include "pch.h"
#ifdef RADIOCAST_DEFINED
#include "RadioCast.h"
#endif
#include "PBar.h"
#include <fstream>

// CPBar

IMPLEMENT_DYNAMIC(CPBar, CWnd)

BEGIN_MESSAGE_MAP(CPBar, CWnd)
ON_WM_ERASEBKGND()
ON_WM_PAINT()
ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

CPBar::CPBar() : memDC(*this) {}

CPBar::~CPBar() {}
std::string m_name;

//=============================================================================
BOOL CPBar::Create(const std::string& name, HINSTANCE hInstance, DWORD dwExStyle,
    DWORD dwStyle, const RECT& rect, CWnd* pParentWindow, UINT nID)

//=============================================================================
{
    m_name = name;
#ifdef PBAR_PERF
    m_perf.m_name = name;

#endif

    m_props.m_pbar = this;
    m_parent = pParentWindow;
    if (pParentWindow == (CWnd*)nullptr) {
        ASSERT(0);
        return FALSE;
    }
    m_benabled = TRUE;
    const TCHAR* pszClassName = _T("AudioPBar");

    WNDCLASS wc = {
        CS_DBLCLKS, // class style - want WM_LBUTTONDBLCLK messages
        AfxWndProc, // window proc
        0, // class extra bytes
        0, // window extra bytes
        hInstance, // instance handle
        0, // icon
        ::LoadCursor(0, IDC_ARROW), // cursor
        0, // background brush
        0, // menu name
        pszClassName // class name
    };

    if (::RegisterClass(&wc) == 0U) {
        DWORD dwLastError = GetLastError();
        if (dwLastError != ERROR_CLASS_ALREADY_EXISTS) {
            // TRACE(_T("ERROR - RegisterClass failed, GetLastError() returned %u\n"),
            // dwLastError);
            ASSERT(FALSE);
            return FALSE;
        }
    }

    BOOL rc = CWnd::CreateEx(
        dwExStyle, pszClassName, _T(""), dwStyle, rect, pParentWindow, nID, 0);

    if (rc == 0) {
#ifdef _DEBUG
        DWORD dwLastError = GetLastError();
        UNUSED_ALWAYS(dwLastError);
        (void)dwLastError;
        // TRACE(_T("ERROR - Create failed, GetLastError() returned %u\n"), dwLastError);
        ASSERT(FALSE);
#endif
    }

    CRect r;
    GetClientRect(r);
    memDC.resize(r, true);

    return TRUE;
}

void prepare_invert(HDC& dc, const CRect& rectClient) {

    BOOL set
        = ::SetMapMode(dc, MM_ISOTROPIC); // I want the y-axis to go UP from the bottom
    ASSERT(set);
    set = ::SetWindowExtEx(dc, rectClient.Width(), rectClient.Height(), NULL);
    ASSERT(set);
    set = ::SetViewportExtEx(dc, rectClient.Width(), -rectClient.Height(), NULL);
    ASSERT(set);
    set = ::SetViewportOrgEx(dc, 0, rectClient.Height(), NULL);
    ASSERT(set);
}

// CPBar message handlers
void CPBar::OnPaint() {

    CRect rectClient;
    CPaintDC dc(this);

    static auto constexpr MAX_UPDATE_MS = 25;
#ifdef PBAR_PERF
    perf_timer t(m_perf);
#endif

    GetClientRect(&rectClient);

    const auto since = timeGetTime() - when_last_drawn;

    if (m_props.m_double_buffered != 0) {
        if (since >= MAX_UPDATE_MS) {
            if (m_props.m_invert) {
                prepare_invert(dc.m_hDC, rectClient);
            }
            memDC.drawDoubleBuffered(rectClient, dc);
        }
    } else {
        if (m_props.m_invert) {
            prepare_invert(dc.m_hDC, rectClient);
        }
        if (since >= MAX_UPDATE_MS) {
            Draw(&dc, rectClient);
        }
    }

    when_last_drawn = timeGetTime();
}

BOOL CPBar::OnEraseBkgnd(CDC* pDC) {
    (void)pDC;
    return 1;
}

void CPBar::OnLButtonDown(UINT /*unused*/, CPoint /*unused*/) {
    GetParent()->SendMessage(WM_VU_CLICKED);
}
