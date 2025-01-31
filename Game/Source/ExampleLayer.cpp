#include "ExampleLayer.h"

using namespace Lion;

void ExampleLayer::OnEvent(Lion::Event& event)
{
	// Bind the member methods to engine events
	EventDispatcher dispatcher(event);

    // Keyboard
	dispatcher.Bind<Lion::EventInputKeyboardPress>(LN_EVENT_BIND(ExampleLayer::OnEventInputKeyboardPress));
	dispatcher.Bind<Lion::EventInputKeyboardRelease>(LN_EVENT_BIND(ExampleLayer::OnEventInputKeyboardRelease));
	dispatcher.Bind<Lion::EventInputKeyboardRepeat>(LN_EVENT_BIND(ExampleLayer::OnEventInputKeyboardRepeat));

    // Mouse
	dispatcher.Bind<Lion::EventInputMouseMove>(LN_EVENT_BIND(ExampleLayer::OnEventInputMouseMove));
	dispatcher.Bind<Lion::EventInputMousePress>(LN_EVENT_BIND(ExampleLayer::OnEventInputMousePress));
	dispatcher.Bind<Lion::EventInputMouseRelease>(LN_EVENT_BIND(ExampleLayer::OnEventInputMouseRelease));
	dispatcher.Bind<Lion::EventInputMouseScroll>(LN_EVENT_BIND(ExampleLayer::OnEventInputMouseScroll));

    // Window
	dispatcher.Bind<Lion::EventWindowFocusEnter>(LN_EVENT_BIND(ExampleLayer::OnEventWindowFocusEnter));
	dispatcher.Bind<Lion::EventWindowFocusExit>(LN_EVENT_BIND(ExampleLayer::OnEventWindowFocusExit));
	dispatcher.Bind<Lion::EventWindowResize>(LN_EVENT_BIND(ExampleLayer::OnEventWindowResize));
}

bool ExampleLayer::OnEventInputKeyboardPress(const Lion::EventInputKeyboardPress& event)
{
    Log::Console(ELogMode::Success, LN_LOG_FORMAT("[KEYBOARD] Key Pressed: {}.", event.GetKeyCode()));
    return false;
}

bool ExampleLayer::OnEventInputKeyboardRelease(const Lion::EventInputKeyboardRelease& event)
{
    Log::Console(ELogMode::Information, LN_LOG_FORMAT("[KEYBOARD] Key Released: {}.", event.GetKeyCode()));
    return false;
}

bool ExampleLayer::OnEventInputKeyboardRepeat(const Lion::EventInputKeyboardRepeat& event)
{
    Log::Console(ELogMode::Warning, LN_LOG_FORMAT("[KEYBOARD] Key Repeated: {}.", event.GetKeyCode()));
    return false;
}

bool ExampleLayer::OnEventInputMouseMove(const Lion::EventInputMouseMove& event)
{
    Log::Console(ELogMode::Trace, LN_LOG_FORMAT("[MOUSE] Moved To: {}x{}.", event.GetX(), event.GetY()));
    return false;
}

bool ExampleLayer::OnEventInputMousePress(const Lion::EventInputMousePress& event)
{
    Log::Console(ELogMode::Success, LN_LOG_FORMAT("[MOUSE] Button Pressed: {}.", event.GetButton()));
    return false;
}

bool ExampleLayer::OnEventInputMouseRelease(const Lion::EventInputMouseRelease& event)
{
    Log::Console(ELogMode::Information, LN_LOG_FORMAT("[MOUSE] Button Released: {}.", event.GetButton()));
    return false;
}

bool ExampleLayer::OnEventInputMouseScroll(const Lion::EventInputMouseScroll& event)
{
    Log::Console(ELogMode::Warning, LN_LOG_FORMAT("[MOUSE] Scrolled: OffsetX={} OffsetY={}.", event.GetOffsetX(), event.GetOffsetY()));
    return false;
}

bool ExampleLayer::OnEventWindowFocusEnter(const Lion::EventWindowFocusEnter& event)
{
    Log::Console(ELogMode::Warning, "[WINDOW] Focus Entered.");
    return false;
}

bool ExampleLayer::OnEventWindowFocusExit(const Lion::EventWindowFocusExit& event)
{
    Log::Console(ELogMode::Warning, "[WINDOW] Focus Exited.");
    return false;
}

bool ExampleLayer::OnEventWindowResize(const Lion::EventWindowResize& event)
{
    Log::Console(ELogMode::Information, LN_LOG_FORMAT("[WINDOW] Resized: {}x{}.", event.GetWidth(), event.GetHeight()));
    return false;
}
