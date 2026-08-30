#pragma once

#include <Lion/Lion.h>

class Ball;
class Paddle;

// Runs the round: observes the scene-authored bricks, decides when it is won or lost, and resets it.
//
// It owns none of the entities it makes — the scene does. It finds the ball the same way anything
// here finds anything: by asking the scene for the trait, never for the object.
class BrickField : public Lion::Component
{
public:
	void OnAwake() override;
	void OnUpdate() override;
	void Reflect(Lion::Reflector& reflector) override;

private:
	enum class State
	{
		Playing,
		Won,
		Lost,
	};

	State mState = State::Playing;
	Ball* mBall = nullptr;
	Paddle* mPaddle = nullptr;
	Lion::int32 mLevel = 1;
	std::string mDifficulty = "Very Easy";
	std::string mPreviousScene;
	std::string mNextScene;
	Lion::float32 mLoseHeight = -310.0f;

	void CheckWinLose();
	void EndRound(State state);
	void Restart();
	void HandleDebugLevelKeys();
};
