#include "Engine.h"
#include "Header/Input.h"

#include "../Core/Header/Application.h"
#include "../Core/Header/Window.h"
#include "../Event/Header/Debug.h"

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
    case WM_SYSKEYUP:
        s_Keys[wParam] = false;
        return 0;

    case WM_SYSKEYDOWN:
        s_Keys[wParam] = true;

        if (s_Keys[Key_F4])
        {
			if (Debug::Question(Negative, "Are you sure you want to quit? This might cause some problems."))
				SendMessage(Window::GetInstance().m_hWindow, WM_DESTROY, 0, 0);

            s_Keys[Key_F4] = false;
        }

        return 0;

	case WM_KEYUP:
		s_Keys[wParam] = false;
		return 0;

    case WM_KEYDOWN:
        s_Keys[wParam] = true;
        return 0;

	case WM_SETFOCUS:
		Application::Resume();
		return 0;

    case WM_KILLFOCUS:
		Application::Pause();
		return 0;

	case WM_DESTROY:
		PostQuitMessage(WM_DESTROY);
		return 0;

	case WM_CLOSE:
		PostQuitMessage(EXIT_SUCCESS);
		return 0;

	default:
		return DefWindowProc(Window, Message, wParam, lParam);
	}
}
