#pragma once

#include <Lion/Logic/Component.h>
#include <Lion/Math/Vector2.h>

namespace Lion
{
	// The eye a running game looks through: attach it to an entity and the scene is framed on that
	// entity, at this component's zoom. It is what Godot's Camera2D is — a camera you place in the world
	// like anything else, so following a player is parenting it to the player and nothing more.
	//
	// The scene renders through the first enabled camera it holds. There is no flag to elect one: a
	// camera you do not want looking is a camera you disable, which is the switch every other component
	// already has.
	class Camera2D : public Component
	{
	public:
		LION_API Camera2D() = default;

		// How much world one screen pixel covers: 1 is one texel to one pixel, above it sees more, below
		// it sees less. Named the way the editor's viewport names it, so the two never mean opposites.
		LION_API float32 GetZoom() const { return mZoom; }
		LION_API void SetZoom(float32 zoom);

		// Where the camera looks, relative to its owner. A camera parented to a player wants to look
		// ahead of it, not exactly at it.
		LION_API const Vector2& GetOffset() const { return mOffset; }
		LION_API void SetOffset(const Vector2& offset) { mOffset = offset; }

		// The edges the camera may not look past, each its own number — the four sides of a level, which
		// is how a level is measured. They bound what the camera *sees*, so the view stops with its edge
		// on the limit rather than its centre.
		LION_API bool HasLimit() const { return mLimit; }
		LION_API void SetLimit(bool limit) { mLimit = limit; }

		LION_API float32 GetLimitTop() const { return mLimitTop; }
		LION_API float32 GetLimitRight() const { return mLimitRight; }
		LION_API float32 GetLimitBottom() const { return mLimitBottom; }
		LION_API float32 GetLimitLeft() const { return mLimitLeft; }

		LION_API void SetLimitTop(float32 value) { mLimitTop = value; }
		LION_API void SetLimitRight(float32 value) { mLimitRight = value; }
		LION_API void SetLimitBottom(float32 value) { mLimitBottom = value; }
		LION_API void SetLimitLeft(float32 value) { mLimitLeft = value; }

		// Catching up rather than snapping: the camera eases toward where it should be, at these speeds,
		// which is what keeps a following camera from transmitting every twitch of what it follows.
		LION_API bool HasSmoothing() const { return mSmooth; }
		LION_API void SetSmoothing(bool smooth) { mSmooth = smooth; }

		LION_API float32 GetPositionSmoothing() const { return mPositionSmoothing; }
		LION_API float32 GetRotationSmoothing() const { return mRotationSmoothing; }
		LION_API void SetPositionSmoothing(float32 speed) { mPositionSmoothing = std::max(speed, 0.0f); }
		LION_API void SetRotationSmoothing(float32 speed) { mRotationSmoothing = std::max(speed, 0.0f); }

		// Where the camera sits and how it is turned, after the offset, the limits and the smoothing.
		LION_API glm::vec2 GetViewPosition() const;
		LION_API float32 GetViewRotation() const;

		// How much of the world the camera shows at its zoom, in world units. The editor draws this as the
		// framing rectangle, and the limits are applied against it.
		LION_API glm::vec2 GetViewSize() const;

		// Where the camera would sit with no smoothing — what the smoothing is easing toward. The editor
		// draws this, since an editor is not running the game and has nothing to ease from.
		LION_API glm::vec2 GetTargetPosition() const;

		void OnAwake() override;
		void OnUpdate() override;
		void Reflect(Reflector& reflector) override;
		void Serialize(Serializer& serializer) const override;
		void Deserialize(const Serializer& serializer) override;

	private:
		float32 mZoom = 1.0f;
		Vector2 mOffset;

		bool mLimit = false;
		float32 mLimitTop = 540.0f;
		float32 mLimitRight = 960.0f;
		float32 mLimitBottom = -540.0f;
		float32 mLimitLeft = -960.0f;

		bool mSmooth = false;
		float32 mPositionSmoothing = 5.0f;
		float32 mRotationSmoothing = 5.0f;

		// Where the camera actually is while it eases toward the target, and whether it has been there
		// once — a camera should start framed, not fly in from the origin on the first frame.
		glm::vec2 mSmoothedPosition{ 0.0f, 0.0f };
		float32 mSmoothedRotation = 0.0f;
		bool mSettled = false;

		// The target held inside the limits, which is what both the smoothing and the editor's overlay
		// are measured against.
		glm::vec2 ClampToLimit(const glm::vec2& position) const;
	};
}
