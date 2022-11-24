#ifndef _H_SIMPLESOUND_
#define _H_SIMPLESOUND_

#undef PlaySound

namespace SimpleSound {
	struct Sound;
	struct Bus;
	struct Device;
	struct PCMData;

	typedef void* (*fpAllocate)(unsigned int bytes);
	typedef void(*fpRelease)(void* mem);
	typedef float(*fpCos)(float v);
	typedef float(*fpSin)(float v);
	typedef float(*fpSqrt)(float v);

	struct Platform {
		fpAllocate allocate;
		fpRelease release;
		fpCos cos;
		fpSin sin;
		fpSqrt sqrt;
	};

	Device* InitWithSamples(Platform& platform, unsigned int sampleCount, unsigned int numChannels, void* platformSpecificPointer);
	Device* InitWithTime(Platform& platform, unsigned int bufferLengthMs, unsigned int numChannels, void* platformSpecificPointer);
	void Shutdown(Device* device);
	const char* GetLastError(Device& device);
	PCMData* LoadWavFromMemory(Device& device, void* data, unsigned int bytes);

	short* GetBuffer(Device& device);
	short* GetBuffer(PCMData& pcm);
	unsigned int GetBufferSize(Device& device);
	unsigned int GetNumSamples(PCMData& pcm);

	void MakeLeftHanded(Device& device);
	void MakeRightHanded(Device& device);

	unsigned int GetNumChannels(Device& device);
	unsigned int GetNumChannels(Bus& bus);
	unsigned int GetNumChannels(Sound& sound);
	unsigned int GetNumChannels(PCMData& pcm);

	float* GetMixer(Device& device);
	float* GetMixer(Bus& bus);

	void SetListener(Device& device, float posX, float posY, float posZ, float fwdX, float fwdY, float fwdZ, float upX = 0.0f, float upY = 1.0f, float upZ = 0.0f);
	float* GetListenerPosition(Device& device);
	float* GetListenerForward(Device& device);
	float* GetListenerUp(Device& device);
	// Results are only valid until the next Get function is called

	Bus* CreateBus(Device& device);
	void Destroy(Bus* bus);
	void Destroy(PCMData* pcm);

	// Valid on device, bus and 2D sound handles
	void SetPan(Device& device, float pan);
	void SetPan(Bus& bus, float pan);
	void SetPan(Sound& sound, float pan);
	
	float GetPan(Device& device);
	float GetPan(Bus& bus);
	float GetPan(Sound& sound);
	
	// Valid on every handle type
	void SetVolume(Device& device, float volume);
	void SetVolume(Bus& bus, float volume);
	void SetVolume(Sound& sound, float volume);

	float GetVolume(Device& device);
	float GetVolume(Bus& bus);
	float GetVolume(Sound& sound);

	bool IsValid(Device& device);
	bool IsValid(Sound& sound);
	inline bool IsValid(Bus& bus) {
		return true;
	}

	Sound* Play(Bus& bus, PCMData& pcm, bool looping);
	Sound* Play(Bus& bus, PCMData& pcm, float x, float y, float z, bool looping, float minAttenuation = 0.0f, float maxAttentuation = 10.0f);
	void Stop(Sound* sound);
	
	void Pause(Sound* sound);
	void Resume(Sound* sound);

	void SetPosition(Sound* sound, float x, float y, float z);
	inline void SetPosition(Sound* sound, float* position) {
		SetPosition(sound, position[0], position[1], position[2]);
	}
	float* GetPosition(Sound* sound);

	void SetAttenuation(Sound* sound, float aMin, float aMax);
	float GetMinAttenuation(Sound* sound);
	float GetMaxAttenuation(Sound* sound);
};

#endif