#pragma once
// VST.h
#include "../Helpers/vstsdk2.4/public.sdk/source/vst2.x/audioeffectx.h"
#include "../Helpers/vstsdk2.4/public.sdk/source/vst2.x/aeffeditor.h"
#include <utility>
#include <string>
#include <cassert>

#ifndef logWarn
#define logWarn TRACE
#endif

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
        printf(
            "Failed trying to load VST from '%s', error %d\n", vstPath, GetLastError());
        return NULL;
    }

    vstPluginFuncPtr mainEntryPoint
        = (vstPluginFuncPtr)GetProcAddress(modulePtr, "VSTPluginMain");
    // Instantiate the plugin
    plugin = mainEntryPoint(hostCallback);
    return plugin;
}

static inline AEffect* configurePluginCallbacks(
    AEffect* plugin, dispatcherFuncPtr* dispatcher) {
    // Check plugin's magic number
    // If incorrect, then the file either was not loaded properly, is not a
    // real VST plugin, or is otherwise corrupt.
    if (plugin->magic != kEffectMagic) {
        TRACE("Plugin's magic number is bad\n");
        return nullptr;
    }

    // Create dispatcher handle
    *dispatcher = (dispatcherFuncPtr)(plugin->dispatcher);

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

static inline void startPlugin(AEffect* plugin, dispatcherFuncPtr dispatcher) {
    dispatcher(plugin, effOpen, 0, 0, NULL, 0.0f);

    // Set some default properties
    float sampleRate = 44100.0f;
    dispatcher(plugin, effSetSampleRate, 0, 0, NULL, sampleRate);
    int blocksize = 512;
    dispatcher(plugin, effSetBlockSize, 0, blocksize, NULL, 0.0f);

    resumePlugin(plugin, dispatcher);
}

static inline auto currentPluginUniqueId = 77;
static inline auto pluginIdString = "pluginidString";
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

extern "C" {
inline VstIntPtr VSTCALLBACK hostCallback(AEffect* effect, VstInt32 opcode,
    VstInt32 index, VstInt32 value, void* ptr, float opt) {

    VstIntPtr result = 0;
    assert(0);

    switch (opcode) {
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
                TRACE("Plugin '%s' asked for time in nanoseconds (unsupported)",
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
            TRACE("Plugin '%s' asked us to resize window (unsupported)", pluginIdString);
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
            TRACE("Plugin '%s' asked us to start offline processing (unsupported)",
                pluginIdString);
            break;

        case audioMasterOfflineRead:
            logWarn(
                "Plugin '%s' asked to read offline data (unsupported)", pluginIdString);
            break;

        case audioMasterOfflineWrite:
            logWarn(
                "Plugin '%s' asked to write offline data (unsupported)", pluginIdString);
            break;

        case audioMasterOfflineGetCurrentPass:
            logWarn("Plugin '%s' asked for current offline pass (unsupported)",
                pluginIdString);
            break;

        case audioMasterOfflineGetCurrentMetaPass:
            logWarn("Plugin '%s' asked for current offline meta pass (unsupported)",
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
                    "%d, %d, %f",
                pluginIdString, index, value, opt);
            break;

        case audioMasterCanDo: {
            assert(0);
            /// result = _canHostDo(pluginIdString, (char*)ptr);
            break;
        }

        case audioMasterGetDirectory:
            logWarn(
                "Plugin '%s' asked for directory pointer (unsupported)", pluginIdString);
            break;

        case audioMasterUpdateDisplay:
            // Ignore
            break;

        case audioMasterBeginEdit:
            logWarn("Plugin '%s' asked to begin parameter automation (unsupported)",
                pluginIdString);
            break;

        case audioMasterEndEdit:
            logWarn("Plugin '%s' asked to end parameter automation (unsupported)",
                pluginIdString);
            break;

        case audioMasterOpenFileSelector:
            logWarn("Plugin '%s' asked us to open file selector (unsupported)",
                pluginIdString);
            break;

        case audioMasterCloseFileSelector:
            logWarn("Plugin '%s' asked us to close file selector (unsupported)",
                pluginIdString);
            break;

        default:
            logWarn("Plugin '%s' asked if host can do unknown opcode %d", pluginIdString,
                opcode);
            break;
    }

    // freePluginVst2xId(pluginId);
    return result;
}
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

namespace vst {
class Plug {

    struct PlugData {

        std::string m_filepath;
        std::string m_description;
        AEffect* m_plugin{nullptr};
        dispatcherFuncPtr m_dispatcher{nullptr};
    };
    PlugData m_data;

    public:
    Plug(HWND h,
        std::string_view filepath
        = "C:\\Users\\DevNVME\\Documents\\VSTPlugins\\reagate-standalone.dll") {
        m_data.m_filepath = filepath;
        m_data.m_plugin = loadPlugin(filepath.data());
        configurePluginCallbacks(m_data.m_plugin, &m_data.m_dispatcher);
        startPlugin(m_data.m_plugin, m_data.m_dispatcher);
        char paramName[256] = {0};
        char paramLabel[256] = {0};
        char paramDisplay[256] = {0};
        auto effect = m_data.m_plugin;
        for (VstInt32 paramIndex = 0; paramIndex < m_data.m_plugin->numParams;
             paramIndex++) {
            char paramName[256] = {0};
            char paramLabel[256] = {0};
            char paramDisplay[256] = {0};

            effect->dispatcher(effect, effGetParamName, paramIndex, 0, paramName, 0);
            effect->dispatcher(effect, effGetParamLabel, paramIndex, 0, paramLabel, 0);
            effect->dispatcher(
                effect, effGetParamDisplay, paramIndex, 0, paramDisplay, 0);
            float value = effect->getParameter(effect, paramIndex);

            printf("Param %03d: %s [%s %s] (normalized = %f)\n", paramIndex, paramName,
                paramDisplay, paramLabel, value);
        };

        auto x = effect->dispatcher(effect, effEditOpen, 0, 0, h, 0);
        TRACE("shown?\n");
    }
    Plug(const Plug& rhs) {
        using std::swap;
        m_data = rhs.m_data;
        assert(0);
    }
};

} // namespace vst
