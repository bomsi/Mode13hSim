#ifndef UNICODE
#define UNICODE
#endif

#include <Windows.h>
#include <gl\GL.h>
#include <gl\GLU.h>

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
bool WindowCreate();
void WindowKill();
void SceneResize(GLsizei width, GLsizei height);
void SceneDraw();

static HGLRC rc = nullptr; // rendering context
static HDC dc = nullptr; // GDI device context
static HWND w = nullptr; // window
static HINSTANCE appInstance = nullptr; // application instance
static bool active; // is app visible or minimized?

const int windowWidth = 320;
const int windowHeight = 200;
const wchar_t* windowClass = L"OpenGL";
const static UINT_PTR IDT_TIMER1 = 1;

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int)
{
	appInstance = instance;

	if (!WindowCreate())
		return 0;

	SetTimer(w, IDT_TIMER1, 15, nullptr);

	MSG message;
	while (GetMessage(&message, w, 0, 0))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	KillTimer(w, IDT_TIMER1);
	WindowKill();
	return message.wParam;
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CLOSE:
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_ACTIVATE:
		active = HIWORD(wParam) == 0;
		return 0;
	case WM_SYSCOMMAND:
		switch (wParam)
		{
		case SC_SCREENSAVE:
		case SC_MONITORPOWER:
			return 0;
		}
		break;
	case WM_SIZE:
		SceneResize(LOWORD(lParam), HIWORD(lParam));
		return 0;
	case WM_TIMER:
		switch (wParam)
		{
		case IDT_TIMER1:
			if (active)
			{
				SceneDraw();
				SwapBuffers(dc);
			}
			return 0;
		}
		break;
	}

	return DefWindowProc(window, message, wParam, lParam);
}

void SceneResize(GLsizei width, GLsizei height)
{
	if (height == 0)
		height = 1;
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, static_cast<GLdouble>(width) / static_cast<GLdouble>(height), 0.1, 100.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void SceneDraw()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	// TODO draw
}

void WindowKill()
{
	const wchar_t* title = L"WindowKill";

	if (rc)
	{
		if (!wglMakeCurrent(nullptr, nullptr))
			MessageBox(nullptr, L"Detaching RC from DC failed.", title, MB_OK | MB_ICONEXCLAMATION);
		if (!wglDeleteContext(rc))
			MessageBox(nullptr, L"Releasing RC failed.", title, MB_OK | MB_ICONEXCLAMATION);
		rc = nullptr;
	}
	if (dc && !ReleaseDC(w, dc))
	{
		MessageBox(nullptr, L"Releasing DC failed.", title, MB_OK | MB_ICONEXCLAMATION);
		dc = nullptr;
	}
	if (w && !DestroyWindow(w))
	{
		MessageBox(nullptr, L"Could not destroy window.", title, MB_OK | MB_ICONEXCLAMATION);
		w = nullptr;
	}
	if (!UnregisterClass(windowClass, appInstance))
	{
		MessageBox(nullptr, L"Could not unregister the window class.", title, MB_OK | MB_ICONEXCLAMATION);
		appInstance = nullptr;
	}
}

bool WindowCreate()
{
	WNDCLASS wc;
	RECT windowRectangle;
	windowRectangle.left = 0;
	windowRectangle.right = windowWidth;
	windowRectangle.top = 0;
	windowRectangle.bottom = windowWidth;

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; // redraw on move, own DC for window
	wc.lpfnWndProc = WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = appInstance;
	wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = windowClass;

	const wchar_t* title = L"WindowCreate";
	if (!RegisterClass(&wc))
	{
		MessageBox(nullptr, L"Could not register the window class.", title, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	// adjust to full requested size
	AdjustWindowRectEx(&windowRectangle, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW | WS_EX_WINDOWEDGE);

	w = CreateWindowEx(
		WS_EX_APPWINDOW | WS_EX_WINDOWEDGE,
		windowClass,
		L"Mode 13h Simulator",
		WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowRectangle.right - windowRectangle.left,
		windowRectangle.bottom - windowRectangle.top,
		nullptr,
		nullptr,
		appInstance,
		nullptr
	);
	if (!w)
	{
		WindowKill();
		MessageBox(nullptr, L"Could not create window.", title, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	static	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW |
		PFD_SUPPORT_OPENGL |
		PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		16,
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		16,
		0,
		0,
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};

	dc = GetDC(w);
	if (!dc)
	{
		WindowKill();
		MessageBox(nullptr, L"Could not get the DC.", title, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	const GLuint pixelformat = ChoosePixelFormat(dc, &pfd);
	if (!pixelformat)
	{
		WindowKill();
		MessageBox(nullptr, L"Could not find a suitable PixelFormat.", title, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	if (!SetPixelFormat(dc, pixelformat, &pfd))
	{
		WindowKill();
		MessageBox(nullptr, L"Could not set the PixelFormat.", title, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	rc = wglCreateContext(dc);
	if (!rc)
	{
		WindowKill();
		MessageBox(nullptr, L"Could not create the RC.", title, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	if (!wglMakeCurrent(dc, rc))
	{
		WindowKill();
		MessageBox(nullptr, L"Could not activate the RC.", title, MB_OK | MB_ICONEXCLAMATION);
		return false;
	}

	ShowWindow(w, SW_SHOW);
	SetForegroundWindow(w);
	SetFocus(w);
	SceneResize(windowWidth, windowHeight);

	glShadeModel(GL_SMOOTH);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

	return true;
}
