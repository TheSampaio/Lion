#include "Engine.h"
#include "Graphics.h"

#include "../Core/Window.h"

namespace Lion
{
    Graphics* Graphics::sInstance = nullptr;

    void Graphics::New()
    {
        sInstance = new Graphics();
    }

    void Graphics::Delete()
    {
        delete sInstance;
        sInstance = nullptr;
    }

    void Lion::Graphics::ClearBuffers()
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f); // TODO: Remove hard code (Magic numbers)
    }

    bool Lion::Graphics::Initialize()
    {
        glfwMakeContextCurrent(Window::GetId());

        if (!gladLoadGL())
            return false;

        return true;
    }
}
