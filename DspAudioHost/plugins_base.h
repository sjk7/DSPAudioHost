#pragma once
// plugins_base.h
#include "../portaudio/portaudio_wrapper.h"
#include <string>
#include <string_view>
#include <libloaderapi.h>

static inline int filter_msvc_exceptions(
    unsigned int code, struct _EXCEPTION_POINTERS* ep) {

    if (code == EXCEPTION_ACCESS_VIOLATION) {
        return EXCEPTION_EXECUTE_HANDLER;
    }

    else {
        return EXCEPTION_CONTINUE_SEARCH;
    };
}

namespace strings {

static inline size_t catSize(std::string_view sv) {
    return sv.size();
}

template <typename... Args>
static inline size_t catSize(std::string_view const first, Args&&... args) {
    return catSize(first) + catSize(std::forward<Args>(args)...);
}
template <typename... Args>
static inline std::string const& concatenate(std::string& s, Args&&... args) {
    static_assert((std::is_constructible_v<std::string_view, Args&&> && ...));

    // super cheap concatenation,
    // sometimes without allocation
    size_t const reserve_size = (catSize(s, std::forward<Args>(args)...));

    // size_t reserve_size = 1024;
    if (reserve_size > s.capacity()) s.reserve(reserve_size);

    (s.append(std::forward<Args>(args)), ...);

    return s;
}

} // namespace strings

struct plugins_base {

    enum class PlugType { Winamp_dsp, VST };

    struct base_data {
        std::string filepath;
        std::string errorString;
        std::string description;
        int lastError = 0;
        HWND parent = 0;
        int init_result = -1; // zero or positive for activation success
        bool for_enumeration_only{true};
        mutable HMODULE hinst = nullptr;
        volatile bool is_running{false}; // this *IS* thread safe  in doze
    };
    mutable base_data m_base_data;

    plugins_base() : m_base_data() {}
    plugins_base(const plugins_base& other) : m_base_data(other.m_base_data) {
        other.m_base_data.hinst = nullptr;
    }

    plugins_base(plugins_base&& other) noexcept : m_base_data() {
        std::swap(m_base_data, other.m_base_data);
        assert(other.hinstance() == nullptr);
    }

    plugins_base& operator=(const plugins_base& other) {
        this->m_base_data.filepath = other.m_base_data.filepath;
        return *this;
    }

    plugins_base& operator=(plugins_base&& other) noexcept {
        m_base_data = std::move(other.m_base_data);
        other.m_base_data = base_data();
        assert(other.hinstance() == nullptr);
        return *this;
    }

    virtual ~plugins_base() { base_clean(); }

    const std::string& errorString() const noexcept { return m_base_data.errorString; }
    const std::string& filePath() const noexcept { return m_base_data.filepath; }
    const std::string& description() const noexcept { return m_base_data.description; }
    int lastError() const noexcept { return m_base_data.lastError; }
    HMODULE hinstance() const noexcept { return m_base_data.hinst; }

    virtual int doDSP(void* const buf, const int frameCount,
        const portaudio_cpp::SampleRateType& sampleRate = 44100,
        const portaudio_cpp::ChannelsType& nch
        = portaudio_cpp::ChannelsType::MonoStereo::stereo,
        const int bps = 16) const
        = 0;

    virtual bool has_header() const noexcept = 0;
    virtual PlugType getType() const noexcept = 0; // enforce plugtype in plug
    virtual int showConfig() const = 0;
    virtual const std::wstring& configHandleWindowText() const = 0;
    virtual HWND configHandle() const noexcept = 0;

    protected:
    // protected functions you must override:
    virtual void activate_guarded() const = 0;

    virtual void hinstanceSet(HMODULE mod) { m_base_data.hinst = mod; }

    template <typename... Args> void errorSet(int e, Args&&... args) {
        std::string info
            = strings::concatenate(m_base_data.errorString, std::forward<Args>(args)...);
        assert(!info.empty());
        std::string what(this->filePath());
        info = strings::concatenate(what, std::string("\n"), info);
        assert(!info.empty());
        m_base_data.errorString = std::move(info);
        m_base_data.lastError = e;
    }

    template <typename... Args> void throw_error(Args&&... args) {
        auto info
            = strings::concatenate(m_base_data.errorString, std::forward<Args>(args)...);
        if (info.empty()) {
            assert(!m_base_data.errorString.empty());
            throw std::runtime_error(m_base_data.errorString);
        }
        throw std::runtime_error(info.data());
    }

    void errorSet(std::string_view s, int e) const noexcept {
        std::string msg("Plugin at: ");
        std::string p(this->filePath());
        msg += p;
        msg += "\n\n";
        if (has_header()) {
            msg += m_base_data.description;
            msg += ":\n\n";
        }
        msg += s;

        m_base_data.errorString = std::move(msg);
        m_base_data.lastError = e;
    }
    void errorClear() {
        m_base_data.errorString.clear();
        m_base_data.lastError = 0;
    }

    void base_clean() {

        if (m_base_data.hinst) {
            FreeLibrary(m_base_data.hinst);
            m_base_data.hinst = nullptr;
        }

        m_base_data.init_result = -1;
    }
};
