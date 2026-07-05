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

		LION_API void OnUpdate();
		LION_API void OnRender();

		// Physics world owned by this scene (never null).
		LION_API PhysicsWorld* GetPhysicsWorld() const { return mPhysicsWorld.get(); }

		// Sets the world gravity, in meters per second squared.
		LION_API void SetGravity(const glm::vec2& gravity);

	private:
		std::list<Reference<Entity>> mEntities;
		std::vector<Reference<Entity>> mPendingRemoval;
		Scope<PhysicsWorld> mPhysicsWorld;

		void FlushPendingRemoval();
	};
}
