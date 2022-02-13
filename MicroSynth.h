/*
MIT License

Copyright (c) 2022 Thom Johansen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
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

enum class OscType : uint8_t
{
    Saw = 0,
    Pulse,
    Triangle
};

enum class FilterType : uint8_t
{
    LPF = 0,
    HPF,
    BPF
};

#if 0
subosc vol?
noise vol
vca env/gate
#endif

struct Preset
{
    OscType osc1Shape, osc2Shape;
    // multiplicative transpose factor, typical 0.5 to 2.0
    float osc2Transpose;
    // linear amplitude factor, 0 to 1
    float osc1Vol, osc2Vol;
    // -1 to 1, 0 is square
    float osc1Pw, osc2Pw;
    // LFO to osc PWM, 0 to 1
    float osc1Pwm, osc2Pwm;
    // Osc1 -> Osc2 PM amount
    float fmAmount;
    FilterType vcfType;
    // 0 to 1, covers almost all spectrum
    float vcfCutoff;
    // 1 is self resonance, 0 is no resonance
    float vcfReso;
    // portion of envelope to add to vcfCutoff, 0 to 1
    float vcfEnv;
    // portion of lfo to add to vcfCutoff, 0 to 1
    float vcfLfo;
    // portion of note freq to add to cutoff, 0 to 1
    float vcfKeyFollow;
    // seconds, sustain is amplitude factor 0 to 1
    float envA, envD, envS, envR;
    OscType lfoShape;
    float lfoFreq;
    // vibrato frequency in hz
    float vibFreq;
    // vibrato amount in semitones
    float vibAmount;
    float gain;
    float tune;
    float noise;
    bool ampGate;
};

// Vadim Zavilishin's TPT state variable filter from "The Art of VA Filter Design"
class SVF
{
    float g_, g1_, d_;
    float s1_, s2_;
    float tan(float x)
    {
        // stolen from mutable instruments
        // TODO for 48 kHz, try to make a 44.1 khz version
        const float pi_two = _PI*_PI;
        const float pi_three = pi_two*_PI;
        const float pi_five = pi_three*pi_two;
        const float a = 3.260e-01f*pi_three;
        const float b = 1.823e-01f*pi_five;
        const float f2 = x*x;
        return x*(_PI + f2*(a + b*f2));
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
    float process(float x, FilterType f = FilterType::LPF)
    {
        const float hp = (x - g1_*s1_ - s2_)*d_;
        const float v1 = g_*hp;
        const float bp = v1 + s1_;
        s1_ = bp + v1;
        const float v2 = g_*bp;
        const float lp = v2 + s2_;
        s2_ = lp + v2;
        switch (f) {
        case FilterType::LPF:
        default:
            return lp;
        case FilterType::BPF:
            return bp;
        case FilterType::HPF:
            return hp;
        }
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

// Naive oscillators with plenty of aliasing
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
            return (out > pw_ ? 1.f : -1.f) + pw_;  // remove dc offset
        case OscType::Triangle:
        default:
            return fabsf(out)*2.f - 1.f;
        }
    };
    float processPM(float pm)
    {
        float out = acc_;
        acc_ += delta_ + pm;
        if (acc_ > 1.f) acc_ -= 2.f;
        else if (acc_ < -1.f) acc_ += 2.f;
        switch (wave_) {
        case OscType::Saw:
            return out;
        case OscType::Pulse:
            return (out > pw_ ? 1.f : -1.f) + pw_;
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

// all parameter modulations except the amplitude envelope are computed once per block 
// to save on processing
// do NOT call any parameter settings functions before setting a preset
class Voice
{
    Osc osc_[2];
    Osc lfo_;
    Osc vibLfo_;
    SVF filter_;
    ADSR env_;
    float gain_;
    float smoothedGate_;
    int gateLength_ = -1;   // -1 means no preset time duration
    int8_t note_ = -1;      // -1 means inactive voice
    bool stopping_ = false; // set to true after we've received a note off
    const Preset* preset_;
    int32_t noise_;
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
    void set_note(float note)
    {
        const float t = 440.f*SynthTables::interpNote(note + preset_->tune);
        osc_[0].setFreq(t);
        osc_[1].setFreq(t*SynthTables::interpNote(preset_->osc2Transpose + 69.f));
    }
public:
    Voice()
    {
        env_.set(0.1f, 0.1f, 0.3f, 0.2f);
        vibLfo_.setType(OscType::Triangle);
    }
    // per sample process
    float process()
    {
        const float env = env_.process();
        const float osc1 = osc_[0].process();
        const float gate = stopping_ ? 0.f : 1.f;

        smoothedGate_ += (gate - smoothedGate_)*0.005f;
        const float amp_env = preset_->ampGate ? smoothedGate_ : env;
        auto oscs = osc1*preset_->osc1Vol + osc_[1].processPM(preset_->fmAmount*osc1)*preset_->osc2Vol;
        noise_ = 1664525*noise_ + 1013904223;
        const float noise = preset_->noise*noise_*1.f/std::numeric_limits<int32_t>::max();
        auto out = gain_*amp_env*filter_.process(oscs + noise, preset_->vcfType);
        return out;
    }
    void process(float* buf, int num)
    {
        if (preset_ == nullptr) return;
        const float lfo = lfo_.process();
        vibLfo_.setFreq(preset_->vibFreq*BlockSize);
        const float vib = vibLfo_.process()*preset_->vibAmount;
        const float lfo_flt = preset_->vcfLfo*lfo*40.f;
        const float env_flt = preset_->vcfEnv*env_.value()*80.f;
        const float key_flt = preset_->vcfKeyFollow*static_cast<float>(note_ + preset_->tune - 60); // arbitrary subtract...
        // this mapping assumes SR = 44100, which it is for now. About 100+ hz to about 20k
        const float filt_freq = 700.f/44100.f*SynthTables::interpNote(preset_->vcfCutoff*(127.f - 40.f) + 40.f + lfo_flt + env_flt + key_flt);
        set_note(static_cast<float>(note_) + vib);
        filter_.set(filt_freq, preset_->vcfReso);
        osc_[0].setPW(preset_->osc1Pw + preset_->osc1Pwm*lfo);
        osc_[1].setPW(preset_->osc2Pw + preset_->osc2Pwm*lfo);
        for (int i = 0; i < num; ++i) {
            buf[i] += process();
        }
        // check if it's time to move amp envelope to release
        if (gateLength_ >= 0) gateLength_ -= std::min(gateLength_, BlockSize);
        if (!stopping_ && gateLength_ == 0) detrig();
        // check if amp envelope has died out and deactivate voice if so
        if (env_.done() || (preset_->ampGate && smoothedGate_ < 1e-3)) note_ = -1;
    }
    void trig(int8_t note, float velocity, const Preset* preset, int length = -1)
    {
        preset_ = preset;
        stopping_ = false;
        note_ = note;
        gateLength_ = length;
        smoothedGate_ = 0.f;
        apply_preset();
        gain_ = preset_->gain*velocity;
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
    float mixbuf[256];
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
        // TODO make betterer. maybe steal oldest?
        for (int i = 0; i < NumVoices; ++i) {
            if (voice[i].getNote() == -1) return voice[i];
        }
        return voice[0];
    };
public:
    Synth()
    {
        SynthTables::init();
    }
    void playSample(const uint8_t* sample, int len, float gain = 1.f)
    {
        sample_table_ = sample;
        sample_len_ = len;
        sample_gain_ = gain/128.f; // bake in 8 bit conversion
        sample_pos_ = 0;
    }
    void noteOn(int8_t note, float velocity, float duration, const Preset* preset)
    {
        Voice& v = alloc(note);
        const int length = duration != 0.f ? static_cast<int>(duration*44100.f) : -1;
        v.trig(note, velocity, preset, length);
    }
    void noteOff(int8_t note)
    {
        int ind = findVoice(note);
        if (ind != -1) voice[ind].detrig();
    }

    void process_noclip(float* buf, int num)
    {
        std::fill_n(buf, num, 0.f);

        for (auto& v : voice) {
            auto p = v.getPreset();
            if (v.getNote() == -1) continue;
            v.process(buf, num);
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
            buf[i] = std::max(std::min(buf[i], 1.f), -1.f);
        }
    }

    void process(uint16_t* buf, int num)
    {
        process_noclip(mixbuf, num);
        for (int i = 0; i < num; ++i) {
            buf[i] = std::max(std::min(static_cast<int>(mixbuf[i]*511.f + 512.f), 1023), 0);
        }
    }
};
