#include "pxt.h"
#include "MicroBit.h"
#include "MicroSynth.h"
#include <cmath>

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

//% block="Orch blocks"
namespace orchestra {

static constexpr int NumVoices = 8;

Preset preset;
Synth<NumVoices> synth(&preset);

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

AudioTest atest(synth);
bool audioInited = false;	
/**
 * Set frequency of pellets.
 * @param freq frequency 20..4000
 */
//% help=orch/set-freq weight=30
//% group="Orchestra"
//% blockId=orch_set_freq block block="set frequency %freq"
//% freq.min=20 freq.max=8000
//% freq.defl=200
void setFreq(int freq)
{
    static constexpr uint8_t minor[7] = { 0, 2, 4, 5, 7, 9, 11 };
    if (!audioInited) {
        uBit.audio.setSpeakerEnabled(false);
        uBit.audio.setVolume(255);
        uBit.audio.enable();
        uBit.audio.mixer.addChannel(atest);
        synth.setGain(0.2f);
        atest.go();
        audioInited = true;
    }
    //while (1) {
        //uBit.sleep(1000);
        synth.noteOn(42 + uBit.random(3)*12 + minor[uBit.random(7)], 1.f, 1.0f);
    //auto& voice = atest.nextVoice();
    //uBit.audio.soundExpressions.playAsync("giggle");
    //uBit.display.scrollAsync("Hel");
    
    //voice.setFreq(freq); 
    //voice.trig();
}

}
