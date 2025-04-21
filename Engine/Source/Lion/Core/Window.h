#pragma once

#include <Lion/Base/Platform.h>
#include <Lion/Type/Size.h>

namespace Lion
{
    class Application;
    class Event;
    class EventCallback;
    class Graphics;

    class Window
    {
    public:
        static LION_API GLFWwindow* GetId() { return sInstance->mId; }
        static LION_API std::array<float32, 3> GetBackgroundColor() { return sInstance->mBackgroundColor; }
        static LION_API Size GetSize() { return { sInstance->mData.Width, sInstance->mData.Height }; }
        static LION_API std::string GetTitle() { return sInstance->mData.Title; }

        static LION_API void SetBackgroundColor(float32 red, float32 green, float32 blue);
        static LION_API void SetIcon(const std::string& icon);
        static LION_API void SetSize(uint32 width, uint32 height);
        static LION_API void SetTitle(const std::string& title);

        static LION_API bool Close();

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

        struct WindowData
        {
            std::string Title;
            uint32 Width, Height;
            EventCallback mEventCallback;
        };

        GLFWwindow* mId;
        GLFWimage* mIcon;
        WindowData mData;
        std::array<float32, 3> mBackgroundColor;

        Window();
        ~Window();

        static void SetEventCallback(const EventCallback& callback) { sInstance->mData.mEventCallback = callback; }

        static bool Initialize();
        static void Show();
        static void PollEvents();
    };
}
