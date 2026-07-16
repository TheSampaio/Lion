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

		// A window that draws its own caption still has corners, and on Windows 11 the corners are round while
		// it is windowed. Maximised, they are square: a rounded corner on a window filling the screen would
		// show the desktop through the gap, so it is turned off — which is what Windows does with its own.
		void SetCornerRounding(HWND window, bool rounded)
		{
			constexpr DWORD kCornerPreference = 33;   // DWMWA_WINDOW_CORNER_PREFERENCE
			const DWORD preference = rounded ? 2u /* ROUND */ : 1u /* DONOTROUND */;

			DwmSetWindowAttribute(window, kCornerPreference, &preference, sizeof(preference));
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

		// The windowed rectangle to put the window back to when it leaves the maximised state — which it does
		// by being sized to the work area rather than Windows-maximised (see the flash it avoids, below).
		RECT sRestore = {};

		// Sizes the window to the monitor's work area (maximised) or back to sRestore (windowed), in window
		// coordinates, and records the new logical state. The one place the maximise happens, shared by the
		// button, a caption double-click and the drag-off-the-top-edge — so all three behave the same.
		void SetMaximizedState(HWND window, bool maximized, bool captureRestore)
		{
			if (maximized)
			{
				if (captureRestore)
					GetWindowRect(window, &sRestore);

				MONITORINFO info = {};
				info.cbSize = sizeof(info);

				if (GetMonitorInfo(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &info))
					SetWindowPos(window, nullptr, info.rcWork.left, info.rcWork.top,
						info.rcWork.right - info.rcWork.left, info.rcWork.bottom - info.rcWork.top,
						SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
			}
			else
			{
				SetWindowPos(window, nullptr, sRestore.left, sRestore.top,
					sRestore.right - sRestore.left, sRestore.bottom - sRestore.top,
					SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
			}

			// Square corners fill the screen edge to edge; round ones leave the desktop showing through.
			SetCornerRounding(window, !maximized);

			if (sChrome)
				sChrome->maximized = maximized;
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
			if (!sChrome || !sChrome->resizable || sChrome->maximized)
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
				case WM_SYSCOMMAND:
				{
					// Windows must never maximise this window itself — not from a title-bar double-click, not
					// from an Aero snap to the top edge. Its own maximise puts a borderless window over the
					// taskbar and off the top of the screen for a frame before it settles, which is the flash.
					// The editor maximises by sizing to the work area instead (ApplyMaximized), with no flash,
					// so those requests are simply dropped.
					const WPARAM command = wParam & 0xFFF0;

					if (command == SC_MAXIMIZE)
						return 0;

					break;
				}

				case WM_NCLBUTTONDBLCLK:
				{
					// Double-click the caption to maximise or restore, the way a Windows title bar does — through
					// the editor's own sizing, so it does not flash.
					if (wParam == HTCAPTION && sChrome && sChrome->resizable)
					{
						SetMaximizedState(window, !sChrome->maximized, true);
						return 0;
					}

					break;
				}

				case WM_NCLBUTTONDOWN:
				{
					// Grabbing the caption of a maximised window and dragging restores it and lets it follow the
					// cursor — the standard Windows gesture. The window has no real maximised state to leave, so
					// it is done by hand: restore to the windowed size, place it under the cursor at the same
					// horizontal spot on the title bar, then hand the drag back to Windows to run from there.
					//
					// Only a drag does this. DragDetect holds the press until the pointer either moves past the
					// drag threshold — a drag — or lets go where it was — a plain click, which leaves a maximised
					// window maximised. Without it a single click on the caption un-maximised the window.
					if (wParam == HTCAPTION && sChrome && sChrome->maximized && sChrome->resizable)
					{
						const POINT cursor = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

						if (!DragDetect(window, cursor))
							return 0;   // A click, not a drag: leave it maximised.

						RECT maximized = {};
						GetWindowRect(window, &maximized);
						const int32 maximizedWidth = maximized.right - maximized.left;
						const float64 ratioX = maximizedWidth > 0 ? static_cast<float64>(cursor.x - maximized.left) / maximizedWidth : 0.5;

						SetMaximizedState(window, false, false);

						RECT windowed = {};
						GetWindowRect(window, &windowed);
						const int32 windowedWidth = windowed.right - windowed.left;

						// The cursor keeps its place along the title bar, a few pixels down from the top edge.
						const int32 left = cursor.x - static_cast<int32>(ratioX * windowedWidth);
						const int32 top = cursor.y - static_cast<int32>(sChrome->titleBarHeight * 0.25f);

						SetWindowPos(window, nullptr, left, top, windowedWidth, windowed.bottom - windowed.top,
							SWP_NOZORDER | SWP_NOACTIVATE);

						// The button is still down, so re-posting the caption press drops straight into the move.
						ReleaseCapture();
						SendMessage(window, WM_NCLBUTTONDOWN, HTCAPTION, lParam);
						return 0;
					}

					break;
				}

				case WM_NCCALCSIZE:
				{
					if (wParam == FALSE)
						break;

					// The whole window becomes the client area, always: it is never Windows-maximised, so there
					// is no off-screen frame to take back off here — sized to the work area, the window already
					// is the work area.
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
			SetCornerRounding(glfwGetWin32Window(mWindow), !mData->maximized);
		}
#endif

		// Sized to the work area while still hidden, so it opens at exactly that — the whole work area, none of
		// the taskbar — with no maximise for Windows to flash through. The windowed rectangle it restores to is
		// the created size, centred on the work area, so a restore lands it somewhere sensible.
#ifdef LN_PLATFORM_WIN
		const int32 windowedWidth = static_cast<int32>(mData->width);
		const int32 windowedHeight = static_cast<int32>(mData->height);

		MONITORINFO restoreInfo = {};
		restoreInfo.cbSize = sizeof(restoreInfo);

		if (GetMonitorInfo(MonitorFromWindow(glfwGetWin32Window(mWindow), MONITOR_DEFAULTTONEAREST), &restoreInfo))
		{
			sRestore.left = restoreInfo.rcWork.left + ((restoreInfo.rcWork.right - restoreInfo.rcWork.left) - windowedWidth) / 2;
			sRestore.top = restoreInfo.rcWork.top + ((restoreInfo.rcWork.bottom - restoreInfo.rcWork.top) - windowedHeight) / 2;
			sRestore.right = sRestore.left + windowedWidth;
			sRestore.bottom = sRestore.top + windowedHeight;
		}
#endif

		if (mData->maximized)
			ApplyMaximized(true, false);   // Keep the centred windowed rectangle just set as the restore.

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
		// The window is already at its final geometry — sized to the work area in Initialize when it opens
		// maximised — so showing it is one plain call and the first frame is the right one.
		glfwShowWindow(mWindow);
	}

	// Sizes the window to the monitor's work area (the maximised look) or back to the windowed rect. Done by
	// hand, in window coordinates, rather than through Windows' maximise (which flashes a borderless window on
	// the way in) or GLFW's set-size (which speaks in content size and assumes a frame this window does not
	// have, so it lands off by the frame it imagines).
	void GlfwWindow::ApplyMaximized(bool maximized, bool captureRestore)
	{
#ifdef LN_PLATFORM_WIN
		// The button and the boot both go through the one place the maximise lives, so they cannot drift from
		// the caption double-click and the drag-off-the-top that share it.
		SetMaximizedState(glfwGetWin32Window(mWindow), maximized, captureRestore);
#else
		(void)captureRestore;
		if (maximized) glfwMaximizeWindow(mWindow); else glfwRestoreWindow(mWindow);
		mData->maximized = maximized;
#endif
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
		ApplyMaximized(!mData->maximized);
	}

	bool GlfwWindow::IsMaximized() const
	{
		// The manual state, not Windows' — the window is never Windows-maximised (see ApplyMaximized).
		return mData->maximized;
	}

	void* GlfwWindow::GetNativeHandle() const
	{
		return mWindow;
	}
}
