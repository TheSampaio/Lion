#include "GameRules.h"
#include "Ball.h"
#include "Brick.h"
#include "GameSettings.h"
#include "Paddle.h"
#include "ScreenTransition.h"

#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Logic/Reflector.h>

using namespace Lion;

void GameRules::OnAwake()
{
	sActiveRules = this;
}

void GameRules::InitializeForScene()
{
	mInitialized = true;
	mLevel = ActiveLevel();

	const Reference<Scene> scene = GetOwner().GetScene();
	const Reference<Entity> scoreEntity = scene->FindEntity("Score Text");
	const Reference<Entity> attemptsEntity = scene->FindEntity("Attempts Text");
	const Reference<Entity> levelEntity = scene->FindEntity("Level Text");
	const Reference<Entity> powerEntity = scene->FindEntity("Power Text");
	const Reference<Entity> controllerPrompts = scene->FindEntity("HUD Controller Prompts");
	mScoreText = scoreEntity ? scoreEntity->GetComponent<TextRenderer>() : nullptr;
	mAttemptsText = attemptsEntity ? attemptsEntity->GetComponent<TextRenderer>() : nullptr;
	mLevelText = levelEntity ? levelEntity->GetComponent<TextRenderer>() : nullptr;
	mPowerText = powerEntity ? powerEntity->GetComponent<TextRenderer>() : nullptr;
	mControllerPrompts = controllerPrompts.get();

	if (powerEntity)
	{
		powerEntity->SetVisible(false);
		powerEntity->SetEnabled(false);
	}
	UpdateHud();
	UpdateControllerPrompts();

	if (mLevel == 0)
		return;

	if (!sSessionActive)
	{
		sScore = 0;
		sAttempts = kStartingAttempts;
		sSessionActive = true;
		UpdateHud();
	}

	mBall = scene->FindComponent<Ball>();
	mPaddle = scene->FindComponent<Paddle>();
	mCamera = scene->FindComponent<Camera2D>();
	mImpactParticles = GetOwner().GetComponent<ParticleComponent>();

	if (mCamera)
		mCameraBaseOffset = mCamera->GetOffset();

	if (!mBall || !mPaddle || scene->CountActiveComponents<Brick>() == 0)
	{
		Log::Console(LogLevel::Error,
			"[GameRules] A gameplay scene requires a Ball, Paddle and authored Bricks.");
		SetEnabled(false);
		return;
	}

	Log::Console(LogLevel::Information,
		LION_FORMAT_TEXT("[Game] Level {}/{} - score {:06}, {} balls remaining.",
			mLevel, kFinalLevel, sScore, sAttempts));
}

void GameRules::OnUpdate()
{
	// SceneManager publishes the new resource path after deserialization has Awakened its entities.
	// Resolve path-dependent state on the first update, when that transition is complete.
	if (!mInitialized)
	{
		InitializeForScene();
		return;
	}

	UpdateShake();
	UpdateControllerPrompts();

	if (mPowerMessageRemaining > 0.0f)
	{
		mPowerMessageRemaining = std::max(0.0f, mPowerMessageRemaining - Clock::GetDeltaTime());

		if (mPowerMessageRemaining <= 0.0f && mPowerText)
		{
			mPowerText->GetOwner().SetVisible(false);
			mPowerText->GetOwner().SetEnabled(false);
		}
	}

	HandleDebugLevelKeys();

	if (mLevel > 0 && !mTransitionQueued)
		HandleLevelFlow();
}

void GameRules::OnDestroy()
{
	if (mCamera)
		mCamera->SetOffset(mCameraBaseOffset);

	if (sActiveRules == this)
		sActiveRules = nullptr;
}

void GameRules::Reflect(Reflector& reflector)
{
	reflector.Field("Lose Height", mLoseHeight);
	reflector.Field("Shake Duration", mShakeDuration);
	reflector.Field("Shake Strength", mShakeStrength);
}

void GameRules::StartNewGame()
{
	sScore = 0;
	sAttempts = kStartingAttempts;
	sSessionActive = true;
	ScreenTransition::LoadScene(LevelScene(1));
}

void GameRules::RegisterBrickDamage(const Vector2& position, bool destroyed, const std::string& power,
	Ball* sourceBall)
{
	if (!sSessionActive)
		return;

	if (destroyed)
		sScore += kBrickScore;

	if (!sActiveRules)
		return;

	if (GameSettings::HasCameraShake())
	{
		sActiveRules->mShakeRemaining = sActiveRules->mShakeDuration;
		sActiveRules->mShakeFrame = 0;
	}
	if (sActiveRules->mImpactParticles)
		sActiveRules->mImpactParticles->EmitAt(position, destroyed ? 30 : 14);

	if (destroyed && !power.empty())
		sActiveRules->ActivatePower(power, position, sourceBall);

	sActiveRules->UpdateHud();
}

void GameRules::UpdateHud()
{
	if (mScoreText)
		mScoreText->SetText(LION_FORMAT_TEXT("{} {:06}", GameSettings::Text(GameText::Score), sScore));

	if (mAttemptsText)
		mAttemptsText->SetText(LION_FORMAT_TEXT("{} {}", GameSettings::Text(GameText::Balls), sAttempts));

	if (mLevelText && mLevel > 0)
		mLevelText->SetText(LION_FORMAT_TEXT("{} {:02}", GameSettings::Text(GameText::Level), mLevel));
}

void GameRules::UpdateShake()
{
	if (!GameSettings::HasCameraShake())
	{
		mShakeRemaining = 0.0f;
		if (mCamera)
			mCamera->SetOffset(mCameraBaseOffset);
		return;
	}

	if (!mCamera || mShakeRemaining <= 0.0f)
		return;

	mShakeRemaining = std::max(0.0f, mShakeRemaining - Clock::GetDeltaTime());

	if (mShakeRemaining <= 0.0f)
	{
		mCamera->SetOffset(mCameraBaseOffset);
		return;
	}

	static const Vector2 offsets[] = {
		{ -1.0f,  0.5f },
		{  0.75f, -1.0f },
		{  1.0f,  0.75f },
		{ -0.5f, -0.75f },
	};

	const Vector2& direction = offsets[mShakeFrame++ % std::size(offsets)];
	mCamera->SetOffset(Vector2(
		mCameraBaseOffset.x + direction.x * mShakeStrength,
		mCameraBaseOffset.y + direction.y * mShakeStrength));
}

void GameRules::UpdateControllerPrompts()
{
	const bool show = Input::GetLastInputMethod() == InputMethod::Gamepad;

	if (!mControllerPrompts || show == mShowingControllerPrompts)
		return;

	mShowingControllerPrompts = show;
	mControllerPrompts->SetVisible(show);
	mControllerPrompts->SetEnabled(show);
}

void GameRules::HandleLevelFlow()
{
	const Reference<Scene> scene = GetOwner().GetScene();

	if (scene->CountActiveComponents<Brick>() == 0)
	{
		mTransitionQueued = true;

		if (mLevel < kFinalLevel)
			ScreenTransition::LoadScene(LevelScene(mLevel + 1));
		else
		{
			sSessionActive = false;
			ScreenTransition::LoadScene("Scenes/Victory.lnscene");
		}
	}
	else
	{
		bool hasBallInPlay = false;
		bool lostBall = false;

		for (const Reference<Entity>& entity : scene->GetEntities())
		{
			Ball* ball = entity->GetComponent<Ball>();

			if (!ball || !entity->IsActive())
				continue;

			if (entity->GetWorldPosition().y >= mLoseHeight)
			{
				hasBallInPlay = true;
				continue;
			}

			lostBall = true;
			ball->Stop();
			ball->SetVisible(false);
			entity->SetEnabled(false);
		}

		if (!lostBall || hasBallInPlay)
			return;

		sAttempts = std::max(sAttempts - 1, 0);
		UpdateHud();

		if (sAttempts > 0)
			RespawnBall();
		else
		{
			mTransitionQueued = true;
			sSessionActive = false;
			ScreenTransition::LoadScene("Scenes/Defeat.lnscene");
		}
	}
}

void GameRules::RespawnBall()
{
	if (!mBall)
		return;

	mBall->GetOwner().SetEnabled(true);
	mBall->SetVisible(true);
	mBall->Reset();
}

void GameRules::ActivatePower(const std::string& power, const Vector2& position, Ball* sourceBall)
{
	std::string message;

	if (power == "Extra Life")
	{
		sAttempts++;
		message = GameSettings::Text(GameText::ExtraBall);
	}
	else if (power == "Multiball")
	{
		if (sourceBall)
			SpawnExtraBalls(*sourceBall);
		message = GameSettings::Text(GameText::Multiball);
	}
	else
		return;

	if (mImpactParticles)
		mImpactParticles->EmitAt(position, 48);

	if (mPowerText)
	{
		mPowerText->SetText(message);
		mPowerText->GetOwner().SetVisible(true);
		mPowerText->GetOwner().SetEnabled(true);
		mPowerMessageRemaining = 1.4f;
	}

	UpdateHud();
}

void GameRules::SpawnExtraBalls(Ball& sourceBall)
{
	const Reference<Scene> scene = GetOwner().GetScene();
	const Vector2 origin = sourceBall.GetOwner().GetWorldPosition();

	for (int32 index = 0; index < 2; ++index)
	{
		const float32 side = index == 0 ? -1.0f : 1.0f;
		Reference<Entity> entity = MakeReference<Entity>();
		entity->SetName(LION_FORMAT_TEXT("Power Ball {}", index + 1));
		entity->GetTransform()->SetPosition(origin);
		entity->GetTransform()->SetScale(Vector2(1.0f, 1.0f));
		SpriteRenderer* renderer = entity->AddComponent<SpriteRenderer>("Sprites/Brickout/ball.png");
		renderer->SetOrder(20);
		entity->AddComponent<RigidBody2D>(BodyType::Dynamic, true);
		entity->AddComponent<CircleCollider2D>(7.0f, 1.0f, 0.0f, 1.0f);
		Ball* ball = entity->AddComponent<Ball>();
		scene->Add(entity);
		ball->Launch(glm::vec2(side * 0.65f, 1.0f));
	}
}

void GameRules::HandleDebugLevelKeys()
{
#ifndef LN_SHIPPING
	if (!Application::IsEditor() || !Input::GetKeyPress(KeyCode::Shift))
		return;

	if (Input::GetKeyTap(KeyCode::Period) && mLevel < kFinalLevel)
		ScreenTransition::LoadScene(LevelScene(mLevel + 1));
	else if (Input::GetKeyTap(KeyCode::Comma) && mLevel > 1)
		ScreenTransition::LoadScene(LevelScene(mLevel - 1));
#endif
}

int32 GameRules::ActiveLevel()
{
	const std::string& path = SceneManager::GetActivePath();
	const size_t marker = path.find("Level");

	if (marker == std::string::npos || marker + 7 > path.size())
		return 0;

	try
	{
		return std::clamp(std::stoi(path.substr(marker + 5, 2)), 1, kFinalLevel);
	}
	catch (const std::exception&)
	{
		return 0;
	}
}

std::string GameRules::LevelScene(int32 level)
{
	return LION_FORMAT_TEXT("Scenes/Level{:02}.lnscene", std::clamp(level, 1, kFinalLevel));
}

LION_REGISTER_COMPONENT(GameRules)
