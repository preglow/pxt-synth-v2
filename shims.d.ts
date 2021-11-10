// Auto-generated. Do not edit.



    //% block="Orch blocks"
declare namespace orchestra {

    /**
     * Set synth preset.
     * @param preset synth preset
     */
    //% help=orch/set-preset weight=30
    //% group="Orchestra"
    //% blockId=orch_set_preset block block="set preset %preset" shim=orchestra::setPreset
    function setPreset(preset: SynthPreset): void;

    /**
     * Set synth preset parameters.
     * @param preset synth preset
     * @param param synth parameter
     * @val parameter value
     */
    //% help=orch/set-synth-parameter weight=30
    //% group="Orchestra"
    //% blockId=orch_set_paramt block block="set preset %preset parameter %param to %val" shim=orchestra::setParameter
    function setParameter(preset: SynthUserPreset, param: SynthParameter, val: number): void;

    /**
     * Trigger a note.
     * @param note note number, 0 to 127, MIDI
     * @param duration duration in ms
     * @param velocity velocity, 0 to 127, MIDI 
     */
    //% help=orch/note weight=30
    //% group="Orchestra"
    //% inlineInputMode=inline
    //% blockId=orch_note block block="trigger note %note duration %duration velocity %velocity"
    //% note.min=0 note.max=127
    //% duration.min=1 duration.max=10000
    //% velocity.min=0 velocity.max=127 note.defl=69 duration.defl=1000 velocity.defl=127 shim=orchestra::note
    function note(note?: int32, duration?: int32, velocity?: int32): void;
}

// Auto-generated. Do not edit. Really.
