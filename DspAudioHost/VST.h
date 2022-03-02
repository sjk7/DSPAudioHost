#pragma once
// VST.h
#ifndef VST_FORCE_DEPRECATED
#define VST_FORCE_DEPRECATED 1
#endif

#include "../Helpers/vstsdk2.4/public.sdk/source/vst2.x/audioeffectx.h"
#include "../Helpers/vstsdk2.4/pluginterfaces/vst2.x/aeffect.h"
#include "../Helpers/vstsdk2.4/public.sdk/source/vst2.x/aeffeditor.h"
#include "../Helpers/vstsdk2.4/pluginterfaces/vst2.x/vstfxstore.h"
#include <utility>
#include <string>
#include <cassert>

#ifndef logWarn
#define logWarn TRACE
#endif

inline std::string parseFilename(
    std::string_view filepath, bool include_extenstion = false) {
    std::string ret;
    const auto found = filepath.find_last_of("\\");
    if (found != std::string::npos) {
        ret = filepath.substr(found + 1);
    }
    if (!include_extenstion && !ret.empty()) {
        const auto f = ret.find_last_of(".");
        if (f != std::string::npos) {
            ret = ret.substr(0, f);
        }
    }
    return ret;
}

// C callbacks
extern "C" {
// Main host callback
VstIntPtr VSTCALLBACK hostCallback(AEffect* effect, VstInt32 opcode, VstInt32 index,
    VstInt32 value, void* ptr, float opt);
}

// Plugin's entry point
typedef AEffect* (*vstPluginFuncPtr)(audioMasterCallback host);
// Plugin's dispatcher function
typedef VstIntPtr (*dispatcherFuncPtr)(AEffect* effect, VstInt32 opCode, VstInt32 index,
    VstInt32 value, void* ptr, float opt);
// Plugin's getParameter() method
typedef float (*getParameterFuncPtr)(AEffect* effect, VstInt32 index);
// Plugin's setParameter() method
typedef void (*setParameterFuncPtr)(AEffect* effect, VstInt32 index, float value);
// Plugin's processEvents() method
typedef VstInt32 (*processEventsFuncPtr)(VstEvents* events);
// Plugin's process() method
typedef void (*processFuncPtr)(
    AEffect* effect, float** inputs, float** outputs, VstInt32 sampleFrames);

static inline void logDeprecated(const char* s, const char* s2) {
    std::string str(s);
    str += " ";
    str += s2;
    ::OutputDebugStringA(str.c_str());
}
static inline AEffect* loadPlugin(const char* vstPath) {
    AEffect* plugin = NULL;

    static HRESULT hrco = ::CoInitialize(NULL);

    auto modulePtr = LoadLibraryA(vstPath);
    if (modulePtr == NULL) {
        TRACE("Failed trying to load VST from '%s', error %d\n", vstPath, GetLastError());
        return NULL;
    }

    vstPluginFuncPtr mainEntryPoint
        = (vstPluginFuncPtr)GetProcAddress(modulePtr, "VSTPluginMain");

    if (!mainEntryPoint) {
        // plugs used to use 'main' before 'VSTPluginMain' was mandated. FFS!
        mainEntryPoint = (vstPluginFuncPtr)GetProcAddress(modulePtr, "main");
    }
    if (!mainEntryPoint) {
        return nullptr;
    }
    // Instantiate the plugin
    plugin = mainEntryPoint(hostCallback);
    return plugin;
}

static inline AEffect* configurePlugin(AEffect* plugin) {
    // Check plugin's magic number
    // If incorrect, then the file either was not loaded properly, is not a
    // real VST plugin, or is otherwise corrupt.
    if (plugin->magic != kEffectMagic) {
        TRACE("Plugin's magic number is bad\n");
        ASSERT("Bad magic, not a VST 2.x plug" == nullptr);
        return nullptr;
    }

    // Create dispatcher handle
    (dispatcherFuncPtr)(plugin->dispatcher);

    // Set up plugin callback functions
    plugin->getParameter = (getParameterFuncPtr)plugin->getParameter;
    plugin->processReplacing = (processFuncPtr)plugin->processReplacing;
    plugin->setParameter = (setParameterFuncPtr)plugin->setParameter;

    return plugin;
}

static inline void resumePlugin(AEffect* plugin, dispatcherFuncPtr dispatcher) {
    dispatcher(plugin, effMainsChanged, 0, 1, NULL, 0.0f);
}

static inline void suspendPlugin(AEffect* plugin, dispatcherFuncPtr dispatcher) {
    dispatcher(plugin, effMainsChanged, 0, 0, NULL, 0.0f);
}

static inline void startPlugin(AEffect* plugin) {
    plugin->dispatcher(plugin, effOpen, 0, 0, NULL, 0.0f);

    // Set some default properties
    float sampleRate = 44100.0f;
    plugin->dispatcher(plugin, effSetSampleRate, 0, 0, NULL, sampleRate);
    int blocksize = 512;
    plugin->dispatcher(plugin, effSetBlockSize, 0, blocksize, NULL, 0.0f);

    resumePlugin(plugin, plugin->dispatcher);
}

static inline auto currentPluginUniqueId = 77;
static inline VstTimeInfo vstTimeInfo{};
static inline auto getSampleRate() {
    return 44100;
}
static inline auto getBlocksize() {
    return 8;
}

static auto constexpr VENDOR_NAME = "AudioEnhance";
static auto constexpr PROGRAM_NAME = "DSPHost";
static auto constexpr VERSION_MAJOR = 1;
static auto constexpr VERSION_MINOR = 1;
static auto constexpr VERSION_PATCH = 1;

inline bool canPluginDo(
    char* canDoString, AEffect* plugin, dispatcherFuncPtr dispatcher) {
    return (dispatcher(plugin, effCanDo, 0, 0, (void*)canDoString, 0.0f) > 0);
}

inline std::string getEffectName(AEffect* effect) noexcept {

    char ceffname[kVstMaxEffectNameLen] = "";
    int ret = effect->dispatcher(effect, effGetEffectName, 0, 0, ceffname, 0.0f);
    if (ret == 0) {
        return std::string(ceffname);
    }

    return std::string();
}

inline int getChunk(AEffect* effect, void** data, const int is_preset = 0) noexcept {
    ///< [ptr]: void** for chunk data address [index]: 0 for bank, 1 for program  @see
    ///< AudioEffect::getChunk
    int ret = effect->dispatcher(effect, effGetChunk, is_preset, 0, data, 0);
    if (ret) {
        *data = data;
    }
    return ret;
}

inline int setChunk(
    AEffect* effect, std::string_view data, const int is_preset = 0) noexcept {
    ///< [ptr]: chunk data [value]: byte size [index]: 0 for bank, 1 for program  @see
    ///< AudioEffect::setChunk
    int ret
        = effect->dispatcher(effect, effSetChunk, 1, data.size(), (void*)data.data(), 0);
    return ret;
}

static const size_t data_offset = ((size_t) & ((fxBank*)0)->content.data.chunk);

#ifndef BIG_ENDIAN
// custom endian swapper to avoid dependency on winsock
// you could use htonl() if you're linking it anyway
inline static unsigned fxbEndian(unsigned x) {
    x = ((x & 0xff00ff00) >> 8) | ((x & 0xff00ff) << 8);
    return (x >> 16) | (x << 16);
}
#else
// on big-endian systems don't need to do anything
static unsigned fxbEndian(unsigned x) {
    return x;
}
#endif

inline bool loadChunkData(std::string_view filepath, AEffect* plugin) {
    bool ret = false;
    FILE* pFile;
    pFile = fopen(filepath.data(), "rb");
    if (!pFile) {
        return ret;
    }
    fseek(pFile, 0, SEEK_END);
    long lSize = ftell(pFile);
    rewind(pFile);
    fxBank* header = (fxBank*)malloc(lSize);
    fread(header, 1, (size_t)lSize, pFile);
    fclose(pFile);

    // you need to byteswap it again..
    int numBytes = fxbEndian(header->byteSize);
    { TRACE("numbytes load: %ld\n", numBytes); }

    int numPrograms = fxbEndian(header->numPrograms);
    TRACE("numPrograms: %ld\n", numPrograms);
    // the size to pass to plugin is the content size
    int chunkSize = fxbEndian(header->content.data.size);
    // you need &-operator here to take the address
    void* chunkDat = (void*)&(header->content.data.chunk);
    plugin->dispatcher(plugin, effSetChunk, (int)0, chunkSize, chunkDat, 0.f);
    free(header);
    return true;
}

inline void saveChunkData(
    std::string_view filepath, AEffect* plugin, int currentProgram) {
    FILE* pFile;
    pFile = fopen(filepath.data(), "wb");
    void* chunk;
    VstInt32 chunkSize = plugin->dispatcher(plugin, effGetChunk, 0, 0, &chunk, 0);

    fxBank header;
    // zero out by default "best practice"
    // saves the trouble of filling every field
    memset(&header, 0, sizeof(fxBank));

    header.chunkMagic = fxbEndian('CcnK');
    header.byteSize = fxbEndian(data_offset + chunkSize - 2 * sizeof(VstInt32));
    // Opaque chunk is 'FBCh'! 'FxBk' is for "host saves parameters"
    header.fxMagic = fxbEndian('FBCh');
    header.version = fxbEndian(2);
    header.fxID = fxbEndian(plugin->uniqueID);
    header.fxVersion = fxbEndian(plugin->version);
    header.numPrograms = fxbEndian(plugin->numPrograms);
    // FIXME: probably want to store current program here?
    header.currentProgram = currentProgram;
    // future part is zeroed by the memset
    // but data size-field is required
    header.content.data.size = fxbEndian(chunkSize);

    // need to convert back to host byte-order here
    int a = fxbEndian(header.byteSize);
    {
        char buf[50];
        sprintf(buf, "%d", a);
        TRACE("size is: %s\n", buf);
    }

    fwrite(&header, data_offset, 1, pFile);
    fwrite(chunk, chunkSize, 1, pFile);

    fclose(pFile);
}

//-------------------------------------------------------------------------------------------------------
inline void checkEffectProperties(AEffect* effect, std::vector<std::string>& presets) {
    TRACE("HOST> Gathering properties...\n");

    char effectName[256] = {0};
    char vendorString[256] = {0};
    char productString[256] = {0};

    effect->dispatcher(effect, effGetEffectName, 0, 0, effectName, 0);
    effect->dispatcher(effect, effGetVendorString, 0, 0, vendorString, 0);
    effect->dispatcher(effect, effGetProductString, 0, 0, productString, 0);

    TRACE("Name = %s\nVendor = %s\nProduct = %s\n\n", effectName, vendorString,
        productString);

    TRACE("numPrograms = %d\nnumParams = %d\nnumInputs = %d\nnumOutputs = %d\n\n",
        effect->numPrograms, effect->numParams, effect->numInputs, effect->numOutputs);

    presets.reserve(effect->numPrograms);
    // Iterate programs... (what any sane person would call 'presets'. FFS.)
    for (VstInt32 progIndex = 0; progIndex < effect->numPrograms; progIndex++) {
        char progName[256] = {0};
        if (!effect->dispatcher(
                effect, effGetProgramNameIndexed, progIndex, 0, progName, 0)) {
            effect->dispatcher(effect, effSetProgram, 0, progIndex, 0,
                0); // Note: old program not restored here!
            effect->dispatcher(effect, effGetProgramName, 0, 0, progName, 0);
        }
        if (strlen(progName) == 0)
            presets.push_back("Preset " + std::to_string(progIndex + 1));
        else
            presets.push_back(progName);
        TRACE("Program %03d: %s\n", progIndex, progName);
    }

    TRACE("\n");

    // Iterate parameters...
    for (VstInt32 paramIndex = 0; paramIndex < effect->numParams; paramIndex++) {
        char paramName[256] = {0};
        char paramLabel[256] = {0};
        char paramDisplay[256] = {0};

        effect->dispatcher(effect, effGetParamName, paramIndex, 0, paramName, 0);
        effect->dispatcher(effect, effGetParamLabel, paramIndex, 0, paramLabel, 0);
        effect->dispatcher(effect, effGetParamDisplay, paramIndex, 0, paramDisplay, 0);
        float value = effect->getParameter(effect, paramIndex);

        TRACE("Param %03d: %s [%s %s] (normalized = %f)\n", paramIndex, paramName,
            paramDisplay, paramLabel, value);
    }

    TRACE("\n");

    // Can-do nonsense...
    static const char* canDos[]
        = {"receiveVstEvents", "receiveVstMidiEvent", "midiProgramNames"};

    for (VstInt32 canDoIndex = 0; canDoIndex < sizeof(canDos) / sizeof(canDos[0]);
         canDoIndex++) {
        TRACE("Can do %s... ", canDos[canDoIndex]);
        VstInt32 result = (VstInt32)effect->dispatcher(
            effect, effCanDo, 0, 0, (void*)canDos[canDoIndex], 0);
        switch (result) {
            case 0: TRACE("don't know\n"); break;
            case 1: TRACE("yes\n"); break;
            case -1: TRACE("definitely not!\n"); break;
            default: TRACE("?????\n");
        }
        TRACE("\n");
    }

    TRACE("\n");
}

static inline float** inputs;
static inline float** outputs;
static inline size_t numChannels = 2;
static inline size_t blocksize = 8;

static inline void initializeIO() {
    // inputs and outputs are assumed to be float** and are declared elsewhere,
    // most likely the are fields owned by this class. numChannels and blocksize
    // are also fields, both should be size_t (or unsigned int, if you prefer).
    inputs = (float**)malloc(sizeof(float**) * numChannels);
    outputs = (float**)malloc(sizeof(float**) * numChannels);
    for (unsigned int channel = 0; channel < numChannels; channel++) {
        inputs[channel] = (float*)malloc(sizeof(float*) * blocksize);
        outputs[channel] = (float*)malloc(sizeof(float*) * blocksize);
    }
}
static inline void silenceChannel(float** channelData, unsigned int nch, long numFrames) {
    for (unsigned int channel = 0; channel < nch; ++channel) {
        for (long frame = 0; frame < numFrames; ++frame) {
            channelData[channel][frame] = 0.0f;
        }
    }
}

static inline void processAudio(
    AEffect* plugin, float** ins, float** outs, long numFrames) {
    // Always reset the output array before processing.
    silenceChannel(outs, numChannels, numFrames);

    // Note: If you are processing an instrument, you should probably zero
    // out the input channels first to avoid any accidental noise. If you
    // are processing an effect, you should probably zero the values in the
    // output channels. See the silenceChannel() function below.
    // However, if you are reading input data from file (or elsewhere), this
    // step is not necessary.
    silenceChannel(ins, numChannels, numFrames);

    plugin->processReplacing(plugin, ins, outs, numFrames);
}

struct PlugData {
    std::string m_filepath;
    std::string m_description;
};

namespace vst {
class Plug;
namespace detail {
    static inline Plug* g_current_plug{nullptr};
    using plugs_by_effect_ptr_type = std::unordered_map<std::ptrdiff_t, Plug*>;
    inline plugs_by_effect_ptr_type plug_map;

} // namespace detail
class Plug {
    AEffect* m_effect; // MUST be first!

    PlugData m_data;
    std::string m_serr;
    Plug() = default; // note it is private. And no, you may not call this one!
    void addToMap() {
        auto peff = (std::ptrdiff_t)m_effect;
        detail::plug_map.insert({peff, this});
    }
    void removeFromMap() {
        auto it = detail::plug_map.find((uintptr_t)m_effect);
        assert(it != detail::plug_map.end());
        detail::plug_map.erase(it);
    }

    public:
    inline auto is_valid() const noexcept {
        return m_effect && m_effect->magic == kEffectMagic;
    }
    std::string_view description() const noexcept { return m_data.m_description; }
    static inline auto does_not_work
        = "C:\\Program Files (x86)\\REAPER\\Plugins\\FX\\reagate.dll";
    static inline auto dll_path
        = "C:\\Users\\DevNVME\\Documents\\VSTPlugins\\reaxcomp-standalone.dll";
    static inline auto dlll_path
        = "C:\\Users\\DevNVME\\Documents\\VSTPlugins\\Auburn Sounds Graillon 2.dll";

    CRect getUIRect() const noexcept {
        assert(m_effect);
        auto effect = m_effect;
        ERect* rect{nullptr};
        effect->dispatcher(effect, effEditGetRect, 0, 0, &rect, 0);
        CRect myrect(rect->left, rect->top, rect->right, rect->bottom);
        return myrect;
    }

    Plug(HWND hTab, HWND hTabControl, std::string_view filepath = dll_path) : Plug() {
        detail::g_current_plug = this;
        m_data.m_filepath = filepath;
        m_data.m_description = filepath; // set as temporary so we know who we are in the
                                         // callback, which my be called durn creation

        m_effect = loadPlugin(filepath.data());
        if (!m_effect) {
            m_serr = "Failed to load plugin: ";
            m_serr += filepath;
            return;
        }
        m_data.m_description = getEffectName(m_effect);
        if (m_data.m_description.empty()) {
            m_data.m_description = parseFilename(filepath);
        }
        addToMap();
        detail::g_current_plug = nullptr;

        configurePlugin(m_effect);
        startPlugin(m_effect);
        char paramName[256] = {0};
        char paramLabel[256] = {0};
        char paramDisplay[256] = {0};
        auto effect = m_effect;
        for (VstInt32 paramIndex = 0; paramIndex < effect->numParams; paramIndex++) {

            effect->dispatcher(effect, effGetParamName, paramIndex, 0, paramName, 0);
            effect->dispatcher(effect, effGetParamLabel, paramIndex, 0, paramLabel, 0);
            effect->dispatcher(
                effect, effGetParamDisplay, paramIndex, 0, paramDisplay, 0);
            float value = effect->getParameter(effect, paramIndex);

            TRACE("Param %03d: %s [%s %s] (normalized = %f)\n", paramIndex, paramName,
                paramDisplay, paramLabel, value);
        };

        effect->dispatcher(effect, effEditOpen, 0, 0, hTab, 0);
        // reaplugs always seem to return 400x300, or some too-small Rect: wtf?
        static constexpr auto border_h = 150;
        static constexpr auto border_w = 45;
        CRect siteRect;
        ::GetClientRect(hTabControl, &siteRect);

        CRect plugRect = getUIRect();

        ::SetWindowPos(hTabControl, 0, 0, 0, plugRect.Width() + border_w,
            plugRect.Height() + border_h, SWP_NOMOVE);
        // ::SetWindowPos(hTab, 0, 0, 0, plugRect.Width() + border_w,
        //    plugRect.Height() + border_h, SWP_NOMOVE);

        ::RedrawWindow(hTabControl, 0, 0, RDW_INVALIDATE);

        CRect plugRect2 = getUIRect();
        // find pluginwindow:
        int count = 0;
        HWND child = hTab;
        HWND plugEditor = 0;

        for (child = ::GetWindow(child, GW_CHILD); child;
             child = ::GetWindow(child, GW_HWNDNEXT)) {
            count++;
            char buf[MAX_PATH];
            ::GetWindowTextA(child, buf, MAX_PATH);
            CRect r;
            ::GetWindowRect(child, &r);
            TRACE("Got window text: %s\n", buf);
            if (r.Width() == plugRect2.Width() && r.Height() == plugRect2.Height()) {
                // is this needed? there seems to be only one child
                plugEditor = child;
                auto pt = r.TopLeft();
                ScreenToClient(child, &pt);
                pt.y += 50;
                ::SetWindowPos(child, 0, 0, pt.y, 0, 0, SWP_NOSIZE);

                break;
            }
        }
        checkEffectProperties(m_effect, m_presets);

        if (::loadChunkData(preset_path(), m_effect)) {
            m_presets.clear();
            checkEffectProperties(m_effect, m_presets);
        }
    }

    Plug(const Plug& rhs) {
        using std::swap;
        m_data = rhs.m_data;
        assert(0);
    }

    ~Plug() { ::saveChunkData(preset_path(), m_effect, m_currentPreset); }

    int m_currentPreset{0};
    std::vector<std::string> m_presets;
    std::string preset_path() const noexcept { return m_data.m_description + ".fxb"; }
};

namespace detail {

    inline Plug* find_plug(AEffect* eff) {
        auto it = plug_map.find((std::uintptr_t(eff)));
        if (it != plug_map.end()) {
            return it->second;
        }
        return nullptr;
    }
} // namespace detail

} // namespace vst

extern "C" {
inline VstIntPtr VSTCALLBACK hostCallback(AEffect* effect, VstInt32 opcode,
    VstInt32 index, VstInt32 value, void* ptr, float opt) {

    VstIntPtr result = 0;
    vst::Plug* plugptr = vst::detail::find_plug(effect);
    if (!plugptr) {
        plugptr = vst::detail::g_current_plug;
    } else {
        assert(plugptr);
        bool ok = plugptr->is_valid();
        ASSERT(ok);
        if (!ok) return 0;
    }

    const auto name = plugptr->description();
    const auto pluginIdString = name.data();

    if (opcode < 0) {
        TRACE("plugin sent some invalid (negative) value %d\n", opcode);
        return 0;
    }

    switch (opcode) {

        case __audioMasterWantMidiDeprecated: {
            TRACE(
                "Plugin %s wants midi. Tuff Shit. This is not something we worry about\n",
                name.data());
            break;
        }
        case audioMasterAutomate:
            // The plugin will call this if a parameter has changed via MIDI or the GUI,
            // so the host can update itself accordingly. We don't care about this (for
            // the time being), and as we don't support either GUI's or live MIDI, this
            // opcode can be ignored.
            break;

        case audioMasterVersion:
            // We are a VST 2.4 compatible host
            result = 2400;
            break;

        case audioMasterCurrentId:
            // Use the current plugin ID, needed by VST shell plugins to determine which
            // sub-plugin to load
            result = currentPluginUniqueId;
            break;

        case audioMasterIdle:
            // Ignore
            result = 1;
            break;

        case audioMasterPinConnected:
            logDeprecated("audioMasterPinConnected", pluginIdString);
            break;

        case audioMasterGetTime: {
            // AudioClock audioClock = getAudioClock();
            assert(0);
            // These values are always valid
            // vstTimeInfo.samplePos = audioClock->currentFrame;
            // vstTimeInfo.sampleRate = getSampleRate();

            // Set flags for transport state
            vstTimeInfo.flags = 0;
            // vstTimeInfo.flags |= audioClock->transportChanged ? kVstTransportChanged :
            // 0; vstTimeInfo.flags |= audioClock->isPlaying ? kVstTransportPlaying : 0;

            // Fill values based on other flags which may have been requested
            if (value & kVstNanosValid) {
                // It doesn't make sense to return this value, as the plugin may try to
                // calculate something based on the current system time. As we are running
                // offline, anything the plugin calculates here will probably be wrong
                // given the way we are running.  However, for real-time mode, this flag
                // should be implemented in that case.
                TRACE("Plugin '%s' asked for time in nanoseconds (unsupported)\n",
                    pluginIdString);
            }

            /*/
            if (value & kVstPpqPosValid) {
                // TODO: Move calculations to AudioClock
                double samplesPerBeat = (60.0 / getTempo()) * getSampleRate();
                // Musical time starts with 1, not 0
                vstTimeInfo.ppqPos = (vstTimeInfo.samplePos / samplesPerBeat) + 1.0;
                logDebug("Current PPQ position is %g", vstTimeInfo.ppqPos);
                vstTimeInfo.flags |= kVstPpqPosValid;
            }

            if (value & kVstTempoValid) {
                vstTimeInfo.tempo = getTempo();
                vstTimeInfo.flags |= kVstTempoValid;
            }

            if (value & kVstBarsValid) {
                if (!(value & kVstPpqPosValid)) {
                    logError("Plugin requested position in bars, but not PPQ");
                }

                // TODO: Move calculations to AudioClock
                double currentBarPos = floor(
                    vstTimeInfo.ppqPos / (double)getTimeSignatureBeatsPerMeasure());
                vstTimeInfo.barStartPos
                    = currentBarPos * (double)getTimeSignatureBeatsPerMeasure() + 1.0;
                logDebug("Current bar is %g", vstTimeInfo.barStartPos);
                vstTimeInfo.flags |= kVstBarsValid;
            }

            if (value & kVstCyclePosValid) {
                // We don't support cycling, so this is always 0
            }

            if (value & kVstTimeSigValid) {
                vstTimeInfo.timeSigNumerator = getTimeSignatureBeatsPerMeasure();
                vstTimeInfo.timeSigDenominator = getTimeSignatureNoteValue();
                vstTimeInfo.flags |= kVstTimeSigValid;
            }

            if (value & kVstSmpteValid) {
                puts("Current time in SMPTE format");
            }

            if (value & kVstClockValid) {
                puts("Sample frames until next clock");
            }

            result = (VstIntPtr)&vstTimeInfo;
            /*/
            break;
        }

        case audioMasterProcessEvents:
            puts("VST master opcode audioMasterProcessEvents");
            break;

        case audioMasterIOChanged: {
            assert(0);
            /*/
            if (effect != NULL) {
                PluginChain pluginChain = getPluginChain();
                logDebug("Number of inputs: %d", effect->numInputs);
                logDebug("Number of outputs: %d", effect->numOutputs);
                logDebug("Number of parameters: %d", effect->numParams);
                logDebug("Initial Delay: %d", effect->initialDelay);
                result = -1;

                for (unsigned int i = 0; i < pluginChain->numPlugins; ++i) {
                    if ((unsigned long)effect->uniqueID
                        == pluginVst2xGetUniqueId(pluginChain->plugins[i])) {
                        logDebug("Updating plugin");
                        pluginVst2xAudioMasterIOChanged(pluginChain->plugins[i], effect);
                        result = 0;
                        break; // Only one plugin will match anyway.
                    }
                }

                break;
            }
            /*/
        }

        case audioMasterSizeWindow:
            TRACE(
                "Plugin '%s' asked us to resize window (unsupported)\n", pluginIdString);
            break;

        case audioMasterGetSampleRate: result = (int)getSampleRate(); break;

        case audioMasterGetBlockSize: result = (VstIntPtr)getBlocksize(); break;

        case audioMasterGetInputLatency:
            // Input latency is not used, and is always 0
            result = 0;
            break;

        case audioMasterGetOutputLatency:
            // Output latency is not used, and is always 0
            result = 0;
            break;

        case audioMasterGetCurrentProcessLevel:
            // We are not a multi-threaded app and have no GUI, so this is unsupported
            result = kVstProcessLevelUnknown;
            break;

        case audioMasterGetAutomationState:
            // Automation is also not supported (for now)
            result = kVstAutomationUnsupported;
            break;

        case audioMasterOfflineStart:
            TRACE("Plugin '%s' asked us to start offline processing (unsupported)\n",
                pluginIdString);
            break;

        case audioMasterOfflineRead:
            logWarn(
                "Plugin '%s' asked to read offline data (unsupported)\n", pluginIdString);
            break;

        case audioMasterOfflineWrite:
            logWarn("Plugin '%s' asked to write offline data (unsupported)\n",
                pluginIdString);
            break;

        case audioMasterOfflineGetCurrentPass:
            logWarn("Plugin '%s' asked for current offline pass (unsupported)\n",
                pluginIdString);
            break;

        case audioMasterOfflineGetCurrentMetaPass:
            logWarn("Plugin '%s' asked for current offline meta pass (unsupported)\n",
                pluginIdString);
            break;

        case audioMasterGetVendorString:
            strncpy((char*)ptr, VENDOR_NAME, kVstMaxVendorStrLen);
            result = 1;
            break;

        case audioMasterGetProductString:
            strncpy((char*)ptr, PROGRAM_NAME, kVstMaxProductStrLen);
            result = 1;
            break;

        case audioMasterGetVendorVersion:
            // Return our version as a single string, in the form ABCC, which
            // corresponds to version A.B.C
            // Often times the patch can reach double-digits, so it gets two decimal
            // places.
            result = VERSION_MAJOR * 1000 + VERSION_MINOR * 100 + VERSION_PATCH;
            break;

        case audioMasterVendorSpecific:
            logWarn("Plugin '%s' made a vendor specific call (unsupported). Arguments: "
                    "%d, %d, %f\n",
                pluginIdString, index, value, opt);
            break;

        case audioMasterCanDo: {
            assert(0);
            /// result = _canHostDo(pluginIdString, (char*)ptr);
            break;
        }

        case audioMasterGetDirectory:
            logWarn("Plugin '%s' asked for directory pointer (unsupported)\n",
                pluginIdString);
            break;

        case audioMasterUpdateDisplay:
            // Ignore
            break;

        case audioMasterBeginEdit:
            logWarn("Plugin '%s' asked to begin parameter automation (unsupported)\n",
                pluginIdString);
            break;

        case audioMasterEndEdit:
            logWarn("Plugin '%s' asked to end parameter automation (unsupported)\n",
                pluginIdString);
            break;

        case audioMasterOpenFileSelector:
            logWarn("Plugin '%s' asked us to open file selector (unsupported)\n",
                pluginIdString);
            break;

        case audioMasterCloseFileSelector:
            logWarn("Plugin '%s' asked us to close file selector (unsupported)\n",
                pluginIdString);
            break;

        default:
            logWarn("Plugin '%s' asked if host can do unknown opcode %d\n", name.data(),
                opcode);
            break;
    }

    // freePluginVst2xId(pluginId);
    return result;
}
}
