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
	mScoreText = scoreEntity ? scoreEntity->GetComponent<TextRenderer>() : nullptr;
	mAttemptsText = attemptsEntity ? attemptsEntity->GetComponent<TextRenderer>() : nullptr;
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

void GameRules::RegisterBrickHit(const Vector2& position)
{
	if (!sSessionActive)
		return;

	sScore += kBrickScore;

	if (!sActiveRules)
		return;

	sActiveRules->mShakeRemaining = sActiveRules->mShakeDuration;
	sActiveRules->mShakeFrame = 0;
	if (sActiveRules->mImpactParticles)
		sActiveRules->mImpactParticles->EmitAt(position, 12);
	sActiveRules->UpdateHud();
}

void GameRules::UpdateHud()
{
	if (mScoreText)
		mScoreText->SetText(LION_FORMAT_TEXT("SCORE {:06}", sScore));

	if (mAttemptsText)
		mAttemptsText->SetText(LION_FORMAT_TEXT("BALLS {}", sAttempts));
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
	else if (mBall && mBall->GetOwner().GetWorldPosition().y < mLoseHeight)
	{
		mTransitionQueued = true;
		mBall->Stop();
		mBall->SetVisible(false);
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
