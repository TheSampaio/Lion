#include "Engine.h"
#include "Header/Scene.h"

#include "Header/Entity.h"

Lion::Scene::~Scene()
{
    // Removes entities from memory heap
    for (auto i : m_Entities)
        delete i;
}

void Lion::Scene::Add(Entity* Entity)
{
    // Adds a entity to the scene's list
    m_Entities.push_back(Entity);
}

void Lion::Scene::Update()
{
    // Updates all entities
    m_Next = m_Entities.begin();

    while (m_Next != m_Entities.end())
    {
        m_Current = m_Next++;
        (*m_Current)->OnUpdate();
    }
}

void Lion::Scene::Draw()
{
    // Draws all entities
    m_Next = m_Entities.begin();

    while (m_Next != m_Entities.end())
    {
        m_Current = m_Next++;
        (*m_Current)->OnDraw();
    }
}

void Lion::Scene::Begin()
{
    // Points for the first list's element
    m_Next = m_Entities.begin();
}

void Lion::Scene::Remove()
{
    // Removes an element being processed with Next, Update or Draw
    delete (*m_Current);
    m_Entities.erase(m_Current);
}

Lion::Entity* Lion::Scene::GetNext()
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
