#include "WindowLayer.h"

using namespace Lion;

void WindowLayer::OnAttach()
{
	Window::SetSize(800, 600);
	//Window::SetSize(1280, 720);
	Window::SetTitle("Brickout");
	Window::SetBackgroundColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void WindowLayer::OnCreate()
{
	LN_CREATE_OPENGL_CONTEXT();
}

void WindowLayer::OnEvent(Event& event)
{
	EventDispatcher dispatcher(event);
	dispatcher.Bind<EventWindowClose>(LN_EVENT_BIND(WindowLayer::OnEventWindowClose));
}

bool WindowLayer::OnEventWindowClose(const EventWindowClose& event)
{
    Window::Close();
    return false;
}
