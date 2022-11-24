#include "SimpleSound.h"

#undef PlaySound

namespace SimpleSound {
	struct Device {
		unsigned int dummy;
	};

	struct PCMData { 
		unsigned int dummy;
	};

	struct Bus {
		unsigned int dummy;
	};

	struct Sound {
		unsigned int dummy;
	};

	short* GetBuffer(Device& device) {
		return 0;
	}

	unsigned int GetBufferSize(Device& device) {
		return 0;
	}

	short* GetBuffer(PCMData& pcm) {
		return 0;
	}

	unsigned int GetNumSamples(PCMData& pcm) {
		return 0;
	}

	PCMData* LoadWavFromMemory(Device& device, void* data, unsigned int bytes) {
		return 0;
	}

	void Destroy(PCMData* pcm) {
	}

	// Returns true if the sound finished mixing and needs to be removed
	bool Mix(Bus& destination, Sound& source, unsigned int destOffsetInSamples, bool clear) {
		return false;
	}

	void Mix(Device& device, Bus& bus, bool clear) {
	}

	void SubmitPcmBytes(Device& device, unsigned int _writePointer, void* data, unsigned int dataSize) {
	}

	void Blit(Device& device, unsigned int targetChunk) {
	}

	Device* InitWithTime(Platform& platform, unsigned int bufferLengthMs, unsigned int numChannels, void* hwndPtr) {
		return 0;
	}

	void MakeLeftHanded(Device& device) {
	}

	void MakeRightHanded(Device& device) {
	}

	Device* InitWithSamples(Platform& platform, unsigned int sampleCount, unsigned int numChannels, void* hwndPtr) {
		return 0;
	}

	void Shutdown(Device* device) {
	}

	bool IsValid(Device& device) {
		return false;
	}

	const char* GetLastError(Device& device) {
		return 0;
	}

	unsigned int GetNumChannels(Device& device) {
		return 0;
	}

	Bus* CreateBus(Device& device) {
		return 0;
	}

	Sound* Play(Bus& bus, PCMData& pcm, bool looping) {
		return 0;
	}

	Sound* Play(Bus& bus, PCMData& pcm, float x, float y, float z, bool looping, float minAttenuation, float maxAttenuation) {
		return 0;
	}

	void Stop(Sound* sound) {
	}

	void Destroy(Bus* bus) {
	}

	unsigned int GetNumChannels(Bus& bus) {
		return 0;
	}

	void SetVolume(Device& device, float volume) {
	}

	void SetVolume(Bus& bus, float volume) {
	}

	float GetVolume(Device& device) {
		return 0.0f;
	}

	float GetVolume(Bus& bus) {
		return 0.0f;
	}

	float GetPan(Device& device) {
		return 0.0f;
	}

	float GetPan(Bus& bus) {
		return 0.0f;
	}

	void SetPan(Device& device, float pan) {
	}

	void SetPan(Bus& bus, float pan) {
	}

	unsigned int GetNumChannels(PCMData& pcm) {
		return 0;
	}

	short* GetOut(Device& device) {
		return 0;
	}

	float* GetMixer(Device& device) {
		return 0;
	}

	float* GetMixer(Bus& bus) {
		return 0;
	}

	unsigned int GetNumChannels(Sound& sound) {
		return 0;
	}

	void SetPan(Sound& sound, float pan) {
	}

	float GetPan(Sound& sound) {
		return 0.0f;
	}

	void SetVolume(Sound& sound, float volume) {
	}

	float GetVolume(Sound& sound) {
		return 0.0f;
	}

	bool IsValid(Sound& sound) {
		return false;
	}

	void Pause(Sound* sound) {
	}

	void Resume(Sound* sound) {
	}

	void SetAttenuation(Sound* sound, float aMin, float aMax) {
	}

	float GetMinAttenuation(Sound* sound) {
		return 0.0f;
	}

	float GetMaxAttenuation(Sound* sound) {
		return 0.0f;
	}

	void SetPosition(Sound* sound, float x, float y, float z) {
	}

	float* GetPosition(Sound* sound) {
		return 0;
	}

	float* GetListenerPosition(Device& device) {
		return 0;
	}

	float* GetListenerForward(Device& device) {
		return 0;
	}

	float* GetListenerUp(Device& device) {
		return 0;
	}

	void SetListener(Device& device, float posX, float posY, float posZ, float fwdX, float fwdY, float fwdZ, float upX, float upY, float upZ) {
	}
};