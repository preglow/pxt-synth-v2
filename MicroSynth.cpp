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

#include "MicroSynth.h"

bool SynthTables::inited = false;
float SynthTables::notetab[129];

void SynthTables::init()
{
    // singletons are bad, but assume no race conditions, true for microbit use
    if (inited) return;
    for (int i = 0; i < 128; ++i)
        notetab[i] = powf(2.f, (i - 69)/12.f);
    notetab[128] = notetab[127];
    inited = true;
}

inline float SynthTables::interpNote(float ind)
{
    const int i = std::min(std::max(static_cast<int>(ind), 0), 127);
    const float frac = ind - i;
    return (1.f - frac)*notetab[i] + frac*notetab[i + 1];
}

inline float SVF::tan(float x)
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

SVF::SVF()
{
    reset();
}

inline void SVF::set(float cutoff, float res)
{
    // TODO we will probably want to clip cutoff for part of the range
    const float r = 1.f - res;
    g_ = tan(cutoff);
    g1_ = 2.f*r + g_;
    d_ = 1.f/(1.f + 2.f*r*g_ + g_*g_);
}

inline float SVF::process(float x, FilterType f)
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

void SVF::reset()
{
    s1_ = s2_ = 0.f;
}

ADSR::ADSR()
{
    reset();
}

inline float ADSR::process()
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

inline void ADSR::gate(bool g)
{
    start_val_ = cur_;
    phase_ = 0.f;
    state_ = g ? State::A : State::R;
    phase_inc_ = inc_[static_cast<int>(state_)];
}

bool ADSR::done() const
{
    return state_ == State::Done;
}

void ADSR::set(float a, float d, float s, float r)
{
    const float r_SR = 1.f/SampleRate_f;
    inc_[0] = std::min(r_SR/a, 1.f);
    inc_[1] = std::min(r_SR/d, 1.f);
    levels_[2] = s;
    inc_[3] = std::min(r_SR/r, 1.f);
}

void ADSR::reset()
{
    phase_ = phase_inc_ = 0.f;
    cur_ = 0.f;
    state_ = State::Done;
}

float ADSR::value() const
{
    return cur_;
}

inline float Osc::process()
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
}

inline float Osc::processPM(float pm)
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
}

inline void Osc::setFreq(float f)
{
    delta_ = 2.f*f/SampleRate_f;
}

void Osc::setType(OscType t)
{
    wave_ = t;
}

inline void Osc::setPW(float pw)
{
    pw_ = pw;
}

void Voice::apply_preset()
{
    const Preset& p = *preset_;
    filter_.reset();
    osc_[0].setPW(p.osc1Pw); osc_[1].setPW(p.osc2Pw);
    osc_[0].setType(p.osc1Shape); osc_[1].setType(p.osc2Shape);
    lfo_.setType(p.lfoShape);
    lfo_.setFreq(preset_->lfoFreq*BlockSize);
    env_.set(p.envA, p.envD, p.envS, p.envR);
    filter_.set(p.vcfCutoff, p.vcfReso);
}

void Voice::set_note(float note)
{
    const float t = 440.f*SynthTables::interpNote(note + preset_->tune);
    osc_[0].setFreq(t);
    osc_[1].setFreq(t*SynthTables::interpNote(preset_->osc2Transpose + 69.f));
}

Voice::Voice()
{
    env_.set(0.1f, 0.1f, 0.3f, 0.2f);
    vibLfo_.setType(OscType::Triangle);
}

inline float Voice::process()
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

void Voice::process(float* buf, int num)
{
    if (preset_ == nullptr) return;
    const float lfo = lfo_.process();
    vibLfo_.setFreq(preset_->vibFreq*BlockSize);
    const float vib = vibLfo_.process()*preset_->vibAmount;
    const float lfo_flt = preset_->vcfLfo*lfo*40.f;
    const float env_flt = preset_->vcfEnv*env_.value()*80.f;
    const float key_flt = preset_->vcfKeyFollow*static_cast<float>(note_ + preset_->tune - 60); // arbitrary subtract...
    // this mapping assumes SR = 44100, which it is for now. About 100+ hz to about 20k
    const float filt_freq = 700.f/SampleRate_f*SynthTables::interpNote(preset_->vcfCutoff*(127.f - 40.f) + 40.f + lfo_flt + env_flt + key_flt);
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

void Voice::trig(int8_t note, float velocity, const Preset* preset, int length)
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

void Voice::detrig()
{
    env_.gate(false);
    stopping_ = true;
}

int8_t Voice::getNote() const
{
    return note_;
}

bool Voice::isStopping() const
{
    return stopping_;
}

int Synth::findVoice(int8_t note)
{
    for (int i = 0; i < numVoices_; ++i) {
        if (voice_[i].getNote() == note && !voice_[i].isStopping()) return i;
    }
    return -1;
}

Voice& Synth::alloc(int /*note*/)
{
    // TODO make betterer. maybe steal oldest?
    for (int i = 0; i < numVoices_; ++i) {
        if (voice_[i].getNote() == -1) return voice_[i];
    }
    return voice_[0];
}

Synth::Synth(int num_voices) : numVoices_(num_voices)
{
    voice_ = new Voice[numVoices_];
    SynthTables::init();
}

Synth::~Synth()
{
    delete[] voice_;
}

void Synth::playSample(const uint8_t* sample, int len, float gain)
{
    sample_table_ = sample;
    sample_len_ = len;
    sample_gain_ = gain/128.f; // bake in 8 bit conversion
    sample_pos_ = 0;
}

void Synth::noteOn(int8_t note, float velocity, float duration, const Preset* preset)
{
    Voice& v = alloc(note);
    const int length = duration != 0.f ? static_cast<int>(duration*SampleRate_f) : -1;
    v.trig(note, velocity, preset, length);
}

void Synth::noteOff(int8_t note)
{
    int ind = findVoice(note);
    if (ind != -1) voice_[ind].detrig();
}

void Synth::process_noclip(float* buf, int num)
{
    std::fill_n(buf, num, 0.f);

    for (int i = 0; i < numVoices_; ++i) {
        Voice& v = voice_[i];
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

void Synth::process(float* buf, int num)
{
    process_noclip(buf, num);
    for (int i = 0; i < num; ++i) {
        buf[i] = std::max(std::min(buf[i], 1.f), -1.f);
    }
}

void Synth::process(uint16_t* buf, int num)
{
    process_noclip(mixbuf_, num);
    for (int i = 0; i < num; ++i) {
        buf[i] = static_cast<uint16_t>(std::max(std::min(static_cast<int>(mixbuf_[i]*511.f + 512.f), 1023), 0));
    }
}

