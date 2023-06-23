#include "Engine.h"
#include "Header/Cursor.h"

#include "Header/Window.h"

owl::Cursor::Cursor()
	: m_hCursor(nullptr)
{
}

void owl::Cursor::SetCursor(const uint Cursor)
{
	GetInstance().m_hCursor = LoadCursor(Window::GetInstance().m_hInstance, MAKEINTRESOURCE(Cursor));
}
