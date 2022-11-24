// https://stackoverflow.com/questions/72568387/why-is-an-objects-constructor-being-called-in-every-exported-wasm-function
extern "C" void __wasm_call_ctors(void);
__attribute__((export_name("_initialize")))
extern "C" void _initialize(void) {
	// The linker synthesizes this to call constructors.
	__wasm_call_ctors();
}

extern "C" void* WasmAllocate(unsigned int bytes, unsigned int alignment);
extern "C" void  WasmRelease(void* ptr);
extern "C" void* WasmCopy(void* dst, const void* src, unsigned int bytes);
extern "C" void* WasmClear(void* dst, unsigned int bytes); 

extern "C" unsigned int WasmGetCurrentPosition();
extern "C" unsigned int WasmGetNumSamples();

extern unsigned char __heap_base;
__attribute__ (( visibility( "default" ) )) extern "C" void* WasmHeapBase(void) {
  return &__heap_base;
}

extern "C" float MathCos(float x);
extern "C" float MathSin(float x);
extern "C" float MathSqrt(float x);

typedef void (*OnFileLoaded)(const char* path, void* data, unsigned int bytes, void* userData);
extern "C" void FileLoad(const char* path, void* target, unsigned int bytes, OnFileLoaded callback, void* userData);
__attribute__ (( visibility( "default" ) )) extern "C" void TriggerWasmLoaderCallback(OnFileLoaded callback, const char* path, void* data, unsigned int bytes, void* userdata) {
	callback(path, data, bytes, userdata);
}

#include "../SimpleSound.h"
#include "../SimpleSound_wasm.cpp"
#define export __attribute__ (( visibility( "default" ) )) extern "C"

using namespace SimpleSound;

struct LoadedFile {
	void* data;
	unsigned int bytes;
};

struct UserData {
	Device* device;
	LoadedFile* pcm1;
	LoadedFile* pcm2;
	LoadedFile* pcm3;
	unsigned int pcmsLoaded;
};

void FileLoadedCallback(const char* path, void* data, unsigned int bytes, void* userData) {
	if (data != 0) {
		*(unsigned int*)userData = *(unsigned int*)userData + 1; 
	}
	else {
		int stop = 7;
	}
}

export void* StartSample() {
  UserData* result = (UserData*)WasmAllocate(sizeof(UserData), 0);
  result->pcmsLoaded = 0;

	SimpleSound::Platform plat;
	plat.allocate = [](unsigned int bytes) {
		return WasmAllocate(bytes, 0);
	};
	plat.release = [](void* mem) {
		WasmRelease(mem);
	};
	plat.cos = [](float v) {
		return MathCos(v);
	};
	plat.sin = [](float v) {
		return MathSin(v);
	};
	plat.sqrt = [](float v) {
		return MathSqrt(v);
	};

	result->pcm1 = (LoadedFile*)WasmAllocate(sizeof(LoadedFile), 0);
	result->pcm2 = (LoadedFile*)WasmAllocate(sizeof(LoadedFile), 0);
	result->pcm3 = (LoadedFile*)WasmAllocate(sizeof(LoadedFile), 0);

	result->pcm1->bytes = 1024 * 1024 * 8;
	result->pcm2->bytes = 1024 * 1024 * 8;
	result->pcm3->bytes = 1024 * 1024 * 8;

	result->pcm1->data = WasmAllocate(result->pcm1->bytes, 0);
	result->pcm2->data = WasmAllocate(result->pcm2->bytes, 0);
	result->pcm3->data = WasmAllocate(result->pcm3->bytes, 0);

	const char* pcm_path1 = "pcm/piano.wav";
	const char* pcm_path2 = "pcm/equinox.wav";
	const char* pcm_path3 = "pcm/LRMonoPhase4.wav";

	FileLoad(pcm_path1, result->pcm1->data, result->pcm1->bytes, FileLoadedCallback, &result->pcmsLoaded);
	FileLoad(pcm_path2, result->pcm1->data, result->pcm1->bytes, FileLoadedCallback, &result->pcmsLoaded);
	FileLoad(pcm_path3, result->pcm1->data, result->pcm1->bytes, FileLoadedCallback, &result->pcmsLoaded);

	result->device = InitWithSamples(plat, WasmGetNumSamples(), 2, 0);
	return result;
}

export void UpdateSample(void* userData, float dt) {
	UserData* u = (UserData*)userData;
	if (u ->pcmsLoaded != 3) {
		return;
	}
	int stop = 8;
	int now = 9;
}
