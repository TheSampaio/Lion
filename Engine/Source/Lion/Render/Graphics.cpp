#include "Engine.h"
#include "Graphics.h"

#include <Lion/Core/Log.h>
#include <Lion/Core/Window.h>
#include <Lion/Render/RenderCommand.h>
#include <Lion/Type/Macro.h>

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
            Log::Console(LogLevel::Error, "[Graphics] No valid window reference. Initialization aborted.");
            return false;
        }

        // Load OpenGL with GLAD
        glfwMakeContextCurrent(Window::GetId());
        
        if (!gladLoadGL())
        {
            Log::Console(LogLevel::Error, "[Graphics] GLAD initialization failed.");
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

    void Graphics::ShowSpecification()
    {
        Log::Console(LogLevel::Information, LION_FORMAT_TEXT("[Graphics] Graphics Card:  {}.", reinterpret_cast<const char8*>(glGetString(GL_RENDERER))));
        Log::Console(LogLevel::Information, LION_FORMAT_TEXT("[Graphics] OpenGL Version: {}.", reinterpret_cast<const char8*>(glGetString(GL_VERSION))));
    }
}
