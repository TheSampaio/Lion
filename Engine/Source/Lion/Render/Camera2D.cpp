#include "Engine.h"
#include "Camera2D.h"

#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Logic/Entity.h>
#include <Lion/Logic/Reflector.h>
#include <Lion/Logic/Scene.h>
#include <Lion/Logic/Serializer.h>

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

	glm::vec2 Camera2D::GetViewPosition() const
	{
		const Vector position = GetOwner().GetWorldPosition();
		return glm::vec2(position.x + mOffset.x, position.y + mOffset.y);
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
	}

	void Camera2D::Serialize(Serializer& serializer) const
	{
		serializer.Write("zoom", mZoom);
		serializer.Write("offsetX", mOffset.x);
		serializer.Write("offsetY", mOffset.y);
		serializer.Write("current", mCurrent);
	}

	void Camera2D::Deserialize(const Serializer& serializer)
	{
		mZoom = serializer.ReadFloat("zoom", 1.0f);
		mOffset = Vector(serializer.ReadFloat("offsetX", 0.0f), serializer.ReadFloat("offsetY", 0.0f));
		mCurrent = serializer.ReadBool("current", true);
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
