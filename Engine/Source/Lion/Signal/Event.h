#pragma once

#define LN_EVENT_BIND(callable) std::bind(&callable, this, std::placeholders::_1)
#define LN_EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return EventType::type; }\
                                  virtual EventType GetEventType() const override { return GetStaticType(); }\
                                  virtual const char* GetName() const override { return #type; }

namespace Lion
{
    enum class EventType
    {
        None = 0,
        EventInputKeyboardPress, EventInputKeyboardRelease, EventInputKeyboardRepeat,
        EventInputMouseMove, EventInputMousePress, EventInputMouseRelease, EventInputMouseScroll,
        EventWindowClose, EventWindowFocusEnter, EventWindowFocusExit, EventWindowResize
    };

    class Event
    {
    public:
        bool Handled = false;

        virtual ~Event() = default;
        virtual const char* GetName() const = 0;
        virtual EventType GetEventType() const = 0;
        virtual std::string ToString() const { return GetName(); }
    };
}