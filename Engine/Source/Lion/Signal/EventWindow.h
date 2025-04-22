#pragma once

#include <Lion/Signal/Event.h>

namespace Lion
{
    // Window Close Event
    class EventWindowClose : public Event
    {
    public:
        EventWindowClose() = default;
        LION_EVENT_TYPE(EventWindowClose)
    };

    // Window Enter Focus Event
    class EventWindowFocusEnter : public Event
    {
    public:
        EventWindowFocusEnter() = default;
        LION_EVENT_TYPE(EventWindowFocusEnter)
    };

    // Window Exit Focus Event
    class EventWindowFocusExit : public Event
    {
    public:
        EventWindowFocusExit() = default;
        LION_EVENT_TYPE(EventWindowFocusExit)
    };

    // Window Resize Event
    class EventWindowResize : public Event
    {
    public:
        EventWindowResize(int32 width, int32 height)
            : mWidth(width), mHeight(height)
        {
        }

        int32 GetWidth() const { return mWidth; }
        int32 GetHeight() const { return mHeight; }

        LION_EVENT_TYPE(EventWindowResize)

    private:
        int32 mWidth, mHeight;
    };
}
