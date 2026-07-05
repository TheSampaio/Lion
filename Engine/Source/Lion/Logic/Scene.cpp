#include "Engine.h"
#include "Scene.h"

#include <Lion/Logic/Actor.h>
#include <Lion/Logic/Entity.h>

namespace Lion
{
    void Scene::Add(Reference<Entity> entity)
    {
        mEntities.push_back(entity);
        entity->mScene = shared_from_this();
        entity->Awake();
    }

    void Scene::Remove(Reference<Entity> entity)
    {
        mEntities.remove(entity);
        entity->Destroy();
    }

    void Scene::OnUpdate()
    {
        for (auto& entity : mEntities)
            entity->UpdateBegin();

        for (auto& entity : mEntities)
            entity->Update();

        for (auto& entity : mEntities)
            entity->UpdateEnd();

        CollisionDetection();
    }

    void Scene::OnRender()
    {
        for (auto& entity : mEntities)
            entity->Render();
    }

    bool Scene::Collision(Reference<Actor> a, Reference<Actor> b)
    {
        return false;
    }

    void Scene::CollisionDetection()
    {
    }
}
