#include "GameLayer.h"

#include "../Actor/Ball.h"
#include "../Actor/Manager.h"
#include "../Actor/Paddle.h"

using namespace Lion;

void GameLayer::OnCreate()
{
	mCamera = MakeReference<CameraOrthographic>();
	mScene = MakeReference<Scene>();

	mScene->Add(MakeReference<Ball>());
	mScene->Add(MakeReference<Manager>());
	mScene->Add(MakeReference<Paddle>());
}

void GameLayer::OnUpdate()
{
	mScene->OnUpdate();
}

void GameLayer::OnRender()
{
	Renderer::RenderBegin(mCamera);
	mScene->OnRender();
	Renderer::RenderEnd();
}

void GameLayer::OnEvent(Event& event)
{
	EventDispatcher dispatcher(event);
	dispatcher.Bind<EventWindowResize>(LION_BIND_EVENT(GameLayer::OnEventWindowResize));
}

bool GameLayer::OnEventWindowResize(const EventWindowResize& event)
{
	mCamera->OnResize(static_cast<float32>(event.GetWidth()), static_cast<float32>(event.GetHeight()));
	return false;
}
