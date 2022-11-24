/* Exposes the following functions to web assembly:
extern "C" void* WasmAllocate(unsigned int bytes, unsigned int alignment);
extern "C" void  WasmRelease(void* ptr);
extern "C" void* WasmCopy(void* dst, const void* src, unsigned int bytes);
extern "C" void* WasmClear(void* dst, unsigned int bytes); 


extern unsigned char __heap_base;
__attribute__ (( visibility( "default" ) )) extern "C" void* WasmHeapBase(void) {
  return &__heap_base;
}
*/

class MemoryAllocator {
    constructor(memorySize, wasmImportObject) {
        this.pageSize = 512;
        this.heapBase = 0;
        this.numPages = Math.ceil(memorySize / this.pageSize);

        const maskCount = Math.ceil(this.numPages / 32);
        this.mask = new Uint32Array(maskCount);
        for (let i = 0; i < maskCount; this.mask[i++] = 0);

        const wasmPageSize = 64 * 1024; // 64 KiB
        const wasmPageCount = Math.ceil(memorySize / wasmPageSize);
        this.wasmMemory = new WebAssembly.Memory({
            initial: wasmPageCount,
            maximum: wasmPageCount
        });
        this.mem_u8 = new Uint8Array(this.wasmMemory.buffer, 0, this.wasmMemory.buffer.byteLength);

        this.allocations = {};

        if (!wasmImportObject.hasOwnProperty("env")) {
            wasmImportObject.env = {};
        }
        wasmImportObject.env.memory = this.wasmMemory;
        
        let self = this;

        wasmImportObject.env["WasmAllocate"] = function(bytes, alignment) {
            if (alignment === undefined) {
                alignment = 0;
            }
            return self.Allocate(bytes, alignment);
        };

        wasmImportObject.env["WasmRelease"] = function(ptr) {
            self.Release(ptr);
        };

        wasmImportObject.env["WasmCopy"] = function(dst, src, bytes) {
            return self.Copy(dst, src, bytes);
        };

        wasmImportObject.env["WasmClear"] = function(dst, bytes) {
            return self.Set(dst, 0, bytes);
        };

        wasmImportObject.env["memcpy"] = function(ptr_dest, ptr_src, int_len) {
            let dst_buffer = new Uint8Array(self.wasmMemory.buffer, ptr_dest, int_len);
            let src_buffer = new Uint8Array(self.wasmMemory.buffer, ptr_src, int_len);
            for (let i = 0; i < int_len; ++i) {
                dst_buffer[i] = src_buffer[i];
            }
            return ptr_dest;
        }

        this.decoder =  new TextDecoder();
    }

    AttachToWasmInstance(wasmInsance) {
        const heap_base = wasmInsance.exports.WasmHeapBase();
        this.heapBase = heap_base;
    }

    Allocate(bytes, alignment) {
        const memNeeded = bytes + alignment;
        const pagesNeeded = Math.ceil(memNeeded / this.pageSize);

        let first_page = -1;
        let num_pages = 0;

        const numPages = this.numPages;
        // Keep first page as 1 to avoid writing to 0
        for (let page = 0; page < numPages; ++page) {
            let i = ~~(page / 32);
            let j = page % 32;

            const occupied = this.mask[i] & (1 << j);
            if (!occupied) {
                if (first_page == -1) {
                    first_page = page;
                }
                num_pages += 1;
            }
            else {
                first_page = -1;
                num_pages = 0;
            }

            if (num_pages >= pagesNeeded) {
                break; // Break J loop
            }
        }

        if (num_pages == 0 || first_page == -1) {
            console.error("Failed to allocate " + pagesNeeded + " pages. " + bytes + " bytes requested + " + alignment + " alignment.");
            return 0;
        }

        for (let i = first_page; i < first_page + num_pages; ++i) {
            const index = ~~(i / 32);
            const bit = i % 32;
            this.mask[index] |= (1 << bit);
        }

        let pointer = first_page * this.pageSize  + this.heapBase;
        let alignedPtr = pointer + ((alignment != 0)? (pointer % alignment) : 0);

        let allocationDescriptor = {
            start: first_page,
            length: num_pages,
        }

        if (this.allocations.hasOwnProperty(alignedPtr)) {
            console.error("Trying to double allocate the same pointer: " + pointer);
        }
        this.allocations[alignedPtr] = allocationDescriptor;
      
        return alignedPtr;
    }

    Release(alignedPtr) {
        if (this.allocations.hasOwnProperty(alignedPtr)) {
            let descriptor = this.allocations[alignedPtr];

            for (let i = descriptor.start; i < descriptor.start + descriptor.length; ++i) {
                const index = ~~(i / 32);
                const bit = i % 32;
                this.mask[index] &= ~(1 << bit);
            }

            delete this.allocations[alignedPtr];
        }
        else {
            console.error("Trying to release untracked memory: " + pointer + ", " + alignedPtr);
        }
    }

    Copy(dst, src, bytes) {
        for (let i = 0; i < bytes; ++i) {
            this.mem_u8[dst + i] = this.mem_u8[src + i];
        }
        return dst;
    }

    Clear(mem, bytes) {
        for (let i = 0; i < bytes; ++i) {
            this.mem_u8[mem + i] = 0;
        }
        return mem;
    }

    PointerToString(ptr) {
        let iter = ptr;
        while(this.mem_u8[iter] != 0) {
            iter += 1;

            if (iter - ptr > 5000) {
                console.error("MemoryAllocator.PointerToString loop took too long");
                break;
            }
        }
        return this.decoder.decode(new Uint8Array(this.wasmMemory.buffer, ptr, iter - ptr));
    }
}