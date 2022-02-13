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
#include "MicroBit.h"
#include "MicroSynth.h"
#include <cmath>
#include "samples/BD_sample.h"
#include "samples/SD_sample.h"
#include "samples/CL_sample.h"
#include "samples/HH_sample.h"
#include "samples/OHH_sample.h"
#include "samples/Cowbell_sample.h"

enum class SynthPreset {
    //% preset1
    Preset1 = 0,
    //% preset2
    Preset2,
    //% preset3
    Preset3,
};

enum class SynthParameter {
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

enum class Sample {
    BassDrum = 0,
    SnareDrum,
    Clap,
    HighHat,
    OpenHighHat,
    Cowbell
};

//% block="Orch blocks"
namespace orchestra {

static constexpr int NumVoices = 8;

Preset presets[3];
Synth synth(NumVoices);

class AudioTest : public DataSource
{
    DataSink* downStream_;
    bool init_ = false;
    Synth& synth;
public:
	AudioTest(Synth& s) : synth(s)
	{
	}
    void go()
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
		uint16_t* out = (uint16_t*)&buf[0];
        synth.process(out, 256);
        downStream_->pullRequest();
		return buf;
	}
};

AudioTest atest(synth);
bool audioInited = false;

static void audioInit()
{
    // manage speaker from PXT
    //uBit.audio.setSpeakerEnabled(false);
    uBit.audio.setVolume(255);
    uBit.audio.enable();
    uBit.audio.mixer.addChannel(atest);
    atest.go();
    audioInited = true;
    for (int i = 0; i < 3; ++i) {
        memset(&presets[i], 0, sizeof(Preset));
        presets[i].osc1Vol = presets[i].vcfCutoff = presets[i].gain = 0.5f;
        presets[i].ampGate = true;
    }
}

//%
void setParameter(SynthPreset preset, SynthParameter param, float val)
{
    auto& p = presets[static_cast<int>(preset)];

    if (!audioInited) audioInit();
    switch (param) {
    case SynthParameter::Osc1Shape:
        p.osc1Shape = static_cast<OscType>(static_cast<int>(val));
        break;
    case SynthParameter::Osc2Shape:
        p.osc2Shape = static_cast<OscType>(static_cast<int>(val));
        break;
    case SynthParameter::Osc2Transpose:
        p.osc2Transpose = val;
        break;
    case SynthParameter::Osc1Pw:
        p.osc1Pw = val;
        break;
    case SynthParameter::Osc2Pw:
        p.osc2Pw = val;
        break;
    case SynthParameter::Osc1Pwm:
        p.osc1Pwm = val;
        break;
    case SynthParameter::Osc2Pwm:
        p.osc2Pwm = val;
        break;
    case SynthParameter::OscFm:
        p.fmAmount = val;
        break;
    case SynthParameter::Osc1Gain:
        p.osc1Vol = val;
        break;
    case SynthParameter::Osc2Gain:
        p.osc2Vol = val;
        break;
    case SynthParameter::Cutoff:
        p.vcfCutoff = val;
        break;
    case SynthParameter::Resonance:
        p.vcfReso = val;
        break;
    case SynthParameter::FilterKeyFollow:
        p.vcfKeyFollow = val;
        break;
    case SynthParameter::FilterEnvAmount:
        p.vcfEnv = val;
        break;
    case SynthParameter::FilterLfoAmount:
        p.vcfLfo = val;
        break;
	case SynthParameter::FilterType:
		p.vcfType = static_cast<FilterType>(static_cast<int>(val));
		break;
    case SynthParameter::EnvAttackTime:
        p.envA = val;
        break;
    case SynthParameter::EnvDecayTime:
        p.envD = val;
        break;
    case SynthParameter::EnvSustainLevel:
        p.envS = val;
        break;
    case SynthParameter::EnvRelease:
        p.envR = val;
        break;
    case SynthParameter::AmpGate:
        p.ampGate = val > 0.f;
        break;
    case SynthParameter::Gain:
        p.gain = val;
        break;
    case SynthParameter::LFOFreq:
        p.lfoFreq = val;
        break;
    case SynthParameter::LFOShape:
        p.lfoShape = static_cast<OscType>(static_cast<int>(val));
        break;
    case SynthParameter::VibratoFreq:
        p.vibFreq = val;
        break;
    case SynthParameter::VibratoAmount:
        p.vibAmount = val;
		break;
    case SynthParameter::Tune:
        p.tune = val;
        break;
    case SynthParameter::Noise:
        p.noise = val;
        break;
    default:
        break;
    }
}

//%
float getParameter(SynthPreset preset, SynthParameter param)
{
    const auto& p = presets[static_cast<int>(preset)];

    if (!audioInited) audioInit();
    switch (param) {
    case SynthParameter::Osc1Shape:
        return static_cast<float>(p.osc1Shape);
    case SynthParameter::Osc2Shape:
        return static_cast<float>(p.osc2Shape);
    case SynthParameter::Osc2Transpose:
        return p.osc2Transpose;
    case SynthParameter::Osc1Pw:
        return p.osc1Pw;
    case SynthParameter::Osc2Pw:
        return p.osc2Pw;
    case SynthParameter::Osc1Pwm:
        return p.osc1Pwm;
    case SynthParameter::Osc2Pwm:
        return p.osc2Pwm;
    case SynthParameter::OscFm:
        return p.fmAmount;
    case SynthParameter::Osc1Gain:
        return p.osc1Vol;
    case SynthParameter::Osc2Gain:
        return p.osc2Vol;
    case SynthParameter::Cutoff:
        return p.vcfCutoff;
    case SynthParameter::Resonance:
        return p.vcfReso;
    case SynthParameter::FilterKeyFollow:
        return p.vcfKeyFollow;
    case SynthParameter::FilterEnvAmount:
        return p.vcfEnv;
    case SynthParameter::FilterLfoAmount:
        return p.vcfLfo;
    case SynthParameter::FilterType:
        return static_cast<float>(p.vcfType);
    case SynthParameter::EnvAttackTime:
        return p.envA;
    case SynthParameter::EnvDecayTime:
        return p.envD;
    case SynthParameter::EnvSustainLevel:
        return p.envS;
    case SynthParameter::EnvRelease:
        return p.envR;
    case SynthParameter::AmpGate:
        return static_cast<float>(p.ampGate);
    case SynthParameter::Gain:
        return p.gain;
    case SynthParameter::LFOFreq:
        return p.lfoFreq;
    case SynthParameter::LFOShape:
        return static_cast<float>(p.lfoShape);
    case SynthParameter::VibratoFreq:
        return p.vibFreq;
    case SynthParameter::VibratoAmount:
        return p.vibAmount;
    case SynthParameter::Tune:
        return p.tune;
    case SynthParameter::Noise:
        return p.noise;
    default:
        return 0.f;
    }
}

//%
void playSample(Sample sample, float gain)
{
    const uint8_t* samples[] = { BD_sample, SD_sample, CL_sample, HH_sample, OHH_sample, Cowbell_sample };
    const uint16_t sample_len[] = { sizeof(BD_sample), sizeof(SD_sample), sizeof(CL_sample), sizeof(HH_sample), sizeof(OHH_sample), sizeof(Cowbell_sample) };
    const int sample_index = static_cast<int>(sample);
    const uint8_t* buf = samples[sample_index];
    const auto len = sample_len[sample_index];

    if (!audioInited) audioInit();
    synth.playSample(buf, len, gain);
}

//%
void note(int note, int duration, int velocity, SynthPreset preset)
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
