#pragma once

#include <Lion/Signal/Event.h>

namespace Lion
{
    // Keyboard Press Event
    class EventInputKeyboardPress : public Event
    {
    public:
        EventInputKeyboardPress(int32 keyCode)
            : m_keyCode(keyCode)
        {
        }

        int32 GetKeyCode() const { return m_keyCode; }

        LN_EVENT_CLASS_TYPE(EventInputKeyboardPress);

    private:
        int32 m_keyCode;
    };

    // Keyboard Release Event
    class EventInputKeyboardRelease : public Event
    {
    public:
        EventInputKeyboardRelease(int32 keyCode)
            : m_keyCode(keyCode)
        {
        }

        int32 GetKeyCode() const { return m_keyCode; }

        LN_EVENT_CLASS_TYPE(EventInputKeyboardRelease);

    private:
        int32 m_keyCode;
    };

    // Keyboard Repeat Event (Key held down)
    class EventInputKeyboardRepeat : public Event
    {
    public:
        EventInputKeyboardRepeat(int32 keyCode)
            : m_keyCode(keyCode)
        {
        }

        int32 GetKeyCode() const { return m_keyCode; }

        LN_EVENT_CLASS_TYPE(EventInputKeyboardRepeat);

    private:
        int32 m_keyCode;
    };

    // Mouse Move Event
    class EventInputMouseMove : public Event
    {
    public:
        EventInputMouseMove(float32 x, float32 y)
            : m_positionX(x), m_positionY(y)
        {
        }

        float32 GetX() const { return m_positionX; }
        float32 GetY() const { return m_positionY; }

        LN_EVENT_CLASS_TYPE(EventInputMouseMove);

    private:
        float32 m_positionX, m_positionY;
    };

    // Mouse Press Event
    class EventInputMousePress : public Event
    {
    public:
        EventInputMousePress(int32 button)
            : m_button(button)
        {
        }

        int32 GetButton() const { return m_button; }

        LN_EVENT_CLASS_TYPE(EventInputMousePress);

    private:
        int32 m_button;
    };

    // Mouse Release Event
    class EventInputMouseRelease : public Event
    {
    public:
        EventInputMouseRelease(int32 button)
            : m_button(button)
        {
        }

        int32 GetButton() const { return m_button; }

        LN_EVENT_CLASS_TYPE(EventInputMouseRelease);

    private:
        int32 m_button;
    };

    // Mouse Scroll Event
    class EventInputMouseScroll : public Event
    {
    public:
        EventInputMouseScroll(float32 offsetX, float32 offsetY)
            : m_offsetX(offsetX), m_offsetY(offsetY)
        {
        }

        float32 GetOffsetX() const { return m_offsetX; }
        float32 GetOffsetY() const { return m_offsetY; }

        LN_EVENT_CLASS_TYPE(EventInputMouseScroll);

    private:
        float32 m_offsetX, m_offsetY;
    };
}
