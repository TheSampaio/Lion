#include "Engine.h"
#include "WidgetAnchor.h"

#include <Lion/Core/Window.h>
#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Logic/Entity.h>
#include <Lion/Logic/Reflector.h>

namespace Lion
{
	void WidgetAnchor::OnAwake()
	{
		ApplyLayout();
	}

	void WidgetAnchor::OnUpdateBegin()
	{
		ApplyLayout();
	}

	void WidgetAnchor::Reflect(Reflector& reflector)
	{
		reflector.Field("Anchor", mAnchor);
		reflector.Field("Offset", mOffset);
	}

	void WidgetAnchor::ApplyLayout()
	{
		const float32 width = static_cast<float32>(Window::kDefaultViewportWidth);
		const float32 height = static_cast<float32>(Window::kDefaultViewportHeight);
		GetOwner().SetWorldPosition(Vector2(
			(std::clamp(mAnchor.x, 0.0f, 1.0f) - 0.5f) * width + mOffset.x,
			(std::clamp(mAnchor.y, 0.0f, 1.0f) - 0.5f) * height + mOffset.y));
	}

	LION_REGISTER_COMPONENT(WidgetAnchor)
}
