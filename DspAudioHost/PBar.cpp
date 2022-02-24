// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

//

// have to add Maximod path and #include stdafx this way, else Clang can't find stdafx!
#include <pch.h>
#ifdef RADIOCAST_DEFINED
#include "RadioCast.h"
#endif
#include "PBar.h"

// CPBar

IMPLEMENT_DYNAMIC(CPBar, CWnd)

BEGIN_MESSAGE_MAP(CPBar, CWnd)
ON_WM_ERASEBKGND()
ON_WM_PAINT()
ON_WM_LBUTTONDOWN()
END_MESSAGE_MAP()

CPBar::CPBar() = default;

CPBar::~CPBar() = default;
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

// CPBar message handlers
void CPBar::OnPaint() {

    {

        CRect rectClient;
        CPaintDC dc(this); // device context for painting
        CDC dcMem;

        if (m_props.m_double_buffered != 0) {
            if (dcMem.CreateCompatibleDC(&dc) == 0) {
                ASSERT(FALSE);
                return;
            }
        }

        GetClientRect(&rectClient);

        if (m_props.m_double_buffered != 0) {
            CBitmap bmpMem;

            bmpMem.CreateCompatibleBitmap(&dc, rectClient.Width(), rectClient.Height());
            CBitmap* pOldBitmap = dcMem.SelectObject(&bmpMem);

            Draw(&dcMem, rectClient);

            dc.BitBlt(
                0, 0, rectClient.Width(), rectClient.Height(), &dcMem, 0, 0, SRCCOPY);

            if (pOldBitmap != nullptr) {
                dcMem.SelectObject(pOldBitmap);
            }
        } else {
            Draw(&dc, rectClient);
        }
    }
}

BOOL CPBar::OnEraseBkgnd(CDC* pDC) {
    (void)pDC;
    return 1;
}

void CPBar::OnLButtonDown(UINT /*unused*/, CPoint /*unused*/) {
    GetParent()->SendMessage(WM_VU_CLICKED);
}
