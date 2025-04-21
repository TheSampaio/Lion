#pragma once

namespace Lion
{
	class Event;

	class Layer
	{
	public:
		virtual ~Layer() = default;

		// Lifetime
		virtual void OnAttach() {};
		virtual void OnDetach() {};

		// Resource
		virtual void OnCreate() {}

		// Update
		virtual void OnUpdateBegin() {};
		virtual void OnUpdate() {};
		virtual void OnUpdateEnd() {};

		// OnRender
		virtual void OnRender() {}

		// OnEvent
		virtual void OnEvent(Event& event) {};
	};
}
