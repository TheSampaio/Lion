#include "Engine.h"
#include "Graphics.h"

#include "../Core/Log.h"
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

    void Graphics::ClearBuffers()
    {
        const auto& backgroundColor = Window::GetBackgroundColor();

        glClearColor(
            static_cast<GLfloat>(backgroundColor[0]),
            static_cast<GLfloat>(backgroundColor[1]),
            static_cast<GLfloat>(backgroundColor[2]),
            static_cast<GLfloat>(backgroundColor[3])
        );
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void Graphics::SwapBuffers()
    {
        glfwSwapInterval(0); // TODO: Remove hard code
        glfwSwapBuffers(Window::GetId());
    }

    bool Graphics::Initialize()
    {
        if (!Window::GetId())
        {
            Log::Console(ELogMode::Error, "[Graphics] No valid window reference. Initialization aborted.");
            return false;
        }

        // Load OpenGL with GLAD
        glfwMakeContextCurrent(Window::GetId());
        
        if (!gladLoadGL())
        {
            Log::Console(ELogMode::Error, "[Graphics] GLAD initialization failed on engine side. Check if OpenGL context is correctly bound.");
            return false;
        }

        return true;
    }
}
