#pragma once

#define LN_EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return EventType::type; }\
                                  virtual EventType GetEventType() const override { return GetStaticType(); }

namespace Lion
{
	enum class EventType
	{
		None = 0,
		EventInputKeyPress, EventInputKeyRelease, EventInputKeyRepeat,
		EventInputMousePress, EventInputMouseRelease, EventInputMouseMove, EventInputMouseScroll,
		EventWindowClose, EventWindowFocusEnter, EventWindowFocusExit, EventWindowResize
	};

    // Base class for all events
    class Event
    {
    public:
        bool Handled = false;

        virtual ~Event() = default;
        virtual EventType GetEventType() const = 0;
    };
}