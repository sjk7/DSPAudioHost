
// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#pragma once

#include "../Winamp_SDK/DSP.H"
#include <string_view>
#include <afx.h>
#include <vector>
#include <cassert>
#include <string_view>
#include "plugins_base.h"

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
                retval.emplace_back(str.substr(start, end - start));
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

class Plugin : public plugins_base {

    friend struct PlugEnumerator;
    struct data {

        bool has_header() const noexcept { return m_header != nullptr; }
        bool has_module() const noexcept { return m_module != nullptr; }
        void header_set(winampDSPHeader* p) const { m_header = p; }
        void module_set(winampDSPModule* mod) const { m_module = mod; }
        winampDSPHeader* header() { return m_header; }
        winampDSPModule* module() const noexcept { return m_module; }
        void module_set(winampDSPModule* p) { m_module = p; };

        void configHandleWindowTextSet(const std::wstring& s) {
            m_config_window_text = s;
        }
        const std::wstring& configHandleWindowText() const {
            return m_config_window_text;
        }

        void configHandleWindowSet(HWND hwnd) { m_config_hwnd = hwnd; }
        HWND configHandleWindowGet() const noexcept { return m_config_hwnd; }

        private:
        mutable winampDSPHeader* m_header = nullptr;
        mutable winampDSPModule* m_module = nullptr;
        std::wstring m_config_window_text;
        HWND m_config_hwnd = nullptr;
    };
    data m_data;

    // HMODULE hinstance() const noexcept { return plugins_base::hinstance(); }
    bool has_header() const noexcept override { return m_data.has_header(); }
    bool has_module() const noexcept { return m_data.has_module(); }
    void quitModule(winampDSPModule* mod) const {
        assert(mod);

        __try {
            mod->Quit(mod);
        } __except (
            filter_msvc_exceptions(GetExceptionCode(), GetExceptionInformation())) {
            TRACE("module threw exception when Quit() called. Meh\n");
        }
    }

    void configModule(winampDSPModule* mod) const {
        assert(mod);
        __try {
            mod->Config(mod);
        } __except (
            filter_msvc_exceptions(GetExceptionCode(), GetExceptionInformation())) {
            this->errorSet("Threw exception when Config() called", -1);
            throw std::runtime_error(this->errorString());
        }
    }

    static inline void create_guarded(
        const bool for_enumeration_only, Plugin& plug, HWND parent) {
        assert(!plug.filePath().empty());
        __try {
            _create(for_enumeration_only, plug, parent);
            return;
        }

        __except (filter_msvc_exceptions(GetExceptionCode(), GetExceptionInformation())) {
            plug.errorSet("Fatal error when loading", -2);
        }
        return;
    }

    inline friend void _create(
        bool for_enumeration_only, Plugin& plug, HWND parent = ::GetDesktopWindow()) {

        std::string_view filepath = plug.filePath();
        assert(!filepath.empty());
        plug.m_base_data.parent = parent;
        plug.m_base_data.for_enumeration_only = for_enumeration_only;
        TRACE("--------------------------------------------------------------------\n");
        TRACE("Loading dsp plugin: %s\n", plug.m_base_data.filepath.data());
        if (plug.hinstance() == nullptr || (int)plug.hinstance() == -1) {
            plug.hinstanceSet(::LoadLibraryA(plug.m_base_data.filepath.data()));

            if (!plug.hinstance() || (int)plug.hinstance() == -1) {
                plug.errorSet("LoadLibrary failed", GetLastError());
                return;
            }
        }

        if (!plug.has_header()) {
            auto headertype = (winampDSPGetHeaderType)GetProcAddress(
                plug.m_base_data.hinst, "winampDSPGetHeader2");

            if (!headertype) {
                headertype = (winampDSPGetHeaderType)GetProcAddress(
                    plug.hinstance(), "SPLGetDSPHeader2");
                if (!headertype) {
                    plug.errorSet("No DSP dll header found", GetLastError());
                    return;
                }
            }

            plug.m_data.header_set(headertype());
        }

        assert(plug.has_header());

        if (plug.m_data.header()->version > DSP_HDRVER) { //-V807
            plug.errorSet("Header version too great", -1);
            return;
        }
        plug.m_base_data.description = plug.m_data.header()->description;
        TRACE("--------------------------------------------------------------------\n\n");
        if (for_enumeration_only) {
            // when we are just getting stuff to show it as an available plugin,
            // do not keep the dll "open", otherwise you cannot delete unwanted plugs
            // in explorer whilst the app is running -- even if the plug is not activated!
            plug.close(); // ignore clang warning about bypasses
                          // dispatch
            assert(!plug.has_header());
            assert(plug.m_base_data.hinst == nullptr);
            return;
        }

        plug.m_data.module_set(plug.m_data.header()->getModule(0));

        if (!plug.m_data.module()) {
            plug.errorSet("Unable to get dsp module", -1);
            return;
        }

        plug.m_data.module()->hwndParent = parent;
        plug.m_data.module()->hDllInstance = plug.hinstance();

        return;
    }

    virtual void activate_guarded() const override {

        EXCEPTION_POINTERS* xp = nullptr;
        __try {

            m_base_data.init_result = m_data.module()->Init(m_data.module());
        } __except (
            filter_msvc_exceptions(GetExceptionCode(), xp = GetExceptionInformation())) {

            if (m_data.module()) {
                this->quitModule(this->m_data.module());
            }
            errorSet("module threw exception when Init() called.", -2000);
            throw std::runtime_error(this->errorString());
        }
    }

    // can throw std::runtime_error
    // Returns whatever the plugin replied in response to being activated
    int do_activate() {
        if (m_base_data.init_result >= 0) {
            // already active!
            return m_base_data.init_result;
        }
        assert(!filePath().empty() && hinstance());
        if (filePath().empty()) {
            throw std::runtime_error("cannot activate a plug with no filepath");
        }
        if (!m_data.module() || !m_data.header()) {
            throw std::runtime_error(
                "plugin cannot be activated if it is not initialized first.");
        }

        activate_guarded();

        const auto desc = this->description();
        if (desc == "AudioEnhance Windows Media Encoders Plugin for Winamp") {
            if (m_base_data.init_result != 0) {
                m_base_data.init_result = -1000;
                errorSet("Only one instance of this plug is allowed per process", -1000);
                if (m_data.module()) {
                    this->quitModule(this->m_data.module());
                }
                throw std::runtime_error(this->errorString());
            }
        }
        return m_base_data.init_result; // may be zero or 1 for success
    }

#ifdef DEBUG
#pragma warning(disable : 26800) // moved from, I know! I know!
    void test_move_assign_copy(std::string_view fp) {
        Plugin plug(false, fp);
        const std::string filePath = plug.filePath();
        const std::string desc = plug.description();

        assert(plug.has_header());
        assert(plug.has_module());

        Plugin copied = plug;
        assert(plug.filePath() == fp);
        assert(!plug.hinstance());
        assert(copied.hinstance());
        assert(!plug.has_header());
        assert(!plug.has_module());
        assert(copied.has_header());
        assert(copied.has_module());

        Plugin moved_into = std::move(copied);
        assert(moved_into.hinstance());
        assert(!plug.hinstance());
        assert(moved_into.description() == desc);
        assert(moved_into.filePath() == filePath);
        assert(!copied.has_header());
        assert(!copied.has_module());
        assert(moved_into.has_header());
        assert(moved_into.has_module());

        Plugin moved_constructed(std::move(moved_into));
        assert(moved_constructed.hinstance());
        assert(!moved_into.hinstance());
        assert(moved_constructed.description() == desc);
        assert(moved_constructed.filePath() == filePath);
        assert(!moved_into.has_header());
        assert(!moved_into.has_module());
        assert(moved_constructed.has_header());
        assert(moved_constructed.has_module());

        Plugin constructed_from_copy(moved_constructed);
        assert(constructed_from_copy.filePath() == filePath);
        assert(constructed_from_copy.hinstance());
        assert(!moved_constructed.has_header());
        assert(!moved_constructed.m_data.has_module());
        // keeps the filename but gives up ownership of hinstance
        assert(constructed_from_copy.hinstance());
        assert(constructed_from_copy.has_header());
        assert(constructed_from_copy.has_module());

        plug = constructed_from_copy;
        assert(plug.filePath() == filePath);
        assert(plug.hinstance() && !copied.hinstance());

        Plugin p(false, fp);
        plug = std::move(p);
        assert(plug.has_header());
        assert(plug.has_module());
        assert(!p.has_header());
        assert(!p.has_module());
        assert(plug.filePath() == filePath);
        assert(plug.hinstance() && !copied.hinstance());
        assert(plug.hinstance() && !p.hinstance());
    }
#pragma warning(default : 26800)

#endif

    void close() { free_internal_structures(); }

    public:
    Plugin() {}
    Plugin(const bool for_enumeration_only, std::string_view path, HWND parent = nullptr)
        : m_data() {
        m_base_data.filepath = path;
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
    virtual HWND configHandle() const noexcept override {
        return m_data.configHandleWindowGet();
    }

    const std::wstring& configHandleWindowText() const override {
        return m_data.configHandleWindowText();
    }

    public:
    Plugin(const Plugin& other) : plugins_base(other), m_data() {

        create_guarded(
            other.m_base_data.for_enumeration_only, *this, other.m_base_data.parent);
        other.m_data.header_set(nullptr);
        other.m_data.module_set(nullptr);
    }

    Plugin(Plugin&& other) noexcept : plugins_base(std::move(other)), m_data() {
        std::swap(m_data, other.m_data);
        assert(other.hinstance() == nullptr);
    }

    Plugin& operator=(const Plugin& other) {
        plugins_base::operator=(other);
        create_guarded(
            other.m_base_data.for_enumeration_only, *this, other.m_base_data.parent);
        return *this;
    }

    Plugin& operator=(Plugin&& other) noexcept {
        plugins_base::operator=(std::move(other));
        m_data = std::move(other.m_data);

        other.m_data = data();

        assert(other.hinstance() == nullptr);
        return *this;
    }

    int activate(Plugin& plug, Plugin& activated) {
        if (m_base_data.for_enumeration_only) {
            m_base_data.for_enumeration_only = false;
        }
        create_guarded(false, plug, m_base_data.parent);
        activated = plug;

        return activated.do_activate();
    }

    void free_internal_structures() {
        if (m_base_data.init_result >= 0) {
            if (m_data.module()) {
                quitModule(m_data.module());
                m_data.module_set(nullptr);
            }
        }
        m_data.header_set(nullptr);
        this->base_clean();
    }

    virtual ~Plugin() { this->free_internal_structures(); }
    virtual plugins_base::PlugType getType() const noexcept override {
        return plugins_base::PlugType::Winamp_dsp;
    }

    virtual int doDSP(void* const buf, const int frameCount,
        const portaudio_cpp::SampleRateType& sampleRate = 44100,
        const portaudio_cpp::ChannelsType& nch
        = portaudio_cpp::ChannelsType::MonoStereo::stereo,
        const int bps = 16) const override {
        assert(bps == 16); // winamp plugs can typically handle only 16-bit audio

        auto mod = m_data.module();
        int nsamps = mod->ModifySamples(
            mod, (short* const)buf, frameCount, bps, nch.m_value, sampleRate.m_value);
        assert(nsamps == frameCount); // we really don't want to be holding on to
                                      // data for real-time stuff
        return nsamps;
    }

    // might throw std::runtime_error
    virtual int showConfig() const override {
        if (m_base_data.init_result < 0) {
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
                if (plug.lastError() == 0) {
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
        return m_activePlugins.emplace_back(std::move(activated));
    }

#pragma warning(disable : 4130)
    Plugin* findActivatedPlug(const unsigned int index) {
        if (index >= m_activePlugins.size()) {
            assert("findActivatedPlug: index out of bounds" == nullptr); //-V547
            return nullptr;
        }
        return &m_activePlugins[index];
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

    void swapPlugins(int indexA, int indexB) {
        // lock?
        using namespace std;
        swap(m_activePlugins[indexA], m_activePlugins[indexB]);
    }
};

} // namespace winamp_dsp
