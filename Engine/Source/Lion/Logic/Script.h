#pragma once

namespace Lion
{
	class Entity;
	class Transform;

	// Base class for native C++ gameplay scripts.
	//
	// Derive from Script, override the lifecycle hooks, and register the type with
	// LION_REGISTER_SCRIPT so it can be attached to an entity through a ScriptComponent (by name,
	// which keeps it serializable and selectable in the editor).
	class Script
	{
	public:
		virtual LION_API ~Script() = default;

		Script(const Script&) = delete;
		Script& operator=(const Script&) = delete;

		// Called once when the owning entity enters the scene.
		virtual void OnAwake() {}

		// Called every frame while the scene is simulating.
		virtual void OnUpdate() {}

		// Called when the script (or its owner) is destroyed.
		virtual void OnDestroy() {}

		// The entity this script is attached to, and a shortcut to its Transform.
		LION_API Entity& GetOwner() const { return *mOwner; }
		LION_API Reference<Transform> GetTransform() const;

		friend class ScriptComponent;

	protected:
		LION_API Script() = default;

	private:
		Entity* mOwner = nullptr;
	};
}
