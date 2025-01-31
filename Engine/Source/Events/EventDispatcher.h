#pragma once

#include "Event.h"

#define LN_EVENT_BIND(callable) std::bind(&callable, this, std::placeholders::_1)

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
            {
                callback(event);
            }
        }

        // Bind for normal functions
        template<typename T, typename Func>
        bool Bind(const Func& func)
        {
            if (m_Event.GetEventType() == T::GetStaticType())
            {
                // Se a função retornar void
                if constexpr (std::is_void_v<std::invoke_result_t<Func, T&>>)
                {
                    func(static_cast<T&>(m_Event));
                }
                else // Se a função retornar bool
                {
                    m_Event.Handled = func(static_cast<T&>(m_Event));
                }
                return true;
            }
            return false;
        }

    private:
        Event& m_Event;

        // Usado para obter callbacks de um tipo de evento específico
        template<typename EventType>
        std::vector<std::function<void(const EventType&)>>& GetEventCallbacks()
        {
            static std::vector<std::function<void(const EventType&)>> eventCallbacks;
            return eventCallbacks;
        }
    };
}
