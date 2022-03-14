// This is an independent project of an individual developer. Dear PVS-Studio, please
// check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#pragma once
// portaudio wrapper
#include "include/portaudio.h"
#include "../Helpers/DSPFilters/shared/DSPFilters/include/DspFilters/Utilities.h" //EnvelopeFollower

#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>
#include <array>
#include <mmsystem.h>
#include <cassert>
#include <atomic>
#include <set>
#include <cmath>
#include <unordered_map>

#pragma warning(disable : 26812)

inline void myPaInitCallback(char* info);
static inline int filter_msvc_exceptions2(
    unsigned int code, struct _EXCEPTION_POINTERS* ep) {

    if (code == EXCEPTION_ACCESS_VIOLATION) {
        return EXCEPTION_EXECUTE_HANDLER;
    }

    else {
        return EXCEPTION_CONTINUE_SEARCH;
    };
}

namespace portaudio_cpp {

static const double TINY = double(0.000000001);

struct SampleFormat {

    SampleFormat(unsigned int fmt) : m_fmt(fmt) {}
    SampleFormat() : m_fmt(paFloat32) {}
    SampleFormat(const SampleFormat& rhs) = default;
    SampleFormat& operator=(const SampleFormat& rhs) = default;
    SampleFormat(SampleFormat&& rhs) = default;
    SampleFormat& operator=(SampleFormat&& rhs) = default;
    ~SampleFormat() = default;

    static constexpr auto Float32 = paFloat32; /**< @see PaSampleFormat */
    static constexpr auto Int32 = paInt32;
    static constexpr auto Int24 = paInt24;
    static constexpr auto Int16 = paInt16;
    static constexpr auto Int8 = paInt8;
    static constexpr auto UInt8 = paUInt8;
    static constexpr auto CustomFormat = paCustomFormat;
    static constexpr auto NonInterleaved = paNonInterleaved;
    unsigned int m_fmt = paFloat32;

    std::string to_string() const noexcept {
        std::string s;

        if (m_fmt & Float32) s = "32-bit float";
        if (m_fmt & Int32) s = "32-bit integer";
        if (m_fmt & Int24) s = "24-bit";
        if (m_fmt & Int16) s = "16-bit integer";
        if (m_fmt & Int8) s = "signed 8-bit integer";
        if (m_fmt & UInt8) s = "unsigned 8-bit integer";
        if (m_fmt & CustomFormat) s += " Custom format";
        if (m_fmt & NonInterleaved)
            s += " (non-interleaved)";
        else
            s += " (interleaved)";
        return s;
    }

    int bits() const noexcept {

        int maskoff = ~NonInterleaved;
        switch (m_fmt & maskoff) {
            case Float32: return 32; //-V1037
            case Int32: return 32;
            case Int24: return 24;
            case Int16: return 16;
            case Int8: return 32;
            case UInt8: return 8;
            default: {
                assert(0);
                return -1;
            }
        }
    }

    inline friend bool operator<(const SampleFormat& lhs, const SampleFormat& rhs) {
        return lhs.m_fmt < rhs.m_fmt;
    }
    inline friend bool operator==(const SampleFormat& lhs, const SampleFormat& rhs) {
        return lhs.m_fmt == rhs.m_fmt;
    }
};

// add to this collection in your class if you support other samplerates.
// Remove any samplerates you do not support.
inline auto allowed_samplerates
    = std::vector<unsigned int>{22050, 44100, 48000, 96000, 128000, 196000};

// add to this collection in your class if you support other bitdepths.
// Remove any bitdepths you do not support.
inline auto allowed_bitdepths
    = std::vector<SampleFormat>{SampleFormat(SampleFormat::Int32)};
enum class DeviceTypes { none = -1, input, output };

struct ChannelsType {
    enum MonoStereo { mono = 1, stereo = 2 };
    unsigned int m_value = MonoStereo::stereo;
    std::string to_string() const noexcept {
        std::string s("stereo");
        if (m_value == MonoStereo::mono) s = "mono";
        return s;
    }
    ChannelsType(MonoStereo ms) : m_value(ms) {}
    ChannelsType() = default;
    inline bool operator==(const ChannelsType& rhs) const noexcept {
        return m_value == rhs.m_value;
    }
};

inline PaStreamParameters prepareInOutParams(const unsigned int deviceIndex,
    const SampleFormat& fmt, const PaTime suggestedLatency, const ChannelsType& nch) {
    PaStreamParameters ip = {0};
    ip.channelCount = nch.m_value;
    ip.device = deviceIndex;
    ip.sampleFormat = fmt.m_fmt;

    ip.suggestedLatency = suggestedLatency;
    return ip;
}

// Returns a linear value.
template <typename T> static inline T db_to_value(T dB) {
    dB = -dB;
    T g = (T)std::pow(10, dB / 20);
    return g;
};

// calculate, eg how many db down 15000 is on a max of 30000 (6db, innit?)
// returns a dB value. Example call: 15000/32768  returns approx -6dB
template <typename T> static inline T db_down(const T value, const T max) noexcept {
    static_assert(std::is_floating_point<T>::value,
        "db_down should be templated on a floating point");
    T retval = 0;

    // e.g: get how many db down 15000 is on a max of 32768:
    T ratio = value / max;
    if (ratio <= 0) {
        ratio = TINY;
    }

    T d = T(20 * log10(ratio));
    retval = d;
    return retval;
};

static inline constexpr double SECSPERDAY = 86400;
static inline constexpr double SECSPERHOUR = 3600;
static inline constexpr double SECSPERMIN = 60;
static inline constexpr double MSPERSEC = 1000;

namespace time_flags {
    static constexpr int short_format = 0;
    static constexpr int verbose_format = 1;
    static constexpr int simple_format = 2;
} // namespace time_flags

static inline void timestring(bool show_millis, std::wstring& s, const uint64_t ms,
    int flags = time_flags::simple_format, wchar_t* pbuf = nullptr) {

    double seconds = (double)ms / 1000;
    uint64_t days = (uint64_t)(seconds / SECSPERDAY);
    uint64_t hours = (uint64_t)((seconds - (days * SECSPERDAY)) / SECSPERHOUR);
    uint64_t mins = (uint64_t)((seconds - (days * SECSPERDAY) - (hours * SECSPERHOUR))
        / SECSPERMIN);
    uint64_t secs = (uint64_t)seconds % (uint64_t)SECSPERMIN;
    uint64_t millis = (uint64_t)(seconds * MSPERSEC) % (uint64_t)MSPERSEC;

    s.clear();

    static wchar_t buf[1024] = {0};
    if (pbuf == nullptr) {
        pbuf = buf;
    }

    if (flags & time_flags::verbose_format) {
        wsprintf(pbuf, L"%02d days, ", (int)days);
    } else {
        if (flags & time_flags::simple_format) {
            wsprintf(pbuf, L"%02d:", (int)days);
        } else {
            wsprintf(pbuf, L"%02dd:", (int)days);
        }
    }

    s += pbuf;

    if (flags & time_flags::verbose_format) {
        wsprintf(pbuf, L"%02d hours, ", (int)hours);
    } else {
        if (flags & time_flags::simple_format) {
            wsprintf(pbuf, L"%02d:", (int)hours);
        } else {
            wsprintf(pbuf, L"%02dhh:", (int)hours);
        }
    }
    s += pbuf;

    if (flags & time_flags::verbose_format) {
        wsprintf(pbuf, L"%02d mins, ", (int)mins);
    } else {
        if (flags & time_flags::simple_format) {
            wsprintf(pbuf, L"%02d:", (int)mins);
        } else {
            wsprintf(pbuf, L"%02dm:", (int)mins);
        }
    }

    s += pbuf;

    if (flags & time_flags::verbose_format) {
        wsprintf(pbuf, L"%02d seconds ", (int)secs);
    } else {
        if (flags & time_flags::simple_format) {
            wsprintf(pbuf, L"%02d", (int)secs);
        } else {
            wsprintf(pbuf, L"%02ds", (int)secs);
        }
    }
    s += pbuf;

    if (show_millis) {
        if (flags & time_flags::verbose_format) {
            wsprintf(pbuf, L".%02d millis, ", (int)millis);
        } else {
            if (flags & time_flags::simple_format) {
                wsprintf(pbuf, L".%03d", (int)millis);
            } else {
                wsprintf(pbuf, L".%03dd", (int)millis);
            }
        }

        s += pbuf;
    }
}
struct timer {
    timer(std::string_view id) : m_id(id) { m_start = timeGetTime(); }
    ~timer() {
        m_end = timeGetTime();
        TRACE("timer, with id: %s, took: %ld ms.", m_id.c_str(), (long)(m_end - m_start));
    }

    private:
    std::string m_id;
    DWORD m_start = 0;
    DWORD m_end = 0;
};

struct SampleRateType {

    static constexpr unsigned int defaultSampleRate = 44100;
    unsigned int m_value = defaultSampleRate;
    SampleRateType(unsigned int val) : m_value(val) {}
    SampleRateType(const SampleRateType& rhs) = default;
    SampleRateType& operator=(const SampleRateType& rhs) = default;
    SampleRateType(SampleRateType&& rhs) = default;
    SampleRateType& operator=(SampleRateType&& rhs) = default;
    SampleRateType() : m_value(defaultSampleRate){};
    inline bool operator==(const SampleRateType& rhs) const noexcept {
        return m_value == rhs.m_value;
    }
    inline bool operator!=(const SampleRateType& rhs) const noexcept {
        return !(*this == rhs);
    }
    inline bool operator==(unsigned int to_what) const noexcept {
        return to_what == m_value;
    }
    inline bool operator<(unsigned int to_what) const noexcept {
        return m_value < to_what;
    }
    inline bool operator<=(unsigned int to_what) const noexcept {
        return m_value <= to_what;
    }
    inline friend bool operator<(
        const SampleRateType& lhs, const SampleRateType& rhs) noexcept {
        return lhs.m_value < rhs.m_value;
    }
    inline bool operator>(unsigned int to_what) const noexcept {
        return m_value > to_what;
    }

    std::string to_string() const noexcept { return std::to_string(m_value) + " Hz"; }
};

struct AudioFormat {
    DeviceTypes forInOrOut{DeviceTypes::input};
    SampleRateType samplerate{0};
    SampleFormat samplefmt{SampleFormat::Float32};
    ChannelsType channels;

    static AudioFormat makeAudioFormat(const DeviceTypes& forInOrOut,
        const SampleFormat& sf, const SampleRateType& sr,
        const ChannelsType& chans = ChannelsType::stereo) {
        AudioFormat ret{forInOrOut, sr, sf, chans};
        return ret;
    }

    bool is_empty() const noexcept { return samplerate <= 0; }
    inline std::string to_string() const noexcept {
        std::string s("Channels: ");
        s += channels.to_string();
        s += "\nBitdepth:";
        s += samplefmt.to_string();
        s += "\nAt Samplerate: ";
        s += std::to_string(samplerate.m_value);
        s += "\n\n";
        return s;
    }
};

static inline std::vector<SampleFormat> test_formats = {SampleFormat::Float32,
    SampleFormat::Int16, SampleFormat::Int24, SampleFormat::Int8, SampleFormat::UInt8,
    SampleFormat::Int32, SampleFormat::Int8 | SampleFormat::NonInterleaved,
    SampleFormat::UInt8 | SampleFormat::NonInterleaved,
    SampleFormat::Int16 | SampleFormat::NonInterleaved,
    SampleFormat::Int24 | SampleFormat::NonInterleaved,
    SampleFormat::Int32 | SampleFormat::NonInterleaved,
    SampleFormat::Float32 | SampleFormat::NonInterleaved};

using supported_audio_t = std::vector<AudioFormat>;

struct PortAudioGlobalIndex {
    int m_value{-1};
    inline bool operator==(const PortAudioGlobalIndex& rhs) const noexcept {
        return m_value == rhs.m_value;
    }
};
struct PaDeviceInfoEx {

    PaDeviceInfo info = {0};
    int index = -1;
    std::string extendedName; // used for the case of name clashes
    PortAudioGlobalIndex globalIndex;

    std::set<SampleRateType> getSupportedSamplerates(const DeviceTypes& dt,
        const SampleFormat& fmt = SampleFormat::Float32) const noexcept {
        std::set<SampleRateType> ret;

        const auto& fmts = get_supported_audio_types(dt);
        for (const auto& f : fmts) {
            if (f.samplefmt == fmt) {
                ret.insert(f.samplerate);
            }
        }
        return ret;
    }

    inline std::string to_string(const std::vector<AudioFormat>& fmts) const {
        std::string s("Device ");
        s += extendedName;
        s += " supports the following:\n";
        for (const auto& t : fmts) {
            s += t.to_string();
        }
        return s;
    }

#pragma warning(disable : 4130)
    PaError isFormatSupported(const AudioFormat& fmt) const noexcept {
        if (fmt.forInOrOut == DeviceTypes::input) {
            if (info.maxInputChannels == 0) {
                assert("Querying an output device for INPUT formats? WTF!"
                    == nullptr); //-V547
            }
        } else {
            if (info.maxOutputChannels == 0) {
                assert("Querying an input device for OUTPUT formats? WTF!"
                    == nullptr); //-V547
            }
        }
        if (fmt.forInOrOut == DeviceTypes::input) {
            auto ip = prepareInOutParams(index, fmt.samplefmt, 0, fmt.channels);
            return Pa_IsFormatSupported(&ip, nullptr, fmt.samplerate.m_value);
        } else {
            auto op = prepareInOutParams(index, fmt.samplefmt, 0, fmt.channels);
            return Pa_IsFormatSupported(nullptr, &op, fmt.samplerate.m_value);
        }
    }

    supported_audio_t get_supported_audio_types(const DeviceTypes& type) const noexcept {
        supported_audio_t& ret = supported_audio_types;
        ret.clear();
        supported_audio_t tmp; //-V808
        using std::begin, std::end;

        if (type == DeviceTypes::input) {
            if (info.maxInputChannels > 0) {
                for (const auto& f : test_formats) {
                    assert(f.m_fmt > 0);
                    tmp = get_supported_input(f, ChannelsType::mono);
                    ret.insert(end(ret), begin(tmp), end(tmp));
                }
                if (info.maxInputChannels >= 2) {
                    for (const auto& f : test_formats) {
                        tmp = get_supported_input(f, ChannelsType::stereo);
                        ret.insert(end(ret), begin(tmp), end(tmp));
                    }
                }
            }
        }

        if (type == DeviceTypes::output) {
            if (info.maxOutputChannels > 0) {
                for (const auto& f : test_formats) {
                    tmp = get_supported_output(f, ChannelsType::mono);
                    ret.insert(end(ret), begin(tmp), end(tmp));
                }
                if (info.maxOutputChannels >= 2) {
                    for (const auto& f : test_formats) {
                        tmp = get_supported_output(f, ChannelsType::stereo);
                        ret.insert(end(ret), begin(tmp), end(tmp));
                    }
                }
            }
        }

        return ret;
    }

    private:
    mutable supported_audio_t supported_audio_types;
    supported_audio_t get_supported_input(
        const SampleFormat& sf, const ChannelsType& nch) const noexcept {
        supported_audio_t ret;
        auto ip = prepareInOutParams(index, sf, 0, nch);
        for (const auto& sr : allowed_samplerates) {
            if (Pa_IsFormatSupported(&ip, nullptr, (double)sr) == paFormatIsSupported) {
                ret.emplace_back(AudioFormat{DeviceTypes::input, sr, sf, nch});
            }
        }
        return ret;
    }
    supported_audio_t get_supported_output(
        const SampleFormat& sf, const ChannelsType& nch) const noexcept {
        supported_audio_t ret;
        auto op = prepareInOutParams(index, sf, (PaTime)0, nch);
        for (const auto& sr : allowed_samplerates) {
            if (Pa_IsFormatSupported(nullptr, &op, (double)sr) == paFormatIsSupported) {
                ret.emplace_back(AudioFormat{DeviceTypes::output, sr, sf, nch});
            }
        }
        return ret;
    }
};

struct notification_interface {
    virtual void onApiChanged(const PaHostApiInfo& newApi) noexcept {}
    virtual void onInputDeviceChanged(const PaDeviceInfoEx& newInputDevice) noexcept {}
    virtual void onOutputDeviceChanged(const PaDeviceInfoEx& newOutputDevice) noexcept {}
    virtual void onStreamStarted(const PaStreamInfo& streamInfo) noexcept {}
    virtual void onStreamStopped(const PaStreamInfo& streamInfo) noexcept {}
    virtual void onStreamAbort(const PaStreamInfo& streamInfo) noexcept {}
};

using notification_collection = std::vector<notification_interface*>;

template <typename AUDIOCALLBACK> struct PortAudio {

    private:
    AUDIOCALLBACK& m_audioCallback;
    void myTerminate() {
        __try {
            Pa_Terminate();
        } __except (
            filter_msvc_exceptions2(GetExceptionCode(), GetExceptionInformation())) {
            TRACE("module threw exception when Quit() called. Meh\n");
        }
    }

    std::array<PaDeviceInfoEx, 2> m_currentDevices;

    std::array<AudioFormat, 2> m_currentFormats;

    public:
    const PaDeviceInfoEx& currentDevice(const DeviceTypes ty) {
        const auto idx = static_cast<int>(ty);
        return m_currentDevices[idx];
    }

    // could be index 0 for input, index 1 for output. But currently they are always the
    // same, as implemented.
    const auto& currentFormats(
        DeviceTypes deviceType = DeviceTypes::input) const noexcept {
        const auto idx = static_cast<int>(deviceType);
        return m_currentFormats[idx];
    }
    int errCode() const noexcept { return this->m_errcode; }

    PortAudio(AUDIOCALLBACK& acb, notification_interface* pnotify = nullptr)
        : m_audioCallback(acb) {
        timer t("Initting portaudio");
        if (pnotify) {
            notification_add(pnotify);
        }
        init();
    }
    PortAudio(const PortAudio& other) = delete;
    PortAudio(PortAudio&& rhs) = delete;
    ~PortAudio() {
        Stop();

        myTerminate();
    }

    struct info_t {
        std::string api_name;
        std::string input_name;
        std::string output_name;
        bool was_running{false};
    };

    info_t info() const {
        info_t ret{};
        ret.api_name = m_currentApi.name;
        ret.input_name = m_currentDevices[0].info.name;
        ret.output_name = m_currentDevices[1].info.name;
        ret.was_running = m_running;
        return ret;
    }

    void restoreInfo(const info_t& info) {
        auto api = findApi(info.api_name);
        if (!api) {
            std::string s("Cannot restore api, with name: ");
            s += info.api_name;
            throw std::runtime_error(s);
        }
        this->changeApi(api);
        auto in = findDevice(info.input_name);
        if (!in) {
            std::string s("Cannot restore input device, with name: ");
            s += info.input_name;
            throw std::runtime_error(s);
        }
        auto out = findDevice(info.output_name);
        if (!out) {
            std::string s("Cannot restore output device, with name: ");
            s += info.output_name;
            throw std::runtime_error(s);
        }

        this->changeDevice(*in, DeviceTypes::input);
        this->changeDevice(*out, DeviceTypes::output);
        if (info.was_running) {
            prepareStream(sampleRate(), m_channels, true);
            this->Start();
        }
    }

    enum class formatSuppRecurseReasons {
        none,
        changeInputChannels,
        changeOutputChannels
    };

    // find a common sample rate between the input and output, if possible
    auto findCommonSamplerate(
        const SampleFormat& fmt = SampleFormat::Float32) const noexcept {
        const auto& in
            = m_currentDevices[0].get_supported_audio_types(DeviceTypes::input);
        const auto& out
            = m_currentDevices[1].get_supported_audio_types(DeviceTypes::output);

        int idx = -1;
        unsigned common_sr = 0;
        unsigned int last_result = 0;

        for (const auto& f : in) {
            if (f.samplefmt == fmt) {
                for (const auto& sup : out) {
                    if (sup.samplefmt == fmt) {
                        if (f.samplerate == sup.samplerate
                            && f.channels == sup.channels) {
                            const auto fsr = f.samplerate;
                            if (fsr > last_result) {
                                common_sr = fsr.m_value;
                                last_result = common_sr;
                            }
                        }
                    }
                    idx++;
                };
            }
        };

        return common_sr;
    }

    PaError isFormatSupported(const AudioFormat& fmt) const noexcept {
        int i = static_cast<int>(fmt.forInOrOut);
        return m_currentDevices[i].isFormatSupported(fmt);
    }

    /*/
    // throws
    bool isFormatSupported(
        ChannelsType& chans, SampleRateType& samplerate, int& bitdepth, const DeviceTypes
    inOrOut) {

        static formatSuppRecurseReasons recurse_reason = formatSuppRecurseReasons::none;
        if (samplerate == 0) samplerate = 44100;
        if (bitdepth <= 0) bitdepth = 32;
        PaStreamParameters ip = {0};
        PaStreamParameters op = {0};

        ip.channelCount = chans[0];
        ip.device = m_currentDevices[0].index;
        int sampleFormat = 0;
        if (bitdepth == 16) {
            sampleFormat = paInt16;
        } else {
            sampleFormat = paFloat32;
        }

        ip.sampleFormat = sampleFormat;
        op.channelCount = chans[1];
        op.device = m_currentDevices[1].index;
        op.sampleFormat = sampleFormat;
        m_errcode = Pa_IsFormatSupported(&ip, &op, (double)samplerate.m_value);

        if (m_errcode != paNoError) {
            std::string msg("Portaudio: Requested properties not supported\n\n");
            msg += Pa_GetErrorText(m_errcode);
            msg += "\n";
            msg += Pa_GetLastHostErrorInfo()->errorText;

            if (m_errcode == paInvalidSampleRate) {
                msg = ("Required sample rate of ");
                msg += samplerate.to_string();
                msg += " is not supported by your device.\n\nYou may need to change its "
                       "properties in "
                       "Windows to allow "
                       "a bit depth of 32-bit and the required sample rate, if possible";
                msg += "\n\n\nPortaudio reports:\n";
                msg += Pa_GetLastHostErrorInfo()->errorText;
                throw(std::runtime_error(msg));
            } else {
                if (m_errcode != paInvalidChannelCount) {
                    throw std::runtime_error(msg);
                }
            }

            if (m_errcode == paInvalidChannelCount) {
                msg = "";
                msg += Pa_GetErrorText(m_errcode);
                msg += "\n\n";
                msg += Pa_GetLastHostErrorInfo()->errorText;

                for (int i = 0; i < 2; ++i) {
                    auto& dev = m_currentDevices[i];
                    auto avail_chans = dev.info.maxInputChannels;
                    if (i == 1) {
                        avail_chans = dev.info.maxOutputChannels;
                    }

                    if (avail_chans < chans[i]) {
                        msg = "The requested device:\n";
                        msg += dev.extendedName;
                        msg += " only has ";
                        msg += std::to_string(avail_chans);
                        msg += " channel(s) available, but you are requesting ";
                        msg += std::to_string(chans[0]);
                        msg += " channels";

                        if (recurse_reason
                            != formatSuppRecurseReasons::changeInputChannels) {

                            chans[i] = (uint8_t)avail_chans;
                            recurse_reason
                                = formatSuppRecurseReasons::changeInputChannels;
                            bool is = isFormatSupported(chans, samplerate, bitdepth);

                            if (is) {
                                m_errcode = paFormatIsSupported;
                                recurse_reason = formatSuppRecurseReasons::none;
                                return is;
                            }
                        } else {

                            chans[i] = (uint8_t)avail_chans;
                            bool is = isFormatSupported(chans, samplerate, bitdepth);

                            if (is) {
                                m_errcode = paFormatIsSupported;
                                recurse_reason = formatSuppRecurseReasons::none;
                                return is;
                            }
                        }
                        break;
                    }
                };
            }
            recurse_reason = formatSuppRecurseReasons::none;
            throw std::runtime_error(msg);
        }
        return true;
    }
    /*/

    // we do not own these:
    notification_collection m_notification_interfaces;

    bool hasNotification(const notification_interface* ni) {
        auto it = std::find_if(m_notification_interfaces.begin(),
            m_notification_interfaces.end(), [&](auto i) {
                if (i == ni) return true;
                return false;
            });
        return it != m_notification_interfaces.end();
    }
    bool notification_add(notification_interface* ni) {
        assert(ni);
        auto it = std::find_if(m_notification_interfaces.begin(),
            m_notification_interfaces.end(), [&](auto i) {
                if (i == ni) return true;
                return false;
            });

        if (it != m_notification_interfaces.end()) {
            return false; // already added
        }
        if (ni) {
            m_notification_interfaces.push_back(ni);
            return true;
        }
        return false;
    }

    bool notification_remove(notification_interface* ni) {
        if (ni) {
            auto it = std::remove_if(m_notification_interfaces.begin(),
                m_notification_interfaces.end(), [&](auto i) {
                    if (i == ni) return true;
                    return false;
                });

            if (it != m_notification_interfaces.end()) {
                m_notification_interfaces.erase(it);
                return true;
            }
        }
        return false;
    }

    bool subscribedForNotifications(notification_interface* ni) const noexcept {
        assert(ni);
        if (!ni) return false;

        auto it = std::find_if(m_notification_interfaces.begin(),
            m_notification_interfaces.end(), [&](auto i) { return i == ni; });
        return it != m_notification_interfaces.end();
    }

    void notifyNewApi() {
        for (auto i : m_notification_interfaces) {
            i->onApiChanged(m_currentApi);
        }
    }

    void notifyNewInputDevice() {
        for (auto i : m_notification_interfaces) {
            i->onInputDeviceChanged(
                this->m_currentDevices[static_cast<int>(DeviceTypes::input)]);
        }
    }

    void notifyNewOutputDevice() {
        for (auto i : m_notification_interfaces) {
            i->onOutputDeviceChanged(
                this->m_currentDevices[static_cast<int>(DeviceTypes::output)]);
        }
    }

    size_t getByteCount(unsigned long frameCount) {
        static constexpr unsigned long chans = 2;
        return frameCount * chans * sizeof(float);
    }

    Dsp::EnvelopeFollower<> m_env_input;
    Dsp::EnvelopeFollower<> m_env_output;

    std::atomic<int> m_running{0};
    ChannelsType m_channels[2];

    PaTime m_startTime{0};

    const std::array<int, 2>& channels() const noexcept { return m_channels; }
    SampleFormat sampleFormat() const noexcept {
        return SampleFormat(m_outputParams.sampleFormat);
    }
    auto bits() const noexcept {
        return SampleFormat(m_outputParams.sampleFormat).bits();
    }

    double currentLatency() const {
        if (!m_stream) return -1;
        if (!m_running) return -1;
        auto info = Pa_GetStreamInfo(m_stream);
        return (std::max)(info->inputLatency, info->outputLatency);
    }

    double currentTime() const {
        if (!m_stream) return -1;
        if (!m_running) return -1;
        return Pa_GetStreamTime(m_stream) - m_startTime;
    }

    std::atomic<bool> m_bMonoInput{false};
    bool monoInput() { return m_bMonoInput; }
    void monoInputSet(bool what) { m_bMonoInput = what; }

    template <typename T>
    static inline void downMixToStereo(
        const T* input, T* output, const int frameCount, const int nch_in) {

        int i = 0;

        while (i++ < frameCount) {
            float left_in = 0;
            float right_in = 0;
            for (int ch = 0; ch < nch_in; ++ch) {
                if (ch == 0)
                    left_in = *input++;
                else
                    right_in = *input++;
            }
            *output++ = (left_in / 2.0f) + (right_in / 2.0f);
            *output++ = (left_in / 2.0f) + (right_in / 2.0f);
        }
    }

    static inline void zero_every_other(float* ptr, const int frameCount) {
        int i = 0;
        while (i++ < frameCount) {
            for (int ch = 0; ch < 2; ++ch) {
                if (ch == 0) *ptr = 0;
                ptr++;
            }
        }
    }

    void process_meter(Dsp::EnvelopeFollower<>& env, const float* values,
        const unsigned int frameCount) {

        env.ProcessInterleaved(frameCount, values);
    }

    void process_meter_in(const float* input, const unsigned int frameCount) {
        process_meter(this->m_env_input, input, frameCount);
    }

    void process_meter_out(const float* output, const unsigned int frameCount) {
        process_meter(this->m_env_output, output, frameCount);
    }

    static inline int streamCallback(const void* input, void* output,
        unsigned long frameCount, const PaStreamCallbackTimeInfo* timeInfo,
        PaStreamCallbackFlags statusFlags, void* userData) {

        static_assert(std::atomic<PaTime>::is_always_lock_free);
        auto pthis = static_cast<PortAudio*>(userData);
        assert(pthis);

        if (pthis) {
            const auto byteCount = pthis->getByteCount(frameCount);

            if (pthis->monoInput() && pthis->m_channels[0] == ChannelsType::mono) {
                downMixToStereo((float*)input, (float* const)input, frameCount,
                    pthis->m_channels[0].m_value);
            }
            memset(output, 0, byteCount);

            int ret = pthis->m_audioCallback.streamCallback(
                input, output, frameCount, timeInfo, statusFlags, byteCount);

            if (ret == 0) {
                memcpy(output, input, byteCount);

            } else {
                if (ret < 0) {
                    return paComplete;
                }
            }

            pthis->process_meter_in((float*)input, frameCount);
            pthis->process_meter_out((float* const)output, frameCount);
        }

        return paContinue;
    }

    PaStreamFlags prepareFlags() {
        PaStreamFlags flags = {0};
        return flags;
    }

    PaStreamParameters m_inputParams = {};
    PaStreamParameters m_outputParams = {};
    SampleRateType m_sampleRate = {};

    SampleRateType sampleRate() const noexcept { return m_sampleRate; }

    // throws
    PaError prepareStream(const SampleRateType& samplerate,
        const ChannelsType channels[2], const SampleFormat& sf = SampleFormat::Float32,
        const bool super_low_latency = true) {
        if (m_stream) this->Close();
        PaStreamParameters& ip = m_inputParams;
        ip = prepareInOutParams(this->m_currentDevices[0].index, sf,
            m_currentDevices[0].info.defaultLowInputLatency,
            channels[static_cast<int>(DeviceTypes::input)]);
        ip.channelCount = channels[0].m_value;

        PaStreamParameters& op = m_outputParams;
        op = prepareInOutParams(this->m_currentDevices[1].index, sf,
            m_currentDevices[1].info.defaultLowOutputLatency,
            channels[static_cast<int>(DeviceTypes::output)]);
        op.channelCount = channels[1].m_value;
        PaStreamFlags flags = prepareFlags();

        if (super_low_latency) {
            ip.suggestedLatency = ip.suggestedLatency / 2;
            op.suggestedLatency = op.suggestedLatency / 2;
        }
        m_sampleRate = samplerate;

        m_errcode = Pa_OpenStream(&m_stream, &ip, &op, samplerate.m_value,
            paFramesPerBufferUnspecified, flags, streamCallback, this);

        std::string errInfo;

        if (m_errcode != paNoError) {
            errInfo = Pa_GetErrorText(m_errcode);
            errInfo += "\n\n";
            errInfo += "Additional info:\n";
            errInfo += Pa_GetLastHostErrorInfo()->errorText;
            throw std::runtime_error(errInfo);
        }

        m_env_input.Setup(m_sampleRate.m_value, 1.0, 1000.0);
        m_env_output.Setup(m_sampleRate.m_value, 1.0, 1000.0);

        m_currentFormats[0]
            = AudioFormat{DeviceTypes::input, samplerate, sf, channels[0]};
        m_currentFormats[1]
            = AudioFormat{DeviceTypes::input, samplerate, sf, channels[0]};

        return m_errcode;
    }

    // throws
    int preparePlay(const SampleRateType& samplerate, const ChannelsType& chans,
        const bool super_low_latency = true) {
        if (m_stream) {
            Close();
        }
        try {
            if (!m_stream) {
                ChannelsType mychans[2];
                mychans[0] = chans;
                mychans[1] = chans;
                m_errcode = prepareStream(samplerate, mychans);
            }
        } catch (const std::exception& e) {
            if (m_errcode == 0) {
                m_errcode = -1;
            }
            throw e;
        }

        return 0;
    }

    PaError Stop() {
        if (m_stream) {
            auto ret = Pa_StopStream(m_stream);
            m_running = false;
            return ret;
        }

        return paNoError;
    }

    // throws
    PaError Start() {
        if (!m_stream) {
            throw std::runtime_error("PortAudio: cannot start if there is no stream. "
                                     "Call PreparePlay() first");
        }
        auto ret = Pa_StartStream(m_stream);
        if (ret == paNoError) {
            m_running = 1;
            m_startTime = Pa_GetStreamTime(m_stream);
        } else {
            std::string msg("PortAudio: cannot start, error code: ");
            msg += std::to_string(ret);
            msg += "\n";
            msg += Pa_GetErrorText(ret);
            if (ret == paUnanticipatedHostError) {
                msg += "\n";
                msg += Pa_GetLastHostErrorInfo()->errorText;
            }
            throw std::runtime_error(msg);
        }
        return ret;
    }

    PaError Close() {
        int ret = 0;
        if (m_stream) {
            m_running = 0;

            ret = Pa_CloseStream(m_stream);
        }

        m_stream = nullptr;
        m_errcode = 0;
        return ret;
    }
    PaStream* m_stream = nullptr;

    using device_list = std::vector<PaDeviceInfoEx>;
    using api_list = std::vector<PaHostApiInfo>;
    device_list m_devices;
    api_list m_apis;
    PaHostApiInfo m_currentApi = {};

    std::string getErrors() noexcept {
        const auto err = m_errcode;
        std::string ret("PortAudio reports, for error code: ");
        ret += std::to_string(err);
        ret += Pa_GetErrorText(err);
        ret += "\n";
        ret += "Api Error: ";
        const auto apierr = Pa_GetLastHostErrorInfo();

        ret += Pa_GetHostApiInfo(apierr->hostApiType)->name;
        ret += "\n";
        ret += std::to_string(apierr->errorCode);
        ret += apierr->errorText;
        return ret;
    }

    const device_list& devices() const noexcept { return m_devices; }
    device_list& devices() { return m_devices; }
    const api_list& apis() const noexcept { return m_apis; }

    PaDeviceInfoEx* findDevice(std::string_view name) noexcept {
        for (auto&& d : this->m_devices) {
            std::string_view n1(d.extendedName);
            if (n1 == name) {
                return &d;
            }
        }
        assert(0);
        return nullptr;
    }

    const PaDeviceInfoEx* findDevice(const PortAudioGlobalIndex& idx) const noexcept {
        const auto index = idx.m_value;
        assert(index > 0);
        for (auto&& d : m_devices) {
            if (d.globalIndex == idx) return &d;
        }
        return nullptr;
    }
    PaDeviceInfoEx* findDevice(unsigned int index) noexcept {
        for (auto&& d : m_devices) {
            if (d.index == (int)index) return &d;
        }
        return nullptr;
    }

    const PaHostApiInfo* findApi(std::string_view name) const noexcept {
        for (const auto& api : this->apis()) {
            std::string_view n1(api.name);
            if (n1 == name) {
                return &api;
            }
        }
        return nullptr;
    }

    // throws std::runtime_error
    void changeApi(const PaHostApiInfo* api) {
        Close();
        if (!api) {
            std::string s("PortAudio: fatal : api is null ");
            throw std::runtime_error(s);
        }
        m_currentApi = *api;

        this->m_devices = enumDevices(m_currentApi);
        this->notifyNewApi(); // only once we have enumerated devices, so client can
                              // populate them
        changeDevice(api->defaultInputDevice, DeviceTypes::input);
        changeDevice(api->defaultOutputDevice, DeviceTypes::output);
    }

    // throws std::runtime_error
    void changeApi(const int hostApiIndex) {
        const auto api = Pa_GetHostApiInfo(hostApiIndex);
        if (!api) {
            std::string s("PortAudio: fatal : cannot get audio api from index: ");
            s += std::to_string(hostApiIndex);
            throw std::runtime_error(s);
        }
        changeApi(api);
    }

    // throws
    void changeDevice(int deviceIndex, const DeviceTypes& inOrOut) {
        const auto info = Pa_GetDeviceInfo(deviceIndex);
        Close();

        if (!info) {
            std::string s = "Fatal: unable to get device info for index: ";
            s += std::to_string(deviceIndex);
            throw std::runtime_error(s);
        }
        std::string_view n1(m_currentApi.name);
        auto devhost = Pa_GetHostApiInfo(info->hostApi);
        std::string_view n2(devhost->name);
        assert(n1 == n2); // must belong to current api
        if (n1 != n2) {
            std::string err("PortAudio: changeDevice: Api mismatch. Current API is: ");
            err += n1;
            err += ", but device belongs to api: ";
            err += n2;
            err += "\nSet failsafe defaults, and try again.";
            throw std::runtime_error(err);
        }
        PaDeviceInfoEx* p = findDevice(deviceIndex);
        if (!p) {
            assert(0);
            throw std::runtime_error("Unexpected: changeDevice cannot find device");
        }

        m_currentDevices[static_cast<int>(inOrOut)] = *p;

        assert(!m_currentDevices[static_cast<int>(inOrOut)].extendedName.empty());
        if (inOrOut == DeviceTypes::input) {
            notifyNewInputDevice();
        } else {
            notifyNewOutputDevice();
        }
    }

    // throws
    void changeDevice(const PaDeviceInfoEx& di, const DeviceTypes& inOrOut) {

        auto devhost = Pa_GetHostApiInfo(di.info.hostApi);
        if (!devhost) {

            throw std::runtime_error("Cannot find host");
        }

        assert(std::string_view(m_currentApi.name)
            == std::string_view(devhost->name)); // must belong to current api
        m_currentDevices[static_cast<int>(inOrOut)] = di;
        assert(!m_currentDevices[static_cast<int>(inOrOut)].extendedName.empty());
        if (inOrOut == DeviceTypes::input) {
            notifyNewInputDevice();
        } else {
            notifyNewOutputDevice();
        }
    }

    void failsafeDefaults() {
        const auto in_dev_index = Pa_GetDefaultInputDevice();
        const auto out_dev_index = Pa_GetDefaultOutputDevice();
        const auto api_index = Pa_GetDefaultHostApi();

        if (in_dev_index < 0 || out_dev_index < 0 || api_index < 0) {
            m_errcode = -1;
            throw std::runtime_error("Portaudio: fatal: cannot set fail-safe defaults");
        }
        try {
            this->changeApi(api_index);

        } catch (...) { //-V565
            // changeAPi might have some intermediate failure, ignore
        }

        // cannot ignore any errors here
        changeDevice(in_dev_index, portaudio_cpp::DeviceTypes::input);
        changeDevice(out_dev_index, portaudio_cpp::DeviceTypes::output);
    }

    private:
    int m_errcode = 0;
    int init() {
        m_errcode = 0;
        m_errcode = Pa_InitializeEx(myPaInitCallback);
        if (m_errcode) {
            throw std::runtime_error("Failed to init PortAudio");
        }
        m_apis = enumApis();
        return m_errcode;
    }

    api_list enumApis() {
        api_list retval;
        retval.reserve(6);
        for (auto i = 0; i < Pa_GetHostApiCount(); ++i) {
            auto inf = Pa_GetHostApiInfo(i);
            if (inf) retval.push_back(*inf);
        }

        const auto api_index = Pa_GetDefaultHostApi();
        const auto info = Pa_GetHostApiInfo(api_index);
        if (!info) {
            throw std::runtime_error(
                "PortAudio: fatal : cannot get default audio api for this machine.");
        }
        // whether you want to or not, if you are re-initting, the api gets changed to
        // the default as otherwise the program is in an undefined state, pointing to
        // stack memory that no longer exists inside portAudio.
        changeApi(api_index);
        return retval;
    }

    void setDefaultDevices() {
        auto dev = m_currentApi.defaultInputDevice;

        if (!dev) {
            std::string e("PortAudio: fatal: cannot get default input device for api: ");
            e += m_currentApi.name;
            throw std::runtime_error(e);
        }
        changeDevice(dev, INPUT_DEVICE);

        dev = m_currentApi.defaultOutputDevice;
        if (!dev) {
            std::string e("PortAudio: fatal: cannot get default output device for api: ");
            e += m_currentApi.name;
            throw std::runtime_error(e);
        }
        changeDevice(dev, OUTPUT_DEVICE);
    }

    device_list enumDevices(const PaHostApiInfo& api) {
        device_list retval;
        retval.reserve(12);
        std::set<std::string> uset;
        int portAudioGlobalIndex = 0;

        for (auto i = 0; i < Pa_GetDeviceCount(); ++i) {
            auto inf = Pa_GetDeviceInfo(i);
            if (inf) {
                const auto hostInfo = Pa_GetHostApiInfo(inf->hostApi);
                if (std::string_view(hostInfo->name)
                    == std::string_view(m_currentApi.name)) {
                    PaDeviceInfoEx p;
                    // he does know the type of device, else how does app populate input
                    // and output devices sep?
                    p.info = (*inf);
                    p.index = i;
                    p.globalIndex.m_value = portAudioGlobalIndex;
                    if (uset.find(inf->name) != uset.end()) {
                        // more than one!
                        int ctr = 0;
                        do {
                            p.extendedName = inf->name;
                            p.extendedName += "_" + std::to_string(ctr++);
                        } while (uset.find(p.extendedName) != uset.end());
                    } else {
                        // it's the only one
                        p.extendedName = inf->name;
                    }
                    uset.insert(p.extendedName);
                    assert(!p.extendedName.empty());
                    retval.push_back(p);
                } else {
                    // not for this host
                    // TRACE("Device not for this host\n");
                }
            }
            ++portAudioGlobalIndex;
        }

        return retval;
    }
};

} // namespace portaudio_cpp
