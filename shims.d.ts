// Auto-generated. Do not edit.



    //% block="Orch blocks"
declare namespace orchestra {

    /**
     * Set frequency of pellets.
     * @param freq frequency 20..4000
     */
    //% help=orch/set-freq weight=30
    //% group="Orchestra"
    //% blockId=orch_set_freq block block="set frequency %freq"
    //% freq.min=20 freq.max=4000
    //% freq.defl=200 shim=orchestra::setFreq
    function setFreq(freq?: int32): void;
}

// Auto-generated. Do not edit. Really.
