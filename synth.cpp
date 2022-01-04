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
    //% user1
    User1,
    //% user2
    User2
};

enum class SynthUserPreset {
    //% user1
    User1 = 0,
    //% user2
    User2
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
    //% envelope attack
    EnvAttackTime,
    //% envelope decay
    EnvDecayTime,
    //% envelope sustain
    EnvSustainLevel,
    EnvRelease,
    Gain,
    LFOFreq,
    LFOShape,
    VibratoFreq,
    VibratoAmount
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

Preset presets[5] {
    { OscType::Triangle, OscType::Triangle, 0.504590, 0.493457, 0.500000, 0.500000, 0.700000, 0.300000, 0.300000, 0.125977, FilterType::LPF, 0.408789, 0.888332, 0.300000, 0.000000, 1.000000, 0.000000, 0.214355, 0.324000, 2.633301, OscType::Triangle, 0.500000, 5.000000, 0.000000, 0.298145 },
    { OscType::Triangle, OscType::Triangle, 0.504590, 0.493457, 0.500000, 0.500000, 0.700000, 0.300000, 0.300000, 0.125977, FilterType::LPF, 0.408789, 0.888332, 0.300000, 0.000000, 1.000000, 0.000000, 0.214355, 0.324000, 2.633301, OscType::Triangle, 0.500000, 5.000000, 0.000000, 0.298145 },
    { OscType::Triangle, OscType::Triangle, 0.504590, 0.493457, 0.500000, 0.500000, 0.700000, 0.300000, 0.300000, 0.125977, FilterType::LPF, 0.408789, 0.888332, 0.300000, 0.000000, 1.000000, 0.000000, 0.214355, 0.324000, 2.633301, OscType::Triangle, 0.500000, 5.000000, 0.000000, 0.298145 },
    { OscType::Triangle, OscType::Triangle, 0.504590, 0.493457, 0.500000, 0.500000, 0.700000, 0.300000, 0.300000, 0.125977, FilterType::LPF, 0.408789, 0.888332, 0.300000, 0.000000, 1.000000, 0.000000, 0.214355, 0.324000, 2.633301, OscType::Triangle, 0.500000, 5.000000, 0.000000, 0.298145 },
    { OscType::Triangle, OscType::Triangle, 0.504590, 0.493457, 0.500000, 0.500000, 0.700000, 0.300000, 0.300000, 0.125977, FilterType::LPF, 0.408789, 0.888332, 0.300000, 0.000000, 1.000000, 0.000000, 0.214355, 0.324000, 2.633301, OscType::Triangle, 0.500000, 5.000000, 0.000000, 0.298145 }
};
Synth<NumVoices> synth(&presets[0]);

class AudioTest : public DataSource
{
    DataSink* downStream_;
    bool init_ = false;
    Synth<NumVoices>& synth;
public:
	AudioTest(Synth<NumVoices>& s) : synth(s)
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

/**
 * Set synth preset parameters.
 * @param preset synth preset
 * @param param synth parameter
 * @param val parameter value
 */
//% help=orch/set-synth-parameter weight=30
//% group="Orchestra"
//% blockId=orch_set_parameter block block="set preset %preset parameter %param to %val"
void setParameter(SynthUserPreset preset, SynthParameter param, float val)
{
    auto& p = presets[3 + static_cast<int>(preset)];
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
    default:
        break;
    }
}
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
}

/**
 * Play sample.
 * @param sample which sample
 * @param gain gain
 */
//% help=orch/play-sample weight=30
//% group="Orchestra"
//% blockId=orch_play_sample block block="play sample %sample with gain %gain"
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

/**
 * Trigger a note.
 * @param note note number, 0 to 127, MIDI
 * @param duration duration in ms
 * @param velocity velocity, 0 to 127, MIDI 
 * @param preset preset
 */
//% help=orch/note weight=30
//% group="Orchestra"
//% inlineInputMode=inline
//% blockId=orch_note block block="trigger note %note with duration %duration and velocity %velocity using preset %preset"
//% note.min=0 note.max=127 note.defl=69
//% duration.min=1 duration.max=10000 duration.defl=1000
//% velocity.min=0 velocity.max=127 velocity.defl=127
//% preset.defl=1
void note(int note, int duration, int velocity, SynthPreset preset)
{
    const int preset_index = static_cast<int>(preset);
    synth.setPreset(&presets[preset_index]);
    if (!audioInited) audioInit();
    synth.noteOn(note, velocity/127.f, duration/1000.f);
}

}
