#include "Engine.h"
#include "Header/Debug.h"

#include "../Core/Header/Window.h"

Lion::Debug::Debug()
{
	// Gets the console
	m_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
}

void Lion::Debug::IConsole(EDebugMode Mode, const char* Text, bool bBreakLine, bool bUseTag)
{
	WORD colour = 0;
	const char* tag = "";
	
	switch (Mode)
	{
	case Information:
		colour = 3;
		tag = "[INFO] ";
		break;

	case Warning:
		colour = 6;
		tag = "[WARN] ";
		break;

	case Error:
		colour = 4;
		tag = "[ERRO] ";
		break;

	default:
		colour = 2;
		tag = "[SPEC] ";
		break;
	}

	SetConsoleTextAttribute(m_hConsole, colour);

	if (bUseTag)
		(bBreakLine) ? std::cout << tag << Text << std::endl : std::cout << tag << Text;

	else
		(bBreakLine) ? std::cout << Text << std::endl : std::cout << Text;

	if (Mode == Error)
		char pause = std::getchar();
}

void Lion::Debug::IMessage(EDebugMode Mode, std::string_view Text)
{
	if (Mode == Information || Mode == Specification)
		MessageBox(Window::GetInstance().m_hWindow, std::wstring(Text.begin(), Text.end()).data(), L"Information", MB_ICONINFORMATION | MB_OK);

	else if (Mode == Warning)
		MessageBox(Window::GetInstance().m_hWindow, std::wstring(Text.begin(), Text.end()).data(), L"Warning", MB_ICONWARNING | MB_OK);

	else
		MessageBox(Window::GetInstance().m_hWindow, std::wstring(Text.begin(), Text.end()).data(), L"Error", MB_ICONERROR | MB_OK);
}

bool Lion::Debug::IQuestion(EQuestionMode Mode, std::string_view Text)
{
	int Answer = 0;

	if (Mode == Affirmative)
		Answer = MessageBox(Window::GetInstance().m_hWindow, std::wstring(Text.begin(), Text.end()).data(), L"Question", MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON1);

	else
		Answer = MessageBox(Window::GetInstance().m_hWindow, std::wstring(Text.begin(), Text.end()).data(), L"Question", MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2);

	return (Answer == IDYES) ? true : false;
}
