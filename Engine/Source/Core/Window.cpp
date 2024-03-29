#include "Engine.h"
#include "Header/Window.h"

#include "../Event/Header/Debug.h"
#include "../Event/Header/Input.h"
#include "../Logic/Header/Cursor.h"

Lion::Window::Window()
	: m_hInstance(nullptr), m_hWindow(nullptr), m_hIcon(nullptr)
{
	m_hInstance = GetModuleHandle(0);
	m_BackgroundColour = RGB(0, 0, 0);
	m_Style = WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE | WS_EX_TOPMOST;
	m_Title = "Window";
	m_Screen = { static_cast<ushort>(GetSystemMetrics(SM_CXSCREEN)), static_cast<ushort>(GetSystemMetrics(SM_CYSCREEN)) };
	m_Size = { 800, 600 };
	m_Center = { static_cast<ushort>(m_Size[0] / 2), static_cast<ushort>(m_Size[1] / 2) };
	m_Position = { static_cast<ushort>(m_Screen[0] / 2 - m_Size[0] / 2), static_cast<ushort>(m_Screen[1] / 2 - m_Size[1] / 2) };
	m_DisplayMode = Windowed;
}

bool Lion::Window::Create()
{
	// Set-up the window
	WNDCLASSEX WndClass = {};
	LPCWSTR WndName = L"Window";

	WndClass.cbSize = sizeof(WndClass);
	WndClass.lpfnWndProc = Input::Procedure;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = m_hInstance;
	WndClass.hIcon = m_hIcon;
	WndClass.hIconSm = m_hIcon;
	WndClass.hCursor = Cursor::GetInstance().m_hCursor;
	WndClass.hbrBackground = static_cast<HBRUSH>(CreateSolidBrush(m_BackgroundColour));
	WndClass.lpszClassName = WndName;

	// Register the window
	if (!RegisterClassEx(&WndClass))
	{
		Debug::Message(Error, "Failed to register the window's class.");
		return false;
	}

	if (m_DisplayMode != Windowed)
	{
		m_Position = { 0, 0 };
		m_Style = WS_POPUP | WS_VISIBLE | WS_EX_TOPMOST;

		if (m_DisplayMode == Borderless)
			m_Size = { m_Screen[0], m_Screen[1] };
	}

	else
	{
		// Creates a rect for the new client area inside the window
		RECT Rect = { 0, 0, m_Size[0], m_Size[1]};

		// Adjusts rect's size
		AdjustWindowRectEx(&Rect, GetWindowStyle(m_hWindow), (GetMenu(m_hWindow) != nullptr), GetWindowExStyle(m_hWindow));

		// Adjust window's position
		m_Position[0] = static_cast<ushort>(m_Screen[0] / 2 - (Rect.right - Rect.left) / 2);
		m_Position[1] = static_cast<ushort>(m_Screen[1] / 2 - (Rect.bottom - Rect.top) / 2);

		MoveWindow(
			m_hWindow,               // Window's id
			m_Position[0],           // X position 
			m_Position[1],           // Y position
			Rect.right - Rect.left,  // Width
			Rect.bottom - Rect.top,  // Height
			true);                   // Redraw
	}

	// Creates the window
	m_hWindow = CreateWindowEx(
		0,                                                    // Extras styles
		WndName,                                              // Window class's name
		std::wstring(m_Title.begin(), m_Title.end()).data(),  // Window's title
		m_Style,                                              // Window's style
		m_Position[0], m_Position[1],                         // Initial position
		m_Size[0], m_Size[1],                                 // Window's size
		nullptr,                                              // Father window's id
		nullptr,                                              // Menu's id
		m_hInstance,                                          // Application's
		nullptr);                                             // Other creation parameters

	// Returns the window's initialization state
	return (m_hWindow) ? true : false;
}
