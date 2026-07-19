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

		// Where the camera sits, in world space: its owner's position shifted by the offset. A camera
		// parented to a player wants to look ahead of it, not exactly at it.
		LION_API glm::vec2 GetViewPosition() const;

		void OnAwake() override;
		void Reflect(Reflector& reflector) override;
		void Serialize(Serializer& serializer) const override;
		void Deserialize(const Serializer& serializer) override;

	private:
		float32 mZoom = 1.0f;
		Vector mOffset;
		bool mCurrent = true;

		// Leaves this camera the only current one in its scene.
		void ClaimCurrent();
	};
}
