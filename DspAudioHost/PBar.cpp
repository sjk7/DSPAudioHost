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

CPBar::CPBar() = default;

#ifdef PBAR_PERF
void trace_perf(CPBar& p, CPBar::perf& perf) {
    std::stringstream ss;
    if (p.double_buffered()) {
        ss << "----- Double-buffered results -----\n\n";
    } else {
        ss << "----- Non-buffered results -----\n\n";
    }

    ss << "Name: " << p.m_name << "\n";
    ss << "nTimes: " << perf.ntimes << "\n";
    ss << "Time in func: " << perf.time_in_func << "\n";

    const auto s = ss.str();
    std::fstream f(p.m_name, std::ios::out | std::ios::binary);
    assert(f);
    f.write(s.data(), s.size());
    assert(f);
    f.close();
}
#endif

CPBar::~CPBar() {
#ifdef PBAR_PERF
    if (!double_buffered())
        trace_perf(*this, m_perf[0]);
    else
        trace_perf(*this, m_perf[1]);
#endif
}
std::string m_name;

//=============================================================================
BOOL CPBar::Create(const std::string& name, HINSTANCE hInstance, DWORD dwExStyle,
    DWORD dwStyle, const RECT& rect, CWnd* pParentWindow, UINT nID)

//=============================================================================
{
    m_name = name;
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

    return TRUE;
}

void prepare_invert(CPaintDC& dc, const CRect& rectClient) {
    HDC hdc = dc.GetSafeHdc();
    BOOL set = dc.SetMapMode(MM_ISOTROPIC); // I want the y-axis to go UP from the bottom
    ASSERT(set);
    set = ::SetWindowExtEx(hdc, rectClient.Width(), rectClient.Height(), NULL);
    ASSERT(set);
    set = ::SetViewportExtEx(hdc, rectClient.Width(), -rectClient.Height(), NULL);
    ASSERT(set);
    set = ::SetViewportOrgEx(hdc, 0, rectClient.Height(), NULL);
    ASSERT(set);
}

// CPBar message handlers
void CPBar::OnPaint() {

    {

        CRect rectClient;
        CPaintDC dc(this);
        CBitmap* poldBitmap;
        static auto constexpr MAX_UPDATE_MS = 25;
#ifdef PBAR_PERF
        if (!m_props.m_double_buffered) {
            if (m_perf[0].last_time == 0) m_perf[0].last_time = timeGetTime();
            m_perf[0].entered = timeGetTime();
        } else {
            if (m_perf[1].last_time == 0) m_perf[1].last_time = timeGetTime();
            m_perf[1].entered = timeGetTime();
        }
#endif

        GetClientRect(&rectClient);
        if (m_props.m_invert) {
            prepare_invert(dc, rectClient);
        }

        if (m_props.m_double_buffered != 0) {
            if (!memDC.m_hDC) memDC.CreateCompatibleDC(&dc);

            if (!memBitmap.m_hObject) {
                memBitmap.CreateCompatibleBitmap(
                    &dc, rectClient.Width(), rectClient.Height());
            }

            poldBitmap = (CBitmap*)memDC.SelectObject(&memBitmap);
            const auto since = timeGetTime() - when_last_drawn;
            if (since >= MAX_UPDATE_MS) {
                Draw(&memDC, rectClient);
                when_last_drawn = timeGetTime();
            }

            dc.BitBlt(
                0, 0, rectClient.Width(), rectClient.Height(), &memDC, 0, 0, SRCCOPY);

            if (poldBitmap != nullptr) {
                memDC.SelectObject(poldBitmap);
            }

        } else {

            Draw(&dc, rectClient);
        }
#ifdef PBAR_PERF
        if (!m_props.m_double_buffered) {
            m_perf[0].left = timeGetTime();
            m_perf[0].time_in_func += m_perf[0].left - m_perf[0].entered;
            m_perf[0].ntimes++;
        } else {

            m_perf[1].left = timeGetTime();
            m_perf[1].time_in_func += m_perf[1].left - m_perf[1].entered;
            m_perf[1].ntimes++;
        }
#endif
    }
}

BOOL CPBar::OnEraseBkgnd(CDC* pDC) {
    (void)pDC;
    return 1;
}

void CPBar::OnLButtonDown(UINT /*unused*/, CPoint /*unused*/) {
    GetParent()->SendMessage(WM_VU_CLICKED);
}
