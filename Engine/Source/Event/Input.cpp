#include "Engine.h"
#include "Header/Input.h"

bool owl::Input::s_Keys[256] = { false };
bool owl::Input::s_Ctrl[256] = { false };

bool owl::Input::IGetKeyTap(EKeyCode keyCode)
{
	if (s_Ctrl[keyCode])
	{
		if (GetKeyRelease(keyCode))
		{
			s_Ctrl[keyCode] = false;
			return true;
		}
	}
	else if (GetKeyPress(keyCode))
	{
		s_Ctrl[keyCode] = true;
	}

	return false;
}

LRESULT owl::Input::Procedure(HWND Window, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		s_Keys[wParam] = true;
		return 0;

	case WM_SYSKEYUP:
	case WM_KEYUP:
		s_Keys[wParam] = false;
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	default:
		return DefWindowProc(Window, Message, wParam, lParam);
	}
}
