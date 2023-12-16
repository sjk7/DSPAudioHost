
// DspAudioHostDlg.h : header file
//
// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#pragma once
#define WM_FIRST_SHOWN WM_USER + 100
#include <unordered_map>
#include <map>
#include "PASettings.h"
#include <fstream>
#include "PBar.h"
#include "db_conversions.h"
#include "DSPHost.h"
#include "VST.h"
#include "PropSheet.h"
#include <iostream>
#include <variant>

using variant_plug = std::variant<winamp_dsp::Plugin, vst::Plug>;
using variant_plugs_list = std::vector<variant_plug>;

struct variant_plugs {

    static inline constexpr const char PLUG_SEP = '\t';
    variant_plugs_list m_plugs;
    // variant_plugs() { m_plugs.reserve(6); }

    auto begin() const noexcept { return m_plugs.begin(); }
    auto end() const noexcept { return m_plugs.end(); }
    auto begin() { return m_plugs.begin(); }
    auto end() { return m_plugs.end(); }

    static inline const plugins_base* get_base(const variant_plug& v) {
        const plugins_base* base = nullptr;
        std::visit(
            [&](auto& value) { base = static_cast<const plugins_base*>(&value); }, v);
        return base;
    }

    inline bool remove(const plugins_base* plugBase) {
        size_t cnt = m_plugs.size();
        auto it
            = std::remove_if(m_plugs.begin(), m_plugs.end(), // compare addresses to find
                [&](const auto& p) { return get_base(p) == plugBase; });

        if (it == m_plugs.end()) {
            assert("Plug not found to remove" == nullptr);
            return false;
        }

        m_plugs.erase(it, m_plugs.end());
        assert(m_plugs.size() == cnt - 1);
        return true;
    }

    inline const plugins_base* findActivatedPlugByDesc(
        std::string_view desc) const noexcept {

        for (const auto& plug : m_plugs) {
            auto c = get_base(plug);
            if (c->description() == desc) {
                return c;
            }
        }

        return (plugins_base*)nullptr;
    }

    inline const plugins_base* findActivatedPlug(unsigned int index) const noexcept {
        assert(index <= m_plugs.size());
        auto& p = m_plugs.at(index);
        return get_base(p);
    }

    inline std::string getPlugsAsString() const noexcept {
        std::string ret;
        for (const auto& plug : m_plugs) {
            auto* p = get_base(plug);
            ret += p->description();
            ret += PLUG_SEP;
        }
        return ret;
    }
};

class CSortMFCListCtrl : public CMFCListCtrl

{

    public:
    // DECLARE_MESSAGE_MAP()
    virtual int OnCompareItems(LPARAM lParam1, LPARAM lParam2, int iColumn) {

        if (iColumn == 0) {
            const char* p1 = (const char*)lParam1;
            const char* p2 = (const char*)lParam2;
            return _stricmp(p1, p2);
        }
        return 0;
    }

    public:
};

// CDspAudioHostDlg dialog
class CDspAudioHostDlg : public CDialogEx, public portaudio_cpp::notification_interface {
    // Construction
    public:
    // plugins m_plugs;

    variant_plugs m_active_plugs;
    vst::Plugins m_vsts;
    winamp_dsp::Host m_winamp_host;
    CDspAudioHostDlg(CWnd* pParent = nullptr); // standard constructor
    PASettings m_paSettings;

    PropSheet myPropSheet;
    void paSettingsSave();
    CRect m_sizeRect;
    void saveMyPosition();
    void paSettingsLoad();
    virtual ~CDspAudioHostDlg();
    void myInitDialog();
    void mypopApis();
    void mypopInputDevices();
    void mypopOutputDevices();
    void myChangeAPI(CString piName);
    void mypopAllDevices();
    void mypopDevices(const portaudio_cpp::DeviceTypes forInputOrOutput);
    void myShowCurrentDevice(const portaudio_cpp::DeviceTypes forInputOrOutput);
    int myPreparePortAudio(const portaudio_cpp::ChannelsType& chans,
        const portaudio_cpp::SampleRateType& samplerate);
    void showPaProgress(const char* what);
    std::atomic<int> m_portaudio_create_thread_state{0};
    void afterCreatePortAudio();
    void waitForPortAudio();
    void doEvents();
    void createPortAudio(BOOL wait = FALSE);
    void centreWindowOnMe(
        HWND hwnd, HWND parent, bool force = false, int topOffset = 250);
    void setUpDownButtonsState();
    void myShowActivePlugs();
    const winamp_dsp::plugins_type& findAvailDSPs();
    void showAvailDSPs();
    void setUpButtons();
    void addActivatedPlugToUI(const plugins_base& plug);
    void removeActivatedPlugFromUI(int idx);
    void settingsSavePlugins();
    void settingsGetPlugins(bool apply = false);

    plugins_base* myActivatePlug(
        plugins_base* plug, bool addToUI = true, bool force_show = true);
    HWND FindPluginWindow(std::string_view desc);
    CBrush m_brush;
    CFont m_windows10Font;
    void sliderMoved(CSliderCtrl& which);

    void saveAPI();
    void saveInput();
    void saveOutput();
    BOOL m_loadingPaSettings = FALSE;

    // notification interface:
    void onApiChanged(const PaHostApiInfo& newApi) noexcept override;
    void onInputDeviceChanged(
        const portaudio_cpp::PaDeviceInfoEx& newInputDevice) noexcept override;
    void onOutputDeviceChanged(
        const portaudio_cpp::PaDeviceInfoEx& newOutputDevice) noexcept override;
    void onStreamStarted(const PaStreamInfo& streamInfo) noexcept override;
    void onStreamStopped(const PaStreamInfo& streamInfo) noexcept override;
    void onStreamAbort(const PaStreamInfo& streamInfo) noexcept override;

    std::unique_ptr<portaudio_cpp::PortAudio<CDspAudioHostDlg>> m_portaudio;
    void tracePlugins();
    using font_map_t = std::unordered_map<UINT_PTR, CFont*>;

    using short_buffer = std::vector<short>;
    using float_buffer = std::vector<float>;
    short_buffer m_buf16;
    float_buffer m_buf32;
    float_buffer m_buf32in;
    volatile float m_volume = 1.0f;
    volatile float m_volumeIn = 1.0f;

    inline void convertTo16bit(
        const float* input, int frameCount, int nch = 2, int bps = 32) {
        assert(bps == 32);
        assert(nch == 2);
        m_buf16.resize(frameCount * nch);
        short* const input16 = this->m_buf16.data();
        int i = 0;

        while (i < frameCount * nch) {
            for (int ch = 0; ch < nch; ++ch) {
                short val = (short)((*input) * 32767.0f);
                input16[i] = val;
                ++input;
                ++i;
            }
        };
    }

    inline void mix(const short_buffer& in, float_buffer& out, int frameCount, size_t n,
        int nch = 2) {

        const short* myinput = in.data();
        out.resize(in.size());
        int i = 0;

        while (i < frameCount * nch) {
            for (int ch = 0; ch < nch; ++ch) {
                float val = (float)*myinput / 32767.0f;
                val /= n;
                val *= m_volume;
                out[i] = val;
                ++myinput;
                ++i;
            }
        };
    }

    template <typename T> bool are_same(const T a, const T b) {
        return fabs(a - b) < std::numeric_limits<T>::epsilon();
    }

    std::atomic<int> m_clipped{0};

    void showClipped();

    void showSamplerate();

    template <typename T>
    void apply_volume(
        T* audio, const unsigned int nframes, float volume, const unsigned int nch) {

        m_clipped = 0;
        if (are_same(volume, 1.0f)) {
            return;
        }

        unsigned int myframes = 0;
        while (myframes < nframes) {
            for (unsigned int ch = 0; ch < nch; ++ch) {
                *audio = *audio * volume;
                if (fabs(*audio) > 1.0) {
                    m_clipped++;
                    if (*audio > 1.0) *audio = 1.0;
                    if (*audio < 1.0) *audio = -1.0;
                }
                ++audio;
            }
            ++myframes;
        }
    }

    template <typename T>
    void monoToStereo(const T* input, T* output, const int frameCount) {
        int i = 0;

        while (i++ < frameCount) {
            *output++ = *input;
            *output++ = *input;
            input++;
        }
    }

    // safe to call inplace, provided output buffer is twice input buffer size
    template <typename T>
    void stereoToMono(const T* input, T* output, const int frameCount) {
        int i = 0;

        while (i++ < frameCount) {
            *output = *input;
            *output++ = *input++;
        }
    }

#ifdef _DEBUG
// #define CHECK_MS_ROUNDTRIP 1
#endif

    int streamCallback(const void* input, void* output, unsigned long frameCount,
        const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags,
        const unsigned int byteCount) {

        // return 0; // FIXME

        const auto nPlugs = this->m_active_plugs.m_plugs.size();
        const int nch_in = m_portaudio->m_inputParams.channelCount;
        const int nch_out = m_portaudio->m_outputParams.channelCount;
        const int nch_max = (std::max)(nch_in, nch_out);
        const auto samplerate = m_portaudio->sampleRate().m_value;

        // const auto sampleRate = m_portaudio->sampleRate();
        static constexpr int WINAMP_PLUG_BITDEPTH = 16;
        static constexpr int VST_PLUG_BITDEPTH = 32;
        static constexpr auto WINAMP_PLUG_CHANS = portaudio_cpp::ChannelsType::stereo;
        m_buf32in.resize(frameCount * nch_in); //-V525
        m_buf32.resize(frameCount * nch_max);
        m_buf16.resize(frameCount * nch_max);

        apply_volume((float* const)input, frameCount, (float)m_volumeIn, nch_in);

        if (nch_in == nch_out) {
#ifdef CHECK_MS_ROUNDTRIP

            monoToStereo((float* const)input, m_buf32in.data(), frameCount);
            stereoToMono((float* const)m_buf32.data(), m_buf32.data(), frameCount);
#else
            memcpy(m_buf32in.data(), input, byteCount);
#endif
        } else {
            if (nch_in == 1) {
                monoToStereo((float* const)input, m_buf32in.data(), frameCount);
            } else {
                assert("Unexpected channel config" == 0);
            }
        }

        auto& plugs = this->m_active_plugs;

        // we always have a stereo buffer here:
        convertTo16bit((float* const)m_buf32in.data(), frameCount, 2, 32);

        for (unsigned i = 0; i < plugs.m_plugs.size(); ++i) {
            auto d = plugs.findActivatedPlug(i);
            if (d->getType() == plugins_base::PlugType::Winamp_dsp) {
                d->doDSP((short* const)m_buf16.data(), frameCount, samplerate,
                    WINAMP_PLUG_CHANS, WINAMP_PLUG_BITDEPTH);
                mix(m_buf16, m_buf32, frameCount, nPlugs);
            } else {

                d->doDSP((float* const)m_buf32.data(), frameCount, samplerate,
                    WINAMP_PLUG_CHANS, VST_PLUG_BITDEPTH);
                // mix(m_buf32, m_buf32, frameCount, nPlugs);
                // What to do here, Steve?
            }
        }

        // m_buf32 = m_buf32in; // <--- TEST FOR VST: be sure to remove!

        if (nPlugs) {
            if (nch_in == nch_out) {
                memcpy(output, m_buf32.data(), byteCount);

            } else {
                if (nch_out == 2 && nch_in == 1) {
                    // already coonverted on input
                } else {
                    assert(nch_in == 2 && nch_out == 1);
                    stereoToMono(
                        (float* const)m_buf32.data(), m_buf32.data(), frameCount);
                }
            }
        }

        return nPlugs;
    }

    CFont* myfontStore(UINT_PTR addr, BOOL destroy_all = false) {
        static font_map_t fonts;

        if (destroy_all) {
            for (auto& f : fonts) {
                CFont* ptr = f.second;
                delete ptr;
            }
            fonts.clear();
            return nullptr;
        }

        CFont* retval = nullptr;
        auto it = fonts.find(addr);
        if (it == fonts.end()) {
            auto pr = std::make_pair(addr, new CFont);
            retval = fonts.insert(pr).first->second;
        } else {
            retval = it->second;
        }

        return retval;
    }

    void myCurListShowConfig(int idx);

// Dialog Data
#ifdef AFX_DESIGN_TIME
    enum {IDD = IDD_DSPAUDIOHOST_DIALOG};
#endif

    protected:
    virtual void DoDataExchange(CDataExchange* pDX) override; // DDX/DDV support

    // Implementation
    protected:
    HICON m_hIcon;

    // Generated message map functions
    virtual BOOL OnInitDialog() override;
    void setupMeters();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    DECLARE_MESSAGE_MAP()
    public:
    void showMeters();
    void portaudioStart();

    void pbar_ctrl_create(BOOL double_buffered, const std::string& name, const int idctrl,
        CPBar& pbar, int invert);
    CComboBox cboAPI;
    CComboBox cboInput;
    CComboBox cboOutput;
    afx_msg void OnSelchangeComboApi();
    afx_msg void OnBnClickedRefreshPA();
    CMFCButton m_btnRefresh;
    CStatic lblPluginPath;
    CMFCButton btnPluginPath;
    afx_msg void OnBnClickedBtnPlugpath();
    afx_msg void OnStnClickedStaticPlugpath();
    afx_msg void OnSelchangeComboInput();
    afx_msg void OnCbnSelchangeComboOutput();
    afx_msg void OnBnClickedBtnRefreshPlugs();
    afx_msg LRESULT OnDialogShown(WPARAM w, LPARAM l);
    afx_msg void OnBnClickedBtnRefreshPlugs2();
    CMFCListCtrl listAvail;
    CMFCListCtrl listAvailVST;
    CMFCListCtrl listCur;
    CMFCButton btnUp;
    CMFCButton btnDown;
    CMFCButton btnAdd;
    CMFCButton btnRemove;
    afx_msg void OnBnClickedBtnAdd();
    afx_msg void OnBnClickedBtnRemove();
    afx_msg void OnBnClickedBtnConfigPlug();
    afx_msg void OnDblclkListCur(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnDblclkListAvail(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnBnClickedBtnUp();
    afx_msg void OnBnClickedBtnDown();
    CMFCButton btnConfigPlug;
    CMFCButton btnRefreshPlugs;
    CStatic lblState;
    CStatic lblPa;
    afx_msg void OnClose();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    CMFCButton btnStop;
    CMFCButton btnPlay;
    afx_msg void OnBnClickedBtnPlay();
    afx_msg void OnBnClickedBtnStop();
    CStatic lblElapsedTime;
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    CSliderCtrl sldVol;
    afx_msg void OnNMCustomdrawSliderVol(NMHDR* pNMHDR, LRESULT* pResult);

    afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    CButton chkForceMono;
    afx_msg void OnBnClickedChkForceMono();
    CPBar vuOutL;
    CPBar vuOutR;
    CPBar vuInR;
    CPBar vuInL;
    afx_msg void OnStnClickedVuOutR();
    CSliderCtrl sldVolIn;
    CStatic lblClip;
    afx_msg void OnStnClickedClip();
    CComboBox cboSampleRate;
    afx_msg void OnCbnSelchangeComboSamplerate();
    void afterSamplerateChanged(bool fromUserClick = true);
    afx_msg void OnSize(UINT nType, int cx, int cy);
    std::map<HWND, std::wstring> m_child_windowsA;
    std::map<HWND, std::wstring> m_child_windowsB;
    plugins_base* manageActivatePlug(plugins_base&);
    LRESULT OnThemeChanged();
    virtual BOOL OnWndMsg(
        UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult) override;
    afx_msg void OnBnClickedBtnConfigAudio();
    void savePlugWindowPositions();
    BOOL restorePlugWindowPosition(const plugins_base& plugin);
    bool myPreparePlay(portaudio_cpp::AudioFormat& myfmt);
    virtual void OnCancel();
    virtual void OnOK();

    virtual void PreSubclassWindow();
    CTabCtrl tabAvailPlugs;
    void myTabSelChange(CTabCtrl& tab, int tabIndex);

    virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
};
