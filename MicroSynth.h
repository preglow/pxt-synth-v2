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
static constexpr int SampleRate = 44100;
static constexpr float SampleRate_f = 44100.f;

class SynthTables
{
    static float notetab[129]; // including guard point
    static bool inited;
public:
    static void init();
    static float interpNote(float ind);
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
    float tan(float x);
public:
    SVF();
    void set(float cutoff, float res);
    float process(float x, FilterType f = FilterType::LPF);
    void reset();
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
    ADSR();
    float process();
    void gate(bool g = true);
    bool done() const;
    void set(float a, float d, float s, float r);
    void reset();
    float value() const;
};

// Naive oscillators with plenty of aliasing
class Osc
{
    float acc_ = 0.f, delta_ = 0.f, pw_ = 0.f;
    OscType wave_ = OscType::Saw;
public:
    float process();
    float processPM(float pm);
    void setFreq(float f);
    void setType(OscType t);
    void setPW(float pw);
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
    void apply_preset();
    void set_note(float note);
    // per sample process
    float process();
public:
    Voice();
    void process(float* buf, int num);
    void trig(int8_t note, float velocity, const Preset* preset, int length = -1);
    void detrig();
    int8_t getNote() const;
    bool isStopping() const;
};

class Synth
{
    Voice* voice_;
    float mixbuf_[BlockSize];
    int numVoices_;
    int sample_pos_ = -1;
    int sample_len_ = 0;
    float sample_gain_ = 1.f;
    const uint8_t* sample_table_;

    int findVoice(int8_t note);
    Voice& alloc(int note);
    void process_noclip(float* buf, int num);
public:
    Synth(int num_voices);
    ~Synth();
    void playSample(const uint8_t* sample, int len, float gain = 1.f);
    void noteOn(int8_t note, float velocity, float duration, const Preset* preset);
    void noteOff(int8_t note);
    void process(float* buf, int num);
    void process(uint16_t* buf, int num);
};
