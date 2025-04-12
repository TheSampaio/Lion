#include "GameplayLayer.h"

using namespace Lion;

void GameplayLayer::OnAttach()
{
	mCamera = nullptr;
	mSprite = nullptr;
}

void GameplayLayer::OnCreate()
{
	mCamera = new CameraOrthographic();
	mSprite = new Sprite("Resource/Sprite/showcase-alex-kidd-remastered.jpg");
}

void GameplayLayer::OnRender()
{
	Renderer::RenderBegin(*mCamera);

    mSprite->Draw(0.0f, 0.0f);

	Renderer::RenderEnd();
}

void GameplayLayer::OnDetach()
{
	delete mCamera;
	delete mSprite;
}

void GameplayLayer::OnEvent(Lion::Event& event)
{
	EventDispatcher dispatcher(event);
	dispatcher.Bind<EventWindowResize>(LN_EVENT_BIND(GameplayLayer::OnEventWindowResize));
}

bool GameplayLayer::OnEventWindowResize(const Lion::EventWindowResize& event)
{
	mCamera->OnResize(event.GetWidth() / 2, event.GetHeight());
	return false;
}
