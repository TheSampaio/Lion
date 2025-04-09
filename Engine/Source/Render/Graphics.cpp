#include "Engine.h"
#include "Graphics.h"

#include "../Core/Log.h"
#include "../Core/Window.h"

namespace Lion
{
    Graphics* Graphics::sInstance = nullptr;
    Window* Graphics::sWindow = nullptr;

    void Graphics::New()
    {
        sInstance = new Graphics();
        sWindow = Window::sInstance;
    }

    void Graphics::Delete()
    {
        sWindow = nullptr;

        delete sInstance;
        sInstance = nullptr;
    }

    void Graphics::ClearBuffers()
    {
        const auto& backgroundColor = sWindow->GetBackgroundColor();

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
        glfwSwapBuffers(sWindow->mId);
    }

    bool Graphics::Initialize()
    {
        if (!sWindow)
        {
            Log::Console(ELogMode::Error, "[Graphics] No valid window reference. Initialization aborted.");
            return false;
        }

        glfwMakeContextCurrent(sWindow->mId);

        if (!gladLoadGL())
        {
            Log::Console(ELogMode::Error, "[Graphics] Failed to initialize OpenGL context with GLAD.");
            return false;
        }

        return true;
    }
}
