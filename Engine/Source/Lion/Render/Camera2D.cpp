#include "Engine.h"
#include "Camera2D.h"

#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Logic/Entity.h>
#include <Lion/Logic/Reflector.h>
#include <Lion/Logic/Scene.h>
#include <Lion/Logic/Serializer.h>
#include <Lion/Core/Window.h>

namespace Lion
{
	namespace
	{
		// A zoom of zero is a view of nothing, and a negative one turns the world inside out.
		constexpr float32 kMinimumZoom = 0.01f;
		constexpr float32 kMaximumZoom = 100.0f;
	}

	void Camera2D::SetZoom(float32 zoom)
	{
		mZoom = std::clamp(zoom, kMinimumZoom, kMaximumZoom);
	}

	void Camera2D::SetCurrent(bool current)
	{
		mCurrent = current;

		if (current)
			ClaimCurrent();
	}

	void Camera2D::SetLimits(const Vector& minimum, const Vector& maximum)
	{
		// Held the right way round, whichever way round it was given: a limit box with its corners
		// crossed is a box with no inside.
		mLimitMinimum = Vector(std::min(minimum.x, maximum.x), std::min(minimum.y, maximum.y));
		mLimitMaximum = Vector(std::max(minimum.x, maximum.x), std::max(minimum.y, maximum.y));
	}

	glm::vec2 Camera2D::GetViewSize() const
	{
		// The window is what the projection is built from, and the zoom is how much world each of its
		// pixels covers.
		const Size window = Window::GetSize();
		return glm::vec2(window.width * mZoom, window.height * mZoom);
	}

	glm::vec2 Camera2D::GetViewPosition() const
	{
		const Vector position = GetOwner().GetWorldPosition();
		glm::vec2 view(position.x + mOffset.x, position.y + mOffset.y);

		if (!mLimited)
			return view;

		// The limits bound what is *seen*, so the view stops with its edge against them — half a screen
		// short of the line, not centred on it. A limit narrower than the view has no room to slide in,
		// so the camera sits in the middle of it and shows the overspill on both sides.
		const glm::vec2 half = GetViewSize() * 0.5f;
		const glm::vec2 minimum(mLimitMinimum.x + half.x, mLimitMinimum.y + half.y);
		const glm::vec2 maximum(mLimitMaximum.x - half.x, mLimitMaximum.y - half.y);

		view.x = (minimum.x <= maximum.x) ? std::clamp(view.x, minimum.x, maximum.x)
			: (mLimitMinimum.x + mLimitMaximum.x) * 0.5f;
		view.y = (minimum.y <= maximum.y) ? std::clamp(view.y, minimum.y, maximum.y)
			: (mLimitMinimum.y + mLimitMaximum.y) * 0.5f;

		return view;
	}

	void Camera2D::OnAwake()
	{
		// A scene woken with two cameras claiming the title settles it here, in the order they wake: the
		// last one to say it is current is the one that is.
		if (mCurrent)
			ClaimCurrent();
	}

	void Camera2D::Reflect(Reflector& reflector)
	{
		reflector.Field("Zoom", mZoom);
		reflector.Field("Offset", mOffset);
		reflector.Field("Current", mCurrent);
		reflector.Field("Limited", mLimited);
		reflector.Field("Limit Min", mLimitMinimum);
		reflector.Field("Limit Max", mLimitMaximum);
	}

	void Camera2D::Serialize(Serializer& serializer) const
	{
		serializer.Write("zoom", mZoom);
		serializer.Write("offsetX", mOffset.x);
		serializer.Write("offsetY", mOffset.y);
		serializer.Write("current", mCurrent);
		serializer.Write("limited", mLimited);
		serializer.Write("limitMinX", mLimitMinimum.x);
		serializer.Write("limitMinY", mLimitMinimum.y);
		serializer.Write("limitMaxX", mLimitMaximum.x);
		serializer.Write("limitMaxY", mLimitMaximum.y);
	}

	void Camera2D::Deserialize(const Serializer& serializer)
	{
		mZoom = serializer.ReadFloat("zoom", 1.0f);
		mOffset = Vector(serializer.ReadFloat("offsetX", 0.0f), serializer.ReadFloat("offsetY", 0.0f));
		mCurrent = serializer.ReadBool("current", true);
		mLimited = serializer.ReadBool("limited", false);
		mLimitMinimum = Vector(serializer.ReadFloat("limitMinX", -960.0f), serializer.ReadFloat("limitMinY", -540.0f));
		mLimitMaximum = Vector(serializer.ReadFloat("limitMaxX", 960.0f), serializer.ReadFloat("limitMaxY", 540.0f));
	}

	void Camera2D::ClaimCurrent()
	{
		const Reference<Scene> scene = GetOwner().GetScene();

		if (!scene)
			return;

		for (const auto& entity : scene->GetEntities())
		{
			Camera2D* other = entity->GetComponent<Camera2D>();

			if (other && other != this)
				other->mCurrent = false;
		}
	}

	LION_REGISTER_COMPONENT(Camera2D)
}
