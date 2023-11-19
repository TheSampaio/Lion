#pragma once

namespace owl
{
    // Forward declarations
    class Entity;

    class Scene
    {
    public:
        OWL_API Scene() {};
        OWL_API ~Scene();

        // === MAIN methods ======

        // Adds a entity to the scene
        void OWL_API Add(Entity* Entity);

        // Updates all entities inside
        void OWL_API Update();

        // Draws all entities inside
        void OWL_API Draw();

        // Resets the list for starts at the begining
        void OWL_API Begin();

        // Removes last entity returned by GetNext()
        void OWL_API Remove();

        // === GET methods ======

        OWL_API Entity* GetNext();

    private:
        // Attributes
        std::list <Entity*> m_Entities;
        std::list <Entity*>::iterator m_Next;
        std::list <Entity*>::iterator m_Current;
    };
}
