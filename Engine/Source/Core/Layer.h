#pragma once

namespace Lion
{
	class Event;

	class LION_API Layer
	{
	public:
		virtual void OnAttach() {};
		virtual void OnUpdate() {};
		virtual void OnRender() {};
		virtual void OnDetach() {};

		virtual void OnEvent(Event& event) {};
	};
}
