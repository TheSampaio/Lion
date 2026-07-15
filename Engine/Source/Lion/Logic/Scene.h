#pragma once

namespace Lion
{
	class Entity;
	class PhysicsWorld;

	template<typename T>
	using Referenceable = std::enable_shared_from_this<T>;

	// Container and driver for a set of entities and their physics simulation.
	//
	// The scene owns a PhysicsWorld, updates entities and their components each frame, steps the
	// simulation and dispatches collisions. Entity removal is deferred to the end of the frame so
	// it is safe to call from within update or collision callbacks.
	class Scene : public Referenceable<Scene>
	{
	public:
		LION_API Scene();
		virtual LION_API ~Scene();

		virtual LION_API void Add(Reference<Entity> entity);
		virtual LION_API void Remove(Reference<Entity> entity);

		// Removes an entity by raw pointer (used for self-removal); deferred to end of frame.
		LION_API void Remove(Entity* entity);

		// Advances the scene by 'deltaTime' seconds. A negative value (the default) means "however long the
		// last frame took" — the running game. A step passes a whole fixed frame instead, so one step is one
		// visible frame and not a slice of real time too small for the fixed-timestep physics to act on.
		LION_API void OnUpdate(float32 deltaTime = -1.0f);
		LION_API void OnRender();

		// Physics world owned by this scene (never null).
		LION_API PhysicsWorld* GetPhysicsWorld() const { return mPhysicsWorld.get(); }

		// Sets the world gravity, in meters per second squared.
		LION_API void SetGravity(const glm::vec2& gravity);

		// Returns the world gravity, in meters per second squared.
		LION_API glm::vec2 GetGravity() const;

		// Entities currently in the scene (used by serialization and editor panels).
		const std::list<Reference<Entity>>& GetEntities() const { return mEntities; }

		// Removes every entity from the scene (used by New/Open in the editor).
		LION_API void Clear();

		// Immediately processes pending removals (for tools that don't run the update loop).
		LION_API void FlushRemovals();

	private:
		std::list<Reference<Entity>> mEntities;
		std::vector<Reference<Entity>> mPendingRemoval;
		Scope<PhysicsWorld> mPhysicsWorld;

		void FlushPendingRemoval();
	};
}
