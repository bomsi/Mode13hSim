#ifndef UNICODE
#define UNICODE
#endif

#include <Windows.h>

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, const int cmdShow)
{
	const wchar_t className[] = L"main";
	WNDCLASS wc = { 0 };
	const int windowWidth = 320;
	const int windowHeight = 200;

	wc.lpfnWndProc = WindowProc;
	wc.hInstance = instance;
	wc.lpszClassName = className;

	RegisterClass(&wc);

	HWND window = CreateWindowEx(
		0,
		className,
		L"Mode 13h Simulator",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowWidth,
		windowHeight,
		nullptr,
		nullptr,
		instance,
		nullptr
	);

	if (window == nullptr)
		return 1;
	
	ShowWindow(window, cmdShow);

	MSG message = { 0 };
	while (GetMessage(&message, nullptr, 0, 0))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	return 0;
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_PAINT:
		PAINTSTRUCT ps = { 0 };
		HDC dc = BeginPaint(window, &ps);
		FillRect(dc, &ps.rcPaint, reinterpret_cast<HBRUSH>(COLOR_WINDOW));
		EndPaint(window, &ps);
	}

	return DefWindowProc(window, message, wParam, lParam);
}