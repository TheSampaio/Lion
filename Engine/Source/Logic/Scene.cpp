#include "Engine.h"
#include "Scene.h"

#include "Actor.h"
#include "Entity.h"

namespace Lion
{
    void Scene::Add(Reference<Entity> entity)
    {
        mEntities.push_back(entity);
        entity->mScene = shared_from_this();
        entity->OnAwake();
    }

    void Scene::Remove(Reference<Entity> entity)
    {
        mEntities.remove(entity);
        entity->OnDestroy();
    }

    void Scene::OnUpdate()
    {
        for (auto& entity : mEntities)
            entity->OnUpdateBegin();

        for (auto& entity : mEntities)
            entity->OnUpdate();

        for (auto& entity : mEntities)
            entity->OnUpdateEnd();

        CollisionDetection();
    }

    void Scene::OnRender()
    {
        for (auto& entity : mEntities)
            entity->OnRender();
    }

    bool Scene::Collision(Reference<Actor> a, Reference<Actor> b)
    {
        return false;
    }

    void Scene::CollisionDetection()
    {
    }
}
