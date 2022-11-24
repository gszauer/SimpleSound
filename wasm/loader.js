/* TODO: Any exposed function should start with Wasm

typedef void (*OnFileLoaded)(const char* path, void* data, unsigned int bytes, void* userData);
void FileLoad(const char* path, void* target, unsigned int bytes, OnFileLoaded callback, void* userData);

__attribute__ (( visibility( "default" ) )) extern "C" void TriggerWasmLoaderCallback(OnFileLoaded callback, const char* path, void* data, unsigned int bytes, void* userdata) {
    callback(path, data, userdata, bytes);
}
*/

class FileLoader {
    constructor(wasmImportObject) {
        if (!wasmImportObject.hasOwnProperty("env")) {
            wasmImportObject.env = {};
        }
        let self = this;
        
        wasmImportObject.env["FileLoad"] = function(path, target, bytes, callback, userData) {
            self.FileLoad(path, target, bytes, callback, userData)
        }

        this.callbacksEnabled = false;
        this.queuedCallbacks = null;
        this.wasmInstance = null;
        this.wasmMemory = null;
        this.mem_u8 = null;
        this.decoder = new TextDecoder();
    }

    EnableCallbacks(wasmInstance, wasmMemory) {
        this.wasmInstance = wasmInstance;
        this.wasmMemory = wasmMemory;
        this.mem_u8 = new Uint8Array(this.wasmMemory.buffer);

        this.callbacksEnabled = true;
        if (this.queuedCallbacks != null) {
            while (this.queuedCallbacks.length != 0) {
                let callback = this.queuedCallbacks.pop();
                this.FileLoad(callback.path, callback.target, callback.bytes, callback.callback, callback.userData);
            }
            this.queuedCallbacks = null;
        }
    }

    FileLoad(_path, _target, _bytes, _callback, _userData) {
        let self = this;
        if (!this.callbacksEnabled) {
            if (this.queuedCallbacks == null) {
                this.queuedCallbacks = [];
            }
            let calback = {
                path: _path,
                target: _target,
                bytes: _bytes,
                callback: _callback,
                userData: _userData
            };
            this.queuedCallbacks.push(callback);
        }
        else {
            let iter = _path;
            while(this.mem_u8[iter] != 0) {
                iter += 1;
                if (iter - _path > 5000) {
                    console.error("FileLoader.FileLoad string decode loop took too long");
                    break;
                }
            }
            let stringPath = this.decoder.decode(new Uint8Array(this.wasmMemory.buffer, _path, iter - _path));
            if (stringPath == null || stringPath.length == 0) {
                console.error("FileLoader.FileLoad file path was empty, pointer:" + _path);
            }

            this.LoadFileAsArrayBuffer(stringPath, function(path, arrayBuffer) {
                let writtenBytes = 0;
                if (arrayBuffer == null) {
                    _target = 0; // Set data to null
                }
                else {
                    let dst_array = new Uint8Array(self.wasmMemory.buffer, _target, _bytes);
                    let src_array = new Uint8Array(arrayBuffer);
                    
                    writtenBytes = _bytes < arrayBuffer.byteLength? _bytes : arrayBuffer.byteLength;
                    for (let i = 0; i < writtenBytes; dst_array[i] = src_array[i++]);
                }
                self.wasmInstance.exports.TriggerWasmLoaderCallback(_callback, _path, _target, writtenBytes, _userData);
            });
        }
    }

    LoadFileAsArrayBuffer(path, onFileLoaded) { 
        const req = new XMLHttpRequest();
        req.customCallbackTriggered = false;
        req.open('GET', path, true);
        req.responseType = "arraybuffer";

        req.onload = (event) => {
            const arrayBuffer = req.response; // Note: not req.responseText
            if (arrayBuffer) {
                if (!req.customCallbackTriggered ) {
                    req.customCallbackTriggered  = true;
                    onFileLoaded(path, arrayBuffer);
                }
            }
            else {
                if (!req.customCallbackTriggered ) {
                    console.error("FileLoader.LoadFileAsArrayBuffer Could not load: " + path);
                    req.customCallbackTriggered  = true;
                    onFileLoaded(path, null);
                }
            }
        };

        req.send(null);
    }
}