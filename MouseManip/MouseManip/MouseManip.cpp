#include <fstream>
#include <iostream>

#include <windows.h>
#include <TlHelp32.h>

#include <limits.h>
#include <stdio.h>
#include <time.h>

struct ScreenExtents
{
	int width;
	int height;
	int width_center;
	int height_center;
	int left_mid_corner_x;
	int left_mid_corner_y;
};

ScreenExtents get_screen_extents();
void init_cursor();
POINT get_cursor_position();
void move_mouse(long posx, long posy);
void sleep(int ms);
void set_keyboard_interrupts(HINSTANCE h_instance);
void set_mouse_interrupts(HINSTANCE h_instance);
void create_console();

LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK keyboard_hook_proc(int code, WPARAM w_param, LPARAM l_param);
LRESULT CALLBACK mouse_hook_proc(int code, WPARAM w_param, LPARAM l_param);
DWORD WINAPI mouse_updater_thread(LPVOID lp_parameter);

static HHOOK mouse_hook = nullptr;
static HHOOK keyboard_hook = nullptr;
static bool running = false;
static bool cancelled = false;

DWORD WINAPI mouse_updater_thread(LPVOID lp_parameter)
{
	while (!cancelled)
	{
		if (running)
		{
			const auto extents = get_screen_extents();
			auto ctr_x = extents.left_mid_corner_x;
			auto ctr_y = extents.left_mid_corner_y;
			move_mouse(ctr_x, ctr_y);
			
			for (auto i = 0; i < 10000; i++)
			{
				move_mouse(++ctr_x, ctr_y);
				if (!running) {
					break;
				}
			}
			for (auto i = 0; i < 10000; i++)
			{
				move_mouse(ctr_x, --ctr_y);
				if (!running) {
					break;
				}
			}
			for (auto i = 0; i < 10000; i++)
			{
				move_mouse(--ctr_x, ctr_y);
				if (!running) {
					break;
				}
			}
			for (auto i = 0; i < 10000; i++)
			{
				move_mouse(ctr_x, ++ctr_y);
				if (!running) {
					break;
				}
			}
		}
		else
		{
			sleep(250);
		}
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdline, int nCmdShow)
{
	// required for scaling properly
	::SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);
	
	create_console();
	
	std::cout << "--- Mouse Mover ---" << std::endl;
	std::cout << "Click Pause+Break to start/stop and Esc to exit." << std::endl;
	
	init_cursor();
	set_keyboard_interrupts(hInstance);
	set_mouse_interrupts(hInstance);

	DWORD mouse_updater_thread_id;
	const auto mouse_updater_handle = CreateThread(
		0, 
		0, 
		mouse_updater_thread, 
		0, 
		0, 
		&mouse_updater_thread_id);
	
	MSG msg = { };
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	WaitForSingleObject(mouse_updater_handle, INFINITE);

	return 0;
}

LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void create_console()
{
	AllocConsole();
	freopen("CONOUT$", "w", stdout);
}

void set_keyboard_interrupts(const HINSTANCE h_instance)
{
	keyboard_hook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboard_hook_proc, h_instance, 0);
}

void set_mouse_interrupts(const HINSTANCE h_instance)
{
	mouse_hook = SetWindowsHookEx(WH_MOUSE_LL, mouse_hook_proc, h_instance, 0);
}

LRESULT CALLBACK mouse_hook_proc(int code, WPARAM w_param, LPARAM l_param)
{
	const auto mouse_hook_struct = reinterpret_cast<KBDLLHOOKSTRUCT*>(l_param);

	if (code < 0 || mouse_hook_struct->flags & 0x10)
	{
		return CallNextHookEx(mouse_hook, code, w_param, l_param);
	}

	if (w_param == WM_MOUSEMOVE)
	{
		POINT point;
		GetCursorPos(&point);
		std::cout << "Move mouse to " << point.x << ", " << point.y << std::endl;
	}

	const auto result = CallNextHookEx(mouse_hook, code, w_param, l_param);
	return result;
}

LRESULT CALLBACK keyboard_hook_proc(const int code, const WPARAM w_param, const LPARAM l_param)
{
	const auto keyboard_hook_struct = reinterpret_cast<KBDLLHOOKSTRUCT*>(l_param);

	if (code < 0 || keyboard_hook_struct->flags & 0x10) {
		return CallNextHookEx(keyboard_hook, code, w_param, l_param);
	}

	if (w_param == WM_KEYDOWN) {
		switch (keyboard_hook_struct->vkCode) {
		case VK_PAUSE:
			running = !running;
			std::cout << (running ? "Running." : "Stopped running.") << std::endl;
			break;
		case VK_ESCAPE:
			cancelled = !cancelled;
			std::cout << "Cancelled." << std::endl;
			break;
		default:
			break;
		}
	}

	const auto result = CallNextHookEx(keyboard_hook, code, w_param, l_param);
	return result;
}

void sleep(int ms)
{
	Sleep(ms);
}

void move_mouse(long posx, long posy)
{
	INPUT input = { 0 };
	input.type = INPUT_MOUSE;
	input.mi.time = 0;
	
	input.mi.dx = LONG(posx);
	input.mi.dy = LONG(posy);

	input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
	
	SendInput(1, &input, sizeof(INPUT));
}

POINT get_cursor_position()
{
	POINT cursor_pos;
	GetCursorPos(&cursor_pos);
	return cursor_pos;
}

void init_cursor()
{
	ShowCursor(0);
	ShowCursor(1);
}

ScreenExtents get_screen_extents()
{
	const auto width = GetSystemMetrics(SM_CXSCREEN) * 30;
	const auto height = GetSystemMetrics(SM_CYSCREEN) * 30;
	const auto width_center = int(width / 2);
	const auto height_center = int(height / 2);
	
	return ScreenExtents{
		width,
		height,
		width_center,
		height_center,
		width_center / 2,
		height_center / 2
	};
}


