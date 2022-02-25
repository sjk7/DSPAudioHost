// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
// DspAudioHostDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "DspAudioHost.h"
#include "DspAudioHostDlg.h"
#include "afxdialogex.h"
#include <vector>
#include <thread>
#include <limits>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
// #include "../win32-darkmode/win32-darkmode/ListViewUtil.h"

HWND hwnd_found_plug_config = nullptr;

static inline BOOL CALLBACK find_plug_handle_proc(_In_ HWND hwnd, _In_ LPARAM lParam) {

    std::wstring looking_for((wchar_t*)lParam);
    std::wstring this_one(MAX_PATH, L'\0');
    assert(this_one.size() == MAX_PATH);
    ::GetWindowText(hwnd, this_one.data(), MAX_PATH);
    this_one.resize(this_one.find(L'\0'));
    if (this_one == looking_for) {
        hwnd_found_plug_config = hwnd;
        return FALSE;
    }
    return TRUE;
}

HWND find_plug_window(const std::wstring& txt) {
    hwnd_found_plug_config = nullptr;
    EnumThreadWindows(GetCurrentThreadId(), &find_plug_handle_proc, (LPARAM)txt.data());
    return hwnd_found_plug_config;
}

HWND hwnd_found = 0;

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx {
    public:
    CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ABOUTBOX };
#endif

    protected:
    virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV support

    // Implementation
    protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX) {}

void CAboutDlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
}
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#endif
BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)

END_MESSAGE_MAP()

#ifdef __clang__
#pragma clang diagnostic pop
#endif

// CDspAudioHostDlg dialog

CDspAudioHostDlg* g_dlg = nullptr;

extern void myPaInitCallback(char* info) {
    g_dlg->showPaProgress(info);
}
PaInitCallback g_PaInitCallback = &myPaInitCallback;

CDspAudioHostDlg::CDspAudioHostDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_DSPAUDIOHOST_DIALOG, pParent) {

    BOOL b = ::SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
    if (b == FALSE) {
        // will succeed if not run as admin, but you get less than realtime.
    }
    createPortAudio();
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    g_dlg = this;
}

void CDspAudioHostDlg::paSettingsSave() {
    saveAPI();
    saveInput();
    saveOutput();
}

void CDspAudioHostDlg::paSettingsLoad() {
    assert(m_portaudio);
    assert(IsWindow(GetSafeHwnd()));
    assert(m_portaudio->hasNotification(this));
    m_loadingPaSettings = TRUE;

    std::string s = GetCommandLineA();
    if (s.find("safemode") == std::string::npos) {
        m_paSettings.getAll();
    } else {
    }

    CString what;
    try {

        what = "Restoring API: ";
        what += m_paSettings.m_api;

        if (!m_paSettings.api().empty()) {
            auto api = m_portaudio->findApi(m_paSettings.api());
            assert(api);
            // throw std::runtime_error("ffs");
            m_portaudio->changeApi(api);
        }

        what = "Restoring input: "; //-V519
        what += m_paSettings.m_input;
        if (!m_paSettings.input().empty()) {
            auto input = m_portaudio->findDevice(m_paSettings.input());
            if (!input) {
                std::string e("Could not find input device, with name:\n");
                e += m_paSettings.input().data();
                throw std::runtime_error(e);
            }
            m_portaudio->changeDevice(input->index, portaudio_cpp::DeviceTypes::input);
        }

        what = "Restoring API: ";
        what += m_paSettings.m_output;
        if (!m_paSettings.output().empty()) {
            auto output = m_portaudio->findDevice(m_paSettings.output());
            if (!output) {
                std::string e("Could not find output device, with name:\n");
                e += m_paSettings.output().data();
                throw std::runtime_error(e);
            }
            m_portaudio->changeDevice(output->index, portaudio_cpp::DeviceTypes::output);
        }

        what = "Restoring sample rate: ";
        what += std::to_string(m_paSettings.samplerate).data();
        showSamplerate();

    } catch (const std::exception& e) {
        CString msg = L"Portaudio settings cannot be applied, for: ";
        msg += L"\n";
        msg += what;
        msg += "\n\n";
        msg += e.what();
        msg += L"\n\n\nClick Ignore to revert to using defaults?\n";
        msg += L"NOTE: you will LOSE your settings if you do this. So if you need, for "
               L"example, to plug in a device, do it now and hit retry.";
        auto reply = MessageBox(
            msg, L"Error restoring saved devices", MB_ABORTRETRYIGNORE | MB_ICONQUESTION);
        if (reply == IDRETRY) {
            m_portaudio_create_thread_state = 0;
            createPortAudio(TRUE);
            return;
        } else if (reply == IDABORT) {
            exit(-1);
        } else {
            // do nothing, ignore
            m_paSettings = std::move(PASettings(false));
            m_paSettings.saveAll();
            m_portaudio->failsafeDefaults();
            showSamplerate();
            myShowCurrentDevice(portaudio_cpp::DeviceTypes::input);
            myShowCurrentDevice(portaudio_cpp::DeviceTypes::output);
        }
    }

    m_loadingPaSettings = FALSE;
}

CDspAudioHostDlg::~CDspAudioHostDlg() {

    this->myfontStore(0, TRUE);
    if (m_portaudio && m_portaudio->m_running) {
        m_portaudio->Stop();
    }
}

void CDspAudioHostDlg::DoDataExchange(CDataExchange* pDX) {
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_COMBO_API, cboAPI);
    DDX_Control(pDX, IDC_COMBO_INPUT, cboInput);
    DDX_Control(pDX, IDC_COMBO_OUTPUT, cboOutput);
    DDX_Control(pDX, IDC_BTNREFRESH, m_btnRefresh);
    DDX_Control(pDX, IDC_STATIC_PLUGPATH, lblPluginPath);
    DDX_Control(pDX, IDC_BTN_PLUGPATH, btnPluginPath);
    DDX_Control(pDX, IDC_LIST_AVAIL, listAvail);
    DDX_Control(pDX, IDC_LIST_CUR, listCur);
    DDX_Control(pDX, IDC_BTN_UP, btnUp);
    DDX_Control(pDX, IDC_BTN_DOWN, btnDown);
    DDX_Control(pDX, IDC_BTN_ADD, btnAdd);
    DDX_Control(pDX, IDC_BTN_REMOVE, btnRemove);
    DDX_Control(pDX, IDC_BTN_CONFIG_PLUG, btnConfigPlug);
    DDX_Control(pDX, IDC_BTN_REFRESH_PLUGS, btnRefreshPlugs);
    DDX_Control(pDX, IDC_STATIC_INFO, lblState);
    DDX_Control(pDX, IDC_STATIC_PAINFO, lblPa);
    DDX_Control(pDX, IDC_BTN_STOP, btnStop);
    DDX_Control(pDX, IDC_BTN_PLAY, btnPlay);
    DDX_Control(pDX, IDC_STATIC_ELAPSED, lblElapsedTime);
    DDX_Control(pDX, IDC_SLIDER_VOL, sldVol);
    DDX_Control(pDX, IDC_CHK_FORCE_MONO, chkForceMono);
    // DDX_Control(pDX, IDC_VUOUTL, vuOutL);
    // DDX_Control(pDX, IDC_VU_OUT_R, vuOutR);
    DDX_Control(pDX, IDC_SLIDER_VOL_IN, sldVolIn);
    DDX_Control(pDX, IDC_CLIP, lblClip);
    DDX_Control(pDX, IDC_COMBO_SAMPLERATE, cboSampleRate);
}
enum class special_chars : wchar_t {
    leftArrow = 223,
    rightArrow = 224,
    upArrow = 225,
    downArrow = 226

};

void btnFontChar(CDspAudioHostDlg& dlg, CMFCButton& btn, special_chars c,
    int fontSize = 100, CString fontName = L"Wingdings") {

    const UINT_PTR addr = (UINT_PTR)&btn;
    CFont* font = dlg.myfontStore(addr);
    if (font) {
        LOGFONT logFont;
        memset(&logFont, 0, sizeof(LOGFONT));
        logFont.lfCharSet = DEFAULT_CHARSET;
        logFont.lfHeight = fontSize;
        logFont.lfWeight = FW_BOLD;
        Checked::tcsncpy_s(
            logFont.lfFaceName, _countof(logFont.lfFaceName), fontName, _TRUNCATE);

        font->CreatePointFontIndirect(&logFont);

        wchar_t text[] = {static_cast<wchar_t>(c), 0x0000};

        btn.SetFont(font);

        btn.SetWindowTextW(text);
        btn.SetRedraw(TRUE);
        btn.RedrawWindow();
    } else {
        ASSERT(0);
    }
}

template <typename T>
void SetFontSize(T& btn, CDspAudioHostDlg& dlg, int fontSize,
    const wchar_t* fontName = L"Segoe UI", const bool bold = true) {
    const UINT_PTR addr = (UINT_PTR)&btn;
    CFont* font = dlg.myfontStore(addr);

    if (font) {
        LOGFONT logFont;
        memset(&logFont, 0, sizeof(LOGFONT));
        btn.GetFont()->GetLogFont(&logFont);
        LOGFONT lf = logFont;
        lf.lfHeight = fontSize;
        if (bold) lf.lfWeight = FW_BOLD;
        _tcscpy(lf.lfFaceName, fontName);

        font->CreatePointFontIndirect(&lf);
        btn.SetFont(font);
        btn.SetRedraw(TRUE);
        btn.RedrawWindow();
    } else {
        ASSERT(0);
    }
}

void CDspAudioHostDlg::doEvents() {
    MSG Msg;
    while (PeekMessage(&Msg, NULL, 0, 0, PM_REMOVE)) {
        if (!PreTranslateMessage(&Msg)) {
            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
    }
}

void CDspAudioHostDlg::waitForPortAudio() {

    if (m_portaudio_create_thread_state == 0) {
        createPortAudio();
        assert(m_portaudio_create_thread_state);
    }
    unsigned int waited = 0;
    btnStop.EnableWindow(FALSE);
    btnPlay.EnableWindow(FALSE);

    while (m_portaudio_create_thread_state == 2 || m_portaudio_create_thread_state == 1) {
        Sleep(10);
        doEvents();
        waited += 10;
        if (waited % 100 == 0 && IsWindow(*this)) CCmdTarget::BeginWaitCursor();
        if (waited % 250 == 0) {
            std::string msg("Waiting for PortAudio, waited: ");
            msg += std::to_string(waited);
            msg += " ms ...";
            ::SetWindowTextA(lblState.GetSafeHwnd(), msg.data());
        }
        if (waited % 500 == 0 && IsWindow(*this)) {
            if (m_btnRefresh.IsWindowEnabled()) {
                this->m_btnRefresh.EnableWindow(FALSE);
                this->cboAPI.EnableWindow(FALSE);
                this->cboInput.EnableWindow(FALSE);
                this->cboOutput.EnableWindow(FALSE);
            }
        }
    }
    m_portaudio_create_thread_state = 0;

    std::string msg("PortAudio enabled in: ");
    msg += std::to_string(waited);
    msg += " ms.  PortAudio ready.";

    if (IsWindow(*this)) {
        ::SetWindowTextA(lblState, msg.data());
        SetWindowTextA(lblPa.GetSafeHwnd(), "");
    }
    afterCreatePortAudio();

    if (IsWindow(*this)) {

        this->m_btnRefresh.EnableWindow(TRUE);
        this->cboAPI.EnableWindow(TRUE);
        this->cboInput.EnableWindow(TRUE);
        this->cboOutput.EnableWindow(TRUE);

        ::SetWindowTextA(lblState, msg.data());
        SetWindowTextA(lblPa.GetSafeHwnd(), "");
        btnStop.EnableWindow(TRUE);
        btnPlay.EnableWindow(TRUE);
        CCmdTarget::EndWaitCursor();
    } //-V1020
}

void CDspAudioHostDlg::createPortAudio(BOOL wait) {

    if (m_portaudio_create_thread_state) {
        throw std::runtime_error("Portaudio thread in bad state for creating right now");
    }

    m_portaudio_create_thread_state = 1;

    if (IsWindow(*this)) {

        this->m_btnRefresh.EnableWindow(FALSE);
        this->cboAPI.EnableWindow(FALSE);
        this->cboInput.EnableWindow(FALSE);
        this->cboOutput.EnableWindow(FALSE);
    }

    std::thread t([&]() {
        m_portaudio_create_thread_state = 2;
        TRACE("Thread\n");
        TRACE("Thread agn\n");
        m_portaudio = nullptr;
        m_portaudio = std::make_unique<portaudio_cpp::PortAudio<CDspAudioHostDlg>>(*this);
        m_portaudio_create_thread_state = 3;
    });
    t.detach();

    if (wait) {
        this->waitForPortAudio();
    }
}

void CDspAudioHostDlg::afterCreatePortAudio() {

    while (!IsWindow(GetSafeHwnd())) {
        Sleep(1);
        doEvents();
    }
    mypopApis();
    using namespace portaudio_cpp;
    mypopDevices(DeviceTypes::input);
    mypopDevices(DeviceTypes::output);
    myShowCurrentDevice(DeviceTypes::input);
    myShowCurrentDevice(DeviceTypes::output);

    // note: notifications not enabled until AFTER initial setup
    m_portaudio->notification_add(this);

    paSettingsLoad();
}

void CDspAudioHostDlg::myInitDialog() {
    CCmdTarget::BeginWaitCursor();
    EnableWindow(FALSE);

    HRESULT themed = ::SetWindowTheme(listAvail.GetSafeHwnd(), L"explorer", nullptr);
    assert(themed == S_OK);

    themed = ::SetWindowTheme(listCur.GetSafeHwnd(), L"explorer", nullptr);
    assert(themed == S_OK);
    findAvailDSPs();
    showAvailDSPs();

    std::string s = GetCommandLineA();
    if (s.find("safemode") == std::string::npos) {
        settingsGetPlugins(true);
    } else {
    }

    this->waitForPortAudio();

    {
        const auto pos = theApp.GetProfileIntW(L"WinampHost", L"OutVol", 0);
        sldVol.SetPos(pos);
        sliderMoved(sldVol);
    }

    {
        const auto pos = theApp.GetProfileIntW(L"WinampHost", L"InputVolume", 2500);
        sldVolIn.SetPos(pos);
        sliderMoved(sldVolIn);
    }

    if (chkForceMono.GetCheck() == TRUE) {
        m_portaudio->monoInputSet(true);
    } else {
        m_portaudio->monoInputSet(false);
    }
    EnableWindow(TRUE);

    if (m_portaudio->errCode() == 0) {
        OnBnClickedBtnPlay();
    }
    SetTimer(1, 25, nullptr);
    CCmdTarget::EndWaitCursor();
}

void CDspAudioHostDlg::mypopApis() {
    this->cboAPI.Clear();
    cboAPI.ResetContent();

    for (const auto& api : m_portaudio->apis()) {
        CString name(api.name);
        cboAPI.AddString(name);
    }

    CString default_api_name(m_portaudio->m_currentApi.name);
    auto api = m_portaudio->findApi("Windows DirectSound");
    if (api) {
        // DirectSound as default, please
        CString tmp(api->name);
        default_api_name = tmp;
        m_portaudio->changeApi(api);
    }
    int idx = cboAPI.FindStringExact(0, default_api_name);
    assert(idx >= 0 && idx < cboAPI.GetCount());
    cboAPI.SetCurSel(idx);
}

void myFillDevices(const portaudio_cpp::DeviceTypes forInOrOut,
    const portaudio_cpp::PortAudio<CDspAudioHostDlg>::device_list& devices,
    CComboBox* pcbo) {

    pcbo->Clear();
    pcbo->ResetContent();
    for (const auto& d : devices) {
        if (forInOrOut == portaudio_cpp::DeviceTypes::output) {
            if (d.info.maxOutputChannels > 0) {
                const CString name(d.extendedName.data());
                pcbo->AddString(name);
            }
        } else {
            if (d.info.maxInputChannels > 0) {
                const CString name(d.extendedName.data());
                pcbo->AddString(name);
            }
        }
    }
}

void CDspAudioHostDlg::myShowCurrentDevice(const portaudio_cpp::DeviceTypes forInOrOut) {
    const auto& cur = m_portaudio->currentDevice(forInOrOut);
    const CString inName(cur.extendedName.data());

    /*/
    const auto& types = cur.get_supported_audio_types();
    auto s = cur.to_string(types);
    CString ws(s.c_str());
    const auto ffs = ws.GetLength();
    TRACE(L"ffs\n%s\n", ws.GetBuffer());
    /*/
    auto* pcbo = &this->cboInput;
    if (forInOrOut == portaudio_cpp::DeviceTypes::output) {
        pcbo = &this->cboOutput;
    } else {
        pcbo = &this->cboInput;
    }
    auto idx = pcbo->FindStringExact(0, inName);
#ifdef DEBUG
    const auto how_many = pcbo->GetCount();
    assert(idx >= 0 && idx < how_many);
#endif
    pcbo->SetCurSel(idx);
}
int CDspAudioHostDlg::myPreparePortAudio(const portaudio_cpp::ChannelsType& chans,
    const portaudio_cpp::SampleRateType& samplerate) {
    try {
        return m_portaudio->preparePlay(samplerate, chans, true);
    } catch (const std::exception& e) {
        std::string s("Failed to open audio devices:\n\n");
        s += e.what();
        MessageBoxA(
            this->GetSafeHwnd(), s.data(), "Fatal PortAudio Error", MB_OK | MB_ICONSTOP);
        throw;
    }
}
void CDspAudioHostDlg::showPaProgress(const char* what) {
    if (IsWindow(*this)) SetWindowTextA(lblPa.GetSafeHwnd(), what);
}
const winamp_dsp::plugins_type& CDspAudioHostDlg::findAvailDSPs() {

    winamp_dsp::Host& host = m_winamp_host;
    CString root;
    lblPluginPath.GetWindowTextW(root);
    CStringA myroot(root);
    host.setParent(this->GetSafeHwnd());
    host.setFolder(myroot.GetBuffer());
    tracePlugins();
    return host.plugins();
}
void clear_cols(CListCtrl& list) {
    if (list.GetHeaderCtrl()) {

        int nColumnCount = list.GetHeaderCtrl()->GetItemCount();

        // Delete all of the columns.
        for (int i = 0; i < nColumnCount; i++) {
            list.DeleteColumn(0);
        }
    }
}
void CDspAudioHostDlg::showAvailDSPs() {
    const auto& dsps = m_winamp_host.plugins();

    listAvail.DeleteAllItems();
    clear_cols(listAvail);
    clear_cols(listCur);

    listAvail.InsertColumn(0, L"Plugins available");
    listCur.InsertColumn(0, L"Active plugins");
    assert(listAvail.GetHeaderCtrl());

    for (const auto& d : dsps) {
        if (!d.description().empty()) {
            CString wDesc(d.description().data());
            int cnt = listAvail.InsertItem(listAvail.GetItemCount(), wDesc);
            auto ptr = (DWORD_PTR)d.description().data();
            listAvail.SetItemData(cnt, ptr);
        }
    }

    listAvail.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
    listCur.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
    if (listAvail.GetItemCount() > 0) {
        listAvail.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
    }

    listAvail.Sort(0);
    listAvail.SetRedraw();
    listAvail.UpdateWindow();
    listCur.SetRedraw();
    listCur.UpdateWindow();
    BOOL en = dsps.empty() == false;

    btnAdd.EnableWindow(en);
    btnRemove.EnableWindow(en);
}

void CDspAudioHostDlg::setUpButtons() {

    btnFontChar(*this, btnAdd, special_chars::rightArrow);
    btnFontChar(*this, btnRemove, special_chars::leftArrow);
    btnFontChar(*this, btnUp, special_chars::upArrow);
    btnFontChar(*this, btnDown, special_chars::downArrow);
    SetFontSize(lblElapsedTime, *this, 150);

    auto icon = (HICON)::LoadImage(
        theApp.m_hInstance, MAKEINTRESOURCE(IDI_ICON_SETTINGS), IMAGE_ICON, 16, 16, 0);
    btnConfigPlug.SetIcon(icon);

    icon = (HICON)::LoadImage(
        theApp.m_hInstance, MAKEINTRESOURCE(IDI_ICON_REFRESH), IMAGE_ICON, 16, 16, 0);
    btnRefreshPlugs.SetIcon(icon);

    icon = (HICON)::LoadImage(theApp.m_hInstance, MAKEINTRESOURCE(IDI_ICON_RELOAD_AUDIO),
        IMAGE_ICON, 16, 16, 0);
    m_btnRefresh.SetIcon(icon);

    icon = (HICON)::LoadImage(
        theApp.m_hInstance, MAKEINTRESOURCE(IDI_FLAT_STOP), IMAGE_ICON, 24, 24, 0);

    this->btnStop.SetIcon(icon);

    icon = (HICON)::LoadImage(
        theApp.m_hInstance, MAKEINTRESOURCE(IDI_FLAT_PLAY), IMAGE_ICON, 24, 24, 0);

    this->btnPlay.SetIcon(icon);

    btnPlay.EnableWindow(FALSE);
    btnStop.EnableWindow(FALSE);
}

void CDspAudioHostDlg::mypopDevices(const portaudio_cpp::DeviceTypes forInOrOut) {

    // const auto& device = m_portaudio->m_currentDevices[forInOrOut];
    auto* pcbo = &this->cboInput;
    if (forInOrOut == portaudio_cpp::DeviceTypes::output) {
        pcbo = &this->cboOutput;
    } else {
        pcbo = &this->cboInput;
    }
    myFillDevices(forInOrOut, m_portaudio->devices(), pcbo);
}

void CDspAudioHostDlg::onApiChanged(const PaHostApiInfo& newApi) noexcept {
    CString apiName(newApi.name);
    int i = cboAPI.FindStringExact(0, apiName);
    assert(i >= 0 && i < cboAPI.GetCount());
    cboAPI.SetCurSel(i);
    this->mypopAllDevices();
    if (!m_loadingPaSettings) {
        saveAPI();
    }
}

using namespace portaudio_cpp;
void CDspAudioHostDlg::onInputDeviceChanged(
    const PaDeviceInfoEx& newInputDevice) noexcept {
    (void)newInputDevice;
    myShowCurrentDevice(portaudio_cpp::DeviceTypes::input);
    if (!m_loadingPaSettings) saveInput();
}

void CDspAudioHostDlg::onOutputDeviceChanged(
    const PaDeviceInfoEx& newOutputDevice) noexcept {
    (void)newOutputDevice;
    myShowCurrentDevice(portaudio_cpp::DeviceTypes::output);
    if (!m_loadingPaSettings) saveOutput();
}

void CDspAudioHostDlg::onStreamStarted(const PaStreamInfo&) noexcept {}

void CDspAudioHostDlg::onStreamStopped(const PaStreamInfo&) noexcept {}

void CDspAudioHostDlg::onStreamAbort(const PaStreamInfo&) noexcept {}

void CDspAudioHostDlg::mypopInputDevices() {
    mypopDevices(portaudio_cpp::DeviceTypes::input);
}

void CDspAudioHostDlg::mypopOutputDevices() {
    mypopDevices(portaudio_cpp::DeviceTypes::output);
}

void CDspAudioHostDlg::mypopAllDevices() {
    mypopDevices(portaudio_cpp::DeviceTypes::input);
    mypopDevices(portaudio_cpp::DeviceTypes::output);
}

void CDspAudioHostDlg::myChangeAPI(CString apiName) {

    if (apiName.IsEmpty()) {
        cboAPI.GetWindowTextW(apiName);
    }
    CStringA apiNameN(apiName);
    std::string sapi(apiNameN.GetBuffer());
    auto ptr = m_portaudio->findApi(sapi);
    if (ptr) {
        m_portaudio->changeApi(ptr);
        if (!m_portaudio->subscribedForNotifications(this)) {
            mypopAllDevices();
            myShowCurrentDevice(portaudio_cpp::DeviceTypes::input);
            myShowCurrentDevice(portaudio_cpp::DeviceTypes::output);
        } // otherwise UI is populated from notifications
    } else {
        CString msg = L"Unable to find api, with name: ";
        msg += apiName;
        MessageBox(msg, L"Fatal Error", MB_OK);
    }
}

#pragma warning(disable : 26454)
BEGIN_MESSAGE_MAP(CDspAudioHostDlg, CDialogEx)
ON_WM_SYSCOMMAND()
ON_WM_PAINT()
ON_WM_QUERYDRAGICON()
ON_CBN_SELCHANGE(IDC_COMBO_API, &CDspAudioHostDlg::OnSelchangeComboApi)
ON_BN_CLICKED(IDC_BTNREFRESH, &CDspAudioHostDlg::OnBnClickedRefreshPA)
ON_BN_CLICKED(IDC_BTN_PLUGPATH, &CDspAudioHostDlg::OnBnClickedBtnPlugpath)
ON_STN_CLICKED(IDC_STATIC_PLUGPATH, &CDspAudioHostDlg::OnStnClickedStaticPlugpath)
ON_CBN_SELCHANGE(IDC_COMBO_INPUT, &CDspAudioHostDlg::OnSelchangeComboInput)
ON_CBN_SELCHANGE(IDC_COMBO_OUTPUT, &CDspAudioHostDlg::OnCbnSelchangeComboOutput)
ON_BN_CLICKED(IDC_BTN_REFRESH_PLUGS, &CDspAudioHostDlg::OnBnClickedBtnRefreshPlugs)
ON_MESSAGE(WM_FIRST_SHOWN, &CDspAudioHostDlg::OnDialogShown)
ON_BN_CLICKED(IDC_BTN_TEST, &CDspAudioHostDlg::OnBnClickedBtnRefreshPlugs2)
ON_BN_CLICKED(IDC_BTN_ADD, &CDspAudioHostDlg::OnBnClickedBtnAdd)
ON_BN_CLICKED(IDC_BTN_REMOVE, &CDspAudioHostDlg::OnBnClickedBtnRemove)
ON_BN_CLICKED(IDC_BTN_CONFIG_PLUG, &CDspAudioHostDlg::OnBnClickedBtnConfigPlug)
ON_NOTIFY(NM_DBLCLK, IDC_LIST_CUR, &CDspAudioHostDlg::OnDblclkListCur)
ON_NOTIFY(NM_DBLCLK, IDC_LIST_AVAIL, &CDspAudioHostDlg::OnDblclkListAvail)
ON_BN_CLICKED(IDC_BTN_UP, &CDspAudioHostDlg::OnBnClickedBtnUp)
ON_BN_CLICKED(IDC_BTN_DOWN, &CDspAudioHostDlg::OnBnClickedBtnDown)
ON_WM_CLOSE()
ON_WM_TIMER()
ON_BN_CLICKED(IDC_BTN_PLAY, &CDspAudioHostDlg::OnBnClickedBtnPlay)
ON_BN_CLICKED(IDC_BTN_STOP, &CDspAudioHostDlg::OnBnClickedBtnStop)
ON_WM_CTLCOLOR()
ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER_VOL, &CDspAudioHostDlg::OnNMCustomdrawSliderVol)
ON_WM_VSCROLL()
ON_BN_CLICKED(IDC_CHK_FORCE_MONO, &CDspAudioHostDlg::OnBnClickedChkForceMono)
ON_STN_CLICKED(IDC_VU_OUT_R, &CDspAudioHostDlg::OnStnClickedVuOutR)
ON_STN_CLICKED(IDC_CLIP, &CDspAudioHostDlg::OnStnClickedClip)
ON_CBN_SELCHANGE(IDC_COMBO_SAMPLERATE, &CDspAudioHostDlg::OnCbnSelchangeComboSamplerate)
ON_WM_SIZE()
ON_WM_THEMECHANGED()
ON_BN_CLICKED(IDC_BTN_CONFIG_AUDIO, &CDspAudioHostDlg::OnBnClickedBtnConfigAudio)
END_MESSAGE_MAP()
#pragma warning(default : 26454)

RECT my_get_window_position(HWND hwnd) {
    RECT rect{0};
    WINDOWPLACEMENT wp{0};
    wp.length = sizeof(WINDOWPLACEMENT);
    ASSERT(IsWindow(hwnd));
    ::GetWindowPlacement(hwnd, &wp);

    rect = wp.rcNormalPosition;
    return rect;
}

static bool window_is_offscreen(HWND hwnd) {

    // int nScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    // int nScreenHeight = GetSystemMetrics(SM_CYSCREEN);
    CRect windows_work_area;
    RECT rect = my_get_window_position(hwnd);
    SystemParametersInfo(SPI_GETWORKAREA, 0, &windows_work_area, 0);

    if (rect.bottom > windows_work_area.bottom + 50
        || rect.right > windows_work_area.right + 50
        || rect.left < windows_work_area.left - 50
        || rect.top < windows_work_area.top - 50) {

        return true;
    }

    return false;
}

// CDspAudioHostDlg message handlers

BOOL CDspAudioHostDlg::OnInitDialog() {
    CDialogEx::OnInitDialog();

    CStringA tit = "DSPAudioHost (RC) Built at: ";
    tit += __TIME__;
    tit += " on: ";
    tit += __DATE__;

    CStringW wtit(tit);
    SetWindowText(wtit);

    NONCLIENTMETRICS metrics{0};
    metrics.cbSize = sizeof(NONCLIENTMETRICS);
    ::SystemParametersInfo(
        SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &metrics, 0);
    m_windows10Font.CreateFontIndirect(&metrics.lfMessageFont);
    SendMessageToDescendants(WM_SETFONT, (WPARAM)m_windows10Font.GetSafeHandle(), 0);

    sldVol.SetRange(0, 6000, TRUE);
    sldVol.SetTicFreq(50);

    sldVolIn.SetRange(0, 6000, TRUE);
    sldVolIn.SetTicFreq(50);

    // Add "About..." menu item to system menu.

    // IDM_ABOUTBOX must be in the system command range.
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    /*/
    CMFCVisualManagerOffice2007 ::SetStyle(
        CMFCVisualManagerOffice2007::Office2007_LunaBlue);

    CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(
        CMFCVisualManagerOffice2007)); // <--Added to support deviant themes!
        /*/
    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != nullptr) {
        BOOL bNameValid;
        CString strAboutMenu;
        bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
        ASSERT(bNameValid);
        if (!strAboutMenu.IsEmpty()) {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE); // Set big icon
    SetIcon(m_hIcon, FALSE); // Set small icon
    setUpButtons();

    setupMeters();

    PostMessage(WM_FIRST_SHOWN);

    WINDOWPLACEMENT wp{0};
    wp.length = sizeof(wp);

    UINT nSize = 0;

    LPBYTE pPlacementBytes = NULL;
    WINDOWPLACEMENT orig{0};
    orig.length = sizeof(orig);

    GetWindowPlacement(&orig);
    std::string s = GetCommandLineA();
    if (s.find("safemode") == std::string::npos) {
        if (theApp.GetProfileBinary(
                L"MainWindow", L"Position", &pPlacementBytes, &nSize)) {
            ASSERT(pPlacementBytes != NULL);
            if (pPlacementBytes != NULL) {
                ASSERT(nSize == sizeof(WINDOWPLACEMENT));
                if (nSize == sizeof(WINDOWPLACEMENT)) {
                    memcpy(&wp, pPlacementBytes, nSize);
                }
                delete[] pPlacementBytes;
                wp.showCmd = 1;
                this->SetWindowPlacement(&wp);
                if (window_is_offscreen(m_hWnd)) {
                    orig.showCmd = 1;
                    SetWindowPlacement(&orig);
                }
            }
        }

    } else {
    }

    return TRUE; // return TRUE  unless you set the focus to a control
}

void CDspAudioHostDlg::setupMeters() {
    pbar_ctrl_create("Left Output", IDC_VUOUTL, vuOutL, TRUE);
    pbar_ctrl_create("Right Output", IDC_VU_OUT_R, vuOutR, TRUE);

    pbar_ctrl_create("Left Input", IDC_VU_IN_L, vuInL, TRUE);
    pbar_ctrl_create("Right Input", IDC_VU_IN_R, vuInR, TRUE);
    CPBar::colorvec_t cv;
    cv.emplace_back(
        CPBar::colors(RGB(51, 34, 0), RGB(180, 131, 2), 85)); // off / on  colors
    cv.emplace_back(
        CPBar::colors(RGB(81, 0, 0), RGB(220, 220, 0), 15)); // off / on colors
    vuOutL.colors_set(cv);
    vuOutR.colors_set(cv);
    vuOutL.peak_hold_color_set(RGB(250, 201, 40));
    vuOutR.peak_hold_color_set(RGB(250, 201, 40));

    vuInL.colors_set(cv);
    vuInR.colors_set(cv);
    vuInL.peak_hold_color_set(RGB(250, 201, 40));
    vuInR.peak_hold_color_set(RGB(250, 201, 40));
}

LRESULT CDspAudioHostDlg::OnDialogShown(WPARAM, LPARAM) {
    UpdateWindow();
    myInitDialog();
    return 0;
}

void CDspAudioHostDlg::OnSysCommand(UINT nID, LPARAM lParam) {
    if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    } else {
        if ((nID & 0xFFF0) == SC_CLOSE) {
            // if user clicked the "X"
            EndDialog(IDOK); // Close the dialog with IDOK (or IDCANCEL)
            //---end of code you have added
        } else {
            CDialog::OnSysCommand(nID, lParam);
        }
    }
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CDspAudioHostDlg::OnPaint() {
    if (IsIconic()) {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    } else {
        CDialogEx::OnPaint();
    }
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CDspAudioHostDlg::OnQueryDragIcon() {
    return static_cast<HCURSOR>(m_hIcon);
}

int findTextInCombo(CString findWhat, CComboBox& cb) {
    std::wstring_view vold = findWhat.GetBuffer();
    std::wstring_view vnow;

    for (int x = 0; x < cb.GetCount(); ++x) {
        CString ws;
        cb.GetLBText(x, ws);
        vnow = ws.GetBuffer();
        if (vnow.length() < vold.length()) {
            if (vold.find(vnow) != std::string::npos) {
                return x;
            }
        }
    }
    return -1;
}

void CDspAudioHostDlg::OnSelchangeComboApi() {

    CStringW old_input;
    CStringW old_output;
    PortAudioGlobalIndex old_ip_index;
    PortAudioGlobalIndex old_op_index;
    portaudio_cpp::AudioFormat old_format;
    // bool was_running = m_portaudio->m_running;
    int found = 0;

    if (m_portaudio) {
        const auto& ip = m_portaudio->currentDevice(DeviceTypes::input);
        const auto& op = m_portaudio->currentDevice(DeviceTypes::output);
        old_input = ip.extendedName.c_str();
        old_output = op.extendedName.c_str();
        old_format = m_portaudio->currentFormats(DeviceTypes::input);
        old_ip_index = ip.globalIndex;
        old_op_index = op.globalIndex;
    }
    this->myChangeAPI(L"");

    if (m_portaudio) {

        auto i = cboInput.FindStringExact(0, old_input);
        if (i < 0) {
            i = cboInput.FindString(0, old_input);
        }
        if (i < 0) {
            i = findTextInCombo(old_input, cboInput);
        }
        if (i >= 0) {
            ++found;
            cboInput.SetCurSel(i);
            OnSelchangeComboInput();
        }

        i = cboOutput.FindStringExact(0, old_output);
        if (i < 0) {
            i = cboOutput.FindString(0, old_output);
        }
        if (i < 0) {
            i = findTextInCombo(old_output, cboOutput);
        }
        if (i >= 0) {
            ++found;
            cboOutput.SetCurSel(i);
            OnCbnSelchangeComboOutput();
        }

        if (found == 2) {
            try {
                if (!old_format.is_empty()) {
                    OnBnClickedBtnPlay();
                }
            } catch (const std::exception& e) { //-V565
                ::MessageBoxA(m_hWnd, e.what(),
                    "Audio failed to automatically restart after api change",
                    MB_OK | MB_ICONSTOP);
            }
        }
    }
}

void CDspAudioHostDlg::OnBnClickedRefreshPA() {
    CString orig_text;
    auto info = m_portaudio->info();
    m_btnRefresh.GetWindowTextW(orig_text);
    m_btnRefresh.SetWindowTextW(L"Please wait ...");
    CCmdTarget::BeginWaitCursor();

    myInitDialog();
    m_btnRefresh.SetWindowTextW(orig_text);

    m_portaudio->restoreInfo(info);

    CCmdTarget::EndWaitCursor();
}

void CDspAudioHostDlg::OnBnClickedBtnPlugpath() {
    CString strOutFolder;
    // note: the application class is derived from CWinAppEx
    CShellManager* pShellManager = theApp.GetShellManager();
    CString existing_folder;
    const CString default_plug_folder(
        L"C:\\Program Files (x86)\\AudioEnhance Sound Router\\plugins\\");

    lblPluginPath.GetWindowTextW(existing_folder);
    if (!PathFileExists(existing_folder)) {
        existing_folder = default_plug_folder;
    }
    if (pShellManager->BrowseForFolder(
            strOutFolder, this, existing_folder, L"Choose the plugins folder to use")) {
        strOutFolder += L"\\";
        lblPluginPath.SetWindowTextW(strOutFolder);
        findAvailDSPs();
    }
}

void CDspAudioHostDlg::OnStnClickedStaticPlugpath() {
    // TODO: Add your control notification handler code here
}

void CDspAudioHostDlg::OnSelchangeComboInput() {
    CString name;
    const auto idx = cboInput.GetCurSel();
    cboInput.GetLBText(idx, name);
    CStringA nName(name);
    const auto dev = m_portaudio->findDevice(nName.GetBuffer());
    if (!dev) {
        CString msg(L"Sorry, can't find input device: ");
        msg += name;
        MessageBox(msg, L"Error: Cannot find input device");
    } else {
        m_portaudio->changeDevice(*dev, portaudio_cpp::DeviceTypes::input);
    }
}

void CDspAudioHostDlg::OnCbnSelchangeComboOutput() {
    CString name;
    const auto idx = cboOutput.GetCurSel();
    cboOutput.GetLBText(idx, name);
    CStringA nName(name);
    const auto dev = m_portaudio->findDevice(nName.GetBuffer());
    if (!dev) {
        CString msg(L"Sorry, can't find output device: ");
        msg += name;
        MessageBox(msg, L"Error: Cannot find output device");
    } else {
        m_portaudio->changeDevice(*dev, portaudio_cpp::DeviceTypes::output);
    }
}

void CDspAudioHostDlg::OnBnClickedBtnRefreshPlugs() {
    CCmdTarget::BeginWaitCursor();
    EnableWindow(FALSE);
    findAvailDSPs();
    showAvailDSPs();
    EnableWindow(TRUE);
    CCmdTarget::EndWaitCursor();
}

void CDspAudioHostDlg::tracePlugins() {
    winamp_dsp::Host& host = m_winamp_host;
    for (const auto& plug : host.m_enumerator.plugins()) {
        TRACE("Plugin found @ %s\n", plug.filepath().data());
        TRACE("Plugin has description: %s\n", plug.description().data());
        TRACE("\n");
    }

    TRACE("\n");
}

void CDspAudioHostDlg::showClipped() {
    static DWORD last_shown_clip = timeGetTime();

    if (m_clipped) {
        lblClip.ShowWindow(TRUE);
        last_shown_clip = timeGetTime();
    } else {

        if (timeGetTime() - last_shown_clip > 500) {
            lblClip.ShowWindow(FALSE);
        }
    }
}

void CDspAudioHostDlg::showSamplerate() {

    if (cboSampleRate.GetCount() == 0) {

        cboSampleRate.AddString(L"192000");
        cboSampleRate.AddString(L"128000");
        cboSampleRate.AddString(L"96000");
        cboSampleRate.AddString(L"48000");
        cboSampleRate.AddString(L"44100");
        cboSampleRate.AddString(L"22050");
    }

    auto thing = cboSampleRate.FindStringExact(
        0, std::to_wstring(m_paSettings.samplerate).data());
    assert(thing >= 0);
    cboSampleRate.SetCurSel(thing);
}

void CDspAudioHostDlg::OnBnClickedBtnRefreshPlugs2() {}

void CDspAudioHostDlg::addActivatedPlugToUI(const winamp_dsp::Plugin& plug) {
    CString ws(plug.description().data());
    auto count = listCur.GetItemCount();
    listCur.InsertItem(count, ws, 0);

    count++;
    setUpDownButtonsState();
    listCur.SetItemState(count - 1, LVIS_SELECTED, LVIS_SELECTED);
}

void CDspAudioHostDlg::removeActivatedPlugFromUI(int idx) {

    listCur.DeleteItem(idx);
    const auto count = listCur.GetItemCount();
    setUpDownButtonsState();

    if (count > 1) {
        listCur.SetItemState(idx - 1, LVIS_SELECTED, LVIS_SELECTED);
    } else {
        if (count) {
            listCur.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
        }
    }
}

BOOL CDspAudioHostDlg::restorePlugWindowPosition(const winamp_dsp::Plugin& plug) {

    HWND hwnd = find_plug_window(plug.configHandleWindowText());
    ASSERT(hwnd);
    if (!hwnd) {
        hwnd = FindPluginWindow(plug.description());
    }

    if (!hwnd) return FALSE;
    WINDOWPLACEMENT wp{0};
    wp.length = sizeof(wp);
    CString wdesc(plug.description().data());

    UINT nSize = 0;
    BOOL ret = FALSE;
    LPBYTE pPlacementBytes = NULL;

    std::string s = GetCommandLineA();
    if (s.find("safemode") == std::string::npos) {

        if (theApp.GetProfileBinary(
                L"PlugWindowPositions", wdesc, &pPlacementBytes, &nSize)) {
            ASSERT(pPlacementBytes != NULL);
            if (pPlacementBytes != NULL) {
                ASSERT(nSize == sizeof(WINDOWPLACEMENT));
                if (nSize == sizeof(WINDOWPLACEMENT)) {
                    memcpy(&wp, pPlacementBytes, nSize);
                }
                delete[] pPlacementBytes;
                wp.showCmd = 1;
                ret = ::SetWindowPlacement(hwnd, &wp);
            }
        }
    }

    return ret;
}

void CDspAudioHostDlg::savePlugWindowPositions() {

    WINDOWPLACEMENT mypos{0};
    mypos.length = sizeof(mypos);
    if (GetWindowPlacement(&mypos)) {
        theApp.WriteProfileBinary(
            L"MainWindow", L"Position", (LPBYTE)&mypos, sizeof(mypos));
    }

    for (auto&& plug : m_winamp_host.activePlugins()) {

        HWND hwnd = plug.configHandleWindowGet();
        if (!IsWindow(hwnd)) { // eg when user closes plugin window altogether
            hwnd = find_plug_window(plug.configHandleWindowText());
        }
        ASSERT(hwnd);
        if (!hwnd) {
            hwnd = FindPluginWindow(plug.description());
        }
        ASSERT(hwnd);
        if (IsWindow(hwnd)) {
            WINDOWPLACEMENT wp{0};
            wp.length = sizeof(wp);

            BOOL got = ::GetWindowPlacement(hwnd, &wp);
            ASSERT(got);
            CString wdesc(plug.description().data());
            if (got) {
                theApp.WriteProfileBinary(
                    L"PlugWindowPositions", wdesc, (LPBYTE)(&wp), sizeof(wp));
            }
        }
    }
}

void CDspAudioHostDlg::settingsSavePlugins() {
    std::string plugs = m_winamp_host.getActivePlugsAsString();
    CString ws(plugs.data());
    theApp.WriteProfileStringW(L"WinampHost", L"ActivePlugs", ws);

    savePlugWindowPositions();

    BOOL b = FALSE;
    if (m_portaudio->m_bMonoInput) {
        b = TRUE;
    } else {
        b = FALSE;
    }
    {
        theApp.WriteProfileInt(L"WinampHost", L"ForceMonoInput", b);
        const auto pos = sldVol.GetPos();
        theApp.WriteProfileInt(L"WinampHost", L"OutVol", pos);
    }

    {
        const auto pos = sldVolIn.GetPos();
        theApp.WriteProfileInt(L"WinampHost", L"InputVolume", pos);
    }
}

void CDspAudioHostDlg::settingsGetPlugins(bool apply) {
    CString plugs = theApp.GetProfileStringW(L"WinampHost", L"ActivePlugs", L"");
    if (!plugs.IsEmpty()) {
        if (apply) {
            CStringA csa(plugs);
            const char sep = *winamp_dsp::Host::PLUG_SEP;
            auto descriptions = winamp_dsp::my::strings::split(csa.GetBuffer(), sep);
            for (const auto& s : descriptions) {
                auto ptr = m_winamp_host.findPlug(s);
                auto existing = m_winamp_host.findActivatedPlugByDesc(s);
                if (!existing) {
                    if (ptr) {
                        myActivatePlug(ptr);
                    }
                }
            }
        }
    }
    const auto b = theApp.GetProfileIntW(L"WinampHost", L"ForceMonoInput", FALSE);
    chkForceMono.SetCheck(b);
    OnBnClickedChkForceMono();

    SetActiveWindow();
}

static inline BOOL CALLBACK EnumWindowsProcFindAllA(_In_ HWND hwnd, _In_ LPARAM lParam) {
    auto pthis = (CDspAudioHostDlg*)lParam;
    std::wstring str(MAX_PATH, L'\0');

    ::GetWindowText(hwnd, str.data(), MAX_PATH);
    pthis->m_child_windowsA.insert({hwnd, str});
    return TRUE;
}

static inline BOOL CALLBACK EnumWindowsProcFindAllB(_In_ HWND hwnd, _In_ LPARAM lParam) {
    auto pthis = (CDspAudioHostDlg*)lParam;
    std::wstring str(MAX_PATH, L'\0');

    ::GetWindowText(hwnd, str.data(), MAX_PATH);
    pthis->m_child_windowsB.insert({hwnd, str});
    return TRUE;
}

winamp_dsp::Plugin& CDspAudioHostDlg::manageActivatePlug(winamp_dsp::Plugin& plug) {
    m_child_windowsA.clear();
    m_child_windowsB.clear();

    EnumThreadWindows(GetCurrentThreadId(), EnumWindowsProcFindAllA, (LPARAM)this);
    auto& activated = m_winamp_host.activatePlug(plug);
    activated.showConfig();
    doEvents();
    EnumThreadWindows(GetCurrentThreadId(), EnumWindowsProcFindAllB, (LPARAM)this);

    assert(m_child_windowsB.size() > m_child_windowsA.size());
    std::map<HWND, std::wstring> diff;
    auto& a = m_child_windowsA;
    auto& b = m_child_windowsB;
    std::set_symmetric_difference(
        a.begin(), a.end(), b.begin(), b.end(), std::inserter(diff, diff.begin()));

    const auto this_hwnd = this->m_hWnd;
    size_t ctr = 0;

    for (const auto& pr : diff) {
        ctr++;
        auto parent = ::GetParent(pr.first);
        if (parent == this_hwnd) {
            TRACE(L"Found plug window, with text: %s\n", pr.second.data());
            ASSERT(IsWindow(pr.first));
            activated.configHandleWindowTextSet(pr.second);
            activated.configHandleWindowSet(pr.first);
            break;
        }
    }

    return activated;
}

winamp_dsp::Plugin* CDspAudioHostDlg::myActivatePlug(
    winamp_dsp::Plugin* plug, bool addToUI) {

    bool was_running = false;
    if (m_portaudio && m_portaudio->m_running) {
        was_running = true;
        m_portaudio->Stop();
    }

    assert(plug);
    if (!plug) {
        return plug;
    }

    try {

        auto& activated = manageActivatePlug(*plug);
        hwnd_found = find_plug_window(activated.configHandleWindowText());

        if (hwnd_found == nullptr) {
            hwnd_found = FindPluginWindow(activated.description());
        }

        if (hwnd_found) {
            restorePlugWindowPosition(activated);
            ::BringWindowToTop(hwnd_found);
            ::SetActiveWindow(hwnd_found);
            ::ShowWindow(hwnd_found, SW_SHOWNORMAL);
        }
        if (addToUI) {
            addActivatedPlugToUI(activated);
            SetForegroundWindow();
        }

        if (was_running && m_portaudio) {
            portaudioStart();
        }
        return &activated;
    } catch (const std::exception& e) {
        MessageBoxA(m_hWnd, e.what(), "Error loading plugin", MB_OK);
    }

    return nullptr;
}

std::string desc_find;
int find_level = 0;

static inline BOOL CALLBACK EnumWindowsProcFindPlug(_In_ HWND hwnd, _In_ LPARAM) {
    std::string s;
    s.resize(256);
    ::GetWindowTextA(hwnd, &s[0], 256);

    s.resize(s.find_first_of('\0'));
    if (find_level == 0) {

        if (s == desc_find) {
            hwnd_found = hwnd;
            return FALSE;
        }
    } else {

        if (find_level == 1) {

            if (s.find(desc_find) == 0) {
                hwnd_found = hwnd;
                return FALSE;
            }
        } else {
            if (s.find(desc_find) != std::string::npos) {
                hwnd_found = hwnd;
                return FALSE;
            }
        }
    }
    return TRUE;
}

HWND CDspAudioHostDlg::FindPluginWindow(std::string_view desc) {

    if (desc == "Tomass' Limiter V1.0") {
        desc = "Tomass Limiter V1.0";
    }
    desc_find = desc;

    find_level = 0;
    hwnd_found = nullptr;
    EnumThreadWindows(GetCurrentThreadId(), EnumWindowsProcFindPlug, (LPARAM)this);
    if (!hwnd_found) { //-V547
        find_level = 1;
        EnumThreadWindows(GetCurrentThreadId(), EnumWindowsProcFindPlug, (LPARAM)this);
    }

    if (!hwnd_found) { //-V547
        find_level = 2;
        EnumThreadWindows(GetCurrentThreadId(), EnumWindowsProcFindPlug, (LPARAM)this);
    }
    return hwnd_found;
}

void CDspAudioHostDlg::saveAPI() {
    cboAPI.GetLBText(cboAPI.GetCurSel(), m_paSettings.m_api);
    m_paSettings.saveAll();
}

void CDspAudioHostDlg::saveInput() {
    cboInput.GetLBText(cboInput.GetCurSel(), m_paSettings.m_input);
    m_paSettings.saveAll();
}

void CDspAudioHostDlg::saveOutput() {
    cboOutput.GetLBText(cboOutput.GetCurSel(), m_paSettings.m_output);
    m_paSettings.saveAll();
}

void CDspAudioHostDlg::OnBnClickedBtnAdd() {

    CCmdTarget::BeginWaitCursor();
    POSITION pos = this->listAvail.GetFirstSelectedItemPosition();
    if (pos == NULL) {
        MessageBox(L"You need to select at least one plugin on the left list first.");
    } else {
        while (pos) {
            int nItem = listAvail.GetNextSelectedItem(pos);
            TRACE1("Item %d was selected!\n", nItem);
            CString ws = listAvail.GetItemText(nItem, 0);
            CStringA wsa(ws);
            auto existing = m_winamp_host.findActivatedPlugByDesc(wsa.GetBuffer());

            if (existing != nullptr) {
                CString msg = L"The plugin: ";
                msg += wsa;
                msg += L"\nis already loaded\n\nYou cannot load a plugin more than "
                       L"once\n\n";
                msg += L"Do you want to show the window for this plugin? (Bear in mind "
                       L"it may be minimised)";
                int result = SHMessageBoxCheck(this->GetSafeHwnd(), msg,
                    TEXT("One Instance Of Plugin Warning"), MB_ICONINFORMATION | MB_YESNO,
                    IDYES, TEXT("{9D5CE92A-2EA9-43A2-A19E-736853BCCBB9}"));
                if (result == IDYES) {

                    existing->showConfig();
                    auto fnd = FindPluginWindow(existing->description());
                    if (fnd) { //-V547
                        centreWindowOnMe(fnd, GetSafeHwnd(), false);
                    }
                }
                break;
            }
            if (!ws.IsEmpty()) {

                auto* found = m_winamp_host.findPlug(wsa.GetBuffer());
                if (!found) {
                    MessageBox(L"Unexpected, cannot find plugin");
                } else {
                    if (myActivatePlug(found)) {
                        settingsSavePlugins();
                    }
                }
            }
        }
    }
    CCmdTarget::EndWaitCursor();
}

int getSelectedIndex(CListCtrl& lv) {
    POSITION pos = lv.GetFirstSelectedItemPosition();
    if (pos == NULL) {
        return -1;
    }

    int nItem = lv.GetNextSelectedItem(pos);
    return nItem;
}

void CDspAudioHostDlg::OnBnClickedBtnRemove() {
    POSITION pos = this->listCur.GetFirstSelectedItemPosition();

    bool was_running = false;
    if (this->m_portaudio) {
        was_running = m_portaudio->m_running;
        m_portaudio->Stop();
    }

    if (pos == NULL) {
        MessageBox(L"You need to select at least one plugin on the right list first.");
    } else {
        while (pos) {
            int nItem = listCur.GetNextSelectedItem(pos);
            TRACE1("Item %d was selected!\n", nItem);
            CString ws = listCur.GetItemText(nItem, 0);
            if (!ws.IsEmpty()) {
                const auto* found = m_winamp_host.findActivatedPlug(nItem);
                if (found == nullptr) { //-V547
                    MessageBox(L"Unexpected, cannot find activate plugin");
                } else {
                    bool removed = m_winamp_host.removeActivatedPlug(found);
                    if (removed) {
                        removeActivatedPlugFromUI(nItem);
                        settingsSavePlugins();
                    }
                }
            }
        }
    }

    if (was_running) {
        try {
            this->portaudioStart();
        } catch (const std::exception&) { //-V565
        }
    }
}

void CDspAudioHostDlg::myCurListShowConfig(int idx) {
    if (idx < 0) {
        MessageBox(L"First select a plugin in the active plugin list");
    } else {
        auto plug = m_winamp_host.findActivatedPlug(idx);
        if (!plug) {
            MessageBox(L"Unexpected: could not find the plugin");
        } else {
            plug->showConfig();
            auto hWnd = find_plug_window(plug->configHandleWindowText());
            if (hWnd == nullptr) {
                hWnd = FindPluginWindow(plug->description()); //-V1048
            }
            if (hWnd != nullptr) {
                centreWindowOnMe(hWnd, GetSafeHwnd());
            }
        }
    }
}

void CDspAudioHostDlg::OnBnClickedBtnConfigPlug() {
    const auto idx = getSelectedIndex(listCur);
    myCurListShowConfig(idx);
}

void CDspAudioHostDlg::OnDblclkListCur(NMHDR* pNMHDR, LRESULT* pResult) {

    // LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    LPNMITEMACTIVATE pitem = (LPNMITEMACTIVATE)pNMHDR;
    int row = pitem->iItem;
    myCurListShowConfig(row);

    *pResult = 0;
}

void CDspAudioHostDlg::OnDblclkListAvail(NMHDR* pNMHDR, LRESULT* pResult) {
    // LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    LPNMITEMACTIVATE pitem = (LPNMITEMACTIVATE)pNMHDR;
    int row = pitem->iItem;
    if (row > 0) {
        CString ws = listAvail.GetItemText(row, 0);
        CStringA wsa(ws);
        auto plug = m_winamp_host.findPlug(wsa.GetBuffer());
        if (!plug) {
            MessageBox(L"Unexpected: cannot find plug!");
        }
        myActivatePlug(plug);
    }
    *pResult = 0;
}

void CDspAudioHostDlg::myShowActivePlugs() {

    listCur.DeleteAllItems();
    for (const auto& plug : m_winamp_host.activePlugins()) {

        if (!plug.description().empty()) {
            // auto p = m_winamp_host.findPlug(plug.description());
            CString wcs(plug.description().data());
            auto inserted = listCur.InsertItem(listCur.GetItemCount(), wcs, 0);
            (void)inserted;
            assert(inserted >= 0);
        }
    };
}

static inline BOOL CenterWindow(HWND hwndWindow, int topOffset = 0) {
    HWND hwndParent;
    RECT rectWindow, rectParent;

    // make the window relative to its parent
    if ((hwndParent = GetParent(hwndWindow)) != NULL) {
        GetWindowRect(hwndWindow, &rectWindow);
        GetWindowRect(hwndParent, &rectParent);

        WINDOWPLACEMENT wp{0};
        wp.length = sizeof(WINDOWPLACEMENT);
        BOOL gotp = ::GetWindowPlacement(hwndWindow, &wp);
        ASSERT(gotp);

        rectWindow = wp.rcNormalPosition;
        int nWidth = rectWindow.right - rectWindow.left;
        int nHeight = rectWindow.bottom - rectWindow.top;

        int nX = ((rectParent.right - rectParent.left) - nWidth) / 2 + rectParent.left;
        int nY = ((rectParent.bottom - rectParent.top) - nHeight) / 2 + rectParent.top;

        int nScreenWidth = GetSystemMetrics(SM_CXSCREEN);
        int nScreenHeight = GetSystemMetrics(SM_CYSCREEN);

        // make sure that the dialog box never moves outside of the screen
        if (nX < 0) nX = 0;
        if (nY < 0) nY = 0;
        if (nX + nWidth > nScreenWidth) nX = nScreenWidth - nWidth;
        if (nY + nHeight > nScreenHeight) nY = nScreenHeight - nHeight;

        CRect final(nX, nY + topOffset, nX + nWidth, nY + nHeight + topOffset);
        // MoveWindow(hwndWindow, nX, nY + topOffset, nWidth, nHeight, FALSE);
        wp.rcNormalPosition = final;
        gotp = ::SetWindowPlacement(hwndWindow, &wp);
        assert(gotp);

        return TRUE;
    }

    return FALSE;
}

// if you don't force then it will only be centred on me when hwnd is off the screen
// partially or completely
void CDspAudioHostDlg::centreWindowOnMe(
    HWND hwnd, HWND parent, bool force, int topOffset) {

    (void)parent;
    if (force) {
        ::CenterWindow(hwnd, topOffset);

    } else {
        if (window_is_offscreen(hwnd)) {
            ::CenterWindow(hwnd, topOffset);
        }
    }

    ::ShowWindow(hwnd, SW_SHOWNORMAL);
    ::BringWindowToTop(hwnd_found);
    ::SetActiveWindow(hwnd_found);
}

void CDspAudioHostDlg::setUpDownButtonsState() {
    int cnt = listCur.GetItemCount();
    BOOL en = cnt > 0;
    btnUp.EnableWindow(en);
    btnDown.EnableWindow(en);
}

void CDspAudioHostDlg::OnBnClickedBtnUp() {
    int idxA = getSelectedIndex(listCur);
    if (idxA < 0) {
        MessageBox(L"Please select an active plugin in the list");
        return;
    }

    int idxB = idxA - 1;
    if (idxB < 0) {
        // the first one cannot be moved up
        return;
    }

    m_winamp_host.swapPlugins(idxA, idxB);
    int selindex = idxB;

    myShowActivePlugs();
    listCur.SetItemState(selindex, LVIS_SELECTED, LVIS_SELECTED);

    setUpDownButtonsState();
    settingsSavePlugins();
}

void CDspAudioHostDlg::OnBnClickedBtnDown() {
    int idxA = getSelectedIndex(listCur);
    const int cnt = listCur.GetItemCount();
    if (idxA < 0) {
        MessageBox(L"Please select an active plugin in the list");
        return;
    }

    int idxB = idxA + 1;
    if (idxB >= cnt) return; // last item cannot be moved down
    m_winamp_host.swapPlugins(idxA, idxB);

    myShowActivePlugs();
    listCur.SetItemState(idxB, LVIS_SELECTED, LVIS_SELECTED);

    setUpDownButtonsState();
    settingsSavePlugins();
}

void CDspAudioHostDlg::OnClose() {

    if (m_portaudio) m_portaudio->Stop();

    // make plugs visible before you save their positions
    for (int i = 0; i < listCur.GetItemCount(); i++) {
        myCurListShowConfig(i);
        doEvents();
    }
    settingsSavePlugins();

    m_paSettings.saveAll();
    __super::OnClose();
}

void CDspAudioHostDlg::OnTimer(UINT_PTR nIDEvent) {
    static bool busy = false;

    if (busy) {
        return;
    }
    busy = true;
    showMeters();
    static std::wstring ws;
    if (ws.size() != 1024) ws.resize(1024);
    std::wstring sw;

    if (m_portaudio) {
        if (m_portaudio->m_running) {
            double t = m_portaudio->currentTime();
            portaudio_cpp::timestring(true, sw, (uint64_t)(t * 1000.0),
                portaudio_cpp::time_flags::simple_format, ws.data());
            assert(sw.size() == 15);
            lblElapsedTime.SetWindowTextW(sw.data());
        } else {
            portaudio_cpp::timestring(
                true, sw, 0, portaudio_cpp::time_flags::simple_format, ws.data());
            lblElapsedTime.SetWindowTextW(sw.data());
        }
    }

    __super::OnTimer(nIDEvent);
    busy = false;
}

bool CDspAudioHostDlg::myPreparePlay(portaudio_cpp::AudioFormat& fmt) {

    bool retval = true;
    auto& myfmt = fmt;
    myfmt.forInOrOut = DeviceTypes::input;
    auto errcode = m_portaudio->isFormatSupported(myfmt);
    if (errcode != paFormatIsSupported) {
        std::string serr(Pa_GetErrorText(errcode));
        serr += "\n";
        serr += Pa_GetLastHostErrorInfo()->errorText;
        MessageBox(L"The input device does not support the requested input audio format",
            L"Bad input device Parameters");
        retval = false;
    }

    if (retval) {
        myfmt.forInOrOut = DeviceTypes::output;
        errcode = m_portaudio->isFormatSupported(myfmt);

        if (errcode == paInvalidChannelCount) {
            myfmt.channels = portaudio_cpp::ChannelsType::mono;

            errcode = m_portaudio->isFormatSupported(myfmt);
        }
        if (errcode) {
            std::string serr = Pa_GetErrorText(errcode);
            serr += "\n";
            serr += Pa_GetLastHostErrorInfo()->errorText;
            serr += "\n";
        }

        if (errcode != paFormatIsSupported) {
            MessageBox(
                L"The output device does not support the requested output audio format",
                L"Bad output device Parameters");

            retval = false;
        }
    }

    /*/
    if (1) {
        const auto& d = m_portaudio->currentDevice(portaudio_cpp::DeviceTypes::output);
        const auto& id = m_portaudio->currentDevice(portaudio_cpp::DeviceTypes::input);
        const auto& sup_out = d.get_supported_audio_types();
        const auto& sup_in = id.get_supported_audio_types();
        const auto rates_out = d.getSupportedSamplerates();
        const auto rates_in = id.getSupportedSamplerates();
        TRACE("ffs");
    }
    /*/

    return retval;
}

void CDspAudioHostDlg::OnBnClickedBtnPlay() {

    if (!m_portaudio) return;
    if (m_portaudio && m_portaudio->m_running) {
        m_portaudio->Close();
    }
    int samplerates[] = {0, 0};
    samplerates[0] = (int)m_portaudio->devices()[0].info.defaultSampleRate;
    samplerates[1] = (int)m_portaudio->devices()[1].info.defaultSampleRate;
    std::string strSamplerates("Input default samplerate = ");
    strSamplerates += std::to_string(samplerates[0]);
    strSamplerates += "\n";
    strSamplerates += "Output default samplerate = ";
    strSamplerates += std::to_string(samplerates[1]);

    try {
        portaudio_cpp::ChannelsType chans; // stereo by default
        auto sr = portaudio_cpp::SampleRateType(m_paSettings.samplerate);

        auto myfmt = portaudio_cpp::AudioFormat::makeAudioFormat(
            portaudio_cpp::DeviceTypes::input, SampleFormat::Float32, sr, chans);

        // if (!myPreparePlay(myfmt)) {
        //     return;
        // }
        //  do this even if errors detected, so that we might find a common samplerate
        myPreparePlay(myfmt);

        if (1) {
            if (myPreparePortAudio(chans, sr) == NOERROR) {
                portaudioStart();
                std::string sTime;
                int l = (int)(m_portaudio->currentLatency() * 1000.0);
                sTime += "[Latency] ";
                sTime += std::to_string(l);
                sTime += " ms.  ";
                sTime += "Format: ";
                sTime += m_portaudio->sampleFormat().to_string();
                sTime += " ";
                sTime += m_portaudio->sampleRate().to_string();
                ::SetWindowTextA(lblPa.GetSafeHwnd(), sTime.c_str());

                m_paSettings.saveAll();
            }
        }
    } catch (const std::exception& e) {
        const auto code = m_portaudio->errCode();
        if (code == paInvalidSampleRate || code == paSampleFormatNotSupported) {
            auto data = m_portaudio->findCommonSamplerate();
            if (data <= 0) {
                MessageBox(CStringW(e.what()));

            } else {
                std::wstringstream ws;
                ws << L"Information: these two devices do share a common sample rate "
                      L"of: ";
                ws << data;
                ws << L"\n\n Would you like to try that instead?";
                CString w(ws.str().c_str());
                auto response = MessageBox(
                    w, L"Try a different sample rate?", MB_YESNO | MB_ICONINFORMATION);
                if (response == IDYES) {
                    std::wstring wsamplerate = std::to_wstring(data);
                    auto found = cboSampleRate.FindStringExact(0, wsamplerate.c_str());
                    if (found >= 0) {
                        cboSampleRate.SetCurSel(found);
                        OnCbnSelchangeComboSamplerate(); // which calls us again
                        return;
                    }
                }
            }
        } else {
            MessageBox(CStringW(e.what()));
        }
//::ShellExecute(*this, L"rundll32.exe", L"shell32.dll,Control_RunDLL
//: mmsys.cpl,,2",
//    L"", L"", SW_SHOWNORMAL);
#pragma warning(disable : 28159)
        if (std::string_view(m_portaudio->m_currentApi.name) != "ASIO")
            ::WinExec(
                "rundll32.exe shell32.dll,Control_RunDLL mmsys.cpl,,0", SW_SHOWNORMAL);
    }
}

void CDspAudioHostDlg::OnBnClickedBtnStop() {

    if (m_portaudio && m_portaudio->m_running) {

        m_portaudio->Stop();
    }
}

HBRUSH CDspAudioHostDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor) {

    HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
    // this is how you stop flicker in a static text control.#
    if (true) {
        if (CTLCOLOR_STATIC == nCtlColor && pWnd == (CWnd*)&lblElapsedTime) {
            if (NULL == m_brush.GetSafeHandle()) {
                m_brush.CreateStockObject(NULL_BRUSH);
            } else {
                return m_brush;
            }
        } else {

            if (CTLCOLOR_STATIC == nCtlColor && pWnd == (CWnd*)&lblClip) {
                static CBrush brush(RGB(200, 0, 0));
                return brush;
            }
        }
    }

    /*/
    if (nCtlColor == CTLCOLOR_LISTBOX) {
        COMBOBOXINFO info;
        info.cbSize = sizeof(info);
        ::SendMessage(cboInput.m_hWnd, CB_GETCOMBOBOXINFO, 0, (LPARAM)&info);
        COMBOBOXINFO info1;
        info1.cbSize = sizeof(info1);
        //::SendMessage(hWndComboBox1, CB_GETCOMBOBOXINFO, 0, (LPARAM)&info1);

        if (pWnd->GetSafeHwnd() == info.hwndCombo) {
            HDC dc = pDC->GetSafeHdc();
            SetBkMode(dc, OPAQUE);
            SetTextColor(dc, RGB(255, 255, 0));
            SetBkColor(dc, 0x383838); // 0x383838
            static HBRUSH comboBrush = CreateSolidBrush(0x383838); // global var
            return comboBrush;
        }
    } else if (nCtlColor == WM_CTLCOLOREDIT) {
        {
            HWND hWnd = pWnd->GetSafeHwnd();
            HDC dc = pDC->GetSafeHdc();
            if (hWnd == cboInput.GetSafeHwnd()) {
                SetBkMode(dc, OPAQUE);
                SetTextColor(dc, RGB(255, 0, 255));
                SetBkColor(dc, 0x383838); // 0x383838
                static HBRUSH comboBrush = CreateSolidBrush(0x383838); // global var
                return comboBrush;
            }
        }
    }

    constexpr COLORREF darkBkColor = 0x383838;
    constexpr COLORREF darkTextColor = 0xFFFFFF;
    static HBRUSH hbrBkgnd = nullptr;
    if (g_darkModeSupported && g_darkModeEnabled) {
        HDC hdc = pDC->GetSafeHdc();
        SetTextColor(hdc, darkTextColor);
        SetBkColor(hdc, darkBkColor);
        if (!hbrBkgnd) hbrBkgnd = CreateSolidBrush(darkBkColor);
        return hbrBkgnd;
    }
    /*/

    return hbr;
}

void CDspAudioHostDlg::OnNMCustomdrawSliderVol(NMHDR*, LRESULT* pResult) {
    // LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
    //  TODO: Add your control notification handler code here
    *pResult = 0;
}

void CDspAudioHostDlg::sliderMoved(CSliderCtrl& which) {
    if (&which == &sldVol) {
        float pos = -((float)sldVol.GetPos() + std::numeric_limits<float>::epsilon());
        pos /= 100.0f; // db
        const auto val = portaudio_cpp::db_to_value(-pos);
        this->m_volume = val;
    } else if (&which == &sldVolIn) {
        float pos = -((float)sldVolIn.GetPos() + std::numeric_limits<float>::epsilon());

        pos /= 100.0f; // db
        pos += 25.0f; // provide some boost on the input
        auto val = portaudio_cpp::db_to_value(-pos);

        this->m_volumeIn = val;
    }
}

void CDspAudioHostDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) {
    CSliderCtrl* pSlider = reinterpret_cast<CSliderCtrl*>(pScrollBar);

    // Check which slider sent the notification
    if (TRUE) {
        switch (nSBCode) {
            case TB_LINEUP:
            case TB_LINEDOWN:
            case TB_PAGEUP:
            case TB_PAGEDOWN:
            case TB_THUMBPOSITION: {
                sliderMoved(*pSlider); //-V1037
                break;
            }
            case TB_TOP:
            case TB_BOTTOM:
            case TB_THUMBTRACK: {
                sliderMoved(*pSlider);
                break;
            }
            case TB_ENDTRACK:
            default: break;
        }
    }

    __super::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CDspAudioHostDlg::OnBnClickedChkForceMono() {
    // TODO: Add your control notification handler code here

    if (m_portaudio) {
        if (chkForceMono.GetCheck() == TRUE) {
            m_portaudio->monoInputSet(true);
        } else {
            m_portaudio->monoInputSet(false);
        }
    }
}

void CDspAudioHostDlg::OnStnClickedVuOutR() {
    // TODO: Add your control notification handler code here
}

void CDspAudioHostDlg::showMeters() {
    double left = 0;
    double right = 0;
    if (m_portaudio && m_portaudio->m_running) {
        const auto& env = m_portaudio->m_env_output.m_env;
        cpp::audio::envelope_val<double>(left, right, 2, env[0], env[1], 10000L, 80L);
    }
    vuOutL.value_set(left);
    vuOutR.value_set(right);

    if (m_portaudio && m_portaudio->m_running) {
        const auto& env = m_portaudio->m_env_input.m_env;
        cpp::audio::envelope_val<double>(left, right, 2, env[0], env[1], 10000L, 80L);
        showClipped();
    }
    vuInL.value_set(left);
    vuInR.value_set(right);
}

void CDspAudioHostDlg::portaudioStart() {

    m_portaudio->Start();
    if (chkForceMono.GetCheck()) {
        m_portaudio->monoInputSet(true);
    } else {
        m_portaudio->monoInputSet(false);
    }
}

void CDspAudioHostDlg::pbar_ctrl_create(
    const std::string& name, const int idctrl, CPBar& pbar, int invert) {
#pragma warning(disable : 4130) // VC wants to complain about my ASSERT statement.
    CRect rect;
    GetDlgItem(idctrl)->GetWindowRect(&rect);
    rect.top += 4;
    rect.bottom += 4;
    rect.bottom -= 1;
    ScreenToClient(&rect);
    GetDlgItem(idctrl)->ShowWindow(SW_HIDE);
    if (pbar.GetSafeHwnd()) {
        ASSERT("You likely screwed up. This pbar already has a window. Are you "
               "assigning "
               "the wrong one?"
            == 0);
    }
    pbar.Create(name, AfxGetInstanceHandle(), 0, WS_CHILD | WS_VISIBLE /*| WS_BORDER*/,
        rect, this, idctrl);
    pbar.orientation_set(CPBar::orientation_t::vertical);
    pbar.maximum_set(10000);
    pbar.smooth_set(false);
    pbar.draw_inverted_set(invert);
}

void CDspAudioHostDlg::OnStnClickedClip() {
    // TODO: Add your control notification handler code here
}

void CDspAudioHostDlg::OnCbnSelchangeComboSamplerate() {
    afterSamplerateChanged();
}

void CDspAudioHostDlg::afterSamplerateChanged(bool fromUserClick) {

    const auto idx = cboSampleRate.GetCurSel();
    CString cs;
    cboSampleRate.GetLBText(idx, cs);
    CStringA csa(cs);
    const auto sr = atoi(csa);

    if (fromUserClick) {
        if (sr != 44100) {
            CString msg(
                L"Please note that almost all winamp dsp plugins (with the notable "
                L"exception of Maximod)\n were never designed for anything other "
                L"than "
                L"44100 "
                L"sample rate.\n\nThey may give silent output, or crash the "
                L"application "
                L"if "
                L"you try to use them with a non-44100 sample rate");

            SHMessageBoxCheck(this->GetSafeHwnd(), msg,
                TEXT("Non-standard sample rate warning"), MB_ICONINFORMATION | MB_OK,
                IDYES, TEXT("{0C44B5F5-F102-4115-B782-7EDC567302B1}"));
        }
    }
    m_paSettings.samplerate = sr;
    this->OnBnClickedBtnPlay();
}

void CDspAudioHostDlg::OnSize(UINT nType, int cx, int cy) {
    __super::OnSize(nType, cx, cy);

    /*/
        The wParam contains the reason:

        SIZE_MAXIMIZED The window has been maximized. SIZE_MINIMIZED The window has
       been minimized. SIZE_RESTORED The window has been resized, but neither the
       SIZE_MINIMIZED nor SIZE_MAXIMIZED value applies.
        /*/
    if (nType == SIZE_MINIMIZED) {
        // restorePlugWindowPositions();
    }
}

LRESULT CDspAudioHostDlg::OnThemeChanged() {
    /*/
    if (g_darkModeSupported) {
        _AllowDarkModeForWindow(*this, g_darkModeEnabled);
        RefreshTitleBarThemeColor(*this);

        CWnd* pwndChild = GetWindow(GW_CHILD);
        while (pwndChild) {
            _AllowDarkModeForWindow(*pwndChild, g_darkModeEnabled);
            ::SendMessageW(*pwndChild, WM_THEMECHANGED, 0, 0);
            wchar_t buf[MAX_PATH] = {0};
            ::GetWindowText(*pwndChild, buf, MAX_PATH);
            TRACE(L"setting dark mode (%d) to window: %s\n", (int)g_darkModeEnabled, buf);
            pwndChild = pwndChild->GetWindow(GW_HWNDNEXT);
        }

        UpdateWindow();
    }
    /*/
    return 1;
}

#define WM_WININICHANGE 0x001A
#define WM_SETTINGCHANGE WM_WININICHANGE

BOOL CDspAudioHostDlg::OnWndMsg(
    UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult) {

    return __super::OnWndMsg(message, wParam, lParam, pResult);
}

void CDspAudioHostDlg::OnBnClickedBtnConfigAudio() {
    ::WinExec("rundll32.exe shell32.dll,Control_RunDLL mmsys.cpl,,0", SW_SHOWNORMAL);
}

void CDspAudioHostDlg::OnCancel() {
    // TODO: Add your specialized code here and/or call the base class

    //__super::OnCancel();
    // nope, don't want to close window on enter key, thanks
}

void CDspAudioHostDlg::OnOK() {
    // TODO: Add your specialized code here and/or call the base class

    // __super::OnOK();
    // nope, don't want to close window on enter key, thanks
}
