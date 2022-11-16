#ifndef _H_IMGUI_
#define _H_IMGUI_

#include <windows.h>

class Imgui {
public:
	unsigned int mHotControl;
	unsigned int mActiveControl;
	unsigned int mIdGenerator;
	unsigned int mMouseX;
	unsigned int mMouseY;
	bool mLeftMouseIsDown;
	bool mLeftMouseWasDown;
	HWND mHwnd;
	HDC mTarget;
	HBRUSH mBackgroundBrush;
	COLORREF mBackgroundColor;
	HBRUSH mNormalBrush;
	COLORREF mNormalColor;
	HBRUSH mHotBrush;
	COLORREF mHotColor;
	HBRUSH mActiveBrush;
	COLORREF mActiveColor;
	unsigned int mTextHeight;
	COLORREF mTextNormal;
	COLORREF mTextHighlight;
	HPEN mPlayheadPen;
	COLORREF mPlayheadColor;
protected:
	bool ContainsPoint(int x, int y, const RECT& area);
	bool ContainsPoint(float pX, float pY, float x, float y, float radius);
public:
	Imgui(HWND hwnd, HDC hdc);
	~Imgui();

	void BeginFrame();
	void EndFrame();

	bool Button(const char* label, const RECT& area);
	float Slider(const char* label, float value, float _min, float _max, const RECT& area);
	bool Toggle(const char* label, bool value, const RECT& area);
	
	bool Wave_short(short* buffer, unsigned int bufferSize, unsigned int numChannels, const RECT& area, const char* label = 0);
	bool Wave_float(float* buffer, unsigned int bufferSize, unsigned int numChannels, const RECT& area, const char* label = 0);

	bool DraggableCircle(float& x, float& y, float radius);
	bool ListenerWidget(float& x, float& y, float& fwdX, float& fwdY);
};

#endif