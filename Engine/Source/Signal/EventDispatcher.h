#pragma once

#include "Event.h"

namespace Lion
{
    class EventDispatcher
    {
    public:
        EventDispatcher(Event& event)
            : m_Event(event)
        {
        }

        // Bind for lambda functions
        template<typename EventType>
        void Bind(const EventType& event)
        {
            for (auto& callback : GetEventCallbacks<EventType>())
                callback(event);
        }

        // Bind for normal functions
        template<typename T, typename Func>
        bool Bind(const Func& func)
        {
            if (m_Event.GetEventType() == T::GetStaticType())
            {
                if constexpr (std::is_void_v<std::invoke_result_t<Func, T&>>)
                    func(static_cast<T&>(m_Event));

                else
                    m_Event.Handled = func(static_cast<T&>(m_Event));

                return true;
            }

            return false;
        }

    private:
        Event& m_Event;

        template<typename EventType>
        std::vector<std::function<void(const EventType&)>>& GetEventCallbacks()
        {
            static std::vector<std::function<void(const EventType&)>> eventCallbacks;
            return eventCallbacks;
        }
    };
}
