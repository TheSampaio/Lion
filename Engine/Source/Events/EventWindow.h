#pragma once

#include "Event.h"

namespace Lion
{
    // Window Close Event
    class EventWindowClose : public Event
    {
    public:
        EventWindowClose() = default;
        LN_EVENT_CLASS_TYPE(EventWindowClose)
    };

    // Window Enter Focus Event
    class EventWindowFocusEnter : public Event
    {
    public:
        EventWindowFocusEnter() = default;
        LN_EVENT_CLASS_TYPE(EventWindowFocusEnter)
    };

    // Window Exit Focus Event
    class EventWindowFocusExit : public Event
    {
    public:
        EventWindowFocusExit() = default;
        LN_EVENT_CLASS_TYPE(EventWindowFocusExit)
    };

    // Window Resize Event
    class EventWindowResize : public Event
    {
    public:
        EventWindowResize(int width, int height)
            : mWidth(width), mHeight(height)
        {
        }

        int GetWidth() const { return mWidth; }
        int GetHeight() const { return mHeight; }

        LN_EVENT_CLASS_TYPE(EventWindowResize)

    private:
        int mWidth, mHeight;
    };
}
