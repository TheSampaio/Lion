#pragma once

#include <Lion/Lion.h>

class Ball;
class Brick;
class Paddle;

// Owns the complete Brickout session: score, remaining balls, level progression and game feedback.
// Every scene links the same Assembly definition; static session values survive scene transitions.
class GameRules final : public Lion::Component
{
public:
	void OnAwake() override;
	void OnUpdate() override;
	void OnDestroy() override;
	void Reflect(Lion::Reflector& reflector) override;

	static void StartNewGame();
	static void ContinueGame();
	static void StartAtLevel(Lion::int32 level);
	static void AbandonSession();
	static void RegisterBrickDamage(const Lion::Vector2& position, bool destroyed,
		const std::string& power, Ball* sourceBall);
	static Lion::int32 GetScore() { return sScore; }
	static Lion::int32 GetAttempts() { return sAttempts; }

private:
	static constexpr Lion::int32 kStartingAttempts = 3;
	static constexpr Lion::int32 kBrickScore = 100;
	static constexpr Lion::int32 kFinalLevel = 100;
	static constexpr Lion::int32 kShockwaveHitsRequired = 8;

	static inline GameRules* sActiveRules = nullptr;
	static inline Lion::int32 sScore = 0;
	static inline Lion::int32 sAttempts = kStartingAttempts;
	static inline Lion::int32 sCombo = 1;
	static inline Lion::int32 sShockwaveCharge = 0;
	static inline Lion::int32 sLevelScore = 0;
	static inline Lion::float32 sSessionSeconds = 0.0f;
	static inline bool sSessionActive = false;

	Ball* mBall = nullptr;
	Paddle* mPaddle = nullptr;
	Lion::Camera2D* mCamera = nullptr;
	Lion::ParticleComponent* mImpactParticles = nullptr;
	Lion::TextRenderer* mScoreText = nullptr;
	Lion::TextRenderer* mAttemptsText = nullptr;
	Lion::TextRenderer* mLevelText = nullptr;
	Lion::TextRenderer* mComboText = nullptr;
	Lion::TextRenderer* mShockwaveText = nullptr;
	Lion::TextRenderer* mPowerText = nullptr;
	Lion::TextRenderer* mPowerTimerText = nullptr;
	Lion::SpriteRenderer* mPowerTimerIcon = nullptr;
	Lion::SpriteRenderer* mPowerTimerRing = nullptr;
	Lion::Entity* mPowerTimer = nullptr;
	Lion::Entity* mControllerPrompts = nullptr;
	Lion::Entity* mKeyboardPrompts = nullptr;
	Lion::Entity* mDuplicatePaddle = nullptr;
	Lion::Entity* mBackground = nullptr;
	Lion::Vector2 mCameraBaseOffset;
	Lion::Vector2 mBackgroundBasePosition;
	Lion::Vector2 mBackgroundBaseScale{ 1.0f, 1.0f };
	Lion::float32 mLoseHeight = -310.0f;
	Lion::float32 mShakeDuration = 0.06f;
	Lion::float32 mShakeStrength = 0.72f;
	Lion::float32 mShakeRemaining = 0.0f;
	Lion::float32 mActiveShakeStrength = 0.0f;
	Lion::float32 mPowerMessageRemaining = 0.0f;
	Lion::float32 mWidePaddleRemaining = 0.0f;
	Lion::float32 mDuplicatePaddleRemaining = 0.0f;
	Lion::int32 mShakeFrame = 0;
	Lion::int32 mLevel = 0;
	Lion::int32 mPendingMultiball = 0;
	Lion::float32 mAmbientTime = 0.0f;
	bool mInitialized = false;
	bool mTransitionQueued = false;
	bool mShowingGamepadPrompts = false;
	bool mShowingKeyboardPrompts = false;

	struct PowerDrop
	{
		Lion::Reference<Lion::Entity> entity;
		std::string power;
		Lion::float32 phase = 0.0f;
	};

	struct TransientEffect
	{
		Lion::Reference<Lion::Entity> entity;
		Lion::SpriteRenderer* renderer = nullptr;
		Lion::float32 age = 0.0f;
		Lion::float32 lifetime = 0.36f;
	};

	std::vector<PowerDrop> mPowerDrops;
	std::vector<TransientEffect> mTransientEffects;

	void InitializeForScene();
	void UpdateHud();
	void UpdateShake();
	void UpdateInputPrompts();
	void UpdateTemporaryPowers();
	void UpdatePowerDrops();
	void UpdateTransientEffects();
	void UpdateAmbientMotion();
	void UpdateAchievementNotifications();
	void HandleLevelFlow();
	void RespawnBall();
	void SpawnPowerDrop(const std::string& power, const Lion::Vector2& position);
	void ActivatePower(const std::string& power, const Lion::Vector2& position);
	void SpawnExtraBalls(Ball& sourceBall);
	void ExplodeBomb(const Lion::Vector2& origin);
	void ActivateWidePaddle();
	void ActivateDuplicatePaddle();
	void ActivateShockwave();
	void SpawnShockwaveEffect(const Lion::Vector2& origin);
	void ShowPowerMessage(const std::string& message);
	void FinishAttempt(bool completed);
	void HandleDebugReset();
	void HandleDebugLevelKeys();
	static Lion::int32 ActiveLevel();
	static std::string LevelScene(Lion::int32 level);
};
