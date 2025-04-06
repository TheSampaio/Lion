#pragma once

namespace Lion
{
	class Event;

	class Layer
	{
	public:
		virtual LION_API void OnAttach() {};
		virtual LION_API void OnUpdate() {};
		virtual LION_API void OnRender() {};
		virtual LION_API void OnDetach() {};

		virtual void OnEvent(Event& event) {};
	};
}
