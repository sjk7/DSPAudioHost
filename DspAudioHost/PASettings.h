// This is an open source non-commercial project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com
#pragma once
#include "DspAudioHost.h"
#include <string>
#include <string_view>

struct PASettings {
    static inline constexpr auto SECTION = L"PortAudio";
    static inline constexpr auto API_ENTRY = L"Api";
    static inline constexpr auto INPUT_ENTRY = L"Input";
    static inline constexpr auto OUTPUT_ENTRY = L"Output";
    static inline constexpr auto PP_ENTRY = L"Process Priority";
    static inline constexpr auto SR_ENTRY = L"Samplerate";

    PASettings(bool loadAll = false) : m_pri(0) {
        if (loadAll) getAll();
    }
    ~PASettings() { saveAll(); }
    void saveAll() {
        theApp.WriteProfileStringW(SECTION, API_ENTRY, m_api);
        theApp.WriteProfileStringW(SECTION, INPUT_ENTRY, m_input);
        theApp.WriteProfileStringW(SECTION, OUTPUT_ENTRY, m_output);
        theApp.WriteProfileInt(SECTION, PP_ENTRY, m_pri);
        theApp.WriteProfileInt(SECTION, SR_ENTRY, samplerate);
    }
    void getAll() {
        m_api = theApp.GetProfileStringW(SECTION, API_ENTRY);
        m_input = theApp.GetProfileStringW(SECTION, INPUT_ENTRY);
        m_output = theApp.GetProfileStringW(SECTION, OUTPUT_ENTRY);
        m_pri = theApp.GetProfileInt(SECTION, PP_ENTRY, HIGH_PRIORITY_CLASS);
        samplerate = theApp.GetProfileInt(SECTION, SR_ENTRY, 44100);
    }

    std::string api() const {
        CStringA s(m_api);
        return std::string(s.GetBuffer());
    }

    std::string input() const {
        CStringA s(m_input);
        return s.GetBuffer();
    }

    std::string output() const {
        CStringA s(m_output);
        return s.GetBuffer();
    }

    int priority() const { return m_pri; }

    void priority(int pri) { m_pri = pri; }

    CString m_api;
    CString m_input;
    CString m_output;
    unsigned int samplerate{44100};
    int m_pri = 0;
};
