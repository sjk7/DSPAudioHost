#pragma once
// db_conversions.h
#include <algorithm>
static const double TINY = double(0.000000001);

template <typename T, typename U>
static constexpr bool TEST_EQUALITY(const T& a, const U& b) {

    if (abs(a - b) <= TINY) {
        return true;
    }
    return false;
}

namespace cpp {
namespace audio_helpers {
    static const double MINUS_THIRTY_FIVE_DB = 0.017783;
    static const double TEN_DB_DOWN = 0.316228;
    static const double SIX_DB_DOWN = 0.5;
    static const double TWENTY_DB_DOWN = 0.1;
    static const double THIRTY_DB_DOWN = 0.031623;
    static const double THIRTYFIVE_DB_DOWN = 0.017783;
    static constexpr double FORTY_DB_DOWN = 0.01;

    template <typename SAMP_COUNT_1, typename SAMP_COUNT_2, typename SR>
    float samples_to_ms(SAMP_COUNT_1 s1, SAMP_COUNT_2 s2, SR samplerate) {
        int count = (int)s2 - (int)s1;
        ASSERT(s2 >= s1);
        float ret = (float)count / (float)samplerate;
        ret *= 1000.0;
        return ret;
    }

    struct false_pred {
        inline bool operator()(size_t) { return false; }
    };

    template <typename I, typename IT, typename T, typename PRED = false_pred>
    size_t count_over_samps(
        double gain, I begin, IT end, const T reference_value, PRED pred = false_pred()) {

        size_t count = 0;

        while (begin != end) {
            const auto& val = *begin;
            auto m = (std::max)(val[0], val[1]);
            if (m * gain > reference_value) {
                if (pred(++count)) break;
            }
            ++begin;
        }
        return count;
    }

    // Returns a linear value.
    template <typename T> static inline T db_to_value(T dB) {
        dB = -dB;
        T g = (T)pow(10, dB / 20);
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

    // get how many db down val is wrt ref
    // Returns a dB value.
    //// e.g: get how many db down 15000 is on a max of 32768:
    // my_db_down = db(15000/32768, 32768) == -6 (approx.)
    template <typename T>
    static inline double db(const T val, T ref = MINUS_THIRTY_FIVE_DB) noexcept {

        T retval = 0;

        T ratio = val / ref;
        if (ratio <= 0) {
            ratio = (T)TINY;
        }

        T d = 20 * log10(ratio);
        retval = d;

        return retval;
    }

    template <typename T> static inline T take_antilog(const T val) {

        return (T)pow(10, val);
    }

    // convert, for example 32768 to be 1
    template <typename T>
    static inline double normalize_value(double val, const T ref = 1.0) {
        static double max = abs((std::numeric_limits<T>::min)());
        double ret = (val / max) * ref;
        return ret;
    }
} // namespace audio_helpers
namespace audio {
    static constexpr auto min_release_multiplier = 0.09f;
    //-------------------------------------------------------------
    // gain functions
    //-------------------------------------------------------------
    static constexpr double TINY_NUMBER = 0.000000001f;
    // linear -> dB conversion
    template <typename T> static inline constexpr T lin2dB(T lin) noexcept {
        if (lin < TINY_NUMBER) lin = TINY_NUMBER;
        constexpr T LOG_2_DB = (T)8.6858896380650365530225783783321; // 20 / ln( 10 )
        return T(log(lin) * LOG_2_DB);
    }

    // dB -> linear conversion
    template <typename T> static inline constexpr T dB2lin(T dB) {
        constexpr T DB_2_LOG = (T)0.11512925464970228420089957273422; // ln( 10 ) / 20
        return T(exp(dB * DB_2_LOG));
    }

    template <typename T>
    static inline void envelope_val(long& l, long& r, const int nch, const T env_l,
        const T env_r, const long max = 10000, const long min_display_db = 35) {

        double left = audio_helpers::normalize_value<T>(env_l);
        double right = audio_helpers::normalize_value<T>(env_r);
        double db = lin2dB(left);
        left = max + (max * (db / min_display_db));
        if (left < 0) {
            left = 0;
        }

        if (nch < 2) {
            right = left;
        } else {
            db = lin2dB(right);
            right = max + (max * (db / min_display_db));
            if (right < 0) {
                right = 0;
            }
        }

        l = (long)(left);
        r = (long)(right);
    }

    template <typename T>
    static inline void envelope_val(T& l, T& r, const int nch, const T env_l,
        const T env_r, const long max = 10000, const long min_display_db = 35) {
        static_assert(std::is_same<T, double>::value,
            "envelope_val (T) should be of samp_t type to avoid a ton of casts");
        T db = cpp::audio::lin2dB(env_l);
        l = max + (max * (db / min_display_db));
        if (l < 0) {
            l = 0;
        }

        if (nch < 2) {
            r = l;
        } else {
            db = cpp::audio::lin2dB(env_r);
            r = max + (max * (db / min_display_db));
            if (r < 0) {
                r = 0;
            }
        }
    }
} // namespace audio
} // namespace cpp
