#pragma once

namespace Lion
{
	class Actor;
	class Entity;

	template<typename T>
	using Referenceable = std::enable_shared_from_this<T>;

	class Scene : public Referenceable<Scene>
	{
	public:
		LION_API Scene() = default;
		virtual LION_API ~Scene() = default;

		virtual LION_API void Add(Reference<Entity> entity);
		virtual LION_API void Remove(Reference<Entity> entity);

		LION_API void OnUpdate();
		LION_API void OnRender();

	private:
		std::list<Reference<Entity>> mEntities;

		bool Collision(Reference<Actor> a, Reference<Actor> b);
		void CollisionDetection();
	};
}
