#ifndef UNICODE
#define UNICODE
#endif

#define RESOLUTION_W 320
#define RESOLUTION_H 200
#define WINDOWCLASS L"OpenGL"

#include <Windows.h>
#include <gl\GL.h>
#include <cwchar>
#include <cassert>

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
bool WindowCreate();
void WindowKill();
void SceneResize(GLsizei width, GLsizei height);
void SceneDraw();

void TransformVgaToRgb(unsigned char* vgaBuffer);

void ExampleDrawing();

static HGLRC rc = nullptr; // rendering context
static HDC dc = nullptr; // GDI device context
static HWND w = nullptr; // window
static HINSTANCE appInstance = nullptr; // application instance
static bool active; // is app visible or minimized?

const static UINT_PTR IDT_TIMER1 = 1;

static int windowWidth = RESOLUTION_W;
static int windowHeight = RESOLUTION_H;

static unsigned char exampleVgaBuffer[RESOLUTION_W * RESOLUTION_H] = { 0 };
static GLubyte rgbBuffer[RESOLUTION_W * RESOLUTION_H * 3] = { 0 };

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int)
{
	appInstance = instance;

	ExampleDrawing();
	TransformVgaToRgb(exampleVgaBuffer);

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
	// make it so that scene covers the whole drawable area
	// and the scene coordinates match to window coordinates
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0, width, 0.0, height, -1.0, 1.0);
	glScalef(1.0f, -1.0f, 1.0f);
	glTranslatef(0.0f, -height, 0.0f);

	windowWidth = width;
	windowHeight = height;
	wchar_t windowTitle[31] = {0};
	wsprintf(windowTitle, L"Mode 13h Simulator %dx%d", width, height);
	SetWindowText(w, windowTitle);
}

void SceneDraw()
{
	glClear(GL_COLOR_BUFFER_BIT);

	glEnable(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, RESOLUTION_W, RESOLUTION_H, 0, GL_RGB, GL_UNSIGNED_BYTE, rgbBuffer);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 1.0f);
	glVertex2i(0, 0);
	glTexCoord2f(1.0f, 1.0f);
	glVertex2i(windowWidth, 0);
	glTexCoord2f(1.0f, 0.0f);
	glVertex2i(windowWidth, windowHeight);
	glTexCoord2f(0.0f, 0.0f);
	glVertex2i(0, windowHeight);
	glEnd();

	glDisable(GL_TEXTURE_2D);

	glFlush();
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
	if (!UnregisterClass(WINDOWCLASS, appInstance))
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
	windowRectangle.right = RESOLUTION_W;
	windowRectangle.top = 0;
	windowRectangle.bottom = RESOLUTION_H;

	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; // redraw on move, own DC for window
	wc.lpfnWndProc = WindowProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = appInstance;
	wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = WINDOWCLASS;

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
		WINDOWCLASS,
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
	SceneResize(RESOLUTION_W, RESOLUTION_H);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	return true;
}

void TransformVgaToRgb(unsigned char* vgaBuffer)
{
	unsigned char *p = vgaBuffer;
	GLubyte *q = rgbBuffer;

	for (int i = 0; i < RESOLUTION_W * RESOLUTION_H; ++i)
	{
		// currently, custom color pallette is not supported... only the first 16 VGA colors (default)

		switch (*p)
		{
		case 0x00: // black
			*q++ = 0x00; *q++ = 0x00; *q++ = 0x00;
			break;
		case 0x01: // blue
			*q++ = 0x00; *q++ = 0x00; *q++ = 0x80;
			break;
		case 0x02: // green
			*q++ = 0x00; *q++ = 0x80; *q++ = 0x00;
			break;
		case 0x03: // cyan
			*q++ = 0x00; *q++ = 0x80; *q++ = 0x80;
			break;
		case 0x04: // red
			*q++ = 0x80; *q++ = 0x00; *q++ = 0x00;
			break;
		case 0x05: // magenta
			*q++ = 0x80; *q++ = 0x00; *q++ = 0x80;
			break;
		case 0x06: // brown
			*q++ = 0x80; *q++ = 0x80; *q++ = 0x00;
			break;
		case 0x07: // light gray
			*q++ = 0xc0; *q++ = 0xc0; *q++ = 0xc0;
			break;
		case 0x08: // dark gray
			*q++ = 0x80; *q++ = 0x80; *q++ = 0x80;
			break;
		case 0x09: // light blue
			*q++ = 0x00; *q++ = 0x00; *q++ = 0xff;
			break;
		case 0x0a: // light green
			*q++ = 0x00; *q++ = 0xff; *q++ = 0x00;
			break;
		case 0x0b: // light cyan
			*q++ = 0x00; *q++ = 0xff; *q++ = 0xff;
			break;
		case 0x0c: // light red
			*q++ = 0xff; *q++ = 0x00; *q++ = 0x00;
			break;
		case 0x0d: // light magenta
			*q++ = 0xff; *q++ = 0x00; *q++ = 0xff;
			break;
		case 0x0e: // yellow
			*q++ = 0xff; *q++ = 0xff; *q++ = 0x00;
			break;
		case 0x0f: // white
			*q++ = 0xff; *q++ = 0xff; *q++ = 0xff;
			break;
		default:
			assert(0);
		}

		++p;
	}
}

// To avoid multiplication we use: 320 * y = 256 * y + 64 * y = 2^8 * y + 2^6 * y
#define PlotPixel(vga, x, y, color) vga[(y << 8) + (y << 6) + x] = color

template <typename T> int sgn(T value)
{
	return (T(0) < value) - (value < T(0));
}

/* Bresenham's line algorithm */
void PlotLine(int x0, int y0, int x1, int y1, unsigned char color)
{
	assert(x0 >= 0 && x0 < 320);
	assert(x1 >= 0 && x1 < 320);
	assert(y0 >= 0 && y0 < 200);
	assert(y1 >= 0 && y1 < 200);

	const int dx = x1 - x0;
	const int dy = y1 - y0;
	const int dxabs = abs(dx);
	const int dyabs = abs(dy);
	const int sdx = sgn(dx);
	const int sdy = sgn(dy);
	int x = dyabs >> 1;
	int y = dxabs >> 1;
	int px = x0;
	int py = y0;

	PlotPixel(exampleVgaBuffer, px, py, color);

	if (dxabs >= dyabs) // the line is more horizontal than vertical
	{
		for (int i = 0; i < dxabs; ++i)
		{
			y += dyabs;
			if (y >= dxabs)
			{
				y -= dxabs;
				py += sdy;
			}
			px += sdx;
			PlotPixel(exampleVgaBuffer, px, py, color);
		}
	}
	else // the line is more vertical than horizontal
	{
		for (int i = 0; i < dyabs; ++i)
		{
			x += dxabs;
			if (x >= dyabs)
			{
				x -= dyabs;
				px += sdx;
			}
			py += sdy;
			PlotPixel(exampleVgaBuffer, px, py, color);
		}
	}
}

void ExampleDrawing()
{
	// green
	PlotLine(0, 100, 319, 100, 0x0a);
	// cyan
	PlotLine(160, 0, 160, 199, 0x0b);
	// red
	PlotLine(0, 0, 319, 199, 0x0c);
	// magenta
	PlotLine(0, 199, 319, 0, 0x0d);
}

