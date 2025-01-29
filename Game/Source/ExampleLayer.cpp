#include "ExampleLayer.h"

using namespace Lion;

void ExampleLayer::OnAttach()
{
	Log::Console(ELogMode::Trace, "This is a 'Trace' message.");
	Log::Console(ELogMode::Success, "This is a 'Success' message.");
	Log::Console(ELogMode::Information, "This is an 'Information' message.");
	Log::Console(ELogMode::Warning, "This is a 'Warning' message.");
	Log::Console(ELogMode::Error, "This is an 'Error' message.");
}

void ExampleLayer::OnEvent(Event& event)
{
	EventDispatcher dispatcher(event);

	// Bind the member methods to engine events
	dispatcher.Bind<Lion::EventInputMouseMove>(LN_EVENT_BIND(ExampleLayer::OnEventInputMouseMove));
}

bool ExampleLayer::OnEventInputMouseMove(const EventInputMouseMove& event)
{
	Log::Console(ELogMode::Trace, LN_LOG_FORMAT("Mouse postion: {}x{}.", event.GetX(), event.GetY()));
	return true;
}
