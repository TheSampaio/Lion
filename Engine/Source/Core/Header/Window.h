#pragma once

// Enumerates possible display modes
enum EDisplayMode
{
	Borderless = 0,
	Fullscreen,
	Windowed
};

namespace Lion
{
	class Window
	{
	public:
		// === MAIN methods ======

		// Closes the window
		static void LION_API Close() { SendMessage(GetInstance().m_hWindow, WM_CLOSE, 0, 0); }

		// === GET methods ======

		// Gets window's title
		static std::string LION_API GetTitle() { return GetInstance().m_Title; }

		// Gets window's size
		static std::array<ushort, 2> LION_API GetSize() { return GetInstance().m_Size; }

		// Gets window's center
		static std::array<ushort, 2> LION_API GetCenter() { return GetInstance().m_Center; }

		// Gets window's background colour
		static COLORREF LION_API GetBackgroundColour() { return GetInstance().m_BackgroundColour; }

		// Gets window's display mode
		static EDisplayMode LION_API GetDisplayMode() { return GetInstance().m_DisplayMode; }

		// === SET methods ======

		// Sets the window's title
		static void LION_API SetTitle(const char* Title) { GetInstance().m_Title = Title; }

		// Sets the window's size
		static void LION_API SetSize(const ushort Width, const ushort Height) { GetInstance().m_Size = { Width, Height }; }

		// Sets the window's icon
		static void LION_API SetIcon(const uint Icon) { GetInstance().m_hIcon = LoadIcon(GetInstance().m_hInstance, MAKEINTRESOURCE(Icon)); }

		// Sets the window's background's colour
		static void LION_API SetBackgroudColour(const ushort Red, const ushort Green, const ushort Blue) { GetInstance().m_BackgroundColour = RGB(Red, Green, Blue); }

		// Sets the window's display mode
		static void LION_API SetDisplayMode(const EDisplayMode Mode) { GetInstance().m_DisplayMode = Mode; }

		// === Friends ======

		friend class Application;
		friend class Cursor;
		friend class Debug;
		friend class Input;
		friend class Time;
		friend class Graphics;
		friend class Renderer;

	protected:
		Window();

		// Deletes copy constructor and assigment operator
		Window(const Window&) = delete;
		Window operator=(const Window&) = delete;

		// Gets the class's static reference
		static Window& GetInstance() { static Window s_Instance; return s_Instance; }

	private:
		// Attributes
		HWND m_hWindow;
		HINSTANCE m_hInstance;
		HICON m_hIcon;
		COLORREF m_BackgroundColour;
		DWORD m_Style;

		std::string m_Title;
		std::array<ushort, 2> m_Screen;
		std::array<ushort, 2> m_Size;
		std::array<ushort, 2> m_Center;
		std::array<ushort, 2> m_Position;

		EDisplayMode m_DisplayMode;

		// MAIN methods
		bool Create();
	};
}

