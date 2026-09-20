#include "GameRules.h"
#include "Ball.h"
#include "Brick.h"
#include "GameAudio.h"
#include "GameProgress.h"
#include "GameSettings.h"
#include "Paddle.h"
#include "ScreenTransition.h"

#include <Lion/Logic/ComponentRegistry.h>
#include <Lion/Logic/Reflector.h>
#include <Lion/Render/Sprite.h>

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
	const Reference<Entity> powerTimer = scene->FindEntity("Power Timer");
	const Reference<Entity> powerTimerIcon = scene->FindEntity("Power Timer Icon");
	const Reference<Entity> powerTimerRing = scene->FindEntity("Power Timer Ring");
	const Reference<Entity> powerTimerText = scene->FindEntity("Power Timer Text");
	const Reference<Entity> controllerPrompts = scene->FindEntity("HUD Controller Prompts");
	const Reference<Entity> keyboardPrompts = scene->FindEntity("HUD Keyboard Prompts");
	const Reference<Entity> overdriveParticles = scene->FindEntity("Shockwave Overdrive Particles");
	mScoreText = scoreEntity ? scoreEntity->GetComponent<TextRenderer>() : nullptr;
	mAttemptsText = attemptsEntity ? attemptsEntity->GetComponent<TextRenderer>() : nullptr;
	mLevelText = levelEntity ? levelEntity->GetComponent<TextRenderer>() : nullptr;
	mComboText = comboEntity ? comboEntity->GetComponent<TextRenderer>() : nullptr;
	mShockwaveText = shockwaveEntity ? shockwaveEntity->GetComponent<TextRenderer>() : nullptr;
	mPowerText = powerEntity ? powerEntity->GetComponent<TextRenderer>() : nullptr;
	mPowerTimer = powerTimer.get();
	mPowerTimerIcon = powerTimerIcon ? powerTimerIcon->GetComponent<SpriteRenderer>() : nullptr;
	mPowerTimerRing = powerTimerRing ? powerTimerRing->GetComponent<SpriteRenderer>() : nullptr;
	mPowerTimerText = powerTimerText ? powerTimerText->GetComponent<TextRenderer>() : nullptr;
	mControllerPrompts = controllerPrompts.get();
	mKeyboardPrompts = keyboardPrompts.get();
	mOverdriveParticles = overdriveParticles ? overdriveParticles->GetComponent<ParticleComponent>() : nullptr;

	if (powerEntity)
	{
		powerEntity->SetVisible(false);
		powerEntity->SetEnabled(false);
	}
	if (mPowerTimer)
	{
		mPowerTimer->SetVisible(false);
		mPowerTimer->SetEnabled(false);
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
	mBackground = scene->FindEntity("Background").get();
	if (mBackground)
	{
		mBackgroundBasePosition = mBackground->GetTransform()->GetPosition();
		mBackgroundBaseScale = mBackground->GetTransform()->GetScale();
	}

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

	GameAudio::EnsureMusic(mLevel, mOverdriveRemaining > 0.0f);
	HandleDebugReset();
	if (mTransitionQueued)
		return;

	UpdateShake();
	UpdateInputPrompts();
	UpdateTemporaryPowers();
	UpdatePowerDrops();
	UpdateTransientEffects();
	UpdateAmbientMotion();
	UpdateAchievementNotifications();
	if (mPendingMultiball > 0 && mBall && mBall->IsLaunched())
	{
		mPendingMultiball--;
		SpawnExtraBalls(*mBall);
		ShowPowerMessage(GameSettings::Text(GameText::Multiball));
	}

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
	if (const Reference<Scene> scene = GetOwner().GetScene())
		for (const Reference<Entity>& entity : scene->GetEntities())
			if (Ball* ball = entity->GetComponent<Ball>())
				ball->SetPiercing(false);
	for (PowerDrop& drop : mPowerDrops)
		if (drop.entity) drop.entity->RemoveFromScene();
	for (TransientEffect& effect : mTransientEffects)
		if (effect.entity) effect.entity->RemoveFromScene();

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
		const int32 multiplier = sCombo * (sActiveRules && sActiveRules->mOverdriveRemaining > 0.0f ? 2 : 1);
		const int32 points = kBrickScore * multiplier;
		sScore += points;
		sLevelScore += points;
		GameProgress::RegisterBrickDestroyed();
		GameProgress::RegisterCombo(sCombo);
	}

	if (!sActiveRules)
		return;
	if (sActiveRules->mApplyingAreaDamage)
		return;

	if (GameSettings::HasCameraShake())
	{
		sActiveRules->mShakeRemaining = sActiveRules->mShakeDuration;
		sActiveRules->mActiveShakeStrength = sActiveRules->mShakeStrength;
		sActiveRules->mShakeFrame = 0;
	}
	if (sActiveRules->mImpactParticles)
		sActiveRules->mImpactParticles->EmitAt(position, destroyed ? 64 : 28);

	if (destroyed && power == "Bomb")
		sActiveRules->ActivatePower(power, position);
	else if (destroyed && !power.empty())
		sActiveRules->SpawnPowerDrop(power, position);

	sActiveRules->UpdateHud();
}

void GameRules::UpdateHud()
{
	if (mScoreText)
		mScoreText->SetText(LION_FORMAT_TEXT("{} {:06}", GameSettings::Text(GameText::Score), sScore));

	if (mAttemptsText)
		mAttemptsText->SetText(LION_FORMAT_TEXT("{} {}", GameSettings::Text(GameText::Balls), sAttempts));

	if (mLevelText && mLevel > 0)
		mLevelText->SetText(LION_FORMAT_TEXT("{} {:03}", GameSettings::Text(GameText::Level), mLevel));

	if (mComboText)
	{
		const bool overdrive = mOverdriveRemaining > 0.0f;
		const int32 multiplier = sCombo * (overdrive ? 2 : 1);
		mComboText->SetText(sCombo >= 2 || overdrive
			? LION_FORMAT_TEXT("{} X{}{}", GameSettings::Text(GameText::Combo), multiplier,
				overdrive ? "  OVERDRIVE" : "")
			: std::string());
		mComboText->GetOwner().SetVisible(sCombo >= 2 || overdrive);
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
		mCameraBaseOffset.x + direction.x * mActiveShakeStrength,
		mCameraBaseOffset.y + direction.y * mActiveShakeStrength));
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

	if (mPiercingBallRemaining > 0.0f)
	{
		mPiercingBallRemaining = std::max(0.0f, mPiercingBallRemaining - deltaTime);
		if (mPiercingBallRemaining <= 0.0f)
		{
			const Reference<Scene> scene = GetOwner().GetScene();
			for (const Reference<Entity>& entity : scene->GetEntities())
				if (Ball* ball = entity->GetComponent<Ball>())
					ball->SetPiercing(false);
		}
	}

	if (mOverdriveRemaining > 0.0f)
	{
		mOverdriveRemaining = std::max(0.0f, mOverdriveRemaining - deltaTime);
		mOverdriveParticleDelay -= deltaTime;
		if (mOverdriveParticles && mOverdriveParticleDelay <= 0.0f)
		{
			mOverdriveParticleDelay = 0.08f;
			const Reference<Scene> scene = GetOwner().GetScene();
			for (const Reference<Entity>& entity : scene->GetEntities())
				if (Ball* ball = entity->GetComponent<Ball>(); ball && entity->IsActive())
					mOverdriveParticles->EmitAt(entity->GetWorldPosition(), 5);
		}
		if (mOverdriveRemaining <= 0.0f)
		{
			GameAudio::EnsureMusic(mLevel);
			UpdateHud();
		}
	}

	const float32 remaining = std::max({ mWidePaddleRemaining, mPiercingBallRemaining,
		mOverdriveRemaining });
	if (mPowerTimer)
	{
		const bool active = remaining > 0.0f;
		mPowerTimer->SetVisible(active);
		mPowerTimer->SetEnabled(active);
		if (active && mPowerTimerIcon)
		{
			if (mOverdriveRemaining >= mWidePaddleRemaining
				&& mOverdriveRemaining >= mPiercingBallRemaining)
				mPowerTimerIcon->SetTexturePath("Sprites/Brickout/power-shockwave.png");
			else if (mPiercingBallRemaining >= mWidePaddleRemaining)
				mPowerTimerIcon->SetTexturePath("Sprites/Brickout/power-piercing.png");
			else
				mPowerTimerIcon->SetTexturePath("Sprites/Brickout/power-wide.png");
		}
		if (active && mPowerTimerRing)
		{
			const int32 frame = std::clamp(static_cast<int32>(std::ceil(remaining / 12.0f * 11.0f)), 0, 11);
			mPowerTimerRing->SetTexturePath(LION_FORMAT_TEXT("Sprites/Brickout/power-timer-{:02}.png", frame));
		}
		if (active && mPowerTimerText)
			mPowerTimerText->SetText(LION_FORMAT_TEXT("{:.1f}s", remaining));
	}
}

void GameRules::UpdatePowerDrops()
{
	if (!mPaddle)
		return;

	const float32 deltaTime = Clock::GetDeltaTime();
	const Vector2 paddlePosition = mPaddle->GetOwner().GetWorldPosition();
	for (auto drop = mPowerDrops.begin(); drop != mPowerDrops.end();)
	{
		if (!drop->entity || !drop->entity->IsActive())
		{
			drop = mPowerDrops.erase(drop);
			continue;
		}

		drop->phase += deltaTime;
		Vector2 position = drop->entity->GetWorldPosition();
		position.y -= 78.0f * deltaTime;
		position.x += std::sin(drop->phase * 3.2f) * 9.0f * deltaTime;
		drop->entity->SetWorldPosition(position);
		drop->entity->GetTransform()->SetRotation(std::sin(drop->phase * 2.4f) * 7.0f);

		const bool collected = std::abs(position.x - paddlePosition.x) <= mPaddle->GetHalfWidth() + 24.0f
			&& std::abs(position.y - paddlePosition.y) <= mPaddle->GetHalfHeight() + 26.0f;
		if (collected)
		{
			const std::string power = drop->power;
			drop->entity->RemoveFromScene();
			drop = mPowerDrops.erase(drop);
			ActivatePower(power, position);
			continue;
		}
		if (position.y < mLoseHeight - 40.0f)
		{
			drop->entity->RemoveFromScene();
			drop = mPowerDrops.erase(drop);
			continue;
		}
		++drop;
	}
}

void GameRules::UpdateTransientEffects()
{
	const float32 deltaTime = Clock::GetDeltaTime();
	for (auto effect = mTransientEffects.begin(); effect != mTransientEffects.end();)
	{
		effect->age += deltaTime;
		if (!effect->entity || effect->age >= effect->lifetime)
		{
			if (effect->entity) effect->entity->RemoveFromScene();
			effect = mTransientEffects.erase(effect);
			continue;
		}

		const float32 progress = effect->age / effect->lifetime;
		const float32 scale = 0.42f + progress * 1.25f;
		effect->entity->SetWorldScale(Vector2(scale, scale));
		if (effect->renderer)
		{
			const float32 intensity = 1.0f - progress * 0.82f;
			effect->renderer->GetSprite().SetColor(Vector(intensity, intensity, intensity));
		}
		++effect;
	}
}

void GameRules::UpdateAmbientMotion()
{
	if (!mBackground)
		return;

	mAmbientTime += Clock::GetDeltaTime();
	mBackground->GetTransform()->SetPosition(Vector2(
		mBackgroundBasePosition.x + std::sin(mAmbientTime * 0.17f) * 2.4f,
		mBackgroundBasePosition.y + std::cos(mAmbientTime * 0.13f) * 1.8f));
	const bool overdrive = mOverdriveRemaining > 0.0f;
	const float32 pulse = 1.01f + std::sin(mAmbientTime * (overdrive ? 2.8f : 0.21f))
		* (overdrive ? 0.018f : 0.008f);
	mBackground->GetTransform()->SetScale(Vector2(
		mBackgroundBaseScale.x * pulse, mBackgroundBaseScale.y * pulse));
}

void GameRules::UpdateAchievementNotifications()
{
	if (mPowerMessageRemaining > 0.0f)
		return;

	const int32 achievementIndex = GameProgress::ConsumeRecentlyUnlockedAchievement();
	if (achievementIndex < 0)
		return;

	const GameProgress::Achievement& achievement = GameProgress::GetAchievement(achievementIndex);
	ShowPowerMessage(LION_FORMAT_TEXT("ACHIEVEMENT UNLOCKED\n{}", achievement.title));
	mPowerMessageRemaining = 3.2f;
	GameAudio::PlaySfx("Sounds/achievement.wav", 0.72f);
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
		GameAudio::PlaySfx(sAttempts > 0 ? "Sounds/ball-lost.wav" : "Sounds/game-over.wav",
			sAttempts > 0 ? 0.62f : 0.84f);
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

void GameRules::SpawnPowerDrop(const std::string& power, const Vector2& position)
{
	const Reference<Scene> scene = GetOwner().GetScene();
	if (!scene)
		return;

	std::string texture;
	if (power == "Extra Life") texture = "Sprites/Brickout/power-life.png";
	else if (power == "Multiball") texture = "Sprites/Brickout/power-multiball.png";
	else if (power == "Bomb") texture = "Sprites/Brickout/power-bomb.png";
	else if (power == "Wide Paddle") texture = "Sprites/Brickout/power-wide.png";
	else if (power == "Piercing Ball") texture = "Sprites/Brickout/power-piercing.png";
	else return;

	Reference<Entity> entity = MakeReference<Entity>();
	entity->SetName(LION_FORMAT_TEXT("{} Drop", power));
	entity->GetTransform()->SetPosition(position);
	entity->GetTransform()->SetScale(Vector2(0.72f, 0.72f));
	SpriteRenderer* renderer = entity->AddComponent<SpriteRenderer>(texture);
	renderer->SetOrder(35);
	scene->Add(entity);
	mPowerDrops.push_back({ entity, power, static_cast<float32>(mPowerDrops.size()) * 0.7f });
	GameAudio::PlaySfx("Sounds/power-drop.wav", 0.44f);
	if (mImpactParticles)
		mImpactParticles->EmitAt(position, 84);
}

void GameRules::ActivatePower(const std::string& power, const Vector2& position)
{
	std::string message;

	if (power == "Extra Life")
	{
		sAttempts++;
		message = GameSettings::Text(GameText::ExtraBall);
	}
	else if (power == "Multiball")
	{
		Ball* sourceBall = nullptr;
		const Reference<Scene> scene = GetOwner().GetScene();
		for (const Reference<Entity>& entity : scene->GetEntities())
			if (Ball* candidate = entity->GetComponent<Ball>(); candidate && candidate->IsLaunched()
				&& entity->IsActive())
			{
				sourceBall = candidate;
				break;
			}
		if (sourceBall)
			SpawnExtraBalls(*sourceBall);
		else
			mPendingMultiball++;
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
	else if (power == "Piercing Ball")
	{
		ActivatePiercingBall();
		message = GameSettings::Text(GameText::PiercingBall);
	}
	else
		return;

	GameProgress::RegisterPowerCollected();
	GameAudio::PlayPower(power);

	if (mImpactParticles)
		mImpactParticles->EmitAt(position, 110);

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
		ball->SetPiercing(mPiercingBallRemaining > 0.0f);
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

	constexpr float32 kBlastRadius = 220.0f;
	mApplyingAreaDamage = true;
	for (const Reference<Entity>& entity : scene->GetEntities())
	{
		Brick* brick = entity->GetComponent<Brick>();
		if (!brick || !entity->IsActive())
			continue;

		const Vector2 offset = entity->GetWorldPosition() - origin;
		if (offset.x * offset.x + offset.y * offset.y <= kBlastRadius * kBlastRadius)
			brick->Damage(99, nullptr, false);
	}
	mApplyingAreaDamage = false;
	for (int32 ring = 0; ring < 3; ++ring)
	{
		Reference<Entity> effect = MakeReference<Entity>();
		effect->SetName(LION_FORMAT_TEXT("Bomb Blast {}", ring + 1));
		effect->GetTransform()->SetPosition(origin);
		effect->GetTransform()->SetScale(Vector2(0.5f, 0.5f));
		SpriteRenderer* renderer = effect->AddComponent<SpriteRenderer>("Sprites/Brickout/power-bomb.png");
		renderer->SetOrder(44);
		scene->Add(effect);
		mTransientEffects.push_back({ effect, renderer, ring * -0.055f, 0.46f + ring * 0.04f });
	}

	if (mImpactParticles)
		mImpactParticles->EmitAt(origin, 280);
	mShakeRemaining = 0.14f;
	mActiveShakeStrength = 1.25f;
	mShakeFrame = 0;
}

void GameRules::ActivateWidePaddle()
{
	mWidePaddleRemaining = 12.0f;
	if (mPaddle)
		mPaddle->SetWide(true);
}

void GameRules::ActivatePiercingBall()
{
	mPiercingBallRemaining = 12.0f;
	const Reference<Scene> scene = GetOwner().GetScene();
	if (!scene)
		return;
	for (const Reference<Entity>& entity : scene->GetEntities())
		if (Ball* ball = entity->GetComponent<Ball>(); ball && entity->IsActive())
			ball->SetPiercing(true);
}

void GameRules::ActivateShockwave()
{
	if (!mPaddle || sShockwaveCharge < kShockwaveHitsRequired)
		return;

	const Vector2 origin = mPaddle->GetOwner().GetWorldPosition();
	const Reference<Scene> scene = GetOwner().GetScene();
	if (!scene)
		return;

	mOverdriveRemaining = 12.0f;
	mOverdriveParticleDelay = 0.0f;
	GameAudio::EnsureMusic(mLevel, true);
	UpdateHud();

	mApplyingAreaDamage = true;
	for (const Reference<Entity>& entity : scene->GetEntities())
	{
		Brick* brick = entity->GetComponent<Brick>();
		if (!brick || !entity->IsActive())
			continue;

		brick->Damage(1, nullptr, false);
	}
	mApplyingAreaDamage = false;

	std::vector<Vector2> ballPositions;
	ballPositions.reserve(4);
	for (const Reference<Entity>& entity : scene->GetEntities())
		if (Ball* ball = entity->GetComponent<Ball>(); ball && entity->IsActive())
			ballPositions.push_back(entity->GetWorldPosition());
	for (const Vector2& ballPosition : ballPositions)
	{
		if (mOverdriveParticles)
			mOverdriveParticles->EmitAt(ballPosition, 96);
		SpawnShockwaveEffect(ballPosition);
	}
	mShakeRemaining = 0.16f;
	mActiveShakeStrength = 1.45f;
	mShakeFrame = 0;
	GameAudio::PlaySfx("Sounds/power-shockwave.wav", 0.88f);

	sShockwaveCharge = 0;
	GameProgress::RegisterShockwave();
	ShowPowerMessage(GameSettings::Text(GameText::ShockwaveFired));
	UpdateHud();
}

void GameRules::SpawnShockwaveEffect(const Vector2& origin)
{
	const Reference<Scene> scene = GetOwner().GetScene();
	if (!scene)
		return;

	for (int32 index = 0; index < 5; ++index)
	{
		Reference<Entity> entity = MakeReference<Entity>();
		entity->SetName(LION_FORMAT_TEXT("Shockwave Effect {}", index + 1));
		entity->GetTransform()->SetPosition(Vector2(origin.x, origin.y + 45.0f + index * 76.0f));
		entity->GetTransform()->SetScale(Vector2(0.42f, 0.42f));
		SpriteRenderer* renderer = entity->AddComponent<SpriteRenderer>("Sprites/Brickout/power-shockwave.png");
		renderer->SetOrder(45);
		scene->Add(entity);
		mTransientEffects.push_back({ entity, renderer, index * -0.035f, 0.38f + index * 0.025f });
	}
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

void GameRules::HandleDebugReset()
{
#ifndef LN_SHIPPING
	if (!Input::GetKeyPress(KeyCode::Shift) || !Input::GetKeyTap(KeyCode::Delete))
		return;

	GameProgress::ResetAll();
	sScore = 0;
	sAttempts = kStartingAttempts;
	sCombo = 1;
	sShockwaveCharge = 0;
	sLevelScore = 0;
	sSessionSeconds = 0.0f;
	sSessionActive = false;
	mTransitionQueued = true;
	ScreenTransition::LoadScene("Scenes/MainMenu.lnscene");
#endif
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

	if (marker == std::string::npos || marker + 6 > path.size())
		return 0;

	try
	{
		return std::clamp(std::stoi(path.substr(marker + 5)), 1, kFinalLevel);
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
