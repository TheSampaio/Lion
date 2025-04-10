#pragma once

namespace Lion
{
	class Event;

	class Layer
	{
	public:
		virtual ~Layer() = default;

		// Lifetime
		virtual LION_API void OnAttach() {};
		virtual LION_API void OnDetach() {};

		// Resource
		virtual LION_API void OnCreate() {}

		// Update
		virtual LION_API void OnUpdateBegin() {};
		virtual LION_API void OnUpdate() {};
		virtual LION_API void OnUpdateEnd() {};

		// OnRender
		virtual LION_API void OnRender() {}

		// OnEvent
		virtual LION_API void OnEvent(Event& event) {};
	};
}
