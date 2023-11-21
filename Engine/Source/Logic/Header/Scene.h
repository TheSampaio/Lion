#pragma once

namespace Lion
{
    // Forward declarations
    class Entity;

    class Scene
    {
    public:
        LION_API Scene() {};
        LION_API ~Scene();

        // === MAIN methods ======

        // Adds a entity to the scene
        void LION_API Add(Entity* Entity);

        // Updates all entities inside
        void LION_API Update();

        // Draws all entities inside
        void LION_API Draw();

        // Resets the list for starts at the begining
        void LION_API Begin();

        // Removes last entity returned by GetNext()
        void LION_API Remove();

        // === GET methods ======

        LION_API Entity* GetNext();

    private:
        // Attributes
        std::list <Entity*> m_Entities;
        std::list <Entity*>::iterator m_Next;
        std::list <Entity*>::iterator m_Current;
    };
}
