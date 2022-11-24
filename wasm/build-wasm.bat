@echo off

REM Expand Compile WasmSample.cpp to AudioSample.wasm
C:/WASM/clang.exe -x c++ ^
    --target=wasm32 ^
    -nostdinc ^
    -nostdlib ^
    -O0 ^
    -gfull ^
    -fno-threadsafe-statics ^
    -Wl,--allow-undefined ^
    -Wl,--import-memory ^
    -Wl,--no-entry ^
    -Wl,--export-dynamic ^
    -Wl,-z,stack-size=4194304 ^
    -D WASM32=1 ^
    -D _WASM32=1 ^
    -D DEBUG=1 ^
    -D _DEBUG=1 ^
    -o AudioSample.wasm ^
    WasmSample.cpp

REM  Build AudioSample.wasm.map from AudioSample.wasm
python C:/WASM/wasm-sourcemap.py ^
    AudioSample.wasm -s ^
    -o AudioSample.wasm.map ^
    -u "./AudioSample.wasm.map" ^
    -w AudioSample.debug.wasm ^
    --dwarfdump=C:/WASM/llvm-dwarfdump.exe