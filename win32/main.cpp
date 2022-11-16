#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 615
#define WINDOW_CLASS "GDI_Sample"
#define WINDOW_TITLE "Double Buffered GDI Sample"

struct WindowData {
	HWND hwnd;
	HDC hdcDisplay; // Front Buffer DC
	HDC hdcMemory;  // Back Buffer DC
	HINSTANCE hInstance; // Window instance
};

void* Initialize(const WindowData& windowData);
void Update(void* userData, float deltatTime);
void Render(void* userData, HDC backBuffer, RECT clientRect);
void Shutdown(void* userData);

int main(int argc, char** argv) {
	return WinMain(GetModuleHandle(NULL), NULL, GetCommandLineA(), SW_SHOWDEFAULT);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow) {
	WNDCLASSA wndclass;
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
	wndclass.lpszMenuName = 0;
	wndclass.lpszClassName = WINDOW_CLASS;
	RegisterClassA(&wndclass);

	RECT rClient;
	SetRect(&rClient, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	AdjustWindowRect(&rClient, WS_OVERLAPPEDWINDOW | WS_VISIBLE, FALSE);

	const char* title_a = WINDOW_TITLE;
	wchar_t title_w[64] = { 0 };
	for (int i = 0; i < 63 && title_a[i] != 0; title_w[i] = title_a[i], ++i);

	HWND hwnd = CreateWindowA(WINDOW_CLASS, (char*)title_w, WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		(GetSystemMetrics(SM_CXSCREEN) / 2) - ((rClient.right - rClient.left) / 2),
		(GetSystemMetrics(SM_CYSCREEN) / 2) - ((rClient.right - rClient.left) / 2),
		(rClient.right - rClient.left), (rClient.bottom - rClient.top), NULL, NULL, hInstance, 0);

	WindowData windowData = { 0 };
	windowData.hwnd = hwnd;
	windowData.hInstance = hInstance;
	HBITMAP hbmBackBuffer; // Back Buffer Bitmap
	HBITMAP hbmOld; // For restoring previous state
	{ // Create back buffer
		windowData.hdcDisplay = GetDC(hwnd);
		windowData.hdcMemory = CreateCompatibleDC(windowData.hdcDisplay);
		hbmBackBuffer = CreateCompatibleBitmap(windowData.hdcDisplay, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
		hbmOld = (HBITMAP)SelectObject(windowData.hdcMemory, hbmBackBuffer);
	}

	LONG_PTR lptr = (LONG_PTR)(&windowData);
	SetWindowLongPtrA(hwnd, GWLP_USERDATA, lptr);

	ShowWindow(hwnd, SW_NORMAL);
	void* userData = Initialize(windowData);

	MSG msg;
	bool running = true;
	bool quit = false;

	LARGE_INTEGER timerFrequency;
	LARGE_INTEGER thisTick;
	LARGE_INTEGER lastTick;
	LARGE_INTEGER timerStart;
	LONGLONG timeDelta;

	if (!QueryPerformanceFrequency(&timerFrequency)) {
		OutputDebugStringA("WinMain: QueryPerformanceFrequency failed\n");
	}

	QueryPerformanceCounter(&thisTick);
	lastTick = thisTick;

	while (!quit) {
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT) {
				quit = true;
				break;
			}
			else if (msg.message == WM_DESTROY) {
				Shutdown(userData);
				{ // Destroy back buffer
					SelectObject(windowData.hdcMemory, hbmOld);
					DeleteObject(hbmBackBuffer);
					DeleteDC(windowData.hdcMemory);
					ReleaseDC(hwnd, windowData.hdcDisplay);
				}
				running = false;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (running) {
			RECT clientRect = {};
			GetClientRect(hwnd, &clientRect);

			QueryPerformanceCounter(&thisTick);
			timeDelta = thisTick.QuadPart - lastTick.QuadPart;
			double deltaTime = (double)timeDelta * 1000.0 / (double)timerFrequency.QuadPart;
			Update(userData, (float)deltaTime);
			Render(userData, windowData.hdcMemory, clientRect);
			lastTick = thisTick;

			InvalidateRect(hwnd, NULL, false);
			Sleep(1);
		}
	}

	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam) {
	switch (iMsg) {
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_ERASEBKGND:
		return TRUE;
	case WM_PAINT:
	{
		LONG_PTR lptr = GetWindowLongPtrA(hwnd, GWLP_USERDATA);
		WindowData* windowData = (WindowData*)lptr;

		RECT rClient = {};
		GetClientRect(hwnd, &rClient);
		BitBlt(windowData->hdcDisplay, 0, 0, rClient.right - rClient.left, rClient.bottom - rClient.top, windowData->hdcMemory, 0, 0, SRCCOPY);
		ValidateRect(hwnd, NULL);
	}
	return TRUE;
	}
	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}