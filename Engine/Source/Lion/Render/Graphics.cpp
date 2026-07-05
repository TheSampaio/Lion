#include "Engine.h"
#include "Graphics.h"

#include <Lion/Core/Log.h>
#include <Lion/Core/Window.h>

#include <Lion/Render/GraphicsContext.h>
#include <Lion/Render/RenderCommand.h>

namespace Lion
{
    Graphics* Graphics::sInstance = nullptr;

    void Graphics::New()
    {
        sInstance = new Graphics();
    }

    void Graphics::Delete()
    {
        RenderCommand::Shutdown();

        delete sInstance;
        sInstance = nullptr;
    }

    Graphics::Graphics()
    {
        mIsVerticalSynchronizedEnabled = false;
        mIsVerticalSynchronizationDirty = true;  // Apply the initial swap interval on the first present.
    }

    void Graphics::SetVerticalSynchronization(bool enable)
    {
        if (sInstance->mIsVerticalSynchronizedEnabled == enable)
            return;

        sInstance->mIsVerticalSynchronizedEnabled = enable;
        sInstance->mIsVerticalSynchronizationDirty = true;
    }

    bool Graphics::Initialize()
    {
        if (!Window::GetId())
        {
            Log::Console(LogLevel::Error, "[Graphics] No valid window reference. Initialization aborted.");
            return false;
        }

        sInstance->mContext = GraphicsContext::Create(Window::GetId());

        if (!sInstance->mContext || !sInstance->mContext->Init())
            return false;

        RenderCommand::Init();
        return true;
    }

    void Graphics::ClearBuffers()
    {
        const auto& backgroundColor = Window::GetBackgroundColor();

        RenderCommand::SetClearColor(
            static_cast<float32>(backgroundColor[0]),
            static_cast<float32>(backgroundColor[1]),
            static_cast<float32>(backgroundColor[2]),
            1.0f
        );

        RenderCommand::Clear();
    }

    void Graphics::SwapBuffers()
    {
        // The swap interval only needs to be reapplied when the user toggles VSync.
        if (sInstance->mIsVerticalSynchronizationDirty)
        {
            sInstance->mContext->SetVerticalSync(sInstance->mIsVerticalSynchronizedEnabled);
            sInstance->mIsVerticalSynchronizationDirty = false;
        }

        sInstance->mContext->SwapBuffers();
    }

    void Graphics::ShowSpecification()
    {
        Log::Console(LogLevel::Information, LION_FORMAT_TEXT("[Graphics] Graphics Card:  {}.", sInstance->mContext->GetDeviceName()));
        Log::Console(LogLevel::Information, LION_FORMAT_TEXT("[Graphics] OpenGL Version: {}.", sInstance->mContext->GetApiVersion()));
    }
}
