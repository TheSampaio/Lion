#include "Engine.h"
#include "Application.h"

#include "Asset.h"
#include "Clock.h"
#include "Input.h"
#include "Layer.h"
#include "Log.h"
#include "Stack.h"
#include "Window.h"

#include "../Events/Event.h"
#include "../Events/EventDispatcher.h"
#include "../Events/EventWindow.h"

#include "../Render/Graphics.h"
#include "../Render/Renderer.h"

namespace Lion
{
	template<typename T>
	static T TryInitialize(T result, const char8* name)
	{
		if (!result)
		{
			Lion::Log::Console(Lion::LogLevel::Fatal, LN_LOG_FORMAT("[Application] {} initialization failed.", name));
			return result;
		}

		Log::Console(LogLevel::Success, LN_LOG_FORMAT("[Application] {} initialized successfully.", name));
		return result;
	}

	void Application::PushLayer(Layer* layer)
	{
		mStack->PushLayer(layer);
		layer->OnAttach();
	}

	void Application::PushOverlay(Layer* overlay)
	{
		mStack->PushOverlay(overlay);
		overlay->OnAttach();
	}

	void Application::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);

		// Event window resize
		dispatcher.Bind<EventWindowResize>([this](const EventWindowResize& e)
			{
				if (e.GetWidth() == 0 || e.GetHeight() == 0)
				{
					mMinimized = true;
					return false;
				}

				mMinimized = false;
				Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());
				return false;
			});

		for (auto it = mStack->rbegin(); it != mStack->rend(); ++it)
		{
			if (event.Handled)
				break;

			(*it)->OnEvent(event);
		}
	}

	Application::Application()
		: mStack(nullptr), mMinimized(false)
	{
#ifndef LN_SHIPPING
		Log::New();

#endif // !LN_SHIPPING

		mAsset = MakeScope<Asset>();
		mStack = MakeScope<Stack>();

		Window::New();
		Input::New();
		Graphics::New();
		Renderer::New();
		Clock::New();
	}

	Application::~Application()
	{
		Clock::Delete();
		Renderer::Delete();
		Graphics::Delete();
		Input::Delete();
		Window::Delete();


#ifndef LN_SHIPPING
		Log::Delete();
#endif 
	}

	void Application::Run()
	{
		// Initializes Window, Graphics and Renderer
		Initialize();

		// Window's events sign up
		Window::SetEventCallback(LN_EVENT_BIND(Application::OnEvent));

		// Starts engine's clock
		Clock::GetTimer().Start();

		// Load resources
		for (Layer* layer : *mStack)
			layer->OnCreate();

		// Show window
		Window::Show();

		do
		{
			Window::PollEvents();
			Clock::UpdateFrameTime();

			if (!mMinimized)
			{
				// Update
				for (Layer* layer : *mStack)
					layer->OnUpdateBegin();

				for (Layer* layer : *mStack)
					layer->OnUpdate();

				for (Layer* layer : *mStack)
					layer->OnUpdateEnd();

				// Clear
				Graphics::ClearBuffers();

				// Render
				for (Layer* layer : *mStack)
					layer->OnRender();

				Graphics::SwapBuffers();
			}

		} while (!Window::Close());
	}

	void Application::Initialize()
	{
		if (!TryInitialize(glfwInit(), "GLFW"))
			return;

		if(!TryInitialize(Window::Initialize(), "Window"))
			return;

		if(!TryInitialize(Graphics::Initialize(), "Graphics"))
			return;

		if (!TryInitialize(Renderer::Initialize(), "Renderer"))
			return;
	}
}
