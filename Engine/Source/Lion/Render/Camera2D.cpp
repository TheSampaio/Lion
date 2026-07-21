#include "Engine.h"
#include "Camera2D.h"

#include <Lion/Core/Clock.h>
#include <Lion/Core/Window.h>
#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Logic/Entity.h>
#include <Lion/Logic/Reflector.h>
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

	glm::vec2 Camera2D::GetViewSize() const
	{
		// The window is what the projection is built from, and the zoom is how much world each of its
		// pixels covers.
		const Size window = Window::GetSize();
		return glm::vec2(window.width * mZoom, window.height * mZoom);
	}

	glm::vec2 Camera2D::ClampToLimit(const glm::vec2& position) const
	{
		if (!mLimit)
			return position;

		// The limits bound what is *seen*, so the view stops with its edge against them — half a screen
		// short of the line, not centred on it. Held the right way round whichever way round they were
		// typed: a top below its bottom is a mistake, not an instruction.
		const glm::vec2 half = GetViewSize() * 0.5f;

		const float32 lowX = std::min(mLimitLeft, mLimitRight) + half.x;
		const float32 highX = std::max(mLimitLeft, mLimitRight) - half.x;
		const float32 lowY = std::min(mLimitBottom, mLimitTop) + half.y;
		const float32 highY = std::max(mLimitBottom, mLimitTop) - half.y;

		// A limit narrower than the view has no room to slide in, so the camera centres in it and shows
		// the overspill on both sides rather than jittering between two impossible edges.
		return glm::vec2(
			(lowX <= highX) ? std::clamp(position.x, lowX, highX) : (mLimitLeft + mLimitRight) * 0.5f,
			(lowY <= highY) ? std::clamp(position.y, lowY, highY) : (mLimitBottom + mLimitTop) * 0.5f);
	}

	glm::vec2 Camera2D::GetTargetPosition() const
	{
		const Vector position = GetOwner().GetWorldPosition();
		return ClampToLimit(glm::vec2(position.x + mOffset.x, position.y + mOffset.y));
	}

	glm::vec2 Camera2D::GetViewPosition() const
	{
		return (mSmooth && mSettled) ? mSmoothedPosition : GetTargetPosition();
	}

	float32 Camera2D::GetViewRotation() const
	{
		return (mSmooth && mSettled) ? mSmoothedRotation : GetOwner().GetWorldRotation();
	}

	void Camera2D::OnAwake()
	{
		// A camera starts framed on its target rather than flying in from the origin.
		mSmoothedPosition = GetTargetPosition();
		mSmoothedRotation = GetOwner().GetWorldRotation();
		mSettled = true;
	}

	void Camera2D::OnUpdate()
	{
		if (!mSmooth)
			return;

		const glm::vec2 target = GetTargetPosition();
		const float32 targetRotation = GetOwner().GetWorldRotation();

		if (!mSettled)
		{
			mSmoothedPosition = target;
			mSmoothedRotation = targetRotation;
			mSettled = true;
			return;
		}

		// An exponential ease: each second the camera closes the same *proportion* of the gap, so the
		// speed reads the same whatever the frame rate — a lerp against delta time does not.
		const float32 deltaTime = Clock::GetDeltaTime();

		const float32 positionBlend = 1.0f - std::exp(-mPositionSmoothing * deltaTime);
		const float32 rotationBlend = 1.0f - std::exp(-mRotationSmoothing * deltaTime);

		mSmoothedPosition += (target - mSmoothedPosition) * positionBlend;

		// Turning takes the shorter way round, so a camera at 350 degrees eases to 10 rather than back
		// through everything in between.
		float32 difference = std::fmod(targetRotation - mSmoothedRotation + 540.0f, 360.0f) - 180.0f;
		mSmoothedRotation += difference * rotationBlend;
	}

	void Camera2D::Reflect(Reflector& reflector)
	{
		// The editor draws this component itself (the limit's four sides collapse into one section), so
		// this is what a game reads and writes through code rather than what the Inspector shows.
		reflector.Field("Zoom", mZoom);
		reflector.Field("Offset", mOffset);
		reflector.Field("Limit", mLimit);
		reflector.Field("Smooth", mSmooth);
	}

	void Camera2D::Serialize(Serializer& serializer) const
	{
		serializer.Write("zoom", mZoom);
		serializer.Write("offsetX", mOffset.x);
		serializer.Write("offsetY", mOffset.y);

		serializer.Write("limit", mLimit);
		serializer.Write("limitTop", mLimitTop);
		serializer.Write("limitRight", mLimitRight);
		serializer.Write("limitBottom", mLimitBottom);
		serializer.Write("limitLeft", mLimitLeft);

		serializer.Write("smooth", mSmooth);
		serializer.Write("positionSmoothing", mPositionSmoothing);
		serializer.Write("rotationSmoothing", mRotationSmoothing);
	}

	void Camera2D::Deserialize(const Serializer& serializer)
	{
		mZoom = serializer.ReadFloat("zoom", 1.0f);
		mOffset = Vector(serializer.ReadFloat("offsetX", 0.0f), serializer.ReadFloat("offsetY", 0.0f));

		mLimit = serializer.ReadBool("limit", false);
		mLimitTop = serializer.ReadFloat("limitTop", 540.0f);
		mLimitRight = serializer.ReadFloat("limitRight", 960.0f);
		mLimitBottom = serializer.ReadFloat("limitBottom", -540.0f);
		mLimitLeft = serializer.ReadFloat("limitLeft", -960.0f);

		mSmooth = serializer.ReadBool("smooth", false);
		mPositionSmoothing = serializer.ReadFloat("positionSmoothing", 5.0f);
		mRotationSmoothing = serializer.ReadFloat("rotationSmoothing", 5.0f);

		mSettled = false;
	}

	LION_REGISTER_COMPONENT(Camera2D)
}
