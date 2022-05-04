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
    // intbits must include a bit for sign
    function quantParam(param: number, bits: number, intbits: number): number {
        // how many bits to code fractional part with 
        const fracbits = bits - intbits;
        let q: number;
        if (intbits == 0) {
            // multiplier required to convert 0..1 float to integer
            const mult = (1 << fracbits) - 1;
            // quantize
            q = Math.round(param*mult);
        } else {
            // if we need to code some integer bits, we'll use a more general
            // and bit more wasteful fixed point encoding
            const mult = 1 << fracbits;
            // offset to make param unsigned
            const int_offset = (1 << intbits) / 2;
            // add offset and quantize
            q = Math.round((param + int_offset) * mult);
        }
        return q;
    }

    // this basically just undoes the operations in quantParam
    function reconstructParam(qparam: number, bits: number, intbits: number): number {
        const fracbits = bits - intbits;
        let n: number;
        if (intbits == 0) {
            const mult = (1 << fracbits) - 1;
            n = qparam/mult;
        } else {
            const mult = 1 << fracbits;
            const int_offset = (1 << intbits)/2;
            n = qparam/mult - int_offset;
        }
        return n;
    }

	function encodeParam(param: number, buf: Buffer, pos: number, chars: number): void {
        for (let i = 0; i < chars; i++) {
            buf.setUint8(pos + i, (param & 0x3f) + 48);
            param >>= 6;
        }
    }

    function decodeParam(buf: Buffer, pos: number, chars: number): number {
        let out = 0;
        for (let i = 0; i < chars; i++) {
            out |= (buf.getUint8(pos + i) - 48) << (i*6);
        }
        return out;
    }


    //% help=orch/set-synth-parameter weight=30
    //% group="Orchestra"
    //% blockId=orch_set_parameter
    //% block="set preset %preset parameter %param to %val"
    //% shim=orchestra::setParameter
    export function setParameter(preset: PxtSynthPreset, param: PxtSynthParameter, val: number): void
    {
        return
    }

    //% help=orch/get-synth-parameter weight=30
    //% group="Orchestra"
    //% blockId=orch_get_parameter
    //% block="get value of preset %preset parameter %param"
    //% shim=orchestra::getParameter
    export function getParameter(preset: PxtSynthPreset, param: PxtSynthParameter): number
    {
        return 0
    }

    //% help=orch/note weight=30
    //% group="Orchestra"
    //% blockId=orch_note
    //% block="trigger note %note with duration %duration and velocity %velocity using preset %preset"
    //% note.min=0 note.max=127
    //% duration.min=1 duration.max=10000
    //% velocity.min=0 velocity.max=127
    //% note.defl=69 duration.defl=1000 velocity.defl=127 preset.defl=1
    //% shim=orchestra::note
    export function note(note?: number, duration?: number, velocity?: number, preset?: PxtSynthPreset): void
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
