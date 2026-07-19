#pragma once

#include <Lion/Logic/Component.h>
#include <Lion/Math/Vector.h>

namespace Lion
{
	// The eye a running game looks through: attach it to an entity and the scene is framed on that
	// entity, at this component's zoom. It is what Godot's Camera2D is — a camera you place in the world
	// like anything else, so following a player is parenting it to the player and nothing more.
	//
	// One camera is the current one. Marking a second current takes the title from the first, because a
	// scene rendered through two cameras at once is a question with no answer.
	class Camera2D : public Component
	{
	public:
		LION_API Camera2D() = default;

		// How much world one screen pixel covers: 1 is one texel to one pixel, above it sees more, below
		// it sees less. Named the way the editor's viewport names it, so the two never mean opposites.
		LION_API float32 GetZoom() const { return mZoom; }
		LION_API void SetZoom(float32 zoom);

		// Whether the scene is rendered through this camera. Setting it takes the title from whichever
		// camera in the scene held it.
		LION_API bool IsCurrent() const { return mCurrent; }
		LION_API void SetCurrent(bool current);

		// Where the camera sits, in world space: its owner's position shifted by the offset, held inside
		// the limits when they are on. A camera parented to a player wants to look ahead of it, not
		// exactly at it — and to stop at the edge of the level rather than follow it into the void.
		LION_API glm::vec2 GetViewPosition() const;

		// The box the camera may not look past — Godot's limits, as a rectangle rather than four numbers.
		// It bounds what the camera *sees*, so the view stops with its edge on the limit, not its centre.
		LION_API bool HasLimits() const { return mLimited; }
		LION_API void SetLimited(bool limited) { mLimited = limited; }
		LION_API const Vector& GetLimitMinimum() const { return mLimitMinimum; }
		LION_API const Vector& GetLimitMaximum() const { return mLimitMaximum; }
		LION_API void SetLimits(const Vector& minimum, const Vector& maximum);

		// How much of the world the camera shows at its zoom, in world units. The editor draws this as
		// the framing rectangle, and the limits are applied against it.
		LION_API glm::vec2 GetViewSize() const;

		void OnAwake() override;
		void Reflect(Reflector& reflector) override;
		void Serialize(Serializer& serializer) const override;
		void Deserialize(const Serializer& serializer) override;

	private:
		float32 mZoom = 1.0f;
		Vector mOffset;
		bool mCurrent = true;

		bool mLimited = false;
		Vector mLimitMinimum{ -960.0f, -540.0f };
		Vector mLimitMaximum{ 960.0f, 540.0f };

		// Leaves this camera the only current one in its scene.
		void ClaimCurrent();
	};
}
