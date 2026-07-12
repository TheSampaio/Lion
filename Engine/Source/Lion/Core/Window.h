#pragma once

#include <Lion/Platform/WindowBackend.h>
#include <Lion/Type/Size.h>

namespace Lion
{
    class Application;
    class Event;
    class Graphics;

    // Engine-facing window facade.
    //
    // Exposes a windowing-library-agnostic API and delegates every native operation to a
    // WindowBackend. No windowing library type appears here, so the rest of the engine is
    // decoupled from the platform backend.
    class Window
    {
    public:
        // Opaque native window handle (consumed by the graphics backend to build its surface).
        static LION_API void* GetNativeHandle() { return sInstance->mBackend->GetNativeHandle(); }

        static LION_API std::array<float32, 3> GetBackgroundColor() { return sInstance->mBackgroundColor; }
        static LION_API Size GetSize() { return { sInstance->mData.width, sInstance->mData.height }; }
        static LION_API std::string GetTitle() { return sInstance->mData.title; }

        static LION_API void SetBackgroundColor(float32 red, float32 green, float32 blue);
        static LION_API void SetIcon(const std::string& icon);
        static LION_API void SetResizable(bool enable);
        static LION_API void SetMaximized(bool enable);
        static LION_API void SetSize(uint32 width, uint32 height);
        static LION_API void SetTitle(const std::string& title);

        // Sets the title-bar text without changing the stored base title (used for live stats).
        static LION_API void SetDisplayTitle(const std::string& title);

        // The window's chrome, which only the editor has an opinion about.
        //
        // A game's window should look like every other window on the machine — that is what the user set
        // the machine up to look like. The editor is the engine's own face, so it takes the dark caption
        // and then takes the caption away and draws one: a title bar it draws is a title bar it can put
        // its logo, its menus and its own buttons in.
        //
        // 'blocked' is set every frame while the cursor is over something clickable in the drawn caption:
        // the platform drags the window by the rest of it, and a drag that started on a menu would open
        // nothing and move everything.
        static LION_API void SetDarkTitleBar(bool enable);
        static LION_API void SetCustomTitleBar(bool enable, float32 height);
        static LION_API void SetTitleBarBlocked(bool blocked);

        static LION_API void Minimize();
        static LION_API void ToggleMaximize();
        static LION_API bool IsMaximized();

        // Immediate keyboard state, using engine key codes.
        static LION_API bool IsKeyPressed(int32 keyCode) { return sInstance->mBackend->IsKeyPressed(keyCode); }
        static LION_API bool IsKeyReleased(int32 keyCode) { return sInstance->mBackend->IsKeyReleased(keyCode); }

        static LION_API bool Close();

        // Requests the window to close (used by editor/menu "Exit" actions).
        static LION_API void RequestClose();

        friend Application;
        friend Graphics;

    protected:
        static Window* sInstance;

        static void New();
        static void Delete();

        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

    private:
        using EventCallback = std::function<void(Event&)>;

        WindowData mData;
        std::array<float32, 3> mBackgroundColor;
        std::string mIconPath;
        Scope<WindowBackend> mBackend;

        Window();

        static void SetEventCallback(const EventCallback& callback) { sInstance->mData.eventCallback = callback; }

        static bool Initialize();
        static void Show();
        static void PollEvents();
    };
}
