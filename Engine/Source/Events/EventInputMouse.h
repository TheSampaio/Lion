#pragma once

#include "Event.h"

namespace Lion
{
	class EventInputMouseMove : public Event
	{
	public:
		EventInputMouseMove(float x, float y)
			: m_positionX(x), m_positionY(y) {
		}

		float GetX() const { return m_positionX; }
		float GetY() const { return m_positionY; }

		LN_EVENT_CLASS_TYPE(EventInputMouseMove)

	private:
		float m_positionX, m_positionY;
	};
}
