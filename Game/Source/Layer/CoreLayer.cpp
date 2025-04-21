#include "CoreLayer.h"

using namespace Lion;

void CoreLayer::OnAttach()
{
	Window::SetSize(800, 600);
	Window::SetTitle("Brickout");
	Window::SetBackgroundColor(0.05f, 0.05f, 0.05f);
	Window::SetIcon("Resource/Sprite/Brickout/tile-3.png");

	Graphics::SetVerticalSynchronization(false);
}

void CoreLayer::OnEvent(Event& event)
{
	EventDispatcher dispatcher(event);
	dispatcher.Bind<EventWindowClose>(LN_EVENT_BIND(CoreLayer::OnEventWindowClose));
}

bool CoreLayer::OnEventWindowClose(const EventWindowClose& event)
{
    Window::Close();
    return false;
}
