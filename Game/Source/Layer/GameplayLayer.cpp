#include "GameplayLayer.h"

#include "../Actor/Ball.h"
#include "../Actor/GameManager.h"
#include "../Actor/Paddle.h"

using namespace Lion;

void GameplayLayer::OnCreate()
{
	mCamera = MakeReference<CameraOrthographic>();
	mScene = MakeReference<Scene>();

	mScene->Add(MakeReference<Ball>());
	mScene->Add(MakeReference<GameManager>());
	mScene->Add(MakeReference<Paddle>());
}

void GameplayLayer::OnUpdate()
{
	mScene->OnUpdate();
}

void GameplayLayer::OnRender()
{
	Renderer::RenderBegin(mCamera);
	mScene->OnRender();
	Renderer::RenderEnd();
}

void GameplayLayer::OnEvent(Event& event)
{
	EventDispatcher dispatcher(event);
	dispatcher.Bind<EventWindowResize>(LN_EVENT_BIND(GameplayLayer::OnEventWindowResize));
}

bool GameplayLayer::OnEventWindowResize(const EventWindowResize& event)
{
	mCamera->OnResize(static_cast<float32>(event.GetWidth()), static_cast<float32>(event.GetHeight()));
	return false;
}
