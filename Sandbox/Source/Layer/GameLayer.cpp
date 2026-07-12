#include "GameLayer.h"

#include "../../Assets/Scripts/Ball.h"
#include "../../Assets/Scripts/BrickField.h"
#include "../../Assets/Scripts/Paddle.h"

using namespace Lion;

namespace
{
	// An invisible boundary: a body and a shape, and nothing else. It has no behaviour, so it has no
	// component of the game's own — a wall is not a kind of entity, it is an entity made of two traits.
	Reference<Entity> CreateWall(const Vector& position, float32 width, float32 height)
	{
		auto wall = MakeReference<Entity>();
		wall->SetName("Wall");
		wall->GetTransform()->SetPosition(position);

		wall->AddComponent<RigidBody2D>(BodyType::Static);
		wall->AddComponent<BoxCollider2D>(width, height, 1.0f, 0.0f, 1.0f);

		return wall;
	}
}

void GameLayer::OnCreate()
{
	mCamera = MakeReference<CameraOrthographic>();

	mScene = MakeReference<Scene>();
	mScene->SetGravity(glm::vec2(0.0f, 0.0f));  // Brickout keeps the ball moving without gravity.

	// Static walls around the play area. The bottom is intentionally open: missing the ball loses.
	const Size field = Window::GetSize();
	const float32 halfWidth = field.width / 2.0f;
	const float32 halfHeight = field.height / 2.0f;
	const float32 thickness = 20.0f;

	mScene->Add(CreateWall(Vector(0.0f, halfHeight, Depth::Back), field.width, thickness));   // Top
	mScene->Add(CreateWall(Vector(-halfWidth, 0.0f, Depth::Back), thickness, field.height));  // Left
	mScene->Add(CreateWall(Vector(halfWidth, 0.0f, Depth::Back), thickness, field.height));   // Right

	// The paddle goes in before the ball, which looks the paddle up when it wakes.
	auto paddle = MakeReference<Entity>();
	paddle->SetName("Paddle");
	paddle->GetTransform()->SetPosition(Vector(0.0f, -260.0f, Depth::Front));

	const Size paddleSize = paddle->AddComponent<SpriteRenderer>("Sprites/Brickout/player.png")->GetSize();
	paddle->AddComponent<RigidBody2D>(BodyType::Kinematic);  // Driven by input, unmoved by the ball's impacts.
	paddle->AddComponent<BoxCollider2D>(paddleSize.width, paddleSize.height, 1.0f, 0.0f, 1.0f);
	paddle->AddComponent<Paddle>();
	mScene->Add(paddle);

	auto ball = MakeReference<Entity>();
	ball->SetName("Ball");
	ball->GetTransform()->SetPosition(Vector(0.0f, 0.0f, Depth::Middle));

	const Size ballSize = ball->AddComponent<SpriteRenderer>("Sprites/Brickout/ball.png")->GetSize();
	ball->AddComponent<RigidBody2D>(BodyType::Dynamic, true);  // Frictionless, perfectly elastic, never spins.
	ball->AddComponent<CircleCollider2D>(ballSize.width * 0.5f, 1.0f, 0.0f, 1.0f);
	ball->AddComponent<Ball>();
	mScene->Add(ball);

	// The round itself: the backdrop it is played on, and the field of bricks it is played against.
	auto round = MakeReference<Entity>();
	round->SetName("Brick Field");
	round->GetTransform()->SetPosition(Vector(0.0f, 0.0f, Depth::Back));

	round->AddComponent<SpriteRenderer>("Sprites/Brickout/background.jpg");
	round->AddComponent<BrickField>();
	mScene->Add(round);
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
