#include "ExampleLayer.h"

using namespace Lion;

void ExampleLayer::OnEvent(Event& event)
{
	// Bind the member methods to engine events
	EventDispatcher dispatcher(event);

    // Keyboard
	dispatcher.Bind<EventInputKeyboardPress>(LN_EVENT_BIND(ExampleLayer::OnEventInputKeyboardPress));
	dispatcher.Bind<EventInputKeyboardRelease>(LN_EVENT_BIND(ExampleLayer::OnEventInputKeyboardRelease));
	dispatcher.Bind<EventInputKeyboardRepeat>(LN_EVENT_BIND(ExampleLayer::OnEventInputKeyboardRepeat));

    // Mouse
	dispatcher.Bind<EventInputMouseMove>(LN_EVENT_BIND(ExampleLayer::OnEventInputMouseMove));
	dispatcher.Bind<EventInputMousePress>(LN_EVENT_BIND(ExampleLayer::OnEventInputMousePress));
	dispatcher.Bind<EventInputMouseRelease>(LN_EVENT_BIND(ExampleLayer::OnEventInputMouseRelease));
	dispatcher.Bind<EventInputMouseScroll>(LN_EVENT_BIND(ExampleLayer::OnEventInputMouseScroll));

    // Window
	dispatcher.Bind<EventWindowFocusEnter>(LN_EVENT_BIND(ExampleLayer::OnEventWindowFocusEnter));
	dispatcher.Bind<EventWindowFocusExit>(LN_EVENT_BIND(ExampleLayer::OnEventWindowFocusExit));
	dispatcher.Bind<EventWindowResize>(LN_EVENT_BIND(ExampleLayer::OnEventWindowResize));
}

bool ExampleLayer::OnEventInputKeyboardPress(const EventInputKeyboardPress& event)
{
    Log::Console(ELogMode::Success, LN_LOG_FORMAT("[KEYBOARD] Key Pressed: {}.", event.GetKeyCode()));
    return false;
}

bool ExampleLayer::OnEventInputKeyboardRelease(const EventInputKeyboardRelease& event)
{
    Log::Console(ELogMode::Information, LN_LOG_FORMAT("[KEYBOARD] Key Released: {}.", event.GetKeyCode()));
    return false;
}

bool ExampleLayer::OnEventInputKeyboardRepeat(const EventInputKeyboardRepeat& event)
{
    Log::Console(ELogMode::Warning, LN_LOG_FORMAT("[KEYBOARD] Key Repeated: {}.", event.GetKeyCode()));
    return false;
}

bool ExampleLayer::OnEventInputMouseMove(const EventInputMouseMove& event)
{
    Log::Console(ELogMode::Trace, LN_LOG_FORMAT("[MOUSE] Moved To: {}x{}.", event.GetX(), event.GetY()));
    return false;
}

bool ExampleLayer::OnEventInputMousePress(const EventInputMousePress& event)
{
    Log::Console(ELogMode::Success, LN_LOG_FORMAT("[MOUSE] Button Pressed: {}.", event.GetButton()));
    return false;
}

bool ExampleLayer::OnEventInputMouseRelease(const EventInputMouseRelease& event)
{
    Log::Console(ELogMode::Information, LN_LOG_FORMAT("[MOUSE] Button Released: {}.", event.GetButton()));
    return false;
}

bool ExampleLayer::OnEventInputMouseScroll(const EventInputMouseScroll& event)
{
    Log::Console(ELogMode::Warning, LN_LOG_FORMAT("[MOUSE] Scrolled: OffsetX={} OffsetY={}.", event.GetOffsetX(), event.GetOffsetY()));
    return false;
}

bool ExampleLayer::OnEventWindowFocusEnter(const EventWindowFocusEnter& event)
{
    Log::Console(ELogMode::Warning, "[WINDOW] Focus Entered.");
    return false;
}

bool ExampleLayer::OnEventWindowFocusExit(const EventWindowFocusExit& event)
{
    Log::Console(ELogMode::Warning, "[WINDOW] Focus Exited.");
    return false;
}

bool ExampleLayer::OnEventWindowResize(const EventWindowResize& event)
{
    Log::Console(ELogMode::Information, LN_LOG_FORMAT("[WINDOW] Resized: {}x{}.", event.GetWidth(), event.GetHeight()));
    return false;
}
