/*
  ==============================================================================

    MicroSynth.h
    Created: 3 Nov 2021 2:05:33pm
    Author:  Thom Johansen

  ==============================================================================
*/

#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

#define _PI 3.14159265359f

static constexpr int BlockSize = 256;

struct SynthTables
{
    static float notetab[129]; // including guard point
    static bool inited;
    static void init()
    {
        // singletons are bad, but assume no race conditions, true for microbit use
        if (inited) return;
        for (int i = 0; i < 128; ++i)
            notetab[i] = powf(2.f, (i - 69)/12.f);
        notetab[128] = notetab[127];
        inited = true;
    }
    static float interpNote(float ind)
    {
        const int i = std::min(std::max(static_cast<int>(ind), 0), 127);
        const float frac = ind - i;
        return (1.f - frac)*notetab[i] + frac*notetab[i + 1];
    }
};

// zavilishin svf
class SVF
{
    float g_, g1_, d_;
    float s1_, s2_;
    float tan(float x)
    {
#if 0
        // Clip coefficient to about 100.
        x = x < 0.497f ? x : 0.497f;
        return tanf(_PI*x);
#elif 0
        // below 8 hz version
        const float a = 3.736e-01*_PI*_PI*_PI;
        return x*(_PI + a*x*x);
#else
        // stolen from mutable instruments
        // TODO for 48 kHz
        // TODO check out license, preferably replace with 44.1khz version
        const float pi_two = _PI*_PI;
        const float pi_three = pi_two*_PI;
        const float pi_five = pi_three*pi_two;
        const float a = 3.260e-01f*pi_three;
        const float b = 1.823e-01f*pi_five;
        const float f2 = x*x;
        return x*(_PI + f2*(a + b*f2));
#endif
    }
public:
    SVF()
    {
        reset();
    }
    void set(float cutoff, float res)
    {
        // TODO we will probably want to clip cutoff for part of the range
        const float r = 1.f - res;
        g_ = tan(cutoff);
        g1_ = 2.f*r + g_;
        d_ = 1.f/(1.f + 2.f*r*g_ + g_*g_);
    }
    float process(float x)
    {
#if 0
        const float hp = (x - g1_*s1_ - s2_)*d_;
        const float v1 = g_*hp;
        const float bp = v1 + s1_;
        s1_ = bp + v1;
        const float v2 = g_*bp;
        const float lp = v2 + s2_;
        s2_ = lp + v2;
        return lp;
#else // no hp
        const float bp = (g_*(x - s2_) + s1_)*d_;
        const float v1 = bp - s1_;
        s1_ = bp + v1;
        const float v2 = g_*bp;
        const float lp = v2 + s2_;
        s2_ = lp + v2;
        return lp;
#endif
    }
    void reset()
    {
        s1_ = s2_ = 0.f;
    }
};

class ADSR
{
    enum class State : uint8_t
    {
        A = 0, D, S, R, Done
    };
    float phase_, phase_inc_ = 0.f;
    float inc_[4] = { 0.f, 0.f, 0.f, 0.f };
    float levels_[5] = { 0.f, 1.f, 0.5f, 0.f, 0.f };
    float start_val_ = 0.f;
    float cur_ = 0.f;
    State state_;
public:
    ADSR()
    {
        reset();
    }
    float process()
    {
        if (state_ == State::Done) return 0.f;
        if (phase_ >= 1.f) {
            phase_ = 0.f;
            // yeah, maybe enum class isn't the right choice
            int next_state = static_cast<int>(state_) + 1;
            state_ = static_cast<State>(next_state);
            start_val_ = levels_[next_state];
            phase_inc_ = inc_[static_cast<int>(state_)];
        }
        phase_ += phase_inc_;

        cur_ = start_val_ + (levels_[static_cast<int>(state_) + 1] - start_val_)*phase_;
        return cur_;
    }
    void gate(bool g = true)
    {
        start_val_ = cur_;
        phase_ = 0.f;
        state_ = g ? State::A : State::R;
        phase_inc_ = inc_[static_cast<int>(state_)];
    }
    bool done() const
    {
        return state_ == State::Done;
    }
    void set(float a, float d, float s, float r)
    {
        const float r_SR = 1.f/44100.f;
        inc_[0] = std::min(r_SR/a, 1.f);
        inc_[1] = std::min(r_SR/d, 1.f);
        levels_[2] = s;
        inc_[3] = std::min(r_SR/r, 1.f);
    }
    void reset()
    {
        phase_ = phase_inc_ = 0.f;
        cur_ = 0.f;
        state_ = State::Done;
    }
    float value() const
    {
        return cur_;
    }
};

enum class OscType
{
    Saw = 0,
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
osc1 pwm
ocs2 pwm
subosc vol?
noise vol
osc vibrato (lfoamt?)
vcf keyfollow
env a, d, s, r
vca env/gate
#endif

struct Preset
{
    OscType osc1Shape = OscType::Pulse, osc2Shape = OscType::Pulse;
    // multiplicative transpose factor, typical 0.5 to 2.0
    float osc2Transpose = 1.501f;
    // linear amplitude factor, 0 to 1
    float osc1Vol = 0.5f, osc2Vol = 0.5f;
    // -1 to 1, 0 is square
    float osc1Pw = 0.5f, osc2Pw = 0.7f;
    // LFO to osc PWM, 0 to 1
    float osc1Pwm = 0.3f, osc2Pwm = 0.2f;
    FilterType vcfType;
    // 0.5 is nyquist (max)
    float vcfCutoff = 0.05f;
    // 1 is self resonance, 0 is no resonance
    float vcfReso = 0.7f;
    // portion of envelope to add to vcfCutoff, 0 to 1
    float vcfEnv = 0.1f;
    // portion of lfo to add to vcfCutoff, 0 to 1
    float vcfLfo = 0.01f;
    // seconds, sustain is amplitude factor 0 to 1
    float envA = 0.8f, envD = 0.3f, envS = 0.3f, envR = 1.f;
    OscType lfoShape = OscType::Triangle;
    float lfoFreq = 1.f;
    // vibrato frequency in hz
    float vibFreq = 5.f;
    // vibrato amount in semitones
    float vibAmount = 0.5f;
    float gain = 0.4f;
#if 1
    void dump()
    {

    }
#endif
};

class Osc
{
    float acc_ = 0.f, delta_ = 0.f, pw_ = 0.f;
    OscType wave_ = OscType::Saw;
public:
    float process()
    {
        float out = acc_;
        acc_ += delta_;
        if (acc_ > 1.f) acc_ -= 2.f;
        switch (wave_) {
        case OscType::Saw:
            return out;
        case OscType::Pulse:
            return out > pw_ ? 1.f : -1.f;
        case OscType::Triangle:
        default:
            return fabsf(out)*2.f - 1.f;
        }
    };
    void setFreq(float f)
    {
        delta_ = 2.f*f/44100.f;
    }
    void setType(OscType t)
    {
        wave_ = t;
    }
    void setPW(float pw)
    {
        pw_ = pw;
    }
};

class Voice
{
    Osc osc_[2];
    Osc lfo_;
    SVF filter_;
    ADSR env_;
    int gateLength_ = -1;
    uint8_t envflags = 0;
    int8_t note_ = -1;
    bool stopping_ = false;
    Preset* preset_;
    void apply_preset()
    {
        const Preset& p = *preset_;
        const float r_SR = 1.f/44100.f;
        filter_.reset();
        osc_[0].setPW(p.osc1Pw); osc_[1].setPW(p.osc2Pw);
        osc_[0].setType(p.osc1Shape); osc_[1].setType(p.osc2Shape);
        lfo_.setType(p.lfoShape);
        lfo_.setFreq(preset_->lfoFreq*BlockSize);
        env_.set(p.envA, p.envD, p.envS, p.envR);
        filter_.set(p.vcfCutoff, p.vcfReso);
    }
public:
    Voice()
    {
        env_.set(0.1f, 0.1f, 0.3f, 0.2f);
    }
    float process()
    {
        //std::ostringstream lol;
        float env = env_.process();

        auto oscs = osc_[0].process()*preset_->osc1Vol + osc_[1].process()*preset_->osc2Vol;
        auto out = env*filter_.process(oscs);
        // TODO check gatelength once per block somehow?
        if (gateLength_ > 0) --gateLength_;
        if (gateLength_ == 0 && !stopping_) detrig();
        if (env_.done()) note_ = -1;
        return out;
    }
    void process(float* buf, int num, float vib = 0.f)
    {
        const float lfo = lfo_.process();
        const float filt_freq = preset_->vcfCutoff + env_.value()*preset_->vcfEnv*0.5f + lfo*preset_->vcfLfo;
        setNote(static_cast<float>(note_) + vib);
        filter_.set(filt_freq, preset_->vcfReso);
        osc_[0].setPW(preset_->osc1Pw + preset_->osc1Pwm*lfo);
        osc_[1].setPW(preset_->osc2Pw + preset_->osc2Pwm*lfo);
        for (int i = 0; i < num; ++i) {
            buf[i] += process();
        }
    }
    void setNote(float note)
    {
        const float t = 440.f*SynthTables::interpNote(note);
        //const float t = 440.f*SynthTables::notetab[note];
        osc_[0].setFreq(t);
        osc_[1].setFreq(t*preset_->osc2Transpose);
    }
    void trig(int8_t note, Preset* preset, int length = -1)
    {
        preset_ = preset;
        stopping_ = false;
        note_ = note;
        gateLength_ = length;
        apply_preset();
        env_.reset();
        env_.gate();
    }
    void detrig()
    {
        env_.gate(false);
        stopping_ = true;
    }
    int8_t getNote() const
    {
        return note_;
    }
    bool isStopping() const
    {
        return stopping_;
    }
    const Preset* getPreset() const
    {
        return preset_;
    }
};

template <int NumVoices = 16>
class Synth
{
    Voice voice[NumVoices];
    Preset* preset_;
    float mixbuf[256];
    Osc viblfo_;
    int sample_pos_ = -1;
    int sample_len_ = 0;
    float sample_gain_ = 1.f;
    const uint8_t* sample_table_;

    int findVoice(int8_t note)
    {
        for (int i = 0; i < NumVoices; ++i) {
            if (voice[i].getNote() == note && !voice[i].isStopping()) return i;
        }
        return -1;
    }
    Voice& alloc(int /*note*/)
    {
        for (int i = 0; i < NumVoices; ++i) {
            if (voice[i].getNote() == -1) return voice[i];
        }
        return voice[0];
    };
public:
    Synth(Preset* preset) : preset_(preset)
    {
        SynthTables::init();
        viblfo_.setType(OscType::Triangle);
    }
    // TODO merge with noteOn
    void setPreset(Preset* p)
    {
        preset_ = p;
    }
    void playSample(const uint8_t* sample, int len, float gain = 1.f)
    {
        sample_table_ = sample;
        sample_len_ = len;
        sample_gain_ = gain/128.f; // bake in 8 bit conversion
        sample_pos_ = 0;
    }
    void noteOn(int8_t note, float /*velocity*/ , float duration = 0.f)
    {
        Voice& v = alloc(note);
        const int length = duration != 0.f ? static_cast<int>(duration*44100.f) : -1;
        v.trig(note, preset_, length);
    }
    void noteOff(int8_t note)
    {
        int ind = findVoice(note);
        if (ind != -1) voice[ind].detrig();
    }

    void process_noclip(float* buf, int num)
    {
        std::fill_n(buf, num, 0.f);

        viblfo_.setFreq(preset_->vibFreq*BlockSize);
        const float vib = viblfo_.process();
        for (auto& v : voice) {
            auto p = v.getPreset();
            if (v.getNote() == -1) continue;
            v.process(buf, num, vib*p->vibAmount);
        }
        if (sample_pos_ > -1) {
            for (int i = 0; i < num; ++i) {
                buf[i] += static_cast<float>(sample_table_[sample_pos_++] - 128.f)*sample_gain_;
                if (sample_pos_ >= sample_len_) {
                    sample_pos_ = -1;
                    break;
                }
            }
        }
    }

    void process(float* buf, int num)
    {
        process_noclip(buf, num);
        for (int i = 0; i < num; ++i) {
            buf[i] = std::max(std::min(preset_->gain*buf[i], 1.f), -1.f);
        }
    }

    void process(uint16_t* buf, int num)
    {
        process_noclip(mixbuf, num);
        for (int i = 0; i < num; ++i) {
            buf[i] = std::max(std::min(static_cast<int>(preset_->gain*mixbuf[i]*511.f + 512.f), 1023), 0);
        }
    }
};
