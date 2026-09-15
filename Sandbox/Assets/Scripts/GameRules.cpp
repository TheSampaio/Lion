#include "GameRules.h"
#include "Ball.h"
#include "Brick.h"
#include "GameProgress.h"
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
	const Reference<Entity> comboEntity = scene->FindEntity("Combo Text");
	const Reference<Entity> shockwaveEntity = scene->FindEntity("Shockwave Text");
	const Reference<Entity> powerEntity = scene->FindEntity("Power Text");
	const Reference<Entity> controllerPrompts = scene->FindEntity("HUD Controller Prompts");
	const Reference<Entity> keyboardPrompts = scene->FindEntity("HUD Keyboard Prompts");
	mScoreText = scoreEntity ? scoreEntity->GetComponent<TextRenderer>() : nullptr;
	mAttemptsText = attemptsEntity ? attemptsEntity->GetComponent<TextRenderer>() : nullptr;
	mLevelText = levelEntity ? levelEntity->GetComponent<TextRenderer>() : nullptr;
	mComboText = comboEntity ? comboEntity->GetComponent<TextRenderer>() : nullptr;
	mShockwaveText = shockwaveEntity ? shockwaveEntity->GetComponent<TextRenderer>() : nullptr;
	mPowerText = powerEntity ? powerEntity->GetComponent<TextRenderer>() : nullptr;
	mControllerPrompts = controllerPrompts.get();
	mKeyboardPrompts = keyboardPrompts.get();

	if (powerEntity)
	{
		powerEntity->SetVisible(false);
		powerEntity->SetEnabled(false);
	}
	UpdateHud();
	UpdateInputPrompts();

	if (mLevel == 0)
		return;

	if (!sSessionActive)
	{
		sScore = 0;
		sAttempts = kStartingAttempts;
		sCombo = 1;
		sShockwaveCharge = 0;
		sLevelScore = 0;
		sSessionSeconds = 0.0f;
		sSessionActive = true;
		GameProgress::StartSession();
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
	UpdateInputPrompts();
	UpdateTemporaryPowers();

	if (mLevel > 0 && sSessionActive)
	{
		sSessionSeconds += Clock::GetDeltaTime();
		if (sShockwaveCharge >= kShockwaveHitsRequired && Input::GetActionTap("player_power"))
			ActivateShockwave();
	}

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
	if (mPaddle)
		mPaddle->SetWide(false);

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
	StartAtLevel(1);
}

void GameRules::ContinueGame()
{
	StartAtLevel(GameProgress::GetHighestUnlockedLevel());
}

void GameRules::StartAtLevel(int32 level)
{
	GameProgress::Load();
	if (!GameProgress::IsLevelUnlocked(level))
		return;

	sScore = 0;
	sAttempts = kStartingAttempts;
	sCombo = 1;
	sShockwaveCharge = 0;
	sLevelScore = 0;
	sSessionSeconds = 0.0f;
	sSessionActive = true;
	GameProgress::StartSession();
	ScreenTransition::LoadScene(LevelScene(level));
}

void GameRules::AbandonSession()
{
	if (!sSessionActive)
		return;

	GameProgress::EndAttempt(ActiveLevel(), sLevelScore, sSessionSeconds);
	sLevelScore = 0;
	sSessionSeconds = 0.0f;
	sSessionActive = false;
}

void GameRules::RegisterBrickDamage(const Vector2& position, bool destroyed, const std::string& power,
	Ball* sourceBall)
{
	if (!sSessionActive)
		return;

	if (sourceBall)
		sShockwaveCharge = std::min(sShockwaveCharge + 1, kShockwaveHitsRequired);

	if (destroyed)
	{
		sCombo = std::min(sCombo + 1, 16);
		const int32 points = kBrickScore * sCombo;
		sScore += points;
		sLevelScore += points;
		GameProgress::RegisterBrickDestroyed();
		GameProgress::RegisterCombo(sCombo);
	}

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

	if (mComboText)
	{
		mComboText->SetText(sCombo >= 2
			? LION_FORMAT_TEXT("{} X{}", GameSettings::Text(GameText::Combo), sCombo)
			: std::string());
		mComboText->GetOwner().SetVisible(sCombo >= 2);
	}

	if (mShockwaveText)
	{
		mShockwaveText->SetText(sShockwaveCharge >= kShockwaveHitsRequired
			? GameSettings::Text(GameText::ShockwaveReady)
			: LION_FORMAT_TEXT("{} {}/{}", GameSettings::Text(GameText::Shockwave),
				sShockwaveCharge, kShockwaveHitsRequired));
	}
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

void GameRules::UpdateInputPrompts()
{
	const bool hints = GameSettings::HasControlHints();
	const bool gamepad = hints && Input::GetLastInputMethod() == InputMethod::Gamepad;
	const bool keyboard = hints && !gamepad;

	if (mControllerPrompts && gamepad != mShowingGamepadPrompts)
	{
		mShowingGamepadPrompts = gamepad;
		mControllerPrompts->SetVisible(gamepad);
		mControllerPrompts->SetEnabled(gamepad);
	}

	if (mKeyboardPrompts && keyboard != mShowingKeyboardPrompts)
	{
		mShowingKeyboardPrompts = keyboard;
		mKeyboardPrompts->SetVisible(keyboard);
		mKeyboardPrompts->SetEnabled(keyboard);
	}
}

void GameRules::UpdateTemporaryPowers()
{
	const float32 deltaTime = Clock::GetDeltaTime();
	if (mWidePaddleRemaining > 0.0f)
	{
		mWidePaddleRemaining = std::max(0.0f, mWidePaddleRemaining - deltaTime);
		if (mWidePaddleRemaining <= 0.0f && mPaddle)
			mPaddle->SetWide(false);
	}

	if (mDuplicatePaddleRemaining > 0.0f)
	{
		mDuplicatePaddleRemaining = std::max(0.0f, mDuplicatePaddleRemaining - deltaTime);
		if (mDuplicatePaddleRemaining <= 0.0f && mDuplicatePaddle)
		{
			mDuplicatePaddle->RemoveFromScene();
			mDuplicatePaddle = nullptr;
		}
	}
}

void GameRules::HandleLevelFlow()
{
	const Reference<Scene> scene = GetOwner().GetScene();

	if (scene->CountActiveComponents<Brick>() == 0)
	{
		mTransitionQueued = true;
		FinishAttempt(true);

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
		sCombo = 1;
		GameProgress::RegisterBallLost();
		UpdateHud();

		if (sAttempts > 0)
			RespawnBall();
		else
		{
			mTransitionQueued = true;
			FinishAttempt(false);
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
	else if (power == "Bomb")
	{
		ExplodeBomb(position);
		message = GameSettings::Text(GameText::Bomb);
	}
	else if (power == "Wide Paddle")
	{
		ActivateWidePaddle();
		message = GameSettings::Text(GameText::WidePaddle);
	}
	else if (power == "Duplicate Paddle")
	{
		ActivateDuplicatePaddle();
		message = GameSettings::Text(GameText::DuplicatePaddle);
	}
	else
		return;

	GameProgress::RegisterPowerCollected();

	if (mImpactParticles)
		mImpactParticles->EmitAt(position, 48);

	ShowPowerMessage(message);

	UpdateHud();
}

void GameRules::SpawnExtraBalls(Ball& sourceBall)
{
	const Reference<Scene> scene = GetOwner().GetScene();
	const Vector2 origin = sourceBall.GetOwner().GetWorldPosition();
	const glm::vec2 sourceDirection = sourceBall.GetDirection();

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
		const glm::vec2 direction = glm::normalize(glm::vec2(
			sourceDirection.x + side * 0.55f, std::max(std::abs(sourceDirection.y), 0.65f)));
		ball->LaunchFrom(origin, direction);
	}
}

void GameRules::ExplodeBomb(const Vector2& origin)
{
	const Reference<Scene> scene = GetOwner().GetScene();
	if (!scene)
		return;

	constexpr float32 kBlastRadius = 135.0f;
	for (const Reference<Entity>& entity : scene->GetEntities())
	{
		Brick* brick = entity->GetComponent<Brick>();
		if (!brick || !entity->IsActive())
			continue;

		const Vector2 offset = entity->GetWorldPosition() - origin;
		if (offset.x * offset.x + offset.y * offset.y <= kBlastRadius * kBlastRadius)
			brick->Damage(99, nullptr, false);
	}

	if (mImpactParticles)
		mImpactParticles->EmitAt(origin, 110);
}

void GameRules::ActivateWidePaddle()
{
	mWidePaddleRemaining = 12.0f;
	if (mPaddle)
		mPaddle->SetWide(true);
	if (mDuplicatePaddle)
		if (Paddle* paddle = mDuplicatePaddle->GetComponent<Paddle>())
			paddle->SetWide(true);
}

void GameRules::ActivateDuplicatePaddle()
{
	mDuplicatePaddleRemaining = 12.0f;
	if (mDuplicatePaddle || !mPaddle)
		return;

	const Reference<Scene> scene = GetOwner().GetScene();
	if (!scene)
		return;

	Reference<Entity> entity = MakeReference<Entity>();
	entity->SetName("Duplicate Paddle");
	const Vector2 source = mPaddle->GetOwner().GetWorldPosition();
	entity->GetTransform()->SetPosition(Vector2(source.x, source.y + 42.0f));
	SpriteRenderer* renderer = entity->AddComponent<SpriteRenderer>("Sprites/Brickout/player.png");
	renderer->SetOrder(11);
	entity->AddComponent<RigidBody2D>(BodyType::Kinematic, true);
	entity->AddComponent<BoxCollider2D>(100.0f, 20.0f, 1.0f, 0.0f, 1.0f);
	Paddle* duplicate = entity->AddComponent<Paddle>();
	scene->Add(entity);
	mDuplicatePaddle = entity.get();
	if (mWidePaddleRemaining > 0.0f)
		duplicate->SetWide(true);
}

void GameRules::ActivateShockwave()
{
	if (!mPaddle || sShockwaveCharge < kShockwaveHitsRequired)
		return;

	const Vector2 origin = mPaddle->GetOwner().GetWorldPosition();
	const Reference<Scene> scene = GetOwner().GetScene();
	if (!scene)
		return;

	for (const Reference<Entity>& entity : scene->GetEntities())
	{
		Brick* brick = entity->GetComponent<Brick>();
		if (!brick || !entity->IsActive())
			continue;

		const Vector2 offset = entity->GetWorldPosition() - origin;
		if (offset.y >= 0.0f && offset.y <= 470.0f
			&& std::abs(offset.x) <= 38.0f + offset.y * 0.58f)
			brick->Damage(1, nullptr, false);
	}

	if (mImpactParticles)
		for (int32 step = 1; step <= 6; ++step)
			mImpactParticles->EmitAt(Vector2(origin.x, origin.y + step * 62.0f), 24);

	sShockwaveCharge = 0;
	GameProgress::RegisterShockwave();
	ShowPowerMessage(GameSettings::Text(GameText::ShockwaveFired));
	UpdateHud();
}

void GameRules::ShowPowerMessage(const std::string& message)
{
	if (!mPowerText)
		return;

	mPowerText->SetText(message);
	mPowerText->GetOwner().SetVisible(true);
	mPowerText->GetOwner().SetEnabled(true);
	mPowerMessageRemaining = 1.4f;
}

void GameRules::FinishAttempt(bool completed)
{
	if (completed)
		GameProgress::CompleteLevel(mLevel, sLevelScore, sSessionSeconds);
	else
		GameProgress::EndAttempt(mLevel, sLevelScore, sSessionSeconds);

	sLevelScore = 0;
	sSessionSeconds = 0.0f;
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
