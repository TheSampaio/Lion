#include "Engine.h"
#include "Header/Cursor.h"

#include "../Core/Header/Window.h"

Lion::Cursor::Cursor()
	: m_hCursor(nullptr)
{
}

void Lion::Cursor::SetCursor(const uint Cursor)
{
	GetInstance().m_hCursor = LoadCursor(Window::GetInstance().m_hInstance, MAKEINTRESOURCE(Cursor));
}
