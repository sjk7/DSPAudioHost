#pragma once
#pragma warning(disable : 26812) // complaints about unscoped enums. I use 'em as flags
                                 // all over the place
// CPBar
#include <vector>
#include <mmsystem.h> // for timeGetTime()
#include <utility> // std::swap()
#include <string>

#define WM_VU_CLICKED WM_USER + 8000

class CPBar : public CWnd {
    DECLARE_DYNAMIC(CPBar)
    enum orientation_t { horizontal, vertical };

    public:
    struct colors {
        colors(COLORREF back, COLORREF fore, int percent)
            : cb(back), cf(fore), m_color_pk(RGB(220, 220, 220)), pos(percent) {}
        COLORREF cb; // backcolor, or "off" color
        COLORREF cf; // forecolor, on "on" color
        COLORREF m_color_pk; // peak indicator color
        float darken_percent = {0};

        int pos; // position: how many percent of the control does it occupy
    };

    typedef std::vector<colors> colorvec_t;

    class props {
        friend class CPBar;
        CPBar* m_pbar{nullptr};

        public:
        props() : m_peak_hold_color(RGB(250, 250, 250)) { colors_default(); }

        void colors_set(const colorvec_t& newcolors) {
            if (m_pbar->m_name.find("BBAGC") != std::string::npos) {
                ASSERT(0);
            }
            m_colors = newcolors;
        }

        colorvec_t& colors_get() { return m_colors; }

        private:
        orientation_t m_orientation{orientation_t::horizontal};
        int m_value{0};
        int m_value_prev{-1};
        int m_force_redraw{1};
        int m_max{100};
        int m_peak{0};
        int m_double_buffered{0};
        int m_invert{0};
        int m_value_is_peak{0};
        COLORREF
        m_color_bkg{0}; // background color for the control (what color the borders are)
        COLORREF m_peak_hold_color;
        colorvec_t m_colors;
        DWORD m_peak_when{0};
        bool m_bsmooth{false};

        void colors_default() {
            m_colors.push_back(colors(RGB(0, 100, 100), RGB(0, 180, 180), 75)); //-V823
            m_colors.push_back(colors(RGB(0, 130, 60), RGB(0, 200, 150), 25)); //-V823
        }
    };

    public:
    CPBar();
    std::string m_name;

    BOOL Create(const std::string& name, HINSTANCE hInstance, DWORD dwExStyle,
        DWORD dwStyle, const RECT& rect,
        CWnd* pParentWindow, // pParentWindow is the plugin window itself
        UINT nID);
    virtual ~CPBar();

    float darken_percent() const {
        float f = m_props.m_colors.at(0).darken_percent;
        return f;
    }
    void darken_back_colors(float percent) {

        static auto cache = m_props.m_colors;
        if ((int)percent == -1000) {
            m_props.m_colors = cache;
            return;
        }
        for (auto& c : this->m_props.m_colors) {

            unsigned char red = GetRValue(c.cb);
            unsigned char green = GetGValue(c.cb);
            unsigned char blue = GetBValue(c.cb);
            float f = (100 - percent) / 100.F;
#pragma warning(disable : 4244)
            red *= f;
            green *= f;
            blue *= f;
            c.cb = RGB(red, green, blue);
            c.darken_percent += percent;

            if (c.cb <= 0) {
                m_props.m_colors = cache;
                c.darken_percent = 0;
                return;
            }
        }
#pragma warning(default : 4244)
    }

    CWnd* m_parent{nullptr};
    void colors_set(const colorvec_t& newcolors) {
        if (m_name.find("BBAGC") != std::string::npos) {
            ASSERT(0);
        }
        m_props.colors_set(newcolors);
        Invalidate(0);
    }

    // draw the peak as if it were the value.
    // You might need this if you are trying to show very transient values
    int value_is_peak() const { return m_props.m_value_is_peak; }

    void value_is_peak_set(int is) {
        m_props.m_value_is_peak = is;
        Invalidate(0);
    }

    // do this if the bar looks ("upside down") if its vertical,
    // or "back to front" if its horizontal
    int draw_inverted() const { return m_props.m_invert; }

    void draw_inverted_set(int invert) {
        m_props.m_invert = invert;
        Invalidate(0);
    }

    int double_buffered() const { return m_props.m_double_buffered; }

    void double_buffered_set(const int buffered) { m_props.m_double_buffered = buffered; }

    void peak_hold_color_set(COLORREF color, bool refresh = false) {
        m_props.m_peak_hold_color = color;
        if (refresh) {
            Invalidate(0);
        }
    }

    COLORREF peak_hold_color() const { return m_props.m_peak_hold_color; }

    void smooth_set(bool smooth) {
        // smooth = false;
        m_props.m_bsmooth = smooth;
        if (GetSafeHwnd() != nullptr) {
            Invalidate(0);
        }
    }

    bool smooth() const { return m_props.m_bsmooth; }

    inline void value_set(const int value, BOOL bRedraw = TRUE) {
        m_props.m_value = value > m_props.m_max ? m_props.m_max : value;
        if (m_benabled == 0) {
            // PRINT("Disabled\n");
        }
        if ((bRedraw != 0) && (GetSafeHwnd() != nullptr)) {
            Invalidate(0);
        }
    }

    DWORD last_time = 0;
    inline void value_set(const double value, BOOL bRedraw = TRUE) {

        if (last_time) {
            //  DWORD between = timeGetTime() - last_time;
            //  PRINT("Time between PBar calls: %s %ld ms\n", this->m_name.c_str(),
            //  (int)between);
        }
        int val = int(value + 0.5F);
        m_props.m_value = val > m_props.m_max ? m_props.m_max : val;
        if (m_benabled == 0) {
            // return; <- no! else it won't *look* disabled,
        }
        if ((bRedraw != 0) && (GetSafeHwnd() != nullptr)) {
            Invalidate(0);
        }
        last_time = timeGetTime();
    }

    void orientation_set(const orientation_t orientation) {
        m_props.m_orientation = orientation;
        Invalidate(0);
    }

    orientation_t orientation() const { return m_props.m_orientation; }

    inline void maximum_set(const int max) { m_props.m_max = max <= 0 ? 100 : max; }

    inline int maximum() const { return m_props.m_max; }

    inline int value() const { return m_props.m_value; }

    inline int enabled() const { return m_benabled; }

    inline void enabled_set(BOOL b) {
        m_benabled = static_cast<BOOL>(b != 0);
        if (b == 0) {
            m_props.m_value = 0;
            m_props.m_peak = 0;
        }
        if (GetSafeHwnd() != nullptr) {
            Invalidate(0);
        }
    }
    props m_props;

    protected:
    DECLARE_MESSAGE_MAP()
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

    void Draw(CDC* pDC, CRect& clientRect) {

        CRect rect(clientRect);

        int BorderWidth = 2;
        CRect RectBorder(clientRect);
        if (m_benabled == 0) {
            CRect merect{};
            this->GetClientRect(&merect);
            m_props.m_value = 0;
            m_props.m_peak = 0;
            pDC->FillSolidRect(&merect, RGB(50, 50, 50));
            return;
        }

        if (m_props.m_value == m_props.m_value_prev && (m_props.m_force_redraw == 0)
            && (m_benabled != 0)) { //-V560
            DrawPeak(pDC, clientRect);
            return;
        }

        if (m_benabled != 0) { //-V547
            pDC->FillSolidRect(&RectBorder, m_props.m_color_bkg);
        } else {
            pDC->FillSolidRect(&RectBorder, ::GetSysColor(COLOR_ACTIVEBORDER));
        }
        rect.top += BorderWidth / 2;
        rect.bottom -= BorderWidth / 2;
        rect.left += BorderWidth / 2;
        rect.right -= BorderWidth / 2;
        CRect myrect(rect);

        if (m_benabled == 0) {
            const COLORREF DISABLED_COLOR = RGB(230, 230, 230);
            pDC->FillSolidRect(rect, DISABLED_COLOR);
            return;
        }

        int w = rect.Width();
        int h = rect.Height();
        auto max = static_cast<float>(m_props.m_max);
        auto myvalue = static_cast<float>(m_props.m_value);

        if (m_props.m_value_is_peak != 0) {
            if (m_props.m_peak > m_props.m_value) {
                myvalue = static_cast<float>(m_props.m_peak);
            }
        }
        float active_ratio = (myvalue / max);
        int active_pos = 0;
        if (m_props.m_orientation == orientation_t::horizontal) {
            active_pos = static_cast<int>((w - BorderWidth) * active_ratio);
        } else {
            active_pos = static_cast<int>((h - BorderWidth) * active_ratio);
            if (m_props.m_invert != 0) {
                HDC hdc = pDC->GetSafeHdc();
                pDC->SetMapMode(
                    MM_ISOTROPIC); // I want the y-axis to go UP from the bottom
                ::SetWindowExtEx(hdc, clientRect.Width(), clientRect.Height(), NULL);
                ::SetViewportExtEx(hdc, clientRect.Width(), -clientRect.Height(), NULL);
                ::SetViewportOrgEx(hdc, 0, clientRect.Height(), NULL);
                // *********************************************************************************************
                // _Thanks_, MS. Another 2 hours of my life I won't get back. I just
                // wanted to change the origin and the y axis direction! So now we know
                // how to do it. I'm leaving this note here in case you need to do it
                // again...
                // *********************************************************************************************
                /*/
                For example, suppose you want a traditional one-quadrant virtual
                coordinate system where (0, 0) is at the lower left corner of the client
                area and the logical width and height ranges from 0 to 32,767. You want
                the x and y units to have the same physical dimensions. Here's what you
                need to do:

                SetMapMode (hdc, MM_ISOTROPIC) ;
                SetWindowExtEx (hdc, 32767, 32767, NULL) ;
                SetViewportExtEx (hdc, cxClient, -cyClient, NULL) ;
                SetViewportOrgEx (hdc, 0, cyClient, NULL) ;

                /*/
            }
        }
        auto color = static_cast<COLORREF>(-1);

        int PeakDecay = 400;

        if (m_props.m_value > m_props.m_peak) {
            m_props.m_peak = m_props.m_value;
            m_props.m_peak_when = timeGetTime();
        }
        if (m_props.m_peak_when != 0U) {
            double secs
                = static_cast<double>(timeGetTime() - m_props.m_peak_when) / 1000.0;
            m_props.m_peak = static_cast<long>(decay(static_cast<double>(m_props.m_peak),
                static_cast<int>(secs), (static_cast<double>(-PeakDecay) / 10000.0)));
            ASSERT(m_props.m_peak <= m_props.m_max);
        }

        if (m_props.m_peak < m_props.m_value) {
            m_props.m_peak = m_props.m_value;
        }
        if (m_props.m_peak < 0) {
            m_props.m_peak = 0;
        }

        if (m_props.m_orientation == orientation_t::horizontal) {
            for (const auto& c : m_props.m_colors) {

                double dw = (static_cast<double>(c.pos) / 100.0) * double(w);
                int this_w = static_cast<int>(dw + 0.5);
                int begin = rect.left;
                int end = begin + this_w;

                rect.right = begin + this_w;

                if (active_pos >= end) {
                    // all active:
                    color = c.cf;
                } else {
                    if (active_pos <= begin) {
                        // all inactive

                        color = c.cb;
                    } else {
                        CRect rectbg(rect);
                        CRect rectfg(rect);
                        rectfg.right = active_pos;
                        rectbg.left = rectfg.right;
                        pDC->FillSolidRect(rectfg, c.cf);
                        rectbg.right = end;
                        pDC->FillSolidRect(rectbg, c.cb);
                        color = static_cast<COLORREF>(-1);
                    }
                }

                if (color != -1) {
                    pDC->FillSolidRect(rect, color);
                }
                rect.left += this_w;
            };
        } else {
            // vertical orientation:
            if (m_props.m_invert != 0) {
                int ffs = 1;
                (void)ffs;
            }

            for (const auto& c : m_props.m_colors) {
                double dh = (static_cast<double>(c.pos) / 100.0) * double(h);
                int this_h = static_cast<int>(dh + 0.5);
                int begin = rect.top;
                int active_end = begin + this_h;
                rect.bottom = begin + this_h;

                if (active_pos >= active_end) {
                    // all active:
                    color = c.cf;
                } else {
                    if (active_pos <= begin) {
                        // all inactive

                        color = c.cb;
                    } else {
                        CRect rectbg(rect);
                        CRect rectfg(rect);
                        rectfg.bottom = active_pos;
                        rectbg.top = rectfg.bottom;
                        pDC->FillSolidRect(rectfg, c.cf);
                        rectbg.bottom = active_end;
                        pDC->FillSolidRect(rectbg, c.cb);
                        color = static_cast<COLORREF>(-1);
                    }
                }

                if (color != -1) {
                    pDC->FillSolidRect(rect, color);
                }
                rect.top += this_h;
            };
        }

        if (!m_props.m_bsmooth) {
            DrawSeps(pDC, myrect);
        }

        DrawPeak(pDC, clientRect);

        m_props.m_value_prev = m_props.m_value;
    }

    __forceinline void DrawSeps(CDC* pDC, const CRect& rect) {
        int h = rect.Height();
        int w = rect.Width();
        int pos = 0;
        int step = 2;
        COLORREF c = m_props.m_color_bkg;
        CRect r(rect);

        if (m_props.m_orientation == orientation_t::horizontal) {
            r.right = r.left + 1;
            while (pos < w) {
                pDC->FillSolidRect(r, c);
                pos += step;
                r.left += step;
                r.right = r.left + 1;
            };
        } else {

            r.bottom = r.top + 1;
            while (pos < h) {

                pDC->FillSolidRect(r, c);
                pos += step;
                r.top += step;
                r.bottom = r.top + 1;
            };
        }
    }

    __forceinline void DrawLine(CDC* pDC, CPen* pen, int x1, int y1, int x2, int y2)
    //=============================================================================
    {
        CPen* pOldPen = 0;
        if (pen != nullptr) {
            pOldPen = pDC->SelectObject(pen);
        }
        pDC->MoveTo(x1, y1);
        pDC->LineTo(x2, y2);
        if (pOldPen != nullptr) {
            pDC->SelectObject(pOldPen);
        }
    }
    BOOL m_benabled{TRUE};

    static inline double decay(double timeZero, int duration, double rate) {
        double growthFactor = rate * duration;
        return timeZero * (exp(growthFactor));
    }

    void DrawPeak(CDC* pDC, const CRect& clientRect) {

        double active_ratio = 0;
        int active_pos = 0;
        auto max = static_cast<double>(m_props.m_max);
        double w = static_cast<double>(clientRect.Width()) - 2;
        double h = static_cast<double>(clientRect.Height()) - 2;

        // if we are totally idle (no peak, no value), then don't draw the peak. Looks
        // crap.
        if (m_props.m_value == m_props.m_value_prev && m_props.m_peak == 0) {
            return;
        }

        if (m_props.m_orientation == orientation_t::horizontal) {
            // Draw the peak value:
            if (m_props.m_peak >= 0) {
                active_ratio = (static_cast<float>(m_props.m_peak) / max);
                active_pos = static_cast<int>(w * active_ratio);
                CRect peakrect(clientRect);
                peakrect.left = static_cast<int>(active_ratio * w);
                peakrect.right = peakrect.left + 2;
                peakrect.top += 1;
                peakrect.bottom -= 1;
                pDC->FillSolidRect(peakrect, m_props.m_peak_hold_color);
            }
        } else {
            // vertical orientation:
            if (m_props.m_peak >= 0) {
                active_ratio = (static_cast<float>(m_props.m_peak) / max);
                active_pos = static_cast<int>(h * active_ratio);
                CRect peakrect(clientRect);
                peakrect.top = static_cast<int>(active_ratio * h);
                peakrect.bottom = peakrect.top + 2;
                // the next 2 lines simply honour the border.
                peakrect.left += 1;
                peakrect.right -= 1;
                // peakrect.top -= 1; peakrect.bottom -= 1;
                pDC->FillSolidRect(peakrect, m_props.m_peak_hold_color);
            }
        }
    }

    public:
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);

    virtual BOOL EnableWindow(BOOL bEnable = TRUE) {
        m_benabled = bEnable;
        if (bEnable == 0) {
            Invalidate(TRUE);
            RedrawWindow();
        }
        // BOOL b = CWnd::EnableWindow(bEnable);

        return m_benabled;
    }
};
