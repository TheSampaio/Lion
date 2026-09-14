#include "GameRules.h"
#include "Ball.h"
#include "Brick.h"
#include "Paddle.h"

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
	mScoreText = scoreEntity ? scoreEntity->GetComponent<TextRenderer>() : nullptr;
	mAttemptsText = attemptsEntity ? attemptsEntity->GetComponent<TextRenderer>() : nullptr;
	mLevelText = levelEntity ? levelEntity->GetComponent<TextRenderer>() : nullptr;
	mPowerText = powerEntity ? powerEntity->GetComponent<TextRenderer>() : nullptr;

	if (powerEntity)
	{
		powerEntity->SetVisible(false);
		powerEntity->SetEnabled(false);
	}
	UpdateHud();

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
	SceneManager::LoadScene(LevelScene(1));
}

void GameRules::RegisterBrickDamage(const Vector2& position, bool destroyed, const std::string& power)
{
	if (!sSessionActive)
		return;

	if (destroyed)
		sScore += kBrickScore;

	if (!sActiveRules)
		return;

	sActiveRules->mShakeRemaining = sActiveRules->mShakeDuration;
	sActiveRules->mShakeFrame = 0;
	if (sActiveRules->mImpactParticles)
		sActiveRules->mImpactParticles->EmitAt(position, destroyed ? 30 : 14);

	if (destroyed && !power.empty())
		sActiveRules->ActivatePower(power, position);

	sActiveRules->UpdateHud();
}

void GameRules::UpdateHud()
{
	if (mScoreText)
		mScoreText->SetText(LION_FORMAT_TEXT("SCORE {:06}", sScore));

	if (mAttemptsText)
		mAttemptsText->SetText(LION_FORMAT_TEXT("BALLS {}", sAttempts));

	if (mLevelText && mLevel > 0)
		mLevelText->SetText(LION_FORMAT_TEXT("LEVEL {:02}", mLevel));
}

void GameRules::UpdateShake()
{
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

void GameRules::HandleLevelFlow()
{
	const Reference<Scene> scene = GetOwner().GetScene();

	if (scene->CountActiveComponents<Brick>() == 0)
	{
		mTransitionQueued = true;

		if (mLevel < kFinalLevel)
			SceneManager::LoadScene(LevelScene(mLevel + 1));
		else
		{
			sSessionActive = false;
			SceneManager::LoadScene("Scenes/Victory.lnscene");
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

		mTransitionQueued = true;
		sAttempts = std::max(sAttempts - 1, 0);
		UpdateHud();

		if (sAttempts > 0)
			SceneManager::ReloadScene();
		else
		{
			sSessionActive = false;
			SceneManager::LoadScene("Scenes/Defeat.lnscene");
		}
	}
}

void GameRules::ActivatePower(const std::string& power, const Vector2& position)
{
	std::string message;

	if (power == "Extra Life")
	{
		sAttempts++;
		message = "EXTRA BALL +1";
	}
	else if (power == "Multiball")
	{
		SpawnExtraBalls();
		message = "MULTIBALL x3";
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

void GameRules::SpawnExtraBalls()
{
	const Reference<Scene> scene = GetOwner().GetScene();
	Ball* sourceBall = nullptr;

	for (const Reference<Entity>& entity : scene->GetEntities())
	{
		if (entity->IsActive() && entity->HasComponent<Ball>())
		{
			sourceBall = entity->GetComponent<Ball>();
			break;
		}
	}

	if (!sourceBall)
		return;

	const Vector2 origin = sourceBall->GetOwner().GetWorldPosition();

	for (int32 index = 0; index < 2; ++index)
	{
		const float32 side = index == 0 ? -1.0f : 1.0f;
		Reference<Entity> entity = MakeReference<Entity>();
		entity->SetName(LION_FORMAT_TEXT("Power Ball {}", index + 1));
		entity->GetTransform()->SetPosition(Vector2(origin.x + side * 10.0f, origin.y));
		entity->GetTransform()->SetScale(Vector2(0.375f, 0.375f));
		SpriteRenderer* renderer = entity->AddComponent<SpriteRenderer>("Sprites/Brickout/ball.png");
		renderer->SetOrder(20);
		entity->AddComponent<RigidBody2D>(BodyType::Dynamic, true);
		entity->AddComponent<CircleCollider2D>(16.0f, 1.0f, 0.0f, 1.0f);
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
		SceneManager::LoadScene(LevelScene(mLevel + 1));
	else if (Input::GetKeyTap(KeyCode::Comma) && mLevel > 1)
		SceneManager::LoadScene(LevelScene(mLevel - 1));
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
