#include "WindowLayer.h"

using namespace Lion;

void WindowLayer::OnAttach()
{
	Window::SetSize(1280, 720);
	Window::SetTitle("Sandbox");
	Window::SetBackgroundColor(0.2f, 0.2f, 0.2f, 1.0f);
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
