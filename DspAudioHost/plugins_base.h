#pragma once
// plugins_base.h
#include "../portaudio/portaudio_wrapper.h"
#include <string>
#include <string_view>

struct plugins_base {

    struct base_data {
        std::string filepath;
        std::string errorString;
        std::string description;
        int lastError = 0;
        HWND parent = 0;
        int init_result = -1; // zero or positive for activation success
        bool for_enumeration_only{true};
        HMODULE hinst = 0;
    };
    base_data m_base_data;

    plugins_base() : m_base_data() {}
    plugins_base(const plugins_base& other) : m_base_data() {
        // std::swap(m_base_data, other.m_base_data);
        m_base_data.filepath = other.filePath();
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

    virtual ~plugins_base() {

        if (m_base_data.hinst) {
            FreeLibrary(m_base_data.hinst);
            m_base_data.hinst = nullptr;
        }

        m_base_data.init_result = -1;
    }

    const std::string& errorString() const noexcept { return m_base_data.errorString; }
    const std::string& filePath() const noexcept { return m_base_data.filepath; }
    const std::string& description() const noexcept { return m_base_data.description; }
    int lastError() const noexcept { return m_base_data.lastError; }
    HMODULE hinstance() const noexcept { return m_base_data.hinst; }

    virtual int doDSP(void* const buf, const int frameCount,
        const portaudio_cpp::SampleRateType& sampleRate = 44100,
        const portaudio_cpp::ChannelsType nch
        = portaudio_cpp::ChannelsType::MonoStereo::stereo,
        const int bps = 16) const = 0;

    // public functions you must override:
    virtual bool has_header() const noexcept = 0;

    protected:
    // protected functions you must override:
    virtual void activate_guarded() = 0;
    virtual void free_internal_structures() = 0;

    virtual void hinstanceSet(HMODULE mod) { m_base_data.hinst = mod; }
    void errorSet(std::string_view s, int e) {
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
};
