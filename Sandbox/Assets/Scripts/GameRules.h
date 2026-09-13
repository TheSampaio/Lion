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
	static void RegisterBrickHit();
	static Lion::int32 GetScore() { return sScore; }
	static Lion::int32 GetAttempts() { return sAttempts; }

private:
	static constexpr Lion::int32 kStartingAttempts = 3;
	static constexpr Lion::int32 kBrickScore = 100;
	static constexpr Lion::int32 kFinalLevel = 5;

	static inline GameRules* sActiveRules = nullptr;
	static inline Lion::int32 sScore = 0;
	static inline Lion::int32 sAttempts = kStartingAttempts;
	static inline bool sSessionActive = false;

	Ball* mBall = nullptr;
	Paddle* mPaddle = nullptr;
	Lion::Camera2D* mCamera = nullptr;
	Lion::TextRenderer* mScoreText = nullptr;
	Lion::TextRenderer* mAttemptsText = nullptr;
	Lion::Vector2 mCameraBaseOffset;
	Lion::float32 mLoseHeight = -310.0f;
	Lion::float32 mShakeDuration = 0.07f;
	Lion::float32 mShakeStrength = 1.0f;
	Lion::float32 mShakeRemaining = 0.0f;
	Lion::int32 mShakeFrame = 0;
	Lion::int32 mLevel = 0;
	bool mInitialized = false;
	bool mTransitionQueued = false;

	void InitializeForScene();
	void UpdateHud();
	void UpdateShake();
	void HandleLevelFlow();
	void HandleDebugLevelKeys();
	static Lion::int32 ActiveLevel();
	static std::string LevelScene(Lion::int32 level);
};
