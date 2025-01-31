#pragma once

#include "Event.h"

namespace Lion
{
    // Keyboard Press Event
    class EventInputKeyboardPress : public Event
    {
    public:
        EventInputKeyboardPress(int keyCode)
            : m_keyCode(keyCode)
        {
        }

        int GetKeyCode() const { return m_keyCode; }

        LN_EVENT_CLASS_TYPE(EventInputKeyboardPress);

    private:
        int m_keyCode;
    };

    // Keyboard Release Event
    class EventInputKeyboardRelease : public Event
    {
    public:
        EventInputKeyboardRelease(int keyCode)
            : m_keyCode(keyCode)
        {
        }

        int GetKeyCode() const { return m_keyCode; }

        LN_EVENT_CLASS_TYPE(EventInputKeyboardRelease);

    private:
        int m_keyCode;
    };

    // Keyboard Repeat Event (Key held down)
    class EventInputKeyboardRepeat : public Event
    {
    public:
        EventInputKeyboardRepeat(int keyCode)
            : m_keyCode(keyCode)
        {
        }

        int GetKeyCode() const { return m_keyCode; }

        LN_EVENT_CLASS_TYPE(EventInputKeyboardRepeat);

    private:
        int m_keyCode;
    };

    // Mouse Move Event
    class EventInputMouseMove : public Event
    {
    public:
        EventInputMouseMove(float x, float y)
            : m_positionX(x), m_positionY(y)
        {
        }

        float GetX() const { return m_positionX; }
        float GetY() const { return m_positionY; }

        LN_EVENT_CLASS_TYPE(EventInputMouseMove);

    private:
        float m_positionX, m_positionY;
    };

    // Mouse Press Event
    class EventInputMousePress : public Event
    {
    public:
        EventInputMousePress(int button)
            : m_button(button)
        {
        }

        int GetButton() const { return m_button; }

        LN_EVENT_CLASS_TYPE(EventInputMousePress);

    private:
        int m_button;
    };

    // Mouse Release Event
    class EventInputMouseRelease : public Event
    {
    public:
        EventInputMouseRelease(int button)
            : m_button(button)
        {
        }

        int GetButton() const { return m_button; }

        LN_EVENT_CLASS_TYPE(EventInputMouseRelease);

    private:
        int m_button;
    };

    // Mouse Scroll Event
    class EventInputMouseScroll : public Event
    {
    public:
        EventInputMouseScroll(float offsetX, float offsetY)
            : m_offsetX(offsetX), m_offsetY(offsetY)
        {
        }

        float GetOffsetX() const { return m_offsetX; }
        float GetOffsetY() const { return m_offsetY; }

        LN_EVENT_CLASS_TYPE(EventInputMouseScroll);

    private:
        float m_offsetX, m_offsetY;
    };
}
