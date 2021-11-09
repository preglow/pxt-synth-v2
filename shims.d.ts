// Auto-generated. Do not edit.



    //% block="Orch blocks"
declare namespace orchestra {

    /**
     * Set synth voice preset.
     * @param preset synth preset
     */
    //% help=orch/set-preset weight=30
    //% group="Orchestra"
    //% blockId=orch_set_preset block block="set preset %preset"
    //% shim=orchestra::setPreset
    function setPreset(preset: SynthPreset): void;

    /**
     * Set frequency of pellets.
     * @param freq frequency 20..4000
     */
    //% help=orch/set-freq weight=30
    //% group="Orchestra"
    //% blockId=orch_set_freq block block="set frequency %freq"
    //% freq.min=20 freq.max=8000
    //% freq.defl=200 shim=orchestra::setFreq
    function setFreq(freq?: int32): void;
}

// Auto-generated. Do not edit. Really.
