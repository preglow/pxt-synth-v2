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

#include "pxt.h"
#include "MicroSynth.h"
#include <cmath>

enum class PxtSynthPreset {
    //% preset1
    Preset1 = 0,
    //% preset2
    Preset2,
    //% preset3
    Preset3,
};

enum class PxtSynthParameter {
    //% osc1shape
    Osc1Shape = 0,
    //% osc2shape
    Osc2Shape,
    //% osc2transpose
    Osc2Transpose,
    Osc1Pw,
    Osc2Pw,
    Osc1Pwm,
    Osc2Pwm,
    Osc1Gain,
    Osc2Gain,
    OscFm,
    //% cutoff
    Cutoff,
    //% resonance
    Resonance,
    //% envelope amount
    FilterKeyFollow,
    FilterEnvAmount,
    FilterLfoAmount,
    FilterType,
    //% envelope attack
    EnvAttackTime,
    //% envelope decay
    EnvDecayTime,
    //% envelope sustain
    EnvSustainLevel,
    EnvRelease,
    AmpGate,
    Gain,
    LFOFreq,
    LFOShape,
    VibratoFreq,
    VibratoAmount,
    Tune,
    Noise
};

//% block="Orch blocks"
namespace orchestra {

static constexpr int NumVoices = 6;

SynthPreset presets[3];
PolySynth synth(NumVoices);

class PolySynthSource : public DataSource
{
    DataSink* downStream_;
    bool init_ = false;
    PolySynth& synth_;
public:
    PolySynthSource(PolySynth& s) : synth_(s)
    {
    }
    void start()
    {
        if (!init_) {
            downStream_->pullRequest();
            init_ = true;
        }
    }
    virtual void connect(DataSink& sink) override
    {
        downStream_ = &sink;
    }
    virtual int getFormat() override
    {
        return DATASTREAM_FORMAT_16BIT_UNSIGNED;
    }
    virtual ManagedBuffer pull() override
    {
        ManagedBuffer buf(512);
        uint16_t* out = reinterpret_cast<uint16_t*>(&buf[0]);
        synth_.process(out, 256);
        downStream_->pullRequest();
        return buf;
    }
};

PolySynthSource atest(synth);
bool audioInited = false;

static void audioInit()
{
    // manage speaker from PXT
    //uBit.audio.setSpeakerEnabled(false);
    uBit.audio.setVolume(255);
    uBit.audio.enable();
    uBit.audio.mixer.addChannel(atest);
    atest.start();
    audioInited = true;
    for (int i = 0; i < 3; ++i) {
        memset(&presets[i], 0, sizeof(SynthPreset));
        presets[i].osc1Vol = presets[i].vcfCutoff = presets[i].gain = 0.5f;
        presets[i].ampGate = true;
    }
}

//%
void setParameter(PxtSynthPreset preset, PxtSynthParameter param, float val)
{
    auto& p = presets[static_cast<int>(preset)];

    if (!audioInited) audioInit();
    switch (param) {
    case PxtSynthParameter::Osc1Shape:
        p.osc1Shape = static_cast<OscType>(static_cast<int>(val));
        break;
    case PxtSynthParameter::Osc2Shape:
        p.osc2Shape = static_cast<OscType>(static_cast<int>(val));
        break;
    case PxtSynthParameter::Osc2Transpose:
        p.osc2Transpose = val;
        break;
    case PxtSynthParameter::Osc1Pw:
        p.osc1Pw = val;
        break;
    case PxtSynthParameter::Osc2Pw:
        p.osc2Pw = val;
        break;
    case PxtSynthParameter::Osc1Pwm:
        p.osc1Pwm = val;
        break;
    case PxtSynthParameter::Osc2Pwm:
        p.osc2Pwm = val;
        break;
    case PxtSynthParameter::OscFm:
        p.fmAmount = val;
        break;
    case PxtSynthParameter::Osc1Gain:
        p.osc1Vol = val;
        break;
    case PxtSynthParameter::Osc2Gain:
        p.osc2Vol = val;
        break;
    case PxtSynthParameter::Cutoff:
        p.vcfCutoff = val;
        break;
    case PxtSynthParameter::Resonance:
        p.vcfReso = val;
        break;
    case PxtSynthParameter::FilterKeyFollow:
        p.vcfKeyFollow = val;
        break;
    case PxtSynthParameter::FilterEnvAmount:
        p.vcfEnv = val;
        break;
    case PxtSynthParameter::FilterLfoAmount:
        p.vcfLfo = val;
        break;
    case PxtSynthParameter::FilterType:
        p.vcfType = static_cast<FilterType>(static_cast<int>(val));
        break;
    case PxtSynthParameter::EnvAttackTime:
        p.envA = val;
        break;
    case PxtSynthParameter::EnvDecayTime:
        p.envD = val;
        break;
    case PxtSynthParameter::EnvSustainLevel:
        p.envS = val;
        break;
    case PxtSynthParameter::EnvRelease:
        p.envR = val;
        break;
    case PxtSynthParameter::AmpGate:
        p.ampGate = val > 0.f;
        break;
    case PxtSynthParameter::Gain:
        p.gain = val;
        break;
    case PxtSynthParameter::LFOFreq:
        p.lfoFreq = val;
        break;
    case PxtSynthParameter::LFOShape:
        p.lfoShape = static_cast<OscType>(static_cast<int>(val));
        break;
    case PxtSynthParameter::VibratoFreq:
        p.vibFreq = val;
        break;
    case PxtSynthParameter::VibratoAmount:
        p.vibAmount = val;
        break;
    case PxtSynthParameter::Tune:
        p.tune = val;
        break;
    case PxtSynthParameter::Noise:
        p.noise = val;
        break;
    default:
        break;
    }
}

//%
float getParameter(PxtSynthPreset preset, PxtSynthParameter param)
{
    const auto& p = presets[static_cast<int>(preset)];

    if (!audioInited) audioInit();
    switch (param) {
    case PxtSynthParameter::Osc1Shape:
        return static_cast<float>(p.osc1Shape);
    case PxtSynthParameter::Osc2Shape:
        return static_cast<float>(p.osc2Shape);
    case PxtSynthParameter::Osc2Transpose:
        return p.osc2Transpose;
    case PxtSynthParameter::Osc1Pw:
        return p.osc1Pw;
    case PxtSynthParameter::Osc2Pw:
        return p.osc2Pw;
    case PxtSynthParameter::Osc1Pwm:
        return p.osc1Pwm;
    case PxtSynthParameter::Osc2Pwm:
        return p.osc2Pwm;
    case PxtSynthParameter::OscFm:
        return p.fmAmount;
    case PxtSynthParameter::Osc1Gain:
        return p.osc1Vol;
    case PxtSynthParameter::Osc2Gain:
        return p.osc2Vol;
    case PxtSynthParameter::Cutoff:
        return p.vcfCutoff;
    case PxtSynthParameter::Resonance:
        return p.vcfReso;
    case PxtSynthParameter::FilterKeyFollow:
        return p.vcfKeyFollow;
    case PxtSynthParameter::FilterEnvAmount:
        return p.vcfEnv;
    case PxtSynthParameter::FilterLfoAmount:
        return p.vcfLfo;
    case PxtSynthParameter::FilterType:
        return static_cast<float>(p.vcfType);
    case PxtSynthParameter::EnvAttackTime:
        return p.envA;
    case PxtSynthParameter::EnvDecayTime:
        return p.envD;
    case PxtSynthParameter::EnvSustainLevel:
        return p.envS;
    case PxtSynthParameter::EnvRelease:
        return p.envR;
    case PxtSynthParameter::AmpGate:
        return static_cast<float>(p.ampGate);
    case PxtSynthParameter::Gain:
        return p.gain;
    case PxtSynthParameter::LFOFreq:
        return p.lfoFreq;
    case PxtSynthParameter::LFOShape:
        return static_cast<float>(p.lfoShape);
    case PxtSynthParameter::VibratoFreq:
        return p.vibFreq;
    case PxtSynthParameter::VibratoAmount:
        return p.vibAmount;
    case PxtSynthParameter::Tune:
        return p.tune;
    case PxtSynthParameter::Noise:
        return p.noise;
    default:
        return 0.f;
    }
}

//%
void note(int note, int duration, int velocity, PxtSynthPreset preset)
{
    const int preset_index = static_cast<int>(preset);
    if (!audioInited) audioInit();
    synth.noteOn(note, velocity/127.f, duration/1000.f, &presets[preset_index]);
}

//%
void noteOff(int note)
{
    synth.noteOff(note);
}
}
