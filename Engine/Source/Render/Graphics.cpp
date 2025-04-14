#include "Engine.h"
#include "Graphics.h"

#include "../Core/Log.h"
#include "../Core/Window.h"
#include "../Render/RenderCommand.h"

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

    Graphics::Graphics()
    {
        mIsVerticalSynchronizedEnabled = false;
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

    void Graphics::ClearBuffers()
    {
        const auto& backgroundColor = Window::GetBackgroundColor();

        RenderCommand::ClearColor(
            static_cast<float32>(backgroundColor[0]),
            static_cast<float32>(backgroundColor[1]),
            static_cast<float32>(backgroundColor[2]),
            1.0f
        );

        RenderCommand::Clear(GL_COLOR_BUFFER_BIT);
    }

    void Graphics::SwapBuffers()
    {
        glfwSwapInterval(sInstance->mIsVerticalSynchronizedEnabled);
        glfwSwapBuffers(Window::GetId());
    }
}
