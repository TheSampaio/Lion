#include "Engine.h"
#include "Window.h"

#include "Log.h"

#include "../Events/EventWindow.h"
#include "../Events/EventInput.h"

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
        : mId(nullptr), mData{ "Lion Engine", 800, 600, nullptr }, mBackgroundColor(0.0f, 0.0f, 0.0f, 1.0f)
    {
        if (glfwInit() != GLFW_TRUE)
        {
            Log::Console(ELogMode::Fatal, "[Application] GLFW initialization failed.");
            return;
        }

        Log::Console(ELogMode::Success, "[Application] GLFW initialized successfully.");
    }

    Window::~Window()
    {
        glfwDestroyWindow(mId);
        glfwTerminate();
    }

    bool Window::Create()
    {
        glfwWindowHint(GLFW_VISIBLE, false);

        sInstance->mId = glfwCreateWindow(sInstance->mData.Width, sInstance->mData.Height, sInstance->mData.Title.c_str(), nullptr, nullptr);

        if (!sInstance->mId)
            return false;

        // Window events
        glfwSetWindowUserPointer(sInstance->mId, &sInstance->mData);

        // Close callback
        glfwSetWindowCloseCallback(sInstance->mId, [](GLFWwindow* window)
            {
                WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
                EventWindowClose event;
                data.mEventCallback(event);
            });

        // Window focus callback
        glfwSetWindowFocusCallback(sInstance->mId, [](GLFWwindow* window, int32 focused)
            {
                WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

                if (focused)
                {
                    EventWindowFocusEnter event;
                    data.mEventCallback(event);
                }

                else
                {
                    EventWindowFocusExit event;
                    data.mEventCallback(event);
                }
            });

        // Resize callback
        glfwSetWindowSizeCallback(sInstance->mId, [](GLFWwindow* window, int32 width, int32 height)
            {
                WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
                data.Width = width;
                data.Height = height;

                EventWindowResize event(width, height);

                if (data.mEventCallback)
                    data.mEventCallback(event);
            });

        // Keyboard callback
        glfwSetKeyCallback(sInstance->mId, [](GLFWwindow* window, int32 key, int32 scancode, int32 action, int32 mods)
            {
                WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

                switch (action)
                {
                    case GLFW_PRESS:
                    {
                        EventInputKeyboardPress event(key);
                        data.mEventCallback(event);
                        break;
                    }

                    case GLFW_RELEASE:
                    {
                        EventInputKeyboardRelease event(key);
                        data.mEventCallback(event);
                        break;
                    }

                    case GLFW_REPEAT:
                    {
                        EventInputKeyboardRepeat event(key);
                        data.mEventCallback(event);
                        break;
                    }
                }
            });

        // Mouse button callback
        glfwSetMouseButtonCallback(sInstance->mId, [](GLFWwindow* window, int32 button, int32 action, int32 mods)
            {
                WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

                switch (action)
                {
                    case GLFW_PRESS:
                    {
                        EventInputMousePress event(button);
                        data.mEventCallback(event);
                        break;
                    }

                    case GLFW_RELEASE:
                    {
                        EventInputMouseRelease event(button);
                        data.mEventCallback(event);
                        break;
                    }
                }
            });

        // Mouse scroll callback
        glfwSetScrollCallback(sInstance->mId, [](GLFWwindow* window, float64 xOffset, float64 yOffset)
            {
                WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

                EventInputMouseScroll event((float32)xOffset, (float32)yOffset);
                data.mEventCallback(event);
            });

        // Mouse move callback
        glfwSetCursorPosCallback(sInstance->mId, [](GLFWwindow* window, float64 xPos, float64 yPos)
            {
                WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

                EventInputMouseMove event((float32)xPos, (float32)yPos);
                data.mEventCallback(event);
            });

        return true;
    }

    void Window::Show()
    {
        glfwShowWindow(sInstance->mId);
    }

    void Window::PollEvents()
    {
        glfwPollEvents();
    }

    void Window::SetSize(uint32 width, uint32 height)
    {
        sInstance->mData.Width = width;
        sInstance->mData.Height = height;
    }

    void Window::SetBackgroundColor(float32 red, float32 green, float32 blue, float32 alpha)
    {
        sInstance->mBackgroundColor = {red, green, blue, alpha};
    }

    void Window::SetTitle(const std::string& title)
    {
        sInstance->mData.Title = title;
    }

    bool Window::Close()
    {
        return glfwWindowShouldClose(sInstance->mId);
    }
}
