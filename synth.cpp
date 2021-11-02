#include "pxt.h"
#include "MicroBit.h"

//% block="Orch blocks"
namespace orchestra {

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

static constexpr int NumVoices = 15;

class Voice 
{
    float acc_ = 0.f, delta_ = 0.f;
    float env_ = 0.f, envFalloff_ = 0.99999f;
public:
    float process()
    {
        float out = (acc_*2.f - 1.f)*env_;
        acc_ += delta_;
        env_ *= envFalloff_;
        if (acc_ > 1.f) acc_ -= 1.f;
        return out;
    }

    void setFreq(float f)
    {
        delta_ = f/44100.;
    }
    void setEnv(float f)
    {
        envFalloff_ = f;
    }
    void trig()
    {
        env_ = 1.f;
    }
};

class AudioTest : public DataSource
{
	DataSink* downStream_;
    int voiceno_ = 0;
    bool init_ = false;
public:
    Voice voice[NumVoices];
	AudioTest()
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

		for (int i = 0; i < 256; ++i) {
            float y = 0.f;
            for (int v = 0; v < NumVoices; ++v)
                y += voice[v].process();
		    out[i] = (y*0.3f + 1.f)*511.f;
		}
        downStream_->pullRequest();
		return buf;
	}
    Voice& nextVoice()
    {
        auto& v = voice[voiceno_++];
        if (voiceno_ > NumVoices) voiceno_ = 0;
        return v;
    }
};

/**
 * Set synth voice preset.
 * @param preset synth preset
 */
//% help=orch/set-preset weight=30
//% group="Orchestra"
//% blockId=orch_set_preset block block="set preset %preset"
//% freq.defl=200
void setPreset(SynthPreset preset)
{
}

AudioTest atest;
bool audioInited = false;	
/**
 * Set frequency of pellets.
 * @param freq frequency 20..4000
 */
//% help=orch/set-freq weight=30
//% group="Orchestra"
//% blockId=orch_set_freq block block="set frequenzy %freq"
//% freq.min=20 freq.max=4000
//% freq.defl=200
void setFreq(int freq)
{
    if (!audioInited) {
        uBit.audio.setSpeakerEnabled(false);
        uBit.audio.setVolume(255);
        uBit.audio.enable();
        uBit.audio.mixer.addChannel(atest);
        atest.go();
        audioInited = true;
    }
    auto& voice = atest.nextVoice();
    //uBit.sleep(random(300) + 100);
    //uBit.audio.soundExpressions.playAsync("giggle");
    //uBit.display.scrollAsync("Hel");
    voice.setFreq(freq); 
    voice.trig();
}

}
