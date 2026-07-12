#include "Engine.h"
#include "Window.h"

#include <Lion/Core/Version.h>

namespace Lion
{
    Window* Window::sInstance = nullptr;

    void Window::New()
    {
        sInstance = new Window();
    }

    void Window::Delete()
    {
        delete sInstance;
        sInstance = nullptr;
    }

    Window::Window()
        : mData{ "Lion Engine", 800, 600 },  // resizable/eventCallback keep their defaults.
        mBackgroundColor{ 0.0f, 0.0f, 0.0f },
        mBackend(WindowBackend::Create())
    {
    }

    bool Window::Initialize()
    {
        if (!sInstance->mBackend->Initialize(&sInstance->mData))
            return false;

        // A window always has an icon: the engine's, until a game says otherwise.
        sInstance->mBackend->SetIcon(sInstance->mIconPath.empty() ? kEngineIconFile : sInstance->mIconPath);

        return true;
    }

    void Window::Show()
    {
        sInstance->mBackend->Show();
    }

    void Window::PollEvents()
    {
        sInstance->mBackend->PollEvents();
    }

    bool Window::Close()
    {
        return sInstance->mBackend->ShouldClose();
    }

    void Window::RequestClose()
    {
        sInstance->mBackend->RequestClose();
    }

    void Window::SetBackgroundColor(float32 red, float32 green, float32 blue)
    {
        sInstance->mBackgroundColor = { red, green, blue };
    }

    void Window::SetRefreshCallback(const std::function<void()>& callback)
    {
        sInstance->mData.refreshCallback = callback;
    }

    void Window::SetDarkTitleBar(bool enable)
    {
        sInstance->mData.darkTitleBar = enable;
    }

    void Window::SetCustomTitleBar(bool enable, float32 height)
    {
        sInstance->mData.customTitleBar = enable;
        sInstance->mData.titleBarHeight = height;
    }

    void Window::SetTitleBarBlocked(bool blocked)
    {
        sInstance->mData.titleBarBlocked = blocked;
    }

    void Window::Minimize()
    {
        sInstance->mBackend->Minimize();
    }

    void Window::ToggleMaximize()
    {
        sInstance->mBackend->ToggleMaximize();
    }

    bool Window::IsMaximized()
    {
        return sInstance->mBackend->IsMaximized();
    }

    bool Window::IsPointerOnResizeBorder()
    {
        return sInstance->mData.pointerOnResizeBorder;
    }

    void Window::SetIcon(const std::string& icon)
    {
        sInstance->mIconPath = icon;

        // Apply immediately when the window already exists; otherwise it is applied at Initialize.
        if (sInstance->mBackend->GetNativeHandle())
            sInstance->mBackend->SetIcon(icon);
    }

    void Window::SetResizable(bool enable)
    {
        sInstance->mData.resizable = enable;
        sInstance->mBackend->SetResizable(enable);  // Applied live too when the window exists.
    }

    void Window::SetMaximized(bool enable)
    {
        sInstance->mData.maximized = enable;
    }

    void Window::SetSize(uint32 width, uint32 height)
    {
        sInstance->mData.width = width;
        sInstance->mData.height = height;
    }

    void Window::SetTitle(const std::string& title)
    {
        sInstance->mData.title = title;
        sInstance->mBackend->SetDisplayTitle(title);
    }

    void Window::SetDisplayTitle(const std::string& title)
    {
        sInstance->mBackend->SetDisplayTitle(title);
    }
}
