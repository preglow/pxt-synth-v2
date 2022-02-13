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

namespace orchestra {
    //% help=orch/set-synth-parameter weight=30
    //% group="Orchestra"
    //% blockId=orch_set_parameter
    //% block="set preset %preset parameter %param to %val"
    //% shim=orchestra::setParameter
    export function setParameter(preset: SynthPreset, param: SynthParameter, val: number): void
    {
        return
    }

    //% help=orch/get-synth-parameter weight=30
    //% group="Orchestra"
    //% blockId=orch_get_parameter
    //% block="get value of preset %preset parameter %param"
    //% shim=orchestra::getParameter
    export function getParameter(preset: SynthPreset, param: SynthParameter): number
    {
        return 0
    }

    //% help=orch/play-sample weight=30
    //% group="Orchestra"
    //% blockId=orch_play_sample
    //% block="play sample %sample with gain %gain"
    //% shim=orchestra::playSample
    export function playSample(sample: Sample, gain: number): void
    {
        return
    }

    //% help=orch/note weight=30
    //% group="Orchestra"
    //% blockId=orch_note
    //% block="trigger note %note with duration %duration and velocity %velocity using preset %preset"
    //% note.min=0 note.max=127
    //% duration.min=1 duration.max=10000
    //% velocity.min=0 velocity.max=127
    //% note.defl=69 duration.defl=1000 velocity.defl=127 preset.defl=1 s
    //% shim=orchestra::note
    export function note(note?: number, duration?: number, velocity?: number, preset?: SynthPreset): void
    {
        return
    }

    //% help=orch/note-off weight=30
    //% group="Orchestra"
    //% blockId=orch_note_off
    //% block="stop note %note"
    //% note.min=0 note.max=127
    //% note.defl=69
    //% shim=orchestra::noteOff
    export function noteOff(note?: number): void
    {
        return
    }

}
