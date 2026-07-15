#include "Engine.h"
#include "Scene.h"

#include <Lion/Core/Clock.h>
#include <Lion/Logic/Entity.h>
#include <Lion/Physics/PhysicsWorld.h>

namespace Lion
{
    Scene::Scene()
        : mPhysicsWorld(MakeScope<PhysicsWorld>())
    {
    }

    Scene::~Scene() = default;

    void Scene::Add(Reference<Entity> entity)
    {
        mEntities.push_back(entity);
        entity->mScene = shared_from_this();
        entity->Awake();
    }

    void Scene::Remove(Reference<Entity> entity)
    {
        if (!entity)
            return;

        // An entity takes its whole subtree with it.
        const std::vector<Entity*> children = entity->GetChildren();

        for (Entity* child : children)
            Remove(child);

        // Deferred: the actual removal happens at the end of the frame (see FlushPendingRemoval).
        mPendingRemoval.push_back(std::move(entity));
    }

    void Scene::Remove(Entity* entity)
    {
        // Keep the owning reference alive in the pending list until it is flushed.
        for (const auto& candidate : mEntities)
        {
            if (candidate.get() == entity)
            {
                Remove(candidate);
                return;
            }
        }
    }

    void Scene::SetGravity(const glm::vec2& gravity)
    {
        mPhysicsWorld->SetGravity(gravity);
    }

    glm::vec2 Scene::GetGravity() const
    {
        return mPhysicsWorld->GetGravity();
    }

    void Scene::Clear()
    {
        for (auto& entity : mEntities)
        {
            entity->DetachFromHierarchy();
            entity->Destroy();
        }

        mEntities.clear();
        mPendingRemoval.clear();
    }

    void Scene::OnUpdate(float32 deltaTime)
    {
        if (deltaTime < 0.0f)
            deltaTime = Clock::GetDeltaTime();

        for (auto& entity : mEntities)
            entity->UpdateBegin();

        for (auto& entity : mEntities)
            entity->Update();

        for (auto& entity : mEntities)
            entity->UpdateEnd();

        // Advance the simulation, then reflect the results on the entities and fire collisions.
        mPhysicsWorld->Step(deltaTime);

        FlushPendingRemoval();
    }

    void Scene::OnRender()
    {
        for (auto& entity : mEntities)
            entity->Render();
    }

    void Scene::FlushRemovals()
    {
        FlushPendingRemoval();
    }

    void Scene::FlushPendingRemoval()
    {
        for (auto& entity : mPendingRemoval)
        {
            const auto it = std::find(mEntities.begin(), mEntities.end(), entity);

            if (it != mEntities.end())
            {
                mEntities.erase(it);
                entity->DetachFromHierarchy();
                entity->Destroy();
            }
        }

        mPendingRemoval.clear();
    }
}
