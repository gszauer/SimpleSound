// Need to use an audio worklet node anyway :( What a bummer. Check out:
// https://stackoverflow.com/questions/61070615/how-can-i-import-a-module-into-an-audioworkletprocessor-that-is-changed-elsewher

/* Adds the following functions:
    extern "C" void WasmSubmitPcmBytes(Device* device, unsigned int _writePointer, void* data, unsigned int dataSize);
    extern "C" unsigned int WasmGetCurrentPosition();
    extern "C" unsigned int WasmGetNumSamples();
    extern "C" void WasmRegisterAudioDevice(Device* device);
    extern "C" void WasmRemoveAudioDevice(Device* device);
*/

class SimpleSound {
    constructor(wasmImportObject, wasmMemory, numPcmSamples) {
        if (!wasmImportObject.hasOwnProperty("env")) {
            wasmImportObject.env = {};
        }
        this.wasmInsance = null;
        
        this.audioCtx = null;
        this.audioBuf = null;

        this.audioCtx = new (window.AudioContext || window.webkitAudioContext)();

        this.audioBuf = this.audioCtx.createBuffer(2, numPcmSamples * 2, 48000);
        for (let channel = 0; channel < this.audioBuf.numberOfChannels; channel++) {
            const buffer = this.audioBuf.getChannelData(channel);
            for (let i = 0; i < this.audioBuf.length; i++) {
                buffer[i] = Math.random() * 2 - 1; // TODO: Silence!
            }
        }

        this.audioSrc = this.audioCtx.createBufferSource();
        this.audioSrc.loop = true;
        this.audioSrc.buffer = this.audioBuf;
        this.audioSrc.connect( this.audioCtx.destination);
        this.audioSrc.start();

        this.mem_u16 = new Uint16Array(wasmMemory.buffer);
        this.c_devices = [];
        let self = this;

        this.leftSamples = new Float32Array(numPcmSamples);
        this.rightSamples = new Float32Array(numPcmSamples);

        // This is probably not the best way to handle get current position.
        let lengthInSecondsX2 = numPcmSamples / 48000.0 * 2.0;
        wasmImportObject.env["WasmGetCurrentPosition"] = function() {
            if (self.audioCtx == null) {
                return 0;
            }
            let playCursorInSeconds = self.audioCtx.currentTime % lengthInSecondsX2;
            let playCursorInBytes = Math.floor(playCursorInSeconds * 48000.0 * (2 * 2)); // sizeof(short) * 2
            //console.log("WasmGetCurrentPosition, currentTime: " + (self.audioCtx.currentTime) + ", playCursorInSeconds: " + playCursorInSeconds + ", playCursorInBytes: " + playCursorInBytes);
            return playCursorInBytes;
        };

        wasmImportObject.env["WasmGetNumSamples"] = function() {
            if (self.audioBuf == null) {
                return 0;
            }
            return self.audioBuf.length / 2;
        };

        wasmImportObject.env["WasmRegisterAudioDevice"] = function(devicePtr) {
            for (let i = 0; i < self.c_devices.length; ++i) {
                if (self.c_devices[i] == devicePtr) {
                    return;
                }
            }
            if (devicePtr == 0) {
                return;
            }
            self.c_devices.push(devicePtr);

            self.wasmInsance.exports.FillWithDebugSound(devicePtr);
        };

        wasmImportObject.env["WasmRemoveAudioDevice"] = function(devicePtr) {
            let remove_at = -1;
            for (let i = 0; i < self.c_devices.length; ++i) {
                if (self.c_devices[i] == devicePtr) {
                    remove_at = i;
                    break;
                }
            }
            if (remove_at >= 0) {
                self.c_devices.splice(remove_at, 1);
            }
        };

    wasmImportObject.env["WasmSubmitPcmBytes"] = function(device_ptr, writeCursor, data_ptr, dataBytes) {
        const numChannels = self.audioBuf.numberOfChannels;
        if (numChannels != 2) {
            console.error("WasmSubmitPcmBytes, wrong number of channels");
        }

        const sizeof_short = 2;
        const num_channels = 2;

        if (data_ptr % 2 != 0) {
            console.error("WasmSubmitPcmBytes: Copying unaligned data");
        }

        let numSamples = Math.floor(dataBytes / (sizeof_short * num_channels));

        const leftBuffer =  self.audioBuf.getChannelData(0);
        const rightBuffer =  self.audioBuf.getChannelData(1);

        for (let sample = 0; sample < numSamples; sample++) {
            let sourceIndexLeft = Math.floor((data_ptr + (sample * num_channels)) / sizeof_short);
            let sourceIndexRight = sourceIndexLeft + 1;
            
            self.leftSamples[sample] = 0.0;//self.mem_u16[sourceIndexLeft] / 32768.0;
            self.rightSamples[sample] = 0.0;//self.mem_u16[sourceIndexRight] / 32768.0;
        }

        let writeCursorInSamples = Math.floor(writeCursor / (sizeof_short * num_channels));
        self.audioBuf.copyToChannel(self.leftSamples, 0, writeCursorInSamples);
        self.audioBuf.copyToChannel(self.rightSamples, 1, writeCursorInSamples);
    };
}

    AttachToWasmInstance(wasmInsance) {
        let self = this;
        this.wasmInsance = wasmInsance;
        window.setInterval(function() {
            if (self.c_devices.length == 1) {
                wasmInsance.exports.AudioTick(self.c_devices[0]);
            }
            else if (self.c_devices.length > 1) {
                for (let i = 0; i < self.c_devices.length; ++i) {
                    wasmInsance.exports.AudioTick(self.c_devices[i]);
                }
            }
        }, 8);
    }
}