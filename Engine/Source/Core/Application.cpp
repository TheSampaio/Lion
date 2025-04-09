#include "Engine.h"
#include "Application.h"

#include "Layer.h"
#include "Log.h"
#include "Stack.h"
#include "Window.h"

#include "../Events/Event.h"
#include "../Events/EventDispatcher.h"
#include "../Events/EventWindow.h"

#include "../Render/Graphics.h"

namespace Lion
{
	Graphics* Application::sGraphics = nullptr;
	Window* Application::sWindow = nullptr;

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
				//Renderer::OnWindowResize(e.GetWidth(), e.GetHeight());
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
#endif 

		mStack = new Stack();

		Window::New();
		sWindow = Window::sInstance;

		Graphics::New();
		sGraphics = Graphics::sInstance;
	}

	Application::~Application()
	{
		sGraphics = nullptr;
		Graphics::Delete();

		sWindow = nullptr;
		Window::Delete();

		delete mStack;

#ifndef LN_SHIPPING
		Log::Delete();
#endif 
	}

	void Application::Run()
	{
		if (!sWindow->Create())
		{
			Lion::Log::Console(Lion::ELogMode::Error, "[Application] Window creation failed.");
			return;
		}

		Log::Console(ELogMode::Success, "[Application] Window created successfully.");

		if (!sGraphics->Initialize())
		{
			Lion::Log::Console(Lion::ELogMode::Error, "[Application] Graphics initialization failed.");
			return;
		}

		Log::Console(ELogMode::Success, "[Application] Graphics initialized successfully.");

		sWindow->SetEventCallback(LN_EVENT_BIND(Application::OnEvent));

		do
		{
			sWindow->PollEvents();

			if (!mMinimized)
			{
				for (Layer* layer : *mStack)
					layer->OnUpdate();

				sGraphics->ClearBuffers();

				for (Layer* layer : *mStack)
					layer->OnRender();

				sGraphics->SwapBuffers();
			}

		} while (!sWindow->Close());
	}
}
