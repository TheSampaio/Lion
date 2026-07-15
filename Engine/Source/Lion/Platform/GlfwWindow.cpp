#include "Engine.h"
#include "GlfwWindow.h"

#include <Lion/Core/Filesystem.h>
#include <Lion/Core/Log.h>
#include <Lion/Render/RendererAPI.h>
#include <Lion/Signal/EventInput.h>
#include <Lion/Signal/EventWindow.h>

#ifdef LN_PLATFORM_WIN
	#define GLFW_EXPOSE_NATIVE_WIN32
	#include <GLFW/glfw3native.h>

	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#include <Windows.h>
	#include <windowsx.h>
	#include <dwmapi.h>

	// windowsx.h defines IsMaximized as a macro over IsZoomed, which quietly rewrites any method that
	// happens to be called that — including this backend's own.
	#undef IsMaximized
	#undef IsMinimized
	#undef IsRestored

	#pragma comment(lib, "Dwmapi.lib")
#endif

namespace Lion
{
	namespace
	{
#ifdef LN_PLATFORM_WIN
		// The title bar is the one part of a window the application does not draw, so it is the one part
		// that can disagree with everything the application does draw. This is the whole of the API for
		// handing it over, and on a Windows that does not have it, it does nothing.
		void UseDarkTitleBar(GLFWwindow* window)
		{
			constexpr DWORD kUseImmersiveDarkMode = 20;
			const BOOL dark = TRUE;

			DwmSetWindowAttribute(glfwGetWin32Window(window), kUseImmersiveDarkMode, &dark, sizeof(dark));
		}

		// A window that draws its own caption still has corners, and on Windows 11 the corners are round.
		// Taking the caption away is not a reason for the window to stop looking like a window.
		void UseRoundedCorners(GLFWwindow* window)
		{
			constexpr DWORD kCornerPreference = 33;
			constexpr DWORD kRound = 2;

			DwmSetWindowAttribute(glfwGetWin32Window(window), kCornerPreference, &kRound, sizeof(kRound));
		}

		// A caption the application draws itself.
		//
		// The window keeps its frame — that is what resizing, snapping and the aero shortcuts hang off —
		// and gives up only the strip Windows would have drawn a caption into, which the client area then
		// covers. Two messages do it: one hands the strip over, and one answers, for every pixel, whether
		// it is a border to drag by, the caption to move the window by, or the application's own.
		//
		// The state is a single window's, because the engine has a single window.
		WNDPROC sOriginalProc = nullptr;
		WindowData* sChrome = nullptr;

		int32 FrameBorder()
		{
			return GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
		}

		// What the cursor has to be within to be resizing rather than clicking. The frame Windows reports is
		// what it *reserves*, not what a hand can hit — the real one is invisible now, and a border you have
		// to find is a border that is not there. A corner is given more, because two edges meet in it.
		constexpr int32 kGrabBorder = 8;
		constexpr int32 kGrabCorner = 16;

		// Which edge, if any, the cursor is on. Zero for none, so a caller can ask the question without
		// spelling out the eight answers — and so the answer is only worked out in one place.
		LRESULT ResizeBorderAt(HWND window, POINT cursor)
		{
			if (!sChrome || !sChrome->resizable || IsZoomed(window))
				return 0;

			RECT bounds = {};
			GetWindowRect(window, &bounds);

			// The corners are tested against the wider band, and first: a cursor in the corner is within both
			// edges, and the diagonal is the one it means.
			const bool leftCorner = cursor.x < bounds.left + kGrabCorner;
			const bool rightCorner = cursor.x >= bounds.right - kGrabCorner;
			const bool topCorner = cursor.y < bounds.top + kGrabCorner;
			const bool bottomCorner = cursor.y >= bounds.bottom - kGrabCorner;

			if (topCorner && leftCorner)     return HTTOPLEFT;
			if (topCorner && rightCorner)    return HTTOPRIGHT;
			if (bottomCorner && leftCorner)  return HTBOTTOMLEFT;
			if (bottomCorner && rightCorner) return HTBOTTOMRIGHT;

			if (cursor.x < bounds.left + kGrabBorder)    return HTLEFT;
			if (cursor.x >= bounds.right - kGrabBorder)  return HTRIGHT;
			if (cursor.y < bounds.top + kGrabBorder)     return HTTOP;
			if (cursor.y >= bounds.bottom - kGrabBorder) return HTBOTTOM;

			return 0;
		}

		LRESULT CALLBACK CustomChromeProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
		{
			switch (message)
			{
				case WM_GETMINMAXINFO:
				{
					// A borderless window maximises to the whole monitor, taskbar and all, and then snaps back
					// to the work area a frame later — the reajuste you see on open. This pins the maximised
					// size and position to the work area up front, so it opens at the right size and stays.
					HMONITOR monitor = MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST);

					MONITORINFO info = {};
					info.cbSize = sizeof(info);

					if (monitor && GetMonitorInfo(monitor, &info))
					{
						MINMAXINFO* bounds = reinterpret_cast<MINMAXINFO*>(lParam);

						bounds->ptMaxPosition.x = info.rcWork.left - info.rcMonitor.left;
						bounds->ptMaxPosition.y = info.rcWork.top - info.rcMonitor.top;
						bounds->ptMaxSize.x = info.rcWork.right - info.rcWork.left;
						bounds->ptMaxSize.y = info.rcWork.bottom - info.rcWork.top;
						bounds->ptMaxTrackSize = bounds->ptMaxSize;
					}

					return 0;
				}

				case WM_NCCALCSIZE:
				{
					if (wParam == FALSE)
						break;

					// The whole window becomes the client area. Maximised, the frame still hangs off the
					// screen the way Windows expects, so it has to be taken off here or the window spills
					// over the taskbar.
					NCCALCSIZE_PARAMS* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam);

					if (IsZoomed(window))
					{
						const int32 border = FrameBorder();

						params->rgrc[0].left += border;
						params->rgrc[0].right -= border;
						params->rgrc[0].top += border;
						params->rgrc[0].bottom -= border;
					}

					return 0;
				}

				case WM_NCHITTEST:
				{
					// Nothing is non-client anymore, so Windows would call every pixel HTCLIENT and the
					// window would neither resize nor move. This says which is which.
					POINT cursor = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

					const LRESULT edge = ResizeBorderAt(window, cursor);

					// Remembered, and not merely returned, because the answer is needed by the frame the GUI
					// is about to draw: it is what tells the GUI not to set a cursor over an edge.
					if (sChrome)
						sChrome->pointerOnResizeBorder = (edge != 0);

					if (edge != 0)
						return edge;

					POINT client = cursor;
					ScreenToClient(window, &client);

					// The caption is what the application drew and nothing it drew in it: a drag that began
					// on a menu would open nothing and move everything.
					if (sChrome && !sChrome->titleBarBlocked &&
						client.y >= 0 && client.y < static_cast<int32>(sChrome->titleBarHeight))
						return HTCAPTION;

					return HTCLIENT;
				}
			}

			return CallWindowProc(sOriginalProc, window, message, wParam, lParam);
		}

		void UseCustomTitleBar(GLFWwindow* window, WindowData* data)
		{
			HWND handle = glfwGetWin32Window(window);

			sChrome = data;
			sOriginalProc = reinterpret_cast<WNDPROC>(
				SetWindowLongPtr(handle, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&CustomChromeProc)));

			// The frame is only recalculated when the window is told its frame changed.
			SetWindowPos(handle, nullptr, 0, 0, 0, 0,
				SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
		}
#endif
	}
	GlfwWindow::~GlfwWindow()
	{
		if (mIcon)
		{
			stbi_image_free(mIcon->pixels);
			delete mIcon;
		}

		if (mWindow)
			glfwDestroyWindow(mWindow);

		glfwTerminate();
	}

	bool GlfwWindow::Initialize(WindowData* data)
	{
		mData = data;

		if (!glfwInit())
		{
			Log::Console(LogLevel::Fatal, "[GlfwWindow] GLFW initialization failed.");
			return false;
		}

		// Match the window's client API to the selected graphics backend: an OpenGL context for
		// OpenGL, or no context for Vulkan (which manages its own surface and device).
		glfwWindowHint(GLFW_CLIENT_API, RendererAPI::GetAPI() == GraphicsAPI::OpenGL ? GLFW_OPENGL_API : GLFW_NO_API);
		glfwWindowHint(GLFW_VISIBLE, false);
		glfwWindowHint(GLFW_RESIZABLE, mData->resizable ? GLFW_TRUE : GLFW_FALSE);

		// Created at its windowed size, always — not maximised through the hint. The window is hidden until
		// Show(), so it is maximised here while nobody is looking: there is no create-then-resize jump to see,
		// and GLFW remembers the created size as the one to restore to. Maximise through the hint instead, and
		// the window opens maximised with no windowed size to fall back on — un-maximising then leaves it
		// filling the screen, which is the bug.
		mWindow = glfwCreateWindow(
			static_cast<int32>(mData->width),
			static_cast<int32>(mData->height),
			mData->title.c_str(),
			nullptr, nullptr);

		if (!mWindow)
		{
			Log::Console(LogLevel::Fatal, "[GlfwWindow] Window creation failed.");
			return false;
		}

#ifdef LN_PLATFORM_WIN
		// Only what was asked for: a game's window looks like every other window on the machine.
		if (mData->darkTitleBar)
			UseDarkTitleBar(mWindow);

		if (mData->customTitleBar)
		{
			UseCustomTitleBar(mWindow, mData);
			UseRoundedCorners(mWindow);
		}
#endif

		// Maximised only after the custom chrome is in place, so the maximise is the one our own window
		// procedure sees — it pins the size to the work area (WM_GETMINMAXINFO). Maximise before the subclass
		// is installed and GLFW's default handling runs instead, which lets a borderless window spill over the
		// taskbar and snap back a frame later. Still hidden, so there is nothing to watch either way.
		if (mData->maximized)
			glfwMaximizeWindow(mWindow);

		glfwSetWindowUserPointer(mWindow, mData);
		RegisterCallbacks();

		return true;
	}

	void GlfwWindow::RegisterCallbacks()
	{
		glfwSetWindowCloseCallback(mWindow, [](GLFWwindow* window)
			{
				WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
				if (!data.eventCallback)
					return;

				EventWindowClose event;
				data.eventCallback(event);
			});

		glfwSetWindowFocusCallback(mWindow, [](GLFWwindow* window, int32 focused)
			{
				WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
				if (!data.eventCallback)
					return;

				if (focused)
				{
					EventWindowFocusEnter event;
					data.eventCallback(event);
				}
				else
				{
					EventWindowFocusExit event;
					data.eventCallback(event);
				}
			});

		glfwSetWindowSizeCallback(mWindow, [](GLFWwindow* window, int32 width, int32 height)
			{
				WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
				data.width = static_cast<uint32>(width);
				data.height = static_cast<uint32>(height);

				if (data.eventCallback)
				{
					EventWindowResize event(width, height);
					data.eventCallback(event);
				}

				// Windows keeps the thread for the whole of a resize drag and only lets go through here, so
				// this is where a frame gets drawn while the edge is being pulled. Without it the window
				// holds whatever it had when the drag began, or nothing at all.
				if (data.refreshCallback)
					data.refreshCallback();
			});

		// A window that was covered and is uncovered redraws for the same reason, through the same door.
		glfwSetWindowRefreshCallback(mWindow, [](GLFWwindow* window)
			{
				WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));

				if (data.refreshCallback)
					data.refreshCallback();
			});

		glfwSetKeyCallback(mWindow, [](GLFWwindow* window, int32 key, int32 scancode, int32 action, int32 mods)
			{
				WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
				if (!data.eventCallback)
					return;

				switch (action)
				{
					case GLFW_PRESS:   { EventInputKeyboardPress event(key);   data.eventCallback(event); break; }
					case GLFW_RELEASE: { EventInputKeyboardRelease event(key); data.eventCallback(event); break; }
					case GLFW_REPEAT:  { EventInputKeyboardRepeat event(key);  data.eventCallback(event); break; }
				}
			});

		glfwSetMouseButtonCallback(mWindow, [](GLFWwindow* window, int32 button, int32 action, int32 mods)
			{
				WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
				if (!data.eventCallback)
					return;

				switch (action)
				{
					case GLFW_PRESS:   { EventInputMousePress event(button);   data.eventCallback(event); break; }
					case GLFW_RELEASE: { EventInputMouseRelease event(button); data.eventCallback(event); break; }
				}
			});

		glfwSetScrollCallback(mWindow, [](GLFWwindow* window, float64 xOffset, float64 yOffset)
			{
				WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
				if (!data.eventCallback)
					return;

				EventInputMouseScroll event(static_cast<float32>(xOffset), static_cast<float32>(yOffset));
				data.eventCallback(event);
			});

		glfwSetCursorPosCallback(mWindow, [](GLFWwindow* window, float64 xPos, float64 yPos)
			{
				WindowData& data = *static_cast<WindowData*>(glfwGetWindowUserPointer(window));
				if (!data.eventCallback)
					return;

				EventInputMouseMove event(static_cast<float32>(xPos), static_cast<float32>(yPos));
				data.eventCallback(event);
			});
	}

	void GlfwWindow::Show()
	{
		glfwShowWindow(mWindow);
	}

	void GlfwWindow::PollEvents()
	{
		glfwPollEvents();
	}

	bool GlfwWindow::ShouldClose() const
	{
		return glfwWindowShouldClose(mWindow);
	}

	void GlfwWindow::RequestClose()
	{
		if (mWindow)
			glfwSetWindowShouldClose(mWindow, GLFW_TRUE);
	}

	void GlfwWindow::SetDisplayTitle(const std::string& title)
	{
		if (mWindow)
			glfwSetWindowTitle(mWindow, title.c_str());
	}

	void GlfwWindow::SetResizable(bool enable)
	{
		if (mWindow)
			glfwSetWindowAttrib(mWindow, GLFW_RESIZABLE, enable ? GLFW_TRUE : GLFW_FALSE);
	}

	void GlfwWindow::SetIcon(const std::string& filePath)
	{
		int32 width = 0, height = 0, channels = 0;
		byte* pixels = stbi_load(ResolveResourcePath(filePath).c_str(), &width, &height, &channels, 4); // Force RGBA.

		if (!pixels)
		{
			Log::Console(LogLevel::Error, LION_FORMAT_TEXT("[GlfwWindow] Failed to load icon image: '{}'.", filePath));
			return;
		}

		if (mIcon)
		{
			stbi_image_free(mIcon->pixels);
			delete mIcon;
		}

		mIcon = new GLFWimage();
		mIcon->width = width;
		mIcon->height = height;
		mIcon->pixels = pixels;

		if (mWindow)
			glfwSetWindowIcon(mWindow, 1, mIcon);
	}

	bool GlfwWindow::IsKeyPressed(int32 keyCode) const
	{
		return glfwGetKey(mWindow, keyCode) == GLFW_PRESS;
	}

	bool GlfwWindow::IsKeyReleased(int32 keyCode) const
	{
		return glfwGetKey(mWindow, keyCode) == GLFW_RELEASE;
	}

	void GlfwWindow::Minimize()
	{
		glfwIconifyWindow(mWindow);
	}

	void GlfwWindow::ToggleMaximize()
	{
		if (IsMaximized())
			glfwRestoreWindow(mWindow);
		else
			glfwMaximizeWindow(mWindow);
	}

	bool GlfwWindow::IsMaximized() const
	{
		return glfwGetWindowAttrib(mWindow, GLFW_MAXIMIZED) == GLFW_TRUE;
	}

	void* GlfwWindow::GetNativeHandle() const
	{
		return mWindow;
	}
}
