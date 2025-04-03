#include "WindowLayer.h"

using namespace Lion;

void WindowLayer::OnAttach()
{
	Window::SetSize(1280, 720);
	Window::SetTitle("Sandbox");
}

void WindowLayer::OnEvent(Event& event)
{
	EventDispatcher dispatcher(event);
	dispatcher.Bind<EventWindowClose>(LN_EVENT_BIND(WindowLayer::OnEventWindowClose));
}

bool WindowLayer::OnEventWindowClose(const EventWindowClose& event)
{
    Log::Console(ELogMode::Error, "[WINDOW] Closing...");
    Window::Close();
    return false;
}
