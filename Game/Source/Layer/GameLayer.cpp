#include "GameLayer.h"

#include "../Actor/Ball.h"
#include "../Actor/Manager.h"
#include "../Actor/Paddle.h"
#include "../Actor/Wall.h"

using namespace Lion;

void GameLayer::OnCreate()
{
	mCamera = MakeReference<CameraOrthographic>();

	mScene = MakeReference<Scene>();
	mScene->SetGravity(glm::vec2(0.0f, 0.0f)); // Brickout keeps the ball moving without gravity.

	// Static walls around the play area. The bottom is intentionally open: missing the ball loses.
	const Size field = Window::GetSize();
	const float32 halfWidth = field.width / 2.0f;
	const float32 halfHeight = field.height / 2.0f;
	const float32 thickness = 20.0f;

	mScene->Add(MakeReference<Wall>(Vector(0.0f, halfHeight, Depth::Back), field.width, thickness));  // Top
	mScene->Add(MakeReference<Wall>(Vector(-halfWidth, 0.0f, Depth::Back), thickness, field.height)); // Left
	mScene->Add(MakeReference<Wall>(Vector(halfWidth, 0.0f, Depth::Back), thickness, field.height));  // Right

	auto ball = MakeReference<Ball>();
	mScene->Add(ball);
	mScene->Add(MakeReference<Paddle>());
	mScene->Add(MakeReference<Manager>(ball));
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
