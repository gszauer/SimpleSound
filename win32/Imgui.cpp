#define _CRT_SECURE_NO_WARNINGS

#include "Imgui.h"
#include <stdio.h>
#include <math.h>

Imgui::Imgui(HWND hwnd, HDC hdc) {
	mHwnd = hwnd;
	mHotControl = 0;
	mActiveControl = 0;
	mIdGenerator = 1;
	mLeftMouseIsDown = false;
	mLeftMouseWasDown = false;
	mTarget = hdc;

	COLORREF color = RGB(30, 30, 30);
	mBackgroundBrush = CreateSolidBrush(color);
	mBackgroundColor = color;

	mActiveColor = RGB(45, 45, 45);
	mActiveBrush = CreateSolidBrush(mActiveColor);

	mHotColor = RGB(60, 60, 60);
	mHotBrush = CreateSolidBrush(mHotColor);

	mNormalColor = RGB(50, 50, 50);
	mNormalBrush = CreateSolidBrush(mNormalColor);

	mTextNormal = RGB(100, 100, 100);
	mTextHighlight = RGB(120, 120, 120);

	color = RGB(130, 130, 130);
	mPlayheadPen = CreatePen(PS_SOLID, 1, color);
	mPlayheadColor = color;

	TEXTMETRIC textMetrics = {};
	GetTextMetrics(hdc, &textMetrics);
	mTextHeight = textMetrics.tmHeight;
}

Imgui::~Imgui() {
	DeleteObject(mBackgroundBrush);
	DeleteObject(mNormalBrush);
	DeleteObject(mHotBrush);
	DeleteObject(mActiveBrush);
	DeleteObject(mPlayheadPen);
}

void Imgui::BeginFrame() {
	mIdGenerator = 1; // 0 is invalid
	mHotControl = 0;

	POINT mousePos = {};
	GetCursorPos(&mousePos);
	ScreenToClient(mHwnd, &mousePos);
	
	mMouseX = mousePos.x;
	mMouseY = mousePos.y;

	mLeftMouseWasDown = mLeftMouseIsDown;
	mLeftMouseIsDown = (GetKeyState(VK_LBUTTON) & 0x8000) != 0;

	RECT clientRect = {};
	GetClientRect(mHwnd, &clientRect);
	FillRect(mTarget, &clientRect, mBackgroundBrush);
}

void Imgui::EndFrame() {
	if (mLeftMouseWasDown && !mLeftMouseIsDown) {
		mActiveControl = 0;
	}
}

bool Imgui::ContainsPoint(int x, int y, const RECT& area) {
	if (x >= area.left && x <= area.right) {
		if (y >= area.top && y <= area.bottom) {
			return true;
		}
	}
	return false;
}

bool Imgui::Toggle(const char* label, bool value, const RECT& area) {
	unsigned int widgetId = mIdGenerator++;
	bool result = false;

	// If the mouse is inside the button, it's hot
	if (ContainsPoint(mMouseX, mMouseY, area)) {
		if (mActiveControl == 0 || mActiveControl == widgetId) {
			mHotControl = widgetId;
		}

		if (!mLeftMouseWasDown && mLeftMouseIsDown && mHotControl == widgetId) {
			mActiveControl = widgetId;
		}
		else if (mLeftMouseWasDown && !mLeftMouseIsDown) {
			if (mActiveControl == widgetId) {
				value = !value;
				mActiveControl = 0;
			}
		}
	}

	RECT checkbox = area;
	int width = checkbox.bottom - checkbox.top;
	checkbox.right = checkbox.left + width;

	if (mActiveControl == widgetId) {
		FillRect(mTarget, &checkbox, mActiveBrush);
		SetTextColor(mTarget, mTextNormal);
		SetBkColor(mTarget, mBackgroundColor);
	}
	else if (mHotControl == widgetId) {
		FillRect(mTarget, &checkbox, mHotBrush);
		SetTextColor(mTarget, mTextHighlight);
		SetBkColor(mTarget, mBackgroundColor);
	}
	else {
		FillRect(mTarget, &checkbox, mNormalBrush);
		SetTextColor(mTarget, mTextNormal);
		SetBkColor(mTarget, mBackgroundColor);
	}

	if (value) {
		RECT check = checkbox;
		check.top += 4;
		check.left += 4;
		check.bottom -= 4;
		check.right -= 4;
		FillRect(mTarget, &check, mBackgroundBrush);
	}

	if (label != 0) {
		RECT textArea = {};
		textArea.top = area.top + ((area.bottom - area.top) / 2) - (mTextHeight / 2);
		textArea.bottom = textArea.top + mTextHeight;
		textArea.left = checkbox.right + 10;
		textArea.right = area.right;

		DrawTextA(mTarget, label, strlen(label), &textArea, DT_LEFT | DT_VCENTER);
	}

	return value;
}

bool Imgui::Button(const char* label, const RECT& area) {
	unsigned int widgetId = mIdGenerator++;
	bool result = false;

	// If the mouse is inside the button, it's hot
	if (ContainsPoint(mMouseX, mMouseY, area)) {
		if (mActiveControl == 0 || mActiveControl == widgetId) {
			mHotControl = widgetId;
		}

		if (!mLeftMouseWasDown && mLeftMouseIsDown && mHotControl == widgetId) {
			mActiveControl = widgetId;
		}
		else if (mLeftMouseWasDown && !mLeftMouseIsDown) {
			if (mActiveControl == widgetId) {
				result = true;
				mActiveControl = 0;
			}
		}
	}

	if (mActiveControl == widgetId) {
		FillRect(mTarget, &area, mActiveBrush);
		SetTextColor(mTarget, mTextNormal);
		SetBkColor(mTarget, mActiveColor);
	}
	else if (mHotControl == widgetId) {
		FillRect(mTarget, &area, mHotBrush);
		SetTextColor(mTarget, mTextHighlight);
		SetBkColor(mTarget, mHotColor);
	}
	else {
		FillRect(mTarget, &area, mNormalBrush);
		SetTextColor(mTarget, mTextNormal);
		SetBkColor(mTarget, mNormalColor);
	}

	if (label != 0) {
		RECT textArea = {};
		textArea.top = area.top + ((area.bottom - area.top) / 2) - (mTextHeight / 2);
		textArea.bottom = textArea.top + mTextHeight;
		textArea.left = area.left + 10;
		textArea.right = area.right;

		DrawTextA(mTarget, label, strlen(label), &textArea, DT_LEFT | DT_VCENTER);
	}

	return result;
}

float Imgui::Slider(const char* label, float value, float _min, float _max, const RECT& area) {
	unsigned int widgetId = mIdGenerator++;

	if (ContainsPoint(mMouseX, mMouseY, area)) {
		if (mActiveControl == 0 || mActiveControl == widgetId) {
			mHotControl = widgetId;
		}

		if (!mLeftMouseWasDown && mLeftMouseIsDown && mHotControl == widgetId) {
			mActiveControl = widgetId;
		}
		else if (mLeftMouseWasDown && !mLeftMouseIsDown) {
			if (mActiveControl == widgetId) {
				mActiveControl = 0;
			}
		}
	}

	if (mActiveControl == widgetId) {
		int x = mMouseX;
		if (x < area.left) {
			x = area.left;
		}
		if (x > area.right) {
			x = area.right;
		}

		float t = float(x - area.left) / float(area.right - area.left);
		value = _min + (_max - _min) * t;
	}

	if (mActiveControl == widgetId) {
		FillRect(mTarget, &area, mActiveBrush);
		SetTextColor(mTarget, mTextNormal);
		SetBkColor(mTarget, mActiveColor);
	}
	else if (mHotControl == widgetId) {
		FillRect(mTarget, &area, mHotBrush);
		SetTextColor(mTarget, mTextHighlight);
		SetBkColor(mTarget, mHotColor);
	}
	else {
		FillRect(mTarget, &area, mNormalBrush);
		SetTextColor(mTarget, mTextNormal);
		SetBkColor(mTarget, mNormalColor);
	}

	if (label != 0) {
		RECT textArea = {};
		textArea.top = area.top + ((area.bottom - area.top) / 2) - (mTextHeight / 2);
		textArea.bottom = textArea.top + mTextHeight;
		textArea.left = area.left + 10;
		textArea.right = area.right;

		DrawTextA(mTarget, label, strlen(label), &textArea, DT_LEFT | DT_VCENTER);
	}

	{
		float t = (value - _min) / (_max - _min);
		int xOffset = t * float(area.right - area.left);

		HPEN OldPen = (HPEN)SelectObject(mTarget, mPlayheadPen);
		MoveToEx(mTarget, area.left + xOffset, area.top, NULL);
		LineTo(mTarget, area.left + xOffset, area.bottom);
		SelectObject(mTarget, OldPen);
	}

	return value;
}

namespace ImguiInternal {
	inline void MulMatVec(float* result, float* mat, float* vec) {
		float v[3] = { vec[0], vec[1], vec[2] };

		result[0] = v[0] * mat[0 * 3 + 0] + v[1] * mat[1 * 3 + 0] + v[2] * mat[2 * 3 + 0];
		result[1] = v[0] * mat[0 * 3 + 1] + v[1] * mat[1 * 3 + 1] + v[2] * mat[2 * 3 + 1];
		result[2] = v[0] * mat[0 * 3 + 2] + v[1] * mat[1 * 3 + 2] + v[2] * mat[2 * 3 + 2];
	}

	inline void ScalesWave(HDC target, HPEN pen, short* buffer, unsigned int bufferSize, unsigned int numChannels, const RECT& area) {
		
	}
}

inline float fMin(float _a, float _b) {
	if (_a <= _b) {
		return _a;
	}
	return _b;
}

bool Imgui::Wave_short(short* buffer, unsigned int bufferSize, unsigned int numChannels, const RECT& area, const char* label) {
	unsigned int widgetId = mIdGenerator++;
	bool result = false;

	if (ContainsPoint(mMouseX, mMouseY, area)) {
		if (mActiveControl == 0 || mActiveControl == widgetId) {
			mHotControl = widgetId;
		}

		if (!mLeftMouseWasDown && mLeftMouseIsDown && mHotControl == widgetId) {
			mActiveControl = widgetId;
		}
		else if (mLeftMouseWasDown && !mLeftMouseIsDown) {
			if (mActiveControl == widgetId) {
				result = true;
				mActiveControl = 0;
			}
		}
	}

	// Border
	HPEN OldPen = (HPEN)SelectObject(mTarget, mPlayheadPen);
	MoveToEx(mTarget, area.left, area.top, NULL);
	LineTo(mTarget, area.right, area.top);
	LineTo(mTarget, area.right, area.bottom);
	LineTo(mTarget, area.left, area.bottom);
	LineTo(mTarget, area.left, area.top);

	int border = 10;

	RECT waveArea;
	waveArea.top = area.top + border;
	waveArea.bottom = area.bottom - border;
	waveArea.left = area.left + border;
	waveArea.right = area.right - border;

	if (label != 0) {
		SetTextColor(mTarget, mPlayheadColor);
		SetBkColor(mTarget, mBackgroundColor);
		DrawTextA(mTarget, label, strlen(label), &waveArea, DT_LEFT | DT_VCENTER);
	}

	unsigned int sampleSize = sizeof(short) * numChannels;
	unsigned int numSamplesInBuffer = bufferSize / sampleSize;

	float min = buffer[0];
	float max = buffer[0];
	for (unsigned int i = 0; i < numSamplesInBuffer; ++i) {
		if (buffer[i * numChannels] < min) {
			min = buffer[i * numChannels];
		}
		if (buffer[i * numChannels] > max) {
			max = buffer[i * numChannels];
		}
	}

	float range = max - min;
	float height = waveArea.bottom - waveArea.top;
	float width = waveArea.right - waveArea.left;
	float fNumSamples = float(numSamplesInBuffer);

	for (int x = waveArea.left; x < waveArea.right; ++x) {
		float t = float(x - waveArea.left) / width;
		unsigned int sampleIndex = fNumSamples * t;

		float fSample = float(buffer[sampleIndex * numChannels + 0]);
		float y = 0.0f;
		if (range != 0.0f) {
			y = fMin(fSample - min, range) / range;
		}

		if (x == waveArea.left) {
			MoveToEx(mTarget, x, waveArea.top + y * height, NULL);
		}
		else {
			LineTo(mTarget, x, waveArea.top + y * height);
		}
	}
	SelectObject(mTarget, OldPen);

	return result;
}

bool Imgui::Wave_float(float* fBuffer, unsigned int bufferSize, unsigned int numChannels, const RECT& area, const char* label) {
	unsigned int widgetId = mIdGenerator++;
	bool result = false;

	if (ContainsPoint(mMouseX, mMouseY, area)) {
		if (mActiveControl == 0 || mActiveControl == widgetId) {
			mHotControl = widgetId;
		}

		if (!mLeftMouseWasDown && mLeftMouseIsDown && mHotControl == widgetId) {
			mActiveControl = widgetId;
		}
		else if (mLeftMouseWasDown && !mLeftMouseIsDown) {
			if (mActiveControl == widgetId) {
				result = true;
				mActiveControl = 0;
			}
		}
	}

	// Border
	HPEN OldPen = (HPEN)SelectObject(mTarget, mPlayheadPen);
	MoveToEx(mTarget, area.left, area.top, NULL);
	LineTo(mTarget, area.right, area.top);
	LineTo(mTarget, area.right, area.bottom);
	LineTo(mTarget, area.left, area.bottom);
	LineTo(mTarget, area.left, area.top);

	int border = 10;

	RECT waveArea;
	waveArea.top = area.top + border;
	waveArea.bottom = area.bottom - border;
	waveArea.left = area.left + border;
	waveArea.right = area.right - border;

	if (label != 0) {
		SetTextColor(mTarget, mPlayheadColor);
		SetBkColor(mTarget, mBackgroundColor);
		DrawTextA(mTarget, label, strlen(label), &waveArea, DT_LEFT | DT_VCENTER);
	}

	unsigned int sampleSize = sizeof(short) * numChannels;
	unsigned int numSamplesInBuffer = bufferSize / sampleSize;

	float min = fBuffer[0];
	float max = fBuffer[0];
	for (unsigned int i = 0; i < numSamplesInBuffer; ++i) {
		if (fBuffer[i * numChannels] < min) {
			min = fBuffer[i * numChannels];
		}
		if (fBuffer[i * numChannels] > max) {
			max = fBuffer[i * numChannels];
		}
	}

	float range = max - min;
	float height = waveArea.bottom - waveArea.top;
	float width = waveArea.right - waveArea.left;
	float fNumSamples = float(numSamplesInBuffer);

	for (int x = waveArea.left; x < waveArea.right; ++x) {
		float t = float(x - waveArea.left) / width;
		unsigned int sampleIndex = fNumSamples * t;

		float fSample = fBuffer[sampleIndex * numChannels + 0];
		float y = 0.0f;
		if (range != 0.0f) {
			y = fMin(fSample - min, range) / range;
		}

		if (x == waveArea.left) {
			MoveToEx(mTarget, x, waveArea.top + y * height, NULL);
		}
		else {
			LineTo(mTarget, x, waveArea.top + y * height);
		}
	}
	SelectObject(mTarget, OldPen);

	return result;
}

bool Imgui::ContainsPoint(float pX, float pY, float x, float y, float radius) {
	float dX = pX - x;
	float dY = pY - y;

	float distSq = dX * dX + dY * dY;
	return distSq < (radius* radius);
}

bool Imgui::DraggableCircle(float& x, float& y, float radius) {
	unsigned int widgetId = mIdGenerator++;

	if (ContainsPoint(x, y, mMouseX, mMouseY, radius)) {
		if (mActiveControl == 0 || mActiveControl == widgetId) {
			mHotControl = widgetId;
		}

		if (!mLeftMouseWasDown && mLeftMouseIsDown && mHotControl == widgetId) {
			mActiveControl = widgetId;
		}
		else if (mLeftMouseWasDown && !mLeftMouseIsDown) {
			if (mActiveControl == widgetId) {
				mActiveControl = 0;
			}
		}
	}

	if (mActiveControl == widgetId) {
		x = mMouseX;
		y = mMouseY;
	}

	HBRUSH oldBrush = { 0 };
	if (mActiveControl == widgetId) {
		oldBrush = (HBRUSH)SelectObject(mTarget, mActiveBrush);
		
	}
	else if (mHotControl == widgetId) {
		oldBrush = (HBRUSH)SelectObject(mTarget, mHotBrush);
	}
	else {
		oldBrush = (HBRUSH)SelectObject(mTarget, mNormalBrush);
	}

	int iRad = (int)radius;
	Ellipse(mTarget, x - iRad, y - iRad, x + iRad, y + iRad);
	SelectObject(mTarget, oldBrush);

	return mActiveControl == widgetId;
}

bool Imgui::ListenerWidget(float& x, float& y, float& fwdX, float& fwdY) {
	int radius = 6;

	bool result = DraggableCircle(x, y, radius); // pos widget

	radius = 3;
	float fwdSqLen = fwdX * fwdY + fwdX * fwdY;
	if (fwdSqLen > 0.0001f) {
		if (fwdSqLen < 0.9999f || fwdSqLen > 1.0001f) { // Not 1
			float invFwdLen = 1.0f / sqrtf(fwdSqLen);
			fwdX *= invFwdLen;
			fwdY *= invFwdLen;
		}
	}

	unsigned int widgetId = mIdGenerator++; // fwd widget
	if (!result) {
		if (ContainsPoint(x + fwdX, y + fwdY, mMouseX, mMouseY, radius)) {
			if (mActiveControl == 0 || mActiveControl == widgetId) {
				mHotControl = widgetId;
			}

			if (!mLeftMouseWasDown && mLeftMouseIsDown && mHotControl == widgetId) {
				mActiveControl = widgetId;
			}
			else if (mLeftMouseWasDown && !mLeftMouseIsDown) {
				if (mActiveControl == widgetId) {
					mActiveControl = 0;
				}
			}
		}
	}

	if (mActiveControl == widgetId) {
		float _x = mMouseX;
		float _y = mMouseY;

		fwdX = _x - x;
		fwdY = _y - y;
		fwdSqLen = fwdX * fwdY + fwdX * fwdY;
		float invFwdLen = 1.0f / sqrtf(fwdSqLen);
		fwdX *= invFwdLen;
		fwdY *= invFwdLen;
	}

	HBRUSH oldBrush = { 0 };
	if (mActiveControl == widgetId) {
		oldBrush = (HBRUSH)SelectObject(mTarget, mActiveBrush);

	}
	else if (mHotControl == widgetId) {
		oldBrush = (HBRUSH)SelectObject(mTarget, mHotBrush);
	}
	else {
		oldBrush = (HBRUSH)SelectObject(mTarget, mNormalBrush);
	}

	int iRad = (int)radius;
	Ellipse(mTarget, x + fwdX - iRad, y + fwdY - iRad, x + fwdX + iRad, y + fwdY + iRad);
	SelectObject(mTarget, oldBrush);

	return result || (mActiveControl == widgetId);
}