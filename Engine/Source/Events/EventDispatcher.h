#pragma once

#include "Event.h"

#define LN_EVENT_BIND(callable) std::bind(&callable, this, std::placeholders::_1)

namespace Lion
{
    class EventDispatcher
    {
    public:
        // Constructor that accepts an event
        EventDispatcher(Event& event)
            : m_Event(event)
        {
        }

        template<typename EventType>
        void Bind(std::function<void(const EventType&)> callback)
        {
            // Store the callback for this event
            GetEventCallbacks<EventType>().emplace_back(callback);
        }

        template<typename EventType>
        void Dispatch(const EventType& event)
        {
            // Call all callbacks for the event
            for (auto& callback : GetEventCallbacks<EventType>())
            {
                callback(event);
            }
        }

    private:
        Event& m_Event; // The event we're dispatching callbacks for

        template<typename EventType>
        std::vector<std::function<void(const EventType&)>>& GetEventCallbacks()
        {
            static std::vector<std::function<void(const EventType&)>> eventCallbacks;
            return eventCallbacks;
        }
    };
}
