#include "SimpleSound.h"

#undef PlaySound

#define SIMPLE_SOUND_MAX_CHANNELS 6
#define SIMPLE_SOUND_NUM_COMMANDS 512
#define SIMPLE_SOUND_ERROR_MSG_LEN 512
#define SIMPLE_SOUND_PI_4 0.78539816339f
#define SIMPLE_SOUND_SQRT2_2 0.70710678118f

extern "C" void WasmSubmitPcmBytes(void* device, unsigned int _writePointer, void* data, unsigned int dataSize); // js function
extern "C" unsigned int WasmGetCurrentPosition();
extern "C" void WasmRegisterAudioDevice(void* device);
extern "C" void WasmRemoveAudioDevice(void* device);

namespace SimpleSound {
	struct Device {
		Platform platform;
		unsigned int numChannels;

		bool isValid;
		char* errorMsg;
		float getResult[4]; // For GetPosition, GetVolume, etc...

		Bus* buses;

		float volume;
		float pan;

		float gainLeft; // For 2d panning
		float gainRight; // For 2d panning

		float listenerPosition[3];
		float listenerForward[3];
		float listenerUp[3];
		float listenerMat[9];
		
		unsigned int activeChunkIndex;
		float* mixer; // = new float[numSamples * numChannels]
		short* buffer; // new short[numSamples * numChannels * 2]
		short* buffer_unaligned; // new short[numSamples * numChannels * 2]
		unsigned int bufferSize; // sizeof(chunk) * 2 (measured in bytes)
		unsigned int numSamples; // Per chunk
		bool mRightHanded; // OpenGL style, forward -1
	};

	struct PCMData { 
		Device* owner;
		short* samples; // samples = new short[numSamples * numChannels];
		unsigned int numSamples;
		unsigned int numChannels; // Always the same as the Device that created it
	};

	struct Bus {
		Device* owner;
		Sound* sounds;

		Bus* prev;
		Bus* next;

		float volume;
		float pan;

		float gainLeft; // For panning
		float gainRight; // For panning

		// Interleaved buffer, ie: mix = new float[numSamples * numChannels];
		float* mix; // [channel1,channel2..channeln, channel1,channel2..channeln, etc..]
		bool clear;
	};

	struct Sound {
		PCMData* source;
		Bus* owner;

		Sound* prev;
		Sound* next;

		bool looping;
		bool paused;
		
		unsigned int cursor;

		float volume;

		float pan;
		float gainLeft;
		float gainRight;

		bool isSpatialized;
		float position[3];
		float minAttenuation;
		float maxAttenuation;
	};

	struct LeftRightGain {
		float leftGain;
		float rightGain;
	};

	inline void Cross(float* out, float* a, float* b) {
		out[0] = a[1] * b[2] - a[2] * b[1];
		out[1] = a[2] * b[0] - a[0] * b[2];
		out[2] = a[0] * b[1] - a[1] * b[0];
	}

	inline float Dot(float* a, float* b) {
		return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
	}

	inline void Normalize(Platform& plat, float* v) {
		float lenSq = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
		if (lenSq > 0.00001f) {
			float inv_len = 1.0f / plat.sqrt(lenSq);
			v[0] *= inv_len;
			v[1] *= inv_len;
			v[2] *= inv_len;
		}
	}

	inline void Mul(float* result, float* mat, float* vec) {
		float v[3] = { vec[0], vec[1], vec[2] };
		result[0] = v[0] * mat[0 * 3 + 0] + v[1] * mat[1 * 3 + 0] + v[2] * mat[2 * 3 + 0];
		result[1] = v[0] * mat[0 * 3 + 1] + v[1] * mat[1 * 3 + 1] + v[2] * mat[2 * 3 + 1];
		result[2] = v[0] * mat[0 * 3 + 2] + v[1] * mat[1 * 3 + 2] + v[2] * mat[2 * 3 + 2];
	}

	inline unsigned int uiMax(unsigned int _a, unsigned int _b) {
		if (_a >= _b) {
			return _a;
		}
		return _b;
	}

	inline unsigned int uiMin(unsigned int _a, unsigned int _b) {
		if (_a <= _b) {
			return _a;
		}
		return _b;
	}

	inline unsigned int StrLen(const char* str) {
		const char* iter;
		for (iter = str; *iter; ++iter);
		return (iter - str);
	}

	inline char* StrCpy(char* dest, const char* src) {
		unsigned int bytes = StrLen(src) + 1;

		char* d = (char*)dest;
		char* s = (char*)src;
		for (unsigned int i = 0; i < bytes; ++i) {
			d[i] = s[i];
		}
		return dest;
	}

	LeftRightGain LeftAndRightGainFromPan(Device& device, float pan) {
		if (pan < -1.0f) {
			pan = -1.0f;
		}
		if (pan > 1.0f) {
			pan = 1.0f;
		}

		float angle = double(pan) * SIMPLE_SOUND_PI_4;
		float cos_angle = device.platform.cos(angle);
		float sin_angle = device.platform.sin(angle);

		LeftRightGain result;
		result.leftGain = (SIMPLE_SOUND_SQRT2_2 * (cos_angle - sin_angle));
		result.rightGain = (SIMPLE_SOUND_SQRT2_2 * (cos_angle + sin_angle));
		return result;
	}

	LeftRightGain LinearGainFromPan(Device& device, float pan) {
		if (pan < -1.0f) {
			pan = -1.0f;
		}
		if (pan > 1.0f) {
			pan = 1.0f;
		}

		float leftPan = (pan + 1.0f) / 2.0f;
		float rightPan = 1.0f - leftPan;

		LeftRightGain result;
		result.leftGain = leftPan;
		result.rightGain = rightPan;
		return result;
	}
	
	short* GetBuffer(Device& device) {
		return device.buffer;
	}

	unsigned int GetBufferSize(Device& device) {
		return device.bufferSize;
	}

	short* GetBuffer(PCMData& pcm) {
		return pcm.samples;
	}

	unsigned int GetNumSamples(PCMData& pcm) {
		return pcm.numSamples;
	}

	PCMData* LoadWavFromMemory(Device& device, void* data, unsigned int bytes) {
		PCMData* result = (PCMData*)device.platform.allocate(sizeof(PCMData));

		result->owner = &device;
		result->numChannels = device.numChannels;

		char* cursor = ((char*)data); 
		cursor += 22;

		unsigned short numChannels = *(unsigned short*)cursor;
		cursor += 2;

		unsigned int samplingRate = *(unsigned int*)cursor;
		cursor += 4;
		
		cursor = ((char*)data);
		cursor += 34;

		unsigned short bitsPerSample = *(unsigned short*)cursor;

		cursor = ((char*)data);
		cursor += 40;

		unsigned int dataLength = *(unsigned int*)cursor;
		cursor += 4;

		result->numSamples = dataLength / (sizeof(short) * numChannels);
		result->samples = (short*)device.platform.allocate(dataLength);

		short* source = (short*)cursor;
		unsigned int numSamples = result->numSamples;
		for (unsigned int sample = 0; sample < numSamples * numChannels; ++sample) {
			result->samples[sample] = source[sample];
		}

		return result;
	}

	void Destroy(PCMData* pcm) {
		pcm->owner->platform.release(pcm->samples);
		pcm->owner->platform.release(pcm);
	}

	// Returns true if the sound finished mixing and needs to be removed
	bool Mix(Bus& destination, Sound& source, unsigned int destOffsetInSamples, bool clear) {
		float volume = source.volume;
		short* samples = source.source->samples;
		unsigned int pcmNumSamples = source.source->numSamples;
		unsigned int numChannels = destination.owner->numChannels;
		bool looping = source.looping;

		if (source.paused) {
			return false;
		}

		float gain[SIMPLE_SOUND_MAX_CHANNELS];
		for (int i = 0; i < SIMPLE_SOUND_MAX_CHANNELS; gain[i++] = 0.0f);
		gain[0] = 1.0f;
		if (numChannels >= 2) {
			gain[0] = source.gainLeft;
			gain[1] = source.gainRight;
		}

		if (source.isSpatialized) {
			const float speakers[6][6 * 3] = { // Positions copied from soloud
				{0, 0, 1,   0, 0, 0,   0, 0, 0,   0, 0, 0,   0, 0, 0,   0, 0, 0},
				{2, 0, 1,   -2, 0, 1,   0, 0, 0,   0, 0, 0,   0, 0, 0,   0, 0, 0},
				{0, 0, 0,   0, 0, 0,   0, 0, 0,   0, 0, 0,   0, 0, 0,   0, 0, 0},
				{2, 0, 1,   -2, 0, 1,   2, 0, -1,   -2, 0, 1,   0, 0, 0,   0, 0, 0},
				{0, 0, 0,   0, 0, 0,   0, 0, 0,   0, 0, 0,   0, 0, 0,   0, 0, 0},
				{2, 0, 1,   -2, 0, 1,   0, 0, 1,   0, 0, 1,   2, 0, -1,   -2, 0, -1},
			};

			Device* device = destination.owner;
			float listenerPos[3] = { device->listenerPosition[0], device->listenerPosition[1], device->listenerPosition[2] };
			float soundRelativeToListener[3] = {
				source.position[0] - listenerPos[0],
				source.position[1] - listenerPos[1],
				source.position[2] - listenerPos[2]
			};
			float distanceSq = Dot(soundRelativeToListener, soundRelativeToListener);
			Normalize(device->platform, soundRelativeToListener);
			Mul(soundRelativeToListener, device->listenerMat, soundRelativeToListener);

			for (unsigned int i = 0; i < device->numChannels; ++i) {
				float speakerRelativeToListener[3] = {
					speakers[device->numChannels - 1][i * 3 + 0],
					speakers[device->numChannels - 1][i * 3 + 1],
					speakers[device->numChannels - 1][i * 3 + 2]
				};
				Normalize(device->platform, speakerRelativeToListener);

				gain[i] = Dot(soundRelativeToListener, speakerRelativeToListener);
				gain[i] = (gain[i] + 1.0f) / 2.0f; // Normalize 0 to 1
			}
			if (device->numChannels == 6) {
				gain[3] = 1.0f; // Sub should mix everything?
			}

			if (distanceSq > 0.00001f) {
				float distance = device->platform.sqrt(distanceSq);
				if (distance < source.minAttenuation) {
					// Don't attenuate sounds if closer than the min attenuation distance
					//for (int i = 0; i < device->numChannels; gain[i++] = 1.0f);
				}
				else if (distance > source.maxAttenuation) { // Too far, mute sound
					for (int i = 0; i < device->numChannels; gain[i++] = 0.0f);
				}
				else { // Lerp
					float range = source.maxAttenuation - source.minAttenuation;
					float t = (distance - source.minAttenuation) / range;

					float attenuation = 1.0f - t;
					for (int i = 0; i < SIMPLE_SOUND_MAX_CHANNELS; gain[i++] *= attenuation);
				}
			}
		}

		unsigned int sourceOffsetInSamples = looping? source.cursor % pcmNumSamples : uiMin(source.cursor, pcmNumSamples);
		destOffsetInSamples = uiMin(destOffsetInSamples, destination.owner->numSamples);
		unsigned int numSamplesToCopy = destination.owner->numSamples - destOffsetInSamples;
		
		for (unsigned int sample = destOffsetInSamples; sample < numSamplesToCopy; ++sample) {
			unsigned int busIndex = (sample * numChannels);
			unsigned int soundIndex = source.cursor * numChannels;

			if (numChannels >= 1) {
				destination.mix[busIndex + 0] += float(samples[soundIndex + 0]) * volume * gain[0] - (clear ? destination.mix[busIndex + 0] : 0.0f);
			}
			if (numChannels >= 2) {
				destination.mix[busIndex + 1] += float(samples[soundIndex + 1]) * volume * gain[1] - (clear ? destination.mix[busIndex + 1] : 0.0f);
			}
			if (numChannels >= 4) {
				destination.mix[busIndex + 2] += float(samples[soundIndex + 2]) * volume * gain[2] - (clear ? destination.mix[busIndex + 2] : 0.0f);
				destination.mix[busIndex + 3] += float(samples[soundIndex + 3]) * volume * gain[3] - (clear ? destination.mix[busIndex + 3] : 0.0f);
			}
			if (numChannels >= 6) {
				destination.mix[busIndex + 4] += float(samples[soundIndex + 4]) * volume * gain[4] - (clear ? destination.mix[busIndex + 4] : 0.0f);
				destination.mix[busIndex + 5] += float(samples[soundIndex + 5]) * volume * gain[5] - (clear ? destination.mix[busIndex + 5] : 0.0f);
			}

			if (looping) {
				source.cursor = (source.cursor + 1) % pcmNumSamples;
			}
			else {
				source.cursor += 1;
				if (source.cursor >= pcmNumSamples) {
					break;
				}
			}
		}

		destination.clear = false;
			
		if (looping) {
			return false;
		}

		return source.cursor >= pcmNumSamples; // Sound can keep playing!
	}

	void Mix(Device& device, Bus& bus, bool clear) {
		unsigned int numChannels = device.numChannels;
		unsigned int numSamples = device.numSamples;

		float* dest = device.mixer;
		float* source = bus.mix;
		
		float volume = bus.volume;
		LeftRightGain pan = LeftAndRightGainFromPan(device, bus.pan);

		float gain[SIMPLE_SOUND_MAX_CHANNELS];
		for (int i = 0; i < SIMPLE_SOUND_MAX_CHANNELS; gain[i++] = 1.0f);
		if (numChannels == 2) {
			gain[0] = pan.leftGain;
			gain[1] = pan.rightGain;
		}
		
		for (unsigned int i = 0; i < numSamples * numChannels; ++i) {
			dest[i] += source[i] * volume * gain[i % numChannels] -(clear ? dest[i] : 0);
		}
	}

	void Blit(Device& device, unsigned int targetChunk) {
		unsigned int numChunks = 2; // 2 chunks since it's double buffered
		unsigned int numChannels = device.numChannels;
		unsigned int samplesize = sizeof(short) * numChannels;
		unsigned int numSamplesInBuffer = device.bufferSize / samplesize;
		unsigned int numSamplesInChunk = numSamplesInBuffer / numChunks;

		float* input = device.mixer;
		short* output = &device.buffer[targetChunk * numSamplesInChunk * numChannels];
		
		float volume = device.volume;
		LeftRightGain gain = LeftAndRightGainFromPan(device, device.pan);
		
		if (numChannels == 2) { // Pan is only applied if there are two speakers
			for (unsigned int i = 0; i < numSamplesInChunk * numChannels; i += 2) { // Could multiply this by numChannels to avoid the manual if statements below
				// Clip sound if it's out of range
				float value = input[i + 0] * volume * gain.leftGain;
				if (value < -32768.0f) {
					output[i + 0] = -32768;
				}
				else if (value > 32767.0f) {
					output[i + 0] = 32767;
				}
				else {
					output[i + 0] = value;
				}

				value = input[i + 1] * volume * gain.rightGain;
				if (value < -32768.0f) {
					output[i + 1] = -32768;
				}
				else if (value > 32767.0f) {
					output[i + 1] = 32767;
				}
				else {
					output[i + 1] = value;
				}
			}
		}
		else {
			for (unsigned int i = 0; i < numSamplesInChunk * numChannels; ++i) { // Could multiply this by numChannels to avoid the manual if statements below
				// Clip sound if it's out of range
				float value = input[i] * volume;
				if (value < -32768.0f) {
					output[i] = -32768;
				}
				else if (value > 32767.0f) {
					output[i] = 32767;
				}
				else {
					output[i] = value;
				}
			}
		}

		// Submit PCM data to sound card
		unsigned int chunkSizeInBytes = numSamplesInChunk * samplesize;
		WasmSubmitPcmBytes(&device, targetChunk * chunkSizeInBytes, output, chunkSizeInBytes);
	}

	Device* InitWithTime(Platform& platform, unsigned int bufferLengthMs, unsigned int numChannels, void* hwndPtr) {
		unsigned int sampleSize = sizeof(short) * numChannels;
		unsigned int samplesPerSecond = 48000;
		unsigned int samplesPerMillisecond = samplesPerSecond / 1000; // 48

		unsigned int numSamplesInChunk = bufferLengthMs * samplesPerMillisecond; // At 16ms (60 fps), this should be 768 samples per frame
		
		return InitWithSamples(platform, numSamplesInChunk, numChannels, hwndPtr);
	}

	void MakeLeftHanded(Device& device) {
		device.mRightHanded = true;
	}

	void MakeRightHanded(Device& device) {
		device.mRightHanded = false;
	}

	Device* InitWithSamples(Platform& platform, unsigned int sampleCount, unsigned int numChannels, void* hwndPtr) {
		if (numChannels > SIMPLE_SOUND_MAX_CHANNELS) {
			numChannels = SIMPLE_SOUND_MAX_CHANNELS;
		}

		Device* result = (Device*)platform.allocate(sizeof(Device));

		result->platform = platform;
		result->numChannels = numChannels;
		result->isValid = true;
		result->buses = 0;
		result->volume = 1.0f;
		result->pan = 0.0f;
		LeftRightGain leftRightGain = LeftAndRightGainFromPan(*result, result->pan);
		result->gainLeft = leftRightGain.leftGain;
		result->gainRight = leftRightGain.rightGain;
		result->mixer = 0;
		result->buffer = 0;
		result->buffer_unaligned = 0;
		result->bufferSize = 0;
		result->mRightHanded = true;
		result->activeChunkIndex = 0;
		for (int i = 0; i < 3; ++i) {
			result->getResult[i] = 0.0f;
			result->listenerPosition[i] = 0.0f;
			result->listenerForward[i] = 0.0f;
			result->listenerUp[i] = 0.0f;
		}
		result->getResult[3] = 0.0f;
		for (int i = 0; i < 9; result->listenerMat[i++] = 0.0f);
		result->listenerMat[0] = result->listenerMat[4] = result->listenerMat[8] = 1.0f;

		result->errorMsg = (char*)platform.allocate(sizeof(char) * SIMPLE_SOUND_ERROR_MSG_LEN);
		
		unsigned int size = sizeof(char) * SIMPLE_SOUND_ERROR_MSG_LEN;
		for (unsigned int i = 0; i < size; ++i) {
			result->errorMsg[i] = 0.0f;
		}

		// These are constant
		unsigned int sampleSize = sizeof(short) * numChannels;
		unsigned int samplesPerSecond = 48000;

		unsigned int numSamplesInChunk = sampleCount;
		unsigned int chunkSizeInBytes = numSamplesInChunk * sampleSize;
		result->numSamples = numSamplesInChunk;

		// The audio works with a double buffer scheme. Allocate enough for both.
		unsigned int bufferSize = chunkSizeInBytes * 2;

		result->isValid = true;
		result->mixer = (float*)platform.allocate(sizeof(float) * numSamplesInChunk * numChannels);
		result->buffer = (short*)platform.allocate(bufferSize + 4); // Already in bytes. +4 is so we can align (it only needs to be +2 tough)
		result->buffer_unaligned = result->buffer;
		if ((unsigned long)result->buffer % 2 != 0) {
			result->buffer = result->buffer + 1;
		}
		result->bufferSize = bufferSize;
		SetListener(*result, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f);

		for (unsigned int i = 0; i < result->numChannels * result->numSamples; result->mixer[i++] = 0.0f);
		for (unsigned int i = 0; i < result->bufferSize / sizeof(short); result->buffer[i++] = 0);
		WasmSubmitPcmBytes(result, 0, result->buffer, result->bufferSize);
		
		WasmRegisterAudioDevice(result);
		return result;
	}

	void Shutdown(Device* device) {
		WasmRemoveAudioDevice(device);

		for (Bus* bus = device->buses; bus != 0;) {
			Bus* process = bus;
			bus = bus->next;
			Destroy(process);
		}
		
		if (device->errorMsg != 0) {
			device->platform.release(device->errorMsg);
		}
		if (device->buffer != 0) {
			device->platform.release(device->buffer_unaligned);
		}
		if (device->mixer != 0) {
			device->platform.release(device->mixer);
		}

		device->platform.release(device);
	}

	bool IsValid(Device& device) {
		return device.isValid;
	}

	const char* GetLastError(Device& device) {
		if (device.isValid) {
			return 0;
		}
		// Make sure it's null terminated
		device.errorMsg[SIMPLE_SOUND_ERROR_MSG_LEN - 1] = '\0';
		return device.errorMsg;
	}

	unsigned int GetNumChannels(Device& device) {
		return device.numChannels;
	}

	Bus* CreateBus(Device& device) {
		// Step 1, make bus
		Bus* result = (Bus*)device.platform.allocate(sizeof(Bus));
		result->owner = &device;
		result->sounds = 0;
		result->prev = 0;
		result->next = 0;
		result->volume = 1.0f;
		result->pan = 0.0f;
		LeftRightGain leftRightGain = LeftAndRightGainFromPan(device, result->pan);
		result->gainLeft = leftRightGain.leftGain;
		result->gainRight = leftRightGain.rightGain;
		result->mix = (float*) device.platform.allocate(sizeof(float) * device.numChannels * device.numSamples);
		result->clear = false;

		// Step 2, Hook up to track
		if (device.buses != 0) {
			device.buses->prev = result;
		}
		result->next = device.buses;
		device.buses = result;

		// Step 3, Clear
		result->clear = true; // No sounds yet, clear bus on init
		for (unsigned int i = 0; i < device.numChannels * device.numSamples; result->mix[i++] = 0.0f);

		return result;
	}

	Sound* Play(Bus& bus, PCMData& pcm, bool looping) {
		Sound* result = (Sound*)bus.owner->platform.allocate(sizeof(Sound));
		result->source = &pcm;
		result->owner = &bus;
		result->prev = 0;
		result->next = 0;
		result->volume = 1.0f;
		result->pan = 0.0f;
		LeftRightGain leftRightGain = LeftAndRightGainFromPan(*bus.owner, result->pan);
		result->gainLeft = leftRightGain.leftGain;
		result->gainRight = leftRightGain.rightGain;
		result->looping = looping;
		result->cursor = 0;
		result->paused = false;

		result->isSpatialized = false;
		result->position[0] = 0.0f;
		result->position[1] = 0.0f;
		result->position[2] = 0.0f;
		result->minAttenuation = 0.0f;
		result->maxAttenuation = 0.0f;

		if (bus.sounds != 0) {
			bus.sounds->prev = result;
		}
		result->next = bus.sounds;
		bus.sounds = result;

		if (Mix(bus, *result, 0, false)) {
			Stop(result);
			return 0;
		}

		return result;
	}

	Sound* Play(Bus& bus, PCMData& pcm, float x, float y, float z, bool looping, float minAttenuation, float maxAttenuation) {
		Sound* result = (Sound*)bus.owner->platform.allocate(sizeof(Sound));
		result->source = &pcm;
		result->owner = &bus;
		result->prev = 0;
		result->next = 0;
		result->volume = 1.0f;
		result->pan = 0.0f;
		LeftRightGain leftRightGain = LeftAndRightGainFromPan(*bus.owner, result->pan);
		result->gainLeft = leftRightGain.leftGain;
		result->gainRight = leftRightGain.rightGain;
		result->looping = looping;
		result->cursor = 0;
		result->paused = false;

		if (bus.sounds != 0) {
			bus.sounds->prev = result;
		}
		result->next = bus.sounds;
		bus.sounds = result;

		if (Mix(bus, *result, 0, false)) {
			Stop(result);
			return 0;
		}

		return result;

		result->isSpatialized = true;
		result->position[0] = x;
		result->position[1] = y;
		result->position[2] = z;
		result->minAttenuation = minAttenuation;
		result->maxAttenuation = maxAttenuation;

		return result;
	}

	void Stop(Sound* sound) {
		// Unlink
		Bus* bus = sound->owner;
		if (sound->next != 0) {
			sound->next->prev = sound->prev;
		}
		if (sound->prev != 0) {
			sound->prev->next = sound->next;
		}
		if (bus->sounds == sound) {
			bus->sounds = sound->next;
		}

		// Delete
		Device* device = bus->owner;
		device->platform.release(sound);
	}

	void Destroy(Bus* bus) {
		for (Sound* sound = bus->sounds; sound != 0;) {
			Sound* process = sound;
			sound = sound->next;
			Stop(process);
		}

		// Release mixer
		if (bus->mix != 0) {
			bus->owner->platform.release(bus->mix);
		}

		// Unhook from tracked busses
		if (bus->next != 0) {
			bus->next->prev = bus->prev;
		}
		if (bus->prev != 0) {
			bus->prev->next = bus->next;
		}
		if (bus == bus->owner->buses) {
			bus->owner->buses = bus->next;
		}

		// Release bus
		bus->owner->platform.release(bus);
	}

	unsigned int GetNumChannels(Bus& bus) {
		return bus.owner->numChannels;
	}

	void SetVolume(Device& device, float volume) {
		if (volume < 0.0f) {
			volume = 0.0f;
		}
		if (volume > 1.0f) {
			volume = 1.0f;
		}
		device.volume = volume;
	}

	void SetVolume(Bus& bus, float volume) {
		if (volume < 0.0f) {
			volume = 0.0f;
		}
		if (volume > 1.0f) {
			volume = 1.0f;
		}
		bus.volume = volume;
	}

	float GetVolume(Device& device) {
		return device.volume;
	}

	float GetVolume(Bus& bus) {
		return bus.volume;
	}

	float GetPan(Device& device) {
		return device.pan;
	}

	float GetPan(Bus& bus) {
		return bus.pan;
	}

	void SetPan(Device& device, float pan) {
		LeftRightGain leftRightGain = LeftAndRightGainFromPan(device, pan);
		device.gainLeft = leftRightGain.leftGain;
		device.gainRight = leftRightGain.rightGain;
		device.pan = pan;
	}

	void SetPan(Bus& bus, float pan) {
		LeftRightGain leftRightGain = LeftAndRightGainFromPan(*bus.owner, pan);
		bus.gainLeft = leftRightGain.leftGain;
		bus.gainRight = leftRightGain.rightGain;
		bus.pan = pan;
	}

	unsigned int GetNumChannels(PCMData& pcm) {
		return pcm.numChannels;
	}

	short* GetOut(Device& device) {
		return device.buffer;
	}

	float* GetMixer(Device& device) {
		return device.mixer;
	}

	float* GetMixer(Bus& bus) {
		return bus.mix;
	}

	unsigned int GetNumChannels(Sound& sound) {
		return sound.owner->owner->numChannels;
	}

	void SetPan(Sound& sound, float pan) {
		LeftRightGain leftRightGain = LeftAndRightGainFromPan(*sound.owner->owner, pan);
		sound.gainLeft = leftRightGain.leftGain;
		sound.gainRight = leftRightGain.rightGain;
		sound.pan = pan;
	}

	float GetPan(Sound& sound) {
		return sound.pan;
	}

	void SetVolume(Sound& sound, float volume) {
		if (volume < 0.0f) {
			volume = 0.0f;
		}
		if (volume > 1.0f) {
			volume = 1.0f;
		}
		sound.volume = volume;
	}

	float GetVolume(Sound& sound) {
		return sound.volume;
	}

	bool IsValid(Sound& sound) {
		if (sound.looping) {
			return true;
		}

		short cursor = (short)sound.cursor;
		if (cursor >= sound.source->numSamples) {
			return false;
		}

		return true;
	}

	void Pause(Sound* sound) {
		sound->paused = true;
	}

	void Resume(Sound* sound) {
		sound->paused = false;
	}

	void SetAttenuation(Sound* sound, float aMin, float aMax) {
		sound->minAttenuation = aMin;
		sound->maxAttenuation = aMax;
	}

	float GetMinAttenuation(Sound* sound) {
		return sound->minAttenuation;
	}

	float GetMaxAttenuation(Sound* sound) {
		return sound->maxAttenuation;
	}

	void SetPosition(Sound* sound, float x, float y, float z) {
		sound->position[0] = x;
		sound->position[1] = y;
		sound->position[2] = z;
	}

	float* GetPosition(Sound* sound) {
		float* result = sound->owner->owner->getResult;
		result[0] = sound->position[0];
		result[1] = sound->position[1];
		result[2] = sound->position[2];
		result[3] = 0.0f;
		return result;
	}

	float* GetListenerPosition(Device& device) {
		float* result = device.getResult;
		result[0] = device.listenerPosition[0];
		result[1] = device.listenerPosition[1];
		result[2] = device.listenerPosition[2];
		result[3] = 0.0f;
		return result;
	}

	float* GetListenerForward(Device& device) {
		float* result = device.getResult;
		result[0] = device.listenerForward[0];
		result[1] = device.listenerForward[1];
		result[2] = device.listenerForward[2];
		result[3] = 0.0f;
		return result;
	}

	float* GetListenerUp(Device& device) {
		float* result = device.getResult;
		result[0] = device.listenerUp[0];
		result[1] = device.listenerUp[1];
		result[2] = device.listenerUp[2];
		result[3] = 0.0f;
		return result;
	}

	void SetListener(Device& device, float posX, float posY, float posZ, float fwdX, float fwdY, float fwdZ, float upX, float upY, float upZ) {
		float forward[3] = { fwdX, fwdY, fwdZ };
		Normalize(device.platform, forward);

		float up[3] = { upX, upY, upZ };
		Normalize(device.platform, up);

		float right[3] = { 0.0f, 0.0f, 0.0f };
		if (device.mRightHanded) {
			Cross(right, forward, up);
		}
		else {
			Cross(right, up, forward);
		}

		device.listenerForward[0] = forward[0];
		device.listenerForward[1] = forward[1];
		device.listenerForward[2] = forward[2];

		device.listenerUp[0] = up[0];
		device.listenerUp[1] = up[1];
		device.listenerUp[2] = up[2];

		device.listenerPosition[0] = posX;
		device.listenerPosition[1] = posY;
		device.listenerPosition[2] = posZ;

		device.listenerMat[0] = right[0];
		device.listenerMat[1] = right[1];
		device.listenerMat[2] = right[2];

		device.listenerMat[3] = up[0];
		device.listenerMat[4] = up[1];
		device.listenerMat[5] = up[2];

		device.listenerMat[6] = forward[0];
		device.listenerMat[7] = forward[1];
		device.listenerMat[8] = forward[2];
	}
};

__attribute__ (( visibility( "default" ) )) extern "C" void FillWithDebugSound(SimpleSound::Device* device) {
	int hertz = 261;

	unsigned int numSamples = device->numSamples;
	unsigned int numChannels = device->numChannels;

	int squareWavePeriod = 48000 / hertz;
	short tone = 1200;
	for (unsigned int i = 0; i < numSamples; ++i) {
		if (squareWavePeriod-- == 0) {
			tone = tone * -1;
			squareWavePeriod = 48000 / hertz;
		}

		if (numChannels >= 1) {
			device->buffer[i * numChannels + 0] = tone;
		}
		if (numChannels >= 2) {
			device->buffer[i * numChannels + 1] = tone;
		}
		if (numChannels >= 4) {
			device->buffer[i * numChannels + 2] = tone;
			device->buffer[i * numChannels + 3] = tone;
		}
		if (numChannels >= 6) {
			device->buffer[i * numChannels + 4] = tone;
			device->buffer[i * numChannels + 5] = tone;
		}
	}

	squareWavePeriod = 48000 / hertz;
	tone = 1200;
	for (unsigned int i = 0; i < numSamples; ++i) {
		if (squareWavePeriod-- == 0) {
			tone = tone * -1;
			squareWavePeriod = 48000 / hertz;
		}

		if (numChannels >= 1) {
			device->mixer[i * numChannels + 0] = tone;
		}
		if (numChannels >= 2) {
			device->mixer[i * numChannels + 1] = tone;
		}
		if (numChannels >= 4) {
			device->mixer[i * numChannels + 2] = tone;
			device->mixer[i * numChannels + 3] = tone;
		}
		if (numChannels >= 6) {
			device->mixer[i * numChannels + 4] = tone;
			device->mixer[i * numChannels + 5] = tone;
		}
	}
}

__attribute__ (( visibility( "default" ) )) extern "C" void AudioTick(SimpleSound::Device* device) {
		if (device == 0) {
			return;
		}

		unsigned int numChannels = device->numChannels;

		unsigned int numChunks = 2; // 2 chunks since it's double buffered
		unsigned int samplesize = sizeof(short) * device->numChannels;

		unsigned int numSamplesInBuffer = device->bufferSize / samplesize;
		unsigned int numSamplesInChunk = numSamplesInBuffer / numChunks;

		{
			// Synch audio based on how far the main buffer has played
			unsigned int playCursorInBytes = WasmGetCurrentPosition();
			unsigned int playCursorInSamples = playCursorInBytes / samplesize;
			
			unsigned int chunkIndex = playCursorInSamples / numSamplesInChunk;
			unsigned int nextChunk = (chunkIndex + 1) % numChunks;

			// Entering next chunk, dump all data, and mix into the one we just left
			if (device->activeChunkIndex != chunkIndex) {
				// All sounds have already been mixed into the mix of each bus
				// Clear the device mixer and Blast all buses into the device mixer
				SimpleSound::Bus* bus = device->buses;
				if (bus != 0) { // First bus being mixed into the device should clear the device
					// Doing this saves us a memset, since the mix function happens regardless.
					SimpleSound::Mix(*device, *bus, true);
					bus = bus->next;
				}
				for (; bus != 0; bus = bus->next) {
					SimpleSound::Mix(*device, *bus, false);
				}

				// Now that everything is mixed, convert float buffer to short buffer
				SimpleSound::Blit(*device, nextChunk); 
				// If there are any sounds left over, or looping, mix overflow for next frame
				for (SimpleSound::Bus* bus = device->buses; bus != 0; bus = bus->next) {
					SimpleSound::Sound* sound = bus->sounds;
					if (sound != 0) { // Clear bus mixer for first sound, avoiding a memset
						SimpleSound::Sound* process = sound;
						sound = sound->next;
						if (SimpleSound::Mix(*bus, *process, 0, true)) {
							SimpleSound::Stop(process);
						}
					}
					else if (!bus->clear) { // No sounds, clear bus
						bus->clear = true; 
						for (unsigned int i = 0; i < device->numChannels * device->numSamples; bus->mix[i++] = 0.0f);
					}
					for (; sound != 0; ) {
						SimpleSound::Sound* process = sound;
						sound = sound->next;
						if (SimpleSound::Mix(*bus, *process, 0, false)) {
							SimpleSound::Stop(process);
						}
					}
				}

				// SwapBuffers
				device->activeChunkIndex = chunkIndex;
			}
		}
	}