#include "Engine.h"
#include "Header/Debug.h"

#include "../Core/Header/Window.h"

owl::Debug::Debug()
{
	// Gets the console
	m_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
}

void owl::Debug::IMessage(EDebugMode mode, std::string_view text)
{
	if (mode == Information || mode == Specification)
	{
		MessageBox(Window::GetInstance().m_hWindow, std::wstring(text.begin(), text.end()).data(), L"Information", MB_ICONINFORMATION | MB_OK);
	}

	else if (mode == Warning)
	{
		MessageBox(Window::GetInstance().m_hWindow, std::wstring(text.begin(), text.end()).data(), L"Warning", MB_ICONWARNING | MB_OK);
	}

	else
	{
		MessageBox(Window::GetInstance().m_hWindow, std::wstring(text.begin(), text.end()).data(), L"Error", MB_ICONERROR | MB_OK);
	}
}

void owl::Debug::IConsole(EDebugMode mode, const char* text, bool bBreakLine)
{
	WORD Colour = (mode == Information) ? 3 : (mode == Warning) ? 6 : (mode == Error) ? 4 : 2;
	SetConsoleTextAttribute(m_hConsole, Colour);
	(bBreakLine) ? std::cout << text << std::endl : std::cout << text;
}
