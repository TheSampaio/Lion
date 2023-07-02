#include "Engine.h"
#include "Header/Scene.h"

#include "Header/Entity.h"

owl::Scene::Scene()
{
}

owl::Scene::~Scene()
{
    // Removes entities from memory heap
    for (auto i : m_Entities)
        delete i;
}

void owl::Scene::Add(Entity* Entity)
{
    // Adds a entity to the scene's list
    m_Entities.push_back(Entity);
}

void owl::Scene::Update()
{
    // Updates all entities
    m_Next = m_Entities.begin();

    while (m_Next != m_Entities.end())
    {
        m_Current = m_Next++;
        (*m_Current)->OnUpdate();
    }
}

void owl::Scene::Draw()
{
    // Draws all entities
    m_Next = m_Entities.begin();

    while (m_Next != m_Entities.end())
    {
        m_Current = m_Next++;
        (*m_Current)->OnDraw();
    }
}

void owl::Scene::Begin()
{
    // Points for the first list's element
    m_Next = m_Entities.begin();
}

void owl::Scene::Remove()
{
    // Removes an element being processed with Next, Update or Draw
    delete (*m_Current);
    m_Entities.erase(m_Current);
}

owl::Entity* owl::Scene::GetNext()
{
    // Points for the next list's element
    if (m_Next != m_Entities.end())
    {
        m_Current = m_Next++;
        return *m_Current;
    }

    else
        return nullptr;
}
