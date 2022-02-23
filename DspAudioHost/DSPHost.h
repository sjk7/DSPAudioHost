
// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#pragma once

#include "../Winamp_SDK/DSP.H"
#include <string_view>
#include <afx.h>
#include <vector>
#include <cassert>
#include <string_view>

// DSPHost.h

namespace winamp_dsp {

namespace my {
    namespace strings {
        using split_result_t = std::vector<std::string>;

        static inline split_result_t split(std::string const& str, const char delim) {
            split_result_t retval;
            size_t start;
            size_t end = 0;

            while ((start = str.find_first_not_of(delim, end)) != std::string::npos) {
                end = str.find(delim, start);
                retval.emplace_back(std::move(str.substr(start, end - start)));
            }

            // for the case when there are no delimiters:
            if (retval.empty()) {
                if (!str.empty()) {
                    retval.push_back(str);
                }
            }

            return retval;
        }

    } // namespace strings
} // namespace my

static inline int filter_msvc_exceptions(
    unsigned int code, struct _EXCEPTION_POINTERS* ep) {

    if (code == EXCEPTION_ACCESS_VIOLATION) {
        return EXCEPTION_EXECUTE_HANDLER;
    }

    else {
        return EXCEPTION_CONTINUE_SEARCH;
    };
}
class Plugin {
    friend struct PlugEnumerator;
    struct data {
        std::string filepath;

        std::string errorString;
        std::string description;
        int lastError = 0;
        HWND parent = 0;
        int init_result = -1; // zero or positive for activation success
        bool has_header() const noexcept { return m_header != nullptr; }
        HMODULE hinstance() const noexcept { return hinst; }
        void hinstance(HMODULE mod) { hinst = mod; }
        void header_set(winampDSPHeader* p) { m_header = p; }
        winampDSPHeader* header() { return m_header; }
        winampDSPModule* module() const noexcept { return m_module; }
        void module_set(winampDSPModule* p) { m_module = p; };
        bool for_enumeration_only{true};
        void configHandleWindowTextSet(const std::wstring& s) {
            m_config_window_text = s;
        }
        const std::wstring& configHandleWindowText() const {
            return m_config_window_text;
        }

        void configHandleWindowSet(HWND hwnd) { m_config_hwnd = hwnd; }
        HWND configHandleWindowGet() const noexcept { return m_config_hwnd; }

        private:
        HMODULE hinst = 0;
        winampDSPHeader* m_header = nullptr;
        winampDSPModule* m_module = nullptr;
        std::wstring m_config_window_text;
        HWND m_config_hwnd = nullptr;
    };
    data m_data;

    void quitModule(winampDSPModule* mod) {
        assert(mod);

        __try {
            mod->Quit(mod);
        } __except (
            filter_msvc_exceptions(GetExceptionCode(), GetExceptionInformation())) {
            TRACE("module threw exception when Quit() called. Meh\n");
        }
    }

    void configModule(winampDSPModule* mod) {
        assert(mod);
        __try {
            mod->Config(mod);
        } __except (
            filter_msvc_exceptions(GetExceptionCode(), GetExceptionInformation())) {
            this->errorSet("Threw exception when Config() called", -1);
            throw std::runtime_error(this->m_data.errorString);
        }
    }

    void errorSet(std::string_view s, int e) {
        std::string msg("Plugin at: ");
        std::string p(this->filepath());
        msg += p;
        msg += "\n\n";
        if (m_data.has_header()) {
            msg += m_data.description;
            msg += ":\n\n";
        }
        msg += s;

        m_data.errorString = msg;
        m_data.lastError = e;
    }
    void errorClear() {
        m_data.errorString.clear();
        m_data.lastError = 0;
    }

    static inline void create_guarded(
        const bool for_enumeration_only, Plugin& plug, HWND parent) {
        assert(!plug.m_data.filepath.empty());
        __try {
            _create(for_enumeration_only, plug, parent);
            return;
        }

        __except (filter_msvc_exceptions(GetExceptionCode(), GetExceptionInformation())) {
            plug.errorSet("Fatal error when loading", -2);
        }
        return;
    }
    static inline void _create(
        bool for_enumeration_only, Plugin& plug, HWND parent = ::GetDesktopWindow()) {

        std::string_view filepath = plug.m_data.filepath;
        assert(!filepath.empty());
        plug.m_data.parent = parent;
        plug.m_data.for_enumeration_only = for_enumeration_only;
        plug.m_data.hinstance(::LoadLibraryA(plug.m_data.filepath.data()));
        if (!plug.m_data.hinstance() || (int)plug.m_data.hinstance() == -1) {
            plug.errorSet("LoadLibrary failed", GetLastError());
            return;
        }

        auto headertype = (winampDSPGetHeaderType)GetProcAddress(
            plug.m_data.hinstance(), "winampDSPGetHeader2");

        if (!headertype) {
            headertype = (winampDSPGetHeaderType)GetProcAddress(
                plug.m_data.hinstance(), "SPLGetDSPHeader2");
            if (!headertype) {
                plug.errorSet("No DSP dll header found", GetLastError());
                return;
            }
        }

        plug.m_data.header_set(headertype());

        if (plug.m_data.header()->version > DSP_HDRVER) {
            plug.errorSet("Header version too great", -1);
            return;
        }
        plug.m_data.description = plug.m_data.header()->description;

        if (for_enumeration_only) {
            // when we are just getting stuff to show it as an available plugin,
            // do not keep the dll "open", otherwise you cannot delete unwanted plugs
            // in explorer whilst the app is running -- even if the plug is not activated!
            plug.free_internal_structures();
            return;
        }

        plug.m_data.module_set(plug.m_data.header()->getModule(0));

        if (!plug.m_data.module()) {
            plug.errorSet("Unable to get dsp module", -1);
            return;
        }

        plug.m_data.module()->hwndParent = parent;
        plug.m_data.module()->hDllInstance = plug.m_data.hinstance();

        return;
    }

    // can throw std::runtime_error
    // Returns whatever the plugin replied in response to being activated
    int do_activate() {
        if (m_data.init_result >= 0) {
            // already active!
            return m_data.init_result;
        }
        assert(!filepath().empty() && m_data.hinstance());
        if (filepath().empty()) {
            throw std::runtime_error("cannot activate a plug with no filepath");
        }
        if (!m_data.module() || !m_data.header()) {
            throw std::runtime_error(
                "plugin cannot be activated if it is not initialized first.");
        }

        EXCEPTION_POINTERS* xp = nullptr;

        __try {

            m_data.init_result = m_data.module()->Init(m_data.module());
        } __except (
            filter_msvc_exceptions(GetExceptionCode(), xp = GetExceptionInformation())) {

            if (m_data.module()) {
                this->quitModule(this->m_data.module());
            }
            errorSet("module threw exception when Init() called.", -2000);
            throw std::runtime_error(this->m_data.errorString);
        }

        const auto desc = description();
        if (desc == "AudioEnhance Windows Media Encoders Plugin for Winamp") {
            if (m_data.init_result != 0) {
                m_data.init_result = -1000;
                errorSet("Only one instance of this plug is allowed per process", -1000);
                if (m_data.module()) {
                    this->quitModule(this->m_data.module());
                }
                throw std::runtime_error(m_data.errorString);
            }
        }
        return m_data.init_result; // may be zero or 1 for success
    }

#ifdef DEBUG
    void test_move_assign_copy(std::string_view fp) {
        Plugin plug(false, fp);
        Plugin copied = plug;
        assert(plug.filepath() == copied.filepath());
        assert(plug.m_data.hinstance());
        assert(copied.m_data.hinstance());

        Plugin moved_into = std::move(plug);
        assert(moved_into.m_data.hinstance());
        assert(!plug.m_data.hinstance());

        Plugin moved_constructed(std::move(moved_into));
        assert(moved_constructed.m_data.hinstance());
        assert(!moved_into.m_data.hinstance());

        Plugin constructed_from_copy(copied);
        assert(constructed_from_copy.filepath() == copied.filepath());
        assert(constructed_from_copy.m_data.hinstance());
        assert(copied.m_data.hinstance());

        plug = copied;
        assert(plug.filepath() == copied.filepath());
        assert(plug.m_data.hinstance() && copied.m_data.hinstance());

        Plugin p(false, fp);
        plug = std::move(p);

        assert(plug.m_data.hinstance() && !p.m_data.hinstance());
    }

#endif

    public:
    Plugin() {}
    Plugin(const bool for_enumeration_only, std::string_view path, HWND parent = nullptr)
        : m_data() {
        m_data.filepath = path;
        create_guarded(for_enumeration_only, *this, parent);
#ifdef _DEBUG
        static bool tested = false;
        if (!tested) {
            tested = true;
            test_move_assign_copy(path);
        }

#endif
    }

    void configHandleWindowTextSet(const std::wstring& h) {
        m_data.configHandleWindowTextSet(h);
    }

    void configHandleWindowSet(HWND h) { m_data.configHandleWindowSet(h); }
    HWND configHandleWindowGet() const noexcept { return m_data.configHandleWindowGet(); }

    auto& configHandleWindowText() const { return m_data.configHandleWindowText(); }

    public:
    Plugin(const Plugin& other) : m_data() {
        m_data.filepath = other.filepath();
        create_guarded(other.m_data.for_enumeration_only, *this, other.m_data.parent);
    }

    Plugin(Plugin&& other) noexcept : m_data() {
        std::swap(m_data, other.m_data);
        assert(other.m_data.hinstance() == nullptr);
    }

    Plugin& operator=(const Plugin& other) {
        m_data.filepath = other.m_data.filepath;
        create_guarded(other.m_data.for_enumeration_only, *this, other.m_data.parent);
        return *this;
    }

    Plugin& operator=(Plugin&& other) noexcept {
        m_data = std::move(other.m_data);
        other.m_data = data();
        assert(other.m_data.hinstance() == nullptr);
        return *this;
    }

    int activate(Plugin& plug, Plugin& activated) {
        if (m_data.for_enumeration_only) {
            m_data.for_enumeration_only = false;
        }
        create_guarded(false, plug, m_data.parent);
        activated = plug;

        return activated.do_activate();
    }

    void free_internal_structures() {
        if (m_data.init_result >= 0) {
            if (m_data.module()) {
                quitModule(m_data.module());
                m_data.module_set(nullptr);
            }
        }
        if (m_data.hinstance()) {
            FreeLibrary(m_data.hinstance());
        }
        m_data.header_set(nullptr);
        m_data.hinstance(nullptr);
        m_data.init_result = -1;
    }

    ~Plugin() { free_internal_structures(); }

    int doDSP(short* const buf, const int frameCount,
        const portaudio_cpp::SampleRateType& sampleRate = 44100, const int nch = 2,
        const int bps = 16) const {
        assert(bps == 16); // winamp plugs can typically handle only 16-bit audio

        auto mod = m_data.module();
        int nsamps = mod->ModifySamples(
            mod, (short* const)buf, frameCount, bps, nch, sampleRate.m_value);
        assert(nsamps == frameCount); // we really don't want to be holding on to
                                      // data for real-time stuff
        return nsamps;
    }

    // might throw std::runtime_error
    int showConfig() {
        if (m_data.init_result < 0) {
            errorSet("Plugins must be activated before show()ing config window", -1);
            throw std::runtime_error(
                "Plugins must be activated before show()ing config window");
        }

        if (!m_data.module()) {
            errorSet("Unexpected: Plugin module missing", -1);
            throw std::runtime_error("Plugin module missing");
        }
        configModule(m_data.module());
        return 0;
    }

    std::string_view description() const noexcept { return m_data.description; }

    std::string_view filepath() const noexcept { return m_data.filepath; }
};

using plugins_type = std::vector<Plugin>;

struct PlugEnumerator {
    std::string rootFolder;
    plugins_type m_plugins;
    HWND m_parentHWND = nullptr;

    void findAvailable(std::string_view root) {
        m_plugins = doEnumAvailable(root);
        return;
    }
    void clear() { m_plugins.clear(); }
    const auto& plugins() const { return m_plugins; }

    Plugin* findPlug(std::string_view description) {
        for (auto& p : m_plugins) {
            if (p.description() == description) {
                return &p;
            }
        }
        return nullptr;
    }

    private:
    plugins_type doEnumAvailable(std::string_view root) {

        plugins_type retval;
        CFileFind finder;
        CString param(root.data());
        param += L"\\dsp_*.dll";

        BOOL bOk = finder.FindFile(param);
        CString fp;

        while (bOk) {
            bOk = finder.FindNextFile();

            if (!finder.IsDirectory() || !finder.IsDots()) {
                fp = finder.GetFilePath();
                CStringA nPath(fp);
                Plugin plug(true, nPath.GetBuffer(), m_parentHWND);
                if (plug.m_data.lastError == 0) {
                    retval.emplace_back(std::move(plug));
                }
            }
        };
        return retval;
    }
};

struct Host {

    using active_plugins_type = std::vector<Plugin>;
    PlugEnumerator m_enumerator;
    active_plugins_type m_activePlugins;

    Host() = default;
    Host(const Host&) = delete;
    Host(Host&&) = delete;
    void setParent(HWND parent) { m_enumerator.m_parentHWND = parent; }

    HWND Parent() const noexcept { return m_enumerator.m_parentHWND; }

    const auto& availablePlugins() const noexcept { return m_enumerator.m_plugins; }

    int m_lastError = 0;

    std::string m_errorString;

    const plugins_type& plugins() const noexcept { return m_enumerator.m_plugins; }
    const auto& activePlugins() const noexcept { return m_activePlugins; }

    void setFolder(std::string_view newFolder) { m_enumerator.findAvailable(newFolder); }

    std::string getFolder() { return m_enumerator.rootFolder; }

    void clearActivePlugs() { m_activePlugins = active_plugins_type(); }
    void clearEnumeratedPlugs() { m_enumerator.clear(); }
    void clearAll() {
        clearActivePlugs();
        clearEnumeratedPlugs();
    }

    Plugin* findPlug(std::string_view description) {
        return m_enumerator.findPlug(description);
    }

    Plugin* findActivatedPlugByDesc(std::string_view desc) {
        for (auto& p : m_activePlugins) {
            if (p.description() == desc) {
                return &p;
            }
        }
        return nullptr;
    }

    // can throw std::runtime_error
    Plugin& activatePlug(Plugin& plug) {
        Plugin activated;
        plug.activate(plug, activated);
        m_activePlugins.push_back(std::move(activated));
        return m_activePlugins.at(m_activePlugins.size() - 1);
    }

#pragma warning(disable : 4130)
    Plugin* findActivatedPlug(const unsigned int index) {
        if (index >= m_activePlugins.size()) {
            assert("findActivatedPlug: index out of bounds" == nullptr); //-V547
            return nullptr;
        }
        return &m_activePlugins.at(index);
    }

    // note: do NOT access plug after this has been called!
    bool removeActivatedPlug(const Plugin* plug) {

        auto it = std::remove_if(m_activePlugins.begin(), m_activePlugins.end(),
            [&](auto& p) { return &p == plug; });

        if (it != m_activePlugins.end()) {
            m_activePlugins.erase(it);
            return true;
        }
        return false;
    }

    static inline constexpr const char* PLUG_SEP = "\x2";

    std::string getActivePlugsAsString() const noexcept {
        std::string ret;
        for (const auto& p : m_activePlugins) {
            ret += p.description();
            ret += PLUG_SEP;
        }

        return ret;
    }

    void swapPlugins(int indexA, int indexB) {
        // lock?
        std::swap(m_activePlugins[indexA], m_activePlugins[indexB]);
    }
};

} // namespace winamp_dsp
