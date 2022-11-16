#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <istream>
#include <fstream>
#include <cmath>
#include <iostream>
#include "../SimpleSound.h"
#include "Imgui.h"

struct WindowData {
	HWND hwnd;
	HDC hdcDisplay; // Front Buffer DC
	HDC hdcMemory;  // Back Buffer DC
	HINSTANCE hInstance; // Window instance
};

struct File {
	char* data;
	unsigned int bytes;
};

File LoadFile(const char* path) {
	FILE* fp;
	long lSize;
	char* buffer;

	fp = fopen(path, "rb");
	fseek(fp, 0L, SEEK_END);
	lSize = ftell(fp);
	rewind(fp);

	buffer = new char[lSize];

	fread(buffer, lSize, 1, fp);
	fclose(fp);
	
	File result;
	result.bytes = lSize;
	result.data = buffer;
	return result;
}

#if 1
struct UserData {
	SimpleSound::Device* device;
	SimpleSound::Platform platform;
	WindowData window;
	SimpleSound::PCMData* piano;
	SimpleSound::Sound* pianoSound;
	float pianoPos[3];
	float pianoAttenuation[2];
	SimpleSound::PCMData* voice;
	SimpleSound::Sound* voiceSound;
	float voicePos[3];
	float voiceAttenuation[2];
	SimpleSound::PCMData* game;
	SimpleSound::Sound* gameSound;
	float gamePos[3];
	float gameAttenuation[2];
	SimpleSound::Bus* bus1;
	SimpleSound::Bus* bus2;
	Imgui* gui;
	HWND hwnd;
};


void* Initialize(const WindowData& windowData) {
	SimpleSound::Platform plat;
	plat.allocate = [](unsigned int bytes) {
		return malloc(bytes);
	};
	plat.release = [](void* mem) {
		free(mem);
	};
	plat.cos = [](float v) {
		return cosf(v);
	};
	plat.sin = [](float v) {
		return sinf(v);
	};
	plat.sqrt = [](float v) {
		return sqrtf(v);
	};

	UserData* result = (UserData*)plat.allocate(sizeof(UserData));
	result->platform = plat;
	result->window = windowData;
	result->gui = new Imgui(windowData.hwnd, windowData.hdcMemory);
	result->hwnd = windowData.hwnd;
	result->device = SimpleSound::InitWithTime(plat, 50, 2, (void*)&windowData.hwnd); // 50 ms is ~3 frames of audio dellay @ 60 FPS

	File piano = LoadFile("pcm/piano.wav");
	result->piano = SimpleSound::LoadWavFromMemory(*result->device, piano.data, piano.bytes);
	delete[] piano.data;

	File voice = LoadFile("pcm/LRMonoPhase4.wav");
	result->voice = SimpleSound::LoadWavFromMemory(*result->device, voice.data, voice.bytes);
	delete[] voice.data;

	File game = LoadFile("pcm/equinox.wav");
	result->game = SimpleSound::LoadWavFromMemory(*result->device, game.data, game.bytes);
	delete[] game.data;

	result->bus1 = SimpleSound::CreateBus(*result->device);
	result->bus2 = SimpleSound::CreateBus(*result->device);

	result->pianoPos[0] = 12.0f;
	result->pianoPos[1] = 0.0f;
	result->pianoPos[2] = 3.0f;

	result->pianoAttenuation[0] = 0.5f;
	result->pianoAttenuation[1] = 5.0f;

	result->voicePos[0] = -6.0f;
	result->voicePos[1] = 0.0f;
	result->voicePos[2] = 6.0f;

	result->voiceAttenuation[0] = 2.0f;
	result->voiceAttenuation[1] = 5.0f;

	result->gamePos[0] = 10.0f;
	result->gamePos[1] = 0.0f;
	result->gamePos[2] = -3.0f;

	result->gameAttenuation[0] = 1.0f;
	result->gameAttenuation[1] = 4.0f;

	result->pianoSound = SimpleSound::Play(*result->bus1, *result->piano, result->pianoPos[0], result->pianoPos[1], result->pianoPos[2], true, result->pianoAttenuation[0], result->pianoAttenuation[1]);
	result->voiceSound = SimpleSound::Play(*result->bus1, *result->voice, result->voicePos[0], result->voicePos[1], result->voicePos[2], true, result->voiceAttenuation[0], result->voiceAttenuation[1]);
	result->gameSound = SimpleSound::Play(*result->bus2, *result->game, result->gamePos[0], result->gamePos[1], result->gamePos[2], true, result->gameAttenuation[0], result->gameAttenuation[1]);
	
	return result;
}

void Render(void* ptr, HDC backBuffer, RECT clientRect) {
	UserData* u = (UserData*)ptr;
	Imgui* gui = u->gui;
	SimpleSound::Device* device = u->device;

	float s = 30.0f;

	gui->BeginFrame();

	{ // Piano
		float x = (10.0f + u->pianoPos[0]) * s;
		float y = (10.0f + u->pianoPos[2]) * s;

		HBRUSH oldBrush = (HBRUSH)SelectObject(gui->mTarget, gui->mHotBrush);
		int iRad = (int)(SimpleSound::GetMaxAttenuation(u->pianoSound) * s);
		Ellipse(gui->mTarget, x - iRad, y - iRad, x + iRad, y + iRad);
		SelectObject(gui->mTarget, oldBrush);

		if (gui->DraggableCircle(x, y, SimpleSound::GetMinAttenuation(u->pianoSound) * s)) {
			u->pianoPos[0] = x / s - 10.0f;
			u->pianoPos[2] = y / s - 10.0f;
		}
	}

	
	{ // Voice
		float x = (10.0f + u->voicePos[0]) * s;
		float y = (10.0f + u->voicePos[2]) * s;
		HBRUSH oldBrush = (HBRUSH)SelectObject(gui->mTarget, gui->mHotBrush);
		int iRad = (int)(SimpleSound::GetMaxAttenuation(u->voiceSound) * s);
		Ellipse(gui->mTarget, x - iRad, y - iRad, x + iRad, y + iRad);
		SelectObject(gui->mTarget, oldBrush);
		if (gui->DraggableCircle(x, y, SimpleSound::GetMinAttenuation(u->voiceSound) * s)) {
			u->voicePos[0] = x / s - 10.0f;
			u->voicePos[2] = y / s - 10.0f;
		}
	}

	{ // Game
		float x = (10.0f + u->gamePos[0]) * s;
		float y = (10.0f + u->gamePos[2]) * s;
		HBRUSH oldBrush = (HBRUSH)SelectObject(gui->mTarget, gui->mHotBrush);
		int iRad = (int)(SimpleSound::GetMaxAttenuation(u->gameSound) * s);
		Ellipse(gui->mTarget, x - iRad, y - iRad, x + iRad, y + iRad);
		SelectObject(gui->mTarget, oldBrush);
		if (gui->DraggableCircle(x, y, SimpleSound::GetMinAttenuation(u->gameSound) * s)) {
			u->gamePos[0] = x / s - 10.0f;
			u->gamePos[2] = y / s - 10.0f;
		}
	}

	{ // Listener
		float* vec = SimpleSound::GetListenerPosition(*device);
		float listenerPos[3] = { (vec[0] + 10.0f) * s, vec[1], (vec[2] + 10.0f) * s };
		if (gui->DraggableCircle(listenerPos[0], listenerPos[2], 10.0f)) {
			listenerPos[0] = listenerPos[0] / s - 10.0f;
			listenerPos[2] = listenerPos[2] / s - 10.0f;
			vec = SimpleSound::GetListenerForward(*device);
			SimpleSound::SetListener(*device, listenerPos[0], listenerPos[1], listenerPos[2], vec[0], vec[1], vec[2]);
		}
	}

	gui->EndFrame();
}

void Shutdown(void* ptr) {
	UserData* userData = (UserData*)ptr;
	SimpleSound::Device* device = userData->device;
	SimpleSound::Shutdown(device);
	userData->platform.release(userData);
}

#else
struct UserData {
	SimpleSound::Device* device;
	SimpleSound::Platform platform;
	WindowData window;
	SimpleSound::PCMData* piano;
	SimpleSound::PCMData* voice;
	SimpleSound::PCMData* game;
	SimpleSound::Bus* bus1;
	SimpleSound::Bus* bus2;
	Imgui* gui;
};

SimpleSound::Device* gDevice;
SimpleSound::PCMData* gPiano;
SimpleSound::PCMData* gVoice;
SimpleSound::PCMData* gGame;
SimpleSound::Bus* gBus1;
SimpleSound::Bus* gBus2;
SimpleSound::Sound* gSound1;
SimpleSound::Sound* gSound2;
SimpleSound::Sound* gSound3;

bool gLoop1;
bool gLoop2;
bool gLoop3;

HWND gHwnd;
char gPath[512];

char* OpenWav() {
	OPENFILENAMEA openFileDialog;

	ZeroMemory(&openFileDialog, sizeof(openFileDialog));
	ZeroMemory(&gPath, sizeof(char) * 512);
	openFileDialog.lStructSize = sizeof(OPENFILENAMEA);
	openFileDialog.hwndOwner = gHwnd;
	openFileDialog.lpstrFilter = "Wav (*.wav)\0*.wav\0All Files (*.*)\0*.*\0";
	openFileDialog.lpstrFile = gPath;
	openFileDialog.nMaxFile = MAX_PATH;
	openFileDialog.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	openFileDialog.lpstrDefExt = "wav";

	if (GetOpenFileNameA(&openFileDialog)) {
		return gPath;
	}

	return 0;
}

void* Initialize(const WindowData& windowData) {
	gPiano = 0;
	gBus1 = 0;
	gBus2 = 0;
	gDevice = 0;
	gSound1 = 0;
	gSound2 = 0;
	gSound3 = 0;

	gLoop1 = true;
	gLoop2 = true;
	gLoop3 = true;

	SimpleSound::Platform plat;
	plat.allocate = [](unsigned int bytes) {
		return malloc(bytes);
	};
	plat.release = [](void* mem) {
		free(mem);
	};
	plat.cos = [](float v) {
		return cosf(v);
	};
	plat.sin = [](float v) {
		return sinf(v);
	};
	plat.sqrt = [](float v) {
		return sqrtf(v);
	};
	gHwnd = windowData.hwnd;

	UserData* result = (UserData*)plat.allocate(sizeof(UserData));
	result->platform = plat;
	result->window = windowData;
	result->gui = new Imgui(windowData.hwnd, windowData.hdcMemory);
	
	result->device = gDevice = SimpleSound::InitWithTime(plat, 50, 2, (void*)& windowData.hwnd); // 50 ms is ~3 frames of audio dellay @ 60 FPS

	File piano = LoadFile("pcm/piano.wav");
	result->piano = gPiano = SimpleSound::LoadWavFromMemory(*result->device, piano.data, piano.bytes);
	delete[] piano.data;

	File voice = LoadFile("pcm/LRMonoPhase4.wav");
	result->voice = gVoice = SimpleSound::LoadWavFromMemory(*result->device, voice.data, voice.bytes);
	delete[] voice.data;

	File game = LoadFile("pcm/equinox.wav");
	result->game = gGame = SimpleSound::LoadWavFromMemory(*result->device, game.data, game.bytes);
	delete[] game.data;

	result->bus1 = gBus1 = SimpleSound::CreateBus(*result->device);
	result->bus2 = gBus2 = SimpleSound::CreateBus(*result->device);

	gSound1 = SimpleSound::Play(*result->bus1, *result->piano, gLoop1);
	gSound2 = SimpleSound::Play(*result->bus1, *result->voice, gLoop2);
	gSound3 = SimpleSound::Play(*result->bus2, *result->game, gLoop3);
	SimpleSound::SetVolume(*gSound3, 0.5f);
	SimpleSound::SetVolume(*gSound1, 0.5f);

	if (!gLoop1) {
		gSound1 = 0;
	}
	if (!gLoop2) {
		gSound2 = 0;
	}
	if (!gLoop3) {
		gSound3 = 0;
	}
	
	return result;
}

namespace SimpleSound {
	void FillWithDebugSound(struct Device* device, int hertz);
}

void Render(void* ptr, HDC backBuffer, RECT clientRect) {
	UserData* u = (UserData*)ptr;
	Imgui* gui = u->gui;
	SimpleSound::Device* device = u->device;

	gui->BeginFrame();

	RECT wavRect;
	wavRect.left = clientRect.left + 10;
	wavRect.right = wavRect.left + (clientRect.right - clientRect.left) / 2 - 5;
	wavRect.top = clientRect.top + 10;
	wavRect.bottom = wavRect.top + 76;

	if (u->piano != 0) {
		if (gui->Wave_short(SimpleSound::GetBuffer(*u->piano), SimpleSound::GetNumSamples(*u->piano), 2, wavRect, "Piano PCM")) {
			
		}
	}

	wavRect.top += 10 + 76;
	wavRect.bottom = wavRect.top + 76;

	if (u->voice != 0) {
		if (gui->Wave_short(SimpleSound::GetBuffer(*u->voice), SimpleSound::GetNumSamples(*u->voice), 2, wavRect, "Voice PCM")) {
			
		}
	}

	wavRect.top += 10 + 76;
	wavRect.bottom = wavRect.top + 76;

	if (u->game != 0) {
		if (gui->Wave_short(SimpleSound::GetBuffer(*u->game), SimpleSound::GetNumSamples(*u->game), 2, wavRect, "Game PCM")) {
			
		}
	}

	unsigned int buffer_size = SimpleSound::GetBufferSize(*device) / 2;

	wavRect.top = clientRect.top + 10;
	wavRect.bottom = wavRect.top + 125;

	wavRect.left = wavRect.right + 10;
	wavRect.right = wavRect.left + (clientRect.right - clientRect.left) / 2 - 30;
	if (gui->Wave_float(SimpleSound::GetMixer(*gBus1), buffer_size, 2, wavRect, "Bus 1 mixer")) {
		float volume = SimpleSound::GetVolume(*gBus1);
		if (volume < 0.5f) {
			volume = 1.0f;
		}
		else {
			volume = 0.0f;
		}
		SimpleSound::SetVolume(*gBus1, volume);
	}

	wavRect.top += 130;
	wavRect.bottom = wavRect.top + 120;
	if (gui->Wave_float(SimpleSound::GetMixer(*gBus2), buffer_size, 2, wavRect, "Bus 2 mixer")) {
		float volume = SimpleSound::GetVolume(*gBus2);
		if (volume < 0.5f) {
			volume = 1.0f;
		}
		else {
			volume = 0.0f;
		}
		SimpleSound::SetVolume(*gBus2, volume);
	}


	wavRect.left = clientRect.left + 10;
	wavRect.right = wavRect.left + (clientRect.right - clientRect.left) / 2 - 5;
	wavRect.top = clientRect.top + 10 + 250 + 10;
	wavRect.bottom = wavRect.top + 250;
	gui->Wave_float(SimpleSound::GetMixer(*gDevice), buffer_size, 2, wavRect, "Device mixer");

	wavRect.left = wavRect.right + 10;
	wavRect.right = wavRect.left + (clientRect.right - clientRect.left) / 2 - 30;
	gui->Wave_short(SimpleSound::GetBuffer(*gDevice), SimpleSound::GetBufferSize(*gDevice), 2, wavRect, "Device out");

	int width = 125;

	wavRect.top = wavRect.bottom + 10;
	wavRect.bottom = wavRect.top + 22;
	wavRect.left = clientRect.left + 10;
	wavRect.right = wavRect.left + width;
	float volume = SimpleSound::GetVolume(*gDevice);
	volume = gui->Slider("Device Volume", volume, 0.0f, 1.0f, wavRect);
	SimpleSound::SetVolume(*gDevice, volume);

	wavRect.left = wavRect.right + 5;
	wavRect.right = wavRect.left + width;
	float pan = SimpleSound::GetPan(*gDevice);
	pan = gui->Slider("Device Pan", pan, -1.0f, 1.0f, wavRect);
	SimpleSound::SetPan(*gDevice, pan);

	wavRect.left = wavRect.right + 5;
	wavRect.right = wavRect.left + width;
	volume = SimpleSound::GetVolume(*gBus1);
	volume = gui->Slider("Bus1 Volume", volume, 0.0f, 1.0f, wavRect);
	SimpleSound::SetVolume(*gBus1, volume);

	wavRect.left = wavRect.right + 5;
	wavRect.right = wavRect.left + width;
	pan = SimpleSound::GetPan(*gBus1);
	pan = gui->Slider("Bus1 Pan", pan, -1.0f, 1.0f, wavRect);
	SimpleSound::SetPan(*gBus1, pan);

	wavRect.left = wavRect.right + 5;
	wavRect.right = wavRect.left + width;
	volume = SimpleSound::GetVolume(*gBus2);
	volume = gui->Slider("Bus2 Volume", volume, 0.0f, 1.0f, wavRect);
	SimpleSound::SetVolume(*gBus2, volume);

	wavRect.left = wavRect.right + 5;
	wavRect.right = wavRect.left + width;
	pan = SimpleSound::GetPan(*gBus2);
	pan = gui->Slider("Bus2 Pan", pan, -1.0f, 1.0f, wavRect);
	SimpleSound::SetPan(*gBus2, pan);

	wavRect.top = wavRect.bottom + 5;
	wavRect.bottom = wavRect.top + 22;
	wavRect.left = clientRect.left + 10;
	wavRect.right = wavRect.left + width;
	volume = 0.0f;
	if (gSound1 != 0) {
		volume = SimpleSound::GetVolume(*gSound1);
	}
	volume = gui->Slider("Piano Volume", volume, 0.0f, 1.0f, wavRect);
	if (gSound1 != 0) {
		SimpleSound::SetVolume(*gSound1, volume);
	}

	wavRect.left = wavRect.right + 5;
	wavRect.right = wavRect.left + width;
	pan = 0.0f;
	if (gSound1 != 0) {
		pan = SimpleSound::GetPan(*gSound1);
	}
	pan = gui->Slider("Piano Pan", pan, -1.0f, 1.0f, wavRect);
	if (gSound1 != 0) {
		SimpleSound::SetPan(*gSound1, pan);
	}

	wavRect.left = wavRect.right + 5;
	wavRect.right = wavRect.left + width;
	volume = 0.0f;
	if (gSound2 != 0) {
		volume = SimpleSound::GetVolume(*gSound2);
	}
	volume = gui->Slider("Voice Volume", volume, 0.0f, 1.0f, wavRect);
	if (gSound2 != 0) {
		SimpleSound::SetVolume(*gSound2, volume);
	}

	wavRect.left = wavRect.right + 5;
	wavRect.right = wavRect.left + width;
	pan = 0.0f;
	if (gSound2 != 0) {
		pan = SimpleSound::GetPan(*gSound2);
	}
	pan = gui->Slider("Voice Pan", pan, -1.0f, 1.0f, wavRect);
	if (gSound2 != 0) {
		SimpleSound::SetPan(*gSound2, pan);
	}

	wavRect.left = wavRect.right + 5;
	wavRect.right = wavRect.left + width;
	volume = 0.0f;
	if (gSound3 != 0) {
		volume = SimpleSound::GetVolume(*gSound3);
	}
	volume = gui->Slider("Game Volume", volume, 0.0f, 1.0f, wavRect);
	if (gSound3 != 0) {
		SimpleSound::SetVolume(*gSound3, volume);
	}

	wavRect.left = wavRect.right + 5;
	wavRect.right = wavRect.left + width;
	pan = 0.0f;
	if (gSound3 != 0) {
		pan = SimpleSound::GetPan(*gSound3);
	}
	pan = gui->Slider("Game Pan", pan, -1.0f, 1.0f, wavRect);
	if (gSound3 != 0) {
		SimpleSound::SetPan(*gSound3, pan);
	}

	wavRect.top = wavRect.bottom + 5;
	wavRect.bottom = wavRect.top + 22;
	wavRect.left = clientRect.left + 10;
	wavRect.right = wavRect.left + width;
	gLoop1 = gui->Toggle("Loop Piano", gLoop1, wavRect);

	wavRect.left = wavRect.right + 5;
	wavRect.right = wavRect.left + width;
	gLoop2 = gui->Toggle("Loop Voice", gLoop2, wavRect);

	wavRect.left = wavRect.right + 5;
	wavRect.right = wavRect.left + width;
	gLoop3 = gui->Toggle("Loop Game", gLoop3, wavRect);

	wavRect.left = wavRect.right + 5;
	wavRect.right = wavRect.left + width;
	if (gSound1 != 0) {
		if (gui->Button("Stop Piano", wavRect)) {
			SimpleSound::Stop(gSound1);
			gSound1 = 0;
		}
	}
	else {
		if (gui->Button("Play Piano", wavRect)) {
			gSound1 = SimpleSound::Play(*gBus1, *gPiano, gLoop1);
			if (!gLoop1) {
				gSound1 = 0;
			}
		}
	}
	

	wavRect.left = wavRect.right + 5;
	wavRect.right = wavRect.left + width;
	if (gSound2 != 0) {
		if (gui->Button("Stop Voice", wavRect)) {
			SimpleSound::Stop(gSound2);
			gSound2 = 0;
		}
	}
	else {
		if (gui->Button("Play Voice", wavRect)) {
			gSound2 = SimpleSound::Play(*gBus1, *gVoice, gLoop2);
			if (!gLoop2) {
				gSound2 = 0;
			}
		}
	}

	wavRect.left = wavRect.right + 5;
	wavRect.right = wavRect.left + width;
	if (gSound3 != 0) {
		if (gui->Button("Stop Game", wavRect)) {
			SimpleSound::Stop(gSound3);
			gSound3 = 0;
		}
	}
	else {
		if (gui->Button("Play Game", wavRect)) {
			gSound3 = SimpleSound::Play(*gBus2, *gGame, gLoop3);
			if (!gLoop3) {
				gSound3 = 0;
			}
		}
	}

	gui->EndFrame();
}

void Shutdown(void* ptr) {
	UserData* userData = (UserData*)ptr;
	SimpleSound::Device* device = userData->device;
	SimpleSound::Shutdown(device);
	userData->platform.release(userData);
}
#endif

void Update(void* ptr, float deltatTime) { }