/*
  ==============================================================================

    MicroSynth.h
    Created: 3 Nov 2021 2:05:33pm
    Author:  Thom Johansen

  ==============================================================================
*/

#pragma once
#include <algorithm>

    enum FilterMode {
        FILTER_MODE_LOW_PASS,
        FILTER_MODE_BAND_PASS,
        FILTER_MODE_BAND_PASS_NORMALIZED,
        FILTER_MODE_HIGH_PASS
    };

    enum FrequencyApproximation {
        FREQUENCY_EXACT,
        FREQUENCY_ACCURATE,
        FREQUENCY_FAST,
        FREQUENCY_DIRTY
    };

#define M_PI 3.14159265359f
#define M_PI_F float(M_PI)
#define M_PI_POW_2 M_PI * M_PI
#define M_PI_POW_3 M_PI_POW_2 * M_PI
#define M_PI_POW_5 M_PI_POW_3 * M_PI_POW_2
#define M_PI_POW_7 M_PI_POW_5 * M_PI_POW_2
#define M_PI_POW_9 M_PI_POW_7 * M_PI_POW_2
#define M_PI_POW_11 M_PI_POW_9 * M_PI_POW_2

class DCBlocker {
    public:
        DCBlocker() { }
        ~DCBlocker() { }

        void Init(float pole) {
            x_ = 0.0f;
            y_ = 0.0f;
            pole_ = pole;
        }

        inline void Process(float* in_out, size_t size) {
            float x = x_;
            float y = y_;
            const float pole = pole_;
            while (size--) {
                float old_x = x;
                x = *in_out;
                *in_out++ = y = y * pole + x - old_x;
            }
            x_ = x;
            y_ = y;
        }

    private:
        float pole_;
        float x_;
        float y_;
    };

class OnePole {
    public:
        OnePole() { }
        ~OnePole() { }

        void Init() {
            set_f<FREQUENCY_DIRTY>(0.01f);
            Reset();
        }

        void Reset() {
            state_ = 0.0f;
        }

        template<FrequencyApproximation approximation>
        static inline float tan(float f) {
            if (approximation == FREQUENCY_EXACT) {
                // Clip coefficient to about 100.
                f = f < 0.497f ? f : 0.497f;
                return tanf(M_PI * f);
            }
            else if (approximation == FREQUENCY_DIRTY) {
                // Optimized for frequencies below 8kHz.
                const float a = 3.736e-01 * M_PI_POW_3;
                return f * (M_PI_F + a * f * f);
            }
            else if (approximation == FREQUENCY_FAST) {
                // The usual tangent approximation uses 3.1755e-01 and 2.033e-01, but
                // the coefficients used here are optimized to minimize error for the
                // 16Hz to 16kHz range, with a sample rate of 48kHz.
                const float a = 3.260e-01f * M_PI_POW_3;
                const float b = 1.823e-01f * M_PI_POW_5;
                float f2 = f * f;
                return f * (M_PI_F + f2 * (a + b * f2));
            }
            else if (approximation == FREQUENCY_ACCURATE) {
                // These coefficients don't need to be tweaked for the audio range.
                const float a = 3.333314036e-01f * M_PI_POW_3;
                const float b = 1.333923995e-01f * M_PI_POW_5;
                const float c = 5.33740603e-02f * M_PI_POW_7;
                const float d = 2.900525e-03f * M_PI_POW_9;
                const float e = 9.5168091e-03f * M_PI_POW_11;
                float f2 = f * f;
                return f * (M_PI_F + f2 * (a + f2 * (b + f2 * (c + f2 * (d + f2 * e)))));
            }
        }

        // Set frequency and resonance from true units. Various approximations
        // are available to avoid the cost of tanf.
        template<FrequencyApproximation approximation>
        inline void set_f(float f) {
            g_ = tan<approximation>(f);
            gi_ = 1.0f / (1.0f + g_);
        }

        template<FilterMode mode>
        inline float Process(float in) {
            float lp;
            lp = (g_ * in + state_) * gi_;
            state_ = g_ * (in - lp) + lp;

            if (mode == FILTER_MODE_LOW_PASS) {
                return lp;
            }
            else if (mode == FILTER_MODE_HIGH_PASS) {
                return in - lp;
            }
            else {
                return 0.0f;
            }
        }

        template<FilterMode mode>
        inline void Process(float* in_out, size_t size) {
            while (size--) {
                *in_out = Process<mode>(*in_out);
                ++in_out;
            }
        }

    private:
        float g_;
        float gi_;
        float state_;
    };

class Svf {
    public:
        Svf() { }
        ~Svf() { }

        void Init() {
            set_f_q<FREQUENCY_DIRTY>(0.01f, 100.0f);
            Reset();
        }

        void Reset() {
            state_1_ = state_2_ = 0.0f;
        }

        // Copy settings from another filter.
        inline void set(const Svf& f) {
            g_ = f.g();
            r_ = f.r();
            h_ = f.h();
        }

        // Set all parameters from LUT.
        inline void set_g_r_h(float g, float r, float h) {
            g_ = g;
            r_ = r;
            h_ = h;
        }

        // Set frequency and resonance coefficients from LUT, adjust remaining
        // parameter.
        inline void set_g_r(float g, float r) {
            g_ = g;
            r_ = r;
            h_ = 1.0f / (1.0f + r_ * g_ + g_ * g_);
        }

        // Set frequency from LUT, resonance in true units, adjust the rest.
        inline void set_g_q(float g, float resonance) {
            g_ = g;
            r_ = 1.0f / resonance;
            h_ = 1.0f / (1.0f + r_ * g_ + g_ * g_);
        }

        // Set frequency and resonance from true units. Various approximations
        // are available to avoid the cost of tanf.
        template<FrequencyApproximation approximation>
        inline void set_f_q(float f, float resonance) {
            g_ = OnePole::tan<approximation>(f);
            r_ = 1.0f / resonance;
            h_ = 1.0f / (1.0f + r_ * g_ + g_ * g_);
        }

        template<FilterMode mode>
        inline float Process(float in) {
            float hp, bp, lp;
            hp = (in - r_ * state_1_ - g_ * state_1_ - state_2_) * h_;
            bp = g_ * hp + state_1_;
            state_1_ = g_ * hp + bp;
            lp = g_ * bp + state_2_;
            state_2_ = g_ * bp + lp;

            if (mode == FILTER_MODE_LOW_PASS) {
                return lp;
            }
            else if (mode == FILTER_MODE_BAND_PASS) {
                return bp;
            }
            else if (mode == FILTER_MODE_BAND_PASS_NORMALIZED) {
                return bp * r_;
            }
            else if (mode == FILTER_MODE_HIGH_PASS) {
                return hp;
            }
        }

        template<FilterMode mode_1, FilterMode mode_2>
        inline void Process(float in, float* out_1, float* out_2) {
            float hp, bp, lp;
            hp = (in - r_ * state_1_ - g_ * state_1_ - state_2_) * h_;
            bp = g_ * hp + state_1_;
            state_1_ = g_ * hp + bp;
            lp = g_ * bp + state_2_;
            state_2_ = g_ * bp + lp;

            if (mode_1 == FILTER_MODE_LOW_PASS) {
                *out_1 = lp;
            }
            else if (mode_1 == FILTER_MODE_BAND_PASS) {
                *out_1 = bp;
            }
            else if (mode_1 == FILTER_MODE_BAND_PASS_NORMALIZED) {
                *out_1 = bp * r_;
            }
            else if (mode_1 == FILTER_MODE_HIGH_PASS) {
                *out_1 = hp;
            }

            if (mode_2 == FILTER_MODE_LOW_PASS) {
                *out_2 = lp;
            }
            else if (mode_2 == FILTER_MODE_BAND_PASS) {
                *out_2 = bp;
            }
            else if (mode_2 == FILTER_MODE_BAND_PASS_NORMALIZED) {
                *out_2 = bp * r_;
            }
            else if (mode_2 == FILTER_MODE_HIGH_PASS) {
                *out_2 = hp;
            }
        }

        template<FilterMode mode>
        inline void Process(const float* in, float* out, size_t size) {
            float hp, bp, lp;
            float state_1 = state_1_;
            float state_2 = state_2_;

            while (size--) {
                hp = (*in - r_ * state_1 - g_ * state_1 - state_2) * h_;
                bp = g_ * hp + state_1;
                state_1 = g_ * hp + bp;
                lp = g_ * bp + state_2;
                state_2 = g_ * bp + lp;

                float value;
                if (mode == FILTER_MODE_LOW_PASS) {
                    value = lp;
                }
                else if (mode == FILTER_MODE_BAND_PASS) {
                    value = bp;
                }
                else if (mode == FILTER_MODE_BAND_PASS_NORMALIZED) {
                    value = bp * r_;
                }
                else if (mode == FILTER_MODE_HIGH_PASS) {
                    value = hp;
                }

                *out = value;
                ++out;
                ++in;
            }
            state_1_ = state_1;
            state_2_ = state_2;
        }

        template<FilterMode mode>
        inline void ProcessAdd(const float* in, float* out, size_t size, float gain) {
            float hp, bp, lp;
            float state_1 = state_1_;
            float state_2 = state_2_;

            while (size--) {
                hp = (*in - r_ * state_1 - g_ * state_1 - state_2) * h_;
                bp = g_ * hp + state_1;
                state_1 = g_ * hp + bp;
                lp = g_ * bp + state_2;
                state_2 = g_ * bp + lp;

                float value;
                if (mode == FILTER_MODE_LOW_PASS) {
                    value = lp;
                }
                else if (mode == FILTER_MODE_BAND_PASS) {
                    value = bp;
                }
                else if (mode == FILTER_MODE_BAND_PASS_NORMALIZED) {
                    value = bp * r_;
                }
                else if (mode == FILTER_MODE_HIGH_PASS) {
                    value = hp;
                }

                *out += gain * value;
                ++out;
                ++in;
            }
            state_1_ = state_1;
            state_2_ = state_2;
        }

        template<FilterMode mode>
        inline void Process(const float* in, float* out, size_t size, size_t stride) {
            float hp, bp, lp;
            float state_1 = state_1_;
            float state_2 = state_2_;

            while (size--) {
                hp = (*in - r_ * state_1 - g_ * state_1 - state_2) * h_;
                bp = g_ * hp + state_1;
                state_1 = g_ * hp + bp;
                lp = g_ * bp + state_2;
                state_2 = g_ * bp + lp;

                float value;
                if (mode == FILTER_MODE_LOW_PASS) {
                    value = lp;
                }
                else if (mode == FILTER_MODE_BAND_PASS) {
                    value = bp;
                }
                else if (mode == FILTER_MODE_BAND_PASS_NORMALIZED) {
                    value = bp * r_;
                }
                else if (mode == FILTER_MODE_HIGH_PASS) {
                    value = hp;
                }

                *out = value;
                out += stride;
                in += stride;
            }
            state_1_ = state_1;
            state_2_ = state_2;
        }

        inline void ProcessMultimode(
            const float* in,
            float* out,
            size_t size,
            float mode) {
            float hp, bp, lp;
            float state_1 = state_1_;
            float state_2 = state_2_;
            float hp_gain = mode < 0.5f ? -mode * 2.0f : -2.0f + mode * 2.0f;
            float lp_gain = mode < 0.5f ? 1.0f - mode * 2.0f : 0.0f;
            float bp_gain = mode < 0.5f ? 0.0f : mode * 2.0f - 1.0f;
            while (size--) {
                hp = (*in - r_ * state_1 - g_ * state_1 - state_2) * h_;
                bp = g_ * hp + state_1;
                state_1 = g_ * hp + bp;
                lp = g_ * bp + state_2;
                state_2 = g_ * bp + lp;
                *out = hp_gain * hp + bp_gain * bp + lp_gain * lp;
                ++in;
                ++out;
            }
            state_1_ = state_1;
            state_2_ = state_2;
        }

        inline void ProcessMultimodeLPtoHP(
            const float* in,
            float* out,
            size_t size,
            float mode) {
            float hp, bp, lp;
            float state_1 = state_1_;
            float state_2 = state_2_;
            float hp_gain = std::min(-mode * 2.0f + 1.0f, 0.0f);
            float bp_gain = 1.0f - 2.0f * fabsf(mode - 0.5f);
            float lp_gain = std::max(1.0f - mode * 2.0f, 0.0f);
            while (size--) {
                hp = (*in - r_ * state_1 - g_ * state_1 - state_2) * h_;
                bp = g_ * hp + state_1;
                state_1 = g_ * hp + bp;
                lp = g_ * bp + state_2;
                state_2 = g_ * bp + lp;
                *out = hp_gain * hp + bp_gain * bp + lp_gain * lp;
                ++in;
                ++out;
            }
            state_1_ = state_1;
            state_2_ = state_2;
        }

        template<FilterMode mode>
        inline void Process(
            const float* in, float* out_1, float* out_2, size_t size,
            float gain_1, float gain_2) {
            float hp, bp, lp;
            float state_1 = state_1_;
            float state_2 = state_2_;

            while (size--) {
                hp = (*in - r_ * state_1 - g_ * state_1 - state_2) * h_;
                bp = g_ * hp + state_1;
                state_1 = g_ * hp + bp;
                lp = g_ * bp + state_2;
                state_2 = g_ * bp + lp;

                float value;
                if (mode == FILTER_MODE_LOW_PASS) {
                    value = lp;
                }
                else if (mode == FILTER_MODE_BAND_PASS) {
                    value = bp;
                }
                else if (mode == FILTER_MODE_BAND_PASS_NORMALIZED) {
                    value = bp * r_;
                }
                else if (mode == FILTER_MODE_HIGH_PASS) {
                    value = hp;
                }

                *out_1 += value * gain_1;
                *out_2 += value * gain_2;
                ++out_1;
                ++out_2;
                ++in;
            }
            state_1_ = state_1;
            state_2_ = state_2;
        }

        inline float g() const { return g_; }
        inline float r() const { return r_; }
        inline float h() const { return h_; }

    private:
        float g_;
        float r_;
        float h_;

        float state_1_;
        float state_2_;
    };




        enum EnvelopeShape {
            ENV_SHAPE_LINEAR,
            ENV_SHAPE_EXPONENTIAL,
            ENV_SHAPE_QUARTIC
        };

        enum EnvelopeFlags {
            ENVELOPE_FLAG_RISING_EDGE = 1,
            ENVELOPE_FLAG_FALLING_EDGE = 2,
            ENVELOPE_FLAG_GATE = 4
        };

        const uint16_t kMaxNumSegments = 6;

        class MultistageEnvelope {
        public:
            MultistageEnvelope() { }
            ~MultistageEnvelope() { }

            void Init();
            inline float Process(uint8_t flags) {
                done_ = false;
                if (flags & ENVELOPE_FLAG_RISING_EDGE) {
                    start_value_ = (segment_ == num_segments_ || hard_reset_)
                        ? level_[0]
                        : value_;
                    segment_ = 0;
                    phase_ = 0.0f;
                } else if (flags & ENVELOPE_FLAG_FALLING_EDGE && sustain_point_) {
                    start_value_ = value_;
                    segment_ = sustain_point_;
                    phase_ = 0.0f;
                } else if (phase_ >= 1.0f) {
                    start_value_ = level_[segment_ + 1];
                    ++segment_;
                    phase_ = 0.0f;
                    if (segment_ == loop_end_) {
                        segment_ = loop_start_;
                    }
                }

                bool done = segment_ == num_segments_;
                bool sustained = sustain_point_ && segment_ == sustain_point_ &&
                    flags & ENVELOPE_FLAG_GATE;

                float phase_increment = 0.0f;
                if (!sustained && !done) {
                    phase_increment = time_[segment_];
                } else if (!sustained && done) done_ = true;
                float t = phase_;
                phase_ += phase_increment;
                value_ = start_value_ + (level_[segment_ + 1] - start_value_) * t;
                return value_;
            }

            inline void Process(const uint8_t* flags_in, float* out, size_t size) {
                while (size--) {
                    *out++ = Process(*flags_in++);
                }
            }

            inline void set_time(uint16_t segment, float time) {
                time_[segment] = time;
            }

            inline void set_level(uint16_t segment, float level) {
                level_[segment] = level;
            }

            inline void set_num_segments(uint16_t num_segments) {
                num_segments_ = num_segments;
            }

            inline void set_sustain_point(float sustain_point) {
                sustain_point_ = sustain_point;
            }

            inline void set_adsr(
                float attack,
                float decay,
                float sustain,
                float release) {
                num_segments_ = 3;
                sustain_point_ = 2;

                level_[0] = 0.0f;
                level_[1] = 1.0f;
                level_[2] = sustain;
                level_[3] = 0.0f;

                time_[0] = attack;
                time_[1] = decay;
                time_[2] = release;

                shape_[0] = ENV_SHAPE_QUARTIC;
                shape_[1] = ENV_SHAPE_EXPONENTIAL;
                shape_[2] = ENV_SHAPE_EXPONENTIAL;

                loop_start_ = loop_end_ = 0;
            }

            inline void set_ad(float attack, float decay) {
                num_segments_ = 2;
                sustain_point_ = 0;

                level_[0] = 0.0f;
                level_[1] = 1.0f;
                level_[2] = 0.0f;

                time_[0] = attack;
                time_[1] = decay;

                shape_[0] = ENV_SHAPE_LINEAR;
                shape_[1] = ENV_SHAPE_EXPONENTIAL;

                loop_start_ = loop_end_ = 0;
            }

            inline void set_adr(
                float attack,
                float decay,
                float sustain,
                float release) {
                num_segments_ = 3;
                sustain_point_ = 0;

                level_[0] = 0.0f;
                level_[1] = 1.0F;
                level_[2] = sustain;
                level_[3] = 0.0f;

                time_[0] = attack;
                time_[1] = decay;
                time_[2] = sustain;

                shape_[0] = ENV_SHAPE_LINEAR;
                shape_[1] = ENV_SHAPE_LINEAR;
                shape_[2] = ENV_SHAPE_LINEAR;

                loop_start_ = loop_end_ = 0;
            }

            inline void set_ar(float attack, float decay) {
                num_segments_ = 2;
                sustain_point_ = 1;

                level_[0] = 0.0f;
                level_[1] = 1.0f;
                level_[2] = 0.0f;

                time_[0] = attack;
                time_[1] = decay;

                shape_[0] = ENV_SHAPE_LINEAR;
                shape_[1] = ENV_SHAPE_LINEAR;

                loop_start_ = loop_end_ = 0;
            }

            inline void set_adsar(
                float attack,
                float decay,
                float sustain,
                float release) {
                num_segments_ = 4;
                sustain_point_ = 2;

                level_[0] = 0.0f;
                level_[1] = 1.0f;
                level_[2] = sustain;
                level_[3] = 1.0f;
                level_[4] = 0.0f;

                time_[0] = attack;
                time_[1] = decay;
                time_[2] = attack;
                time_[3] = release;

                shape_[0] = ENV_SHAPE_LINEAR;
                shape_[1] = ENV_SHAPE_LINEAR;
                shape_[2] = ENV_SHAPE_LINEAR;
                shape_[3] = ENV_SHAPE_LINEAR;

                loop_start_ = loop_end_ = 0;
            }

            inline void set_adar(
                float attack,
                float decay,
                float sustain,
                float release) {
                num_segments_ = 4;
                sustain_point_ = 0;

                level_[0] = 0.0f;
                level_[1] = 1.0f;
                level_[2] = sustain;
                level_[3] = 1.0f;
                level_[4] = 0.0f;

                time_[0] = attack;
                time_[1] = decay;
                time_[2] = attack;
                time_[3] = release;

                shape_[0] = ENV_SHAPE_LINEAR;
                shape_[1] = ENV_SHAPE_LINEAR;
                shape_[2] = ENV_SHAPE_LINEAR;
                shape_[3] = ENV_SHAPE_LINEAR;

                loop_start_ = loop_end_ = 0;
            }

            inline void set_ad_loop(float attack, float decay) {
                num_segments_ = 2;
                sustain_point_ = 0;

                level_[0] = 0.0f;
                level_[1] = 1.0f;
                level_[2] = 0.0f;

                time_[0] = attack;
                time_[1] = decay;

                shape_[0] = ENV_SHAPE_LINEAR;
                shape_[1] = ENV_SHAPE_LINEAR;

                loop_start_ = 0;
                loop_end_ = 2;
            }

            inline void set_adr_loop(
                float attack,
                float decay,
                float sustain,
                float release) {
                num_segments_ = 3;
                sustain_point_ = 0;

                level_[0] = 0.0f;
                level_[1] = 1.0f;
                level_[2] = sustain;
                level_[3] = 0.0f;

                time_[0] = attack;
                time_[1] = decay;
                time_[2] = release;

                shape_[0] = ENV_SHAPE_LINEAR;
                shape_[1] = ENV_SHAPE_LINEAR;
                shape_[2] = ENV_SHAPE_LINEAR;

                loop_start_ = 0;
                loop_end_ = 3;
            }

            inline void set_adar_loop(
                float attack,
                float decay,
                float sustain,
                float release) {
                num_segments_ = 4;
                sustain_point_ = 0;

                level_[0] = 0.0f;
                level_[1] = 1.0f;
                level_[2] = sustain;
                level_[3] = 1.0f;
                level_[4] = 0.0f;

                time_[0] = attack;
                time_[1] = decay;
                time_[2] = attack;
                time_[3] = release;

                shape_[0] = ENV_SHAPE_LINEAR;
                shape_[1] = ENV_SHAPE_LINEAR;
                shape_[2] = ENV_SHAPE_LINEAR;
                shape_[3] = ENV_SHAPE_LINEAR;

                loop_start_ = 0;
                loop_end_ = 4;
            }

            inline void set_hard_reset(bool hard_reset) {
                hard_reset_ = hard_reset;
            }
            bool isDone() const { return done_; }
            float value() const { return value_; }
            void setValue(float v) { value_ = v; }
        private:
            inline float Interpolate8(const float* table, float index) const {
                index *= 256.0f;
                size_t integral = static_cast<size_t>(index);
                float fractional = index - static_cast<float>(integral);
                float a = table[integral];
                float b = table[integral + 1];
                return a + (b - a) * fractional;
            }

            float level_[kMaxNumSegments];
            float time_[kMaxNumSegments];
            EnvelopeShape shape_[kMaxNumSegments];

            int16_t segment_;
            float start_value_;
            float value_;

            float phase_;

            uint16_t num_segments_;
            uint16_t sustain_point_;
            uint16_t loop_start_;
            uint16_t loop_end_;

            bool hard_reset_;
            bool done_;

        };

enum class OscType
{
    Saw,
    Pulse,
    Triangle
};

enum class FilterType
{
    LPF,
    HPF,
    BPF
};

#if 0
osc1 pw
osc1 pwm
osc2 pw
ocs2 pwm
subosc vol?
noise vol
osc vibrato (lfoamt?)
vcf modamt
vcf keyfollow
env a, d, s, r
vca env/gate
#endif

struct Preset
{
    OscType osc1Shape, osc2Shape;
    float osc2Transpose = 2.001f;
    float osc1Vol = 0.5f, osc2Vol = 0.5f;
    FilterType vcfType;
    float vcfCutoff = 0.05f;
    float vcfReso = 7.f;
    float vcfEnv = 0.1f;
    float envA = 0.3f, envD = 0.1f, envS = 1.f, envR = 3.f;
    OscType lfoShape;
    float lfoFreq;
#if 1
    void dump()
    {

    }
#endif
};

enum class PrebakedPresets
{
    //% preset1
    Preset1 = 0,
    //% preset2
    Preset2,
    //% preset3
    Preset3,
    //% user1
    User1,
    //% user2
    User2
};

class Osc
{
    float acc_ = 0.f, delta_ = 0.f;
public:
    float process()
    {
        float out = acc_;
        acc_ += delta_;
        if (acc_ > 1.f) acc_ -= 2.f;
        return out;
    };
    void setFreq(float f)
    {
        delta_ = 2.f*f/44100.f;
    }
    void setType(OscType t)
    {

    }
    void setPW(float pw)
    {
        pw;
    }
};

class SVFFilter
{
public:
    float process(float in)
    {

    }
    void setCutoff(float f)
    {

    }
    void setRes(float r)
    {

    }
};

class ADSR
{


};

class Voice
{
    Osc osc_[2];
    Svf filter_;
    MultistageEnvelope env_;
    int gateLength_ = -1;
    uint8_t envflags = 0;
    int8_t note_ = -1;
    bool stopping_ = false;
    Preset* preset_;
    void apply_preset()
    {
        const Preset& p = *preset_;
        const float r_SR = 1.f/44100.f;
        filter_.Reset();
        env_.set_adsr(1.f/p.envA*r_SR, 1.f/p.envD*r_SR, p.envS, 1.f/p.envR*r_SR);
        filter_.set_f_q<FREQUENCY_FAST>(p.vcfCutoff, p.vcfReso);
    }
public:
    Voice()
    {
        setCutoff(0.1f);
        env_.set_adsr(1.f/10000.f, 1.f/5000.f, 0.3f, 1.f/40000.f);
    }
    float process()
    {
        //std::ostringstream lol;
        float env = env_.Process(envflags);

        auto oscs = osc_[0].process()*preset_->osc1Vol + osc_[1].process()*preset_->osc2Vol;
        auto out = env*filter_.Process<FILTER_MODE_LOW_PASS>(oscs);
        if (envflags & ENVELOPE_FLAG_RISING_EDGE) envflags = ENVELOPE_FLAG_GATE;
        if (envflags & ENVELOPE_FLAG_FALLING_EDGE) envflags = 0;
        if (gateLength_ > 0) --gateLength_;
        if (gateLength_ <= 0 && !stopping_) detrig();
        if (env_.isDone()) note_ = -1;
        return out;
    }
    void process(float* buf, int num)
    {
        float filt_freq = preset_->vcfCutoff + env_.value()*preset_->vcfEnv;
        //setCutoff(filt_freq > 1.f ? 1.f : filt_freq);
        filter_.set_f_q<FREQUENCY_FAST>(filt_freq, preset_->vcfReso);
        for (int i = 0; i < num; ++i) {
            buf[i] += process();
        }
    }
    void setFreq(float f)
    {
        osc_[0].setFreq(f);
        osc_[1].setFreq(f*preset_->osc2Transpose);
    }
    void setCutoff(float c)
    {
        //f = c;
        //filter_.set_f_q<FREQUENCY_FAST>(c, r);
    }
    void setRes(float r)
    {
        //this->r = r;
        //setCutoff(f);
    }
    void trig(int8_t note, Preset* preset, int length = -1)
    {
        preset_ = preset;
        stopping_ = false;
        note_ = note;
        gateLength_ = length;
        envflags = ENVELOPE_FLAG_RISING_EDGE;
        env_.setValue(0.f);
        apply_preset();
        osc_[0].setPW(1.f);  osc_[1].setPW(1.f);
    }
    void detrig()
    {
        stopping_ = true;
        envflags = ENVELOPE_FLAG_FALLING_EDGE;
    }
    int8_t getNote() const
    {
        return note_;
    }
    bool isStopping() const
    {
        return stopping_;
    }
};

template <int NumVoices = 16>
class Synth
{
    Voice voice[NumVoices];
    Preset* preset_;
    float gain_ = 1.f;
    float mixbuf[256];
    int findVoice(int8_t note)
    {
        for (int i = 0; i < NumVoices; ++i) {
            if (voice[i].getNote() == note && !voice[i].isStopping()) return i;
        }
        return -1;
    }
    Voice& alloc(int note)
    {
        for (int i = 0; i < NumVoices; ++i) {
            if (voice[i].getNote() == -1) return voice[i];
        }
        return voice[0];
    };
public:
    Synth(Preset* preset) : preset_(preset) { }
    void setGain(float g)
    {
        gain_ = g;
    }
    void noteOn(int8_t note, float velocity, float duration = 0.f)
    {
        Voice& v = alloc(note);
        v.trig(note, preset_, static_cast<int>(duration*44100.f));
        v.setFreq((440.*powf(2.f, (note - 69)/12.f)));
    }
    void noteOff(int8_t note)
    {
        int ind = findVoice(note);
        if (ind != -1) voice[ind].detrig();
    }
    float process()
    {
        float out = 0.f;
        for (auto& v : voice) {
            if (v.getNote() != -1) out += v.process();
        }
        return std::max(std::min(gain_*out, 1.f), -1.f);
    }
    void process(float* buf, int num)
    {
        std::fill_n(buf, num, 0.f);

        for (auto& v : voice) {
            if (v.getNote() == -1) continue;
            v.process(buf, num);
        }
        for (int i = 0; i < num; ++i) {
            buf[i] = std::max(std::min(gain_*buf[i], 1.f), -1.f);
        }
    }
    void process(uint16_t* buf, int num)
    {
        std::fill_n(mixbuf, num, 0.f);

        for (auto& v : voice) {
            if (v.getNote() == -1) continue;
            v.process(mixbuf, num);
        }
        for (int i = 0; i < num; ++i) {
            buf[i] = std::max(std::min(static_cast<int>(gain_*mixbuf[i]*511.f + 512.f), 1023), 0);
        }
    }
};
